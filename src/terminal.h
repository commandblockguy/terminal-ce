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

typedef struct graphics {
	uint8_t fg_color;
	uint8_t bg_color;
	bool bold        : 1;
	bool blink       : 1;
	bool reverse     : 1;
	bool underline   : 1;
	bool conceal     : 1;
	bool crossed     : 1;
	uint8_t base_col : 3;
} graphics_t;

#define BOLD      (1<<0)
#define BLINK     (1<<1)
#define UNDERLINE (1<<2)
#define CROSSED   (1<<3)
#define REDRAW    (1<<7)

enum Redraw {
    REDRAW_NONE,
    REDRAW_SOME,
    REDRAW_ALL
};

typedef struct {
	char ch;
	uint8_t fg_color;
	uint8_t bg_color;
	uint8_t flags;
} term_char_t;

typedef struct TerminalState {
	uint8_t csr_x;
	uint8_t csr_y;
	uint8_t rows;
	uint8_t cols;
	uint8_t char_width;
	uint8_t char_height;
	term_char_t text_buf[24][80];
	char esc_buf[32];
	uint8_t esc_buf_len;
	uint8_t held_keys[7];
	bool mode_2nd;
	bool mode_alpha;
	uint8_t redraw;
	graphics_t graphics;
	void (*input_callback)(char* pressed, size_t length, void* callback_data);
	void *callback_data;
	terminal_bkp_t backup;
} terminal_state_t;

void write_data(terminal_state_t *term, char *data, size_t size);

/* Mark a position to be redrawn next frame */
void mark_redraw(terminal_state_t *term, uint8_t x, uint8_t y);

void set_cursor_pos(terminal_state_t *term, uint8_t x, uint8_t y);

void init_term(terminal_state_t *term);

#endif