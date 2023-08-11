#ifndef EMULATOR_H
#define EMULATOR_H

#include <stdbool.h>

#include "SDL2/SDL.h"
#include "config.h"
#include "cpu.h"

struct emulator_context
{
    bool quit;

    SDL_Window *window;
    SDL_Renderer *renderer;

    // Stores the RGB values for each pixel. hXw layout to reduce memory accesses since gameboy renders in column order.
    BYTE screen_data[SCREEN_HEIGHT][SCREEN_WIDTH][3];
};

void emulator_run(int argc, char **argv);

void emulator_disable_interupts();
void emulator_request_interrupts(BYTE interrupt_bit);
#endif