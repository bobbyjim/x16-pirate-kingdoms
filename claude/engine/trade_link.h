#ifndef _TRADE_LINK_H_
#define _TRADE_LINK_H_

#include "common.h"

/* Two canonical inter-settlement route families from BUSINESS-LOGIC.md. */
typedef enum {
    TRADE_LINK_CARAVAN  = 0,
    TRADE_LINK_FLEET    = 1,
    TRADE_LINK_PIRATE   = 2
} TradeLinkType;

/* Route lifecycle flags; multiple can be set at once. */
#define TRADE_LINK_ACTIVE     0x01
#define TRADE_LINK_DISRUPTED  0x02
#define TRADE_LINK_BLOCKED    0x04
#define TRADE_LINK_RECOVERING 0x08

/* Travel direction along the (canonical, never-swapped) endpoint pair:
   clear = outbound, unit moving from_settlement_id -> to_settlement_id;
   set   = returning, unit moving to_settlement_id -> from_settlement_id.
   `progress` counts up to path_cost regardless of direction; on arrival the
   unit dwells (cooldown_or_recovery) then this bit flips and progress resets. */
#define TRADE_LINK_RETURNING  0x10

#define TRADE_LINK_EVENT_NONE 0xFF

/*
   Fixed-size 16-byte link record (compact and deterministic):
   0 link_id
   1, 2 location (unit's current x,y; derived/cached for rendering)
   3 type
   4 from_settlement_id  (canonical endpoint, never swapped)
   5 to_settlement_id    (canonical endpoint, never swapped)
   6 status_flags        (includes TRADE_LINK_RETURNING travel direction)
   7 health
   8 throughput
   9 risk
  10 path_cost
  11 range_or_distance
  12 owner_or_controller
  13 last_event_tag
  14 cooldown_or_recovery (also the dwell timer at each endpoint)
  15 progress            (distance travelled this leg, 0..path_cost)
*/
typedef struct {
    byte link_id;
    byte x, y;
    byte type;
    byte from_settlement_id;
    byte to_settlement_id;
    byte status_flags;
    byte health;
    byte throughput;
    byte risk;
    byte path_cost;
    byte range_or_distance;
    byte owner_or_controller;
    byte last_event_tag;
    byte cooldown_or_recovery;
    byte progress;
} TradeLink;

#endif