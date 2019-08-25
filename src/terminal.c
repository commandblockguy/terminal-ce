#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fontlibc.h>

#include "terminal.h"

void write_data(terminal_state_t *term, char *data, size_t size) {
	char *current = data;
	char *end = data + size;

	while(current < end) {
		fontlib_DrawStringL(current, end - current);
		current = fontlib_GetLastCharacterRead() + 1;

		/* TODO: process special characters */
	}
}