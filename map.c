#include <conio.h>
#include <peekpoke.h>
#include <cx16.h>
#include <cbm.h>

#include "map.h"
#include "sprite.h"
#include "memory.h"
#include "common.h"

SpriteDefinition mapSprite;
MapLocation map;

#define         MAP_OFFSET_SCALE                8
#define         TERRAIN_OVERLAP                 0
#define         HALF_VISIBLE_TILE_WIDTH       (32 - TERRAIN_OVERLAP)   

void map_init()
{
   map.x = 0x4c;
   map.y = 0x64;

   map.xoffset = 0;
   map.yoffset = 0;
}

void map_frame_draw()
{
   byte i, j;
   textcolor(COLOR_GRAY1);
   revers(1);
   for(i=0;i<7;++i)
   {
      cputsxy(0,i,"                                                                                ");
      for (j=4; j<50; j += 6)
      {
          cputsxy(0, i+j,"                    ");
          cputsxy(60,i+j,"                    ");
      }
      cputsxy(0,i+53,"                                                                                ");
   }
   revers(0);
   textcolor(COLOR_WHITE);
}

void map_south(unsigned char v)
{
    map.yoffset -= v;
    if (map.yoffset < -HALF_VISIBLE_TILE_WIDTH * MAP_OFFSET_SCALE)
    {
        map.yoffset = HALF_VISIBLE_TILE_WIDTH * MAP_OFFSET_SCALE;
        ++map.y;
        gotoxy(20,7);
        cprintf("%2x,%2x", map.x, map.y);
    }
}

void map_north(unsigned char v)
{
    map.yoffset += v;
    if (map.yoffset > HALF_VISIBLE_TILE_WIDTH * MAP_OFFSET_SCALE)
    {
        map.yoffset = -HALF_VISIBLE_TILE_WIDTH * MAP_OFFSET_SCALE;
        --map.y;
        gotoxy(20,7);
        cprintf("%2x,%2x", map.x, map.y);
    }
}

void map_east(unsigned char v)
{
    map.xoffset -= v;
    if (map.xoffset < -HALF_VISIBLE_TILE_WIDTH * MAP_OFFSET_SCALE)
    {
        map.xoffset = HALF_VISIBLE_TILE_WIDTH * MAP_OFFSET_SCALE;
        ++map.x;
        gotoxy(20,7);
        cprintf("%2x,%2x", map.x, map.y);
    }
}

void map_west(unsigned char v)
{
    map.xoffset += v;
    if (map.xoffset > HALF_VISIBLE_TILE_WIDTH * MAP_OFFSET_SCALE)
    {
        map.xoffset = -HALF_VISIBLE_TILE_WIDTH * MAP_OFFSET_SCALE;
        --map.x;
        gotoxy(20,7);
        cprintf("%2x,%2x", map.x, map.y);
    }
}

unsigned char get_map(unsigned char x, unsigned char y)
{
    byte y3 = y/32;
    byte yy = y%32;
    RAM_BANK = MAP_RAM_BANK_START + y3;
    return PEEK(0xa000 + yy * 256 + x);
}

unsigned char map_has_land_north()
{
    byte land = get_map(map.x+3, map.y-2);
    land &= 15;
    return land;
}

unsigned char map_has_land_south()
{
    byte land = get_map(map.x+3, map.y+4);
    land &= 15;
    return land;
}

unsigned char map_has_land_east()
{
    byte land = get_map(map.x+4, map.y+3);
    land &= 15;
    return land;
}

unsigned char map_has_land_west()
{
    byte land = get_map(map.x-2, map.y+3);
    land &= 15;
    return land;
}

void map_calculate()
{
    byte row,col;
    byte land;
    byte settlement;
    byte terrainSprite = 2;
    byte settlementSprite = 50;  // settlements start at sprite 50
    byte y3, yy;
    
    for(row=0; row<MAP_VISIBLE_ROWS; ++row)
    {
       y3 = (map.y + row)/32;
       RAM_BANK = MAP_RAM_BANK_START + y3;
       yy = (map.y + row) % 32;
       
       for(col=0; col<MAP_VISIBLE_COLS; ++col)
       {
           land = PEEK(0xa000 + yy * 256 + col + map.x);
           settlement = (land >>4);
           land &= 15;
           
           // Draw terrain sprite (64x64)
           mapSprite.mode           = SPRITE_MODE_8BPP;
           mapSprite.layer          = SPRITE_LAYER_0;
           mapSprite.dimensions     = SPRITE_64_BY_64;
           mapSprite.palette_offset = 0;
           mapSprite.x              = SPRITE_X_SCALE(map.xoffset/MAP_OFFSET_SCALE + 128 + col * (64-TERRAIN_OVERLAP));
           mapSprite.y              = SPRITE_Y_SCALE(map.yoffset/MAP_OFFSET_SCALE + 24  + row * (64-TERRAIN_OVERLAP));

           switch(land)
           {
               case 8:
               case 10:
                    mapSprite.block = LAND_ADDR_DESERT; 
                    break;
               case 2: 
               case 3: 
               case 4: 
                    mapSprite.block = LAND_ADDR_SAVANNAH; 
                    break;
               case 5: 
               case 6: 
               case 7: 
                    mapSprite.block = LAND_ADDR_FOREST; 
                    break;
               case 11:
               case 9: 
                    mapSprite.block = LAND_ADDR_MOUNTAIN; 
                    break;
               case 12:
               case 0: 
               case 1:
               default: 
                    mapSprite.block = LAND_ADDR_OCEAN; 
                    break; // draw the sea
                    // mapSprite.layer = SPRITE_DISABLED; break;  // don't draw the sea
           }

           sprite_define(terrainSprite, &mapSprite);
           ++terrainSprite;

           // Draw settlement sprite (32x32) if present
           if (settlement > 0)
           {
               mapSprite.dimensions = SPRITE_32_BY_32;
               mapSprite.layer      = SPRITE_LAYER_1;  // above terrain
               
               switch(settlement)
               {
                   case 1:
                   case 2: mapSprite.block = PEOPLE_ADDR_CAMP; break;
                   case 3:
                   case 4:
                   case 5: mapSprite.block = PEOPLE_ADDR_VILLAGE; break;
                   case 6:
                   case 7:
                   case 8: mapSprite.block = PEOPLE_ADDR_PUEBLO; break;
                   case 9:
                   case 10:
                   case 11: mapSprite.block = PEOPLE_ADDR_AZTEC; break;
                   case 12:
                   case 13:
                   case 14:
                   case 15: mapSprite.block = PEOPLE_ADDR_INCA; break;
               }
               
               sprite_define(settlementSprite, &mapSprite);
               ++settlementSprite;
           }
       }
    }
}

char* landchar = "..123456789abcdef";
byte  landcolor[] = {

    COLOR_BLUE, COLOR_BLUE,
    COLOR_BROWN, COLOR_BROWN, COLOR_BROWN,
    COLOR_BROWN, COLOR_BROWN, COLOR_BROWN,
    COLOR_BROWN, COLOR_BROWN, COLOR_BROWN,
    COLOR_BROWN, COLOR_BROWN, 
    COLOR_BLUE, COLOR_BLUE,
    COLOR_BLUE
};

void map_region()
{
    byte row, r, col, c, land, bank, yy;
    clrscr();

    revers(1);
    
    for(row=0; row<58; ++row)
    {
        r = map.y + row - 25;
        bank = r/32;
        RAM_BANK = MAP_RAM_BANK_START + bank;
        yy = r % 32;

        for(col=0; col<58; ++col)
        {
           c = map.x + col - 25;
           land = PEEK(0xa000 + yy * 256 + c);
           land &= 15;

           textcolor(landcolor[land]);
           cputcxy(col+11,row+1,'.'); // landchar[land]);
        }
    }
    revers(0);

    textcolor(COLOR_YELLOW);
    cputcxy(39,29,'x');
    cgetc();
    clrscr();
}