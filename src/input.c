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
#include "escape.h"
#include "menu.h"

#include "graphics.h"

#define KEY_CHAR_COL_OFFSET 1

const char key_chars_std  [5][8] = {
	"\0\0\0\0\0\0#\0",
	"0147,@\0\x8",
	".258(<\0\0",
	"-369)>\0\0",
	"\xA+-*/^\0\0"
};

const char key_chars_2nd  [5][8] = {
	"\0\0\0\0\0\0=\0",
	"\0!$&`\0\0\0",
	"|@%*{\0\0$",
	"~#^\0}\0\0\0",
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
	"?\0vqlg\0\0",
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

	/* Handle F5 (menu) key */
	if(get_key(keys, kb_KeyGraph)) {
		menu(term);
		return;	
	}

	/* Handle 2nd key */
	if(get_key(keys, kb_Key2nd)) {
		term->mode_2nd = !term->mode_2nd;

		gfx_SetColor(bg_color(&(term->graphics)));
		if(term->mode_2nd) gfx_SetColor(COL_2ND);

		gfx_HorizLine(0, LCD_HEIGHT - 1, LCD_WIDTH / 2);
	}

	/* Handle alpha key */
	if(get_key(keys, kb_KeyAlpha)) {
		term->mode_alpha = !term->mode_alpha;

		gfx_SetColor(bg_color(&(term->graphics)));
		if(term->mode_alpha) gfx_SetColor(COL_ALPHA);
		
		gfx_HorizLine(LCD_WIDTH / 2, LCD_HEIGHT - 1, LCD_WIDTH / 2);
	}

	/* Handle arrow key presses */
	for(i = 0; i < 4; i++) {
		if(keys[6] & (1 << i)) {
			const char codes[4] = "BDCA";
			const size_t len_diff = strlen(CSI_SEQ) + 1;

			/* Break if there is no room for the arrow key sequence */
			if(len + len_diff > 24) break;

			/* Output the CSI escape sequence */
			memcpy(&buf[len], CSI_SEQ, strlen(CSI_SEQ));

			/* Output the character that corresponds to this arrow key */
			buf[len + strlen(CSI_SEQ)] = codes[i];

			len += len_diff;
		}
	}

	/* Handle regular keypresses */
	/* Check each keypad group */
	for(i = 0; i < 6 - KEY_CHAR_COL_OFFSET; i++) {
		int j;
		/* Check each bit of the group */
		for(j = 0; j < 8; j++) {
			/* If there is no room, stop */
			if(len >= 24) goto skip_input;

			/* Check if key is pressed */
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

				/* Check if F1, the "ctrl key" is pressed */
				if(kb_IsDown(kb_KeyYequ)) {
					if(val >= 'a' && val <= 'z') val = val - 'a' + 1;
					else if(val >= 'A' && val <= '[') val = val - 'A' + 1;
					else {
						dbg_sprintf(dbgout, "Bad ctrl char '%c'\n", val);
						val = 0;
					}
				}

				if(val) {
					buf[len] = val;
					len++;
				}
			}
		}
	}

	if(get_key(keys, kb_KeyDel)) {
		const char seq_del[] = CSI_SEQ "3~";
		const size_t len_diff = strlen(seq_del);

		/* If there is room for the sequence */
		if(len + len_diff <= 24) {
			/* Output the CSI escape sequence for delete */
			memcpy(&buf[len], seq_del, len_diff);

			len += len_diff;
		}
	}

	skip_input:

	buf[len] = 0;

	if(len && term->input_callback) {
		(*term->input_callback)(buf, len, term->callback_data);
	}

}
