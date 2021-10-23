#include "input.h"

#include <stdio.h>

#include <keypadc.h>
#include <debug.h>

#include "escape.h"
#include "menu.h"

#define KEY(key, val) [((key) >> 8) - 1][__builtin_ctz((key) & 0xFF)] = (val)

const char key_chars_std[6][8] = {
        // As labeled
        KEY(kb_Key0, '0'),
        KEY(kb_Key1, '1'),
        KEY(kb_Key2, '2'),
        KEY(kb_Key3, '3'),
        KEY(kb_Key4, '4'),
        KEY(kb_Key5, '5'),
        KEY(kb_Key6, '6'),
        KEY(kb_Key7, '7'),
        KEY(kb_Key8, '8'),
        KEY(kb_Key9, '9'),
        KEY(kb_KeyDecPnt, '.'),
        KEY(kb_KeyChs, '-'),
        KEY(kb_KeyAdd, '+'),
        KEY(kb_KeySub, '-'),
        KEY(kb_KeyMul, '*'),
        KEY(kb_KeyDiv, '/'),
        KEY(kb_KeyPower, '^'),
        KEY(kb_KeyComma, ','),
        KEY(kb_KeyLParen, '('),
        KEY(kb_KeyRParen, ')'),
        KEY(kb_KeyEnter, '\n'),
        // From Python app
        KEY(kb_KeySto, '='),
        // Custom
        KEY(kb_KeySin, '@'),
        KEY(kb_KeyCos, '<'),
        KEY(kb_KeyTan, '>'),
        KEY(kb_KeyMode, '\b'),
};

const char key_chars_2nd[6][8] = {
        // As labeled
        KEY(kb_KeyLParen, '{'),
        KEY(kb_KeyRParen, '}'),
        KEY(kb_KeyMul, '['),
        KEY(kb_KeySub, ']'),
        // From Python app
        // the guidebook says it's supposed to be GraphVar, but it's actually Power
        KEY(kb_KeyPower, '\\'),
        KEY(kb_Key3, '#'),
        KEY(kb_KeyAdd, '\''),
        KEY(kb_KeyChs, '_'),
        KEY(kb_KeyEnter, '\t'),
        // Custom
        KEY(kb_Key2, '@'),
        KEY(kb_Key4, '$'),
        KEY(kb_Key5, '%'),
        KEY(kb_Key6, '|'),
        KEY(kb_Key7, '&'),
        KEY(kb_Key8, '~'),
        KEY(kb_Key9, '`'),
        KEY(kb_KeyMode, '\b'),
};

const char key_chars_upper[6][8] = {
        // As labeled
        KEY(kb_KeyMath, 'A'),
        KEY(kb_KeyApps, 'B'),
        KEY(kb_KeyPrgm, 'C'),
        KEY(kb_KeyRecip, 'D'),
        KEY(kb_KeySin, 'E'),
        KEY(kb_KeyCos, 'F'),
        KEY(kb_KeyTan, 'G'),
        KEY(kb_KeyPower, 'H'),
        KEY(kb_KeySquare, 'I'),
        KEY(kb_KeyComma, 'J'),
        KEY(kb_KeyLParen, 'K'),
        KEY(kb_KeyRParen, 'L'),
        KEY(kb_KeyDiv, 'M'),
        KEY(kb_KeyLog, 'N'),
        KEY(kb_Key7, 'O'),
        KEY(kb_Key8, 'P'),
        KEY(kb_Key9, 'Q'),
        KEY(kb_KeyMul, 'R'),
        KEY(kb_KeyLn, 'S'),
        KEY(kb_Key4, 'T'),
        KEY(kb_Key5, 'U'),
        KEY(kb_Key6, 'V'),
        KEY(kb_KeySub, 'W'),
        KEY(kb_KeySto, 'X'),
        KEY(kb_Key1, 'Y'),
        KEY(kb_Key2, 'Z'),
        KEY(kb_Key0, ' '),
        KEY(kb_KeyDecPnt, ':'),
        KEY(kb_KeyChs, '?'),
        KEY(kb_KeyAdd, '"'),
        // From Python app
        KEY(kb_Key3, '@'),
        // Custom
        KEY(kb_KeyMode, '\b'),
};

const char key_chars_lower[6][8] = {
        // As labeled
        KEY(kb_KeyMath, 'a'),
        KEY(kb_KeyApps, 'b'),
        KEY(kb_KeyPrgm, 'c'),
        KEY(kb_KeyRecip, 'd'),
        KEY(kb_KeySin, 'e'),
        KEY(kb_KeyCos, 'f'),
        KEY(kb_KeyTan, 'g'),
        KEY(kb_KeyPower, 'h'),
        KEY(kb_KeySquare, 'i'),
        KEY(kb_KeyComma, 'j'),
        KEY(kb_KeyLParen, 'k'),
        KEY(kb_KeyRParen, 'l'),
        KEY(kb_KeyDiv, 'm'),
        KEY(kb_KeyLog, 'n'),
        KEY(kb_Key7, 'o'),
        KEY(kb_Key8, 'p'),
        KEY(kb_Key9, 'q'),
        KEY(kb_KeyMul, 'r'),
        KEY(kb_KeyLn, 's'),
        KEY(kb_Key4, 't'),
        KEY(kb_Key5, 'u'),
        KEY(kb_Key6, 'v'),
        KEY(kb_KeySub, 'w'),
        KEY(kb_KeySto, 'x'),
        KEY(kb_Key1, 'y'),
        KEY(kb_Key2, 'z'),
        KEY(kb_Key0, ' '),
        KEY(kb_KeyChs, '?'),
        KEY(kb_KeyAdd, '"'),
        KEY(kb_KeyEnter, '\n'),
        // From Python app
        KEY(kb_Key3, '@'),
        // Custom
        KEY(kb_KeyDecPnt, ';'),
        KEY(kb_KeyMode, '\b'),
};

#define get_key(keys, lkey) ((keys)[((lkey) >> 8) - 1] & (lkey))

static const char (*get_key_chars(struct terminal_state *term))[8] {
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
    for(uint8_t i = 0; i < 6; i++) {
        /* Check each bit of the group */
        for(int j = 0; j < 8; j++) {
            /* Check if key is pressed */
            if(keys[i] & (1 << j)) {
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
