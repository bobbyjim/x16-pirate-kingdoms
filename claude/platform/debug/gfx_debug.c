/* Debug graphics HAL: traces every primitive to stdout. Host-only (uses
   stdio); linked into pk-cli so simulation feedback is visible without an
   X16. Swap for platform/null to silence it. */

#include <stdio.h>
#include "../../present/gfx.h"

void gfx_init(void) { printf("[gfx] init\n"); }

void gfx_draw_tile(byte x, byte y, byte tile_id, byte palette)
{ printf("[gfx] tile (%u,%u) id=%u pal=%u\n", x, y, tile_id, palette); }

void gfx_draw_text(byte x, byte y, const char *s, byte color)
{ printf("[gfx] text (%u,%u) col=%u \"%s\"\n", x, y, color, s ? s : ""); }

void gfx_draw_sprite(byte slot, int x, int y, byte frame)
{ printf("[gfx] sprite slot=%u (%d,%d) frame=%u\n", slot, x, y, frame); }

void gfx_present(void) { printf("[gfx] present\n"); }
