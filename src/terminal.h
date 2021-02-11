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

typedef struct {
    bool deccrm   : 1; /* Control character rendering */
    bool decim    : 1; /* Insert mode */
    bool lf_nl    : 1; /* CR after line feed */
    bool decckm   : 1; /* Cursor key mode */
    bool decscnm  : 1; /* Reverse video */
    bool decom    : 1; /* Cursor addressing from scroll region */
    bool decawm   : 1; /* Autowrap mode */
    bool decarm   : 1; /* Autorepeat mode */
    bool dectecm  : 1; /* Cursor visibility */
} mode_t;

typedef struct TerminalState {
    /* Emulated terminal state */
	uint8_t csr_x;
	uint8_t csr_y;

	uint8_t scroll_top;
	uint8_t scroll_bottom;

    char esc_buf[32];
    uint8_t esc_buf_len;

    mode_t mode;

    graphics_t graphics;
    terminal_bkp_t backup;


    /* Terminal configuration */
    uint8_t rows;
	uint8_t cols;
	uint8_t char_width;
	uint8_t char_height;

    void (*input_callback)(char* pressed, size_t length, void* callback_data);
    void *callback_data;


	/* Emulator state */
	uint8_t held_keys[7];
	bool mode_2nd;
	bool mode_alpha;
} terminal_state_t;

void write_char(terminal_state_t *term, char c);
void write_data(terminal_state_t *term, const char *data, size_t size);
void write_string(terminal_state_t *term, const char *str);

void set_cursor_pos(terminal_state_t *term, uint8_t x, uint8_t y);

void scroll_down(terminal_state_t *term);

void init_term(terminal_state_t *term);

#endif