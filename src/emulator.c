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
static void _emulator_handle_interrupts();
static void _emulator_service_interrupt(BYTE bit_to_service);

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

    assert(_emulator.renderer);
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
        temp_print_registers();
        cycles_this_update += cycles;
        printf("Executed, clock = %d (+%d)\n", cycles_this_update, cycles);
        ////////////////////////////////////////////////////
        //    blarggs test - serial output
        if (memory_direct_read(0xff02) == 0x81)
        {
            char c = memory_direct_read(0xff01);
            printf("%c", c);

            memory_direct_write(0xff02, 0x0);
        }
        ////////////////////////////////////////////////////
        if (_emulator.disable_pending > 0)
        {
            _emulator.disable_pending -= 1;
            if (_emulator.disable_pending == 0)
            {
                _emulator.master_interupt = false;
            }
        }
        if (_emulator.enable_pending > 0)
        {
            _emulator.enable_pending -= 1;
            if (_emulator.enable_pending == 0)
            {
                _emulator.master_interupt = true;
            }
        }
        _emulator_update_timers(cycles);
        graphics_update(cycles);
        _emulator_handle_interrupts();
    }
    // assert(false);
    // printf("Rendering\n");
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
    _emulator.disable_pending = 2;
}

void emulator_enable_interrupts()
{
    _emulator.enable_pending = 2;
}
// Set the requested interrupt bit at the interrupt register
void emulator_request_interrupts(BYTE interrupt_bit)
{
    BYTE req = memory_read(INTERRUPT_REGISTER_ADDRESS);
    bit_set(&req, interrupt_bit);
    memory_write(INTERRUPT_REGISTER_ADDRESS, req);
}
static void _emulator_handle_interrupts()
{
    // are interrupts enabled
    if (!_emulator.master_interupt)
    {
        return;
    }

    // Check if an interrupt is present
    BYTE interrupt_register = memory_read(INTERRUPT_REGISTER_ADDRESS);
    if (interrupt_register > 0)
    {
        // Service the highest priority interrupt, lower bit == higher priority
        for (int bit = 0; bit < 8; bit++)
        {
            if (bit_test(interrupt_register, bit))
            {
                // check if interrupt is enabled in Interupt Enabled Register at 0xFFFF
                BYTE enabledReg = memory_read(0xFFFF);
                if (bit_test(enabledReg, bit))
                {
                    _emulator_service_interrupt(bit);
                }
            }
        }
    }
}

static void _emulator_service_interrupt(BYTE bit_to_service)
{

    WORD interrupt_address = 0x00;
    switch (bit_to_service)
    {
    case 0:
        interrupt_address = 0x40;
        break; // V-Blank
    case 1:
        interrupt_address = 0x48;
        break; // LCD-STATE
    case 2:
        interrupt_address = 0x50;
        break; // Timer
    case 4:
        interrupt_address = 0x60;
        break; // JoyPad
    default:
        assert(false);
        break;
    }

    _emulator.master_interupt = false;
    cpu_interrupt(interrupt_address);

    BYTE interrupts = memory_direct_read(INTERRUPT_REGISTER_ADDRESS);
    bit_reset(&interrupts, bit_to_service);
    memory_direct_write(INTERRUPT_REGISTER_ADDRESS, interrupts);
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
    if (_emulator.divider >= 255)
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