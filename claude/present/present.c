#include "present.h"
#include "gfx.h"
#include "sound.h"
#include "../engine/note.h"

void present_init(void)
{
    gfx_init();
    sound_init();
}

/* The game-feel mapping. Deliberately small and centralised: one abstract
   cue per Note kind. A richer front-end would also flash the tile at the
   note's settlement / draw an effect along the route's endpoints -- the
   Note carries enough to do that (see note.h NOTE_LINK_FROM / _TO). */
static void present_note(const Note *n)
{
    switch ((NoteType)n->type) {
        case NOTE_SETTLEMENT_COLLAPSED:
            sound_play(SOUND_BOOM);
            break;

        case NOTE_EVENT_STRUCK:
        case NOTE_LINK_DISRUPTED:
        case NOTE_LINK_BLOCKED:
            sound_play(SOUND_ALERT);
            break;

        case NOTE_LINK_FORMED:
        case NOTE_CARAVAN_ARRIVED:
            sound_play(SOUND_PING);
            break;

        case NOTE_COLONIZED:
        case NOTE_RUIN_RESETTLED:
        case NOTE_FACTION_SPAWNED:
            sound_play(SOUND_CHIME);
            break;

        default:
            break;
    }
}

void present_drain_notes(const World *w)
{
    word i;

    for (i = 0; i < w->note_count; i++) {
        present_note(&w->notes[i]);
    }

    if (w->notes_overflowed) {
        /* More happened this tick than MAX_NOTES holds. A real front-end
           might play one generic "the world convulsed" cue here; the
           stress test in the Makefile exists to keep this path rare. */
        sound_play(SOUND_ALERT);
    }
}
