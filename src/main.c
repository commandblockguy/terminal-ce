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

#undef NDEBUG
#define DEBUG
#include <debug.h>

#define usb_callback_data_t usb_device_t
#include <usbdrvce.h>
#include <srldrvce.h>

static usb_error_t handleUsbEvent(usb_event_t event, void *event_data,
                                  usb_callback_data_t *callback_data) {
    if(event == USB_DEVICE_ENABLED_EVENT) {
        if(!*callback_data) {
            *callback_data = event_data;
        }
    }
    return USB_SUCCESS;
}

void main(void) {
    usb_error_t error;
    usb_device_t dev = NULL;
    srl_device_t srl;
    static char srlbuf[4096];

    dbg_sprintf(dbgout, "srl device: %p\n", &srl);
    fontlib_font_t *font;

    gfx_Begin();
    gfx_FillScreen(gfx_black);
    fontlib_SetWindowFullScreen();
    fontlib_SetCursorPosition(1, 1);
    fontlib_SetColors(gfx_white, gfx_black);
    fontlib_SetTransparency(false);
    fontlib_SetFirstPrintableCodePoint(32);
    fontlib_SetNewlineOptions(FONTLIB_ENABLE_AUTO_WRAP | FONTLIB_PRECLEAR_NEWLINE | FONTLIB_AUTO_SCROLL);

    font = fontlib_GetFontByIndex(settings.font_pack_name, 1);
    if (font) {
        fontlib_SetFont(font, 0);
    } else {
        dbg_sprintf(dbgerr, "Failed to load font pack %.8s\n", settings.font_pack_name);
        return;
    }

    if((error = usb_Init(handleUsbEvent, &dev, NULL, USB_DEFAULT_INIT_FLAGS))) goto exit;
    
    while(!dev) {
        kb_Scan();
        if(kb_IsDown(kb_KeyClear)) {
            goto exit;
            return;
        }
        usb_WaitForEvents();
    }

    if(srl_Init(&srl, dev, srlbuf, sizeof(srlbuf), 115200));

    dbg_sprintf(dbgout, "usb dev: %p\n", dev);

    exit:
    usb_Cleanup();
    gfx_End();
}