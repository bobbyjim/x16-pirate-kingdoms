#include "test.h"
#include "../engine/settlement.h"

static int culture_sum(const Settlement *s)
{
    int i, sum = 0;
    for (i = 0; i < CULTURE_COUNT; i++) sum += s->culture[i];
    return sum;
}

void run_settlement_tests(void)
{
    Settlement s;
    int i;

    /* init: stats clamp, culture starts summed to the constant */
    settlement_init(&s, 0, 10, 20, 1, 30, 200, 5, 0, 15);
    CHECK(settlement_get_stat(&s, STAT_POPULATION) == STAT_MAX); /* 30 clamps to 15 */
    CHECK(settlement_get_stat(&s, STAT_WEALTH) == STAT_MAX);     /* 200 clamps to 15 */
    CHECK(settlement_get_stat(&s, STAT_RESERVES) == 5);
    CHECK(settlement_get_stat(&s, STAT_INFRASTRUCTURE) == STAT_MIN);
    CHECK(settlement_get_stat(&s, STAT_DEFENSE) == STAT_MAX);
    CHECK(culture_sum(&s) == CULTURE_VECTOR_SUM);
    CHECK(s.alive == 1);
    CHECK(!settlement_is_collapsed(&s));

    /* stat add/set clamp at both ends */
    settlement_set_stat(&s, STAT_POPULATION, 0);
    settlement_add_stat(&s, STAT_POPULATION, -5);
    CHECK(settlement_get_stat(&s, STAT_POPULATION) == STAT_MIN);
    CHECK(settlement_is_collapsed(&s));

    settlement_set_stat(&s, STAT_POPULATION, STAT_MAX);
    settlement_add_stat(&s, STAT_POPULATION, 100);
    CHECK(settlement_get_stat(&s, STAT_POPULATION) == STAT_MAX);

    /* culture shift keeps the constant-sum invariant, including across
       many repeated shifts and edge cases where others hit 0 */
    settlement_init(&s, 1, 0, 0, 0, 5, 5, 5, 5, 5);
    for (i = 0; i < 50; i++) {
        settlement_shift_culture(&s, CULTURE_TRA, (i % 2 == 0) ? 17 : -23);
        CHECK(culture_sum(&s) == CULTURE_VECTOR_SUM);
    }

    /* dominant focus reflects the largest component */
    settlement_init(&s, 2, 0, 0, 0, 5, 5, 5, 5, 5);
    settlement_shift_culture(&s, CULTURE_SEC, 200);
    CHECK(settlement_dominant_focus(&s) == CULTURE_SEC);
    CHECK(culture_sum(&s) == CULTURE_VECTOR_SUM);

    /* pushing one focus to its ceiling still balances against the rest */
    settlement_shift_culture(&s, CULTURE_SEC, 1000);
    CHECK(s.culture[CULTURE_SEC] == 255);
    CHECK(culture_sum(&s) == CULTURE_VECTOR_SUM);

    /* recovery: a battered but alive, growth-leaning settlement should
       climb back toward full stats given enough ticks -- with a fixed
       seed and 500 tries, never once saturating would be astronomically
       unlikely, so this is deterministic in practice, not flaky. */
    {
        Rng rng;
        rng_seed(&rng, 1234);
        settlement_init(&s, 3, 0, 0, 0, /*pop*/1, 0, 0, 0, 0);
        settlement_shift_culture(&s, CULTURE_GRO, 200 - s.culture[CULTURE_GRO]);
        for (i = 0; i < 500; i++) settlement_recover(&s, &rng);
        CHECK(settlement_get_stat(&s, STAT_RESERVES) == STAT_MAX);
        CHECK(settlement_get_stat(&s, STAT_INFRASTRUCTURE) == STAT_MAX);
        CHECK(settlement_get_stat(&s, STAT_POPULATION) > 1);
    }

    /* a dead settlement (population 0) never recovers on its own */
    {
        Rng rng;
        rng_seed(&rng, 1);
        settlement_init(&s, 4, 0, 0, 0, /*pop*/0, 5, 5, 5, 5);
        s.alive = 0;
        for (i = 0; i < 100; i++) settlement_recover(&s, &rng);
        CHECK(settlement_get_stat(&s, STAT_RESERVES) == 5);
        CHECK(settlement_get_stat(&s, STAT_WEALTH) == 5);
    }
}
