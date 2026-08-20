#include <stddef.h>
#include "test.h"
#include "../engine/world.h"

#define SAMPLE_MAP "tests/sample.map"

static int culture_sum(const Settlement *s)
{
    int i, sum = 0;
    for (i = 0; i < CULTURE_COUNT; i++) sum += s->culture[i];
    return sum;
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
        CHECK(culture_sum(&w.settlements[i]) == CULTURE_VECTOR_SUM);
    }

    tick0 = w.tick;
    world_tick(&w);
    CHECK(w.tick == tick0 + 1);

    /* running a bunch of ticks never corrupts stat/culture invariants,
       even as settlements collapse and factions spawn */
    for (i = 0; i < 200; i++) world_tick(&w);
    for (i = 0; i < w.settlement_count; i++) {
        int f;
        CHECK(culture_sum(&w.settlements[i]) == CULTURE_VECTOR_SUM);
        for (f = 0; f < STAT_COUNT; f++) {
            CHECK(settlement_get_stat(&w.settlements[i], (StatField)f) <= STAT_MAX);
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

    /* Two adjacent, fragile settlements: forcing #0 to collapse should
       strain #1 too. Drought's secondary erosion hits population once
       reserves bottom out, so a reserves=0, GRO=0 settlement is guaranteed
       to lose population and collapse. */
    settlement_init(&w.settlements[0], 0, 10, 10, 0, /*pop*/1, 1, /*reserves*/0, 1, 5);
    settlement_shift_culture(&w.settlements[0], CULTURE_GRO, -w.settlements[0].culture[CULTURE_GRO]);
    settlement_init(&w.settlements[1], 1, 12, 12, 0, /*pop*/5, 3, 3, 1, 15);

    world_force_event(&w, 0, EVENT_DROUGHT);

    CHECK(w.settlements[0].alive == 0);
    /* neighbor was strained by the refugee cascade: strain is always >= 2,
       so reserves (started at 3) must have dropped. */
    CHECK(settlement_get_stat(&w.settlements[1], STAT_RESERVES) < 3);
}

static void test_civil_war_spawns_faction(void)
{
    World w;
    word count_before;

    world_init_empty(&w, 5);
    w.settlement_count = 1;
    settlement_init(&w.settlements[0], 0, 50, 50, 0, /*pop*/10, 5, 5, 5, /*defense*/0);
    settlement_shift_culture(&w.settlements[0], CULTURE_SEC, -w.settlements[0].culture[CULTURE_SEC]);

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
        settlement_init(&w.settlements[0], 0, 100, 100, 0, /*pop*/1, 1, /*reserves*/0, 1, 5);
        settlement_shift_culture(&w.settlements[0], CULTURE_GRO, -w.settlements[0].culture[CULTURE_GRO]);

        world_force_event(&w, 0, EVENT_DROUGHT);

        CHECK(w.settlements[0].alive == 0);
        if (w.settlement_count > 1) {
            Settlement *colony = &w.settlements[1];
            founded_at_least_once = 1;
            CHECK(settlement_get_stat(colony, STAT_POPULATION) == 1);
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
    settlement_init(&w.settlements[0], 0, 100, 100, 0, /*pop*/1, 1, /*reserves*/0, 1, 5);
    settlement_shift_culture(&w.settlements[0], CULTURE_GRO, -w.settlements[0].culture[CULTURE_GRO]);

    world_force_event(&w, 0, EVENT_DROUGHT);

    CHECK(w.settlements[0].alive == 0);
    CHECK(w.settlement_count == 1);
}

/* Once a settlement's own dominant focus is already at the extreme
   ceiling, neighbor influence should never move it further -- events don't
   touch culture at all, so with the event-chance walk the only other
   source of change, an already-extreme settlement's culture vector should
   come out of many ticks byte-for-byte unchanged. */
static void test_extreme_culture_resists_influence(void)
{
    World w;
    byte before[CULTURE_COUNT];
    int i;

    world_init_empty(&w, 42);
    w.settlement_count = 2;

    settlement_init(&w.settlements[0], 0, 10, 10, 0, 10, 10, 10, 10, 10);
    settlement_shift_culture(&w.settlements[0], CULTURE_SEC,
                              CULTURE_INFLUENCE_EXTREME_CEILING + 20 - w.settlements[0].culture[CULTURE_SEC]);
    CHECK(w.settlements[0].culture[CULTURE_SEC] >= CULTURE_INFLUENCE_EXTREME_CEILING);

    settlement_init(&w.settlements[1], 1, 11, 11, 0, 10, 10, 10, 10, 10);
    settlement_shift_culture(&w.settlements[1], CULTURE_TRA,
                              255 - w.settlements[1].culture[CULTURE_TRA]);

    for (i = 0; i < CULTURE_COUNT; i++) before[i] = w.settlements[0].culture[i];

    for (i = 0; i < 100; i++) world_tick(&w);

    for (i = 0; i < CULTURE_COUNT; i++) {
        CHECK(w.settlements[0].culture[i] == before[i]);
    }
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

    settlement_init(&w->settlements[0], 0, 100, 100, 0, /*pop*/1, 1, /*reserves*/0, 1, 5);
    settlement_shift_culture(&w->settlements[0], CULTURE_GRO, -w->settlements[0].culture[CULTURE_GRO]);

    settlement_init(&w->settlements[1], 1, 10, 10, 0, 10, 10, 10, 10, 10);
    settlement_init(&w->settlements[2], 2, 20, 20, 0, 10, 10, 10, 10, 10);
    settlement_init(&w->settlements[3], 3, 30, 30, 0, 10, 10, 10, 10, 10);
    settlement_init(&w->settlements[4], 4, 40, 40, 0, 10, 10, 10, 10, 10);
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
    settlement_init(&w.settlements[0], 0, 50, 50, 0, /*pop*/10, 5, 5, 5, /*defense*/0);
    settlement_shift_culture(&w.settlements[0], CULTURE_SEC, -w.settlements[0].culture[CULTURE_SEC]);

    world_force_event(&w, 0, EVENT_CIVIL_WAR);
    after_first_split = w.settlement_count;
    CHECK(after_first_split == 2);

    /* Kill the freshly spawned faction, then force another civil war on
       the still-defenseless parent. */
    w.settlements[1].alive = 0;
    settlement_set_stat(&w.settlements[0], STAT_DEFENSE, 0);
    settlement_set_stat(&w.settlements[0], STAT_POPULATION, 10);
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
        settlement_init(&w.settlements[i], (byte)i, (byte)(10 + i * 10), (byte)(10 + i * 10),
                         0, 5, 5, 5, 5, 5);
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
    test_extreme_culture_resists_influence();
    test_event_weather_walk();
    test_sparse_world_colonizes_more_readily();
    test_dead_slots_are_reused();
    test_ambient_resettlement_revives_extinct_world();
    test_ambient_resettlement_noop_on_empty_world();
}
