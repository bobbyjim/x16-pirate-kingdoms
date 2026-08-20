#ifndef _WORLD_H_
#define _WORLD_H_

#include "common.h"
#include "rng.h"
#include "map.h"
#include "settlement.h"
#include "events.h"

/* NOTE: World (and Map within it) is a flat, statically-sized struct with
   no dynamic allocation -- intentional, since the X16 has no real heap.
   sizeof(World) is far too large to live in one blob on real 6502 hardware
   (Map alone is ~139KB); on this host build it's just a CLI/test-owned
   static/stack value. When the X16 UI layer is built against this engine
   it will need to keep the map banked/streamed the way
   src-prototype1/map.c already does, rather than holding it flat. */
#define MAX_SETTLEMENTS   256   /* matches src-prototype1's settlement_cache size */
#define MAX_CASCADE_DEPTH 8     /* refugee-cascade recursion backstop */
#define NEIGHBOR_RADIUS   20    /* tiles (Chebyshev) for influence + cascades */

/* Neighbor cultural influence (world_tick): a small, and not-guaranteed,
   nudge toward the nearest neighbor's dominant focus -- not a deterministic
   pull every tick, or every settlement in a region ends up saturated at
   one focus (255) with the rest at 0 within a couple hundred ticks. Once a
   settlement's own dominant focus is already this committed, it's too set
   in its ways for outside influence to move it further. */
#define CULTURE_INFLUENCE_CHANCE_PCT      25
#define CULTURE_INFLUENCE_STRENGTH        1
#define CULTURE_INFLUENCE_EXTREME_CEILING 200
/* Per-settlement, per-tick chance of a random event isn't a fixed dial --
   it's a "weather system" that random-walks between two bounds, one step
   per world tick (shared by every settlement that tick, not rolled per
   settlement). That gives calmer and harsher eras over time without
   reacting to how the world is actually doing (a deliberate choice: an
   event chance that rises *because* settlements are thriving, or falls
   *because* they're struggling, starts to feel like a hand on the scale
   rather than weather). Bounds stay conservative for the same reason
   EVENT_CHANCE_PCT was kept low before: severity_roll()'s base severity
   (~5-9) can exceed a freshly founded colony's entire stat pool (every
   stat starts at 1), so settlements need real recovery time between hits
   even at the harsh end of the walk. */
#define EVENT_CHANCE_MIN   3
#define EVENT_CHANCE_MAX   7
#define EVENT_CHANCE_STEP  1    /* max drift per tick, toward either bound */

/* When a settlement collapses, refugees don't just strain a neighbor --
   per src-prototype1/README.md's "endless cycle" idea, some of the time
   they instead strike out and found a fresh encampment on nearby open
   land. It starts as small and fragile as a settlement can be (a "1 is an
   encampment" per BUSINESS-LOGIC.md), so this isn't a way to dodge decline,
   just the game's way of letting the story continue elsewhere.

   COLONIZE_CHANCE_PCT is the baseline roll, but a sparse world boosts it:
   the fewer settlements are alive relative to how many the world started
   with, the more likely refugees are to strike out rather than just
   scatter -- an empty niche gets colonized faster, the same as it would
   ecologically. This is deliberately not a global director judging how
   the world is "doing"; it's a mechanical population-pressure rule with
   no notion of good/bad, so it doesn't feel like a hand on the scale the
   way reacting to settlement *health* would. See colonize_chance() in
   world.c. */
#define COLONIZE_CHANCE_PCT    40
#define COLONIZE_SEARCH_RADIUS 15
#define COLONIZE_MIN_SPACING   5
#define COLONIZE_MAX_ATTEMPTS  20

/* All of the above only ever fires as a *reaction* to a specific
   settlement collapsing this tick -- world_tick() skips dead settlements
   entirely, so if every settlement in the table is dead, nothing is left
   alive to collapse and trigger it, and the world would sit frozen
   forever. Rather than special-casing "is anyone alive," a low flat
   per-tick chance instead picks one random slot (dead or alive) out of
   the whole table; if it happens to land on a corpse, that ruin gets
   resettled in place. This handles total extinction for free (there's
   always at least one slot once anything has ever existed), and as a
   side effect also lets ruins get reclaimed in an otherwise-thriving
   world -- fitting BUSINESS-LOGIC.md's ruins/legacy idea, and giving
   worse-off worlds (more corpses in the table) proportionally better odds
   without any explicit density math. */
#define RESETTLE_RUIN_CHANCE_PCT 2

typedef struct {
    Map map;
    Settlement settlements[MAX_SETTLEMENTS];
    word settlement_count;
    unsigned long tick;
    Rng rng;
    byte event_chance_pct; /* current position of the event-chance random walk */
    word initial_settlement_count; /* "carrying capacity" baseline for colonize_chance(); 0 = no boost */
} World;

/* Loads a .map file (see map.h) and seeds one Settlement per OBJ_SETTLEMENT
   object found in it, scaling the map's size byte into the 0-15 stat range. */
int  world_load(World *w, const char *map_path, unsigned long seed);
void world_init_empty(World *w, unsigned long seed);

Settlement *world_get_settlement(World *w, byte id); /* NULL if id out of range */

/* Advances the simulation by one tick: neighbor cultural influence, then a
   per-settlement chance of a random event (with weakness-driven severity
   and refugee cascades on collapse). */
void world_tick(World *w);

/* Forces `type` onto settlement `id` immediately, running the same
   cascade/spawn logic a natural roll would trigger. Used by the CLI and
   by tests that need a specific, non-random outcome. */
void world_force_event(World *w, byte id, EventType type);

#endif
