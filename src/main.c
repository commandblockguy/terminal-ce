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

#define ECHO
//#define SERIAL

#ifdef SERIAL
/* Get the usb_device_t for each newly attached device */
static usb_error_t handle_usb_event(usb_event_t event, void *event_data,
								  usb_callback_data_t *callback_data) {
	if(event == USB_DEVICE_ENABLED_EVENT) {
		dbg_sprintf(dbgout, "Device enabled.\n");
		if(!*callback_data) {
			*callback_data = event_data;
		}
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

void main(void) {
	usb_error_t error = 0;
	usb_device_t dev = NULL;
	srl_device_t srl;
	static char srlbuf[4096];

	fontlib_font_t *font;

	settings_t settings;

	terminal_state_t term = {0};

	int i;

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

	gfx_Begin();
	gfx_SetPalette(gfx_pal, sizeof_gfx_pal, 0);
	gfx_FillScreen(term.graphics.bg_color);
	fontlib_SetWindowFullScreen();
	fontlib_SetCursorPosition(0, 0);
	set_colors(&term.graphics);
	fontlib_SetTransparency(false);
	fontlib_SetFirstPrintableCodePoint(32);
	fontlib_SetNewlineOptions(FONTLIB_ENABLE_AUTO_WRAP | FONTLIB_PRECLEAR_NEWLINE | FONTLIB_AUTO_SCROLL);

	font = fontlib_GetFontByIndex(settings.font_pack_name, settings.reg_font);
	if (font) {
		fontlib_SetFont(font, 0);
		term.char_width = fontlib_GetStringWidth(" ");
		term.char_height = fontlib_GetCurrentFontHeight();
		term.cols = LCD_WIDTH / term.char_width;
		term.rows = LCD_HEIGHT / term.char_height;
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
#endif
	term.csr_x = 1;
	term.csr_y = 1;
	set_cursor_pos(&term, 1, 1, true);


#ifdef SERIAL

	if((error = usb_Init(handle_usb_event, &dev, NULL, USB_DEFAULT_INIT_FLAGS))) goto exit;

	while(!dev) {
		uint8_t val;
		kb_Scan();
		if(kb_IsDown(kb_KeyClear)) {
			goto exit;
		}
		val = usb_HandleEvents();
		if(val)
			dbg_sprintf(dbgout, "%u\n", val);
	}

	dbg_sprintf(dbgout, "usb dev: %p\n", dev);

	if((error = srl_Init(&srl, dev, srlbuf, sizeof(srlbuf), 115200))) goto exit;

	usb_HandleEvents();

	dbg_sprintf(dbgout, "srl dev: %p\n", &srl);

#endif

	while(!kb_IsDown(kb_KeyClear)) {
		char buf[64];
		uint8_t len = 0;

		process_input(&term);

#ifdef SERIAL
		while(len < 64) {
			uint8_t last;
			usb_HandleEvents();
			last = srl_Read(&srl, buf, 64 - len);
			if(!last) break;
			len += last;
		}

		write_data(&term, buf, len);
#endif
	}

	exit:
	if(error) {
		char buf[4];
		sprintf(buf, "0x%X\n", error);
		fontlib_DrawString(buf);
		dbg_sprintf(dbgout, "error %u\n", error);
		while(!kb_IsDown(kb_KeyClear)) kb_Scan();
	}
#ifdef SERIAL
	usb_Cleanup();
#endif
	gfx_End();
}
