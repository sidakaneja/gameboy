#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// Data types
typedef uint8_t BYTE;
typedef int8_t SIGNED_BYTE;
typedef uint16_t WORD;
typedef int16_t SIGNED_WORD;

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

// Graphics
#define SCANLINE_ADDRESS 0xFF44
#define NUM_SCANLINES 144
#define SCANLINE_CLOCK_CYCLES 456
#define LCD_STATUS_ADDRESS 0xFF41

// Interrupt bits
#define VBLANK_INTERRUPT 0
#define LCD_INTERRUPT 1
#define TIMER_INTERRUPT 2
#define JOYPAD_INTERRUPT 4

#define INTERRUPT_REGISTER_ADDRESS 0xFF0F

#endif