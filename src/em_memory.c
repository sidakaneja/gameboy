#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "em_memory.h"

static BYTE *memory = 0;

static void _memory_dma_transfer(BYTE data);

void memory_init(BYTE *mem)
{
    memory = mem;
    // Initial values
    memory[0xFF05] = 0x00;
    memory[0xFF06] = 0x00;
    memory[0xFF07] = 0x00;
    memory[0xFF10] = 0x80;
    memory[0xFF11] = 0xBF;
    memory[0xFF12] = 0xF3;
    memory[0xFF14] = 0xBF;
    memory[0xFF16] = 0x3F;
    memory[0xFF17] = 0x00;
    memory[0xFF19] = 0xBF;
    memory[0xFF1A] = 0x7F;
    memory[0xFF1B] = 0xFF;
    memory[0xFF1C] = 0x9F;
    memory[0xFF1E] = 0xBF;
    memory[0xFF20] = 0xFF;
    memory[0xFF21] = 0x00;
    memory[0xFF22] = 0x00;
    memory[0xFF23] = 0xBF;
    memory[0xFF24] = 0x77;
    memory[0xFF25] = 0xF3;
    memory[0xFF26] = 0xF1;
    memory[0xFF40] = 0x91;
    memory[0xFF42] = 0x00;
    memory[0xFF43] = 0x00;
    memory[0xFF45] = 0x00;
    memory[0xFF47] = 0xFC;
    memory[0xFF48] = 0xFF;
    memory[0xFF49] = 0xFF;
    memory[0xFF4A] = 0x00;
    memory[0xFF4B] = 0x00;
    memory[0xFFFF] = 0x00;
}

BYTE memory_read(WORD address)
{
    return memory[address];
}

void memory_write(WORD address, BYTE data)
{
    if (address == SCANLINE_ADDRESS)
    {
        // When a game writes to the SCANLINE_ADDRESS, it starts re-rendering from the 0th scanline
        memory[address] = 0;
    }
    else if (address == DMA_ADDRESS)
    {
        // game launches a DMA for sprites when it attempts to write to memory address DMA_ADDRESS
        _memory_dma_transfer(data);
    }
    else
    {
        memory[address] = data;
    }
}

// ONLY USED WHEN THE HARDWARE CHAGES MEMORY AND NOT THE GAME
void memory_direct_write(WORD address, BYTE data)
{
    memory[address] = data;
}

BYTE memory_direct_read(WORD address)
{
    return memory[address];
}

// For context, refer to http://www.codeslinger.co.uk/pages/projects/gameboy/dma.html
static void _memory_dma_transfer(BYTE data)
{
    WORD address = data << 8; // source address is data * 100
    for (int i = 0; i < 0xA0; i++)
    {
        memory_write(0xFE00 + i, memory_read(address + i));
    }
}