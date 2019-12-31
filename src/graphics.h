#ifndef H_GRAPHICS
#define H_GRAPHICS

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "terminal.h"

#define NUM_GREYSCALE 24
#define CUBE_SIZE 6

enum Colors {
	WHITE           = 15,
	BLACK           = 0,
	COL_2ND         = 81,
	COL_ALPHA       = 155,
	BRIGHT_START    = 8,
	CUBE_START      = 16,
	GREYSCALE_START = 232
};

/* Process a SGR sequence */
void sgr(terminal_state_t *term, uint24_t *args);

/* Convert a 24-bit color to a color in the current palette */
uint8_t true_color_to_palette(uint8_t r, uint8_t g, uint8_t b);

/* Calculate the text forgeground color based on graphics attributes */
/* You probably want graphics->fg_color instead */
uint8_t get_fg_color(graphics_t *graphics);

/* Sets the fontlib fg and bg color, respecting inverted graphics */
void set_colors(graphics_t *graphics);

/* Get the background color, respecting inverted graphics */
uint8_t bg_color(graphics_t *graphics);

#endif