#ifndef _EVENTS_H_
#define _EVENTS_H_

#include "common.h"
#include "rng.h"
#include "settlement.h"

/* Cascading events per BUSINESS-LOGIC.md: each tests a specific structure
   characteristic (mitigated by a cultural focus) and damages every
   structure that contributes to it, rather than eroding an abstract stat
   -- degradation is expressed as structure damage, and enough of it is
   loss (settlement_damage_slot() clears a slot once its condition hits 0).
   Refugee-cascade propagation across neighboring settlements lives in
   world.c, since it needs the settlement table -- this module only knows
   how to apply one event to one settlement. */

typedef enum {
    EVENT_DROUGHT = 0,      /* tests Industry (reserve potential), mitigated by Growth */
    EVENT_PLAGUE,           /* tests Population, mitigated by Security */
    EVENT_STORM,            /* tests Commerce (exposed logistics), mitigated by Growth */
    EVENT_PIRATES,          /* tests Defense, mitigated by Security */
    EVENT_MARKET_CRASH,     /* tests Commerce, mitigated by Merchant (TRA) */
    EVENT_CIVIL_WAR,        /* tests Defense, mitigated by Security */
    EVENT_COUNT
} EventType;

const char *event_name(EventType type);
int event_from_name(const char *name); /* -1 if unrecognized */

typedef struct {
    byte collapsed;         /* every structure destroyed */
    byte spawned_faction;   /* civil war defense collapsed -> should split */
} EventResult;

/* Applies `type` to `s` in place: rolls severity based on the tested
   characteristic and its mitigating cultural focus (lower values -> harsher
   erosion), damages every structure contributing to that characteristic,
   and records the outcome in `result`. */
void event_apply(Settlement *s, EventType type, Rng *rng, EventResult *result);

#endif
