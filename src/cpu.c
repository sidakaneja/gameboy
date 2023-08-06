#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "cpu.h"
#include "em_memory.h"

static struct cpu_context _cpu;

// static helper functions
static WORD _read_word_at_pc();
static BYTE _read_byte_at_pc();
static SIGNED_BYTE _read_signed_byte_at_pc();
static void _bit_set(BYTE *byte, int bit_to_set);
static void _bit_reset(BYTE *byte, int bit_to_reset);
static bool _bit_test(BYTE byte, int bit_to_test);

// Master instructions for opcodes
static void _CPU_8BIT_LOAD(BYTE *reg);
static void _CPU_16BIT_LOAD(WORD *reg);
static void _CPU_REG_LOAD(BYTE *reg, BYTE val);
static void _CPU_8BIT_XOR(BYTE *reg, BYTE to_xor, bool read_byte);
static void _CPU_16BIT_DEC(WORD *reg);
static void _CPU_8BIT_INC(BYTE *reg);
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
    printf("Executing %0x at PC %x\n", opcode, _cpu.PC.reg);
    _cpu.PC.reg += 1;

    switch (opcode)
    {
    case 0x00: // NOP
    {
        return 4;
    }
    // Load BYTE value to A from register/memory/immediate value
    case 0x3E: // LD A,u8 - 0x3E
    {
        _CPU_REG_LOAD(&_cpu.AF.hi, _read_byte_at_pc());
        _cpu.PC.reg += 1;
        return 8;
    }
    case 0x7F: // LD A, register
    {
        _CPU_REG_LOAD(&_cpu.AF.hi, _cpu.AF.hi);
        return 4;
    }
    case 0x78:
    {
        _CPU_REG_LOAD(&_cpu.AF.hi, _cpu.BC.hi);
        return 4;
    }
    case 0x79:
    {
        _CPU_REG_LOAD(&_cpu.AF.hi, _cpu.BC.lo);
        return 4;
    }
    case 0x7A:
    {
        _CPU_REG_LOAD(&_cpu.AF.hi, _cpu.DE.hi);
        return 4;
    }
    case 0x7B:
    {
        _CPU_REG_LOAD(&_cpu.AF.hi, _cpu.DE.lo);
        return 4;
    }
    case 0x7C:
    {
        _CPU_REG_LOAD(&_cpu.AF.hi, _cpu.HL.hi);
        return 4;
    }
    case 0x7D:
    {
        _CPU_REG_LOAD(&_cpu.AF.hi, _cpu.HL.lo);
        return 4;
    }

    // 8 bit loads
    case 0x06: // LD reg,u8
    {
        _CPU_8BIT_LOAD(&_cpu.BC.hi);
        return 8;
    }
    case 0x0E:
    {
        _CPU_8BIT_LOAD(&_cpu.BC.lo);
        return 8;
    }
    case 0x16:
    {
        _CPU_8BIT_LOAD(&_cpu.DE.hi);
        return 8;
    }
    case 0x1E:
    {
        _CPU_8BIT_LOAD(&_cpu.DE.lo);
        return 8;
    }
    case 0x26:
    {
        _CPU_8BIT_LOAD(&_cpu.HL.hi);
        return 8;
    }
    case 0x2E:
    {
        _CPU_8BIT_LOAD(&_cpu.HL.lo);
        return 8;
    }

    // 16 bit loads
    case 0x01: // LD BC,u16
    {
        _CPU_16BIT_LOAD(&_cpu.BC.reg);
        return 12;
    }
    case 0x11: // LD DE,u16
    {
        _CPU_16BIT_LOAD(&_cpu.DE.reg);
        return 12;
    }
    case 0x21: // LD HL,u16
    {
        _CPU_16BIT_LOAD(&_cpu.HL.reg);
        return 12;
    }
    case 0x31: // LD SP,u16
    {
        _CPU_16BIT_LOAD(&_cpu.SP.reg);
        return 12;
    }

    // 8-bit xor A with something
    case 0xAF: // XOR A,A
    {
        _CPU_8BIT_XOR(&_cpu.AF.hi, _cpu.AF.hi, false);
        return 4;
    }
    case 0xA8: // XOR A,B
    {
        _CPU_8BIT_XOR(&_cpu.AF.hi, _cpu.BC.hi, false);
        return 4;
    }
    case 0xA9: // XOR A,C
    {
        _CPU_8BIT_XOR(&_cpu.AF.hi, _cpu.BC.lo, false);
        return 4;
    }
    case 0xAA: // XOR A,D
    {
        _CPU_8BIT_XOR(&_cpu.AF.hi, _cpu.DE.hi, false);
        return 4;
    }
    case 0xAB: // XOR A,E
    {
        _CPU_8BIT_XOR(&_cpu.AF.hi, _cpu.DE.lo, false);
        return 4;
    }
    case 0xAC: // XOR A,H
    {
        _CPU_8BIT_XOR(&_cpu.AF.hi, _cpu.HL.hi, false);
        return 4;
    }
    case 0xAD: // XOR A,L
    {
        _CPU_8BIT_XOR(&_cpu.AF.hi, _cpu.HL.lo, false);
        return 4;
    }
    case 0xAE: // XOR A,(HL)
    {
        _CPU_8BIT_XOR(&_cpu.AF.hi, memory_read(_cpu.HL.reg), false);
        return 8;
    }
    case 0xEE: // XOR A, *
    {
        _CPU_8BIT_XOR(&_cpu.AF.hi, 0, true);
        return 8;
    }

    // Write A to memory HL, decrement/increment register HL
    case 0x32: // LD (HL-),A
    {
        memory_write(_cpu.HL.reg, _cpu.AF.hi);
        _CPU_16BIT_DEC(&_cpu.HL.reg);
        return 8;
    }
    case 0x22: // LD (HL+),A
    {
        memory_write(_cpu.HL.reg, _cpu.AF.hi);
        _CPU_16BIT_INC(&_cpu.HL.reg);
        return 8;
    }

        // put A into memory address
    case 0x02:
    {
        memory_write(_cpu.BC.reg, _cpu.AF.hi);
        return 8;
    }
    case 0x12:
    {
        memory_write(_cpu.DE.reg, _cpu.AF.hi);
        return 8;
    }
    case 0x77:
    {
        memory_write(_cpu.HL.reg, _cpu.AF.hi);
        return 8;
    }

    case 0xE0: // LD (FF00+u8),A
    {
        BYTE add_to_address = memory_read(_cpu.PC.reg);
        _cpu.PC.reg += 1;
        memory_write((0xFF00 + add_to_address), _cpu.AF.hi);
        return 12;
    }
    case 0xE2:
    {
        memory_write((0xFF00 + _cpu.BC.lo), _cpu.AF.hi);
        return 8;
    }
    // 8-bit inc register
    case 0x3C:
    {
        _CPU_8BIT_INC(&_cpu.AF.hi);
        return 4;
    }
    case 0x04:
    {
        _CPU_8BIT_INC(&_cpu.BC.hi);
        return 4;
    }
    case 0x0C:
    {
        _CPU_8BIT_INC(&_cpu.BC.lo);
        return 4;
    }
    case 0x14:
    {
        _CPU_8BIT_INC(&_cpu.DE.hi);
        return 4;
    }
    case 0x1C:
    {
        _CPU_8BIT_INC(&_cpu.DE.lo);
        return 4;
    }
    case 0x24:
    {
        _CPU_8BIT_INC(&_cpu.HL.hi);
        return 4;
    }
    case 0x2C:
    {
        _CPU_8BIT_INC(&_cpu.HL.lo);
        return 4;
    }

        // If following condition is met then add n to current address and jump to it
    case 0x18: // JR, *
    {
        _CPU_JUMP_IF_CONDITION(true, true);
        return 8;
    }
    case 0x20: // JR NZ,*
    {
        _CPU_JUMP_IF_CONDITION(_bit_test(_cpu.AF.lo, FLAG_Z), false);
        return 8;
    }
    case 0x28: // JR Z,*
    {
        _CPU_JUMP_IF_CONDITION(_bit_test(_cpu.AF.lo, FLAG_Z), true);
        return 8;
    }
    case 0x30: // JR NC,*
    {
        _CPU_JUMP_IF_CONDITION(_bit_test(_cpu.AF.lo, FLAG_C), false);
        return 8;
    }
    case 0x38: // JR C,*
    {
        _CPU_JUMP_IF_CONDITION(_bit_test(_cpu.AF.lo, FLAG_C), true);
        return 8;
    }
    // CB instructions
    case 0xCB:
    {
        return _cpu_execute_cb_instruction();
    }
    default:
    {
        printf("Not implemented %x at PC%x\n", opcode, _cpu.PC.reg);
        assert(false);
    }
    };
}

static int _cpu_execute_cb_instruction()
{
    BYTE opcode = memory_read(_cpu.PC.reg);
    _cpu.PC.reg += 1;

    switch (opcode)
    {
    case 0x7C:
    {
        _CPU_TEST_BIT(_cpu.HL.hi, 7);
        return 8;
    }
    default:
    {
        printf("Not implemented 0xCB-%x\n", opcode);
        assert(false);
    }
    }
}

// Take BYTE at PC and set register to it
static void _CPU_8BIT_LOAD(BYTE *reg)
{
    BYTE value = _read_byte_at_pc();
    *reg = value;
    _cpu.PC.reg += 1;
}

// Take WORD at PC and set register to it
static void _CPU_16BIT_LOAD(WORD *reg)
{
    WORD value = _read_word_at_pc();
    *reg = value;
    _cpu.PC.reg += 2;
}

// Load BYTE value to the register
static void _CPU_REG_LOAD(BYTE *reg, BYTE val)
{
    *reg = val;
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
static void _CPU_8BIT_INC(BYTE *reg)
{
    BYTE before = *reg;
    *reg += 1;

    // If result == 0, set Zero flag
    if (*reg == 0)
    {
        _bit_set(&_cpu.AF.lo, FLAG_Z);
    }
    else
    {
        _bit_reset(&_cpu.AF.lo, FLAG_Z);
    }

    _bit_reset(&_cpu.AF.lo, FLAG_N);

    // If carry from bit 3 to bit 4, that is lower nibble is 1111 before increment, set half carry flag.
    if ((before & 0XF) == 0XF)
    {
        _bit_set(&_cpu.AF.lo, FLAG_H);
    }
    else
    {
        _bit_reset(&_cpu.AF.lo, FLAG_H);
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