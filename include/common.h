#ifndef COMMON_H
#define COMMON_H

#include "config.h"

void bit_set(BYTE *byte, int bit_to_set);
void bit_reset(BYTE *byte, int bit_to_reset);
bool bit_test(BYTE byte, int bit_to_test);
BYTE bit_get(BYTE byte, int bit_to_set);
#endif