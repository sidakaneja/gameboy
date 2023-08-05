#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "cpu.h"
#include "memory.h"

static struct cpu_context _cpu;

// static helper functions
static WORD _read_word_at_pc();
static BYTE _read_byte_at_pc();
static void _bitset(BYTE *byte, int bit_to_set);

// Master instructions for opcodes
static void _CPU_16BIT_LOAD(WORD *reg);
static void _CPU_8BIT_XOR(BYTE *reg, BYTE to_xor, bool read_byte);

void cpu_intialize()
{
    memset(&_cpu, 0, sizeof(_cpu));
}

int cpu_next_execute_instruction()
{
    // Read next opcode and increment PC
    BYTE opcode = memory_read(_cpu.PC.reg);
    _cpu.PC.reg += 1;

    switch (opcode)
    {
    case 0x00: // NOP
        return 4;

    // 16 bit loads
    case 0x01: // LD BC,u16
        _CPU_16BIT_LOAD(&_cpu.BC.reg);
        return 12;
    case 0x11: // LD DE,u16
        _CPU_16BIT_LOAD(&_cpu.DE.reg);
        return 12;
    case 0x21: // LD HL,u16
        _CPU_16BIT_LOAD(&_cpu.HL.reg);
        return 12;
    case 0x31: // LD SP,u16
        _CPU_16BIT_LOAD(&_cpu.SP.reg);
        return 12;

    // 8-bit xor
    case 0xAF: // XOR A,A
        _CPU_8BIT_XOR(&_cpu.AF.hi, _cpu.AF.hi, false);
        return 4;
    case 0xA8: // XOR A,B
        _CPU_8BIT_XOR(&_cpu.AF.hi, _cpu.BC.hi, false);
        return 4;
    case 0xA9: // XOR A,C
        _CPU_8BIT_XOR(&_cpu.AF.hi, _cpu.BC.lo, false);
        return 4;
    case 0xAA: // XOR A,D
        _CPU_8BIT_XOR(&_cpu.AF.hi, _cpu.DE.hi, false);
        return 4;
    case 0xAB: // XOR A,E
        _CPU_8BIT_XOR(&_cpu.AF.hi, _cpu.DE.lo, false);
        return 4;
    case 0xAC: // XOR A,H
        _CPU_8BIT_XOR(&_cpu.AF.hi, _cpu.HL.hi, false);
        return 4;
    case 0xAD: // XOR A,L
        _CPU_8BIT_XOR(&_cpu.AF.hi, _cpu.HL.lo, false);
        return 4;
    case 0xAE: // XOR A,(HL)
        _CPU_8BIT_XOR(&_cpu.AF.hi, memory_read(_cpu.HL.reg), false);
        return 8;
    case 0xEE: // XOR A, *
        _CPU_8BIT_XOR(&_cpu.AF.hi, 0, true);
        return 8;
    default:
        assert(false);
    };
}

// Take WORD at PC and set register to it
static void _CPU_16BIT_LOAD(WORD *reg)
{
    WORD value = _read_word_at_pc();
    *reg = value;
    _cpu.PC.reg += 2;
}

static void _CPU_8BIT_XOR(BYTE *reg, BYTE to_xor, bool read_byte)
{
    // Only for instruction 0xEE
    if (read_byte)
    {
        to_xor = _read_byte_at_pc();
        _cpu.PC.reg += 1;
    }

    *reg ^= to_xor;

    _cpu.AF.lo = 0x00;
    if (*reg == 0)
    {
        _bitset(&_cpu.AF.lo, FLAG_Z);
    }
}

static WORD _read_word_at_pc()
{
    WORD res = memory_read(_cpu.PC.reg + 1);
    res = res << 8;
    res |= memory_read(_cpu.PC.reg);
    return res;
}
static BYTE _read_byte_at_pc()
{
    return memory_read(_cpu.PC.reg);
}

static void _bitset(BYTE *byte, int bit_to_set)
{
    BYTE mask = 0x01 << bit_to_set;
    *byte |= mask;
}
