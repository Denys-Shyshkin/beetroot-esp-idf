#include "helpers.h" 

#define WINDOW 16

static int buffer[WINDOW] = {0};
static int running_sum = 0;
static int idx = 0;
static int count = 0;

int moving_average(int raw) {
    // Subtract the oldest reading from the running sum
    running_sum -= buffer[idx];

    // Put the newest reading to the buffer
    buffer[idx] = raw;

    // Add the newest reading to the running sum
    running_sum += raw;

    // Update current index
    idx = (idx + 1) % WINDOW;

    // Increase the buffer count as it grows up to max value
    if (count < WINDOW) {
        count++;
    }

    return (int)(running_sum / count);
}