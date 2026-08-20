#include "settlement.h"

static byte clamp_stat(int v)
{
    if (v < STAT_MIN) return STAT_MIN;
    if (v > STAT_MAX) return STAT_MAX;
    return (byte)v;
}

void settlement_init(Settlement *s, byte id, byte x, byte y, byte owner,
                      byte population, byte wealth, byte reserves,
                      byte infrastructure, byte defense)
{
    byte base, remainder, i;

    s->id = id;
    s->x = x;
    s->y = y;
    s->owner = owner;
    s->alive = 1;
    s->event_status = EVENT_STATUS_NONE;

    s->population = clamp_stat(population);
    s->wealth = clamp_stat(wealth);
    s->reserves = clamp_stat(reserves);
    s->infrastructure = clamp_stat(infrastructure);
    s->defense = clamp_stat(defense);
    s->_pad1 = 0;

    /* Start with an even culture split that still sums to exactly 255. */
    base = CULTURE_VECTOR_SUM / CULTURE_COUNT;
    remainder = CULTURE_VECTOR_SUM % CULTURE_COUNT;
    for (i = 0; i < CULTURE_COUNT; i++) {
        s->culture[i] = base + (i < remainder ? 1 : 0);
    }
}

byte settlement_get_stat(const Settlement *s, StatField f)
{
    switch (f) {
        case STAT_POPULATION:    return s->population;
        case STAT_WEALTH:        return s->wealth;
        case STAT_RESERVES:      return s->reserves;
        case STAT_INFRASTRUCTURE:return s->infrastructure;
        case STAT_DEFENSE:       return s->defense;
        default:                 return 0;
    }
}

void settlement_set_stat(Settlement *s, StatField f, byte value)
{
    byte v = clamp_stat(value);
    switch (f) {
        case STAT_POPULATION:     s->population = v; break;
        case STAT_WEALTH:         s->wealth = v; break;
        case STAT_RESERVES:       s->reserves = v; break;
        case STAT_INFRASTRUCTURE: s->infrastructure = v; break;
        case STAT_DEFENSE:        s->defense = v; break;
        default: break;
    }
}

void settlement_add_stat(Settlement *s, StatField f, int delta)
{
    int cur = settlement_get_stat(s, f);
    settlement_set_stat(s, f, (byte)clamp_stat(cur + delta));
}

CultureFocus settlement_dominant_focus(const Settlement *s)
{
    byte best = 0, i;
    for (i = 1; i < CULTURE_COUNT; i++) {
        if (s->culture[i] > s->culture[best]) best = i;
    }
    return (CultureFocus)best;
}

void settlement_shift_culture(Settlement *s, CultureFocus focus, int amount)
{
    int cur, newval, applied, remaining, others_sum, i, n;
    byte other_idx[CULTURE_COUNT - 1];
    int shares[CULTURE_COUNT - 1];
    int shared_total;

    cur = s->culture[focus];
    newval = cur + amount;
    if (newval > 255) newval = 255;
    if (newval < 0) newval = 0;
    applied = newval - cur;
    if (applied == 0) return;

    s->culture[focus] = (byte)newval;

    n = 0;
    others_sum = 0;
    for (i = 0; i < CULTURE_COUNT; i++) {
        if (i == (int)focus) continue;
        other_idx[n] = (byte)i;
        others_sum += s->culture[i];
        n++;
    }

    /* The other three focuses must absorb -applied between them. */
    remaining = -applied;

    if (others_sum == 0) {
        /* Nothing to take from (or all are already at 0); split evenly. */
        int base = remaining / n;
        int rem = remaining - base * n;
        for (i = 0; i < n; i++) {
            shares[i] = base + (i < rem ? 1 : 0);
        }
    } else {
        shared_total = 0;
        for (i = 0; i < n - 1; i++) {
            shares[i] = (remaining * s->culture[other_idx[i]]) / others_sum;
            shared_total += shares[i];
        }
        /* last share absorbs the rounding remainder so the total is exact */
        shares[n - 1] = remaining - shared_total;
    }

    for (i = 0; i < n; i++) {
        int v = (int)s->culture[other_idx[i]] + shares[i];
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        s->culture[other_idx[i]] = (byte)v;
    }

    /* Clamping above can leave a small residual; force the invariant by
       applying it to the largest remaining focus (never the one we just
       set, so we don't undo the caller's intended change). */
    {
        int sum = 0;
        byte largest;
        for (i = 0; i < CULTURE_COUNT; i++) sum += s->culture[i];
        if (sum != CULTURE_VECTOR_SUM) {
            largest = other_idx[0];
            for (i = 1; i < n; i++) {
                if (s->culture[other_idx[i]] > s->culture[largest]) largest = other_idx[i];
            }
            {
                int v = (int)s->culture[largest] + (CULTURE_VECTOR_SUM - sum);
                if (v < 0) v = 0;
                if (v > 255) v = 255;
                s->culture[largest] = (byte)v;
            }
        }
    }
}

byte settlement_is_collapsed(const Settlement *s)
{
    return s->population == 0;
}

/* Scales a base percent chance down by how little headroom is left below
   STAT_MAX -- growth tapers off as a stat approaches its ceiling instead
   of racing to 15 and pinning there, and naturally reaches 0 exactly at
   the cap (no separate "already full" check needed). */
static int recovery_chance(int base_pct, byte current)
{
    int headroom = STAT_MAX - (int)current;
    if (headroom <= 0) return 0;
    return base_pct * headroom / STAT_MAX;
}

void settlement_recover(Settlement *s, Rng *rng)
{
    if (!s->alive || s->population == 0) return;

    if (rng_range(rng, 100) < (word)recovery_chance(20 + s->culture[CULTURE_GRO] / 8,
                                                      settlement_get_stat(s, STAT_RESERVES))) {
        settlement_add_stat(s, STAT_RESERVES, 1);
    }

    if (rng_range(rng, 100) < (word)recovery_chance(15 + s->culture[CULTURE_TRA] / 8,
                                                      settlement_get_stat(s, STAT_WEALTH))) {
        settlement_add_stat(s, STAT_WEALTH, 1);
    }

    /* Infrastructure and Defense are investments: they only rebuild once
       there's a reserves cushion to spend, and spending it costs reserves. */
    if (settlement_get_stat(s, STAT_RESERVES) >= 4) {
        if (rng_range(rng, 100) < (word)recovery_chance(10 + s->culture[CULTURE_GRO] / 8,
                                                          settlement_get_stat(s, STAT_INFRASTRUCTURE))) {
            settlement_add_stat(s, STAT_INFRASTRUCTURE, 1);
            settlement_add_stat(s, STAT_RESERVES, -1);
        }
        if (rng_range(rng, 100) < (word)recovery_chance(10 + s->culture[CULTURE_SEC] / 8,
                                                          settlement_get_stat(s, STAT_DEFENSE))) {
            settlement_add_stat(s, STAT_DEFENSE, 1);
            settlement_add_stat(s, STAT_RESERVES, -1);
        }
    }

    /* Population grows once both Wealth and Infrastructure have a cushion. */
    if (settlement_get_stat(s, STAT_WEALTH) >= 4 &&
        settlement_get_stat(s, STAT_INFRASTRUCTURE) >= 4 &&
        rng_range(rng, 100) < (word)recovery_chance(8 + s->culture[CULTURE_GRO] / 16,
                                                      settlement_get_stat(s, STAT_POPULATION))) {
        settlement_add_stat(s, STAT_POPULATION, 1);
    }
}
