#include <stdbool.h>
#include <unistd.h>

#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"

#include "sprites.h"
#include "ssd1306.h"

#define PIN_SDA 10
#define PIN_SCL 9
#define OLED_ADDR 0x3C
#define PIN_JUMP 4

#define BUTTON_USE_ISR 1
#define DEBOUNCE_US 15000

#define GROUND_Y 54
#define FRAME_US 33000
#define SCROLL_PX 2
#define LEG_SWAP_US 130000

#define GROUND_PERIOD 97
#define SKY_PERIOD 256
#define PARALLAX_DIV 4

#define MAX_OBS 3
#define GAP_MIN 55
#define GAP_MAX 95

#define SUB 8
#define JUMP_V 36
#define GRAVITY 4

#define DINO_X 8
#define DEATH_FLASH_US 250000
#define RESTART_LOCK_US 700000

typedef enum { ST_RUN, ST_DEAD } state_t;

static const char *TAG = "dino";

static state_t state;
static int world_x;
static int obs_x[MAX_OBS];
static int dino_h_sub;
static int dino_v_sub;
static int score;
static int best;
static int64_t dead_at;

static int rnd_range(int lo, int hi) {
    return lo + (int)(esp_random() % (uint32_t)(hi - lo + 1));
}

#if BUTTON_USE_ISR

static volatile bool s_pressed;
static volatile int64_t s_last_edge;

static void IRAM_ATTR jump_isr(void *arg) {
    int64_t now = esp_timer_get_time();
    if (now - s_last_edge < DEBOUNCE_US)
        return;
    s_last_edge = now;
    s_pressed = true;
}

static void button_init(void) {
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << PIN_JUMP,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&cfg);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_JUMP, jump_isr, NULL);
}

static bool jump_take(void) {
    if (!s_pressed)
        return false;
    s_pressed = false;
    return true;
}

#else

static void button_init(void) {
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << PIN_JUMP,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
}

static bool jump_take(void) {
    static int prev = 1;
    int now = gpio_get_level(PIN_JUMP);
    bool edge = (prev == 1 && now == 0);
    prev = now;
    return edge;
}

#endif

static void draw_ground(void) {
    fb_hline(0, GROUND_Y, OLED_W, true);

    static const uint8_t bump[][2] = {
        {3, 2}, {11, 5}, {24, 2}, {39, 4}, {51, 2}, {63, 5}, {78, 3}, {91, 2},
    };
    const int n = sizeof(bump) / sizeof(bump[0]);

    int off = world_x % GROUND_PERIOD;
    for (int base = -GROUND_PERIOD; base <= OLED_W; base += GROUND_PERIOD) {
        for (int i = 0; i < n; i++) {
            fb_hline(base + bump[i][0] - off, GROUND_Y + bump[i][1], 3, true);
        }
    }
}

static void draw_sky(void) {
    static const uint16_t star[][2] = {
        {10, 5}, {35, 12}, {60, 3}, {99, 9}, {118, 17}, {150, 7}, {178, 20}, {205, 4}, {230, 14}, {248, 9},
    };
    const int n = sizeof(star) / sizeof(star[0]);

    int off = (world_x / PARALLAX_DIV) % SKY_PERIOD;
    for (int i = 0; i < n; i++) {
        int sx = (int)star[i][0] - off;
        if (sx < -spr_star.w)
            sx += SKY_PERIOD;
        fb_sprite(&spr_star, sx, star[i][1]);
    }
}

static void obs_reset(void) {
    int x = OLED_W + 20;
    for (int i = 0; i < MAX_OBS; i++) {
        obs_x[i] = x;
        x += rnd_range(GAP_MIN, GAP_MAX);
    }
}

static void obs_update(void) {
    for (int i = 0; i < MAX_OBS; i++)
        obs_x[i] -= SCROLL_PX;

    int max = obs_x[0];
    for (int i = 1; i < MAX_OBS; i++)
        if (obs_x[i] > max)
            max = obs_x[i];

    for (int i = 0; i < MAX_OBS; i++) {
        if (obs_x[i] < -spr_cactus.w) {
            max += rnd_range(GAP_MIN, GAP_MAX);
            obs_x[i] = max;
        }
    }
}

static bool dino_on_ground(void) {
    return dino_h_sub == 0;
}

static int dino_top(void) {
    return GROUND_Y - spr_dino_a.h - dino_h_sub / SUB;
}

static void dino_update(bool want_jump) {
    if (want_jump && dino_on_ground()) {
        dino_v_sub = JUMP_V;
    }
    if (dino_on_ground() && dino_v_sub == 0)
        return;

    dino_h_sub += dino_v_sub;
    dino_v_sub -= GRAVITY;

    if (dino_h_sub <= 0) {
        dino_h_sub = 0;
        dino_v_sub = 0;
    }
}

static bool overlap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

static bool dino_hits_cactus(void) {
    int dx = DINO_X + 4, dw = spr_dino_a.w - 7;
    int dy = dino_top() + 2, dh = spr_dino_a.h - 4;

    for (int i = 0; i < MAX_OBS; i++) {
        int cx = obs_x[i] + 3, cw = 5;
        int cy = GROUND_Y - spr_cactus.h + 2, ch = spr_cactus.h - 2;
        if (overlap(dx, dy, dw, dh, cx, cy, cw, ch))
            return true;
    }
    return false;
}

static void draw_number(int value, int right_x, int y) {
    if (value < 0)
        value = 0;

    int digits[5], n = 0;
    do {
        digits[n++] = value % 10;
        value /= 10;
    } while (value && n < 5);

    int x = right_x - n * 4 + 1;
    for (int i = n - 1; i >= 0; i--) {
        fb_sprite(&spr_digit[digits[i]], x, y);
        x += 4;
    }
}

static void draw_hud(bool blink_on) {
    fb_rect_fill(96, 0, 32, 8, false);
    if (blink_on) {
        draw_number(score, 125, 2);
    }
}

static void render(int leg_frame, bool blink_on) {
    fb_clear();
    draw_sky();
    draw_ground();

    for (int i = 0; i < MAX_OBS; i++) {
        fb_sprite(&spr_cactus, obs_x[i], GROUND_Y - spr_cactus.h);
    }

    const sprite_t *dino = (state == ST_RUN && dino_on_ground() && leg_frame) ? &spr_dino_b : &spr_dino_a;
    fb_sprite(dino, DINO_X, dino_top());

    draw_hud(blink_on);
    oled_flush();
}

static void game_reset(void) {
    world_x = 0;
    dino_h_sub = 0;
    dino_v_sub = 0;
    score = 0;
    state = ST_RUN;
    obs_reset();
}

static void frame_wait(int64_t deadline_us) {
    int64_t left = deadline_us - esp_timer_get_time();
    if (left > 0)
        usleep((useconds_t)left);
}

void app_main(void) {
    oled_init(PIN_SDA, PIN_SCL, OLED_ADDR);
    button_init();
    game_reset();

    int64_t next_frame = esp_timer_get_time();
    int64_t leg_at = 0;
    int leg_frame = 0;

    while (1) {
        int64_t now = esp_timer_get_time();

        bool pressed = jump_take();

        if (state == ST_RUN) {
            world_x += SCROLL_PX;
            score = world_x / 20;
            obs_update();
            dino_update(pressed);

            if (now - leg_at >= LEG_SWAP_US) {
                leg_frame ^= 1;
                leg_at = now;
            }

            if (dino_hits_cactus()) {
                state = ST_DEAD;
                dead_at = now;
                if (score > best)
                    best = score;
                oled_invert(true);
                ESP_LOGI(TAG, "програш, рахунок %d (рекорд %d)", score, best);
            }
        } else {

            if (now - dead_at > DEATH_FLASH_US) {
                oled_invert(false);
            }
            if (pressed && now - dead_at > RESTART_LOCK_US) {
                game_reset();
            }
        }

        bool blink_on = (state == ST_RUN) || ((now / 300000) & 1);
        render(leg_frame, blink_on);

        next_frame += FRAME_US;
        if (next_frame < esp_timer_get_time()) {
            next_frame = esp_timer_get_time();
        }
        frame_wait(next_frame);
    }
}