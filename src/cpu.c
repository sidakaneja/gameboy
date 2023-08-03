#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "cpu.h"
#include "em_memory.h"
#include "emulator.h"
#include "common.h"

static struct cpu_context _cpu;

// Helpers ////////////////////////////////////////////////////////////
static WORD _read_word_at_pc();
static BYTE _read_byte_at_pc();
static SIGNED_BYTE _read_signed_byte_at_pc();
static void _push_word_onto_stack(WORD word);
static WORD _pop_word_off_stack();

// Master instructions for opcodes
static void _CPU_8BIT_LOAD(BYTE *reg);
static void _CPU_16BIT_LOAD(WORD *reg);
static void _CPU_REG_LOAD(BYTE *reg, BYTE val);
static void _CPU_REG_LOAD_FROM_MEMORY(BYTE *reg, WORD address);

static void _CPU_8BIT_XOR(BYTE *reg, BYTE to_xor, bool read_byte);
static void _CPU_8BIT_OR(BYTE *reg, BYTE to_or);
static void _CPU_8BIT_AND(BYTE *reg, BYTE to_and);

static void _CPU_8BIT_DEC(BYTE *reg);
static void _CPU_16BIT_DEC(WORD *reg);
static void _CPU_8BIT_INC(BYTE *reg);

static void _CPU_8BIT_ADD(BYTE *reg, BYTE to_add);
static void _CPU_8BIT_SUB(BYTE *reg, BYTE to_sub);

static void _CPU_16BIT_ADD(WORD *reg, WORD to_add);

static void _CPU_16BIT_INC(WORD *reg);
static void _CPU_8BIT_COMPARE(BYTE orig, BYTE comp);

static void _CPU_JUMP_IF_CONDITION(bool condition_result, bool condition);
static void _CPU_JUMP_TO_IMMEDIATE_WORD(bool condition_result, bool condition);
static void _CPU_CALL(bool condition_result, bool condition);
static void _CPU_RETURN(bool condition_result, bool condition);

// CB instructions ///////////////////////////////////////////////////
static void _CPU_TEST_BIT(BYTE reg, int bit);
static void _CPU_RL_THROUGH_CARRY(BYTE *byte);
static void _CPU_SHIFT_RIGHT_INTO_CARRY(BYTE *reg);
static void _CPU_RR_THROUGH_CARRY(BYTE *reg);

static void _CPU_SWAP_NIBBLES(BYTE *reg);

static int _cpu_execute_cb_instruction();

void temp_print_registers()
{
    printf("AF:%0X\tBC:%0X\tDE:%0X\tHL:%0X\tSP:%0X\n", _cpu.AF.reg, _cpu.BC.reg, _cpu.DE.reg, _cpu.HL.reg, _cpu.SP.reg);
}
void cpu_interrupt(WORD interrupt_address)
{
    _push_word_onto_stack(_cpu.PC.reg);
    _cpu.PC.reg = interrupt_address;
}

void cpu_intialize()
{
    memset(&_cpu, 0, sizeof(_cpu));
    _cpu.PC.reg = 0x100;
    _cpu.AF.reg = 0x01B0;
    _cpu.BC.reg = 0x0013;
    _cpu.DE.reg = 0x00D8;
    _cpu.HL.reg = 0x014D;
    _cpu.SP.reg = 0xFFFE;
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

    // Load register with a BYTE
    case 0x40:
    {
        _CPU_REG_LOAD(&_cpu.BC.hi, _cpu.BC.hi);
        return 4;
    }
    case 0x41:
    {
        _CPU_REG_LOAD(&_cpu.BC.hi, _cpu.BC.lo);
        return 4;
    }
    case 0x42:
    {
        _CPU_REG_LOAD(&_cpu.BC.hi, _cpu.DE.hi);
        return 4;
    }
    case 0x43:
    {
        _CPU_REG_LOAD(&_cpu.BC.hi, _cpu.DE.lo);
        return 4;
    }
    case 0x44:
    {
        _CPU_REG_LOAD(&_cpu.BC.hi, _cpu.HL.hi);
        return 4;
    }
    case 0x45:
    {
        _CPU_REG_LOAD(&_cpu.BC.hi, _cpu.HL.lo);
        return 4;
    }
    case 0x48:
    {
        _CPU_REG_LOAD(&_cpu.BC.lo, _cpu.BC.hi);
        return 4;
    }
    case 0x49:
    {
        _CPU_REG_LOAD(&_cpu.BC.lo, _cpu.BC.lo);
        return 4;
    }
    case 0x4A:
    {
        _CPU_REG_LOAD(&_cpu.BC.lo, _cpu.DE.hi);
        return 4;
    }
    case 0x4B:
    {
        _CPU_REG_LOAD(&_cpu.BC.lo, _cpu.DE.lo);
        return 4;
    }
    case 0x4C:
    {
        _CPU_REG_LOAD(&_cpu.BC.lo, _cpu.HL.hi);
        return 4;
    }
    case 0x4D:
    {
        _CPU_REG_LOAD(&_cpu.BC.lo, _cpu.HL.lo);
        return 4;
    }
    case 0x50:
    {
        _CPU_REG_LOAD(&_cpu.DE.hi, _cpu.BC.hi);
        return 4;
    }
    case 0x51:
    {
        _CPU_REG_LOAD(&_cpu.DE.hi, _cpu.BC.lo);
        return 4;
    }
    case 0x52:
    {
        _CPU_REG_LOAD(&_cpu.DE.hi, _cpu.DE.hi);
        return 4;
    }
    case 0x53:
    {
        _CPU_REG_LOAD(&_cpu.DE.hi, _cpu.DE.lo);
        return 4;
    }
    case 0x54:
    {
        _CPU_REG_LOAD(&_cpu.DE.hi, _cpu.HL.hi);
        return 4;
    }
    case 0x55:
    {
        _CPU_REG_LOAD(&_cpu.DE.hi, _cpu.HL.lo);
        return 4;
    }
    case 0x58:
    {
        _CPU_REG_LOAD(&_cpu.DE.lo, _cpu.BC.hi);
        return 4;
    }
    case 0x59:
    {
        _CPU_REG_LOAD(&_cpu.DE.lo, _cpu.BC.lo);
        return 4;
    }
    case 0x5A:
    {
        _CPU_REG_LOAD(&_cpu.DE.lo, _cpu.DE.hi);
        return 4;
    }
    case 0x5B:
    {
        _CPU_REG_LOAD(&_cpu.DE.lo, _cpu.DE.lo);
        return 4;
    }
    case 0x5C:
    {
        _CPU_REG_LOAD(&_cpu.DE.lo, _cpu.HL.hi);
        return 4;
    }
    case 0x5D:
    {
        _CPU_REG_LOAD(&_cpu.DE.lo, _cpu.HL.lo);
        return 4;
    }
    case 0x60:
    {
        _CPU_REG_LOAD(&_cpu.HL.hi, _cpu.BC.hi);
        return 4;
    }
    case 0x61:
    {
        _CPU_REG_LOAD(&_cpu.HL.hi, _cpu.BC.lo);
        return 4;
    }
    case 0x62:
    {
        _CPU_REG_LOAD(&_cpu.HL.hi, _cpu.DE.hi);
        return 4;
    }
    case 0x63:
    {
        _CPU_REG_LOAD(&_cpu.HL.hi, _cpu.DE.lo);
        return 4;
    }
    case 0x64:
    {
        _CPU_REG_LOAD(&_cpu.HL.hi, _cpu.HL.hi);
        return 4;
    }
    case 0x65:
    {
        _CPU_REG_LOAD(&_cpu.HL.hi, _cpu.HL.lo);
        return 4;
    }
    case 0x68:
    {
        _CPU_REG_LOAD(&_cpu.HL.lo, _cpu.BC.hi);
        return 4;
    }
    case 0x69:
    {
        _CPU_REG_LOAD(&_cpu.HL.lo, _cpu.BC.lo);
        return 4;
    }
    case 0x6A:
    {
        _CPU_REG_LOAD(&_cpu.HL.lo, _cpu.DE.hi);
        return 4;
    }
    case 0x6B:
    {
        _CPU_REG_LOAD(&_cpu.HL.lo, _cpu.DE.lo);
        return 4;
    }
    case 0x6C:
    {
        _CPU_REG_LOAD(&_cpu.HL.lo, _cpu.HL.hi);
        return 4;
    }
    case 0x6D:
    {
        _CPU_REG_LOAD(&_cpu.HL.lo, _cpu.HL.lo);
        return 4;
    }

        // Put value at A into another register
    case 0x47: // LD register, A
    {
        _CPU_REG_LOAD(&_cpu.BC.hi, _cpu.AF.hi);
        return 4;
    }
    case 0x4F:
    {
        _CPU_REG_LOAD(&_cpu.BC.lo, _cpu.AF.hi);
        return 4;
    }
    case 0x57:
    {
        _CPU_REG_LOAD(&_cpu.DE.hi, _cpu.AF.hi);
        return 4;
    }
    case 0x5F:
    {
        _CPU_REG_LOAD(&_cpu.DE.lo, _cpu.AF.hi);
        return 4;
    }
    case 0x67:
    {
        _CPU_REG_LOAD(&_cpu.HL.hi, _cpu.AF.hi);
        return 4;
    }
    case 0x6F:
    {
        _CPU_REG_LOAD(&_cpu.HL.lo, _cpu.AF.hi);
        return 4;
    }

    // 8 bit loads, load BYTE at pc to register
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

    // write memory to reg
    case 0x7E:
    {
        _CPU_REG_LOAD_FROM_MEMORY(&_cpu.AF.hi, _cpu.HL.reg);
        return 8;
    }
    case 0x46:
    {
        _CPU_REG_LOAD_FROM_MEMORY(&_cpu.BC.hi, _cpu.HL.reg);
        return 8;
    }
    case 0x4E:
    {
        _CPU_REG_LOAD_FROM_MEMORY(&_cpu.BC.lo, _cpu.HL.reg);
        return 8;
    }
    case 0x56:
    {
        _CPU_REG_LOAD_FROM_MEMORY(&_cpu.DE.hi, _cpu.HL.reg);
        return 8;
    }
    case 0x5E:
    {
        _CPU_REG_LOAD_FROM_MEMORY(&_cpu.DE.lo, _cpu.HL.reg);
        return 8;
    }
    case 0x66:
    {
        _CPU_REG_LOAD_FROM_MEMORY(&_cpu.HL.hi, _cpu.HL.reg);
        return 8;
    }
    case 0x6E:
    {
        _CPU_REG_LOAD_FROM_MEMORY(&_cpu.HL.lo, _cpu.HL.reg);
        return 8;
    }
    case 0x0A:
    {
        _CPU_REG_LOAD_FROM_MEMORY(&_cpu.AF.hi, _cpu.BC.reg);
        return 8;
    }
    case 0x1A:
    {
        _CPU_REG_LOAD_FROM_MEMORY(&_cpu.AF.hi, _cpu.DE.reg);
        return 8;
    }
    case 0xF2:
    {
        _CPU_REG_LOAD_FROM_MEMORY(&_cpu.AF.hi, (0xFF00 + _cpu.BC.lo));
        return 8;
    }
    case 0xF0:
    {
        BYTE read = _read_byte_at_pc();
        _cpu.PC.reg += 1;
        _CPU_REG_LOAD_FROM_MEMORY(&_cpu.AF.hi, memory_read(0xFF00 + read));
        return 12;
    }
    case 0xFA:
    {
        WORD address = _read_word_at_pc();
        _cpu.PC.reg += 2;
        _CPU_REG_LOAD_FROM_MEMORY(&_cpu.AF.hi, address);
        return 16;
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
    // Write memory HL to A, decrement/increment register HL
    case 0x2A: // LD A,(HL+)
    {
        _CPU_REG_LOAD_FROM_MEMORY(&_cpu.AF.hi, _cpu.HL.reg);
        _CPU_16BIT_INC(&_cpu.HL.reg);
        return 8;
    }
    case 0x3A:
    {
        _CPU_REG_LOAD_FROM_MEMORY(&_cpu.AF.hi, _cpu.HL.reg);
        _CPU_16BIT_DEC(&_cpu.HL.reg);
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
    // write register BYTE to memory at HL
    case 0x70:
    {
        memory_write(_cpu.HL.reg, _cpu.BC.hi);
        return 8;
    }
    case 0x71:
    {
        memory_write(_cpu.HL.reg, _cpu.BC.lo);
        return 8;
    }
    case 0x72:
    {
        memory_write(_cpu.HL.reg, _cpu.DE.hi);
        return 8;
    }
    case 0x73:
    {
        memory_write(_cpu.HL.reg, _cpu.DE.lo);
        return 8;
    }
    case 0x74:
    {
        memory_write(_cpu.HL.reg, _cpu.HL.hi);
        return 8;
    }
    case 0x75:
    {
        memory_write(_cpu.HL.reg, _cpu.HL.lo);
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

    // 8-bit OR reg with reg
    case 0xB7:
    {
        _CPU_8BIT_OR(&_cpu.AF.hi, _cpu.AF.hi);
        return 4;
    }
    case 0xB0:
    {
        _CPU_8BIT_OR(&_cpu.AF.hi, _cpu.BC.hi);
        return 4;
    }
    case 0xB1:
    {
        _CPU_8BIT_OR(&_cpu.AF.hi, _cpu.BC.lo);
        return 4;
    }
    case 0xB2:
    {
        _CPU_8BIT_OR(&_cpu.AF.hi, _cpu.DE.hi);
        return 4;
    }
    case 0xB3:
    {
        _CPU_8BIT_OR(&_cpu.AF.hi, _cpu.DE.lo);
        return 4;
    }
    case 0xB4:
    {
        _CPU_8BIT_OR(&_cpu.AF.hi, _cpu.HL.hi);
        return 4;
    }
    case 0xB5:
    {
        _CPU_8BIT_OR(&_cpu.AF.hi, _cpu.HL.lo);
        return 4;
    }
    case 0xB6:
    {
        _CPU_8BIT_OR(&_cpu.AF.hi, memory_read(_cpu.HL.reg));
        return 8;
    }
    case 0xF6:
    {
        BYTE to_or = memory_read(_cpu.PC.reg);
        _cpu.PC.reg += 1;
        _CPU_8BIT_OR(&_cpu.AF.hi, to_or);
        return 8;
    }

    // 8-bit AND A with Byte. Store result back in A. set flags.
    case 0xA7:
    {
        _CPU_8BIT_AND(&_cpu.AF.hi, _cpu.AF.hi);
        return 4;
    }
    case 0xA0:
    {
        _CPU_8BIT_AND(&_cpu.AF.hi, _cpu.BC.hi);
        return 4;
    }
    case 0xA1:
    {
        _CPU_8BIT_AND(&_cpu.AF.hi, _cpu.BC.lo);
        return 4;
    }
    case 0xA2:
    {
        _CPU_8BIT_AND(&_cpu.AF.hi, _cpu.DE.hi);
        return 4;
    }
    case 0xA3:
    {
        _CPU_8BIT_AND(&_cpu.AF.hi, _cpu.DE.lo);
        return 4;
    }
    case 0xA4:
    {
        _CPU_8BIT_AND(&_cpu.AF.hi, _cpu.HL.hi);
        return 4;
    }
    case 0xA5:
    {
        _CPU_8BIT_AND(&_cpu.AF.hi, _cpu.HL.lo);
        return 4;
    }
    case 0xA6:
    {
        _CPU_8BIT_AND(&_cpu.AF.hi, memory_read(_cpu.HL.reg));
        return 8;
    }
    case 0xE6:
    {
        _CPU_8BIT_AND(&_cpu.AF.hi, _read_byte_at_pc());
        _cpu.PC.reg += 1;
        return 8;
    }

    // 8-bit add
    case 0x87:
    {
        _CPU_8BIT_ADD(&_cpu.AF.hi, _cpu.AF.hi);
        return 4;
    }
    case 0x80:
    {
        _CPU_8BIT_ADD(&_cpu.AF.hi, _cpu.BC.hi);
        return 4;
    }
    case 0x81:
    {
        _CPU_8BIT_ADD(&_cpu.AF.hi, _cpu.BC.lo);
        return 4;
    }
    case 0x82:
    {
        _CPU_8BIT_ADD(&_cpu.AF.hi, _cpu.DE.hi);
        return 4;
    }
    case 0x83:
    {
        _CPU_8BIT_ADD(&_cpu.AF.hi, _cpu.DE.lo);
        return 4;
    }
    case 0x84:
    {
        _CPU_8BIT_ADD(&_cpu.AF.hi, _cpu.HL.hi);
        return 4;
    }
    case 0x85:
    {
        _CPU_8BIT_ADD(&_cpu.AF.hi, _cpu.HL.lo);
        return 4;
    }
    case 0x86:
    {
        _CPU_8BIT_ADD(&_cpu.AF.hi, memory_read(_cpu.HL.reg));
        return 8;
    }
    case 0xC6:
    {
        _CPU_8BIT_ADD(&_cpu.AF.hi, _read_byte_at_pc());
        _cpu.PC.reg += 1;
        return 8;
    }

    // 8-bit add + carry
    case 0x8F:
    {

        _CPU_8BIT_ADD(&_cpu.AF.hi, _cpu.AF.hi + bit_get(_cpu.AF.lo, FLAG_C));
        return 4;
    }
    case 0x88:
    {
        _CPU_8BIT_ADD(&_cpu.AF.hi, _cpu.BC.hi + bit_get(_cpu.AF.lo, FLAG_C));
        return 4;
    }
    case 0x89:
    {
        _CPU_8BIT_ADD(&_cpu.AF.hi, _cpu.BC.lo + bit_get(_cpu.AF.lo, FLAG_C));
        return 4;
    }
    case 0x8A:
    {
        _CPU_8BIT_ADD(&_cpu.AF.hi, _cpu.DE.hi + bit_get(_cpu.AF.lo, FLAG_C));
        return 4;
    }
    case 0x8B:
    {
        _CPU_8BIT_ADD(&_cpu.AF.hi, _cpu.DE.lo + bit_get(_cpu.AF.lo, FLAG_C));
        return 4;
    }
    case 0x8C:
    {
        _CPU_8BIT_ADD(&_cpu.AF.hi, _cpu.HL.hi + bit_get(_cpu.AF.lo, FLAG_C));
        return 4;
    }
    case 0x8D:
    {
        _CPU_8BIT_ADD(&_cpu.AF.hi, _cpu.HL.lo + bit_get(_cpu.AF.lo, FLAG_C));
        return 4;
    }
    case 0x8E:
    {
        _CPU_8BIT_ADD(&_cpu.AF.hi, memory_read(_cpu.HL.reg) + bit_get(_cpu.AF.lo, FLAG_C));
        return 8;
    }
    case 0xCE:
    {
        _CPU_8BIT_ADD(&_cpu.AF.hi, _read_byte_at_pc() + bit_get(_cpu.AF.lo, FLAG_C));
        _cpu.PC.reg += 1;
        return 8;
    }

    // 16-bit add to HL
    case 0x09:
    {
        _CPU_16BIT_ADD(&_cpu.HL.reg, _cpu.BC.reg);
        return 8;
    }
    case 0x19:
    {
        _CPU_16BIT_ADD(&_cpu.HL.reg, _cpu.DE.reg);
        return 8;
    }
    case 0x29:
    {
        _CPU_16BIT_ADD(&_cpu.HL.reg, _cpu.HL.reg);
        return 8;
    }
    case 0x39:
    {
        _CPU_16BIT_ADD(&_cpu.HL.reg, _cpu.SP.reg);
        return 8;
    }

    // 8-bit subtract from A
    case 0x97:
    {
        _CPU_8BIT_SUB(&_cpu.AF.hi, _cpu.AF.hi);
        return 4;
    }
    case 0x90:
    {
        _CPU_8BIT_SUB(&_cpu.AF.hi, _cpu.BC.hi);
        return 4;
    }
    case 0x91:
    {
        _CPU_8BIT_SUB(&_cpu.AF.hi, _cpu.BC.lo);
        return 4;
    }
    case 0x92:
    {
        _CPU_8BIT_SUB(&_cpu.AF.hi, _cpu.DE.hi);
        return 4;
    }
    case 0x93:
    {
        _CPU_8BIT_SUB(&_cpu.AF.hi, _cpu.DE.lo);
        return 4;
    }
    case 0x94:
    {
        _CPU_8BIT_SUB(&_cpu.AF.hi, _cpu.HL.hi);
        return 4;
    }
    case 0x95:
    {
        _CPU_8BIT_SUB(&_cpu.AF.hi, _cpu.HL.lo);
        return 4;
    }
    case 0x96:
    {
        _CPU_8BIT_SUB(&_cpu.AF.hi, memory_read(_cpu.HL.reg));
        return 8;
    }
    case 0xD6:
    {
        _CPU_8BIT_SUB(&_cpu.AF.hi, _read_byte_at_pc());
        _cpu.PC.reg += 1;
        return 8;
    }

    // 8-bit subtract + carry
    case 0x9F:
    {
        _CPU_8BIT_SUB(&_cpu.AF.hi, _cpu.AF.hi + bit_get(_cpu.AF.lo, FLAG_C));
        return 4;
    }
    case 0X98:
    {
        _CPU_8BIT_SUB(&_cpu.AF.hi, _cpu.BC.hi + bit_get(_cpu.AF.lo, FLAG_C));
        return 4;
    }
    case 0X99:
    {
        _CPU_8BIT_SUB(&_cpu.AF.hi, _cpu.BC.lo + bit_get(_cpu.AF.lo, FLAG_C));
        return 4;
    }
    case 0X9A:
    {
        _CPU_8BIT_SUB(&_cpu.AF.hi, _cpu.DE.hi + bit_get(_cpu.AF.lo, FLAG_C));
        return 4;
    }
    case 0X9B:
    {
        _CPU_8BIT_SUB(&_cpu.AF.hi, _cpu.DE.lo + bit_get(_cpu.AF.lo, FLAG_C));
        return 4;
    }
    case 0X9C:
    {
        _CPU_8BIT_SUB(&_cpu.AF.hi, _cpu.HL.hi + bit_get(_cpu.AF.lo, FLAG_C));
        return 4;
    }
    case 0X9D:
    {
        _CPU_8BIT_SUB(&_cpu.AF.hi, _cpu.HL.lo + bit_get(_cpu.AF.lo, FLAG_C));
        return 4;
    }
    case 0X9E:
    {
        _CPU_8BIT_SUB(&_cpu.AF.hi, memory_read(_cpu.HL.reg) + bit_get(_cpu.AF.lo, FLAG_C));
        return 8;
    }
    case 0XDE:
    {
        _CPU_8BIT_SUB(&_cpu.AF.hi, _read_byte_at_pc() + bit_get(_cpu.AF.lo, FLAG_C));
        _cpu.PC.reg += 1;
        return 8;
    }

    // 8-bit inc register
    case 0x3c:
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

    // 16-bit increment register
    case 0x03:
    {
        _CPU_16BIT_INC(&_cpu.BC.reg);
        return 8;
    }
    case 0x13:
    {
        _CPU_16BIT_INC(&_cpu.DE.reg);
        return 8;
    }
    case 0x23:
    {
        _CPU_16BIT_INC(&_cpu.HL.reg);
        return 8;
    }
    case 0x33:
    {
        _CPU_16BIT_INC(&_cpu.SP.reg);
        return 8;
    }

    // 8-bit decrement register
    case 0x3D:
    {
        _CPU_8BIT_DEC(&_cpu.AF.hi);
        return 4;
    }
    case 0x05:
    {
        _CPU_8BIT_DEC(&_cpu.BC.hi);
        return 4;
    }
    case 0x0D:
    {
        _CPU_8BIT_DEC(&_cpu.BC.lo);
        return 4;
    }
    case 0x15:
    {
        _CPU_8BIT_DEC(&_cpu.DE.hi);
        return 4;
    }
    case 0x1D:
    {
        _CPU_8BIT_DEC(&_cpu.DE.lo);
        return 4;
    }
    case 0x25:
    {
        _CPU_8BIT_DEC(&_cpu.HL.hi);
        return 4;
    }
    case 0x2D:
    {
        _CPU_8BIT_DEC(&_cpu.HL.lo);
        return 4;
    }
    case 0x35:
    {
        BYTE byte = memory_read(_cpu.HL.reg);
        _CPU_8BIT_DEC(&byte);
        memory_write(_cpu.HL.reg, byte);
        return 12;
    }

    // 16-bit decrement register
    case 0x0B:
    {
        _CPU_16BIT_DEC(&_cpu.BC.reg);
        return 8;
    }
    case 0x1B:
    {
        _CPU_16BIT_DEC(&_cpu.DE.reg);
        return 8;
    }
    case 0x2B:
    {
        _CPU_16BIT_DEC(&_cpu.HL.reg);
        return 8;
    }
    case 0x3B:
    {
        _CPU_16BIT_DEC(&_cpu.SP.reg);
        return 8;
    }

    // 8-Bit compare
    case 0xBF: // Compare A with value, set flags accordingly
    {
        _CPU_8BIT_COMPARE(_cpu.AF.hi, _cpu.AF.hi);
        return 4;
    }
    case 0xB8:
    {
        _CPU_8BIT_COMPARE(_cpu.AF.hi, _cpu.BC.hi);
        return 4;
    }
    case 0xB9:
    {
        _CPU_8BIT_COMPARE(_cpu.AF.hi, _cpu.BC.lo);
        return 4;
    }
    case 0xBA:
    {
        _CPU_8BIT_COMPARE(_cpu.AF.hi, _cpu.DE.hi);
        return 4;
    }
    case 0xBB:
    {
        _CPU_8BIT_COMPARE(_cpu.AF.hi, _cpu.DE.lo);
        return 4;
    }
    case 0xBC:
    {
        _CPU_8BIT_COMPARE(_cpu.AF.hi, _cpu.HL.hi);
        return 4;
    }
    case 0xBD:
    {
        _CPU_8BIT_COMPARE(_cpu.AF.hi, _cpu.HL.lo);
        return 4;
    }
    case 0xBE:
    {
        _CPU_8BIT_COMPARE(_cpu.AF.hi, memory_read(_cpu.HL.reg));
        return 8;
    }
    case 0xFE:
    {
        BYTE n = _read_byte_at_pc();
        _cpu.PC.reg += 1;
        _CPU_8BIT_COMPARE(_cpu.AF.hi, n);
        return 8;
    }

    // Jump to address given by immediate word if condition is met
    case 0xE9:
    {
        _cpu.PC.reg = _cpu.HL.reg;
        return 4;
    }
    case 0xC3:
    {
        _CPU_JUMP_TO_IMMEDIATE_WORD(true, true);
        return 12;
    }
    case 0xC2:
    {
        _CPU_JUMP_TO_IMMEDIATE_WORD(bit_test(_cpu.AF.lo, FLAG_Z), false);
        return 12;
    }
    case 0xCA:
    {
        _CPU_JUMP_TO_IMMEDIATE_WORD(bit_test(_cpu.AF.lo, FLAG_Z), true);
        return 12;
    }
    case 0xD2:
    {
        _CPU_JUMP_TO_IMMEDIATE_WORD(bit_test(_cpu.AF.lo, FLAG_C), false);
        return 12;
    }
    case 0xDA:
    {
        _CPU_JUMP_TO_IMMEDIATE_WORD(bit_test(_cpu.AF.lo, FLAG_C), true);
        return 12;
    }

    // If following condition is met then add n to current address and jump to it
    case 0x18: // JR, *
    {
        _CPU_JUMP_IF_CONDITION(true, true);
        return 8;
    }
    case 0x20: // JR NZ,*
    {
        _CPU_JUMP_IF_CONDITION(bit_test(_cpu.AF.lo, FLAG_Z), false);
        return 8;
    }
    case 0x28: // JR Z,*
    {
        _CPU_JUMP_IF_CONDITION(bit_test(_cpu.AF.lo, FLAG_Z), true);
        return 8;
    }
    case 0x30: // JR NC,*
    {
        _CPU_JUMP_IF_CONDITION(bit_test(_cpu.AF.lo, FLAG_C), false);
        return 8;
    }
    case 0x38: // JR C,*
    {
        _CPU_JUMP_IF_CONDITION(bit_test(_cpu.AF.lo, FLAG_C), true);
        return 8;
    }

    // calls
    case 0xCD:
    {
        _CPU_CALL(true, true);
        return 12;
    }
    case 0xC4:
    {
        _CPU_CALL(bit_test(_cpu.AF.lo, FLAG_Z), false);
        return 12;
    }
    case 0xCC:
    {
        _CPU_CALL(bit_test(_cpu.AF.lo, FLAG_Z), true);
        return 12;
    }
    case 0xD4:
    {
        _CPU_CALL(bit_test(_cpu.AF.lo, FLAG_C), false);
        return 12;
    }
    case 0xDC:
    {
        _CPU_CALL(bit_test(_cpu.AF.lo, FLAG_C), true);
        return 12;
    }

    // returns
    case 0xC9:
    {
        _CPU_RETURN(true, true);
        return 8;
    }
    case 0xC0:
    {
        _CPU_RETURN(bit_test(_cpu.AF.lo, FLAG_Z), false);
        return 8;
    }
    case 0xC8:
    {
        _CPU_RETURN(bit_test(_cpu.AF.lo, FLAG_Z), true);
        return 8;
    }
    case 0xD0:
    {
        _CPU_RETURN(bit_test(_cpu.AF.lo, FLAG_C), false);
        return 8;
    }
    case 0xD8:
    {
        _CPU_RETURN(bit_test(_cpu.AF.lo, FLAG_C), true);
        return 8;
    }

    // push word onto stack
    case 0xF5:
    {
        _push_word_onto_stack(_cpu.AF.reg);
        return 16;
    }
    case 0xC5:
    {
        _push_word_onto_stack(_cpu.BC.reg);
        return 16;
    }
    case 0xD5:
    {
        _push_word_onto_stack(_cpu.DE.reg);
        return 16;
    }
    case 0xE5:
    {
        _push_word_onto_stack(_cpu.HL.reg);
        return 16;
    }

    // Pop word off stack and put into register
    case 0xF1:
    {
        _cpu.AF.reg = _pop_word_off_stack();
        return 12;
    }
    case 0xC1:
    {
        _cpu.BC.reg = _pop_word_off_stack();
        return 12;
    }
    case 0xD1:
    {
        _cpu.DE.reg = _pop_word_off_stack();
        return 12;
    }
    case 0xE1:
    {
        _cpu.HL.reg = _pop_word_off_stack();
        return 12;
    }

    // Unique
    case 0x17: // RLA through carry
    {
        _CPU_RL_THROUGH_CARRY(&_cpu.AF.hi);
        return 4;
    }
    case 0x1F: // RRA through carry
    {
        _CPU_RR_THROUGH_CARRY(&_cpu.AF.hi);
        return 4;
    }
    case 0xF3: // Disable interupts
    {
        emulator_disable_interupts();
        return 4;
    }
    case 0XEA:
    {
        WORD address = _read_word_at_pc();
        _cpu.PC.reg += 2;
        memory_write(address, _cpu.AF.hi);
        return 16;
    }
    // CB instructions
    case 0xCB:
    {
        return _cpu_execute_cb_instruction();
    }
    default:
    {
        printf("Not implemented %x at PC%x\n", opcode, _cpu.PC.reg - 1);
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
    // rotate left through carry, set lsb to old carry
    case 0x10:
    {
        _CPU_RL_THROUGH_CARRY(&_cpu.BC.hi);
        return 8;
    }
    case 0x11:
    {
        _CPU_RL_THROUGH_CARRY(&_cpu.BC.lo);
        return 8;
    }
    case 0x12:
    {
        _CPU_RL_THROUGH_CARRY(&_cpu.DE.hi);
        return 8;
    }
    case 0x13:
    {
        _CPU_RL_THROUGH_CARRY(&_cpu.DE.lo);
        return 8;
    }
    case 0x14:
    {
        _CPU_RL_THROUGH_CARRY(&_cpu.HL.hi);
        return 8;
    }
    case 0x15:
    {
        _CPU_RL_THROUGH_CARRY(&_cpu.HL.lo);
        return 8;
    }
    case 0x17:
    {
        _CPU_RL_THROUGH_CARRY(&_cpu.AF.hi);
        return 8;
    }

    // rotate right through carry
    case 0x18:
    {
        _CPU_RR_THROUGH_CARRY(&_cpu.BC.hi);
        return 8;
    }
    case 0x19:
    {
        _CPU_RR_THROUGH_CARRY(&_cpu.BC.lo);
        return 8;
    }
    case 0x1A:
    {
        _CPU_RR_THROUGH_CARRY(&_cpu.DE.hi);
        return 8;
    }
    case 0x1B:
    {
        _CPU_RR_THROUGH_CARRY(&_cpu.DE.lo);
        return 8;
    }
    case 0x1C:
    {
        _CPU_RR_THROUGH_CARRY(&_cpu.HL.hi);
        return 8;
    }
    case 0x1D:
    {
        _CPU_RR_THROUGH_CARRY(&_cpu.HL.lo);
        return 8;
    }
    case 0x1E:
    {
        BYTE reg = memory_read(_cpu.HL.reg);
        _CPU_RR_THROUGH_CARRY(&reg);
        memory_write(_cpu.HL.reg, reg);
        return 16;
    }
    case 0x1F:
    {
        _CPU_RR_THROUGH_CARRY(&_cpu.AF.hi);
        return 8;
    }

    // swap nibbles
    case 0x37:
    {
        _CPU_SWAP_NIBBLES(&_cpu.AF.hi);
        return 8;
    }
    case 0x30:
    {
        _CPU_SWAP_NIBBLES(&_cpu.BC.hi);
        return 8;
    }
    case 0x31:
    {
        _CPU_SWAP_NIBBLES(&_cpu.BC.lo);
        return 8;
    }
    case 0x32:
    {
        _CPU_SWAP_NIBBLES(&_cpu.DE.hi);
        return 8;
    }
    case 0x33:
    {
        _CPU_SWAP_NIBBLES(&_cpu.DE.lo);
        return 8;
    }
    case 0x34:
    {
        _CPU_SWAP_NIBBLES(&_cpu.HL.hi);
        return 8;
    }
    case 0x35:
    {
        _CPU_SWAP_NIBBLES(&_cpu.HL.lo);
        return 8;
    }
    case 0x36:
    {
        BYTE byte = memory_read(_cpu.HL.reg);
        _CPU_SWAP_NIBBLES(&byte);
        memory_write(_cpu.HL.reg, byte);
        return 16;
    }

    // Shift n right into Carry. MSB set to 0, flags set
    case 0x38:
    {
        _CPU_SHIFT_RIGHT_INTO_CARRY(&_cpu.BC.hi);
        return 8;
    }
    case 0x39:
    {
        _CPU_SHIFT_RIGHT_INTO_CARRY(&_cpu.BC.lo);
        return 8;
    }
    case 0x3A:
    {
        _CPU_SHIFT_RIGHT_INTO_CARRY(&_cpu.DE.hi);
        return 8;
    }
    case 0x3B:
    {
        _CPU_SHIFT_RIGHT_INTO_CARRY(&_cpu.DE.lo);
        return 8;
    }
    case 0x3C:
    {
        _CPU_SHIFT_RIGHT_INTO_CARRY(&_cpu.HL.hi);
        return 8;
    }
    case 0x3D:
    {
        _CPU_SHIFT_RIGHT_INTO_CARRY(&_cpu.HL.lo);
        return 8;
    }
    case 0x3F:
    {
        _CPU_SHIFT_RIGHT_INTO_CARRY(&_cpu.AF.hi);
        return 8;
    }

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

// Load a BYTE from memory into a register
static void _CPU_REG_LOAD_FROM_MEMORY(BYTE *reg, WORD address)
{
    *reg = memory_read(address);
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
        bit_set(&_cpu.AF.lo, FLAG_Z);
    }
}

// OR register with value, set flags
static void _CPU_8BIT_OR(BYTE *reg, BYTE to_or)
{
    *reg |= to_or;
    _cpu.AF.lo = 0;

    if (*reg == 0)
    {
        bit_set(&_cpu.AF.lo, FLAG_Z);
    }
}

static void _CPU_8BIT_AND(BYTE *reg, BYTE to_and)
{
    *reg &= to_and;
    _cpu.AF.lo = 0;
    if (*reg == 0)
    {
        bit_set(&_cpu.AF.lo, FLAG_Z);
    }
    bit_set(&_cpu.AF.lo, FLAG_H);
}

static void _CPU_8BIT_ADD(BYTE *reg, BYTE to_add)
{
    _cpu.AF.lo = 0;

    BYTE before = *reg;
    *reg += to_add;

    if (*reg == 0)
    {
        bit_set(&_cpu.AF.lo, FLAG_Z);
    }

    WORD htest = (before & 0xF);
    htest += (to_add & 0xF);

    if (htest > 0xF)
    {
        bit_set(&_cpu.AF.lo, FLAG_H);
    }

    WORD sum = (WORD)(before + to_add);
    // If overflow,set C flag
    if (sum > 0xFF)
    {
        bit_set(&_cpu.AF.lo, FLAG_C);
    }
}

static void _CPU_8BIT_SUB(BYTE *reg, BYTE to_sub)
{
    BYTE before = *reg;
    *reg -= to_sub;

    _cpu.AF.lo = 0;

    if (*reg == 0)
    {
        bit_set(&_cpu.AF.lo, FLAG_Z);
    }

    bit_set(&_cpu.AF.lo, FLAG_N);

    // set if no borrow
    if (before < to_sub)
    {
        bit_set(&_cpu.AF.lo, FLAG_C);
    }

    SIGNED_WORD htest = (before & 0xF);
    htest -= (before & 0xF);

    if (htest < 0)
    {
        bit_set(&_cpu.AF.lo, FLAG_H);
    }
}

static void _CPU_16BIT_ADD(WORD *reg, WORD to_add)
{
    WORD before = *reg;
    *reg += to_add;

    bit_reset(&_cpu.AF.lo, FLAG_N);

    uint32_t sum = (uint32_t)before + to_add;
    if (sum > 0XFFFF)
    {
        bit_set(&_cpu.AF.lo, FLAG_C);
    }
    else
    {
        bit_reset(&_cpu.AF.lo, FLAG_C);
    }

    WORD half_sum = (before & 0xFF) + (to_add & 0xFF);
    if (half_sum > 0XFF)
    {
        bit_set(&_cpu.AF.lo, FLAG_H);
    }
    else
    {
        bit_reset(&_cpu.AF.lo, FLAG_H);
    }
}
static void _CPU_8BIT_INC(BYTE *reg)
{
    BYTE before = *reg;
    *reg += 1;

    // If result == 0, set Zero flag
    if (*reg == 0)
    {
        bit_set(&_cpu.AF.lo, FLAG_Z);
    }
    else
    {
        bit_reset(&_cpu.AF.lo, FLAG_Z);
    }

    bit_reset(&_cpu.AF.lo, FLAG_N);

    // If carry from bit 3 to bit 4, that is lower nibble is 1111 before increment, set half carry flag.
    if ((before & 0XF) == 0XF)
    {
        bit_set(&_cpu.AF.lo, FLAG_H);
    }
    else
    {
        bit_reset(&_cpu.AF.lo, FLAG_H);
    }
}

// Decrement BYTE in register, set appropriate flags
static void _CPU_8BIT_DEC(BYTE *reg)
{
    BYTE before = *reg;
    *reg -= 1;

    // If result == 0, set Zero flag
    if (*reg == 0)
    {
        bit_set(&_cpu.AF.lo, FLAG_Z);
    }
    else
    {
        bit_reset(&_cpu.AF.lo, FLAG_Z);
    }

    bit_set(&_cpu.AF.lo, FLAG_N);

    // Set H if borrow from bit 4, that is lower nibble is == 0
    if ((before & 0XF) == 0)
    {
        bit_set(&_cpu.AF.lo, FLAG_H);
    }
    else
    {
        bit_reset(&_cpu.AF.lo, FLAG_H);
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

static void _CPU_8BIT_COMPARE(BYTE orig, BYTE comp)
{
    _cpu.AF.lo = 0x0;
    if (orig == comp)
    {
        bit_set(&_cpu.AF.lo, FLAG_Z);
    }
    bit_set(&_cpu.AF.lo, FLAG_N);

    if (orig < comp)
    {
        bit_set(&_cpu.AF.lo, FLAG_C);
    }

    SIGNED_WORD htest = orig & 0xF;
    htest -= (comp & 0xF);

    if (htest < 0)
    {
        bit_set(&_cpu.AF.lo, FLAG_H);
    }
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

static void _CPU_JUMP_TO_IMMEDIATE_WORD(bool condition_result, bool condition)
{

    WORD word = _read_word_at_pc();
    _cpu.PC.reg += 2;

    if (condition_result == condition)
    {
        // If condition is met, go to new address
        _cpu.PC.reg = word;
    }
}

static void _CPU_CALL(bool condition_result, bool condition)
{
    WORD new_address = _read_word_at_pc();
    _cpu.PC.reg += 2;

    if (condition_result == condition)
    {
        _push_word_onto_stack(_cpu.PC.reg);
        _cpu.PC.reg = new_address;
    }
}

// pop word off stack and set PC to it if condition is met.
static void _CPU_RETURN(bool condition_result, bool condition)
{
    if (condition_result == condition)
    {
        _cpu.PC.reg = _pop_word_off_stack();
    }
}
// CB instructions ///////////////////////////////////////////////////

static void _CPU_TEST_BIT(BYTE reg, int bit)
{
    if (bit_test(reg, bit))
    {
        bit_reset(&_cpu.AF.lo, FLAG_Z);
    }
    else
    {
        bit_set(&_cpu.AF.lo, FLAG_Z);
    }
    bit_reset(&_cpu.AF.lo, FLAG_N);
    bit_set(&_cpu.AF.lo, FLAG_H);
}

// Rotate byte left, set Z if result == 0, C constains bit 7 data
static void _CPU_RL_THROUGH_CARRY(BYTE *byte)
{
    bool is_carry_set = bit_test(_cpu.AF.lo, FLAG_C);
    _cpu.AF.lo = 0;

    if (bit_test(*byte, 7))
    {
        bit_set(&_cpu.AF.lo, FLAG_C);
    }

    *byte <<= 1;
    if (*byte == 0)
    {
        bit_set(&_cpu.AF.lo, FLAG_Z);
    }

    if (is_carry_set)
    {
        bit_set(byte, 0);
    }
}

static void _CPU_SHIFT_RIGHT_INTO_CARRY(BYTE *reg)
{

    bool is_lsb_set = bit_test(*reg, 0);

    _cpu.AF.lo = 0;

    *reg >>= 1;

    if (is_lsb_set)
    {
        bit_set(&_cpu.AF.lo, FLAG_C);
    }
    if (reg == 0)
    {
        bit_set(&_cpu.AF.lo, FLAG_Z);
    }
}

static void _CPU_RR_THROUGH_CARRY(BYTE *reg)
{
    bool is_carry_set = bit_test(_cpu.AF.lo, FLAG_C);
    bool is_lsb_set = bit_test(*reg, 0);

    _cpu.AF.lo = 0;

    *reg >>= 1;

    if (is_lsb_set)
    {
        bit_set(&_cpu.AF.lo, FLAG_C);
    }
    if (is_carry_set)
    {
        bit_set(reg, 7);
    }
    if (reg == 0)
    {
        bit_set(&_cpu.AF.lo, FLAG_Z);
    }
}

static void _CPU_SWAP_NIBBLES(BYTE *reg)
{
    _cpu.AF.lo = 0;

    *reg = (((*reg & 0xF0) >> 4) | ((*reg & 0x0F) << 4));

    if (*reg == 0)
    {
        bit_set(&_cpu.AF.lo, FLAG_Z);
    }
}
// Helpers ////////////////////////////////////////////////////////////
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

static void _push_word_onto_stack(WORD word)
{
    BYTE hi = word >> 8;
    BYTE lo = word & 0xFF;
    _cpu.SP.reg -= 1;
    memory_write(_cpu.SP.reg, hi);
    _cpu.SP.reg -= 1;
    memory_write(_cpu.SP.reg, lo);
}

static WORD _pop_word_off_stack()
{
    WORD word = memory_read(_cpu.SP.reg + 1) << 8;
    word |= memory_read(_cpu.SP.reg);
    _cpu.SP.reg += 2;

    return word;
}
