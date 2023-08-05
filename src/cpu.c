#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "cpu.h"
#include "memory.h"

static struct cpu_context _cpu;

// static helper functions
static WORD _read_word_at_pc();
static BYTE _read_byte_at_pc();
static SIGNED_BYTE _read_signed_byte_at_pc();
static void _bit_set(BYTE *byte, int bit_to_set);
static void _bit_reset(BYTE *byte, int bit_to_reset);
static bool _bit_test(BYTE byte, int bit_to_test);

// Master instructions for opcodes
static void _CPU_16BIT_LOAD(WORD *reg);
static void _CPU_8BIT_XOR(BYTE *reg, BYTE to_xor, bool read_byte);
static void _CPU_16BIT_DEC(WORD *reg);
static void _CPU_16BIT_INC(WORD *reg);
static void _CPU_JUMP_IF_CONDITION(bool condition_result, bool condition);

// CP instructions
static void _CPU_TEST_BIT(BYTE reg, int bit);

static int _cpu_execute_cb_instruction();

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

    // 8-bit xor A with something
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

    // Write a to memory HL, decrement/increment register HL
    case 0x32: // LD (HL-),A
        memory_write(_cpu.HL.reg, _cpu.AF.hi);
        _CPU_16BIT_DEC(&_cpu.HL.reg);
        return 8;
    case 0x22: // LD (HL+),A
        memory_write(_cpu.HL.reg, _cpu.AF.hi);
        _CPU_16BIT_INC(&_cpu.HL.reg);
        return 8;

    // If following condition is met then add n to current address and jump to it
    case 0x18: // JR, *
        _CPU_JUMP_IF_CONDITION(true, true);
        return 8;
    case 0x20: // JR NZ,*
        _CPU_JUMP_IF_CONDITION(_bit_test(_cpu.AF.lo, FLAG_Z), false);
        return 8;
    case 0x28: // JR Z,*
        _CPU_JUMP_IF_CONDITION(_bit_test(_cpu.AF.lo, FLAG_Z), true);
        return 8;
    case 0x30: // JR NC,*
        _CPU_JUMP_IF_CONDITION(_bit_test(_cpu.AF.lo, FLAG_C), false);
        return 8;
    case 0x38: // JR C,*
        _CPU_JUMP_IF_CONDITION(_bit_test(_cpu.AF.lo, FLAG_C), true);
        return 8;

    // CB instructions
    case 0xCB:
        // Increment PC to get OXCB prefixed opcode
        _cpu.PC.reg += 1;
        return _cpu_execute_cb_instruction();
    default:
        printf("Not implemented%x\n", opcode);
        assert(false);
    };
}

static int _cpu_execute_cb_instruction()
{
    BYTE opcode = memory_read(_cpu.PC.reg);
    _cpu.PC.reg += 1;

    switch (opcode)
    {
    case 0x7C:
        _CPU_TEST_BIT(_cpu.HL.hi, 7);
        return 8;
    default:
        printf("Not implemented 0xCB-%x\n", opcode);
        assert(false);
    }
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
        _bit_set(&_cpu.AF.lo, FLAG_Z);
    }
}

static void _CPU_16BIT_DEC(WORD *reg)
{
    *reg -= 1;
}

static void _CPU_16BIT_INC(WORD *reg)
{
    *reg += 1;
}

static void _CPU_JUMP_IF_CONDITION(bool condition_result, bool condition)
{
    if (condition_result == condition)
    {
        // If condition is met, go to new address
        SIGNED_BYTE add_to_cur_address = _read_signed_byte_at_pc();
        _cpu.PC.reg += add_to_cur_address;
    }
    // add 1 to PC to go to next instruction in both cases.
    // jump if condition met assumes PC is AT next instruction before adding to it
    _cpu.PC.reg += 1;
}

static void _CPU_TEST_BIT(BYTE reg, int bit)
{
    if (_bit_test(reg, bit))
    {
        _bit_reset(&_cpu.AF.lo, FLAG_Z);
    }
    else
    {
        _bit_set(&_cpu.AF.lo, FLAG_Z);
    }
    _bit_reset(&_cpu.AF.lo, FLAG_N);
    _bit_set(&_cpu.AF.lo, FLAG_H);
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

static SIGNED_BYTE _read_signed_byte_at_pc()
{
    return (SIGNED_BYTE)memory_read(_cpu.PC.reg);
}

static void _bit_set(BYTE *byte, int bit_to_set)
{
    BYTE mask = 0x01 << bit_to_set;
    *byte |= mask;
}

static void _bit_reset(BYTE *byte, int bit_to_reset)
{
    BYTE mask = ~(0x01 << bit_to_reset);
    *byte &= mask;
}

static bool _bit_test(BYTE byte, int bit_to_test)
{
    BYTE mask = 0x01 << bit_to_test;
    return (byte & mask) ? true : false;
}