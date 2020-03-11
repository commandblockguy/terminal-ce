/*
 *--------------------------------------
 * Program Name: Terminal CE
 * Author: commandblockguy
 * License:
 * Description: VT100 emulator for the TI-84+ CE
 *--------------------------------------
*/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <keypadc.h>
#include <graphx.h>
#include <fontlibc.h>
#include <fileioc.h>

#include <debug.h>

#define usb_callback_data_t usb_device_t
#include <usbdrvce.h>
#include <srldrvce.h>

#include "terminal.h"
#include "settings.h"
#include "input.h"

#include "gfx/gfx.h"
#include "graphics.h"

//#define ECHO
#define SERIAL
//#define TEST_DATA
#define test_data lynx_out

extern unsigned char test_data[15251];

#ifdef SERIAL
/* Get the usb_device_t for each newly attached device */
static usb_error_t handle_usb_event(usb_event_t event, void *event_data,
								  usb_callback_data_t *callback_data) {
	if(event == USB_DEVICE_CONNECTED_EVENT || event == USB_HOST_CONFIGURE_EVENT) {
		dbg_sprintf(dbgout, "Device connected.\n");
		if(!*callback_data) {
			*callback_data = event_data;
		}
	}
	if(event == USB_DEVICE_DISCONNECTED_EVENT) {
		dbg_sprintf(dbgout, "Device disconnected.\n");
		*callback_data = NULL;
	}
	return USB_SUCCESS;
}

void serial_out(char *str, size_t len, void *data) {
	srl_Write(data, str, len);
}
#endif

#ifdef ECHO
/* Temporary? input callback function that echoes data to the terminal */
void echo(char *str, size_t len, void *data) {
	dbg_sprintf(dbgout, "%s", str);
	write_data(data, str, len);
}
#endif

#ifdef TEST_DATA
void ignore(char *str, size_t len, void *data) {

}
#endif

void main(void) {
#ifdef SERIAL
	usb_error_t error = 0;
	usb_device_t dev = NULL;
	srl_device_t srl;
	static char srlbuf[4096];
#endif
	uint8_t step = 0;

	fontlib_font_t *font;

	settings_t settings;

	static terminal_state_t term = {0};

	int i = 0;

	dbg_sprintf(dbgout, "\nProgram Started\n");

	ti_CloseAll();

	if(!read_settings(&settings)) {
		write_default_settings();
		if(!read_settings(&settings)) {
			dbg_sprintf(dbgerr, "Failed to read settings.\n");
			return;
		}
	}

	term.graphics.fg_color = WHITE;
	term.graphics.bg_color = BLACK;

	font = fontlib_GetFontByIndex(settings.font_pack_name, settings.reg_font);
	if (font) {
		fontlib_SetFont(font, 0);
		term.char_width = fontlib_GetStringWidth(" ");
		term.char_height = fontlib_GetCurrentFontHeight();
	} else {
		dbg_sprintf(dbgerr, "Failed to load font pack %.8s\n", settings.font_pack_name);
		gfx_End();
		return;
	}
#ifdef ECHO
	term.input_callback = echo;
	term.callback_data = &term;
#elif defined SERIAL
	term.input_callback = serial_out;
	term.callback_data = &srl;
#elif defined TEST_DATA
	term.input_callback = ignore;
	term.callback_data = NULL;
#endif

	init_term(&term);


#ifdef SERIAL

	if((error = usb_Init(handle_usb_event, &dev, srl_GetCDCStandardDescriptors(), USB_DEFAULT_INIT_FLAGS))) goto exit;
	step = 1;

	while(!dev) {
		uint8_t val;
		kb_Scan();
		if(kb_IsDown(kb_KeyClear)) {
			goto exit;
		}
		val = usb_HandleEvents();
		if(val)
			dbg_sprintf(dbgerr, "error in HandleEvents %u\n", val);
	}

	dbg_sprintf(dbgout, "usb dev: %p\n", dev);

	if((error = srl_Init(&srl, dev, srlbuf, sizeof(srlbuf), SRL_INTERFACE_ANY))) goto exit;
	step = 2;

	usb_HandleEvents();

	dbg_sprintf(dbgout, "srl dev: %p\n", &srl);

	srl_SetRate(&srl, 115200);

	dbg_sprintf(dbgout, "set rate\n");

#endif

	while(!kb_IsDown(kb_KeyClear)) {
		char buf[64];
		uint8_t len = 0;

		process_input(&term);

#ifdef SERIAL
		while(len < 64) {
			uint8_t last;
			usb_HandleEvents();
			if(!dev) break;
			last = srl_Read(&srl, buf + len, 64 - len);
			if(!last) break;
			len += last;
		}

		write_data(&term, buf, len);
		if(!dev) break;
#endif
#ifdef TEST_DATA
		write_data(&term, &test_data[i], 1);
        dbg_sprintf(dbgout, "%c", test_data[i]);
        i++;
#endif

        render(&term);
    }

	exit:
#ifdef SERIAL
	if(error) {
		char buf[4];
		sprintf(buf, "%u: 0x%X\n", step, error);
		fontlib_DrawString(buf);
		dbg_sprintf(dbgerr, "error %u\n", error);
		while(!kb_IsDown(kb_KeyClear)) kb_Scan();
	}
	usb_Cleanup();
#endif
	gfx_End();
}
