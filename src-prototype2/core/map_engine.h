#ifndef _MAP_ENGINE_H_
#define _MAP_ENGINE_H_

#include "common.h"

#define  MAP_WIDTH                       256
#define  MAP_HEIGHT                      256

// Pure map engine - no display dependencies

typedef struct
{
    int x : 8;
    int y : 8;
    int xoffset;
    int yoffset;
} MapLocation;

typedef struct {
    byte x;
    byte y;
    byte size;
} Settlement;

// Terrain types
#define TERRAIN_WATER      0
#define TERRAIN_GRASS      1
#define TERRAIN_FOREST     2
#define TERRAIN_HILLS      3
#define TERRAIN_MOUNTAINS  4
#define TERRAIN_DESERT     5
#define TERRAIN_SWAMP      6

// Map format constants
#define TERRAIN_MASK       0x0F
#define FLAG_OBJECT        0x80

// Map state
void map_engine_init();
MapLocation* map_engine_get_location();
void map_engine_set_position(byte x, byte y);
void map_engine_set_offset(int xoff, int yoff);

// Movement (updates position, returns 1 if map tile changed)
byte map_engine_move_north(byte v);
byte map_engine_move_south(byte v);
byte map_engine_move_east(byte v);
byte map_engine_move_west(byte v);

// Terrain queries
byte map_engine_get_terrain_at(byte x, byte y);
byte map_engine_has_object_at(byte x, byte y);
byte map_engine_has_land_north();
byte map_engine_has_land_south();
byte map_engine_has_land_east();
byte map_engine_has_land_west();

// Settlement management
byte map_engine_get_settlement_count();
Settlement* map_engine_get_settlement(byte index);
byte map_engine_has_settlement_nearby(byte x, byte y, byte radius);
byte map_engine_get_settlement_size(byte x, byte y);
void map_engine_place_settlement_at(byte x, byte y, byte size);
void map_engine_get_center_tile(byte *x, byte *y);

#endif
