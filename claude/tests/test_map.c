#include "test.h"
#include "../engine/map.h"

#define SAMPLE_MAP "tests/sample.map"

void run_map_tests(void)
{
    Map m;
    word obj_count, i;
    byte found_settlement = 0;
    MapObject first_settlement;

    CHECK(map_load(&m, SAMPLE_MAP) == 0);
    CHECK(map_load(&m, "tests/does-not-exist.map") == -1);

    obj_count = map_object_count(&m);
    CHECK(obj_count > 0);
    CHECK(obj_count <= MAP_MAX_OBJECTS);
    CHECK(map_count_settlement_objects(&m) > 0);

    for (i = 0; i < obj_count; i++) {
        MapObject obj;
        map_get_object(&m, i, &obj);
        if (obj.type == OBJ_SETTLEMENT && !found_settlement) {
            found_settlement = 1;
            first_settlement = obj;
        }
    }
    CHECK(found_settlement);

    if (found_settlement) {
        byte terrain = map_get_terrain_at(&m, first_settlement.x, first_settlement.y);
        CHECK(terrain <= TERRAIN_SWAMP);
        CHECK(map_has_object_at(&m, first_settlement.x, first_settlement.y));

        /* A generous radius should find at least the other settlements,
           if the sample world has more than one. */
        if (map_count_settlement_objects(&m) > 1) {
            word out[64];
            word found = map_find_nearby_settlements(&m, first_settlement.x, first_settlement.y,
                                                       255, out, 64);
            CHECK(found > 0);
        }
    }

    /* corners are in range and don't crash/overflow */
    CHECK(map_get_terrain_at(&m, 0, 0) <= TERRAIN_SWAMP);
    CHECK(map_get_terrain_at(&m, 255, 255) <= TERRAIN_SWAMP);

    /* an empty map is all water, no objects */
    {
        Map empty;
        map_init_empty(&empty);
        CHECK(map_object_count(&empty) == 0);
        CHECK(map_get_terrain_at(&empty, 5, 5) == TERRAIN_WATER);
        CHECK(!map_has_object_at(&empty, 5, 5));
    }
}
