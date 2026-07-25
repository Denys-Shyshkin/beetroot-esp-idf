#pragma once
#include <stdint.h>

// Notes frequencies list
#define REST 0
#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_C5 523

// Notes duration list
#define WHOLE 800
#define HALF 400
#define QUARTER 200
#define EIGHTH 100

typedef struct {
    uint16_t freq;
    uint16_t duration;
} note_t;

#define NOTE_LEN sizeof(note_t)

void buzzer_init(uint8_t gpio_num);
void play_melody(const note_t melody[], uint8_t length);
