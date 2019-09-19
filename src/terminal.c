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
	char *current = data;
	char *end = data + size;

	while(current < end) {
		/* Don't draw anything if we are in the middle of an escape sequence */
		if(!term->esc_buf_len) {
			erase_cursor(term);
			fontlib_DrawStringL(current, end - current);
			current = fontlib_GetLastCharacterRead();
		}

		/* Update the cursor position */
		set_cursor_pos(term, fontlib_GetCursorX() / term->char_width + 1, fontlib_GetCursorY() / term->char_height + 1, false);
		
		/* Add the last read character to the escape sequence buffer */
		term->esc_buf[term->esc_buf_len++] = *current;

		/* Process the escape sequence */
		process_escape_sequence(term);

		current++;
	}
}

void erase_cursor(terminal_state_t *term) {
	gfx_SetColor(bg_color(&(term->graphics)));
	gfx_SetPixel((term->csr_x - 1) * term->char_width, (term->csr_y - 1) * term->char_height);
}

void set_cursor_pos(terminal_state_t *term, uint8_t x, uint8_t y, bool update_fontlib) {
	/* The pixel locations of the new position */
	uint24_t x_px = (x - 1) * term->char_width;
	uint8_t  y_px = (y - 1) * term->char_height;

	erase_cursor(term);

	/* Update the stored position */
	term->csr_x = x;
	term->csr_y = y;

	if(update_fontlib) {
		/* Set the fontlibc cursor position */
		fontlib_SetCursorPosition(x_px, y_px);
	}

	/* Draw the new cursor */
	gfx_SetColor(gfx_red); // temp
	gfx_SetPixel(x_px, y_px);
}

void init_term(terminal_state_t *term) {
	gfx_Begin();
	gfx_SetPalette(gfx_pal, sizeof_gfx_pal, 0);
	gfx_FillScreen(term->graphics.bg_color);
	fontlib_SetWindowFullScreen();
	fontlib_SetCursorPosition(0, 0);
	set_colors(&term->graphics);
	fontlib_SetTransparency(false);
	fontlib_SetFirstPrintableCodePoint(32);
	fontlib_SetNewlineOptions(FONTLIB_ENABLE_AUTO_WRAP | FONTLIB_PRECLEAR_NEWLINE | FONTLIB_AUTO_SCROLL);

	term->cols = LCD_WIDTH / term->char_width;
	term->rows = LCD_HEIGHT / term->char_height;

	term->csr_x = 1;
	term->csr_y = 1;
	set_cursor_pos(term, 1, 1, true);
}
