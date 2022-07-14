#ifndef INSTRUCTION_H
#define INSTRUCTION_H

typedef enum {
  AL,
  GT,
  LT,
  EQ
} condition;

typedef enum {
  MOV,
  HUH,
  ADC,
  SBC,
  AND,
  OR,
  XOR,
  ROT
} operation;

typedef enum {
  REG_REG,
  REG_MEM,
  MEM_REG,
  IMM_REG
} mode;

// This whole struct and all its enums
// should pack into a simple 32-bit integer,
// and should be castable to that.
typedef struct {
	// byte #1
	operation operation:3;
	unsigned int size:1; // 0 = byte, 1 = 16-bit word
	condition condition:2;
	mode mode:2;
	// byte #2
	unsigned int reg:8;
	// bytes #3 and #4; little endian
	union {
		unsigned int address:16; // that's assuming we're on little endian too!
		struct {
      unsigned int lo:8;
			unsigned int hi:8;
		}__attribute__((packed)) addr;
	}__attribute__((packed));
}__attribute__((packed)) __attribute__((aligned)) instruction;

#endif
