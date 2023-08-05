#ifndef MEMORY_H
#define MEMORY_H

#include <stdbool.h>
#include "config.h"

struct memory_context
{

};

void initialize_memory(bool bootstrap_provided);
BYTE memory_read(WORD address);
#endif