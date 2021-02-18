#include "escape.h"

#include <stdio.h>

#include <debug.h>

#include "graphics.h"

void process_escape_sequence(struct terminal_state *term) {
	/* Purge the buffer if a sequence finishes */
	if(!process_partial_sequence(term)) {
		term->esc_buf_len = 0;
	}
}

/* Returns true if the sequence is incomplete */
bool process_partial_sequence(struct terminal_state *term) {
	/* Do nothing if there is not anything in the escape sequence buffer */
	if(term->esc_buf_len == 0) return false;

	switch(term->esc_buf[0]) {
		case BEL:
			dbg_sprintf(dbgerr, "DING\n");
			return false;
		case BS:
			if(term->csr_x > 1) {
                set_cursor_pos(term, term->csr_x - 1, term->csr_y);
			}
			return false;
		case LF:
		case VT:
		case FF:
			scroll_down(term);
			return false;
		case CR:
            set_cursor_pos(term, 1, term->csr_y);
			return false;
		case (char)CSI:
			return process_csi_sequence(term, &term->esc_buf[1], term->esc_buf_len - 1);
		case ESC:
			return process_esc_sequence(term, &term->esc_buf[1], term->esc_buf_len - 1);

		default:
			return false;
	}
}

bool process_csi_sequence(struct terminal_state *term, const char *seq, uint8_t len) {
    if(!len) return true;

	char final_char = seq[len - 1];

	if(final_char < 0x40 || final_char == 0x7F) {
	    /* Sequence is incomplete */
	    return true;
	}

    uint24_t args[MAX_CSI_ARGS] = {0};
    uint8_t arg_num = 0;
    bool private = false;

	/* Read arguments in the sequence */
	for(uint8_t i = 0; i < len - 1; i++) {
		if(arg_num < MAX_CSI_ARGS) {
			/* If ';', start the next argument */
			if(seq[i] == ';') {
				arg_num++;
				continue;
			}

			/* If a digit, update the value of the argument */
			if(seq[i] <= '9' && seq[i] >= '0') {
				args[arg_num] = args[arg_num] * 10 + (seq[i] - '0');
				continue;
			}
		}

		if(seq[i] == '?') {
            private = true;
		    continue;
		}
	}

    if(private) {
        process_private_csi_sequence(term, final_char, args);
    } else {
        process_complete_csi_sequence(term, final_char, args);
    }

	return false;
}

/* Move cursor up the indicated # of rows. */
static void csi_cuu(struct terminal_state *term, uint8_t rows) {
    if(rows == 0) rows = 1;
    if(term->csr_y >= rows + term->scroll_top) {
        set_cursor_pos(term, term->csr_x, term->csr_y - rows);
    } else {
        set_cursor_pos(term, term->csr_x, term->scroll_top);
    }
}

/* Move cursor down the indicated # of rows. */
static void csi_cud(struct terminal_state *term, uint8_t rows) {
    if(rows == 0) rows = 1;
    if(term->csr_y + rows <= term->scroll_bottom) {
        set_cursor_pos(term, term->csr_x, term->csr_y + rows);
    } else {
        set_cursor_pos(term, term->csr_x, term->scroll_bottom);
    }
}

/* Move cursor right the indicated # of columns. */
static void csi_cuf(struct terminal_state *term, uint8_t cols) {
    if(cols == 0) cols = 1;
    if(term->csr_x + cols < term->cols) {
        set_cursor_pos(term, term->csr_x + cols, term->csr_y);
    } else {
        set_cursor_pos(term, term->cols, term->csr_y);
    }
}

/* Move cursor left the indicated # of columns. */
static void csi_cub(struct terminal_state *term, uint8_t cols) {
    if(cols == 0) cols = 1;
    if(term->csr_x > cols) {
        set_cursor_pos(term, term->csr_x - cols, term->csr_y);
    } else {
        set_cursor_pos(term, 1, term->csr_y);
    }
}

/* Move cursor to indicated column in current row. */
static void csi_cha(struct terminal_state *term, uint8_t col) {
    if(col == 0) col = 1;
    if(col < term->cols) {
        set_cursor_pos(term, col, term->csr_y);
    } else {
        set_cursor_pos(term, term->cols, term->csr_y);
    }
}

/* Move cursor to the indicated row, column */
static void csi_cup(struct terminal_state *term, uint8_t x, uint8_t y) {
    if(x == 0) x = 1;
    if(y == 0) y = 1;
    if(x > term->cols) x = term->cols;
    if(y > term->rows) y = term->rows;
    set_cursor_pos(term, x, y);
}

/* Erase line */
void csi_el(struct terminal_state *term, uint8_t mode) {
    switch(mode) {
        default:
            /* Erase from cursor to end of line */
            erase_chars(term, term->csr_x, term->cols, term->csr_y);
            break;

        case 1:
            /* Erase from start of line to cursor */
            erase_chars(term, 1, term->csr_x - 1, term->csr_y);
            break;

        case 2:
            /* Erase entire line */
            erase_chars(term, 1, term->cols, term->csr_y);
            break;
    }
}

/* Erase display */
static void csi_ed(struct terminal_state *term, uint8_t mode) {
    switch(mode) {
        default:
            /* Erase from cursor to end of display */
            for(uint8_t y = term->csr_y + 1; y <= term->rows; y++) {
                erase_chars(term, 1, term->cols, y);
            }
            csi_el(term, 0);
            break;

        case 1:
            /* Erase from start to cursor */
            for(uint8_t y = 1; y < term->csr_y; y++) {
                erase_chars(term, 1, term->cols, y);
            }
            csi_el(term, 1);
            break;

        case 2:
            /* Erase entire screen */
        case 3:
            /* Erase entire screen and scrollback buffer */
            for(uint8_t y = 1; y <= term->rows; y++) {
                erase_chars(term, 1, term->cols, y);
            }
    }
}

/* Delete the indicated # of characters on current line. */
void csi_dch(struct terminal_state *term, uint8_t chars) {
    if(chars == 0) chars = 1;
    if(term->csr_x + chars > term->cols) {
        chars = term->cols - term->csr_x;
    }

    delete_chars(term, term->csr_x, term->csr_y, chars);
}

/* Answer ESC [ ? 6 c: "I am a VT102". */
void csi_da(struct terminal_state *term) {
    const char *str = CSI_SEQ "?6c";
    term->input_callback(str, strlen(str), term->callback_data);
}

void csi_set_mode(struct terminal_state *term, uint24_t option, bool value) {
    switch(option) {
        case 3:
            term->mode.deccrm = value;
            break;
        case 4:
            term->mode.decim = value;
            break;
        case 20:
            term->mode.lf_nl = value;
            break;
        default:
            dbg_sprintf(dbgerr, "Unknown mode %u\n", option);
    }
}

/* Move cursor to the indicated row, current column. */
void csi_vpa(struct terminal_state *term, uint8_t row) {
    if(row == 0) row = 1;
    if(row < term->rows) {
        set_cursor_pos(term, term->csr_x, row);
    } else {
        set_cursor_pos(term, term->csr_x, term->rows);
    }
}

void process_complete_csi_sequence(struct terminal_state *term, char c, const uint24_t *args) {
    dbg_printf("CSI sequence %c(", c);
    for(uint8_t j = 0; j < 16; j++) {
        dbg_printf("%u;", args[j]);
        if(args[j + 1] == 0) break;
    }
    dbg_printf(")\n");

    switch(c) {

        case 'A': /* CUU */
            csi_cuu(term, args[0]);
            break;
        case 'B': /* CUD */
            csi_cud(term, args[0]);
            break;
        case 'C': /* CUF */
        case 'a': /* HPR */
            csi_cuf(term, args[0]);
            break;
        case 'D': /* CUB */
            csi_cub(term, args[0]);
            break;
        case 'E': /* CNL */
            /* Move cursor down the indicated # of rows, to column 1. */
            csi_cud(term, args[0]);
            set_cursor_pos(term, 1, term->csr_y);
            break;
        case 'F': /* CPL */
            /* Move cursor up the indicated # of rows, to column 1. */
            csi_cuu(term, args[0]);
            set_cursor_pos(term, 1, term->csr_y);
            break;
        case 'G':  /* CHA */
        case '`':  /* HPA */
            csi_cha(term, args[0]);
            break;
        case 'H':  /* CUP */
        case 'f':  /* HVP */
            csi_cup(term, args[1], args[0]);
            break;
        case 'J':  /* ED */
            csi_ed(term, args[0]);
            break;
        case 'K':  /* EL */
            csi_el(term, args[0]);
            break;
        case 'P':  /* DCH */
            csi_dch(term, args[0]);
            break;
        case 'c':  /* DA */
            csi_da(term);
            break;
        case 'd': /* VPA */ {
            csi_vpa(term, args[0]);
            break;
        }
        case 'h':  /* SM */
            /* Set mode */
            csi_set_mode(term, args[0], true);
            break;
        case 'l':  /* RM */
            /* Reset mode */
            csi_set_mode(term, args[0], false);
            break;
        case 'm':  /* SGR */
            /* Set graphics rendition */
            sgr(term, args);
            break;
        case 'r':  /* DECSTBM */
            /* Set scrolling region */
            term->scroll_top = args[0];
            term->scroll_bottom = args[1];
            break;

        default:
            dbg_sprintf(dbgerr, "Unknown CSI sequence %c (%x)\n", c, c);
            break;
    }
}

void csi_private_set_mode(struct terminal_state *term, uint24_t option, bool value) {
    switch(option) {
        case 1:
            term->mode.decckm = value;
            break;
        case 5:
            term->mode.decscnm = value;
            // todo: invert screen
            break;
        case 6:
            term->mode.decom = value;
            break;
        case 7:
            term->mode.decawm = value;
            break;
        case 8:
            term->mode.decarm = value;
            break;
        case 25:
            term->mode.dectecm = value;
            break;
        default:
            dbg_sprintf(dbgerr, "Unknown private mode %u\n", option);
    }
}

void process_private_csi_sequence(struct terminal_state *term, char c, const uint24_t *args) {
    dbg_printf("Private CSI sequence %c(", c);
    for(uint8_t j = 0; j < 16; j++) {
        dbg_printf("%u;", args[j]);
        if(args[j + 1] == 0) break;
    }
    dbg_printf(")\n");

    switch(c) {
        case 'h':  /* SM */
            /* Set mode */
            csi_private_set_mode(term, args[0], true);
            break;
        case 'l':  /* RM */
            /* Reset mode */
            csi_private_set_mode(term, args[0], false);
            break;

        default:
            dbg_sprintf(dbgerr, "Unknown private CSI sequence %c (%x)\n", c, c);
    }
}

bool process_esc_sequence(struct terminal_state *term, char *seq, uint8_t len) {
	if(!len) return true;

	switch(seq[0]) {
		case '[':
            /* Control sequence introducer */
            return process_csi_sequence(term, &seq[1], len - 1);

		case 'M':  /* RI */
		    /* Reverse linefeed. */
		    //todo: backwards scrolling
			if(term->csr_y > 1) {
                set_cursor_pos(term, term->csr_x, term->csr_y - 1);
			}
			return false;

		case '7':  /* DECSC */
		    /* Save current state */
			term->backup.csr_x = term->csr_x;
			term->backup.csr_y = term->csr_y;
			return false;

		case '8':  /* DECRC */
		    /* Restore state most recently saved by ESC 7. */
            set_cursor_pos(term, term->backup.csr_x, term->backup.csr_y);
			return false;

	    case '(':
	        if(len == 1) return true;
	        switch(seq[1]) {
	            default:
	                dbg_printf("Unimplemented char set: ESC ( %c\n", seq[1]);
	        }
	        return false;

        case ')':
            if(len == 1) return true;
            switch(seq[1]) {
                default:
                    dbg_printf("Unimplemented char set: ESC ) %c\n", seq[1]);
            }
            return false;

	    case 'Z': {
            /* Answer ESC [ ? 1 ; 2 c */
            const char *str = CSI_SEQ "?6c";
            term->input_callback(str, strlen(str), term->callback_data);
            return false;
	    }

		default:
			dbg_sprintf(dbgerr, "Unknown ESC sequence %c (%x)\n", seq[0], seq[0]);
			return false;
	}
}
