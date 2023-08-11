#include <memory.h>
#include "em_memory.h"
#include "graphics.h"
#include "common.h"

// All the following funtions have been heavily inspired by http://www.codeslinger.co.uk/pages/projects/gameboy/lcd.html
struct graphics_context graphics;

// helper graphics functions
static void _graphics_set_lcd_status();
static void _graphics_draw_scanline();
static bool _graphics_is_lcd_enabled();

void graphics_init()
{
    memset(&graphics, 0, sizeof(graphics));
}
void graphics_update(int cycles)
{
    _graphics_set_lcd_status();
    if (_graphics_is_lcd_enabled())
    {
        graphics.scanline_counter -= cycles;
    }

    if (graphics.scanline_counter <= 0)
    {
        // increment scanline
        BYTE prev_scanline = memory_direct_read(SCANLINE_ADDRESS);
        memory_direct_write(SCANLINE_ADDRESS, prev_scanline + 1);

        BYTE cur_scanline = memory_read(SCANLINE_ADDRESS);

        graphics.scanline_counter = SCANLINE_CLOCK_CYCLES;

        // If reach end of visible scanlines, request a VBLANK interrupt
        if (cur_scanline == NUM_SCANLINES)
        {
            emulator_request_interrupts(VBLANK_INTERRUPT);
        }
        else if (cur_scanline > NUM_SCANLINES)
        {
            // Start scanlines from 0
            memory_direct_write(SCANLINE_ADDRESS, 0);
        }
        else if (cur_scanline < NUM_SCANLINES)
        {
            // Draw current scanline
            _graphics_draw_scanline();
        }
    }
}

static void _graphics_set_lcd_status()
{
    BYTE status = memory_read(LCD_STATUS_ADDRESS);

    if (_graphics_is_lcd_enabled())
    {
        // must set LCD mode to 1 for some games to work
        graphics.scanline_counter = 456;
        memory_direct_write(SCANLINE_ADDRESS, 0);

        // & sets mode to 1
        status &= 252;
        bit_set(&status, 0);
        memory_write(LCD_STATUS_ADDRESS, status);
        return;
    }

    BYTE cur_scanline = memory_read(SCANLINE_ADDRESS);
    BYTE current_mode = status & 0x3;

    BYTE mode = 0;
    bool req_interrupt = false;

    // in vblank so set mode to 1
    if (cur_scanline >= NUM_SCANLINES)
    {
        mode = 1;
        bit_set(&status, 0);
        bit_reset(&status, 1);
        req_interrupt = bit_test(status, 4);
    }
    else
    {
        int mode_2_bound = 456 - 80;
        int mode_3_bound = mode_2_bound - 172;

        // mode 2
        if (graphics.scanline_counter >= mode_2_bound)
        {
            mode = 2;
            bit_set(&status, 1);
            bit_reset(&status, 0);
            req_interrupt = bit_test(status, 5);
        }
        // mode 3
        else if (graphics.scanline_counter >= mode_3_bound)
        {
            mode = 3;
            bit_set(&status, 1);
            bit_set(&status, 0);
        }
        // mode 0
        else
        {
            mode = 0;
            bit_reset(&status, 1);
            bit_reset(&status, 0);
            req_interrupt = bit_test(status, 3);
        }
    }

    // just entered a new mode so request interupt
    if (req_interrupt && (mode != current_mode))
        emulator_request_interrupts(LCD_INTERRUPT);

    // check the conincidence flag
    if (cur_scanline == memory_read(0xFF45))
    {
        bit_set(&status, 2);
        if (bit_test(status, 6))
        {
            emulator_request_interrupts(LCD_INTERRUPT);
        }
    }
    else
    {
        bit_reset(&status, 2);
    }
    memory_write(LCD_STATUS_ADDRESS, status);
}

static void _graphics_draw_scanline()
{
    // TODO
    printf("TODO: _graphics_draw_scanline");
}

static bool _graphics_is_lcd_enabled()
{
    return bit_test(memory_read(0xFF40), 7);
}