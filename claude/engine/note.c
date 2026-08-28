#include "note.h"

static const char *NOTE_TYPE_NAMES[NOTE_TYPE_COUNT] = {
    "event_struck",
    "settlement_collapsed",
    "faction_spawned",
    "colonized",
    "ruin_resettled",
    "link_formed",
    "link_disrupted",
    "link_blocked",
    "caravan_arrived"
};

const char *note_type_name(NoteType type)
{
    if ((int)type < 0 || type >= NOTE_TYPE_COUNT) return "?";
    return NOTE_TYPE_NAMES[type];
}
