#include "settlement.h"

/* Primary/secondary characteristic per BUSINESS-LOGIC.md's "Structure
   Taxonomy" section, indexed by StructureType. */
static const struct { byte primary, secondary; } STRUCTURE_INFO[STRUCT_TYPE_COUNT] = {
    /* DOCK      */ { CHAR_COMMERCE, CHAR_DEFENSE },
    /* WAREHOUSE */ { CHAR_COMMERCE, CHAR_INDUSTRY },
    /* FORT      */ { CHAR_DEFENSE, CHAR_POPULATION },
    /* TOWNHALL  */ { CHAR_POPULATION, CHAR_CULTURE },
    /* MONUMENT  */ { CHAR_CULTURE, CHAR_COMMERCE }
};

/* Which culture focus a structure type's upkeep/investment favors -- used
   only by settlement_recover(), not by settlement_culture_vector(). */
static const CultureFocus STRUCTURE_GROWTH_FOCUS[STRUCT_TYPE_COUNT] = {
    CULTURE_TRA, /* DOCK */
    CULTURE_TRA, /* WAREHOUSE */
    CULTURE_SEC, /* FORT */
    CULTURE_GRO, /* TOWNHALL */
    CULTURE_GRO  /* MONUMENT */
};

/* Which structure type to invest in to (re-)establish a characteristic
   that has fallen to zero -- see the "recovery-of-last-resort" step in
   settlement_recover(). Every characteristic has exactly one type for
   which it's the *primary*, except Industry (only ever a secondary, on
   Warehouse), so Warehouse doubles up as Industry's owner here too. */
static const StructureType STRUCTURE_FOR_CHARACTERISTIC[CHAR_COUNT] = {
    STRUCT_FORT,      /* CHAR_DEFENSE */
    STRUCT_DOCK,      /* CHAR_COMMERCE */
    STRUCT_WAREHOUSE, /* CHAR_INDUSTRY */
    STRUCT_TOWNHALL,  /* CHAR_POPULATION */
    STRUCT_MONUMENT   /* CHAR_CULTURE */
};

void settlement_init(Settlement *s, byte id, byte x, byte y, byte owner, byte capacity)
{
    byte i;

    s->id = id;
    s->x = x;
    s->y = y;
    s->owner = owner;
    s->event_status = EVENT_STATUS_NONE;
    s->alive = 1;
    s->capacity = (capacity > MAX_STRUCTURE_SLOTS) ? MAX_STRUCTURE_SLOTS : capacity;
    s->spare = 0;

    for (i = 0; i < MAX_STRUCTURE_SLOTS; i++) {
        s->structures[i].type = STRUCT_EMPTY;
        s->structures[i].condition = 0;
    }
}

int settlement_build(Settlement *s, StructureType type)
{
    byte i, limit;

    if (type >= STRUCT_TYPE_COUNT) return -1;

    limit = (s->capacity < MAX_STRUCTURE_SLOTS) ? s->capacity : MAX_STRUCTURE_SLOTS;
    for (i = 0; i < limit; i++) {
        if (s->structures[i].type == STRUCT_EMPTY) {
            s->structures[i].type = type;
            s->structures[i].condition = STAT_MAX;
            return (int)i;
        }
    }
    return -1;
}

void settlement_damage_slot(Settlement *s, byte index, int delta)
{
    int v;

    if (index >= MAX_STRUCTURE_SLOTS) return;
    if (s->structures[index].type == STRUCT_EMPTY) return;

    v = (int)s->structures[index].condition + delta;
    if (v <= 0) {
        s->structures[index].type = STRUCT_EMPTY;
        s->structures[index].condition = 0;
        return;
    }
    if (v > STAT_MAX) v = STAT_MAX;
    s->structures[index].condition = (byte)v;
}

void settlement_damage_characteristic(Settlement *s, CharacteristicField target, int amount)
{
    byte i;

    if (amount <= 0) return;

    for (i = 0; i < MAX_STRUCTURE_SLOTS; i++) {
        StructureSlot *slot = &s->structures[i];
        if (slot->type == STRUCT_EMPTY || slot->type >= STRUCT_TYPE_COUNT) continue;

        if (STRUCTURE_INFO[slot->type].primary == (byte)target) {
            settlement_damage_slot(s, i, -amount);
        } else if (STRUCTURE_INFO[slot->type].secondary == (byte)target) {
            int half = amount / 2;
            if (half < 1) half = 1;
            settlement_damage_slot(s, i, -half);
        }
    }
}

void settlement_repair_characteristic(Settlement *s, CharacteristicField target, int amount)
{
    byte i;

    if (amount <= 0) return;

    for (i = 0; i < MAX_STRUCTURE_SLOTS; i++) {
        StructureSlot *slot = &s->structures[i];
        if (slot->type == STRUCT_EMPTY || slot->type >= STRUCT_TYPE_COUNT) continue;

        if (STRUCTURE_INFO[slot->type].primary == (byte)target) {
            settlement_damage_slot(s, i, amount);
        } else if (STRUCTURE_INFO[slot->type].secondary == (byte)target) {
            int half = amount / 2;
            if (half < 1) half = 1;
            settlement_damage_slot(s, i, half);
        }
    }
}

byte settlement_has_structure(const Settlement *s, StructureType type)
{
    byte i;
    for (i = 0; i < MAX_STRUCTURE_SLOTS; i++) {
        if (s->structures[i].type == type) return 1;
    }
    return 0;
}

void settlement_nudge_focus(Settlement *s, CultureFocus focus)
{
    StructureType pick;
    switch (focus) {
        case CULTURE_TRA: pick = STRUCT_WAREHOUSE; break;
        case CULTURE_AGR: pick = STRUCT_FORT; break;
        case CULTURE_GRO: pick = STRUCT_TOWNHALL; break;
        case CULTURE_SEC: default: pick = STRUCT_FORT; break;
    }
    settlement_build(s, pick); /* no-op if no empty slot within capacity */
}

byte settlement_structure_count(const Settlement *s)
{
    byte i, n = 0;
    for (i = 0; i < MAX_STRUCTURE_SLOTS; i++) {
        if (s->structures[i].type != STRUCT_EMPTY) n++;
    }
    return n;
}

word settlement_characteristic(const Settlement *s, CharacteristicField c)
{
    word total = 0;
    byte i;

    for (i = 0; i < MAX_STRUCTURE_SLOTS; i++) {
        const StructureSlot *slot = &s->structures[i];
        if (slot->type == STRUCT_EMPTY || slot->type >= STRUCT_TYPE_COUNT) continue;

        if (STRUCTURE_INFO[slot->type].primary == (byte)c) {
            total += slot->condition;
        } else if (STRUCTURE_INFO[slot->type].secondary == (byte)c) {
            total += slot->condition / 2;
        }
    }
    return total;
}

byte settlement_infrastructure_resilience(const Settlement *s)
{
    word sum = 0;
    byte i, count = 0;

    for (i = 0; i < MAX_STRUCTURE_SLOTS; i++) {
        if (s->structures[i].type == STRUCT_EMPTY) continue;
        sum += s->structures[i].condition;
        count++;
    }
    if (count == 0) return 0;
    return (byte)(sum / count);
}

word settlement_population_support(const Settlement *s) { return settlement_characteristic(s, CHAR_POPULATION); }
word settlement_wealth_potential(const Settlement *s)   { return settlement_characteristic(s, CHAR_COMMERCE); }
word settlement_reserve_potential(const Settlement *s)  { return settlement_characteristic(s, CHAR_INDUSTRY); }
word settlement_defense_posture(const Settlement *s)    { return settlement_characteristic(s, CHAR_DEFENSE); }

void settlement_culture_vector(const Settlement *s, byte out[CULTURE_COUNT])
{
    word defense    = settlement_characteristic(s, CHAR_DEFENSE);
    word commerce   = settlement_characteristic(s, CHAR_COMMERCE);
    word industry   = settlement_characteristic(s, CHAR_INDUSTRY);
    word population = settlement_characteristic(s, CHAR_POPULATION);
    word culture_ch = settlement_characteristic(s, CHAR_CULTURE);
    byte resilience = settlement_infrastructure_resilience(s); /* 0-15 health */
    word agr_share, sec_share;
    word raw[CULTURE_COUNT];
    word total;
    byte i;

    /* Defense reads as coercive/AGR when the garrison is stressed (low
       resilience) and as stabilizing/SEC when it's healthy -- same
       structures, different posture, matching BUSINESS-LOGIC.md's "AGR:
       ...militarized and stress-reactive" vs "SEC: ...protective and
       stabilizing". Industry and Culture don't map to a single focus, so
       each is folded into the focus its purpose most plausibly supports:
       Industry (productive capacity) feeds expansion (GRO); Culture
       (civic cohesion) stabilizes and retains (SEC), per the doc's "Town
       Hall + Monument ... cohesion and internal retention". */
    agr_share = defense * (word)(STAT_MAX - resilience) / STAT_MAX;
    sec_share = defense - agr_share;

    raw[CULTURE_TRA] = commerce;
    raw[CULTURE_AGR] = agr_share;
    raw[CULTURE_GRO] = population + industry;
    raw[CULTURE_SEC] = sec_share + culture_ch;

    total = 0;
    for (i = 0; i < CULTURE_COUNT; i++) total += raw[i];

    if (total == 0) {
        /* Nothing built yet (or perfectly balanced-to-zero): fall back to
           an even split that still sums to exactly CULTURE_VECTOR_SUM. */
        byte base = CULTURE_VECTOR_SUM / CULTURE_COUNT;
        byte remainder = CULTURE_VECTOR_SUM % CULTURE_COUNT;
        for (i = 0; i < CULTURE_COUNT; i++) out[i] = (byte)(base + (i < remainder ? 1 : 0));
        return;
    }

    {
        word assigned = 0;
        for (i = 0; i < CULTURE_COUNT - 1; i++) {
            word v = raw[i] * CULTURE_VECTOR_SUM / total;
            out[i] = (byte)v;
            assigned += v;
        }
        /* last share absorbs the rounding remainder so the total is exact */
        out[CULTURE_COUNT - 1] = (byte)(CULTURE_VECTOR_SUM - assigned);
    }
}

CultureFocus settlement_dominant_focus(const Settlement *s)
{
    byte v[CULTURE_COUNT];
    byte best = 0, i;

    settlement_culture_vector(s, v);
    for (i = 1; i < CULTURE_COUNT; i++) {
        if (v[i] > v[best]) best = i;
    }
    return (CultureFocus)best;
}

byte settlement_is_collapsed(const Settlement *s)
{
    return settlement_structure_count(s) == 0;
}

/* Scales a base percent chance down by how little headroom is left below
   STAT_MAX -- growth tapers off as condition approaches its ceiling
   instead of racing to 15 and pinning there. */
static int recovery_chance(int base_pct, byte current)
{
    int headroom = STAT_MAX - (int)current;
    if (headroom <= 0) return 0;
    return base_pct * headroom / STAT_MAX;
}

static byte has_empty_slot_within_capacity(const Settlement *s)
{
    byte i, limit = (s->capacity < MAX_STRUCTURE_SLOTS) ? s->capacity : MAX_STRUCTURE_SLOTS;
    for (i = 0; i < limit; i++) {
        if (s->structures[i].type == STRUCT_EMPTY) return 1;
    }
    return 0;
}

void settlement_recover(Settlement *s, Rng *rng)
{
    byte culture[CULTURE_COUNT];
    byte i;

    if (!s->alive || settlement_is_collapsed(s)) return;

    settlement_culture_vector(s, culture);

    for (i = 0; i < MAX_STRUCTURE_SLOTS; i++) {
        StructureSlot *slot = &s->structures[i];
        int bonus;

        if (slot->type == STRUCT_EMPTY || slot->type >= STRUCT_TYPE_COUNT) continue;

        bonus = culture[STRUCTURE_GROWTH_FOCUS[slot->type]] / 32;
        if (rng_range(rng, 100) < (word)recovery_chance(15 + bonus, slot->condition)) {
            settlement_damage_slot(s, i, 1);
        }
    }

    /* Given spare reserve potential (stockpiling headroom) and an empty
       slot within capacity, occasionally invest in a brand-new structure
       biased toward the settlement's dominant focus -- the structural
       equivalent of the old direct-stat model's "population grows once
       wealth and infrastructure have a cushion". */
    if (settlement_reserve_potential(s) >= 4 && has_empty_slot_within_capacity(s) &&
        rng_range(rng, 100) < 8) {
        StructureType pick;
        switch (settlement_dominant_focus(s)) {
            case CULTURE_TRA: pick = (rng_range(rng, 2) == 0) ? STRUCT_DOCK : STRUCT_WAREHOUSE; break;
            case CULTURE_AGR: pick = STRUCT_FORT; break;
            case CULTURE_GRO: pick = STRUCT_TOWNHALL; break;
            case CULTURE_SEC: default: pick = STRUCT_FORT; break;
        }
        settlement_build(s, pick);
    }

    /* Recovery-of-last-resort: even when the dominant focus doesn't favor
       it, occasionally invest in whichever characteristic has fallen to
       zero. Without this, a settlement that loses every Commerce
       structure (its only source of TRA) can never roll TRA-dominant
       again to justify rebuilding one -- a permanent lock-in the
       dominant-focus investment above can't break on its own. Not gated
       on reserve potential like that investment is, since Industry (the
       source of reserve potential) can itself be the zeroed-out
       characteristic this is meant to restore. */
    if (rng_range(rng, 100) < 5) {
        /* Collect every zeroed characteristic and pick one at random --
           Defense is zero at least as often as anything else (many
           settlements never build a Fort), and always taking the
           lowest-numbered one in CHAR_* order would starve every other
           characteristic's chance to ever get picked. */
        CharacteristicField zeroed[CHAR_COUNT];
        byte n = 0, c;

        for (c = 0; c < CHAR_COUNT; c++) {
            if (settlement_characteristic(s, (CharacteristicField)c) == 0) zeroed[n++] = (CharacteristicField)c;
        }

        if (n > 0) {
            StructureType pick = STRUCTURE_FOR_CHARACTERISTIC[zeroed[rng_range(rng, n)]];

            if (settlement_build(s, pick) < 0) {
                /* No empty slot: capacity has filled entirely with one or
                   two duplicated types (recovery above keeps investing in
                   the dominant focus's favorite), which would otherwise
                   make the lock-in permanent even at full capacity.
                   Convert one *redundant* slot -- a type this settlement
                   already has more than one of -- into the needed
                   structure instead. */
                byte counts[STRUCT_TYPE_COUNT];
                byte i;

                for (i = 0; i < STRUCT_TYPE_COUNT; i++) counts[i] = 0;
                for (i = 0; i < MAX_STRUCTURE_SLOTS; i++) {
                    if (s->structures[i].type < STRUCT_TYPE_COUNT) counts[s->structures[i].type]++;
                }
                for (i = 0; i < MAX_STRUCTURE_SLOTS; i++) {
                    byte t = s->structures[i].type;
                    if (t < STRUCT_TYPE_COUNT && counts[t] > 1) {
                        s->structures[i].type = pick;
                        s->structures[i].condition = STAT_MAX;
                        break;
                    }
                }
            }
        }
    }
}
