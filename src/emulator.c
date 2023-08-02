#include <memory.h>

#include "emulator.h"

static struct emulator_context _emulator;

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
        GAMEBOY_SCREEN_WIDTH * GAMEBOY_PIXEL_MULTIPLIER, GAMEBOY_SCREEN_HEIGHT * GAMEBOY_PIXEL_MULTIPLIER, 0);

    // Set background to black
    _emulator.renderer = SDL_CreateRenderer(_emulator.window, -1, SDL_TEXTUREACCESS_TARGET);
    SDL_SetRenderDrawColor(_emulator.renderer, 0, 0, 0, 255);
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
    for (int x = 0; x < GAMEBOY_SCREEN_WIDTH; x++)
    {
        for (int y = 0; y < GAMEBOY_SCREEN_HEIGHT; y++)
        {
            // Set RGB value for the display pixel
            SDL_SetRenderDrawColor(_emulator.renderer, _emulator.screen_data[y][x][0], _emulator.screen_data[y][x][1], _emulator.screen_data[y][x][2], 255);
            SDL_Rect r;
            r.x = x * GAMEBOY_PIXEL_MULTIPLIER;
            r.y = y * GAMEBOY_PIXEL_MULTIPLIER;
            r.w = GAMEBOY_PIXEL_MULTIPLIER;
            r.h = GAMEBOY_PIXEL_MULTIPLIER;
            SDL_RenderFillRect(_emulator.renderer, &r);
            SDL_RenderPresent(_emulator.renderer);
        }
    }
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

    if (!_sdl_init())
    {
        return false;
    }

    return true;
}

static void _emulator_destroy()
{
    _sdl_destroy();
}

static void _emulator_update()
{
    _sdl_render();
    _sdl_poll_quit();
}

// Main emulator loop
void emulator_run(int argc, char **argv)
{
    if (!_emulator_init())
    {
        printf("Could not initialize emulator\n");
        return;
    }

    // Infinite loop that runs until the user closes the window
    // Runs GAMEBOY_FRAME_RATE times a second
    while (!_emulator.quit)
    {
        _emulator_update();
    }

    _emulator_destroy();
}