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

#define LCD_CONTROL_ADDRESS 0xFF40
#define LCD_STATUS_ADDRESS 0xFF41

#define LCD_ENABLED_BIT 7
#define LCD_WINDOW_TILE_ID_LOCATION_BIT 6
#define LCD_WINDOW_ENABLED_BIT 5
#define LCD_TILE_VRAM_LOCATION_BIT 4
#define LCD_BG_TILE_ID_LOCATION_BIT 3
#define LCD_SPRITE_SIZE_BIT 2
#define LCD_SPRITES_ENABLED_BIT 1
#define LCD_BACKGROUND_ENABLED_BIT 0

typedef enum COLOUR
{
    WHITE,
    LIGHT_GRAY,
    DARK_GRAY,
    BLACK
} COLOUR;

// Interrupt bits
#define VBLANK_INTERRUPT 0
#define LCD_INTERRUPT 1
#define TIMER_INTERRUPT 2
#define JOYPAD_INTERRUPT 4

#define INTERRUPT_REGISTER_ADDRESS 0xFF0F

#define DMA_ADDRESS 0xFF46

// Timer Info
#define TIMA 0xFF05
#define TMA 0xFF06
#define TIMER_CONTROLLER_ADDRESS 0xFF07
#define DIVIDER_REGISTER_ADDRESS 0xFF04
#endif