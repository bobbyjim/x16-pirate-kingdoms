#include "test.h"
#include "../core/map_engine.h"

// Test suite for map engine

int test_map_init()
{
    map_engine_init();
    
    MapLocation *loc = map_engine_get_location();
    ASSERT_EQ(loc->x, 0x4c);
    ASSERT_EQ(loc->y, 0x64);
    ASSERT_EQ(loc->xoffset, 0);
    ASSERT_EQ(loc->yoffset, 0);
    
    return 1;
}

int test_map_movement()
{
    map_engine_init();
    MapLocation *loc = map_engine_get_location();
    
    // Test north movement
    byte changed = map_engine_move_north(10);
    ASSERT_EQ(changed, 0);  // Should not change tile yet
    ASSERT_EQ(loc->yoffset, 10);
    
    // Move enough to change tile (need to exceed HALF_TILE_WIDTH * MAP_OFFSET_SCALE = 256)
    map_engine_move_north(250);  // Total offset now > 256
    ASSERT_EQ(loc->y, 0x63);  // Should have decremented by 1
    
    return 1;
}

int test_map_settlement()
{
    map_engine_init();
    
    // Place a settlement
    map_engine_place_settlement_at(100, 100, 50);
    
    ASSERT_EQ(map_engine_get_settlement_count(), 1);
    
    Settlement *s = map_engine_get_settlement(0);
    ASSERT_EQ(s->x, 100);
    ASSERT_EQ(s->y, 100);
    ASSERT_EQ(s->size, 50);
    
    // Check if it's marked on map
    ASSERT_NEQ(map_engine_has_object_at(100, 100), 0);
    
    return 1;
}

int test_map_settlement_nearby()
{
    map_engine_init();
    
    map_engine_place_settlement_at(50, 50, 80);
    
    // Should find nearby settlement
    ASSERT_NEQ(map_engine_has_settlement_nearby(52, 52, 5), 0);
    
    // Should not find distant settlement
    ASSERT_EQ(map_engine_has_settlement_nearby(100, 100, 5), 0);
    
    return 1;
}

int test_map_has_terrain()
{
    map_engine_init();
    
    byte terrain_count[7] = {0};  /* Count of each terrain type */
    int x, y;
    byte terrain;
    int sample_count = 0;
    
    printf("  Sampling map terrain...\n");
    
    /* Sample terrain across map */
    for (y = 0; y < 256; y += 16) {
        for (x = 0; x < 256; x += 16) {
            terrain = map_engine_get_terrain_at((byte)x, (byte)y);
            if (terrain < 7) {
                terrain_count[terrain]++;
            }
            sample_count++;
        }
    }
    
    printf("  Sampled %d points: Water=%d, Grass=%d, Forest=%d, Hills=%d, Mtn=%d, Desert=%d, Swamp=%d\n",
        sample_count, terrain_count[0], terrain_count[1], terrain_count[2], 
        terrain_count[3], terrain_count[4], terrain_count[5], terrain_count[6]);
    
    /* Should have some non-water terrain */
    int non_water = terrain_count[1] + terrain_count[2] + terrain_count[3] + 
                    terrain_count[4] + terrain_count[5] + terrain_count[6];
    ASSERT_NEQ(non_water, 0);  /* At least some non-water terrain */
    
    return 1;
}

// Main test runner
int main()
{
    int passed = 0, failed = 0;
    
    TEST_START();
    
    if (test_map_init()) {
        printf("✓ test_map_init passed\n");
        passed++;
    } else {
        failed++;
    }
    
    if (test_map_movement()) {
        printf("✓ test_map_movement passed\n");
        passed++;
    } else {
        failed++;
    }
    
    if (test_map_settlement()) {
        printf("✓ test_map_settlement passed\n");
        passed++;
    } else {
        failed++;
    }
    
    if (test_map_settlement_nearby()) {
        printf("✓ test_map_settlement_nearby passed\n");
        passed++;
    } else {
        failed++;
    }
    
    if (test_map_has_terrain()) {
        printf("✓ test_map_has_terrain passed\n");
        passed++;
    } else {
        printf("✗ test_map_has_terrain FAILED - map is all water\n");
        failed++;
    }
    
    printf("\n%d passed, %d failed\n", passed, failed);
    
    return (failed == 0) ? 0 : 1;
}
