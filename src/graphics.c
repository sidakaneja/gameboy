#include <memory.h>
#include <assert.h>
#include "em_memory.h"
#include "graphics.h"
#include "common.h"

// All the following funtions have been heavily inspired by http://www.codeslinger.co.uk/pages/projects/gameboy/lcd.html
struct graphics_context graphics;

// helper graphics functions
static void _graphics_set_lcd_status();
static void _graphics_draw_scanline();
static bool _graphics_is_lcd_enabled();
static void _graphics_render_background(BYTE lcd_control);
static void _graphics_render_sprites(BYTE lcd_control);
COLOUR _graphics_get_colour(BYTE colourNum, WORD address);

void graphics_init()
{
    memset(&graphics, 0, sizeof(graphics));
    graphics.scanline_counter = SCANLINE_CLOCK_CYCLES;
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

        BYTE cur_scanline = memory_direct_read(SCANLINE_ADDRESS);

        graphics.scanline_counter += SCANLINE_CLOCK_CYCLES;

        // If reach end of visible scanlines, request a VBLANK interrupt
        if (cur_scanline == VISIBLE_SCANLINES)
        {
            emulator_request_interrupts(VBLANK_INTERRUPT);
        }
        else if (cur_scanline > TOTAL_SCANLINES)
        {
            // Start scanlines from 0
            memory_direct_write(SCANLINE_ADDRESS, 0);
        }
        else if (cur_scanline < VISIBLE_SCANLINES)
        {
            // Draw current scanline
            _graphics_draw_scanline();
        }
    }
}

static void _graphics_set_lcd_status()
{
    BYTE status = memory_read(LCD_STATUS_ADDRESS);

    if (!_graphics_is_lcd_enabled())
    {
        // must set LCD mode to 1 for some games to work
        graphics.scanline_counter = SCANLINE_CLOCK_CYCLES;
        memory_direct_write(SCANLINE_ADDRESS, 0);

        // & sets mode to 1
        status &= 252;
        bit_set(&status, 0);
        memory_direct_write(LCD_STATUS_ADDRESS, status);
        return;
    }

    BYTE cur_scanline = memory_direct_read(SCANLINE_ADDRESS);
    BYTE current_mode = status & 0x3;

    BYTE mode = 0;
    bool req_interrupt = false;

    // in vblank so set mode to 1
    if (cur_scanline >= VISIBLE_SCANLINES)
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
    BYTE lcd_control = memory_read(LCD_CONTROL_ADDRESS);

    // draw scanline if lcd is enabled
    if (_graphics_is_lcd_enabled())
    {
        _graphics_render_background(lcd_control);
        _graphics_render_sprites(lcd_control);
    }
}

static bool _graphics_is_lcd_enabled()
{
    return bit_test(memory_read(LCD_CONTROL_ADDRESS), 7);
}

static void _graphics_render_background(BYTE lcd_control)
{
    // Check if background is enabled
    if (!bit_test(lcd_control, LCD_BACKGROUND_ENABLED_BIT))
    {
        return;
    }

    WORD tile_data_vram_location = 0;
    WORD background_tile_id_location = 0;
    bool unsig = true;

    // Which 160X144 of the 256X256 pixels to draw, that is where are the viewing area and window located
    BYTE viewing_area_start_y = memory_read(0xFF42);
    BYTE viewing_area_start_x = memory_read(0xFF43);
    BYTE window_start_y = memory_read(0xFF4A);
    BYTE window_start_x = memory_read(0xFF4B) - 7;

    bool using_window = false;

    if (bit_test(lcd_control, LCD_WINDOW_ENABLED_BIT))
    {
        if (window_start_y <= memory_read(0xFF44))
            using_window = true;
    }
    else
    {
        using_window = false;
    }

    // Where is the tile data located in VRAM
    if (bit_test(lcd_control, LCD_TILE_VRAM_LOCATION_BIT))
    {
        tile_data_vram_location = 0x8000;
    }
    else
    {
        tile_data_vram_location = 0x8800;
        unsig = false;
    }

    // Which location are we getting the tile ids from
    if (using_window == false)
    {
        if (bit_test(lcd_control, LCD_BG_TILE_ID_LOCATION_BIT))
        {
            // If bit is set, the Tile IDs are located at:
            background_tile_id_location = 0x9C00;
        }
        else
        {
            // else they are located at:
            background_tile_id_location = 0x9800;
        }
    }
    else
    {
        // Using window tile ids
        if (bit_test(lcd_control, LCD_WINDOW_TILE_ID_LOCATION_BIT))
        {
            background_tile_id_location = 0x9C00;
        }
        else
        {
            background_tile_id_location = 0x9800;
        }
    }

    BYTE yPos = 0;
    // yPos is used to calculate which of 32 vertical tiles the
    // current scanline is drawing
    if (!using_window)
    {
        yPos = viewing_area_start_y + memory_read(0xFF44);
    }
    else
    {
        yPos = memory_read(0xFF44) - window_start_y;
    }

    WORD tileRow = (((BYTE)(yPos / 8)) * 32);

    for (int pixel = 0; pixel < 160; pixel++)
    {
        BYTE xPos = pixel + viewing_area_start_x;

        if (using_window)
        {
            if (pixel >= window_start_x)
            {
                xPos = pixel - window_start_x;
            }
        }

        WORD tile_col = (xPos / 8);
        SIGNED_WORD tile_num;

        if (unsig)
        {
            tile_num = (BYTE)memory_read(background_tile_id_location + tileRow + tile_col);
        }
        else
        {
            tile_num = (SIGNED_BYTE)memory_read(background_tile_id_location + tileRow + tile_col);
        }

        WORD tile_location = tile_data_vram_location;

        if (unsig)
        {
            tile_location += (tile_num * 16);
        }
        else
        {
            tile_location += ((tile_num + 128) * 16);
        }

        BYTE line = yPos % 8;
        line *= 2;
        BYTE data1 = memory_read(tile_location + line);
        BYTE data2 = memory_read(tile_location + line + 1);

        int colourBit = xPos % 8;
        colourBit -= 7;
        colourBit *= -1;

        int colourNum = bit_get(data2, colourBit);
        colourNum <<= 1;
        colourNum |= bit_get(data1, colourBit);

        COLOUR col = _graphics_get_colour(colourNum, 0xFF47);
        int red = 0;
        int green = 0;
        int blue = 0;

        switch (col)
        {
        case WHITE:
            red = 255;
            green = 255;
            blue = 255;
            break;
        case LIGHT_GRAY:
            red = 0xCC;
            green = 0xCC;
            blue = 0xCC;
            break;
        case DARK_GRAY:
            red = 0x77;
            green = 0x77;
            blue = 0x77;
            break;
        case BLACK:
            red = 0;
            green = 0;
            blue = 0;
            break;
        }

        int final_y = memory_read(0xFF44);

        if ((final_y < 0) || (final_y > 143) || (pixel < 0) || (pixel > 159))
        {
            assert(false);
            continue;
        }

        graphics.screen_data[final_y][pixel][0] = red;
        graphics.screen_data[final_y][pixel][1] = green;
        graphics.screen_data[final_y][pixel][2] = blue;
    }
}

static void _graphics_render_sprites(BYTE lcd_control)
{

    // Draw sprites if enabled
    if (!bit_test(lcd_control, LCD_SPRITES_ENABLED_BIT))
    {
        return;
    }

    bool use8x16 = false;
    if (bit_test(lcd_control, 2))
        use8x16 = true;

    for (int sprite = 0; sprite < 40; sprite++)
    {
        BYTE index = sprite * 4;
        BYTE yPos = memory_read(0xFE00 + index) - 16;
        BYTE xPos = memory_read(0xFE00 + index + 1) - 8;
        BYTE tileLocation = memory_read(0xFE00 + index + 2);
        BYTE attributes = memory_read(0xFE00 + index + 3);

        bool yFlip = bit_test(attributes, 6);
        bool xFlip = bit_test(attributes, 5);

        int scanline = memory_read(0xFF44);

        int ysize = 8;

        if (use8x16)
            ysize = 16;

        if ((scanline >= yPos) && (scanline < (yPos + ysize)))
        {
            int line = scanline - yPos;

            if (yFlip)
            {
                line -= ysize;
                line *= -1;
            }

            line *= 2;
            BYTE data1 = memory_read((0x8000 + (tileLocation * 16)) + line);
            BYTE data2 = memory_read((0x8000 + (tileLocation * 16)) + line + 1);

            for (int tilePixel = 7; tilePixel >= 0; tilePixel--)
            {
                int colourbit = tilePixel;
                if (xFlip)
                {
                    colourbit -= 7;
                    colourbit *= -1;
                }
                int colourNum = bit_get(data2, colourbit);
                colourNum <<= 1;
                colourNum |= bit_get(data1, colourbit);

                COLOUR col = _graphics_get_colour(colourNum, bit_test(attributes, 4) ? 0xFF49 : 0xFF48);

                // white is transparent for sprites.
                if (col == WHITE)
                    continue;

                int red = 0;
                int green = 0;
                int blue = 0;

                switch (col)
                {
                case WHITE:
                    red = 255;
                    green = 255;
                    blue = 255;
                    break;
                case LIGHT_GRAY:
                    red = 0xCC;
                    green = 0xCC;
                    blue = 0xCC;
                    break;
                case DARK_GRAY:
                    red = 0x77;
                    green = 0x77;
                    blue = 0x77;
                    break;
                case BLACK:
                    red = 0;
                    green = 0;
                    blue = 0;
                    break;
                }

                int xPix = 0 - tilePixel;
                xPix += 7;

                int pixel = xPos + xPix;

                if ((scanline < 0) || (scanline > 143) || (pixel < 0) || (pixel > 159))
                {
                    //	assert(false) ;
                    continue;
                }

                // check if pixel is hidden behind background
                if (bit_test(attributes, 7) == 1)
                {
                    if ((graphics.screen_data[scanline][pixel][0] != 255) || (graphics.screen_data[scanline][pixel][1] != 255) || (graphics.screen_data[scanline][pixel][2] != 255))
                        continue;
                }

                graphics.screen_data[scanline][pixel][0] = red;
                graphics.screen_data[scanline][pixel][1] = green;
                graphics.screen_data[scanline][pixel][2] = blue;
            }
        }
    }
}

COLOUR _graphics_get_colour(BYTE colourNum, WORD address)
{
    COLOUR res = WHITE;
    BYTE palette = memory_read(address);
    int hi = 0;
    int lo = 0;

    // which bits of the colour palette does the colour id map to?
    switch (colourNum)
    {
    case 0:
        hi = 1;
        lo = 0;
        break;
    case 1:
        hi = 3;
        lo = 2;
        break;
    case 2:
        hi = 5;
        lo = 4;
        break;
    case 3:
        hi = 7;
        lo = 6;
        break;
    default:
        assert(false);
        break;
    }

    // use the palette to get the colour
    int colour = 0;
    colour = bit_get(palette, hi) << 1;
    colour |= bit_get(palette, lo);

    switch (colour)
    {
    case 0:
        res = WHITE;
        break;
    case 1:
        res = LIGHT_GRAY;
        break;
    case 2:
        res = DARK_GRAY;
        break;
    case 3:
        res = BLACK;
        break;
    default:
        assert(false);
        break;
    }

    return res;
}

BYTE graphics_get_screen_data(int col, int row, int colour)
{
    return graphics.screen_data[col][row][colour];
}
