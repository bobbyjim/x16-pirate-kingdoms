#ifndef _MAP_H_
#define _MAP_H_

#define MAP_VISIBLE_ROWS    8
#define MAP_VISIBLE_COLS    8


typedef struct
{
    int x : 8;
    int y : 8;
} MapLocation;

void map_init();
void map_show(MapLocation *loc);

#endif
