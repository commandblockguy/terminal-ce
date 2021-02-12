#ifndef H_ESCAPE
#define H_ESCAPE

#include "terminal.h"

#define CSI_SEQ "\x1B\x5B"

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
void process_escape_sequence(struct terminal_state *term);

/* Returns true if there is an escape sequence in progress */
bool process_partial_sequence(struct terminal_state *term);

bool process_esc_sequence(struct terminal_state *term, char *seq, uint8_t len);

bool process_csi_sequence(struct terminal_state *term, const char *seq, uint8_t len);

void process_complete_csi_sequence(struct terminal_state *term, char c, const uint24_t *args);

void process_private_csi_sequence(struct terminal_state *term, char c, const uint24_t *args);

#endif