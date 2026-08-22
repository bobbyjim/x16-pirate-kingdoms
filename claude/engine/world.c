#include <stddef.h>
#include "world.h"

static void apply_event_with_cascade(World *w, Settlement *s, EventType type, int depth);
static void disrupt_links_for_settlement(World *w, const Settlement *s, EventType type);

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

static byte chebyshev_distance(byte ax, byte ay, byte bx, byte by)
{
    byte dx = (ax > bx) ? (byte)(ax - bx) : (byte)(bx - ax);
    byte dy = (ay > by) ? (byte)(ay - by) : (byte)(by - ay);
    return (dx > dy) ? dx : dy;
}

/* True if any recorded link (in either direction, any lifecycle state)
   already connects `a` and `b` -- keeps formation from stacking duplicate
   routes on the same pair. */
static byte settlements_linked(const World *w, byte a, byte b)
{
    word i;
    for (i = 0; i < w->trade_link_count; i++) {
        const TradeLink *l = &w->trade_links[i];
        if ((l->from_settlement_id == a && l->to_settlement_id == b) ||
            (l->from_settlement_id == b && l->to_settlement_id == a)) return 1;
    }
    return 0;
}

/* Swap-pop removal, matching the settlement table's own reuse pattern --
   keeps link_id in sync with its new index. */
static void remove_trade_link(World *w, word idx)
{
    word last = w->trade_link_count - 1;
    if (idx != last) {
        w->trade_links[idx] = w->trade_links[last];
        w->trade_links[idx].link_id = (byte)idx;
    }
    w->trade_link_count--;
}

/* Tries to link `a` to one nearby, trade-compatible settlement it isn't
   already linked to -- "sufficient trade compatibility and reach" per
   BUSINESS-LOGIC.md is read as: both ends have at least some Commerce
   built, and are within TRADE_LINK_REACH tiles. A Fleet link needs port
   access (a Dock) at both ends; otherwise it's a Caravan. Used both to
   bootstrap connectivity at world_load() and as the per-tick formation
   roll in world_tick(). */
static void try_link_settlement(World *w, Settlement *a)
{
    Settlement *neighbors[4];
    int count, i;

    if (!a->alive || settlement_wealth_potential(a) == 0) return;
    if (w->trade_link_count >= MAX_TRADE_LINKS) return;

    count = nearby_alive(w, a, TRADE_LINK_REACH, neighbors, 4);
    for (i = 0; i < count; i++) {
        Settlement *b = neighbors[i];
        byte dist, type;

        if (settlement_wealth_potential(b) == 0) continue;
        if (settlements_linked(w, a->id, b->id)) continue;

        dist = chebyshev_distance(a->x, a->y, b->x, b->y);
        type = (settlement_has_structure(a, STRUCT_DOCK) && settlement_has_structure(b, STRUCT_DOCK))
               ? TRADE_LINK_FLEET : TRADE_LINK_CARAVAN;
        world_create_trade_link(w, type, a->id, b->id, a->owner, dist, dist);
        return; /* one new link per candidate per call */
    }
}

/* One formation roll per tick, on one random settlement -- deliberately
   not an O(settlements^2) scan every tick (see world.h). */
static void step_trade_link_formation(World *w)
{
    word idx;

    if (w->settlement_count == 0) return;
    if (rng_range(&w->rng, 100) >= TRADE_LINK_FORM_CHANCE_PCT) return;

    idx = rng_range(&w->rng, w->settlement_count);
    try_link_settlement(w, &w->settlements[idx]);
}

int world_load(World *w, const char *map_path, unsigned long seed)
{
    word i, obj_count;

    rng_seed(&w->rng, seed);
    w->tick = 0;
    w->settlement_count = 0;
    w->trade_link_count = 0;
    w->event_chance_pct = (EVENT_CHANCE_MIN + EVENT_CHANCE_MAX) / 2;

    if (map_load(&w->map, map_path) != 0) return -1;

    obj_count = map_object_count(&w->map);
    for (i = 0; i < obj_count && w->settlement_count < MAX_SETTLEMENTS; i++) {
        MapObject obj;
        byte size, capacity, extra, k;
        Settlement *s;

        map_get_object(&w->map, i, &obj);
        if (obj.type != OBJ_SETTLEMENT) continue;

        /* Object size byte ranges roughly 20-139 per WORLD-CREATION.md;
           scale into slot capacity (1..MAX_STRUCTURE_SLOTS) and how many
           of those slots start built. */
        size = obj.data[0];
        capacity = (byte)(1 + (word)size * (MAX_STRUCTURE_SLOTS - 1) / 139);

        s = &w->settlements[w->settlement_count];
        settlement_init(s, (byte)w->settlement_count, obj.x, obj.y, /*owner=*/0, capacity);
        settlement_build(s, STRUCT_TOWNHALL); /* every settlement starts with a civic anchor */

        extra = (byte)((word)capacity * size / 139);
        for (k = 0; k < extra && k + 1 < capacity; k++) {
            settlement_build(s, (StructureType)rng_range(&w->rng, STRUCT_TYPE_COUNT));
        }

        w->settlement_count++;
    }

    /* Bootstrap pass: without this, a freshly loaded world would sit
       disconnected for dozens of ticks waiting on step_trade_link_
       formation()'s slow per-tick roll. */
    for (i = 0; i < w->settlement_count; i++) {
        try_link_settlement(w, &w->settlements[i]);
    }

    w->initial_settlement_count = w->settlement_count;
    return 0;
}

void world_init_empty(World *w, unsigned long seed)
{
    rng_seed(&w->rng, seed);
    w->tick = 0;
    w->settlement_count = 0;
    w->trade_link_count = 0;
    w->event_chance_pct = (EVENT_CHANCE_MIN + EVENT_CHANCE_MAX) / 2;
    w->initial_settlement_count = 0; /* no colonize_chance() boost until set explicitly */
    map_init_empty(&w->map);
}

Settlement *world_get_settlement(World *w, byte id)
{
    if (id >= w->settlement_count) return NULL;
    return &w->settlements[id];
}

TradeLink *world_get_trade_link(World *w, word id)
{
    if (id >= w->trade_link_count) return NULL;
    return &w->trade_links[id];
}

TradeLink *world_create_trade_link(World *w,
                                   byte type,
                                   byte from_settlement_id,
                                   byte to_settlement_id,
                                   byte owner_or_controller,
                                   byte path_cost,
                                   byte range_or_distance)
{
    TradeLink *l;

    if (w->trade_link_count >= MAX_TRADE_LINKS) return NULL;
    if (from_settlement_id >= w->settlement_count) return NULL;
    if (to_settlement_id >= w->settlement_count) return NULL;
    if (from_settlement_id == to_settlement_id) return NULL;
    if (type != TRADE_LINK_CARAVAN && type != TRADE_LINK_FLEET) return NULL;

    l = &w->trade_links[w->trade_link_count];
    l->link_id = (byte)w->trade_link_count;
    l->type = type;
    l->from_settlement_id = from_settlement_id;
    l->to_settlement_id = to_settlement_id;
    l->status_flags = TRADE_LINK_ACTIVE;

    /* Neutral starting defaults; future simulation layers tune these. */
    l->health = 200;
    l->throughput = 64;
    l->risk = 32;
    l->path_cost = path_cost;
    l->range_or_distance = range_or_distance;
    l->owner_or_controller = owner_or_controller;
    l->last_event_tag = TRADE_LINK_EVENT_NONE;
    l->cooldown_or_recovery = 0;
    l->reserved_a = 0;
    l->reserved_b = 0;
    l->reserved_c = 0;

    w->trade_link_count++;
    return l;
}

void world_disable_trade_link(World *w, word id)
{
    TradeLink *l = world_get_trade_link(w, id);
    if (!l) return;

    l->status_flags &= (byte)~TRADE_LINK_ACTIVE;
    l->status_flags |= TRADE_LINK_DISRUPTED;
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

/* Splits `parent` into two on a civil-war defense collapse: the parent's
   surviving structures are left half-condition (stressed and depleted by
   the split), and a new faction settlement is created nearby, armed but
   institutionally bare -- a single, freshly-seized Fort at low condition,
   which settlement_culture_vector() reads as AGR (stressed) rather than
   SEC (stable). No-op if there's no room left in the settlement table
   (static array, no heap -- see world.h). */
static void spawn_faction(World *w, Settlement *parent)
{
    Settlement *child;
    byte i;
    int slot;

    child = allocate_settlement_slot(w, parent);
    if (!child) return;

    for (i = 0; i < MAX_STRUCTURE_SLOTS; i++) {
        if (parent->structures[i].type != STRUCT_EMPTY) {
            settlement_damage_slot(parent, i, -(int)(parent->structures[i].condition / 2));
        }
    }

    settlement_init(child, (byte)(child - w->settlements), parent->x, parent->y, parent->owner,
                     parent->capacity);
    slot = settlement_build(child, STRUCT_FORT);
    if (slot >= 0) settlement_damage_slot(child, (byte)slot, -(STAT_MAX - 3)); /* leave it stressed */
}

/* Straining an already-fragile neighbor can push it over the edge too;
   returns nonzero if the neighbor just collapsed as a result. */
static byte apply_refugee_strain(Settlement *neighbor, Rng *rng)
{
    int strain = 2 + (int)rng_range(rng, 3);
    settlement_damage_characteristic(neighbor, CHAR_INDUSTRY, strain);
    settlement_damage_characteristic(neighbor, CHAR_COMMERCE, strain / 2 > 0 ? strain / 2 : 1);
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

/* Founds the smallest possible viable settlement at (x,y): capacity 2, one
   civic anchor built. Shared by refugee colonization and ambient
   resettlement -- both want the same fragile-encampment starting point. */
static void found_encampment(Settlement *s, byte id, byte x, byte y, byte owner)
{
    settlement_init(s, id, x, y, owner, 2);
    settlement_build(s, STRUCT_TOWNHALL);
}

/* Searches near (near_x,near_y) for open, non-water land far enough from
   existing settlements and, if found, founds a brand-new fragile
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
            found_encampment(colony, (byte)(colony - w->settlements), x, y, owner);
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
    disrupt_links_for_settlement(w, s, type);

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

    found_encampment(slot, (byte)idx, slot->x, slot->y, slot->owner);
}

/* Per-tick maintenance for every recorded link: a dead endpoint blocks the
   link and accumulates TRADE_LINK_DEAD_ENDPOINT_TICKS before the link is
   removed outright (per BUSINESS-LOGIC.md's "removed if both endpoints are
   invalid or route viability is lost long-term"); otherwise health
   regenerates a little (faster with Town Hall/Monument "cohesion and
   trust" -- CHAR_CULTURE -- at either end, per the Link Capacity and
   Specialization section), throughput and lifecycle flags are recomputed
   from the resulting health, and that throughput feeds or starves both
   endpoints' Commerce-contributing structures. Reactive disruption from a
   specific event lives in disrupt_links_for_settlement() below, applied
   during the same tick's settlement loop -- its effect on throughput
   shows up here on the *next* tick, an acceptable lag for a route-health
   aggregate rather than a unit-level simulation. */
static void step_trade_link_maintenance(World *w)
{
    word i = 0;

    while (i < w->trade_link_count) {
        TradeLink *l = &w->trade_links[i];
        Settlement *from = world_get_settlement(w, l->from_settlement_id);
        Settlement *to = world_get_settlement(w, l->to_settlement_id);
        byte removed = 0;

        if (!from || !to || !from->alive || !to->alive) {
            l->status_flags &= (byte)~(TRADE_LINK_ACTIVE | TRADE_LINK_RECOVERING);
            l->status_flags |= (TRADE_LINK_BLOCKED | TRADE_LINK_DISRUPTED);
            if (l->cooldown_or_recovery < 255) l->cooldown_or_recovery++;
            if (l->cooldown_or_recovery >= TRADE_LINK_DEAD_ENDPOINT_TICKS) {
                remove_trade_link(w, i);
                removed = 1;
            }
        } else {
            word commerce_avg = (settlement_wealth_potential(from) + settlement_wealth_potential(to)) / 2;
            byte persistence_avg = (byte)((settlement_characteristic(from, CHAR_CULTURE) +
                                            settlement_characteristic(to, CHAR_CULTURE)) / 2);
            word base_capacity = commerce_avg * 2;
            int regen = TRADE_LINK_HEALTH_REGEN + persistence_avg / 16;
            int newhealth = (int)l->health + regen;

            if (base_capacity > 255) base_capacity = 255;
            if (newhealth > 255) newhealth = 255;
            l->health = (byte)newhealth;
            l->cooldown_or_recovery = 0; /* only accumulates while an endpoint is dead */

            l->status_flags &= (byte)~TRADE_LINK_BLOCKED;
            if (l->health >= TRADE_LINK_DISRUPTED_THRESHOLD) {
                l->status_flags &= (byte)~(TRADE_LINK_DISRUPTED | TRADE_LINK_RECOVERING);
                l->status_flags |= TRADE_LINK_ACTIVE;
            } else {
                l->status_flags &= (byte)~TRADE_LINK_ACTIVE;
                l->status_flags |= TRADE_LINK_RECOVERING;
            }

            l->throughput = (byte)(base_capacity * l->health / 255);
            l->risk = (byte)(255 - l->health); /* inverse of health, for UI/inspection */

            if (l->throughput < TRADE_LINK_LOW_THROUGHPUT) {
                settlement_damage_characteristic(from, CHAR_COMMERCE, TRADE_LINK_ENDPOINT_EFFECT);
                settlement_damage_characteristic(to, CHAR_COMMERCE, TRADE_LINK_ENDPOINT_EFFECT);
            } else if (l->throughput > TRADE_LINK_HIGH_THROUGHPUT) {
                settlement_repair_characteristic(from, CHAR_COMMERCE, TRADE_LINK_ENDPOINT_EFFECT);
                settlement_repair_characteristic(to, CHAR_COMMERCE, TRADE_LINK_ENDPOINT_EFFECT);
            }
        }

        if (!removed) i++;
    }
}

/* Reactive half of link disruption: called right after `s` suffers
   `type` (see apply_event_with_cascade), this hits every link touching
   `s` that the event is hazardous to -- Fleet links fear Pirates and
   Storm (open water, exposed docks); Caravan links fear Storm and Civil
   War (terrain friction, inland conflict) -- per BUSINESS-LOGIC.md's Link
   Consequence Model. A Fort-heavy settlement blunts the hit ("harder to
   disrupt" per the Link Capacity and Specialization section). */
static void disrupt_links_for_settlement(World *w, const Settlement *s, EventType type)
{
    word i;
    byte hits_fleet = (byte)(type == EVENT_PIRATES || type == EVENT_STORM);
    byte hits_caravan = (byte)(type == EVENT_STORM || type == EVENT_CIVIL_WAR);

    if (!hits_fleet && !hits_caravan) return;

    for (i = 0; i < w->trade_link_count; i++) {
        TradeLink *l = &w->trade_links[i];
        int damage;

        if (l->from_settlement_id != s->id && l->to_settlement_id != s->id) continue;
        if ((l->type == TRADE_LINK_FLEET && !hits_fleet) ||
            (l->type == TRADE_LINK_CARAVAN && !hits_caravan)) continue;

        damage = TRADE_LINK_HEALTH_DAMAGE - (int)(settlement_defense_posture(s) / 8);
        if (damage < 1) damage = 1;

        l->health = (l->health > (byte)damage) ? (byte)(l->health - damage) : 0;
        l->status_flags &= (byte)~(TRADE_LINK_ACTIVE | TRADE_LINK_RECOVERING);
        l->status_flags |= TRADE_LINK_DISRUPTED;
        l->last_event_tag = (byte)type;
    }
}

void world_tick(World *w)
{
    word i;
    word settlement_count_at_start = w->settlement_count; /* spawns mid-tick don't get a turn yet */

    step_event_weather(w);
    step_ambient_resettlement(w);
    step_trade_link_formation(w);
    step_trade_link_maintenance(w);

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
        if (count > 0 && rng_range(&w->rng, 100) < CULTURE_INFLUENCE_CHANCE_PCT) {
            byte culture[CULTURE_COUNT];
            settlement_culture_vector(s, culture);
            if (culture[settlement_dominant_focus(s)] < CULTURE_INFLUENCE_EXTREME_CEILING) {
                CultureFocus dominant = settlement_dominant_focus(neighbors[0]);
                settlement_nudge_focus(s, dominant);
            }
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
