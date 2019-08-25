#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <keypadc.h>
#include <graphx.h>
#include <fontlibc.h>
#include <debug.h>

#include "terminal.h"

#define KEY_CHAR_COL_OFFSET 1

const char key_chars_std  [5][8] = {
	"\0\0\0\0\0\0#\0",
	"0147,@\0\0",
	".258(<\0\0",
	"-369)>\0\0",
	"\xA+-*/^\0\0"
};

const char key_chars_2nd  [5][8] = {
	"\0\0\0\0\0\0=\0",
	"%&\0\0`\0\0\0",
	"|\0\0\0{\0\0$",
	"~\0\0\0}\0\0\0",
	"\xA'][\\\0\0\0"
};

const char key_chars_upper[5][8] = {
	"\0XSNIDA\0",
	" YTOJEB\0",
	":ZUPKFC\0",
	"?\0VQLG\0\0",
	"\xA\"WRMH\0\0"
};

const char key_chars_lower[5][8] = {
	"\0xsnida\0",
	" ytojeb\0",
	";zupkfc\0",
	"!\0vqlg\0\0",
	"\xA\"wrmh\0\0"
};

#define get_key(keys, lkey) \
(keys[((lkey) >> 8) - 1] & (lkey))

void process_input(terminal_state_t *term) {
	uint8_t i;
	kb_key_t keys[7];

	char buf[25];
	uint8_t len = 0;

	kb_Scan();

	for(i = 0; i < 7; i++) {
		keys[i] = kb_Data[i + 1] & ~term->held_keys[i];
		term->held_keys[i] = kb_Data[i + 1];
	}

	/* Handle 2nd key */
	if(get_key(keys, kb_Key2nd)) {
		term->mode_2nd = !term->mode_2nd;
		dbg_sprintf(dbgout, "2nd mode: %u\n", term->mode_2nd);

		gfx_SetColor(gfx_black);
		if(term->mode_2nd) gfx_SetColor(0x3D);

		gfx_HorizLine(0, LCD_HEIGHT - 1, LCD_WIDTH / 2);
	}

	/* Handle alpha key */
	if(get_key(keys, kb_KeyAlpha)) {
		term->mode_alpha = !term->mode_alpha;
		dbg_sprintf(dbgout, "Alpha mode: %u\n", term->mode_alpha);

		gfx_SetColor(gfx_black);
		if(term->mode_alpha) gfx_SetColor(0x27);
		
		gfx_HorizLine(LCD_WIDTH / 2, LCD_HEIGHT - 1, LCD_WIDTH / 2);
	}

	for(i = 0; i < 7 - KEY_CHAR_COL_OFFSET; i++) {
		int j;
		for(j = 0; j < 8; j++) {
			if(len >= 24) goto skip_input;
			if(keys[i + KEY_CHAR_COL_OFFSET] & (1 << j)) {
				char (*key_chars)[8];
				char val;
				if(term->mode_2nd) {
					if(term->mode_alpha) key_chars = key_chars_lower;
					else key_chars = key_chars_2nd;
				} else {
					if(term->mode_alpha) key_chars = key_chars_upper;
					else key_chars = key_chars_std;
				}

				val = key_chars[i][j];

				if(val) {
					buf[len] = val;
					len++;
				}
			}
		}
	}
	skip_input:

	buf[len] = 0;

	if(term->input_callback) {
		(*term->input_callback)(buf, len, term->callback_data);
	}

}
