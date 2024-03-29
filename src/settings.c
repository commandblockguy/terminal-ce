#include "settings.h"
#include <stddef.h>

#include <fileioc.h>

#define SECONDS(x) ((uint32_t)32768.0 * (x))

const struct settings default_settings = {
	"GOHUFONT",
	1,
	0,
	0, // default colors
    {},
    SECONDS(0.5),
    SECONDS(0.03)
};

bool write_default_settings(void) {
	return write_settings(&default_settings);
}

bool read_settings(struct settings *settings) {
	ti_var_t slot = ti_Open(SETTINGS_FILE, "r");

	if(!slot) return false;

	size_t read = ti_Read(settings, sizeof *settings, 1, slot);

	ti_Close(slot);

	return read;
}

bool write_settings(const struct settings *settings) {
	ti_var_t slot = ti_Open(SETTINGS_FILE, "w");

	if(!slot) return false;

	size_t written = ti_Write(settings, sizeof *settings, 1, slot);

	ti_Close(slot);

	return written;
}
