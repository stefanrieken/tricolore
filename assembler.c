
/*
Mnemonic:
?LT ADDW $00, $1234

Still to do:
MOV PC, [--SP]
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "instruction.h"

int read_non_whitespace_char(FILE * file) {
	int c = fgetc(file);
	while (c == ' ' || c == '\t' || c == '\r' || c == '\n') c = fgetc(file);
	return c;
}

int next_non_whitespace_char(FILE * file) {
  int c = read_non_whitespace_char(file);
  ungetc(c, file);
  return c;
}

int read_condition(FILE * file) {
	char cond[3] = {'A', 'L', '\0'};

	int c = read_non_whitespace_char(file);
	if (c == '?') {
		cond[0] = toupper(fgetc(file));
		cond[1] = toupper(fgetc(file));
	} else {
    ungetc(c, file);
	}

	if (strcmp(cond, "AL") == 0) return 0b00;
	if (strcmp(cond, "GT") == 0) return 0b01;
	if (strcmp(cond, "LT") == 0) return 0b10;
	if (strcmp(cond, "EQ") == 0) return 0b11;

	printf("Error parsing condition.\n");
	exit(-1);
}

int read_mnemonic(FILE * file) {
	char mnemonic[4];

	mnemonic[0] = toupper(read_non_whitespace_char(file));
	mnemonic[1] = toupper(fgetc(file));
	mnemonic[2] = toupper(fgetc(file));
	mnemonic[3] = '\0';

	if (strcmp(mnemonic, "MOV") == 0) return 0b000;
	if (strcmp(mnemonic, "HUH") == 0) return 0b001;
	if (strcmp(mnemonic, "ADC") == 0) return 0b010;
	if (strcmp(mnemonic, "SBC") == 0) return 0b011;
	if (strcmp(mnemonic, "AND") == 0) return 0b100;
	if (strcmp(mnemonic, "ORR") == 0) return 0b101;
	if (strcmp(mnemonic, "XOR") == 0) return 0b110;
	if (strcmp(mnemonic, "ROT") == 0) return 0b111;

	printf("Error parsing mnemonic. %s\n", mnemonic);
	exit(-1);
}

int read_size(FILE * file) {
	int size = 0;
	int c = fgetc(file);
	if (c == 'W' || c == 'w') {
		size = 1;
	} else if (c != 'B' && c != 'b') {
		ungetc(c, file);
	}
	return size;
}

int read_num(FILE * file, uint16_t * result) {
	*result = 0;
	int c = read_non_whitespace_char(file);

	if (c == '$') {
    // hex
		int size=0;
		c = fgetc(file);
		do {
			if (c >= '0' && c <= '9') {
				*result = (*result << 4) | (c - '0');
			} else if (c >= 'a' && c <= 'f') {
				*result = (*result << 4) | ((c - 'a') + 10);
			} else if (c >= 'A' && c <= 'F') {
				*result = (*result << 4) | ((c - 'A') + 10);
			} else {
				break;
			}
			c = fgetc(file);
			size++;
		} while (1);
		ungetc(c, file);
		return size;
  }
	printf("Error reading number\n");
	exit(-1);
}

uint32_t read_instruction(FILE * file, instruction * instr) {
	instr->condition = read_condition(file);
	instr->operation = read_mnemonic(file);
	instr->size = read_size(file);

	uint16_t dst;
	int size_dst = read_num(file, &dst);
	if (size_dst >2) {
		instr->mode = 0b01; // dest is memory
		instr->address = dst;
	} else {
		instr->reg = dst;
	}

	int c = read_non_whitespace_char(file);
	if (c != ',') {
		printf("Expected ','\n");
		exit(-1);
	}

	uint16_t arg;
	int size_arg;
	c = read_non_whitespace_char(file);

	if (c == '#') {
		if (size_dst > 2) {
			printf("Unsupported mode: immediate-to-memory\n");
			exit(-1);
		}
		instr->mode = 0b11; // immmediate->reg
		size_arg = read_num(file, &arg);
		instr->address = arg;
		if (size_arg > 2) instr->size = 0b1;
	} else if (c == '*') {
		// indirect mode
		size_arg = read_num(file, &arg);
		instr->mode = 0b00;
		instr->addr.lo = arg;
		instr->addr.hi = 0x01; // indirect flag; TODO might use other bits for Post- and pre- In- / decrement + Amount
	} else {
		ungetc(c, file);
		size_arg = read_num(file, &arg);
		if (size_arg >2) {
			if (size_dst > 2) {
				printf("Unsupported mode: memory-to-memory\n");
				exit(-1);
			}
			instr->mode = 0b10; // mem->reg
			instr->address = arg;
		} else {
			if (instr->mode == 0b01) {
				instr->reg = arg; // reg->mem
			} else {
				instr->mode = 0b00; // reg->reg
				instr->addr.lo = arg;
				instr->addr.hi = 0x00; // normal reg->reg; TODO might use other bits for Post- and pre- In- / decrement + Amount
			}
		}
	}

  // now to please the compiler in converting from bitfield struct to integer
  uint32_t * result = (uint32_t *) instr;
  return *result;
}


int main (int argc, char ** argv) {
  instruction instr;

  // ensure that the 'packed' attribute worked out well
  //assert(sizeof(instruction) == 4);

  if (argc < 2) {
    printf("Usage: %s [infile] outfile\n", argv[0]);
    exit(-1);
  }

  FILE * in = argc > 2 ? fopen(argv[1], "r") : stdin;
  char * outfile = argc > 2 ? argv[2] : argv[1];
  FILE * out = fopen(outfile, "wb");

  while (next_non_whitespace_char(in) > 0) {
    uint32_t instruction = read_instruction(in, &instr);
    fwrite(&instruction, sizeof(uint32_t), 1, out);
  }
  fclose(out);
  fclose(in);

  return 0;
}
