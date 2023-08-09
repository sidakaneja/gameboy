#ifndef EM_MEMORY_H
#define EM_MEMORY_H

#include <stdbool.h>
#include "config.h"

void memory_init(BYTE *mem);

BYTE memory_read(WORD address);
void memory_write(WORD address, BYTE data);
#endif