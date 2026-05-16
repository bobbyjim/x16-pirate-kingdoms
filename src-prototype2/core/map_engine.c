#include "map_engine.h"

/* Pure map engine logic - no display dependencies */
/* NOTE: For X16 build, map data is in banked RAM, not here */
/* For debug/test builds, we use a small stub */

#include <stdio.h>

static MapLocation map_location;
static Settlement settlements[256];
static byte settlement_count = 0;

/* Stub map data for testing only - real X16 uses banked RAM */
#ifndef __CC65__
static byte map_data[MAP_HEIGHT * MAP_WIDTH];
static byte map_objects[MAP_HEIGHT * MAP_WIDTH];
#endif

#define MAP_OFFSET_SCALE   8
#define HALF_TILE_WIDTH    32

void map_engine_init(void)
{
#ifndef __CC65__
    unsigned int i;
    FILE *fp;
#endif

    map_location.x = 0x4c;
    map_location.y = 0x64;
    map_location.xoffset = 0;
    map_location.yoffset = 0;
    settlement_count = 0;
    
#ifndef __CC65__
    /* Initialize stub map data (debug mode only) */
    for (i = 0; i < MAP_HEIGHT * MAP_WIDTH; i++) {
        map_data[i] = TERRAIN_WATER;
        map_objects[i] = 0;
    }
    
    /* Try to load actual map data from file */
    fp = fopen("assets/maps/archipelago.map", "rb");
    if (fp != NULL) {
        (void)fread(map_data, 1, MAP_HEIGHT * MAP_WIDTH, fp);
        fclose(fp);
        /* If we read data, it replaced the water - good! */
    }
#endif
}

MapLocation* map_engine_get_location(void)
{
    return &map_location;
}

void map_engine_set_position(byte x, byte y)
{
    map_location.x = x;
    map_location.y = y;
}

void map_engine_set_offset(int xoff, int yoff)
{
    map_location.xoffset = xoff;
    map_location.yoffset = yoff;
}

byte map_engine_move_south(byte v)
{
    map_location.yoffset -= v;
    if (map_location.yoffset < -HALF_TILE_WIDTH * MAP_OFFSET_SCALE)
    {
        map_location.yoffset = HALF_TILE_WIDTH * MAP_OFFSET_SCALE;
        ++map_location.y;
        return 1;
    }
    return 0;
}

byte map_engine_move_north(byte v)
{
    map_location.yoffset += v;
    if (map_location.yoffset > HALF_TILE_WIDTH * MAP_OFFSET_SCALE)
    {
        map_location.yoffset = -HALF_TILE_WIDTH * MAP_OFFSET_SCALE;
        --map_location.y;
        return 1;
    }
    return 0;
}

byte map_engine_move_east(byte v)
{
    map_location.xoffset -= v;
    if (map_location.xoffset < -HALF_TILE_WIDTH * MAP_OFFSET_SCALE)
    {
        map_location.xoffset = HALF_TILE_WIDTH * MAP_OFFSET_SCALE;
        ++map_location.x;
        return 1;
    }
    return 0;
}

byte map_engine_move_west(byte v)
{
    map_location.xoffset += v;
    if (map_location.xoffset > HALF_TILE_WIDTH * MAP_OFFSET_SCALE)
    {
        map_location.xoffset = -HALF_TILE_WIDTH * MAP_OFFSET_SCALE;
        --map_location.x;
        return 1;
    }
    return 0;
}

byte map_engine_get_terrain_at(byte x, byte y)
{
#ifndef __CC65__
    return map_data[y * MAP_WIDTH + x] & TERRAIN_MASK;
#else
    (void)x;
    (void)y;
    /* X16: Access banked RAM map data */
    /* TODO: Implement banked RAM access */
    return TERRAIN_WATER;
#endif
}

byte map_engine_has_object_at(byte x, byte y)
{
#ifndef __CC65__
    return map_objects[y * MAP_WIDTH + x] != 0;
#else
    (void)x;
    (void)y;
    /* X16: Access banked RAM map data */
    /* TODO: Implement banked RAM access */
    return 0;
#endif
}

byte map_engine_has_land_north(void)
{
    byte land;
    land = map_engine_get_terrain_at(map_location.x + 3, map_location.y - 2);
    return land != TERRAIN_WATER;
}

byte map_engine_has_land_south(void)
{
    byte land;
    land = map_engine_get_terrain_at(map_location.x + 3, map_location.y + 4);
    return land != TERRAIN_WATER;
}

byte map_engine_has_land_east(void)
{
    byte land;
    land = map_engine_get_terrain_at(map_location.x + 4, map_location.y + 3);
    return land != TERRAIN_WATER;
}

byte map_engine_has_land_west(void)
{
    byte land;
    land = map_engine_get_terrain_at(map_location.x - 2, map_location.y + 3);
    return land != TERRAIN_WATER;
}

byte map_engine_get_settlement_count(void)
{
    return settlement_count;
}

Settlement* map_engine_get_settlement(byte index)
{
    if (index >= settlement_count) return 0;
    return &settlements[index];
}

void map_engine_place_settlement_at(byte x, byte y, byte size)
{
    if (settlement_count >= 255) return;
    
    settlements[settlement_count].x = x;
    settlements[settlement_count].y = y;
    settlements[settlement_count].size = size;
    
#ifndef __CC65__
    /* Mark object in map (debug mode only) */
    map_data[y * MAP_WIDTH + x] |= FLAG_OBJECT;
    map_objects[y * MAP_WIDTH + x] = settlement_count + 1;
#else
    (void)x;
    (void)y;
    /* X16: Write to banked RAM */
    /* TODO: Implement banked RAM write */
#endif
    
    settlement_count++;
}

byte map_engine_has_settlement_nearby(byte x, byte y, byte radius)
{
    byte dx, dy;
    byte startX, endX, startY, endY;
    
    startX = (x > radius) ? (x - radius) : 0;
    endX = (x + radius < 255) ? (x + radius) : 255;
    startY = (y > radius) ? (y - radius) : 0;
    endY = (y + radius < 255) ? (y + radius) : 255;
    
    for (dy = startY; dy <= endY; ++dy) {
        for (dx = startX; dx <= endX; ++dx) {
            if (dx == x && dy == y) continue;
            if (map_engine_has_object_at(dx, dy)) return 1;
        }
    }
    return 0;
}

byte map_engine_get_settlement_size(byte x, byte y)
{
    byte i;
    
    for (i = 0; i < settlement_count; i++) {
        if (settlements[i].x == x && settlements[i].y == y) {
            return settlements[i].size;
        }
    }
    return 0;
}

void map_engine_get_center_tile(byte *x, byte *y)
{
    *x = map_location.x + 3;
    *y = map_location.y + 3;
}
