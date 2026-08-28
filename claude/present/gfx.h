#ifndef _GFX_H_
#define _GFX_H_

#include "../engine/common.h"

/* Platform graphics primitives -- deliberately free of any game concept
   (no settlements, links, events). One implementation is linked per build:
     platform/null   -- empty bodies (silent host runs, tests)
     platform/debug  -- stdio trace (host CLI, so `tick` shows feedback)
     platform/x16    -- VERA (the real Commander X16 build; not written yet)

   Only the presentation layer includes this. The simulation engine never
   does -- see present/present.c and note.h for why. */

void gfx_init(void);
void gfx_draw_tile(byte x, byte y, byte tile_id, byte palette);
void gfx_draw_text(byte x, byte y, const char *s, byte color);
void gfx_draw_sprite(byte slot, int x, int y, byte frame);
void gfx_present(void); /* flush a completed frame */

#endif
