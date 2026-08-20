#ifndef _SETTLEMENT_H_
#define _SETTLEMENT_H_

#include "common.h"
#include "rng.h"

/* Settlement attributes per BUSINESS-LOGIC.md: five 4-bit (0-15) stats,
   bit-packed nibbles the way src-prototype1/map.h's MapLocation already
   demonstrates bitfields under cc65. */

#define STAT_MIN 0
#define STAT_MAX 15

typedef enum {
    STAT_POPULATION,
    STAT_WEALTH,
    STAT_RESERVES,
    STAT_INFRASTRUCTURE,
    STAT_DEFENSE,
    STAT_COUNT
} StatField;

/* Cultural vector focuses. Values always sum to CULTURE_VECTOR_SUM. */
typedef enum {
    CULTURE_TRA = 0,  /* Merchant */
    CULTURE_AGR = 1,  /* Aggression */
    CULTURE_GRO = 2,  /* Growth */
    CULTURE_SEC = 3,  /* Security */
    CULTURE_COUNT = 4
} CultureFocus;

#define CULTURE_VECTOR_SUM 255

typedef struct {
    byte id;
    byte x, y;
    byte owner;
    byte event_status;   /* current EventType in progress, or 0xFF = none */

    unsigned population     : 4;
    unsigned wealth         : 4;
    unsigned reserves       : 4;
    unsigned infrastructure : 4;
    unsigned defense        : 4;
    unsigned alive          : 1; 
    unsigned _pad1          : 3; /* keeps the bitfield group nibble-aligned */

    byte culture[CULTURE_COUNT]; /* TRA, AGR, GRO, SEC -- sums to 255 */
    byte free[4]; /* padding to keep the struct 16 bytes total, for cc65's sake */
} Settlement;

#define EVENT_STATUS_NONE 0xFF

void settlement_init(Settlement *s, byte id, byte x, byte y, byte owner,
                      byte population, byte wealth, byte reserves,
                      byte infrastructure, byte defense);

byte settlement_get_stat(const Settlement *s, StatField f);
void settlement_set_stat(Settlement *s, StatField f, byte value); /* clamped 0-15 */
void settlement_add_stat(Settlement *s, StatField f, int delta);  /* clamped 0-15 */

/* Dominant (highest-value) culture focus. Ties resolve to the lowest enum value. */
CultureFocus settlement_dominant_focus(const Settlement *s);

/* Shifts `focus` by `amount`, taking/giving the difference to the other
   three focuses proportionally so the sum always stays CULTURE_VECTOR_SUM.
   `amount` is clamped so `focus` itself stays within 0..255. */
void settlement_shift_culture(Settlement *s, CultureFocus focus, int amount);

/* A settlement collapses when population bottoms out. */
byte settlement_is_collapsed(const Settlement *s);

/* Natural recovery: the counterweight to events.c's erosion, so decline is
   a real risk rather than a one-way ratchet. Reserves trickle back up on
   their own (faster under a Growth focus); Wealth grows from trade
   (faster under a Merchant/TRA focus); Infrastructure and Defense only
   rebuild once there are Reserves to invest in them (Growth and Security
   focus respectively); Population grows once Wealth and Infrastructure
   both have a cushion. A dead settlement (population 0) never recovers on
   its own -- per BUSINESS-LOGIC.md, decline can be weathered but collapse
   is meant to be real. */
void settlement_recover(Settlement *s, Rng *rng);

#endif
