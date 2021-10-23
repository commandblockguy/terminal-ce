#include "input.h"

#include <stdio.h>

#include <keypadc.h>
#include <debug.h>

#include "escape.h"
#include "menu.h"

#define KEY_CHAR_COL_OFFSET 1

const char key_chars_std[5][8] = {
	"\0\0\0\0\0\0#\0",
	"0147,@\0\x08",
	".258(<\0\0",
	"-369)>\0\0",
	"\x0D+-*/^\0\0"
};

const char key_chars_2nd[5][8] = {
	"\0\0\0\0\0\0=\0",
	"\0!$&`\0\0\0",
	"|@%*{\0\0$",
	"~#^\0}\0\0\0",
	"\x0D'][\\\0\0\0"
};

const char key_chars_upper[5][8] = {
	"\0XSNIDA\0",
	" YTOJEB\0",
	":ZUPKFC\0",
	"?\0VQLG\0\0",
	"\x0D\"WRMH\0\0"
};

const char key_chars_lower[5][8] = {
	"\0xsnida\0",
	" ytojeb\0",
	";zupkfc\0",
	"?\0vqlg\0\0",
	"\x0D\"wrmh\0\0"
};

#define get_key(keys, lkey) ((keys)[((lkey) >> 8) - 1] & (lkey))

static const char (*get_key_chars(struct terminal_state *term))[8]  {
    if(term->mode_2nd) {
        if(term->mode_alpha) return key_chars_lower;
        else return key_chars_2nd;
    } else {
        if(term->mode_alpha) return key_chars_upper;
        else return key_chars_std;
    }
}

static char convert_to_ctrl(char val) {
    if(val >= 'a' && val <= 'z')
        return val - 'a' + 1;
    else if(val >= 'A' && val <= '[')
        return val - 'A' + 1;
    else {
        dbg_sprintf(dbgerr, "Bad ctrl char '%c'\n", val);
        return 0;
    }
}

void send_input(struct terminal_state *term, const char *data, size_t len) {
    if(len && term->input_callback) {
        (*term->input_callback)(data, len, term->callback_data);
    }
}

static void handle_arrow_keys(struct terminal_state *term, const kb_key_t *keys) {
    for(uint8_t i = 0; i < 4; i++) {
        if(keys[6] & (1 << i)) {
            const char codes[4] = "BDCA";
            char seq[3] = CSI_SEQ;

            /* Output the CSI escape sequence */
            seq[2] = codes[i];

            send_input(term, seq, sizeof seq);
        }
    }
}

static void send_stty(struct terminal_state *term) {
    char buf[50];
    size_t len = sprintf(buf, "stty rows %2u cols %2u\n",
                         term->rows, term->cols);
    send_input(term, buf, len);
}

void process_input(struct terminal_state *term) {
	kb_Scan();

    kb_key_t keys[7];
	for(uint8_t i = 0; i < 7; i++) {
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
		// todo: add visual indicator
	}

	/* Handle alpha key */
	if(get_key(keys, kb_KeyAlpha)) {
		term->mode_alpha = !term->mode_alpha;
        // todo: add visual indicator
	}

    handle_arrow_keys(term, keys);

    /* Handle F1+window key (send stty command) */
    if(kb_IsDown(kb_KeyYequ) && get_key(keys, kb_KeyWindow)) {
        send_stty(term);
    }

	/* Handle regular keypresses */
	/* Check each keypad group */
	for(uint8_t i = 0; i < 6 - KEY_CHAR_COL_OFFSET; i++) {
		/* Check each bit of the group */
		for(int j = 0; j < 8; j++) {
			/* Check if key is pressed */
			if(keys[i + KEY_CHAR_COL_OFFSET] & (1 << j)) {
				const char (*key_chars)[8] = get_key_chars(term);

				char val = key_chars[i][j];

				/* Check if F1, the "ctrl key" is pressed */
				if(kb_IsDown(kb_KeyYequ)) {
					val = convert_to_ctrl(val);
				}

				if(val) {
                    send_input(term, &val, 1);
				}
			}
		}
	}

	if(get_key(keys, kb_KeyDel)) {
		const char seq_del[] = CSI_SEQ "3~";
        send_input(term, seq_del, strlen(seq_del));
	}
}
