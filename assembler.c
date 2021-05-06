
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
#include "parse.h"

int read_condition() {
	char cond[3] = {'A', 'L', '\0'};

	int c = read_non_whitespace_char();
	if (c == '?') {
		cond[0] = toupper(read_char());
		cond[1] = toupper(read_char());
	} else {
		unread(c);
	}

	if (strcmp(cond, "AL") == 0) return 0b00;
	if (strcmp(cond, "GT") == 0) return 0b01;
	if (strcmp(cond, "LT") == 0) return 0b10;
	if (strcmp(cond, "EQ") == 0) return 0b11;

	printf("Error parsing condition.\n");
	exit(-1);
}

int read_mnemonic() {
	char mnemonic[4];

	mnemonic[0] = toupper(read_non_whitespace_char());
	mnemonic[1] = toupper(read_char());
	mnemonic[2] = toupper(read_char());
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

int read_size() {
	int size = 0;
	int c = read_char();
	if (c == 'W' || c == 'w') {
		size = 1;
	} else if (c != 'B' && c != 'b') {
		unread(c);
	}
	return size;
}

int read_num(uint16_t * result) {
	int c = read_non_whitespace_char();
	if (c == '$') {
		int size=0;
		c = read_char();
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
			c = read_char();
			size++;
		} while (1);
		unread(c);
		return size;
	}
	printf("Error reading number\n");
	exit(-1);
}

int main (int argc, char ** argv) {
	instruction * instr = malloc(sizeof(instruction));
	
	instr->condition = read_condition();
	instr->operation = read_mnemonic();
	instr->size = read_size();

	uint16_t dst;
	int size_dst = read_num(&dst);
	if (size_dst >2) {
		instr->mode = 0b01; // dest is memory
		instr->address = dst;
	} else {
		instr->reg = dst;
	}

	int c = read_non_whitespace_char();
	if (c != ',') {
		printf("Expected ','\n");
		exit(-1);
	}

	uint16_t arg;
	int size_arg;
	c = read_non_whitespace_char();
	if (c == '#') {
		if (size_dst > 2) {
			printf("Unsupported mode: immediate-to-memory\n");
			exit(-1);
		}
		instr->mode = 0b11; // immmediate->reg
		size_arg = read_num(&arg);
		instr->address = arg;
		if (size_arg > 2) instr->size = 0b1;
	} else if (c == '*') {
		// indirect mode
		size_arg = read_num(&arg);
		instr->mode = 0b00;
		instr->addr.lo = arg;
		instr->addr.hi = 0x01; // indirect flag; TODO might use other bits for Post- and pre- In- / decrement + Amount
	} else {
		unread(c);
		size_arg = read_num(&arg);
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

	uint32_t * result = (uint32_t *) instr;
	printf("Result: $%08x\n", *result);
}
