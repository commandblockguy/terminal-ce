#ifndef H_SETTINGS
#define H_SETTINGS

#include <stdbool.h>
#include <stdint.h>

#define SETTINGS_FILE "TERMCONF"

struct settings {
	char font_pack_name[8];
	uint8_t reg_font;
	uint8_t bold_font;
	uint8_t color_mode;
	uint16_t colors[18];
    uint32_t repeat_delay;
    uint32_t repeat_rate;
};

bool write_default_settings(void);

bool read_settings(struct settings *settings);

bool write_settings(const struct settings *settings);

#endif
