#ifndef H_TERMINAL
#define H_TERMINAL

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "settings.h"

struct state_backup {
	uint8_t csr_x;
	uint8_t csr_y;
};

struct graphics {
	bool bold        : 1;
	bool blink       : 1;
	bool reverse     : 1;
	bool underline   : 1;
	bool conceal     : 1;
	bool crossed     : 1;
	uint8_t base_col : 3;

	// The number of rows from the beginning of VRAM to the beginning of the screen
	uint24_t view_offset;
};

struct mode {
    bool deccrm   : 1; /* Control character rendering */
    bool decim    : 1; /* Insert mode */
    bool lf_nl    : 1; /* CR after line feed */
    bool decckm   : 1; /* Cursor key mode */
    bool decscnm  : 1; /* Reverse video */
    bool decom    : 1; /* Cursor addressing from scroll region */
    bool decawm   : 1; /* Autowrap mode */
    bool decarm   : 1; /* Autorepeat mode */
    bool dectecm  : 1; /* Cursor visibility */
};

struct terminal_state {
    /* Emulated terminal state */
	uint8_t csr_x;
	uint8_t csr_y;

	uint8_t scroll_top;
	uint8_t scroll_bottom;

    char esc_buf[32];
    uint8_t esc_buf_len;

    struct mode mode;

    struct graphics graphics;
    struct state_backup backup;

    /* Terminal configuration */
    uint8_t rows;
	uint8_t cols;
	uint8_t char_width;
	uint8_t char_height;

    const struct settings *settings;

    void (*input_callback)(const char* pressed, size_t length, void* callback_data);
    void *callback_data;

	/* Emulator state */
	uint8_t held_keys[7];
	bool mode_2nd;
	bool mode_alpha;
};

void write_data(struct terminal_state *term, const char *data, size_t size);
void write_string(struct terminal_state *term, const char *str);
void send_input(struct terminal_state *term, const char *data, size_t len);

void set_cursor_pos(struct terminal_state *term, uint8_t x, uint8_t y);
void update_cursor(struct terminal_state *term);

void scroll_down(struct terminal_state *term);

void init_term(struct terminal_state *term, const struct settings *settings);
void reset_term(struct terminal_state *term);

void send_stty(struct terminal_state *term);

#endif