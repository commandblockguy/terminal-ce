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

		dbg_sprintf(dbgout, "CSI sequence %c(", seq[i]);
		for(j = 0; j < 16; j++) {
			dbg_sprintf(dbgout, "%u;", args[j]);
			if(args[j + 1] == 0) break;
		}
		dbg_sprintf(dbgout, ")\n");

		switch(seq[i]) {

			case '?':  /* todo: set flag for this */
				continue;

			case 'F': /* CPL */
                set_cursor_pos(term, 1, term->csr_y);
			case 'A': /* CUU */
				if(args[0] == 0) args[0] = 1;
				if(term->csr_y > args[0]) {
                    set_cursor_pos(term, term->csr_x, term->csr_y - args[0]);
				} else {
                    set_cursor_pos(term, term->csr_x, 1);
				}
				return false;

			case 'E': /* CNL */
				set_cursor_pos(term, 1, term->csr_y);
			case 'B': /* CUD */
				if(args[0] == 0) args[0] = 1;
				if(term->csr_y + args[0] < term->rows) {
                    set_cursor_pos(term, term->csr_x, term->csr_y + args[0]);
				} else {
                    set_cursor_pos(term, term->csr_x, term->rows);
				}
				return false;

			case 'C': /* CUF */
			case 'a': /* HPR */
				if(args[0] == 0) args[0] = 1;
				if(term->csr_x + args[0] < term->cols) {
                    set_cursor_pos(term, term->csr_x + args[0], term->csr_y);
				} else {
                    set_cursor_pos(term, term->cols, term->csr_y);
				}
				return false;

			case 'D': /* CUB */
				if(args[0] == 0) args[0] = 1;
				if(term->csr_x > args[0]) {
                    set_cursor_pos(term, term->csr_x - args[0], term->csr_y);
				} else {
                    set_cursor_pos(term, 1, term->csr_y);
				}
				return false;

			case 'G':  /* CHA */
			case '`':  /* HPA */
				if(args[0] == 0) args[0] = 1;
				if(args[0] < term->cols) {
                    set_cursor_pos(term, args[0], term->csr_y);
				} else {
                    set_cursor_pos(term, term->cols, term->csr_y);
				}
				return false;

			case 'H':  /* CUP */ 
			case 'f':  /* HVP */ {
				uint8_t x = args[1], y = args[0];
				if(x == 0) x = 1;
				if(y == 0) y = 1;
				if(x > term->cols) x = term->cols;
				if(y > term->rows) y = term->rows;
                set_cursor_pos(term, x, y);
				return false;
			}

			case 'J':  /* ED */ {
			    uint8_t x, y;
				switch(args[0]) {
					default:
					    /* Erase from cursor to end of display */
						for(y = term->csr_y + 1; y <= term->rows; y++) {
                            for(x = 1; x <= term->cols; x++) {
                                set_char(term, ' ', x, y);
                            }
						}
						break; // continue to EL

					case 1:
					    /* Erase from start to cursor */
                        for(y = 1; y < term->csr_y; y++) {
                            for(x = 1; x <= term->cols; x++) {
                                set_char(term, ' ', x, y);
                            }
                        }
						break; // continue to EL

					case 2:
					    /* Erase entire screen */
					case 3:
					    /* Erase entire screen and scrollback buffer */
                        for(y = 1; y <= term->rows; y++) {
                            for(x = 1; x <= term->cols; x++) {
                                set_char(term, ' ', x, y);
                            }
                        }
						return false;
				}
			}

			case 'K':  /* EL */ {
				uint8_t x;
				switch(args[0]) {
					default:
						/* Erase from cursor to end of line */
						for(x = term->csr_x; x <= term->cols; x++) {
						    set_char(term, ' ', x, term->csr_y);
						}
						return false;

					case 1:
						/* Erase from start of line to cursor */
                        for(x = 1; x < term->csr_x; x++) {
                            set_char(term, ' ', x, term->csr_y);
                        }
						return false;

					case 2:
						/* Erase entire line */
                        for(x = 1; x <= term->cols; x++) {
                            set_char(term, ' ', x, term->csr_y);
                        }
						return false;
				}
			}

			case 'P':  /* DCH */ {
			    uint8_t actual_width = args[0];
			    uint8_t x;

				if(actual_width == 0) actual_width = 1;
				if(term->csr_x + actual_width > term->cols) {
				    actual_width = term->cols - term->csr_x;
				}

                for(x = term->csr_x; x <= term->cols - actual_width; x++) {
                    char next = term->text_buf[term->csr_y - 1][x + actual_width - 1].ch;
                    set_char(term, next, x, term->csr_y);
                }
                for(x = term->cols - actual_width + 1; x <= term->cols; x++) {
                    set_char(term, ' ', x, term->csr_y);
                }

				return false;
			}


			case 'c':  /* DA */ {
				const char *str = CSI_SEQ "?6c";
				term->input_callback(str, strlen(str), term->callback_data);
				return false;
			}

			case 'm':  /* SGR */ {
				sgr(term, args);
				return false;
			}

			case 'r':  /* DECSTBM */ {
				term->scroll_top = args[0];
				term->scroll_bottom = args[1];
				return false;
			}

			default:
				dbg_sprintf(dbgout, "Unknown CSI sequence %c (%x)\n", seq[i], seq[i]);
				return false;
		}
	}


	return true;
}

bool process_esc_sequence(terminal_state_t *term, char *seq, uint8_t len) {
	if(!len) return true;

	switch(seq[0]) {
		/* Handle CSI sequence */
		case '[':
			return process_csi_sequence(term, &seq[1], len - 1);

		case 'M':  /* RI */
			if(term->csr_y > 1) {
                set_cursor_pos(term, term->csr_x, term->csr_y - 1);
			}
			return false;

		case '7':  /* DECSC */
			term->backup.csr_x = term->csr_x;
			term->backup.csr_y = term->csr_y;
			return false;

		case '8':  /* DECRC */
            set_cursor_pos(term, term->backup.csr_x, term->backup.csr_y);
			return false;

		default:
			dbg_sprintf(dbgout, "Unknown ESC sequence %c (%x)\n", seq[0], seq[0]);
			return false;
	}
}
