#ifndef H_ESCAPE
#define H_ESCAPE

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const char CSI_SEQ[2];

enum ControlChars {
	BEL = 0x07,
	BS  = 0x08,
	HT  = 0x09,
	LF  = 0x0A,
	VT  = 0x0B,
	FF  = 0x0C,
	CR  = 0x0D,
	SO  = 0x0E,
	SI  = 0x0F,
	CAN = 0x18,
	SUB = 0x1A,
	ESC = 0x1B,
	DEL = 0x7F,
	CSI = 0x9B,
};

#define MAX_CSI_ARGS 16

/* Call this to handle escape sequences */
void process_escape_sequence(terminal_state_t *term);

/* Returns true if there is an escape sequence in progress */
bool process_partial_sequence(terminal_state_t *term);

bool process_esc_sequence(terminal_state_t *term, char *seq, uint8_t len);

bool process_csi_sequence(terminal_state_t *term, char *seq, uint8_t len);

#endif