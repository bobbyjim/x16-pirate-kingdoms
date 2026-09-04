#include <stddef.h>
#include "storage.h"

static int static_get_tile_info(const void *ctx, byte x, byte y, WorldTileInfo *out)
{
    return world_get_tile_info((const World *)ctx, x, y, out);
}

static word static_settlement_count(const void *ctx)
{
    return world_settlement_count((const World *)ctx);
}

static word static_alive_settlement_count(const void *ctx)
{
    return world_alive_settlement_count((const World *)ctx);
}

static const Settlement *static_get_settlement(const void *ctx, byte id)
{
    return world_get_settlement_const((const World *)ctx, id);
}

static const Settlement *static_find_settlement_at(const void *ctx, byte x, byte y)
{
    return world_find_settlement_at((const World *)ctx, x, y);
}

static word static_find_settlements_in_rect(const void *ctx, byte x, byte y,
                                            byte width, byte height,
                                            byte *out_ids, word max_out)
{
    return world_find_settlements_in_rect((const World *)ctx, x, y, width, height, out_ids, max_out);
}

static word static_trade_link_count(const void *ctx)
{
    return world_trade_link_count((const World *)ctx);
}

static const TradeLink *static_get_trade_link(const void *ctx, word id)
{
    return world_get_trade_link_const((const World *)ctx, id);
}

static void static_trade_link_status_counts(const void *ctx, word *active_out, word *disrupted_out)
{
    world_trade_link_status_counts((const World *)ctx, active_out, disrupted_out);
}

void world_storage_bind_static(const World *w, WorldStorage *out)
{
    if (!out) return;

    out->map.ctx = w;
    out->map.get_tile_info = static_get_tile_info;

    out->settlements.ctx = w;
    out->settlements.count = static_settlement_count;
    out->settlements.alive_count = static_alive_settlement_count;
    out->settlements.get_by_id = static_get_settlement;
    out->settlements.find_at = static_find_settlement_at;
    out->settlements.find_in_rect = static_find_settlements_in_rect;

    out->trade_links.ctx = w;
    out->trade_links.count = static_trade_link_count;
    out->trade_links.get_by_id = static_get_trade_link;
    out->trade_links.status_counts = static_trade_link_status_counts;
}

int storage_get_tile_info(const WorldStorage *storage, byte x, byte y, WorldTileInfo *out)
{
    if (!storage || !storage->map.get_tile_info) return -1;
    return storage->map.get_tile_info(storage->map.ctx, x, y, out);
}

word storage_settlement_count(const WorldStorage *storage)
{
    if (!storage || !storage->settlements.count) return 0;
    return storage->settlements.count(storage->settlements.ctx);
}

word storage_alive_settlement_count(const WorldStorage *storage)
{
    if (!storage || !storage->settlements.alive_count) return 0;
    return storage->settlements.alive_count(storage->settlements.ctx);
}

const Settlement *storage_get_settlement(const WorldStorage *storage, byte id)
{
    if (!storage || !storage->settlements.get_by_id) return NULL;
    return storage->settlements.get_by_id(storage->settlements.ctx, id);
}

const Settlement *storage_find_settlement_at(const WorldStorage *storage, byte x, byte y)
{
    if (!storage || !storage->settlements.find_at) return NULL;
    return storage->settlements.find_at(storage->settlements.ctx, x, y);
}

word storage_find_settlements_in_rect(const WorldStorage *storage, byte x, byte y,
                                      byte width, byte height,
                                      byte *out_ids, word max_out)
{
    if (!storage || !storage->settlements.find_in_rect) return 0;
    return storage->settlements.find_in_rect(storage->settlements.ctx, x, y, width, height,
                                             out_ids, max_out);
}

word storage_trade_link_count(const WorldStorage *storage)
{
    if (!storage || !storage->trade_links.count) return 0;
    return storage->trade_links.count(storage->trade_links.ctx);
}

const TradeLink *storage_get_trade_link(const WorldStorage *storage, word id)
{
    if (!storage || !storage->trade_links.get_by_id) return NULL;
    return storage->trade_links.get_by_id(storage->trade_links.ctx, id);
}

void storage_trade_link_status_counts(const WorldStorage *storage,
                                      word *active_out, word *disrupted_out)
{
    if (!storage || !storage->trade_links.status_counts) {
        if (active_out) *active_out = 0;
        if (disrupted_out) *disrupted_out = 0;
        return;
    }

    storage->trade_links.status_counts(storage->trade_links.ctx, active_out, disrupted_out);
}