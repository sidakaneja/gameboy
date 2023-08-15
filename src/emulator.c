#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "emulator.h"
#include "em_memory.h"
#include "graphics.h"
#include "common.h"

static struct emulator_context _emulator;

static void _emulator_update_timers(int cycles);

// Initialize SDL Window and renderer, set background color to black
static bool _sdl_init()
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        printf("error initializing SDL: %s\n", SDL_GetError());
        return false;
    }

    // h and w scaled by PIXEL_MULTIPLIER so each pixel of gameboy = PIXEL_MULTIPLIER^2 of native display
    _emulator.window = SDL_CreateWindow(
        EMULATOR_WINDOW_TITLE,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH * PIXEL_MULTIPLIER, SCREEN_HEIGHT * PIXEL_MULTIPLIER, 0);

    // Set background to black
    _emulator.renderer = SDL_CreateRenderer(_emulator.window, -1, SDL_TEXTUREACCESS_TARGET);
    SDL_SetRenderDrawColor(_emulator.renderer, 255, 255, 255, 255);
    SDL_RenderClear(_emulator.renderer);

    return true;
}

static void _sdl_destroy()
{
    SDL_DestroyWindow(_emulator.window);
}

// Each frame, render the pixels
static void _sdl_render()
{
    for (int x = 0; x < SCREEN_WIDTH; x++)
    {
        for (int y = 0; y < SCREEN_HEIGHT; y++)
        {
            // Set RGB value for the display pixel
            SDL_SetRenderDrawColor(_emulator.renderer, graphics_get_screen_data(y, x, 0), graphics_get_screen_data(y, x, 1), graphics_get_screen_data(y, x, 2), 175);
            SDL_Rect r;
            r.x = x * PIXEL_MULTIPLIER;
            r.y = y * PIXEL_MULTIPLIER;
            r.w = PIXEL_MULTIPLIER;
            r.h = PIXEL_MULTIPLIER;
            SDL_RenderFillRect(_emulator.renderer, &r);
        }
    }
    SDL_RenderPresent(_emulator.renderer);
}

static void _sdl_poll_quit()
{
    // Need to poll events in loop, otherwise the window doesn't render on mac
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
        {
            _emulator.quit = true;
        }
    }
}

// Initialize the emulator context, setup sdl
static bool _emulator_init()
{
    memset(&_emulator, 0, sizeof(_emulator));
    _emulator.timer_clocks_per_increment = 1024;

    if (!_sdl_init())
    {
        return false;
    }

    graphics_init();
    return true;
}

static void _emulator_destroy()
{
    _sdl_destroy();
}

static void _emulator_update()
{
    _sdl_poll_quit();
    if (_emulator.quit)
    {
        return;
    }

    const int CYCLES_PER_FRAME = CPU_CLOCK_SPEED / FRAME_RATE;
    int cycles_this_update = 0;
    // Run CYCLES_PER_FRAME clock cycles before rendering to screen
    while (cycles_this_update < CYCLES_PER_FRAME)
    {
        // Replace with cycles for next opcode
        int cycles = cpu_next_execute_instruction();
        cycles_this_update += cycles;
        _emulator_update_timers(cycles);
        graphics_update(cycles);
    }
    _sdl_render();
}

// Main emulator loop
void emulator_run(int argc, char **argv)
{
    if (!_emulator_init())
    {
        printf("Could not initialize emulator\n");
        return;
    }

    // Temporary load rom function
    assert(argc > 1);
    BYTE *m_CartridgeMemory = (BYTE *)malloc(0x200000 * sizeof(BYTE));
    memset(m_CartridgeMemory, 0, (0x200000 * sizeof(BYTE)));

    FILE *in;
    in = fopen(argv[1], "rb");
    fread(m_CartridgeMemory, 1, 0x200000, in);
    fclose(in);

    memory_init(m_CartridgeMemory);
    cpu_intialize();

    // Infinite loop that runs until the user closes the window
    // Runs FRAME_RATE times a second
    while (!_emulator.quit)
    {
        const uint64_t ms_per_frame = 1000 / FRAME_RATE;
        uint64_t frame_start = SDL_GetTicks64();

        // Runs for one frame, that is, CYCLES_PER_FRAME clock cycles
        // _emulator_update is called FRAME_RATE times a second
        // After CYCLES_PER_FRAME clock cycles, renders screen
        _emulator_update();
        uint64_t frame_time = SDL_GetTicks64() - frame_start;

        if (frame_time < ms_per_frame)
        {
            uint32_t delay_for = ms_per_frame - frame_time;
            SDL_Delay(delay_for);
        }
    }

    _emulator_destroy();
}

void emulator_disable_interupts()
{
    // TODO
    printf("TODO disable interupts instruction 0XF3\n");
}

// Set the requested interrupt bit at the interrupt register
void emulator_request_interrupts(BYTE interrupt_bit)
{
    BYTE req = memory_read(INTERRUPT_REGISTER_ADDRESS);
    bit_set(&req, interrupt_bit);
    memory_write(INTERRUPT_REGISTER_ADDRESS, req);
}

static void _emulator_update_timers(int cycles)
{
    // Bit 2 is if timer is enabled

    // Bit 0 and 1 give the frequency of the timer
    // 00: 4096 Hz
    // 01: 262144 Hz
    // 10: 65536 Hz
    // 11: 16384 Hzu
    BYTE timer_controller = memory_direct_read(TIMER_CONTROLLER_ADDRESS);

    _emulator.divider += cycles;

    // Bit 2 of timer_contoller checks if timer is enabled
    if (bit_test(timer_controller, 2))
    {
        // Timer is enabled
        _emulator.timer += cycles;

        // time to increment the timer register
        if (_emulator.timer >= _emulator.timer_clocks_per_increment)
        {
            _emulator.timer = 0;

            // Timer is about to overflow
            if (memory_direct_read(TIMA) == 0XFF)
            {
                memory_direct_write(TIMA, memory_direct_read(TMA));
                // request the interupt
                emulator_request_interrupts(2);
            }
            else
            {
                BYTE cur_timer_val = memory_direct_read(TIMA);
                memory_direct_write(TIMA, cur_timer_val + 1);
            }
        }
    }

    // update divider register if enough clock cycles
    if (_emulator.divider >= 256)
    {
        _emulator.divider = 0;
        BYTE divider_reg_value = memory_direct_read(DIVIDER_REGISTER_ADDRESS);
        memory_direct_write(DIVIDER_REGISTER_ADDRESS, divider_reg_value + 1);
    }
}

int emulator_get_clock_speed()
{
    return _emulator.timer_clocks_per_increment;
}

void emulator_set_clock_speed(int new_speed)
{
    _emulator.timer = 0;
    _emulator.timer_clocks_per_increment = new_speed;
}