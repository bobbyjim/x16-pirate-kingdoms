#include "map.h"

void map_init()
{
}

void map_show(MapLocation *loc)
{
    int row,col;

    for(row=loc->x; row<loc->x + 8; ++row)
       for(col=loc->y; col<loc->y + 8; ++col)
       {
           // bank number = ?
           // memory location = ?
       }
}
