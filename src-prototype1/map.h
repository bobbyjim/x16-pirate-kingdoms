#ifndef _MAP_H_
#define _MAP_H_

#include "common.h"

#define  MAP_VISIBLE_ROWS                7
#define  MAP_VISIBLE_COLS                6

#define  MAP_WIDTH                       256
#define  MAP_HEIGHT                      256

#define  PEOPLE_ADDR_START            0x6000
#define  PEOPLE_ADDR_CAMP             0x6000
#define  PEOPLE_ADDR_CAMP_2           0x6200
#define  PEOPLE_ADDR_VILLAGE          0x6400
#define  PEOPLE_ADDR_VILLAGE_2        0x6600
#define  PEOPLE_ADDR_PUEBLO           0x6800
#define  PEOPLE_ADDR_PUEBLO_2         0x6a00
#define  PEOPLE_ADDR_AZTEC            0x6c00
#define  PEOPLE_ADDR_AZTEC_2          0x6e00
#define  PEOPLE_ADDR_INCA             0x7000

#define  LAND_ADDR_START              0x7400
#define  LAND_ADDR_OCEAN              0x7400
#define  LAND_ADDR_DESERT             0x8400
#define  LAND_ADDR_SAVANNAH           0x9400
#define  LAND_ADDR_FOREST             0xa400
#define  LAND_ADDR_HILLS              0xb400
#define  LAND_ADDR_MOUNTAIN           0xc400

typedef struct
{
    int x : 8;
    int y : 8;
    int xoffset;
    int yoffset;
} MapLocation;

void map_init();
void map_frame_draw();
void map_south(byte v);
void map_north(byte v);
void map_east(byte v);
void map_west(byte v);
void map_calculate();
void map_region(byte dimension);

// Settlement helpers
byte map_get_terrain_at(byte x, byte y);
byte map_has_object_at(byte x, byte y);
void map_place_settlement_at(byte x, byte y);
byte map_has_settlement_nearby(byte x, byte y, byte radius);
void map_get_center_tile(byte *x, byte *y);
byte map_get_settlement_size(byte x, byte y);

byte map_has_land_north();
byte map_has_land_south();
byte map_has_land_east();
byte map_has_land_west();

#endif
