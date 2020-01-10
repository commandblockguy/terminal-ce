#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fontlibc.h>
#include <debug.h>

#include "graphics.h"
#include "terminal.h"

void render(terminal_state_t *term) {
    uint8_t row;

    if(!term->redraw) return;

    for(row = 0; row < term->rows; row++) {
        uint8_t col;
        term_char_t *ch = term->text_buf[row];
        uint8_t y_pos = row * term->char_height;

        for(col = 0; col < term->cols; col++, ch++) {
            bool inverse;
            if(term->redraw != REDRAW_ALL && !(ch->flags & BLINK) && !(ch->flags & REDRAW)) continue;

            inverse = term->mode.dectecm && col == term->csr_x - 1 && row == term->csr_y - 1;

            if(term->mode.decscnm) inverse = !inverse;

            if(inverse) {
                fontlib_SetColors(ch->bg_color, ch->fg_color);
            } else {
                fontlib_SetColors(ch->fg_color, ch->bg_color);
            }
            fontlib_SetCursorPosition(col * term->char_width, y_pos);

            //todo: blink, underline, etc.
            fontlib_DrawGlyph(ch->ch);

            /* Unset redraw bit */
            ch->flags &= ~REDRAW;
        }
    }

    //todo: draw cursor

    term->redraw = false;
}

void sgr(terminal_state_t *term, uint24_t *args) {
	graphics_t *graphics = &term->graphics;

	if(30 <= args[0] && args[0] <= 37) {
		graphics->base_col = args[0] - 30;
		graphics->fg_color = get_fg_color(graphics);
		return;
	}

	if(40 <= args[0] && args[0] <= 47) {
		graphics->bg_color = args[0] - 40;
		return;
	}

	switch(args[0]) {
		case 0: /* Reset all attributes */
			graphics->bg_color = BLACK;
			graphics->bold = false;
			graphics->reverse = false;
			graphics->underline = false;
			graphics->conceal = false;
			graphics->crossed = false;
			graphics->base_col = WHITE;
			break;

		case 1: /* Set bold */
			graphics->bold = true;
			break;

		case 4: /* Set underline */
			graphics->underline = true;
			return;

		case 7: /* Inverted graphics */
			graphics->reverse = true;
			return;

		case 8: /* Conceal */
			graphics->conceal = true;
			return;

		case 9: /* Crossed-out */
			graphics->crossed = true;
			return;

		case 22: /* Reset intensity */
			graphics->bold = false;
			break;

		case 24: /* Reset underline */
			graphics->underline = false;
			return;

		case 27: /* Non-inverted graphics */
			graphics->reverse = false;
			return;

		case 28: /* Reveal */
			graphics->conceal = false;
			return;

		case 29: /* Reset crossout */
			graphics->crossed = false;
			return;

		case 38:
			if(args[1] == 5) {

				/* 8-bit palette */
				graphics->fg_color = args[2];

			} else if(args[1] == 2) {

				/* 24-bit true color */
				graphics->fg_color = true_color_to_palette(args[2], args[3], args[4]);

			} else {
				dbg_sprintf(dbgerr, "Got a weird format (%u) for SGR 38\n", args[1]);
			}
			return;

		case 39:
			graphics->base_col = WHITE;
			break;

		case 48:
			if(args[1] == 5) {

				/* 8-bit palette */
				graphics->bg_color = args[2];

			} else if(args[1] == 2) {

				/* 24-bit true color */
				graphics->bg_color = true_color_to_palette(args[2], args[3], args[4]);

			} else {
				dbg_sprintf(dbgerr, "Got a weird format (%u) for SGR 48\n", args[1]);
			}
			return;

		case 49:
			graphics->bg_color = BLACK;
			return;

		default: 
			dbg_sprintf(dbgerr, "Unknown SGR argument %u\n", args[0]);
			return; /* Skip updating colors */
	}

	graphics->fg_color = get_fg_color(graphics);
}

uint8_t get_fg_color(graphics_t *graphics) {
	return graphics->base_col | graphics->bold << 3;
}

uint8_t true_color_to_palette(uint8_t r, uint8_t g, uint8_t b) {

	/* I would add a relevant comment here but I don't know much about how color works */
	/* Let's hope this does what I think it does */
	uint8_t greyscale_value = ((uint24_t)r + g + b) / 3;


	/* Check if color is greyscale-ish */
	if(
		abs(r - greyscale_value) < (256 / CUBE_SIZE) &&
		abs(g - greyscale_value) < (256 / CUBE_SIZE) &&
		abs(b - greyscale_value) < (256 / CUBE_SIZE)) {
		/* Find the closest greyscale color */

		/* Rescale the 256-value color space into 24-value */
		uint8_t scaled = (uint24_t)greyscale_value * NUM_GREYSCALE / 256;

		/* Add the rescaled value to the value of the first greyscale palette value */
		return scaled + GREYSCALE_START;
	} else {
		/* Color is not greyscale - unleash the C̷̨̙͎̻͇̤̬̤͍̦̺͎̘̋̅̽̽̃̕͘͝͝ͅͅṶ̴͐͆͝B̸̗̗̻͍̼̍̎̈́̍͘Ȩ̴̧͚͇̮͛͗̀͒̆͂͒̎̾͊̕͝͝͠ */

		/* Scale color components properly */
		uint8_t red   = (uint24_t)r * CUBE_SIZE / 256;
		uint8_t green = (uint24_t)g * CUBE_SIZE / 256;
		uint8_t blue  = (uint24_t)b * CUBE_SIZE / 256;

		/* CONSTRUCT the C̷̨̙͎̻͇̤̬̤͍̦̺͎̘̋̅̽̽̃̕͘͝͝ͅͅṶ̴͐͆͝B̸̗̗̻͍̼̍̎̈́̍͘Ȩ̴̧͚͇̮͛͗̀͒̆͂͒̎̾͊̕͝͝͠ */
		return CUBE_START + red * CUBE_SIZE * CUBE_SIZE + green * CUBE_SIZE + blue;
	}

}

uint8_t bg_color(graphics_t *graphics) {
	if(graphics->reverse) {
		return graphics->fg_color;
	} else {
		return graphics->bg_color;
	}
}
