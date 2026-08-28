#ifndef _NOTE_H_
#define _NOTE_H_

#include "common.h"

/* Transient per-tick record of things that *happened* during world_tick()
   that a presentation layer cannot reconstruct by inspecting end-of-tick
   state -- a settlement that collapsed and was resettled in the same tick,
   a link that formed and was blocked before the tick ended, an event that
   struck and left no lasting trace once structures regrew.

   The engine only ever *records* Notes. It never interprets them and never
   includes a gfx/sound header. The presentation layer drains World.notes
   after each world_tick() and maps each Note to its own feedback (a sound,
   a tile flash, a log line); that mapping is a game-feel decision and
   deliberately does not live here.

   Consumption model: world_tick() clears the buffer at its start and fills
   it as it runs; the caller drains it before calling world_tick() again.
   Every Note in the buffer therefore belongs to the current World.tick --
   so there is no timestamp field. Emission is suppressed during
   world_load()'s bootstrap link pass (that is initial state, not history). */

typedef enum {
    NOTE_EVENT_STRUCK = 0,     /* ref = settlement id, aux = EventType      */
    NOTE_SETTLEMENT_COLLAPSED, /* ref = settlement id                       */
    NOTE_FACTION_SPAWNED,      /* ref = new faction settlement id           */
    NOTE_COLONIZED,            /* ref = new colony settlement id            */
    NOTE_RUIN_RESETTLED,       /* ref = settlement id (ambient resettle)    */
    NOTE_LINK_FORMED,          /* ref = endpoints (see NOTE_LINK_REF), aux = TradeLinkType */
    NOTE_LINK_DISRUPTED,       /* ref = endpoints, aux = EventType          */
    NOTE_LINK_BLOCKED,         /* ref = endpoints (a dead endpoint)         */
    NOTE_CARAVAN_ARRIVED,      /* ref = endpoints -- unused until unit movement lands */
    NOTE_TYPE_COUNT
} NoteType;

/* Link Notes carry their two endpoint settlement ids packed into `ref`
   rather than a trade_links[] index: that table is swap-pop compacted
   within a tick (see remove_trade_link in world.c), so an index captured
   mid-tick can dangle by drain time. Endpoint ids are stable and are what
   a presentation actually wants (draw the effect along the route). */
#define NOTE_LINK_REF(from_id, to_id) ((word)(from_id) | ((word)(to_id) << 8))
#define NOTE_LINK_FROM(ref)           ((byte)((ref) & 0xFF))
#define NOTE_LINK_TO(ref)             ((byte)((ref) >> 8))

/* 4 bytes, no padding on cc65. `ref` is a word: it holds a settlement id,
   or two packed endpoint ids for link Notes (NOTE_LINK_REF). */
typedef struct {
    word ref;
    byte type;
    byte aux;
} Note;

/* Upper bound: in one tick a settlement can be struck, collapse, then
   spawn a faction or found a colony -- ~3 presentation-worthy Notes. With
   MAX_SETTLEMENTS = 256 and EVENT_CHANCE_MAX = 11%, 512 covers a
   harsh-era full world with margin. Past this, world_tick() drops Notes
   and sets World.notes_overflowed. Tune from a stress test. */
#define MAX_NOTES 512

const char *note_type_name(NoteType type);

#endif
