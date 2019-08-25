#ifndef H_TERMINAL
#define H_TERMINAL

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct TerminalState {
	uint8_t csr_x;
	uint8_t csr_y;
	uint8_t rows;
	uint8_t cols;
	char esc_buf[32];
	uint8_t held_keys[7];
	bool mode_2nd;
	bool mode_alpha;
	void (*input_callback)(char* pressed, size_t length, void* callback_data);
	void *callback_data;
} terminal_state_t;

void write_data(terminal_state_t *term, char *data, size_t size);

#endif