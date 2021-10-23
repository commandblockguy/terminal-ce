/*
 *--------------------------------------
 * Program Name: Terminal CE
 * Author: commandblockguy
 * License: MIT
 * Description: VT100 emulator for the TI-84+ CE
 *--------------------------------------
*/

#include <stdint.h>

#include <stdio.h>

#include <keypadc.h>
#include <graphx.h>
#include <fontlibc.h>

#include <debug.h>
#include <usbdrvce.h>

#include "graphics.h"
#include "terminal.h"
#include "settings.h"
#include "input.h"
#include "serial.h"

int main(void) {
    dbg_printf("\nProgram Started\n");

    struct settings settings;
    if(!read_settings(&settings)) {
        write_default_settings();
        if(!read_settings(&settings)) {
            dbg_sprintf(dbgerr, "Failed to read settings.\n");
            return 1;
        }
    }

    init_graphics();
    fontlib_font_t *font = fontlib_GetFontByIndex(settings.font_pack_name, settings.reg_font);
    if (font) {
        fontlib_SetFont(font, 0);
    } else {
        dbg_sprintf(dbgerr, "Failed to load font pack %.8s\n", settings.font_pack_name);
        gfx_SetTextFGColor(WHITE);
        gfx_SetTextBGColor(BLACK);
        gfx_PrintStringXY("Font not found: ", 1, 1);
        gfx_PrintString(settings.font_pack_name);
        while(kb_AnyKey()) kb_Scan();
        while(!kb_AnyKey()) kb_Scan();
        goto exit;
    }

    static struct terminal_state term = {0};
    init_term(&term);
    init_serial(&term);

    while(!kb_IsDown(kb_KeyClear)) {
        process_input(&term);
        process_serial(&term);
    }

    exit:
    usb_Cleanup();
    cleanup_graphics();
    return 0;
}
