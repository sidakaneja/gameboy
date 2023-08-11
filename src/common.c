#include <stdbool.h>

#include "common.h"

void bit_set(BYTE *byte, int bit_to_set)
{
    BYTE mask = 0x01 << bit_to_set;
    *byte |= mask;
}

void bit_reset(BYTE *byte, int bit_to_reset)
{
    BYTE mask = ~(0x01 << bit_to_reset);
    *byte &= mask;
}

bool bit_test(BYTE byte, int bit_to_test)
{
    BYTE mask = 0x01 << bit_to_test;
    return (byte & mask) ? true : false;
}
