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
		    set_char(term, *current, term->csr_x, term->csr_y);

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

void write_string(terminal_state_t *term, char *str) {
    write_data(term, str, strlen(str));
}

void set_char(terminal_state_t *term, char ch, uint8_t x, uint8_t y) {
    term_char_t *tc = &term->text_buf[y - 1][x - 1];

    tc->ch = ch;

    if(!term->redraw)
        term->redraw = REDRAW_SOME;

    if(term->graphics.reverse) {
        tc->fg_color = term->graphics.bg_color;
        tc->bg_color = term->graphics.fg_color;
    } else {
        tc->fg_color = term->graphics.fg_color;
        tc->bg_color = term->graphics.bg_color;
    }

    tc->flags = REDRAW;
}

void scroll_down(terminal_state_t *term) {
    if(term->csr_y >= term->rows) {
        uint8_t x;
        memcpy(term->text_buf[0], term->text_buf[1], (term->rows - 1) * 80 * sizeof(term_char_t));
        term->redraw = REDRAW_ALL;
        set_cursor_pos(term, term->csr_x, term->rows);
        for(x = 0; x < term->cols; x++) {
            set_char(term, ' ', x, term->csr_y);
        }
    } else if(term->csr_y == term->scroll_bottom) {
        term_char_t *dest = term->text_buf[term->scroll_top - 1];
        term_char_t *src = term->text_buf[term->scroll_top];
        uint8_t x;

        memcpy(dest, src, sizeof(term->text_buf[0]) * (term->scroll_bottom - term->scroll_top));
        term->redraw = REDRAW_ALL;
        set_cursor_pos(term, term->csr_x, term->scroll_bottom);
        for(x = 0; x < term->cols; x++) {
            set_char(term, ' ', x, term->csr_y);
        }
    } else {
        set_cursor_pos(term, term->csr_x, term->csr_y + 1);
    }
}

void mark_redraw(terminal_state_t *term, uint8_t x, uint8_t y) {
    if(!x) return;
    if(!y) return;
    if(x > term->cols) return;
    if(y > term->rows) return;

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
	fontlib_SetNewlineOptions(FONTLIB_ENABLE_AUTO_WRAP);

	term->cols = LCD_WIDTH / term->char_width;
	term->rows = LCD_HEIGHT / term->char_height;

	term->scroll_top = 1;
	term->scroll_bottom = term->rows;

    memset(&term->mode, 0, sizeof(term->mode));
	term->mode.decawm = true;
	term->mode.decarm = true;
	term->mode.dectecm = true;

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
