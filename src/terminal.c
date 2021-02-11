#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

#include <string.h>

#include <fontlibc.h>
#include <graphx.h>

#include "terminal.h"
#include "escape.h"

#include "gfx/gfx.h"
#include "graphics.h"

void write_data(terminal_state_t *term, const char *data, size_t size) {
	const char *current;
	const char *end = data + size;

	for(current = data; current < end; current++) {
		/* Don't draw anything if we are in the middle of an escape sequence,
		 * or if the character isn't printable */
		if(!term->esc_buf_len && *current >= ' ') {
            set_char_at(term, *current, term->csr_x, term->csr_y);

            set_cursor_pos(term, term->csr_x + 1, term->csr_y);

            if(term->csr_x > term->cols) {
                if(term->mode.decawm) {
                    scroll_down(term);
                    set_cursor_pos(term, 1, term->csr_y);
                } else {
                    set_cursor_pos(term, term->cols, term->csr_y);
                }
            }
		}
		
		/* Add the last read character to the escape sequence buffer */
		term->esc_buf[term->esc_buf_len++] = *current;

		/* Process the escape sequence */
		process_escape_sequence(term);
	}
}

void write_string(terminal_state_t *term, const char *str) {
    write_data(term, str, strlen(str));
}

void set_cursor_pos(terminal_state_t *term, uint8_t x, uint8_t y) {
	/* Update the stored position */
	term->csr_x = x;
	term->csr_y = y;
}

void init_term(terminal_state_t *term) {
	gfx_Begin();
	gfx_SetPalette(gfx_pal, sizeof_gfx_pal, 0);
	gfx_FillScreen(BLACK);
	fontlib_SetWindowFullScreen();
	fontlib_SetCursorPosition(0, 0);
	fontlib_SetTransparency(false);
	fontlib_SetFirstPrintableCodePoint(32);
	fontlib_SetNewlineOptions(FONTLIB_ENABLE_AUTO_WRAP);
	fontlib_SetColors(WHITE, BLACK);

	term->cols = LCD_WIDTH / term->char_width;
	term->rows = LCD_HEIGHT / term->char_height;

	term->scroll_top = 1;
	term->scroll_bottom = term->rows;

    memset(&term->mode, 0, sizeof term->mode);
	term->mode.decawm = true;
	term->mode.decarm = true;
	term->mode.dectecm = true;

    set_cursor_pos(term, 1, 1);
}

void scroll_down(terminal_state_t *term) {
    if(term->csr_y == term->rows) {
        fontlib_ScrollWindowDown();
        erase_chars(term, 1, term->cols, term->rows);
    } else {
        term->csr_y++;
    }
}
