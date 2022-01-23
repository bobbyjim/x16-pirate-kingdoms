#ifndef _MAP_H_
#define _MAP_H_

#define MAP_VISIBLE_ROWS    8
#define MAP_VISIBLE_COLS    8


typedef struct
{
    int x : 8;
    int y : 8;
    int xoffset;
    int yoffset;
} MapLocation;

void map_init();
void map_south(unsigned char v);
void map_north(unsigned char v);
void map_east(unsigned char v);
void map_west(unsigned char v);
void map_calculate();
void map_show();

#endif
