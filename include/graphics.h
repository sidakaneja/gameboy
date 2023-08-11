#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "emulator.h"

struct graphics_context
{
    int scanline_counter;
};

void graphics_init();
void graphics_update(int cycles);
#endif