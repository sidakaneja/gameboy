#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// Data types
typedef uint8_t BYTE;
typedef uint8_t WORD;

#define EMULATOR_WINDOW_TITLE "Gameboy Emulator"
#define GAMEBOY_FRAME_RATE 60

// Display
static const int GAMEBOY_PIXEL_MULTIPLIER = 5;
static const int GAMEBOY_SCREEN_WIDTH = 160;
static const int GAMEBOY_SCREEN_HEIGHT = 144;

#endif