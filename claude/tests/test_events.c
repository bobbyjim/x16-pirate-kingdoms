#include <string.h>
#include "test.h"
#include "../engine/events.h"
#include "../engine/rng.h"

static int culture_sum(const Settlement *s)
{
    int i, sum = 0;
    for (i = 0; i < CULTURE_COUNT; i++) sum += s->culture[i];
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

    /* weakness exploitation: a settlement with a low tested-stat and a low
       mitigating focus should take at least as much damage as one with
       high stat + high focus, given the same rng sequence. Defense starts
       mid-range (not 0) on both sides so the 0-floor doesn't mask the
       comparison. */
    settlement_init(&weak, 0, 0, 0, 0, /*pop*/10, /*wealth*/10, /*reserves*/0, /*infra*/10, /*defense*/5);
    settlement_shift_culture(&weak, CULTURE_SEC, -weak.culture[CULTURE_SEC]); /* drive SEC toward 0 */

    settlement_init(&strong, 1, 0, 0, 0, /*pop*/10, /*wealth*/10, /*reserves*/15, /*infra*/10, /*defense*/15);
    settlement_shift_culture(&strong, CULTURE_SEC, 255 - strong.culture[CULTURE_SEC]); /* drive SEC toward max */

    rng_seed(&rng_a, 42);
    rng_seed(&rng_b, 42);

    {
        byte weak_defense_before = settlement_get_stat(&weak, STAT_DEFENSE);
        byte strong_defense_before = settlement_get_stat(&strong, STAT_DEFENSE);
        byte weak_defense_after, strong_defense_after;
        int weak_damage, strong_damage;

        event_apply(&weak, EVENT_PIRATES, &rng_a, &result);
        event_apply(&strong, EVENT_PIRATES, &rng_b, &result);

        weak_defense_after = settlement_get_stat(&weak, STAT_DEFENSE);
        strong_defense_after = settlement_get_stat(&strong, STAT_DEFENSE);

        weak_damage = weak_defense_before - weak_defense_after;
        strong_damage = strong_defense_before - strong_defense_after;

        CHECK(weak_damage >= strong_damage);
        CHECK(weak_defense_after <= STAT_MAX && weak_defense_after >= STAT_MIN);
        CHECK(strong_defense_after <= STAT_MAX && strong_defense_after >= STAT_MIN);
    }

    /* stats/culture invariants hold across every event type */
    for (i = 0; i < EVENT_COUNT; i++) {
        Settlement s;
        Rng rng;
        int f;

        settlement_init(&s, (byte)i, 0, 0, 0, 8, 8, 8, 8, 8);
        rng_seed(&rng, (unsigned long)(i + 1));
        event_apply(&s, (EventType)i, &rng, &result);

        for (f = 0; f < STAT_COUNT; f++) {
            byte v = settlement_get_stat(&s, (StatField)f);
            CHECK(v <= STAT_MAX);
        }
        CHECK(culture_sum(&s) == CULTURE_VECTOR_SUM);
        CHECK(s.event_status == (byte)i);
    }

    /* civil war: driving defense to 0 with population > 1 signals a split */
    settlement_init(&weak, 2, 0, 0, 0, /*pop*/10, 5, 5, 5, /*defense*/0);
    settlement_shift_culture(&weak, CULTURE_SEC, -weak.culture[CULTURE_SEC]);
    rng_seed(&rng_a, 7);
    event_apply(&weak, EVENT_CIVIL_WAR, &rng_a, &result);
    CHECK(result.spawned_faction == 1);
}
