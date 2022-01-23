#include <conio.h>
#include <peekpoke.h>
#include <cx16.h>

#include "map.h"
#include "sprite.h"
#include "memory.h"

SpriteDefinition mapSprite;
MapLocation map;

#define         MAP_OFFSET_SCALE       8

void map_init()
{
   int i, j;

   map.x = 0x4c;
   map.y = 0x64;
   map.xoffset = 0;
   map.yoffset = 0;

   revers(1);
   for(i=1;i<7;++i)
   {
      cputsxy(13,i,"                                                        ");
      for (j=6; j<50; j += 6)
      {
          cputsxy(13,i+j,"      ");
          cputsxy(63,i+j,"      ");
      }
      cputsxy(13,i+50,"                                                        ");
   }

   revers(0);
}

void map_south(unsigned char v)
{
    map.yoffset -= v;
    if (map.yoffset < -24 * MAP_OFFSET_SCALE)
    {
        map.yoffset = 24 * MAP_OFFSET_SCALE;
        ++map.y;
    }
}

void map_north(unsigned char v)
{
    map.yoffset += v;
    if (map.yoffset > 24 * MAP_OFFSET_SCALE)
    {
        map.yoffset = -24 * MAP_OFFSET_SCALE;
        --map.y;
    }
}

void map_east(unsigned char v)
{
    map.xoffset -= v;
    if (map.xoffset < -24 * MAP_OFFSET_SCALE)
    {
        map.xoffset = 24 * MAP_OFFSET_SCALE;
        ++map.x;
    }
}

void map_west(unsigned char v)
{
    map.xoffset += v;
    if (map.xoffset > 24 * MAP_OFFSET_SCALE)
    {
        map.xoffset = -24 * MAP_OFFSET_SCALE;
        --map.x;
    }
}

#define         TERRAIN_OVERLAP       16

void map_calculate()
{
    int row,col; // the map is 256 x 256
    int y3, yy;
    unsigned char land;
    unsigned char settlement;
    unsigned char spriteNum = 2;

    gotoxy(19,7);
    cprintf("%2x,%2x", map.x, map.y);

    for(row=0; row<MAP_VISIBLE_ROWS; ++row)
    {
       y3 = (map.y + row)/32;
       RAM_BANK = MAP_RAM_BANK_START + y3;  // macro in cx16.h sets the destination bank
       yy = (map.y + row) % 32;

       for(col=0; col<MAP_VISIBLE_COLS; ++col)
       {
           land = PEEK(0xa000 + yy * 256 + col + map.x);
           settlement = (land >>4);
           land &= 15;
           
           mapSprite.mode       = SPRITE_MODE_8BPP;
           mapSprite.layer      = SPRITE_LAYER_0;
           mapSprite.dimensions = SPRITE_64_BY_64;
           mapSprite.x          = SPRITE_X_SCALE(map.xoffset/MAP_OFFSET_SCALE + 129 + col * (64-TERRAIN_OVERLAP));
           mapSprite.y          = SPRITE_Y_SCALE(map.yoffset/MAP_OFFSET_SCALE + 31  + row * (64-TERRAIN_OVERLAP));

           switch(land)
           {
               case 12:
               case 0: mapSprite.block = 0x8000; break;
               case 8:
               case 10:
               case 1: mapSprite.block = 0x9000; break;
               case 2: 
               case 3:
               case 4: mapSprite.block = 0xa000; break;
               case 5:
               case 6:
               case 7: mapSprite.block = 0xb000; break;
               case 11:
               case 9: mapSprite.block = 0xc000; break;
               case 13: mapSprite.block = 0xd000; break;
               default: mapSprite.block = 0xe000; break;
           }

           sprite_define(spriteNum, &mapSprite);
           ++spriteNum;

           mapSprite.dimensions = SPRITE_32_BY_32;

           switch(settlement)
           {
               case 0: 
               case 1:
               case 2: mapSprite.block = 0x6000; break;
               case 3:
               case 4:
               case 5: mapSprite.block = 0x6400; break;
               case 6:
               case 7:
               case 8: mapSprite.block = 0x6800; break;
               case 9:
               case 10:
               case 11: mapSprite.block = 0x6c00; break;
               case 12:
               case 13:
               case 14:
               case 15: mapSprite.block = 0x7000; break;
           }

           if (settlement > 0)
           {
              sprite_define(spriteNum, &mapSprite);
              ++spriteNum;
           }
       }
    }
}

void map_show()
{
   //sprite_refresh();
}