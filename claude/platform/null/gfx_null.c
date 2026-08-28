/* Null graphics HAL: every primitive is a no-op. Linked for silent host
   runs and for tests, which assert on World.notes directly and don't care
   about feedback. */

#include "../../present/gfx.h"

void gfx_init(void) {}
void gfx_draw_tile(byte x, byte y, byte tile_id, byte palette)
{ (void)x; (void)y; (void)tile_id; (void)palette; }
void gfx_draw_text(byte x, byte y, const char *s, byte color)
{ (void)x; (void)y; (void)s; (void)color; }
void gfx_draw_sprite(byte slot, int x, int y, byte frame)
{ (void)slot; (void)x; (void)y; (void)frame; }
void gfx_present(void) {}
