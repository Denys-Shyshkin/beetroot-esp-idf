#pragma once
#include <stdint.h>
#include <stdbool.h>


void fb_text(int x, int y, const char *s);
void fb_printf(int x, int y, const char *fmt, ...);