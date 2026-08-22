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

#define TRADE_LINK_EVENT_NONE 0xFF

/*
   Fixed-size 16-byte link record (compact and deterministic):
   0 link_id
   1 type
   2 from_settlement_id
   3 to_settlement_id
   4 status_flags
   5 health
   6 throughput
   7 risk
   8 path_cost
   9 range_or_distance
  10 owner_or_controller
  11 last_event_tag
  12 cooldown_or_recovery
  13 reserved_a
  14 reserved_b
  15 reserved_c
*/
typedef struct {
    byte link_id;
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
    byte reserved_a;
    byte reserved_b;
    byte reserved_c;
} TradeLink;

#endif