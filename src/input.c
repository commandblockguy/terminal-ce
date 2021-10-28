#include "input.h"

#include <keypadc.h>
#include <debug.h>

#include "escape.h"
#include "menu.h"

#define GROUP(lkey) ((lkey) >> 8)
#define KEY(lkey) (__builtin_ctz((lkey) & 0xFF))
#define MAP_KEY(lkey, val) [GROUP(lkey)][KEY(lkey)] = (val)

const char key_chars_std[7][8] = {
        // As labeled
        MAP_KEY(kb_Key0, '0'),
        MAP_KEY(kb_Key1, '1'),
        MAP_KEY(kb_Key2, '2'),
        MAP_KEY(kb_Key3, '3'),
        MAP_KEY(kb_Key4, '4'),
        MAP_KEY(kb_Key5, '5'),
        MAP_KEY(kb_Key6, '6'),
        MAP_KEY(kb_Key7, '7'),
        MAP_KEY(kb_Key8, '8'),
        MAP_KEY(kb_Key9, '9'),
        MAP_KEY(kb_KeyDecPnt, '.'),
        MAP_KEY(kb_KeyChs, '-'),
        MAP_KEY(kb_KeyAdd, '+'),
        MAP_KEY(kb_KeySub, '-'),
        MAP_KEY(kb_KeyMul, '*'),
        MAP_KEY(kb_KeyDiv, '/'),
        MAP_KEY(kb_KeyPower, '^'),
        MAP_KEY(kb_KeyComma, ','),
        MAP_KEY(kb_KeyLParen, '('),
        MAP_KEY(kb_KeyRParen, ')'),
        MAP_KEY(kb_KeyEnter, '\n'),
        // From Python app
        MAP_KEY(kb_KeySto, '='),
        // Custom
        MAP_KEY(kb_KeySin, '@'),
        MAP_KEY(kb_KeyCos, '<'),
        MAP_KEY(kb_KeyTan, '>'),
        MAP_KEY(kb_KeyMode, '\b'),
};

const char key_chars_2nd[7][8] = {
        // As labeled
        MAP_KEY(kb_KeyLParen, '{'),
        MAP_KEY(kb_KeyRParen, '}'),
        MAP_KEY(kb_KeyMul, '['),
        MAP_KEY(kb_KeySub, ']'),
        // From Python app
        // the guidebook says it's supposed to be GraphVar, but it's actually Power
        MAP_KEY(kb_KeyPower, '\\'),
        MAP_KEY(kb_Key3, '#'),
        MAP_KEY(kb_KeyAdd, '\''),
        MAP_KEY(kb_KeyChs, '_'),
        MAP_KEY(kb_KeyEnter, '\t'),
        // Custom
        MAP_KEY(kb_Key2, '@'),
        MAP_KEY(kb_Key4, '$'),
        MAP_KEY(kb_Key5, '%'),
        MAP_KEY(kb_Key6, '|'),
        MAP_KEY(kb_Key7, '&'),
        MAP_KEY(kb_Key8, '~'),
        MAP_KEY(kb_Key9, '`'),
        MAP_KEY(kb_KeyMode, '\b'),
};

const char key_chars_upper[7][8] = {
        // As labeled
        MAP_KEY(kb_KeyMath, 'A'),
        MAP_KEY(kb_KeyApps, 'B'),
        MAP_KEY(kb_KeyPrgm, 'C'),
        MAP_KEY(kb_KeyRecip, 'D'),
        MAP_KEY(kb_KeySin, 'E'),
        MAP_KEY(kb_KeyCos, 'F'),
        MAP_KEY(kb_KeyTan, 'G'),
        MAP_KEY(kb_KeyPower, 'H'),
        MAP_KEY(kb_KeySquare, 'I'),
        MAP_KEY(kb_KeyComma, 'J'),
        MAP_KEY(kb_KeyLParen, 'K'),
        MAP_KEY(kb_KeyRParen, 'L'),
        MAP_KEY(kb_KeyDiv, 'M'),
        MAP_KEY(kb_KeyLog, 'N'),
        MAP_KEY(kb_Key7, 'O'),
        MAP_KEY(kb_Key8, 'P'),
        MAP_KEY(kb_Key9, 'Q'),
        MAP_KEY(kb_KeyMul, 'R'),
        MAP_KEY(kb_KeyLn, 'S'),
        MAP_KEY(kb_Key4, 'T'),
        MAP_KEY(kb_Key5, 'U'),
        MAP_KEY(kb_Key6, 'V'),
        MAP_KEY(kb_KeySub, 'W'),
        MAP_KEY(kb_KeySto, 'X'),
        MAP_KEY(kb_Key1, 'Y'),
        MAP_KEY(kb_Key2, 'Z'),
        MAP_KEY(kb_Key0, ' '),
        MAP_KEY(kb_KeyDecPnt, ':'),
        MAP_KEY(kb_KeyChs, '?'),
        MAP_KEY(kb_KeyAdd, '"'),
        // From Python app
        MAP_KEY(kb_Key3, '@'),
        // Custom
        MAP_KEY(kb_KeyMode, '\b'),
};

const char key_chars_lower[7][8] = {
        // As labeled
        MAP_KEY(kb_KeyMath, 'a'),
        MAP_KEY(kb_KeyApps, 'b'),
        MAP_KEY(kb_KeyPrgm, 'c'),
        MAP_KEY(kb_KeyRecip, 'd'),
        MAP_KEY(kb_KeySin, 'e'),
        MAP_KEY(kb_KeyCos, 'f'),
        MAP_KEY(kb_KeyTan, 'g'),
        MAP_KEY(kb_KeyPower, 'h'),
        MAP_KEY(kb_KeySquare, 'i'),
        MAP_KEY(kb_KeyComma, 'j'),
        MAP_KEY(kb_KeyLParen, 'k'),
        MAP_KEY(kb_KeyRParen, 'l'),
        MAP_KEY(kb_KeyDiv, 'm'),
        MAP_KEY(kb_KeyLog, 'n'),
        MAP_KEY(kb_Key7, 'o'),
        MAP_KEY(kb_Key8, 'p'),
        MAP_KEY(kb_Key9, 'q'),
        MAP_KEY(kb_KeyMul, 'r'),
        MAP_KEY(kb_KeyLn, 's'),
        MAP_KEY(kb_Key4, 't'),
        MAP_KEY(kb_Key5, 'u'),
        MAP_KEY(kb_Key6, 'v'),
        MAP_KEY(kb_KeySub, 'w'),
        MAP_KEY(kb_KeySto, 'x'),
        MAP_KEY(kb_Key1, 'y'),
        MAP_KEY(kb_Key2, 'z'),
        MAP_KEY(kb_Key0, ' '),
        MAP_KEY(kb_KeyChs, '?'),
        MAP_KEY(kb_KeyAdd, '"'),
        MAP_KEY(kb_KeyEnter, '\n'),
        // From Python app
        MAP_KEY(kb_Key3, '@'),
        // Custom
        MAP_KEY(kb_KeyDecPnt, ';'),
        MAP_KEY(kb_KeyMode, '\b'),
};

static const char (*get_key_chars(struct terminal_state *term))[8] {
    if(term->mode_2nd) {
        if(term->mode_alpha) return key_chars_lower;
        else return key_chars_2nd;
    } else {
        if(term->mode_alpha) return key_chars_upper;
        else return key_chars_std;
    }
}

static void handle_arrow_key(struct terminal_state *term, uint8_t key) {
    const char codes[4] = "BDCA";
    char seq[] = CSI_SEQ;

    /* Output the CSI escape sequence */
    seq[2] = codes[key];

    send_input(term, seq, sizeof seq);
}

static void handle_control_keypress(struct terminal_state *term, char val) {
    if(val >= '@' && val <= '_') {
        val -= '@';
    } else if(val >= 'a' && val <= 'z') {
        val -= 'a' - 1;
    }
    send_input(term, &val, 1);
}

#define LKEY(group, key) (((group) << 8) | (1 << (key)))

static void handle_keypress(struct terminal_state *term, uint8_t group, uint8_t key) {
    const char (*key_chars)[8] = get_key_chars(term);
    char val = key_chars[group][key];

    if(kb_IsDown(kb_KeyYequ)) {
        if(LKEY(group, key) == kb_KeyWindow) {
            send_stty(term);
        }
        handle_control_keypress(term, val);
        return;
    }

    if(val) {
        send_input(term, &val, 1);
        return;
    }

    if(group == GROUP(kb_KeyDown)) {
        handle_arrow_key(term, key);
        return;
    }

    switch(LKEY(group, key)) {
        // todo: visual indicators
        case kb_Key2nd:
            term->mode_2nd = !term->mode_2nd;
            break;
        case kb_KeyAlpha:
            term->mode_alpha = !term->mode_alpha;
            break;
        case kb_KeyDel: {
            const char seq_del[] = CSI_SEQ "3~";
            send_input(term, seq_del, strlen(seq_del));
            break;
        }
        case kb_KeyGraph:
            menu(term);
            break;
        default:
            break;
    }
}

void key_repeat(struct terminal_state *term, kb_lkey_t last_key) {
    static kb_lkey_t held_key = 0;

    if(last_key) {
        held_key = last_key;
        timer_Disable(1);
        timer_Set(1, term->settings->repeat_delay);
        timer_SetReload(1, term->settings->repeat_rate);
        timer_AckInterrupt(1, TIMER_RELOADED);
        timer_Enable(1, TIMER_32K, TIMER_0INT, TIMER_DOWN);
    }

    if(!held_key) return;

    if(!kb_IsDown(held_key)) {
        held_key = 0;
        return;
    }

    if(timer_ChkInterrupt(1, TIMER_RELOADED)) {
        timer_AckInterrupt(1, TIMER_RELOADED);
        handle_keypress(term, GROUP(held_key), KEY(held_key));
    }
}

void process_input(struct terminal_state *term) {
    kb_Scan();

    kb_key_t keys[7];
    for(uint8_t group = 1; group < 8; group++) {
        uint8_t data = kb_Data[group];
        keys[group - 1] = data & ~term->held_keys[group - 1];
        term->held_keys[group - 1] = data;
    }

    /* Handle regular keypresses */
    /* Check each keypad group */
    kb_lkey_t last_key = 0;
    for(uint8_t group = 1; group < 8; group++) {
        uint8_t group_data = keys[group - 1];
        if(!group_data) continue;
        /* Check each bit of the group */
        for(uint8_t key = 0; key < 8; key++) {
            /* Check if key is pressed */
            if(group_data & (1 << key)) {
                handle_keypress(term, group, key);
                last_key = LKEY(group, key);
            }
        }
    }
    key_repeat(term, last_key);
}
