#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "emulator.h"

struct graphics_context
{
    int scanline_counter;

    // Stores the RGB values for each pixel. hXw layout to reduce memory accesses since gameboy renders in column order.
    BYTE screen_data[SCREEN_HEIGHT][SCREEN_WIDTH][3];
};

void graphics_init();
void graphics_update(int cycles);
BYTE graphics_get_screen_data(int col, int row, int colour);
#endif