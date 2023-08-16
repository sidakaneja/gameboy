#ifndef CPU_H
#define CPU_H

#include "config.h"

union cpu_register
{
    WORD reg;
    struct
    {
        BYTE lo;
        BYTE hi;
    };
};

struct cpu_context
{
    union cpu_register PC;
    union cpu_register SP;

    // General purpose registers
    union cpu_register AF;
    union cpu_register BC;
    union cpu_register DE;
    union cpu_register HL;
};

void cpu_intialize();
int cpu_next_execute_instruction();
void cpu_interrupt(WORD interrupt_address);
void temp_print_registers();
#endif