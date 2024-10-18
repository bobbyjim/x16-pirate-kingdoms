#ifndef _MAP_H_
#define _MAP_H_

#define  MAP_VISIBLE_ROWS                7
#define  MAP_VISIBLE_COLS                6

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
void map_south(unsigned char v);
void map_north(unsigned char v);
void map_east(unsigned char v);
void map_west(unsigned char v);
void map_calculate();
void map_region();

unsigned char map_has_land_north();
unsigned char map_has_land_south();
unsigned char map_has_land_east();
unsigned char map_has_land_west();

#endif
