/*
 *--------------------------------------
 * Program Name: Terminal CE
 * Author: commandblockguy
 * License:
 * Description:
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

/* Get the usb_device_t for each newly attached device */
static usb_error_t handle_usb_event(usb_event_t event, void *event_data,
                                  usb_callback_data_t *callback_data) {
    if(event == USB_DEVICE_ENABLED_EVENT) {
        if(!*callback_data) {
            *callback_data = event_data;
        }
    }
    return USB_SUCCESS;
}

/* Temporary? input callback function that echoes data to the terminal */
void echo(char *str, size_t len, void *data) {
    dbg_sprintf(dbgout, "%s", str);
    write_data(data, str, len);
}

void main(void) {
    usb_error_t error;
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

    gfx_Begin();
    gfx_FillScreen(gfx_black);
    fontlib_SetWindowFullScreen();
    fontlib_SetCursorPosition(0, 0);
    fontlib_SetColors(gfx_white, gfx_black);
    fontlib_SetTransparency(false);
    fontlib_SetFirstPrintableCodePoint(32);
    fontlib_SetNewlineOptions(FONTLIB_ENABLE_AUTO_WRAP | FONTLIB_PRECLEAR_NEWLINE | FONTLIB_AUTO_SCROLL);

    font = fontlib_GetFontByIndex(settings.font_pack_name, 1);
    if (font) {
        fontlib_SetFont(font, 0);
        term.char_width = fontlib_GetStringWidth(" ");
        term.char_height = fontlib_GetCurrentFontHeight();
        term.cols = LCD_WIDTH / term.char_width;
        term.rows = LCD_HEIGHT / term.char_height;
    } else {
        dbg_sprintf(dbgerr, "Failed to load font pack %.8s\n", settings.font_pack_name);
        return;
    }

    term.input_callback = echo;
    term.callback_data = &term;
    term.csr_x = 1;
    term.csr_y = 1;
    set_cursor_pos(&term, 1, 1, true);


    goto skip;

    if((error = usb_Init(handle_usb_event, &dev, NULL, USB_DEFAULT_INIT_FLAGS))) goto exit;
    
    while(!dev) {
        kb_Scan();
        if(kb_IsDown(kb_KeyClear)) {
            goto exit;
            return;
        }
        usb_WaitForEvents();
    }

    dbg_sprintf(dbgout, "usb dev: %p\n", dev);

    if(srl_Init(&srl, dev, srlbuf, sizeof(srlbuf), 115200));

    skip:

    while(!kb_IsDown(kb_KeyClear)) {
        process_input(&term);
    }

    exit:
    usb_Cleanup();
    gfx_End();
}
