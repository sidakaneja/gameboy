#ifndef EMULATOR_H
#define EMULATOR_H

#include <stdbool.h>

#include "SDL2/SDL.h"
#include "config.h"
#include "cpu.h"

struct emulator_context
{
    bool quit;
    int timer_clocks_per_increment;
    // Both timer and divider store clocks cycles to / from incrementing their respective registers
    int timer;
    int divider;

    bool master_interupt;
    int disable_pending;
    int enable_pending;

    SDL_Window *window;
    SDL_Renderer *renderer;
};

void emulator_run(int argc, char **argv);

void emulator_disable_interupts();
void emulator_enable_interrupts();
void emulator_request_interrupts(BYTE interrupt_bit);
int emulator_get_clock_speed();
void emulator_set_clock_speed(int new_speed);
#endif