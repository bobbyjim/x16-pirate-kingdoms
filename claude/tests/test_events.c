#include <string.h>
#include "test.h"
#include "../engine/events.h"
#include "../engine/rng.h"

static int culture_sum(const Settlement *s)
{
    byte v[CULTURE_COUNT];
    int i, sum = 0;
    settlement_culture_vector(s, v);
    for (i = 0; i < CULTURE_COUNT; i++) sum += v[i];
    return sum;
}

void run_events_tests(void)
{
    Settlement weak, strong;
    Rng rng_a, rng_b;
    EventResult result;
    int i;

    CHECK(event_from_name("pirates") == EVENT_PIRATES);
    CHECK(event_from_name("civil_war") == EVENT_CIVIL_WAR);
    CHECK(event_from_name("nonsense") == -1);
    CHECK(strcmp(event_name(EVENT_DROUGHT), "drought") == 0);

    /* weakness exploitation: a settlement whose Fort is barely holding on
       (low condition, low SEC) should take at least as much damage from
       Pirates as one with a healthy Fort and high SEC, given the same rng
       sequence. */
    settlement_init(&weak, 0, 0, 0, 0, 3);
    settlement_build(&weak, STRUCT_FORT);
    settlement_damage_slot(&weak, 0, -10); /* condition 5, stressed -> low SEC */

    settlement_init(&strong, 1, 0, 0, 0, 3);
    settlement_build(&strong, STRUCT_FORT); /* full condition -> healthy, high SEC */

    rng_seed(&rng_a, 42);
    rng_seed(&rng_b, 42);

    {
        byte weak_defense_before = weak.structures[0].condition;
        byte strong_defense_before = strong.structures[0].condition;
        byte weak_defense_after, strong_defense_after;
        int weak_damage, strong_damage;

        event_apply(&weak, EVENT_PIRATES, &rng_a, &result);
        event_apply(&strong, EVENT_PIRATES, &rng_b, &result);

        weak_defense_after = (weak.structures[0].type == STRUCT_EMPTY) ? 0 : weak.structures[0].condition;
        strong_defense_after = (strong.structures[0].type == STRUCT_EMPTY) ? 0 : strong.structures[0].condition;

        weak_damage = weak_defense_before - weak_defense_after;
        strong_damage = strong_defense_before - strong_defense_after;

        CHECK(weak_damage >= strong_damage);
        CHECK(weak_defense_after <= STAT_MAX);
        CHECK(strong_defense_after <= STAT_MAX);
    }

    /* culture vector invariant holds across every event type, and every
       slot's condition stays in range afterward */
    for (i = 0; i < EVENT_COUNT; i++) {
        Settlement s;
        Rng rng;
        byte j;

        settlement_init(&s, (byte)i, 0, 0, 0, MAX_STRUCTURE_SLOTS);
        settlement_build(&s, STRUCT_DOCK);
        settlement_build(&s, STRUCT_WAREHOUSE);
        settlement_build(&s, STRUCT_FORT);
        settlement_build(&s, STRUCT_TOWNHALL);
        settlement_build(&s, STRUCT_MONUMENT);

        rng_seed(&rng, (unsigned long)(i + 1));
        event_apply(&s, (EventType)i, &rng, &result);

        for (j = 0; j < MAX_STRUCTURE_SLOTS; j++) {
            if (s.structures[j].type == STRUCT_EMPTY) continue;
            CHECK(s.structures[j].condition <= STAT_MAX);
        }
        CHECK(culture_sum(&s) == CULTURE_VECTOR_SUM);
        CHECK(s.event_status == (byte)i);
    }

    /* civil war: destroying the only Fort (defense posture -> 0) while a
       Town Hall still stands (population support > 0) signals a split */
    settlement_init(&weak, 2, 0, 0, 0, 3);
    settlement_build(&weak, STRUCT_FORT);
    settlement_build(&weak, STRUCT_TOWNHALL);
    settlement_damage_slot(&weak, 0, -(STAT_MAX - 1)); /* Fort at condition 1: one hit from gone */
    rng_seed(&rng_a, 7);
    event_apply(&weak, EVENT_CIVIL_WAR, &rng_a, &result);
    CHECK(result.spawned_faction == 1);
}
