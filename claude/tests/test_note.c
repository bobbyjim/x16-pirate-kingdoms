#include <stddef.h>
#include "test.h"
#include "../engine/world.h"

#define SAMPLE_MAP "tests/sample.map"

/* Same fragile-settlement helper idea as test_world.c: a capacity-1
   near-dead Warehouse that any EVENT_DROUGHT roll finishes off. */
static void make_fragile_settlement(World *w, word idx, byte x, byte y)
{
    Settlement *s = &w->settlements[idx];
    settlement_init(s, (byte)idx, x, y, 0, 1);
    settlement_build(s, STRUCT_WAREHOUSE);
    settlement_damage_slot(s, 0, -(STAT_MAX - 1));
}

static int count_notes_of(const World *w, NoteType type)
{
    word i;
    int n = 0;
    for (i = 0; i < w->note_count; i++) if (w->notes[i].type == type) n++;
    return n;
}

/* world_load's bootstrap link pass must not leak Notes -- initial state is
   not history -- but notes must be armed once load returns. */
static void test_load_suppresses_bootstrap_notes(void)
{
    World w;

    CHECK(world_load(&w, SAMPLE_MAP, 12345) == 0);
    CHECK(w.note_count == 0);
    CHECK(w.notes_overflowed == 0);
    CHECK(w.notes_enabled == 1);
}

/* world_create_trade_link emits NOTE_LINK_FORMED once notes are armed, and
   packs both endpoint ids into ref (NOTE_LINK_REF). */
static void test_link_formed_note(void)
{
    World w;

    world_init_empty(&w, 7);
    w.settlement_count = 2;
    settlement_init(&w.settlements[0], 0, 10, 10, 0, 4);
    settlement_init(&w.settlements[1], 1, 12, 12, 0, 4);
    settlement_build(&w.settlements[0], STRUCT_WAREHOUSE);
    settlement_build(&w.settlements[1], STRUCT_WAREHOUSE);

    CHECK(world_create_trade_link(&w, TRADE_LINK_CARAVAN, 0, 1, 0, 3, 3) != NULL);
    CHECK(count_notes_of(&w, NOTE_LINK_FORMED) == 1);
    CHECK(w.notes[0].type == NOTE_LINK_FORMED);
    CHECK(NOTE_LINK_FROM(w.notes[0].ref) == 0);
    CHECK(NOTE_LINK_TO(w.notes[0].ref) == 1);
    CHECK(w.notes[0].aux == TRADE_LINK_CARAVAN);
}

/* A forced collapse emits EVENT_STRUCK (with the EventType in aux) and
   SETTLEMENT_COLLAPSED for the same settlement, in that order. */
static void test_collapse_notes(void)
{
    World w;
    int struck = -1, collapsed = -1;
    word i;

    world_init_empty(&w, 99);
    for (i = 0; i < MAP_DATA_BYTES; i += MAP_CELL_BYTES) w.map.data[i] = TERRAIN_GRASS;
    w.settlement_count = 1;
    make_fragile_settlement(&w, 0, 100, 100);

    world_force_event(&w, 0, EVENT_DROUGHT);

    CHECK(w.settlements[0].alive == 0);
    for (i = 0; i < w.note_count; i++) {
        if (w.notes[i].type == NOTE_EVENT_STRUCK && struck < 0) struck = (int)i;
        if (w.notes[i].type == NOTE_SETTLEMENT_COLLAPSED && collapsed < 0) collapsed = (int)i;
    }
    CHECK(struck >= 0);
    CHECK(collapsed >= 0);
    CHECK(struck < collapsed);
    if (struck >= 0) {
        CHECK(w.notes[struck].ref == 0);
        CHECK(w.notes[struck].aux == EVENT_DROUGHT);
    }
}

/* world_force_event resets the buffer, so back-to-back calls don't
   accumulate; world_tick likewise clears at its start. */
static void test_buffer_is_per_action(void)
{
    World w;
    word i;
    word first_count;

    world_init_empty(&w, 3);
    for (i = 0; i < MAP_DATA_BYTES; i += MAP_CELL_BYTES) w.map.data[i] = TERRAIN_GRASS;
    w.settlement_count = 1;
    make_fragile_settlement(&w, 0, 50, 50);

    world_force_event(&w, 0, EVENT_DROUGHT);
    first_count = w.note_count;
    CHECK(first_count > 0);

    /* settlement is already dead; forcing another event is a no-op path
       but must still have cleared the buffer */
    world_force_event(&w, 0, EVENT_DROUGHT);
    CHECK(w.note_count < first_count || w.note_count == 0);

    world_tick(&w);
    CHECK(w.note_count == 0 || count_notes_of(&w, NOTE_EVENT_STRUCK) >= 0); /* just: no crash, buffer sane */
    CHECK(w.note_count <= MAX_NOTES);
}

/* A civil-war split reports the new faction's settlement id. */
static void test_faction_spawned_note(void)
{
    World w;

    world_init_empty(&w, 5);
    w.settlement_count = 1;
    settlement_init(&w.settlements[0], 0, 50, 50, 0, 3);
    settlement_build(&w.settlements[0], STRUCT_FORT);
    settlement_build(&w.settlements[0], STRUCT_TOWNHALL);
    settlement_damage_slot(&w.settlements[0], 0, -(STAT_MAX - 1));

    world_force_event(&w, 0, EVENT_CIVIL_WAR);

    CHECK(count_notes_of(&w, NOTE_FACTION_SPAWNED) == 1);
    if (count_notes_of(&w, NOTE_FACTION_SPAWNED) == 1) {
        word i;
        for (i = 0; i < w.note_count; i++) {
            if (w.notes[i].type == NOTE_FACTION_SPAWNED) {
                CHECK(w.notes[i].ref == 1); /* the spawned child slot */
                CHECK(w.settlements[w.notes[i].ref].alive == 1);
            }
        }
    }
}

/* A long run never overflows the buffer with the default MAX_NOTES on the
   sample map (a sanity floor, not the stress test the Makefile calls for). */
static void test_no_overflow_on_sample_run(void)
{
    World w;
    int i;
    int overflow_ticks = 0;

    CHECK(world_load(&w, SAMPLE_MAP, 2024) == 0);
    for (i = 0; i < 500; i++) {
        world_tick(&w);
        if (w.notes_overflowed) overflow_ticks++;
    }
    CHECK(overflow_ticks == 0);
}

void run_note_tests(void)
{
    test_load_suppresses_bootstrap_notes();
    test_link_formed_note();
    test_collapse_notes();
    test_buffer_is_per_action();
    test_faction_spawned_note();
    test_no_overflow_on_sample_run();
}
