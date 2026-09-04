#ifndef _STORAGE_H_
#define _STORAGE_H_

#include "world.h"

/* Read-only storage surface for front ends and future gameplay code that
   must not assume host-style flat memory. The host build binds these calls
   to a plain World; the X16 build can later bind the same surface to a
   banked implementation. Mutation APIs can be layered on once the access
   patterns settle down. */

typedef struct {
    const void *ctx;
    int (*get_tile_info)(const void *ctx, byte x, byte y, WorldTileInfo *out);
} MapStorage;

typedef struct {
    const void *ctx;
    word (*count)(const void *ctx);
    word (*alive_count)(const void *ctx);
    const Settlement *(*get_by_id)(const void *ctx, byte id);
    const Settlement *(*find_at)(const void *ctx, byte x, byte y);
    word (*find_in_rect)(const void *ctx, byte x, byte y,
                         byte width, byte height,
                         byte *out_ids, word max_out);
} SettlementStorage;

typedef struct {
    const void *ctx;
    word (*count)(const void *ctx);
    const TradeLink *(*get_by_id)(const void *ctx, word id);
    void (*status_counts)(const void *ctx, word *active_out, word *disrupted_out);
} TradeLinkStorage;

typedef struct {
    MapStorage map;
    SettlementStorage settlements;
    TradeLinkStorage trade_links;
} WorldStorage;

void world_storage_bind_static(const World *w, WorldStorage *out);

int storage_get_tile_info(const WorldStorage *storage, byte x, byte y, WorldTileInfo *out);
word storage_settlement_count(const WorldStorage *storage);
word storage_alive_settlement_count(const WorldStorage *storage);
const Settlement *storage_get_settlement(const WorldStorage *storage, byte id);
const Settlement *storage_find_settlement_at(const WorldStorage *storage, byte x, byte y);
word storage_find_settlements_in_rect(const WorldStorage *storage, byte x, byte y,
                                      byte width, byte height,
                                      byte *out_ids, word max_out);
word storage_trade_link_count(const WorldStorage *storage);
const TradeLink *storage_get_trade_link(const WorldStorage *storage, word id);
void storage_trade_link_status_counts(const WorldStorage *storage,
                                      word *active_out, word *disrupted_out);

#endif