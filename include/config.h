#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// Data types
typedef uint8_t BYTE;
typedef uint8_t WORD;

#define EMULATOR_WINDOW_TITLE "Gameboy Emulator"
static const int FRAME_RATE = 60;

// Display
static const int PIXEL_MULTIPLIER = 5;
static const int SCREEN_WIDTH = 160;
static const int SCREEN_HEIGHT = 144;

static const int CPU_CLOCK_SPEED = 4194304;

#define FLAG_Z 7
#define FLAG_N 6
#define FLAG_H 5
#define FLAG_C 4
#endif