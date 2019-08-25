#ifndef H_SETTINGS
#define H_SETTINGS

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SETTINGS_FILE "TERMCONF"

typedef struct Settings {
	char font_pack_name[8];
	uint8_t reg_font;
	uint8_t bold_font;
	uint8_t italic_font;
	uint8_t color_mode;
	uint16_t colors[18];
} settings_t;

bool write_default_settings(void);

bool read_settings(settings_t *settings);

bool write_settings(settings_t *settings);

#endif
