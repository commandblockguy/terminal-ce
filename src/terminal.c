#include "terminal.h"
#include <tice.h>

#include <fontlibc.h>
#include <graphx.h>
#include <stdio.h>

#include "escape.h"

#include "graphics.h"

void write_data(struct terminal_state *term, const char *data, size_t size) {
	const char *current;
	const char *end = data + size;

	for(current = data; current < end; current++) {
		/* Don't draw anything if we are in the middle of an escape sequence,
		 * or if the character isn't printable */
		if(!term->esc_buf_len && *current >= ' ') {
            fontlib_DrawGlyph(*current);

            term->csr_x++;

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

    update_cursor(term);
}

void send_input(struct terminal_state *term, const char *data, size_t len) {
    if(len && term->input_callback) {
        (*term->input_callback)(data, len, term->callback_data);
    }
}

void write_string(struct terminal_state *term, const char *str) {
    write_data(term, str, strlen(str));
}

void set_cursor_pos(struct terminal_state *term, uint8_t x, uint8_t y) {
	/* Update the stored position */
	term->csr_x = x;
	term->csr_y = y;
    /* Update fontlib cursor position */
    uint24_t x_px = (x - 1) * term->char_width;
    uint24_t y_px = (y - 1) * term->char_height;
    fontlib_SetCursorPosition(x_px, y_px);
}

void update_cursor(struct terminal_state *term) {
    uint24_t x_pos = (term->csr_x - 1) * term->char_width;
    uint24_t y_pos = (term->csr_y - 1) * term->char_height;

    lcd_CrsrXY = x_pos | ((uint32_t)y_pos << 16);
}

void init_term(struct terminal_state *term, const struct settings *settings) {
    term->settings = settings;

    fontlib_SetColors(WHITE, BLACK);
    term->char_width = fontlib_GetStringWidth(" ");
    term->char_height = fontlib_GetCurrentFontHeight();

	term->cols = LCD_WIDTH / term->char_width;
	term->rows = LCD_HEIGHT / term->char_height;

	term->scroll_top = 1;
	term->scroll_bottom = term->rows;

    memset(&term->mode, 0, sizeof term->mode);
	term->mode.decawm = true;
	term->mode.decarm = true;
	term->mode.dectecm = true;

	term->graphics.view_offset = 0;

    set_cursor_image(term->char_width, term->char_height);

    set_cursor_pos(term, 1, 1);
}

void scroll_down(struct terminal_state *term) {
    // todo: fix this
    if(term->csr_y == term->rows) {
        term->graphics.view_offset++;
        if(term->graphics.view_offset * term->char_height > LCD_HEIGHT) {
            // Shift the view back up to the base of VRAM
            memcpy(gfx_vram, mpLcdBase, LCD_WIDTH * LCD_HEIGHT);
            memset(gfx_vram + LCD_SIZE / 2, fontlib_GetBackgroundColor(), LCD_SIZE / 2);
            term->graphics.view_offset = 0;
        }
        update_view_pos(term);
    } else {
        set_cursor_pos(term, term->csr_x, term->csr_y + 1);
    }
}

void send_stty(struct terminal_state *term) {
    char buf[50];
    size_t len = sprintf(buf, "stty rows %2u cols %2u\n",
                         term->rows, term->cols);
    send_input(term, buf, len);
}
