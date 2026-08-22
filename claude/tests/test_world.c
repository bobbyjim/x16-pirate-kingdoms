#include <stddef.h>
#include "test.h"
#include "../engine/world.h"

#define SAMPLE_MAP "tests/sample.map"

static int culture_sum(const Settlement *s)
{
    byte v[CULTURE_COUNT];
    int i, sum = 0;
    settlement_culture_vector(s, v);
    for (i = 0; i < CULTURE_COUNT; i++) sum += v[i];
    return sum;
}

/* Founds the smallest possible settlement directly into a world slot, for
   tests that need a specific, fragile starting point rather than whatever
   world_load()/settlement recovery would produce. Capacity 1 with a single
   near-destroyed Warehouse (Industry secondary): EVENT_DROUGHT tests
   CHAR_INDUSTRY, and settlement_damage_characteristic()'s secondary-match
   floor is "at least 1", so any Drought roll is guaranteed to finish it
   off and collapse the settlement. */
static void make_fragile_settlement(World *w, word idx, byte x, byte y)
{
    Settlement *s = &w->settlements[idx];
    settlement_init(s, (byte)idx, x, y, 0, 1);
    settlement_build(s, STRUCT_WAREHOUSE);
    settlement_damage_slot(s, 0, -(STAT_MAX - 1)); /* condition 1: one hit from gone */
}

/* Founds a settlement with a full, healthy set of structures -- used as a
   "survivor" that should not itself be at risk of collapsing during a test. */
static void make_healthy_settlement(World *w, word idx, byte x, byte y)
{
    Settlement *s = &w->settlements[idx];
    settlement_init(s, (byte)idx, x, y, 0, MAX_STRUCTURE_SLOTS);
    settlement_build(s, STRUCT_DOCK);
    settlement_build(s, STRUCT_WAREHOUSE);
    settlement_build(s, STRUCT_FORT);
    settlement_build(s, STRUCT_TOWNHALL);
    settlement_build(s, STRUCT_MONUMENT);
}

static void test_load_and_tick(void)
{
    World w;
    unsigned long tick0;
    word i;

    CHECK(world_load(&w, SAMPLE_MAP, 12345) == 0);
    CHECK(w.settlement_count > 0);
    CHECK(w.settlement_count <= MAX_SETTLEMENTS);

    for (i = 0; i < w.settlement_count; i++) {
        CHECK(w.settlements[i].alive == 1);
        CHECK(settlement_structure_count(&w.settlements[i]) > 0); /* every load seeds a Town Hall */
        CHECK(culture_sum(&w.settlements[i]) == CULTURE_VECTOR_SUM);
    }

    tick0 = w.tick;
    world_tick(&w);
    CHECK(w.tick == tick0 + 1);

    /* running a bunch of ticks never corrupts the culture-vector invariant
       or lets a slot's condition escape 0-15, even as settlements collapse
       and factions spawn */
    for (i = 0; i < 200; i++) world_tick(&w);
    for (i = 0; i < w.settlement_count; i++) {
        byte j;
        CHECK(culture_sum(&w.settlements[i]) == CULTURE_VECTOR_SUM);
        for (j = 0; j < MAX_STRUCTURE_SLOTS; j++) {
            if (w.settlements[i].structures[j].type == STRUCT_EMPTY) continue;
            CHECK(w.settlements[i].structures[j].condition <= STAT_MAX);
        }
    }
}

static void test_force_event(void)
{
    World w;
    Settlement *s;

    CHECK(world_load(&w, SAMPLE_MAP, 1) == 0);
    CHECK(world_get_settlement(&w, (byte)w.settlement_count) == NULL); /* out of range */

    s = world_get_settlement(&w, 0);
    CHECK(s != NULL);
    world_force_event(&w, 0, EVENT_PIRATES);
    s = world_get_settlement(&w, 0);
    CHECK(s->event_status == EVENT_PIRATES);
}

/* Cascade behavior is tested against a small hand-built world so it's not
   at the mercy of the sample map's actual layout. */
static void test_refugee_cascade(void)
{
    World w;

    world_init_empty(&w, 99);
    w.settlement_count = 2;

    /* Two adjacent settlements: forcing #0 (fragile) to collapse via
       Drought should strain #1 too. */
    make_fragile_settlement(&w, 0, 10, 10);
    make_healthy_settlement(&w, 1, 12, 12);

    {
        word reserve_before = settlement_reserve_potential(&w.settlements[1]);
        word wealth_before = settlement_wealth_potential(&w.settlements[1]);

        world_force_event(&w, 0, EVENT_DROUGHT);

        CHECK(w.settlements[0].alive == 0);
        /* neighbor was strained by the refugee cascade: strain is always
           >= 2, so its Industry (reserve potential) contributors must have
           taken damage. */
        CHECK(settlement_reserve_potential(&w.settlements[1]) < reserve_before ||
              settlement_wealth_potential(&w.settlements[1]) < wealth_before);
    }
}

static void test_civil_war_spawns_faction(void)
{
    World w;
    word count_before;

    world_init_empty(&w, 5);
    w.settlement_count = 1;
    settlement_init(&w.settlements[0], 0, 50, 50, 0, 3);
    settlement_build(&w.settlements[0], STRUCT_FORT);
    settlement_build(&w.settlements[0], STRUCT_TOWNHALL);
    settlement_damage_slot(&w.settlements[0], 0, -(STAT_MAX - 1)); /* Fort one hit from gone */

    count_before = w.settlement_count;
    world_force_event(&w, 0, EVENT_CIVIL_WAR);

    CHECK(w.settlement_count == count_before + 1);
    if (w.settlement_count == count_before + 1) {
        Settlement *child = &w.settlements[count_before];
        CHECK(settlement_dominant_focus(child) == CULTURE_AGR);
        CHECK(culture_sum(child) == CULTURE_VECTOR_SUM);
    }
}

/* Refugees from a collapsed settlement sometimes found a brand-new
   encampment nearby instead of (or alongside) straining a neighbor. It's
   a COLONIZE_CHANCE_PCT roll, so this runs several independent trials
   with different seeds -- the odds of zero successes across 20 trials are
   astronomically small, so this is deterministic in practice. */
static void test_refugee_founds_colony(void)
{
    int trial;
    int founded_at_least_once = 0;

    for (trial = 0; trial < 20; trial++) {
        World w;
        word i;

        world_init_empty(&w, (unsigned long)(trial + 1) * 97);
        for (i = 0; i < MAP_DATA_BYTES; i += MAP_CELL_BYTES) w.map.data[i] = TERRAIN_GRASS;

        w.settlement_count = 1;
        make_fragile_settlement(&w, 0, 100, 100);

        world_force_event(&w, 0, EVENT_DROUGHT);

        CHECK(w.settlements[0].alive == 0);
        if (w.settlement_count > 1) {
            Settlement *colony = &w.settlements[1];
            founded_at_least_once = 1;
            CHECK(settlement_structure_count(colony) > 0);
            CHECK(colony->alive == 1);
        }
    }
    CHECK(founded_at_least_once);
}

/* On an all-water map there's nowhere to land, so no colony is founded --
   refugees are simply lost/absorbed rather than the search hanging or
   overflowing the settlement table. */
static void test_refugee_colony_needs_land(void)
{
    World w;

    world_init_empty(&w, 555); /* map_init_empty() -> all water */
    w.settlement_count = 1;
    make_fragile_settlement(&w, 0, 100, 100);

    world_force_event(&w, 0, EVENT_DROUGHT);

    CHECK(w.settlements[0].alive == 0);
    CHECK(w.settlement_count == 1);
}

/* The event-chance "weather" starts at the midpoint, drifts by at most
   EVENT_CHANCE_STEP per tick, and never leaves [EVENT_CHANCE_MIN,
   EVENT_CHANCE_MAX] no matter how long it runs. */
static void test_event_weather_walk(void)
{
    World w;
    byte prev;
    int i;

    world_init_empty(&w, 2024);
    CHECK(w.event_chance_pct == (EVENT_CHANCE_MIN + EVENT_CHANCE_MAX) / 2);

    prev = w.event_chance_pct;
    for (i = 0; i < 500; i++) {
        int delta;

        world_tick(&w);

        CHECK(w.event_chance_pct >= EVENT_CHANCE_MIN);
        CHECK(w.event_chance_pct <= EVENT_CHANCE_MAX);

        delta = (int)w.event_chance_pct - (int)prev;
        CHECK(delta >= -EVENT_CHANCE_STEP && delta <= EVENT_CHANCE_STEP);
        prev = w.event_chance_pct;
    }
}

/* Builds a world with 4 healthy, widely-spaced survivors plus one fragile
   settlement at (100,100) that EVENT_DROUGHT will collapse. `baseline`
   becomes initial_settlement_count: setting it equal to the post-collapse
   alive count (4) means no shortfall/no boost; setting it much higher
   simulates a world that used to be far more populous. */
static void setup_one_collapse_among_survivors(World *w, unsigned long seed, word baseline)
{
    word i;

    world_init_empty(w, seed);
    for (i = 0; i < MAP_DATA_BYTES; i += MAP_CELL_BYTES) w->map.data[i] = TERRAIN_GRASS;

    w->settlement_count = 5;
    w->initial_settlement_count = baseline;

    make_fragile_settlement(w, 0, 100, 100);
    make_healthy_settlement(w, 1, 10, 10);
    make_healthy_settlement(w, 2, 20, 20);
    make_healthy_settlement(w, 3, 30, 30);
    make_healthy_settlement(w, 4, 40, 40);
}

/* A sparse world (few alive relative to initial_settlement_count) should
   colonize far more reliably than a full one -- run the same collapse in
   both configurations and compare success rates over many trials. */
static void test_sparse_world_colonizes_more_readily(void)
{
    int trial;
    int sparse_successes = 0, full_successes = 0;
    const int trials = 30;

    for (trial = 0; trial < trials; trial++) {
        World w;
        unsigned long seed = (unsigned long)(trial + 1) * 131;

        /* Sparse: world "started with" far more than the 4 survivors left
           alive once #0 collapses -- a huge shortfall. The boost is
           deliberately halved (see colonize_chance()) so even an extreme
           deficit asymptotically approaches ~90%, never a guaranteed
           100% -- that asymptote is what "huge baseline" is chosen to hit. */
        setup_one_collapse_among_survivors(&w, seed, 1000);
        world_force_event(&w, 0, EVENT_DROUGHT);
        if (w.settlement_count > 5) sparse_successes++;
    }

    for (trial = 0; trial < trials; trial++) {
        World w;
        unsigned long seed = (unsigned long)(trial + 1) * 131;

        /* Full: baseline exactly matches the post-collapse alive count (4)
           -- no shortfall, so no boost above the flat COLONIZE_CHANCE_PCT. */
        setup_one_collapse_among_survivors(&w, seed, 4);
        world_force_event(&w, 0, EVENT_DROUGHT);
        if (w.settlement_count > 5) full_successes++;
    }

    CHECK(sparse_successes > full_successes);
    /* ~90% vs ~40% -- comfortably separated even allowing for rng noise */
    CHECK(sparse_successes >= trials * 6 / 10);
    CHECK(full_successes <= trials * 6 / 10);
}

/* Dead settlements' slots must be reusable, or a long-running world burns
   through MAX_SETTLEMENTS on corpses and permanently locks out further
   colonization/civil-war splits (both gated on that same cap). Trigger two
   civil-war splits in a row, killing the first child in between, and
   confirm the second reuses its slot instead of allocating a new one. */
static void test_dead_slots_are_reused(void)
{
    World w;
    word after_first_split;

    world_init_empty(&w, 3);
    w.settlement_count = 1;
    settlement_init(&w.settlements[0], 0, 50, 50, 0, 3);
    settlement_build(&w.settlements[0], STRUCT_FORT);
    settlement_build(&w.settlements[0], STRUCT_TOWNHALL);
    settlement_damage_slot(&w.settlements[0], 0, -(STAT_MAX - 1));

    world_force_event(&w, 0, EVENT_CIVIL_WAR);
    after_first_split = w.settlement_count;
    CHECK(after_first_split == 2);

    /* Kill the freshly spawned faction, then force another civil war on
       the still-defenseless parent (its Fort is long gone -- rebuild one
       just to knock down again). */
    w.settlements[1].alive = 0;
    if (settlement_structure_count(&w.settlements[0]) < w.settlements[0].capacity) {
        int slot = settlement_build(&w.settlements[0], STRUCT_FORT);
        if (slot >= 0) settlement_damage_slot(&w.settlements[0], (byte)slot, -(STAT_MAX - 1));
    }
    world_force_event(&w, 0, EVENT_CIVIL_WAR);

    CHECK(w.settlement_count == after_first_split); /* reused slot 1, didn't grow to 3 */
    CHECK(w.settlements[1].alive == 1);
}

/* A fully extinct world (every settlement dead) must not be a permanent
   dead end: given enough ticks, RESETTLE_RUIN_CHANCE_PCT should eventually
   land on one of the corpses and revive it in place, without ever growing
   settlement_count. */
static void test_ambient_resettlement_revives_extinct_world(void)
{
    World w;
    word i;
    int tick;
    byte revived = 0;

    world_init_empty(&w, 4242);
    for (i = 0; i < MAP_DATA_BYTES; i += MAP_CELL_BYTES) w.map.data[i] = TERRAIN_GRASS;

    w.settlement_count = 3;
    for (i = 0; i < 3; i++) {
        settlement_init(&w.settlements[i], (byte)i, (byte)(10 + i * 10), (byte)(10 + i * 10), 0, 3);
        settlement_build(&w.settlements[i], STRUCT_TOWNHALL);
        w.settlements[i].alive = 0;
    }

    for (tick = 0; tick < 1000 && !revived; tick++) {
        word count_before = w.settlement_count;
        world_tick(&w);
        CHECK(w.settlement_count == count_before);
        for (i = 0; i < w.settlement_count; i++) {
            if (w.settlements[i].alive) { revived = 1; break; }
        }
    }

    CHECK(revived);
}

/* A world that has never had any settlements at all has no ruins to
   reclaim -- resettlement should quietly do nothing, not crash on an
   empty table. */
static void test_ambient_resettlement_noop_on_empty_world(void)
{
    World w;
    int i;

    world_init_empty(&w, 1);
    for (i = 0; i < 200; i++) world_tick(&w);
    CHECK(w.settlement_count == 0);
}

void run_world_tests(void)
{
    test_load_and_tick();
    test_force_event();
    test_refugee_cascade();
    test_civil_war_spawns_faction();
    test_refugee_founds_colony();
    test_refugee_colony_needs_land();
    test_event_weather_walk();
    test_sparse_world_colonizes_more_readily();
    test_dead_slots_are_reused();
    test_ambient_resettlement_revives_extinct_world();
    test_ambient_resettlement_noop_on_empty_world();
}
