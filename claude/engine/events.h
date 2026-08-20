#ifndef _EVENTS_H_
#define _EVENTS_H_

#include "common.h"
#include "rng.h"
#include "settlement.h"

/* Cascading events per BUSINESS-LOGIC.md: each tests a specific stat
   (mitigated by a cultural focus) and erodes state rather than causing
   instant destruction. Refugee-cascade propagation across neighboring
   settlements lives in world.c, since it needs the settlement table --
   this module only knows how to apply one event to one settlement. */

typedef enum {
    EVENT_DROUGHT = 0,      /* tests Reserves, mitigated by Growth */
    EVENT_PLAGUE,           /* tests Infrastructure, mitigated by Security */
    EVENT_STORM,            /* tests Infrastructure, mitigated by Growth */
    EVENT_PIRATES,          /* tests Defense, mitigated by Security */
    EVENT_MARKET_CRASH,     /* tests Reserves, mitigated by Merchant (TRA) */
    EVENT_CIVIL_WAR,        /* tests Defense, mitigated by Security */
    EVENT_COUNT
} EventType;

const char *event_name(EventType type);
int event_from_name(const char *name); /* -1 if unrecognized */

typedef struct {
    byte collapsed;         /* population reached 0 */
    byte spawned_faction;   /* civil war defense collapsed -> should split */
} EventResult;

/* Applies `type` to `s` in place: rolls severity based on the tested stat
   and its mitigating cultural focus (lower values -> harsher erosion),
   erodes the primary stat and a secondary stat/resource, and records the
   outcome in `result`. */
void event_apply(Settlement *s, EventType type, Rng *rng, EventResult *result);

#endif
