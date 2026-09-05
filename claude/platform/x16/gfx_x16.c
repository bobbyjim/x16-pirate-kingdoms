#include <conio.h>

#include "../../present/gfx.h"

void gfx_init(void)
{
    clrscr();
}

void gfx_draw_tile(byte x, byte y, byte tile_id, byte palette)
{
    (void)x;
    (void)y;
    (void)tile_id;
    (void)palette;
}

void gfx_draw_text(byte x, byte y, const char *s, byte color)
{
    (void)color;
    gotoxy(x, y);
    cputs(s);
}

void gfx_draw_sprite(byte slot, int x, int y, byte frame)
{
    (void)slot;
    (void)x;
    (void)y;
    (void)frame;
}

void gfx_present(void) {}