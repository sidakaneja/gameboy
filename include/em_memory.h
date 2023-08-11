#ifndef EM_MEMORY_H
#define EM_MEMORY_H

#include <stdbool.h>
#include "config.h"

void memory_init(BYTE *mem);

BYTE memory_read(WORD address);
void memory_write(WORD address, BYTE data);

// ONLY USED WHEN THE HARDWARE CHAGES MEMORY AND NOT THE GAME
void memory_direct_write(WORD address, BYTE data);
BYTE memory_direct_read(WORD address);
#endif