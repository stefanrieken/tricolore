#include <stdio.h>

int buffer = -1;

int read_char() {
	int result;
	if (buffer != -1) {
		result = buffer;
		buffer = -1;
	} else {
		result = getchar();
	}
	return result;
}

void unread(int c) {
	buffer = c;
}

int read_non_whitespace_char() {
	int c = read_char();
	while (c == ' ' || c == '\t' || c == '\r' || c == '\n') c = read_char();
	return c;
}

