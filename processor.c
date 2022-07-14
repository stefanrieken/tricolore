/*

A 32-bit fixed-width instruction set for accessing 64k memory
using 128 zero-page style 16-bit registers (or e.g. page 0xFF).


Status register:
- Carry ("bit 9 changed")
- Overflow ("sign bit changed (in the unexpected direction)")
- Negative ("sign bit is now negative")
- Zero ("outcome was zero")

Insruction is 8 bits: MMccSooo
Followed by register (8 bits), usually also dest;
Then operand, which operates depending on mode (MM):

MM = mode:
00 = reg->reg (use free byte to indicate: byte or (variable sized) word, indirect addressing, increment / decrement, etc.)
01 = reg->mem;
10 = mem->reg
11 = immediate->reg

S = size:
0 = 1 bit (write one register or mem location)
1 = 2 bits (write 2 consecutive register or mem locations; NOTE or correctly perform 2 consecutive operations with the right flags!)

cc = conditional:
00 Always
01 GT: operation or subtractive comparison yielded >0 (negative not set)
10 LT: operation or subtractive comparison yielded <0 (negative set)
11 EQ: operation or subtractive comparison yielded zero

Note that there are no built-in conditions for the overflow and carry flags.
If they need to be checked, it has to be done manually instead (TST, and then act on EQ).
More frustratingly, there is also no NEQ condition.
Multiple workarounds can be conceived, such as NOT-ing the test outcome.


Sanity check: subtractive compare of -1 (0xFF) to 15 (0x0F)
- The subtraction properly yields 0xF0 == -6
- Therefore no carry is set
- No overflow either
- Negative is set, suggesting that 15 is bigger => OK!

Subtractive compare of -1 (0xFF) to -15 (0xF1)
- 'Simply' yields 0x0E
- No carry is set (also, carry is ignored in signed numbers)
- Sign changed, but in the expected direction?
- Negative is not set, suggesting that -15 is smaller => OK!

Subtractive compare of -15 (0xF1) to -1 (0xFF)
- Treat as 0xFF1 - 0x0FF = 0xEF2, or 0xF2 with borrow
- Borrow influences carry (how?)
- Sign did not change
- Negative is set, suggesting that -1 is bigger => OK!


S = Size of data:
0 - 1-byte operation
1 - 2-byte operation (may be 2 consecutive steps internally)

Operations:
000 MOV
001 (uh forgot to fill this one in. reserved!)
010 ADC (clear carry beforehand if you don't care for its contents)
011 SBC
100 AND
101 ORR
110 XOR
111 ROT (further flags are in free byte - e.g. shift, rotate carry in)


o All operations are ALU operations; PC, SR and SP are special registers.

o The assembler accepts global variable names as register names, assigning them in static order of declaration.

o Dynamically (scoped) naming of registers, as well as keeping track of local (data stack) variables is trickier
  and should be beyond our scope (no pun intended).

o Convenience operations may be supported as macro's.

Example: JSR (push PC, jmp immediate) (let's say SP points to next free and goes up)

     .global ZERO, PC, SR, SP, XX

     .macro JSR x
       mov PC, [SP]++ ;indirect addressing
       mov x, PC
     .end

     .macro RET
       mov [--SP], PC;
     .end

Other examples:
     .macro CLC
       AND SR, #~0b01 ; assuming that that's where the carry is
     .end

     .macro TST x, y
       mov x, XX
       and XX, #y
     .end

     .macro CMP x, y
       and SR, #~0b01 ; assuming that that's where the carry is
       mov x, XX
       sbc XX, y
     .end

     .macro NOT x
        xor x, #~0x0;
     .end

*/

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "instruction.h"

uint8_t * memory;
uint8_t * regs;
uint16_t * stack;

int allocation_counter=0;

uint16_t * zero;
uint16_t * pc;
uint16_t * xx; // macro scratch register
uint8_t * sr;
uint8_t * sp; // single-page call stack

#define SR_CRY 0b0001
#define SR_OVF 0b0010
#define SR_NEG 0b0100
#define SR_ZRO 0b1000

static inline bool ok(int condition) {
	switch(condition) {
    case AL:
    default:
    return true;
		case GT: return (*sr & SR_NEG) == 0;
		case LT: return (*sr & SR_NEG) != 0;
		case EQ: return (*sr & SR_ZRO) != 0;
	}
}

static inline uint8_t fetch_src(instruction * instr, int turn) {
	// note that 'src' actually equals 'dest' in a 2-arg instruction set
	// so in all cases but reg->mem the src is a register.
	return (instr->mode == REG_MEM) ? memory[instr->address + turn] : regs[instr->reg + turn];
}

static inline uint8_t fetch_arg(instruction * instr, int turn) {
	// so the arg is the left hand side!
	switch(instr->mode) {
		case REG_REG: // reg->reg; arg is in hi
			if (instr->addr.lo & 0b1) { // indirect addressing
				uint16_t * full_address = (uint16_t *) &regs[instr->addr.hi];
				return memory[*full_address + turn];
			} else {
				return regs[instr->addr.hi + turn];
			}
			break;
		case REG_MEM: // reg->mem
			return regs[instr->reg + turn];
			break;
		case MEM_REG: // mem->reg
			return memory[instr->address + turn];
			break;
		case IMM_REG: // immediate->reg
    default: // default only added to please compiler
			return turn == 0 ? instr->addr.lo : instr->addr.hi;
			break;
	}
}

static inline uint8_t perform(instruction * instr, uint8_t src, uint8_t arg) {
	uint16_t result;

	switch(instr->operation) {
		case MOV:
		case HUH: // (uh, reserved?)
			result = arg;
			break;
		case ADC:
			result = src + arg;
			break;
		case SBC:
			result = src - arg;
			break;
		case AND:
			result = src & arg;
			break;
		case OR:
			result = result | arg;
			break;
		case XOR:
			result = result ^ arg;
			break;
		case ROT:
			// TODO
			result = 0;
			break;
	}

	// TODO set flags
	return result & 0xFF;
}

static inline void write_back(instruction * instr, int turn, uint8_t result) {
	// note that 'src' actually equals 'dest' in a 2-arg instruction set
	// so in all cases but reg->mem the dest is a register.
	if (instr->mode == REG_MEM) {
		memory[instr->address + turn] = result;
	} else {
		regs[instr->reg + turn] = result;
	}
}

void run() {

	while (*pc != 0) {
		printf("PC: %d\n", *pc);
		instruction * instr = (instruction *) &memory[*pc];
		if (ok(instr->condition)) {
			int turn = 0;
			do {
				uint8_t src = fetch_src(instr, turn);
				uint8_t arg = fetch_arg(instr, turn);
				uint8_t result = perform(instr, src, arg);
				write_back(instr, turn, result);
			} while (instr->size != turn++); // that is, repeat instruction for both lo and hi data if it is of 'word' length
		}
	}
	printf("PC: %d\n", *pc);
}

static inline uint8_t * reg1() {
	uint8_t * result = &regs[allocation_counter];
	allocation_counter += 1;
	return result;
}

static inline uint16_t * reg2() {
	uint16_t * result = (uint16_t *) &regs[allocation_counter];
	allocation_counter += 2;
	return result;
}

int main (int argc, char ** argv) {
	allocation_counter = 0;

	memory = (uint8_t *) malloc(sizeof(uint8_t) * 0xFFF); // 4 kb
	regs = (uint8_t *) &memory[0xF00]; // "high zero page"
	stack = (uint16_t *) &memory[0xE00];

	zero = reg2();
	pc = reg2();
	xx = reg2(); // macro scratch register
	sr = reg1();
	sp = reg1(); // single-page call stack

	*pc = 0x800; // start execution after video memory
	*sr = 0;
	*sp = 0x00; // either additive stack pointing to next available, or subtractive pointing to before

	instruction * at_pc = (instruction *) &memory[*pc];
	at_pc->operation = MOV; // move
	at_pc->condition = AL; // always
	at_pc->size = 0b1; // 2 bytes
	at_pc->mode = IMM_REG; // immmediate to register
	at_pc->address = 0;
	at_pc->reg = 0x02; // PC
	run();

  return 0;
}
