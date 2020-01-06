#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fontlibc.h>
#include <graphx.h>
#include <debug.h>

#include "terminal.h"
#include "escape.h"
#include "graphics.h"

#include "gfx/gfx.h"

void write_data(terminal_state_t *term, char *data, size_t size) {
	char *current;
	char *end = data + size;

	for(current = data; current < end; current++) {
		/* Don't draw anything if we are in the middle of an escape sequence,
		 * or if the character isn't printable */
		if(!term->esc_buf_len && *current >= ' ') {
		    term_char_t *ch = &term->text_buf[term->csr_y - 1][term->csr_x - 1];

		    ch->ch = *current;

		    if(!term->redraw)
                term->redraw = REDRAW_SOME;

		    if(term->graphics.reverse) {
                ch->fg_color = term->graphics.bg_color;
                ch->bg_color = term->graphics.fg_color;
            } else {
                ch->fg_color = term->graphics.fg_color;
                ch->bg_color = term->graphics.bg_color;
            }

            set_cursor_pos(term, term->csr_x + 1, term->csr_y);

		    if(term->csr_x > term->cols) {
		        set_cursor_pos(term, 1, term->csr_y + 1);
		    }
		    if(term->csr_y > term->rows) {
                set_cursor_pos(term, term->csr_x, term->rows);
		        term->csr_y = term->rows;
		        memcpy(term->text_buf[0], term->text_buf[1], sizeof(term->text_buf) - sizeof(term->text_buf[0]));
		        term->redraw = REDRAW_ALL;
		    }
		    ch->flags = REDRAW;
		}
		
		/* Add the last read character to the escape sequence buffer */
		term->esc_buf[term->esc_buf_len++] = *current;

		/* Process the escape sequence */
		process_escape_sequence(term);

		current++;
	}
}

void mark_redraw(terminal_state_t *term, uint8_t x, uint8_t y) {
    term->text_buf[y - 1][x - 1].flags |= REDRAW;
    if(!term->redraw)
        term->redraw = REDRAW_SOME;
}

void set_cursor_pos(terminal_state_t *term, uint8_t x, uint8_t y) {
	/* Redraw the old position */
	if(x >= 1 && x <= term->cols && y >= 1 && y <= term->rows)
        mark_redraw(term, term->csr_x, term->csr_y);

	/* Update the stored position */
	term->csr_x = x;
	term->csr_y = y;

	/* Redraw the new position */
    mark_redraw(term, x, y);
}

void init_term(terminal_state_t *term) {
    uint8_t row, col;
	gfx_Begin();
	gfx_SetPalette(gfx_pal, sizeof_gfx_pal, 0);
	gfx_FillScreen(term->graphics.bg_color);
	fontlib_SetWindowFullScreen();
	fontlib_SetCursorPosition(0, 0);
	fontlib_SetTransparency(false);
	fontlib_SetFirstPrintableCodePoint(32);
	fontlib_SetNewlineOptions(FONTLIB_ENABLE_AUTO_WRAP | FONTLIB_PRECLEAR_NEWLINE | FONTLIB_AUTO_SCROLL);

	term->cols = LCD_WIDTH / term->char_width;
	term->rows = LCD_HEIGHT / term->char_height;

    set_cursor_pos(term, 1, 1);

    for(row = 0; row < term->rows; row++) {
        for(col = 0; col < term->cols; col++) {
            term_char_t *ch = &term->text_buf[row][col];

            ch->ch = ' ';
            ch->fg_color = term->graphics.fg_color;
            ch->bg_color = term->graphics.bg_color;
            ch->flags = 0;
        }
	}

    term->redraw = REDRAW_ALL;
}
