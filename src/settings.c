#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fileioc.h>

#include "settings.h"

settings_t default_settings = {
	"GOHUFONT",
	1,
	0,
	0 // default colors
};

bool write_default_settings(void) {
	return write_settings(&default_settings);
}

bool read_settings(settings_t *settings) {
	ti_var_t slot;
	size_t read;

	slot = ti_Open(SETTINGS_FILE, "r");

	if(!slot) return false;

	read = ti_Read(settings, sizeof(settings_t), 1, slot);

	ti_Close(slot);

	return read;
}

bool write_settings(settings_t *settings) {
	ti_var_t slot;
	size_t written;

	slot = ti_Open(SETTINGS_FILE, "w");

	if(!slot) return false;

	written = ti_Write(settings, sizeof(settings_t), 1, slot);

	ti_Close(slot);

	return written;
}
