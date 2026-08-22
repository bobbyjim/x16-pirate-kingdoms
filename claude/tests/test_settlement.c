#include "test.h"
#include "../engine/settlement.h"

static int culture_sum(const Settlement *s)
{
    byte v[CULTURE_COUNT];
    int i, sum = 0;
    settlement_culture_vector(s, v);
    for (i = 0; i < CULTURE_COUNT; i++) sum += v[i];
    return sum;
}

void run_settlement_tests(void)
{
    Settlement s;
    int i;

    /* init: capacity clamps, nothing built, culture is an even split with
       no structures, settlement reads as collapsed. */
    settlement_init(&s, 0, 10, 20, 1, 200);
    CHECK(s.capacity == MAX_STRUCTURE_SLOTS);
    CHECK(s.alive == 1);
    CHECK(settlement_structure_count(&s) == 0);
    CHECK(settlement_is_collapsed(&s));
    CHECK(culture_sum(&s) == CULTURE_VECTOR_SUM);

    /* building fills the first empty slot within capacity, at full
       condition, and is reflected in characteristic totals immediately. */
    settlement_init(&s, 1, 0, 0, 0, 2);
    CHECK(settlement_build(&s, STRUCT_DOCK) == 0);
    CHECK(!settlement_is_collapsed(&s));
    CHECK(settlement_structure_count(&s) == 1);
    CHECK(settlement_characteristic(&s, CHAR_COMMERCE) == STAT_MAX);         /* Dock primary */
    CHECK(settlement_characteristic(&s, CHAR_DEFENSE) == STAT_MAX / 2);      /* Dock secondary */

    /* capacity is a hard ceiling: a third build attempt with capacity 2
       fails once both slots are full. */
    CHECK(settlement_build(&s, STRUCT_WAREHOUSE) == 1);
    CHECK(settlement_build(&s, STRUCT_FORT) == -1);
    CHECK(settlement_structure_count(&s) == 2);

    /* damaging a slot to 0 destroys it (loss, not just degradation) and
       its characteristic contribution disappears with it. */
    settlement_damage_slot(&s, 0, -STAT_MAX);
    CHECK(settlement_structure_count(&s) == 1);
    CHECK(settlement_characteristic(&s, CHAR_COMMERCE) == STAT_MAX);         /* Warehouse primary */

    /* now that slot 0 is empty again, a build can reuse it */
    CHECK(settlement_build(&s, STRUCT_FORT) == 0);

    /* damage_slot clamps at STAT_MAX on the way up too */
    settlement_damage_slot(&s, 0, 1000);
    CHECK(settlement_structure_count(&s) == 2);

    /* a settlement with every structure destroyed collapses */
    settlement_init(&s, 2, 0, 0, 0, 3);
    settlement_build(&s, STRUCT_TOWNHALL);
    settlement_damage_slot(&s, 0, -STAT_MAX);
    CHECK(settlement_is_collapsed(&s));

    /* characteristic damage hits every contributing slot: full to a
       primary match, half (min 1) to a secondary match. */
    settlement_init(&s, 3, 0, 0, 0, 3);
    settlement_build(&s, STRUCT_DOCK);      /* primary Commerce, secondary Defense */
    settlement_build(&s, STRUCT_WAREHOUSE); /* primary Commerce, secondary Industry */
    settlement_damage_characteristic(&s, CHAR_COMMERCE, 5);
    CHECK(s.structures[0].condition == STAT_MAX - 5);
    CHECK(s.structures[1].condition == STAT_MAX - 5);
    settlement_damage_characteristic(&s, CHAR_DEFENSE, 4);
    CHECK(s.structures[0].condition == STAT_MAX - 5 - 2); /* secondary match: half of 4 */

    /* dominant focus and the culture vector invariant hold across many
       different compositions, including the empty (fallback) case. */
    settlement_init(&s, 4, 0, 0, 0, MAX_STRUCTURE_SLOTS);
    for (i = 0; i < 5; i++) settlement_build(&s, STRUCT_FORT);
    CHECK(settlement_dominant_focus(&s) == CULTURE_SEC || settlement_dominant_focus(&s) == CULTURE_AGR);
    CHECK(culture_sum(&s) == CULTURE_VECTOR_SUM);

    /* recovery: a battered but alive settlement with a damaged structure
       should climb back toward full condition given enough ticks -- with
       a fixed seed and 500 tries, never once saturating would be
       astronomically unlikely, so this is deterministic in practice. */
    {
        Rng rng;
        rng_seed(&rng, 1234);
        /* Capacity 1 (not 3): this test is about a single slot's condition
           climbing back to full, not about the "recovery of last resort"
           investment/conversion machinery -- give that nothing to work
           with (no empty slot, no redundant duplicate) so it can't dilute
           this settlement's culture vector or touch slot 0. */
        settlement_init(&s, 5, 0, 0, 0, 1);
        settlement_build(&s, STRUCT_TOWNHALL);
        settlement_damage_slot(&s, 0, -(STAT_MAX - 1)); /* leave it at condition 1 */
        for (i = 0; i < 500; i++) settlement_recover(&s, &rng);
        CHECK(s.structures[0].condition == STAT_MAX);
    }

    /* a dead settlement (no structures left) never recovers on its own */
    {
        Rng rng;
        rng_seed(&rng, 1);
        settlement_init(&s, 6, 0, 0, 0, 3);
        s.alive = 0;
        for (i = 0; i < 100; i++) settlement_recover(&s, &rng);
        CHECK(settlement_structure_count(&s) == 0);
    }
}
