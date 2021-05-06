#ifndef INSTRUCTION_H
#define INSTRUCTION_H

typedef struct {
	// byte #1
	unsigned int operation:3;
	unsigned int size:1;
	unsigned int condition:2;
	unsigned int mode:2;
	// byte #2
	unsigned int reg:8;
	// bytes #3 and #4; little endian
	union {
		unsigned int address:16; // that's assuming we're on little endian too!
		struct {
			unsigned int lo:8;
			unsigned int hi:8;
		} addr;
	};
}__attribute__((packed)) instruction;

#endif
