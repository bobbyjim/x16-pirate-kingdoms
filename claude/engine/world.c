#include <stddef.h>
#include "world.h"

static void apply_event_with_cascade(World *w, Settlement *s, EventType type, int depth);

/* Collects up to max_out *alive* settlements (excluding `self`) within
   NEIGHBOR_RADIUS tiles, searched directly over the live settlement table
   rather than the map's (now possibly stale) object list -- settlements
   can be created (civil war splits) or die after the map was loaded. */
static int nearby_alive(World *w, const Settlement *self, byte radius, Settlement *out[], int max_out)
{
    word i;
    int found = 0;
    for (i = 0; i < w->settlement_count && found < max_out; i++) {
        Settlement *cand = &w->settlements[i];
        byte dx, dy, dist;

        if (cand == self || !cand->alive) continue;

        dx = (cand->x > self->x) ? (byte)(cand->x - self->x) : (byte)(self->x - cand->x);
        dy = (cand->y > self->y) ? (byte)(cand->y - self->y) : (byte)(self->y - cand->y);
        dist = (dx > dy) ? dx : dy;

        if (dist <= radius) out[found++] = cand;
    }
    return found;
}

int world_load(World *w, const char *map_path, unsigned long seed)
{
    word i, obj_count;

    rng_seed(&w->rng, seed);
    w->tick = 0;
    w->settlement_count = 0;
    w->event_chance_pct = (EVENT_CHANCE_MIN + EVENT_CHANCE_MAX) / 2;

    if (map_load(&w->map, map_path) != 0) return -1;

    obj_count = map_object_count(&w->map);
    for (i = 0; i < obj_count && w->settlement_count < MAX_SETTLEMENTS; i++) {
        MapObject obj;
        byte size, population, wealth, reserves, infrastructure, defense;
        Settlement *s;

        map_get_object(&w->map, i, &obj);
        if (obj.type != OBJ_SETTLEMENT) continue;

        /* Object size byte ranges roughly 20-139 per WORLD-CREATION.md;
           scale into the engine's 0-15 stat range. */
        size = obj.data[0];
        population = (byte)((word)size * STAT_MAX / 139);
        wealth = (byte)(population > 2 ? population - 2 + rng_range(&w->rng, 3) : rng_range(&w->rng, 3));
        reserves = (byte)(rng_range(&w->rng, STAT_MAX + 1));
        infrastructure = (byte)((population + rng_range(&w->rng, STAT_MAX + 1)) / 2);
        defense = (byte)(rng_range(&w->rng, STAT_MAX + 1));

        s = &w->settlements[w->settlement_count];
        settlement_init(s, (byte)w->settlement_count, obj.x, obj.y, /*owner=*/0,
                         population, wealth, reserves, infrastructure, defense);
        w->settlement_count++;
    }

    w->initial_settlement_count = w->settlement_count;
    return 0;
}

void world_init_empty(World *w, unsigned long seed)
{
    rng_seed(&w->rng, seed);
    w->tick = 0;
    w->settlement_count = 0;
    w->event_chance_pct = (EVENT_CHANCE_MIN + EVENT_CHANCE_MAX) / 2;
    w->initial_settlement_count = 0; /* no colonize_chance() boost until set explicitly */
    map_init_empty(&w->map);
}

Settlement *world_get_settlement(World *w, byte id)
{
    if (id >= w->settlement_count) return NULL;
    return &w->settlements[id];
}

/* Returns a Settlement slot for a new civil-war faction or refugee colony
   to be settlement_init()'d into: reuses a dead settlement's slot if one
   exists (its "ruins" get resettled -- fits BUSINESS-LOGIC.md's ruins/
   legacy idea, and matters for real: without reuse, the table only ever
   grows, so a long-running world eventually burns through all
   MAX_SETTLEMENTS slots on corpses and permanently locks out any further
   colonization *or* civil-war splits, both gated on the same cap). Returns
   NULL only if there's no dead slot AND the table is genuinely full. */
/* `exclude` keeps a settlement that just died *this same call* from being
   immediately overwritten by its own refugees/faction -- without it, a
   colony founded from a collapse could reuse the exact slot of the
   settlement it fled from, silently un-killing it in the same tick. */
static Settlement *allocate_settlement_slot(World *w, const Settlement *exclude)
{
    word i;
    for (i = 0; i < w->settlement_count; i++) {
        if (&w->settlements[i] == exclude) continue;
        if (!w->settlements[i].alive) return &w->settlements[i];
    }
    if (w->settlement_count < MAX_SETTLEMENTS) {
        return &w->settlements[w->settlement_count++];
    }
    return NULL;
}

/* Splits `parent` into two on a civil-war defense collapse: the parent
   keeps half its remaining resources, a new AGR-leaning faction settlement
   is created nearby with the other half. No-op if there's no room left in
   the settlement table (static array, no heap -- see world.h). */
static void spawn_faction(World *w, Settlement *parent)
{
    Settlement *child;
    byte pop_half, wealth_half, reserves_half, infra_half, def_half;

    child = allocate_settlement_slot(w, parent);
    if (!child) return;

    pop_half = settlement_get_stat(parent, STAT_POPULATION) / 2;
    wealth_half = settlement_get_stat(parent, STAT_WEALTH) / 2;
    reserves_half = settlement_get_stat(parent, STAT_RESERVES) / 2;
    infra_half = settlement_get_stat(parent, STAT_INFRASTRUCTURE) / 2;
    def_half = settlement_get_stat(parent, STAT_DEFENSE) / 2;

    settlement_add_stat(parent, STAT_POPULATION, -pop_half);
    settlement_add_stat(parent, STAT_WEALTH, -wealth_half);
    settlement_add_stat(parent, STAT_RESERVES, -reserves_half);
    settlement_add_stat(parent, STAT_INFRASTRUCTURE, -infra_half);
    settlement_add_stat(parent, STAT_DEFENSE, -def_half);

    settlement_init(child, (byte)(child - w->settlements), parent->x, parent->y, parent->owner,
                     pop_half, wealth_half, reserves_half, infra_half, def_half);
    /* Aggressive splinter faction: push its culture hard toward AGR. */
    settlement_shift_culture(child, CULTURE_AGR, 160);
}

/* Straining an already-fragile neighbor can push it over the edge too;
   returns nonzero if the neighbor just collapsed as a result. */
static byte apply_refugee_strain(Settlement *neighbor, Rng *rng)
{
    int strain = 2 + (int)rng_range(rng, 3);
    settlement_add_stat(neighbor, STAT_RESERVES, -strain);
    settlement_add_stat(neighbor, STAT_WEALTH, -(strain / 2));
    /* Refugees also crowd the settlement, but only ever help population;
       the strain is what threatens it. */
    settlement_add_stat(neighbor, STAT_POPULATION, -(strain / 3));
    return settlement_is_collapsed(neighbor);
}

/* True if `x,y` isn't within COLONIZE_MIN_SPACING of any alive settlement,
   so a new encampment doesn't land on top of (or absurdly close to) one
   that already exists. */
static byte tile_far_enough_from_settlements(World *w, byte x, byte y)
{
    word i;
    for (i = 0; i < w->settlement_count; i++) {
        Settlement *o = &w->settlements[i];
        byte dx, dy, dist;

        if (!o->alive) continue;

        dx = (o->x > x) ? (byte)(o->x - x) : (byte)(x - o->x);
        dy = (o->y > y) ? (byte)(o->y - y) : (byte)(y - o->y);
        dist = (dx > dy) ? dx : dy;

        if (dist < COLONIZE_MIN_SPACING) return 0;
    }
    return 1;
}

/* Searches near (near_x,near_y) for open, non-water land far enough from
   existing settlements and, if found, founds a brand-new one-population
   encampment there. Silently gives up after COLONIZE_MAX_ATTEMPTS (no
   land found) or if the settlement table is full -- refugees are lost or
   absorbed elsewhere instead. */
static void try_found_colony(World *w, const Settlement *fallen, byte owner, byte near_x, byte near_y)
{
    int attempt;

    for (attempt = 0; attempt < COLONIZE_MAX_ATTEMPTS; attempt++) {
        int span = COLONIZE_SEARCH_RADIUS * 2 + 1;
        int dx = (int)rng_range(&w->rng, (word)span) - COLONIZE_SEARCH_RADIUS;
        int dy = (int)rng_range(&w->rng, (word)span) - COLONIZE_SEARCH_RADIUS;
        int nx = (int)near_x + dx;
        int ny = (int)near_y + dy;
        byte x, y;

        if (nx < 0 || nx > 255 || ny < 0 || ny > 255) continue;
        x = (byte)nx;
        y = (byte)ny;

        if (map_get_terrain_at(&w->map, x, y) == TERRAIN_WATER) continue;
        if (!tile_far_enough_from_settlements(w, x, y)) continue;

        {
            Settlement *colony = allocate_settlement_slot(w, fallen);
            if (!colony) return; /* table genuinely full */
            settlement_init(colony, (byte)(colony - w->settlements), x, y, owner, 1, 1, 1, 1, 1);
        }
        return;
    }
}

static word count_alive_settlements(const World *w)
{
    word i, count = 0;
    for (i = 0; i < w->settlement_count; i++) {
        if (w->settlements[i].alive) count++;
    }
    return count;
}

/* The fewer settlements are alive relative to how many the world started
   with, the likelier refugees are to found a new one -- see the comment
   on COLONIZE_CHANCE_PCT in world.h for why this is population pressure,
   not a difficulty-adjusting director. initial_settlement_count == 0
   (worlds built by hand, e.g. in tests, rather than world_load()) means
   no baseline to compare against, so the boost is simply skipped. */
static int colonize_chance(World *w)
{
    word alive, baseline;
    int chance = COLONIZE_CHANCE_PCT;

    baseline = w->initial_settlement_count;
    if (baseline == 0) return chance;

    alive = count_alive_settlements(w);
    if (alive < baseline) {
        /* Halved so a modest shortfall doesn't already roll near-guaranteed
           colonization -- a fresh colony is fragile and often dies fast,
           and an over-eager boost turns that into a self-sustaining chain
           of founding/collapsing that can churn through the settlement
           table far faster than is interesting. */
        int deficit_pct = (int)((word)(baseline - alive) * 100 / baseline) / 2;
        chance += deficit_pct;
        if (chance > 100) chance = 100;
    }
    return chance;
}

static void cascade_refugees(World *w, Settlement *fallen, int depth)
{
    Settlement *neighbors[4];
    int count, i;

    if (depth >= MAX_CASCADE_DEPTH) return;

    count = nearby_alive(w, fallen, NEIGHBOR_RADIUS, neighbors, 4);
    for (i = 0; i < count; i++) {
        if (apply_refugee_strain(neighbors[i], &w->rng)) {
            neighbors[i]->alive = 0;
            cascade_refugees(w, neighbors[i], depth + 1);
        }
    }

    if (rng_range(&w->rng, 100) < (word)colonize_chance(w)) {
        try_found_colony(w, fallen, fallen->owner, fallen->x, fallen->y);
    }
}

static void apply_event_with_cascade(World *w, Settlement *s, EventType type, int depth)
{
    EventResult result;

    if (!s->alive) return;

    event_apply(s, type, &w->rng, &result);

    if (result.spawned_faction) {
        spawn_faction(w, s);
    }

    if (result.collapsed) {
        s->alive = 0;
        cascade_refugees(w, s, depth);
    }
}

void world_force_event(World *w, byte id, EventType type)
{
    Settlement *s = world_get_settlement(w, id);
    if (!s) return;
    apply_event_with_cascade(w, s, type, 0);
}

/* One Brownian step of the event-chance "weather," shared by every
   settlement this tick (not re-rolled per settlement) -- a gentle drift
   toward calmer or harsher, clamped to [EVENT_CHANCE_MIN, EVENT_CHANCE_MAX]. */
static void step_event_weather(World *w)
{
    int step = (int)rng_range(&w->rng, (EVENT_CHANCE_STEP * 2) + 1) - EVENT_CHANCE_STEP;
    int next = (int)w->event_chance_pct + step;

    if (next < EVENT_CHANCE_MIN) next = EVENT_CHANCE_MIN;
    if (next > EVENT_CHANCE_MAX) next = EVENT_CHANCE_MAX;
    w->event_chance_pct = (byte)next;
}

/* See RESETTLE_RUIN_CHANCE_PCT in world.h. Picks one random slot out of
   the whole table (dead or alive) and, if it's a corpse, resettles it in
   place -- reusing its own coordinates (already known-good land) rather
   than searching, and reusing its own slot rather than allocating a new
   one, so this never grows settlement_count. No-op if nothing has ever
   existed here (settlement_count == 0). */
static void step_ambient_resettlement(World *w)
{
    word idx;
    Settlement *slot;

    if (w->settlement_count == 0) return;
    if (rng_range(&w->rng, 100) >= RESETTLE_RUIN_CHANCE_PCT) return;

    idx = rng_range(&w->rng, w->settlement_count);
    slot = &w->settlements[idx];
    if (slot->alive) return;

    settlement_init(slot, (byte)idx, slot->x, slot->y, slot->owner, 1, 1, 1, 1, 1);
}

void world_tick(World *w)
{
    word i;
    word settlement_count_at_start = w->settlement_count; /* spawns mid-tick don't get a turn yet */

    step_event_weather(w);
    step_ambient_resettlement(w);

    for (i = 0; i < settlement_count_at_start; i++) {
        Settlement *s = &w->settlements[i];
        Settlement *neighbors[4];
        int count;

        if (!s->alive) continue;

        /* Neighbor cultural influence: an occasional, small nudge toward
           the nearest alive neighbor's dominant focus -- skipped once this
           settlement's own dominant focus is already extreme, so a region
           doesn't inevitably flatten to one focus at 255 with the rest at
           0. */
        count = nearby_alive(w, s, NEIGHBOR_RADIUS, neighbors, 4);
        if (count > 0 &&
            s->culture[settlement_dominant_focus(s)] < CULTURE_INFLUENCE_EXTREME_CEILING &&
            rng_range(&w->rng, 100) < CULTURE_INFLUENCE_CHANCE_PCT) {
            CultureFocus dominant = settlement_dominant_focus(neighbors[0]);
            settlement_shift_culture(s, dominant, CULTURE_INFLUENCE_STRENGTH);
        }

        /* Natural recovery, then a random event roll -- decline has a
           counterweight instead of being a one-way ratchet. */
        settlement_recover(s, &w->rng);

        if (rng_range(&w->rng, 100) < w->event_chance_pct) {
            EventType type = (EventType)rng_range(&w->rng, EVENT_COUNT);
            apply_event_with_cascade(w, s, type, 0);
        }
    }

    w->tick++;
}
