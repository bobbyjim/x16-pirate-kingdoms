#include "test.h"
#include "../engine/storage.h"

#define SAMPLE_MAP "tests/sample.map"

void run_storage_tests(void)
{
    World w;
    WorldStorage storage;
    const Settlement *s;
    WorldTileInfo tile;
    byte ids[16];
    word found;
    word active;
    word disrupted;

    CHECK(world_load(&w, SAMPLE_MAP, 11) == 0);
    world_storage_bind_static(&w, &storage);

    CHECK(storage_settlement_count(&storage) == world_settlement_count(&w));
    CHECK(storage_alive_settlement_count(&storage) == world_alive_settlement_count(&w));

    s = storage_get_settlement(&storage, 0);
    CHECK(s != NULL);
    if (!s) return;

    CHECK(storage_get_tile_info(&storage, s->x, s->y, &tile) == 0);
    CHECK(tile.object.type == OBJ_SETTLEMENT);
    CHECK(storage_find_settlement_at(&storage, s->x, s->y) == s);

    found = storage_find_settlements_in_rect(&storage, 100, 86, 16, 16, ids, 16);
    CHECK(found > 0);
    CHECK(ids[0] == 0);

    CHECK(storage_trade_link_count(&storage) == world_trade_link_count(&w));
    CHECK(storage_get_trade_link(&storage, 0) != NULL);
    storage_trade_link_status_counts(&storage, &active, &disrupted);
    CHECK(active <= storage_trade_link_count(&storage));
    CHECK(disrupted <= storage_trade_link_count(&storage));
}