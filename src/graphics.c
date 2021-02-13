#include "graphics.h"
#include "gfx/gfx.h"

#include <stdbool.h>
#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>

#include <graphx.h>
#include <debug.h>
#include <tice.h>
#include <fontlibc.h>

#define lcd_CrsrImage ((uint8_t*)0xE30800)
#define lcd_CrsrImageLen 256

void init_graphics(void) {
    gfx_Begin();
    gfx_SetPalette(gfx_pal, sizeof_gfx_pal, 0);
    // Fill the entire buffer with the background color
    memset(gfx_vram, 0, LCD_SIZE);
    fontlib_SetWindowFullScreen();
    fontlib_SetCursorPosition(0, 0);
    fontlib_SetTransparency(false);
    fontlib_SetFirstPrintableCodePoint(32);
    fontlib_SetNewlineOptions(FONTLIB_ENABLE_AUTO_WRAP);

    lcd_Timing2 = (uint32_t)(lcd_Timing2 & ~(uint32_t)0x03FF0000) | (uint32_t)(LCD_WIDTH - 1) << 16;

    lcd_CrsrConfig = 0; // select 32x32, image0
    lcd_CrsrPalette0 = 0x000000; // set black palette color
    lcd_CrsrPalette1 = 0xFFFFFF; // set white palette color
    lcd_CrsrXY = 0; // reset cursor position
    lcd_CrsrClip = 0; // reset clipping
    lcd_CrsrCtrl = 1; // enable cursor
}

void cleanup_graphics(void) {
    lcd_CrsrCtrl = 0; // disable cursor
    // reset timings to their defaults
    lcd_Timing2 = (uint32_t)(lcd_Timing2 & ~(uint32_t)0x03FF0000) | (uint32_t)(LCD_HEIGHT - 1) << 16;
    gfx_End();
}

void set_cursor_image(uint8_t width, uint8_t height) {
    // todo: maybe optimize this
    memset(lcd_CrsrImage, 0b10101010, lcd_CrsrImageLen);
    for(uint8_t y = 0; y < height; y++) {
        for(uint8_t x = 0; x < width; x++) {
            lcd_CrsrImage[x / 4 + 8 * y] |= 0b11000000 >> ((x % 4) * 2);
        }
    }
}

void sgr(struct terminal_state *term, const uint24_t *args) {
	struct graphics *graphics = &term->graphics;

	if(30 <= args[0] && args[0] <= 37) {
		graphics->base_col = args[0] - 30;
		update_fg_color(graphics);
		return;
	}

	if(40 <= args[0] && args[0] <= 47) {
		fontlib_SetBackgroundColor(args[0] - 40);
		return;
	}

	switch(args[0]) {
		case 0: /* Reset all attributes */
			graphics->bold = false;
			graphics->reverse = false;
			graphics->underline = false;
			graphics->conceal = false;
			graphics->crossed = false;
			graphics->base_col = BASE_WHITE;
			update_fg_color(graphics);
            fontlib_SetBackgroundColor(BLACK);
			break;

		case 1: /* Set bold */
			graphics->bold = true;
            update_fg_color(graphics);
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
            update_fg_color(graphics);
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
				fontlib_SetForegroundColor(args[2]);

			} else if(args[1] == 2) {
				/* 24-bit true color */
				uint8_t color = true_color_to_palette(args[2], args[3], args[4]);
				fontlib_SetForegroundColor(color);

			} else {
				dbg_sprintf(dbgerr, "Got a weird format (%u) for SGR 38\n", args[1]);
			}
			return;

		case 39:
			graphics->base_col = BASE_WHITE;
            update_fg_color(graphics);
			break;

		case 48:
			if(args[1] == 5) {
				/* 8-bit palette */
				fontlib_SetBackgroundColor(args[2]);

			} else if(args[1] == 2) {
				/* 24-bit true color */
				uint8_t color = true_color_to_palette(args[2], args[3], args[4]);
                fontlib_SetBackgroundColor(color);

			} else {
				dbg_sprintf(dbgerr, "Got a weird format (%u) for SGR 48\n", args[1]);
			}
			return;

		case 49:
            fontlib_SetBackgroundColor(BLACK);
			return;

		default: 
			dbg_sprintf(dbgerr, "Unknown SGR argument %u\n", args[0]);
			return; /* Skip updating colors */
	}
}

void update_fg_color(struct graphics *graphics) {
	fontlib_SetForegroundColor(graphics->base_col | graphics->bold << 3);
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

void set_char_at(struct terminal_state *term, char c, uint8_t x, uint8_t y) {
    uint24_t x_px = (x - 1) * term->char_width;
    uint24_t y_px = (y - 1) * term->char_height;
    fontlib_SetCursorPosition(x_px, y_px);
    fontlib_DrawGlyph(c);
}

void erase_chars(struct terminal_state *term, uint8_t start_x, uint8_t end_x, uint8_t y) {
    for(uint8_t x = start_x; x <= end_x; x++) {
        set_char_at(term, ' ', x, y);
    }
}

void delete_chars(struct terminal_state *term, uint8_t x, uint8_t y, uint8_t amount) {
    uint24_t src_x = (x + amount) * term->char_width;
    uint24_t dst_x = x * term->char_width;
    uint8_t src_y = y * term->char_height;
    uint24_t width = amount * term->char_width;
    gfx_CopyRectangle(gfx_screen, gfx_screen, src_x, src_y, dst_x, src_y, width, term->char_height);
    erase_chars(term, x + amount, term->cols, y);
}

void update_view_pos(struct terminal_state *term) {
    uint24_t lines = term->char_height * term->graphics.view_offset;
    CurrentBuffer = mpLcdBase = gfx_vram + 320 * lines;
}
