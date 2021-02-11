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

#define SERIAL_BUF_SIZE 4096

struct event_callback_data {
    srl_device_t *srl;
    bool *has_device;
    char *buf;
    terminal_state_t *term;
};

struct srl_callback_data {
    srl_device_t *srl;
    bool *has_device;
};

static usb_error_t handle_usb_event(usb_event_t event, void *event_data,
								  usb_callback_data_t *callback_data) {
    const struct event_callback_data *cb_data = callback_data;
    if((event == USB_DEVICE_CONNECTED_EVENT && !(usb_GetRole() & USB_ROLE_DEVICE)) || event == USB_HOST_CONFIGURE_EVENT) {
        if(!*cb_data->has_device) {
            usb_device_t device = event_data;

            /* Initialize the serial library with the newly attached device */
            srl_error_t error = srl_Init(cb_data->srl, device, cb_data->buf, SERIAL_BUF_SIZE, SRL_INTERFACE_ANY);

            if(error) {
                write_string(cb_data->term, "Error initting serial\r\n");
            } else {
                *cb_data->has_device = true;
            }
        }
    }
	if(event == USB_DEVICE_DISCONNECTED_EVENT) {
		dbg_sprintf(dbgout, "Device disconnected.\n");
		*cb_data->has_device = false;
	}
	return USB_SUCCESS;
}

void serial_out(char *str, size_t len, void *data) {
    struct srl_callback_data *srl_out = data;
    if(*srl_out->has_device) {
        srl_Write(srl_out->srl, str, len);
    }
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

int main(void) {
    static terminal_state_t term = {0};
#ifdef SERIAL
	usb_error_t error;
	srl_device_t srl;
	bool has_srl_device = false;
    static char srlbuf[4096];
#endif
	uint8_t step = 0;

	fontlib_font_t *font;

	settings_t settings;

	dbg_sprintf(dbgout, "\nProgram Started\n");

	ti_CloseAll();

	if(!read_settings(&settings)) {
		write_default_settings();
		if(!read_settings(&settings)) {
			dbg_sprintf(dbgerr, "Failed to read settings.\n");
			return 1;
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
		return 1;
	}
#ifdef ECHO
	term.input_callback = echo;
	term.callback_data = &term;
#elif defined SERIAL
	term.input_callback = serial_out;
	struct srl_callback_data cb_data = {&srl, &has_srl_device};
	term.callback_data = &cb_data;
#elif defined TEST_DATA
	term.input_callback = ignore;
	term.callback_data = NULL;
#endif

	init_term(&term);


#ifdef SERIAL

	struct event_callback_data callback_data = {
	        &srl,
	        &has_srl_device,
	        srlbuf,
	        &term
	};
	if((error = usb_Init(handle_usb_event, &callback_data, srl_GetCDCStandardDescriptors(), USB_DEFAULT_INIT_FLAGS))) goto exit;
	step = 1;

	usb_HandleEvents();

#endif

	while(!kb_IsDown(kb_KeyClear)) {
		char buf[64];
		uint8_t len = 0;

		process_input(&term);

#ifdef SERIAL
		if(has_srl_device) {
            while(len < 64) {
                uint8_t last;
                usb_HandleEvents();
                if(!has_srl_device) break;
                last = srl_Read(&srl, buf + len, 64 - len);
                if(!last) break;
                len += last;
            }

            write_data(&term, buf, len);
        }
        usb_HandleEvents();
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
		char buf[16];
		sprintf(buf, "%u: 0x%X\n", step, error);
		write_string(&term, buf);
		dbg_sprintf(dbgerr, "error %u\n", error);
		while(!kb_IsDown(kb_KeyClear)) kb_Scan();
	}
	usb_Cleanup();
#endif
	gfx_End();
	return 0;
}
