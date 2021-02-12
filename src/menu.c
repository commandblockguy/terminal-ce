#include <tice.h>

#include <fontlibc.h>
#include <keypadc.h>
#include <graphx.h>

#include "terminal.h"
#include "settings.h"

#include "graphics.h"

void menu(struct terminal_state *term) {
	struct settings settings;
	char *pack_long_name;

	const uint8_t BORDER_SIZE = 4;
	const uint8_t ROW_HEIGHT = 14;
	const uint8_t START_Y = 3 * BORDER_SIZE + ROW_HEIGHT;

	const char *str_title        = "Settings:";
	const char *str_font_pack    = "Font:";
	const char *str_colors       = "Colors:";

	read_settings(&settings);

	gfx_BlitScreen();

	gfx_FillScreen(WHITE);

	gfx_SetColor(BLACK);
	gfx_SetTextFGColor(BLACK);
	gfx_SetTextScale(1, 1);

	gfx_PrintStringXY(str_title, (LCD_WIDTH - gfx_GetStringWidth(str_title)) / 2, BORDER_SIZE);

	gfx_HorizLine(0, 2 * BORDER_SIZE + 8, LCD_WIDTH);

	gfx_PrintStringXY(str_font_pack,    BORDER_SIZE, START_Y);

	gfx_Rectangle(LCD_WIDTH / 3, START_Y - 2, LCD_WIDTH / 3, 8 + 4);

	pack_long_name = fontlib_GetFontPackName(settings.font_pack_name);

	if(pack_long_name) {
		gfx_PrintStringXY(pack_long_name, (LCD_WIDTH - gfx_GetStringWidth(pack_long_name)) / 2, START_Y);
	}

	do {
		kb_Scan();
	} while(!kb_IsDown(kb_KeyClear));

	while(kb_IsDown(kb_KeyClear)) kb_Scan();

	gfx_BlitBuffer();
}
