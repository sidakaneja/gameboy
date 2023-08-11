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

static void _CPU_8BIT_DEC(BYTE *reg);
static void _CPU_16BIT_DEC(WORD *reg);
static void _CPU_8BIT_INC(BYTE *reg);
static void _CPU_16BIT_INC(WORD *reg);
static void _CPU_8BIT_COMPARE(BYTE orig, BYTE comp);

static void _CPU_JUMP_IF_CONDITION(bool condition_result, bool condition);
static void _CPU_JUMP_TO_IMMEDIATE_WORD(bool condition_result, bool condition);
static void _CPU_CALL(bool condition_result, bool condition);
static void _CPU_RETURN(bool condition_result, bool condition);

// CB instructions ///////////////////////////////////////////////////
static void _CPU_TEST_BIT(BYTE reg, int bit);
static void _CPU_RL_THROUGH_CARRY(BYTE *byte);

static int _cpu_execute_cb_instruction();

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
    case 0xF3: // Disable interupts
    {
        emulator_disable_interupts();
        return 4;
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
        _push_word_onto_stack(new_address);
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
