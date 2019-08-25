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

const char CSI_SEQ[2] = "\x1B\x5B";

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
		case CR:
			set_cursor_pos(term, 1, term->csr_y, true);
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

	uint8_t i;

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

		switch(seq[i]) {
			case 'A': /* Up */
				if(args[0] == 0) args[0] = 1;
				if(term->csr_y > args[0]) {
					set_cursor_pos(term, term->csr_x, term->csr_y - args[0], true);
				} else {
					set_cursor_pos(term, term->csr_x, 1, true);
				}
				return false;

			case 'B': /* Down */
				if(args[0] == 0) args[0] = 1;
				if(term->csr_y + args[0] < term->rows) {
					set_cursor_pos(term, term->csr_x, term->csr_y + args[0], true);
				} else {
					set_cursor_pos(term, term->csr_x, term->rows, true);
				}
				return false;

			case 'C': /* Right */
				if(args[0] == 0) args[0] = 1;
				if(term->csr_x + args[0] < term->cols) {
					set_cursor_pos(term, term->csr_x + args[0], term->csr_y, true);
				} else {
					set_cursor_pos(term, term->cols, term->csr_y, true);
				}
				return false;

			case 'D': /* Left */
				if(args[0] == 0) args[0] = 1;
				if(term->csr_x > args[0]) {
					set_cursor_pos(term, term->csr_x - args[0], term->csr_y, true);
				} else {
					set_cursor_pos(term, 1, term->csr_y, true);
				}
				return false;

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

		default:
			dbg_sprintf(dbgout, "Unknown ESC sequence %c (%x)\n", seq[0], seq[0]);
			return false;
	}
}
