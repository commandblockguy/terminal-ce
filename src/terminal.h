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

typedef struct StateBackup {
	uint8_t csr_x;
	uint8_t csr_y;
} terminal_bkp_t;

typedef struct TerminalState {
	uint8_t csr_x;
	uint8_t csr_y;
	uint8_t rows;
	uint8_t cols;
	uint8_t char_width;
	uint8_t char_height;
	char esc_buf[32];
	uint8_t esc_buf_len;
	uint8_t held_keys[7];
	bool mode_2nd;
	bool mode_alpha;
	bool mode_ctrl;
	void (*input_callback)(char* pressed, size_t length, void* callback_data);
	void *callback_data;
	terminal_bkp_t backup;
} terminal_state_t;

void write_data(terminal_state_t *term, char *data, size_t size);

void erase_cursor(terminal_state_t *term);

void set_cursor_pos(terminal_state_t *term, uint8_t x, uint8_t y, bool update_fontlib);

#endif