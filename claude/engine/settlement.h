#ifndef _SETTLEMENT_H_
#define _SETTLEMENT_H_

#include "common.h"
#include "rng.h"

/* Structure-first settlement model per BUSINESS-LOGIC.md: a settlement is a
   bounded set of structure slots. Population, wealth, reserves,
   infrastructure resilience, defense posture and the culture vector are
   all *derived* from structure composition and condition -- there is no
   direct stat storage anymore. */

#define STAT_MIN 0
#define STAT_MAX 15   /* structure condition range (4 bits) */

#define MAX_STRUCTURE_SLOTS 8

typedef enum {
    STRUCT_DOCK = 0,
    STRUCT_WAREHOUSE,
    STRUCT_FORT,
    STRUCT_TOWNHALL,
    STRUCT_MONUMENT,
    STRUCT_TYPE_COUNT,
    STRUCT_EMPTY = 0xF
} StructureType;

/* Intermediate simulation dimensions per BUSINESS-LOGIC.md's "Structure
   Characteristics" section -- not player-facing scores. Derived settlement
   outputs and the culture vector are both computed from these. */
typedef enum {
    CHAR_DEFENSE = 0,
    CHAR_COMMERCE,
    CHAR_INDUSTRY,
    CHAR_POPULATION,
    CHAR_CULTURE,
    CHAR_COUNT
} CharacteristicField;

/* One structure slot: type (STRUCT_EMPTY if unbuilt) and condition,
   clamped to 0-STAT_MAX by settlement_build()/settlement_damage_slot(). */
typedef struct {
    byte type;
    byte condition;
} StructureSlot;

/* Cultural vector focuses. Values always sum to CULTURE_VECTOR_SUM.
   Fully derived from characteristic composition -- see
   settlement_culture_vector(). */
typedef enum {
    CULTURE_TRA = 0,  /* Merchant */
    CULTURE_AGR = 1,  /* Aggression */
    CULTURE_GRO = 2,  /* Growth */
    CULTURE_SEC = 3,  /* Security */
    CULTURE_COUNT = 4
} CultureFocus;

#define CULTURE_VECTOR_SUM 255

typedef struct {        // 24 bytes
    byte id;
    byte x;
    byte y;
    byte owner;
    byte event_status;   /* current EventType in progress, or 0xFF = none */
    byte alive;
    byte capacity;
    byte spare;
    StructureSlot structures[MAX_STRUCTURE_SLOTS]; 
} Settlement;

#define EVENT_STATUS_NONE 0xFF

/* Starts a settlement with `capacity` usable slots (clamped to
   MAX_STRUCTURE_SLOTS) and nothing built -- callers add structures with
   settlement_build(). */
void settlement_init(Settlement *s, byte id, byte x, byte y, byte owner, byte capacity);

/* Builds `type` into the first empty slot within capacity, at full (15)
   starting condition. Returns the slot index, or -1 if no empty slot is
   available within capacity or `type` is invalid. */
int settlement_build(Settlement *s, StructureType type);

/* Adds `delta` (can be negative) to slot `index`'s condition, clamped to
   0-15. A slot whose condition reaches 0 is destroyed (cleared to
   STRUCT_EMPTY) -- degradation becomes loss, per BUSINESS-LOGIC.md. No-op
   on an out-of-range or already-empty slot. */
void settlement_damage_slot(Settlement *s, byte index, int delta);

/* Damages every occupied slot that contributes to `target`: full `amount`
   to a slot whose primary characteristic is `target`, half (min 1) to a
   slot whose secondary characteristic is `target`. `amount` must be
   positive. Shared by events.c (event effects) and world.c (refugee
   strain), so both erode the world the same way. */
void settlement_damage_characteristic(Settlement *s, CharacteristicField target, int amount);

/* The repair counterpart to settlement_damage_characteristic(): raises
   every occupied slot that contributes to `target` (full `amount` to a
   primary match, half/min-1 to a secondary match), clamped at STAT_MAX.
   `amount` must be positive. Used by world.c's trade-link maintenance to
   feed endpoints of a thriving route. */
void settlement_repair_characteristic(Settlement *s, CharacteristicField target, int amount);

/* True if `s` has at least one occupied slot of `type`, regardless of
   condition. Used to decide port access (a Fleet link needs a Dock at
   both ends) and similar structural gating. */
byte settlement_has_structure(const Settlement *s, StructureType type);

/* Given an empty slot within capacity, builds a structure whose recovery
   affinity (see settlement.c's STRUCTURE_GROWTH_FOCUS) favors `focus`; a
   no-op if there's no room. Used for inter-settlement cultural influence
   (see world.c's world_tick) as the structural replacement for directly
   nudging a stored culture vector. */
void settlement_nudge_focus(Settlement *s, CultureFocus focus);

byte settlement_structure_count(const Settlement *s); /* occupied slots */

/* Sum of every occupied slot's contribution to `c`: full condition for a
   slot whose primary characteristic is `c`, half (integer division) for a
   slot whose secondary characteristic is `c`. */
word settlement_characteristic(const Settlement *s, CharacteristicField c);

/* Average condition across occupied slots (0 if none) -- a composition-
   independent measure of physical upkeep, used as "Infrastructure
   resilience" in BUSINESS-LOGIC.md's derived outcomes. */
byte settlement_infrastructure_resilience(const Settlement *s);

/* Named derived readouts (BUSINESS-LOGIC.md's "Derived outcomes"), each a
   thin wrapper over settlement_characteristic(). */
word settlement_population_support(const Settlement *s); /* CHAR_POPULATION */
word settlement_wealth_potential(const Settlement *s);   /* CHAR_COMMERCE */
word settlement_reserve_potential(const Settlement *s);  /* CHAR_INDUSTRY */
word settlement_defense_posture(const Settlement *s);    /* CHAR_DEFENSE */

/* Fills out[CULTURE_COUNT] with the derived culture vector, normalized to
   sum to CULTURE_VECTOR_SUM. See settlement.c for the characteristic ->
   focus mapping and its rationale. */
void settlement_culture_vector(const Settlement *s, byte out[CULTURE_COUNT]);

/* Dominant (highest-value) culture focus. Ties resolve to the lowest enum
   value. */
CultureFocus settlement_dominant_focus(const Settlement *s);

/* A settlement collapses once every structure is gone -- nothing left to
   call a settlement. */
byte settlement_is_collapsed(const Settlement *s);

/* Natural recovery: existing structures regrow condition (faster under an
   affine culture focus), and -- given spare reserve potential and an empty
   slot within capacity -- a chance to invest in a brand-new structure
   biased toward the settlement's dominant focus. The counterweight to
   events.c's erosion, so decline is a real risk rather than a one-way
   ratchet. A dead settlement (no structures left) never recovers on its
   own. */
void settlement_recover(Settlement *s, Rng *rng);

#endif
