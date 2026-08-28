#ifndef _PRESENT_H_
#define _PRESENT_H_

#include "../engine/world.h"

/* Presentation layer: the only place the "what happened -> what the player
   sees/hears" mapping lives. It sits between the engine and the HAL --
   polling World for the persistent picture (not done here yet) and
   draining World.notes for the transient flourishes. Swap the linked HAL
   (platform/null | platform/debug | platform/x16) without touching this. */

void present_init(void);

/* Turn every Note from the tick just run into gfx/sound feedback. Call
   once after each world_tick() / world_force_event(). */
void present_drain_notes(const World *w);

#endif
