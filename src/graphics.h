#ifndef H_GRAPHICS
#define H_GRAPHICS

#include "terminal.h"

#define NUM_GREYSCALE 24
#define CUBE_SIZE 6

#define mpLcdBase (*(void**)0xE30010)
#define CurrentBuffer (*(void**)0xE30014)

enum colors {
	WHITE           = 15,
	BLACK           = 0,
	COL_2ND         = 81,
	COL_ALPHA       = 155,
	BRIGHT_START    = 8,
	CUBE_START      = 16,
	GREYSCALE_START = 232
};

enum base_colors {
    BASE_WHITE      = 0,
    BASE_BLACK      = 1
};

void init_graphics(void);
void cleanup_graphics(void);

void set_cursor_image(uint8_t width, uint8_t height);

/* Process a SGR sequence */
void sgr(struct terminal_state *term, const uint24_t *args);

/* Convert a 24-bit color to a color in the current palette */
uint8_t true_color_to_palette(uint8_t r, uint8_t g, uint8_t b);

/* Calculate the text foreground color based on graphics attributes */
void update_fg_color(struct graphics *graphics);

void set_char_at(struct terminal_state *term, char c, uint8_t x, uint8_t y);
void erase_chars(struct terminal_state *term, uint8_t start_x, uint8_t end_x, uint8_t y);
void delete_chars(struct terminal_state *term, uint8_t x, uint8_t y, uint8_t amount);

void update_view_pos(struct terminal_state *term);

#endif
