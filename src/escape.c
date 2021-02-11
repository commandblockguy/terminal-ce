#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <debug.h>

#include "terminal.h"
#include "escape.h"
#include "graphics.h"

void process_escape_sequence(terminal_state_t *term) {
	/* Purge the buffer if a sequence finishes */
	if(!process_partial_sequence(term)) {
		term->esc_buf_len = 0;
	}
}

/* Returns true if the sequence is incomplete */
bool process_partial_sequence(terminal_state_t *term) {
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
		case CSI:
			return process_csi_sequence(term, &term->esc_buf[1], term->esc_buf_len - 1);
		case ESC:
			return process_esc_sequence(term, &term->esc_buf[1], term->esc_buf_len - 1);

		default:
			return false;
	}
}

bool process_csi_sequence(terminal_state_t *term, char *seq, uint8_t len) {
	uint24_t args[MAX_CSI_ARGS] = {0};
	uint8_t arg_num = 0;
	bool private = false;

	uint8_t i, j;

	/* Read each character in the sequence */
	for(i = 0; i < len; i++) {
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

        if(private) {
            bool new_setting;

            dbg_sprintf(dbgout, "Private CSI sequence %c(", seq[i]);
            for(j = 0; j < 16; j++) {
                dbg_sprintf(dbgout, "%u;", args[j]);
                if(args[j + 1] == 0) break;
            }
            dbg_sprintf(dbgout, ")\n");

            switch(seq[i]) {
                case 'h':  /* SM */
                    /* Set mode */
                    new_setting = true;
                    goto priv_mode;
                case 'l':  /* RM */
                    /* Reset mode */
                    new_setting = false;
                priv_mode:
                    switch(args[0]) {
                        case 1:
                            term->mode.decckm = new_setting;
                            break;
                        case 5:
                            term->mode.decscnm = new_setting;
                            // todo: redraw screen
                            break;
                        case 6:
                            term->mode.decom = new_setting;
                            break;
                        case 7:
                            term->mode.decawm = new_setting;
                            break;
                        case 8:
                            term->mode.decarm = new_setting;
                            break;
                        case 25:
                            term->mode.dectecm = new_setting;
                            break;
                        default:
                            dbg_sprintf(dbgerr, "Unknown private mode %u\n", args[0]);
                    }
                    return false;
                default:
                    dbg_sprintf(dbgerr, "Unknown private CSI sequence %c (%x)\n", seq[i], seq[i]);
                    return false;
            }
        }

        dbg_sprintf(dbgout, "CSI sequence %c(", seq[i]);
        for(j = 0; j < 16; j++) {
            dbg_sprintf(dbgout, "%u;", args[j]);
            if(args[j + 1] == 0) break;
        }
        dbg_sprintf(dbgout, ")\n");

		switch(seq[i]) {

			case 'F': /* CPL */
			    /* Move cursor up the indicated # of rows, to column 1. */
                set_cursor_pos(term, 1, term->csr_y);
			case 'A': /* CUU */
			    /* Move cursor up the indicated # of rows. */
				if(args[0] == 0) args[0] = 1;
				if(term->csr_y >= args[0] + term->scroll_top) {
                    set_cursor_pos(term, term->csr_x, term->csr_y - args[0]);
				} else {
                    set_cursor_pos(term, term->csr_x, term->scroll_top);
				}
				return false;

			case 'E': /* CNL */
			    /* Move cursor down the indicated # of rows, to column 1. */
				set_cursor_pos(term, 1, term->csr_y);
			case 'B': /* CUD */
			    /* Move cursor down the indicated # of rows. */
				if(args[0] == 0) args[0] = 1;
				if(term->csr_y + args[0] <= term->scroll_bottom) {
                    set_cursor_pos(term, term->csr_x, term->csr_y + args[0]);
				} else {
                    set_cursor_pos(term, term->csr_x, term->scroll_bottom);
				}
				return false;

			case 'C': /* CUF */
			case 'a': /* HPR */
			    /* Move cursor right the indicated # of columns. */
				if(args[0] == 0) args[0] = 1;
				if(term->csr_x + args[0] < term->cols) {
                    set_cursor_pos(term, term->csr_x + args[0], term->csr_y);
				} else {
                    set_cursor_pos(term, term->cols, term->csr_y);
				}
				return false;

			case 'D': /* CUB */
                /* Move cursor left the indicated # of columns. */
				if(args[0] == 0) args[0] = 1;
				if(term->csr_x > args[0]) {
                    set_cursor_pos(term, term->csr_x - args[0], term->csr_y);
				} else {
                    set_cursor_pos(term, 1, term->csr_y);
				}
				return false;

			case 'G':  /* CHA */
			case '`':  /* HPA */
			    /* Move cursor to indicated column in current row. */
				if(args[0] == 0) args[0] = 1;
				if(args[0] < term->cols) {
                    set_cursor_pos(term, args[0], term->csr_y);
				} else {
                    set_cursor_pos(term, term->cols, term->csr_y);
				}
				return false;

			case 'H':  /* CUP */ 
			case 'f':  /* HVP */ {
			    /* Move cursor to the indicated row, column */
				uint8_t x = args[1], y = args[0];
				if(x == 0) x = 1;
				if(y == 0) y = 1;
				if(x > term->cols) x = term->cols;
				if(y > term->rows) y = term->rows;
                set_cursor_pos(term, x, y);
				return false;
			}

			case 'J':  /* ED */ {
			    /* Erase display */
				switch(args[0]) {
					default:
					    /* Erase from cursor to end of display */
						for(uint8_t y = term->csr_y + 1; y <= term->rows; y++) {
						    erase_chars(term, 1, term->cols, y);
						}
						break; // continue to EL

					case 1:
					    /* Erase from start to cursor */
                        for(uint8_t y = 1; y < term->csr_y; y++) {
                            erase_chars(term, 1, term->cols, y);
                        }
						break; // continue to EL

					case 2:
					    /* Erase entire screen */
					case 3:
					    /* Erase entire screen and scrollback buffer */
                        for(uint8_t y = 1; y <= term->rows; y++) {
                            erase_chars(term, 1, term->cols, y);
                        }
						return false;
				}
			}

			case 'K':  /* EL */ {
			    /* Erase line */
				uint8_t x;
				switch(args[0]) {
					default:
						/* Erase from cursor to end of line */
                        erase_chars(term, term->csr_x, term->cols, term->csr_y);
						return false;

					case 1:
						/* Erase from start of line to cursor */
						erase_chars(term, 1, term->csr_x - 1, term->csr_y);
						return false;

					case 2:
						/* Erase entire line */
                        erase_chars(term, 1, term->cols, term->csr_y);
						return false;
				}
			}

			case 'P':  /* DCH */ {
			    /* Delete the indicated # of characters on current line. */
			    uint8_t actual_width = args[0];

				if(actual_width == 0) actual_width = 1;
				if(term->csr_x + actual_width > term->cols) {
				    actual_width = term->cols - term->csr_x;
				}

				delete_chars(term, term->csr_x, term->csr_y, actual_width);

				return false;
			}


			case 'c':  /* DA */ {
			    /* Answer ESC [ ? 6 c: "I am a VT102". */
				const char *str = CSI_SEQ "?6c";
				term->input_callback(str, strlen(str), term->callback_data);
				return false;
			}

			case 'm':  /* SGR */ {
			    /* Set graphics rendition */
				sgr(term, args);
				return false;
			}

			case 'r':  /* DECSTBM */ {
			    /* Set scrolling region */
				term->scroll_top = args[0];
				term->scroll_bottom = args[1];
				return false;
			}

            case 'h':  /* SM */ {
                /* Set mode */
                bool new_setting;
                new_setting = true;
                goto mode;

            case 'l':  /* RM */
                /* Reset mode */
                new_setting = false;
                mode:
                switch(args[0]) {
                    case 3:
                        term->mode.deccrm = new_setting;
                        break;
                    case 4:
                        term->mode.decim = new_setting;
                        break;
                    case 20:
                        term->mode.lf_nl = new_setting;
                        break;
                    default:
                        dbg_sprintf(dbgerr, "Unknown mode %u\n", args[0]);
                }
                return false;
            }

            case 'd': /* VPA */
                /* Move cursor to the indicated row, current column. */
                if(args[0] == 0) args[0] = 1;
                if(args[0] < term->rows) {
                    set_cursor_pos(term, term->csr_x, args[0]);
                } else {
                    set_cursor_pos(term, term->csr_x, term->rows);
                }
                return false;

			default:
				dbg_sprintf(dbgerr, "Unknown CSI sequence %c (%x)\n", seq[i], seq[i]);
				return false;
		}
	}


	return true;
}

bool process_esc_sequence(terminal_state_t *term, char *seq, uint8_t len) {
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
	                dbg_sprintf(dbgout, "Unimplemented char set: ESC ( %c\n", seq[1]);
	        }
	        return false;

        case ')':
            if(len == 1) return true;
            switch(seq[1]) {
                default:
                    dbg_sprintf(dbgout, "Unimplemented char set: ESC ) %c\n", seq[1]);
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
