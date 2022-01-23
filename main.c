
#include <stdio.h>

#include <6502.h>
#include <cbm.h>
#include <conio.h>
#include <peekpoke.h>
#include <cx16.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

#include "common.h"
#include "sprite.h"
#include "map.h"
#include "calendar.h"
#include "memory.h"
#include "song.h"

#define	LOAD_TO_MAIN_RAM	           0
#define	LOAD_TO_VERA	              2  

#define  SHIP_ADDR_PENTEKONTOR        0x4000
#define  SHIP_ADDR_DRAKKAR            0x4400
#define  SHIP_ADDR_TRIREME            0x4800
#define  SHIP_ADDR_DROMON             0x4c00
#define  SHIP_ADDR_TARTANE            0x5000
#define  SHIP_ADDR_XEBEC              0x5400
#define  SHIP_ADDR_GENOESE            0x5800

#define  PEOPLE_ADDR_CAMP             0x6000
#define  PEOPLE_ADDR_VILLAGE          0x6400
#define  PEOPLE_ADDR_PUEBLO           0x6800
#define  PEOPLE_ADDR_AZTEC            0x6c00
#define  PEOPLE_ADDR_INCA             0x7000

#define  LAND_ADDR_OCEAN              0x8000
//#define  LAND_ADDR_COAST_EW           0x9000
//#define  LAND_ADDR_COAST_NS           0x9200
//#define  LAND_ADDR_COAST_CORNER       0x9400
#define  LAND_ADDR_DESERT             0x9000
#define  LAND_ADDR_SAVANNAH           0xa000
#define  LAND_ADDR_FOREST             0xb000
#define  LAND_ADDR_HILLS              0xc000
// something is happening at 0xd000
#define  LAND_ADDR_MOUNTAIN           0xe000

struct regs r;

uint16_t ships[] = {
   SHIP_ADDR_PENTEKONTOR, SHIP_ADDR_DRAKKAR, SHIP_ADDR_TRIREME, SHIP_ADDR_DROMON,
   SHIP_ADDR_TARTANE, SHIP_ADDR_XEBEC, SHIP_ADDR_GENOESE
};



void loadMapToBankedRAM()
{
   RAM_BANK = MAP_RAM_BANK_START;  // macro in cx16.h sets the destination bank

   cbm_k_setnam("map.bin");
   cbm_k_setlfs(0,8,0);
   cbm_k_load(LOAD_TO_MAIN_RAM, 0xa000);
}

void loadVera(char *fname, unsigned int address)
{
   cbm_k_setnam(fname);
   cbm_k_setlfs(0,8,0);
   cbm_k_load(LOAD_TO_VERA, address);
}

void loadSpriteDataToVERA()
{
   // ships at 0x4000
   loadVera("ships/pentekontor-32x32.bin", SHIP_ADDR_PENTEKONTOR);
   loadVera("ships/drakkar-32x32.bin",     SHIP_ADDR_DRAKKAR);
   loadVera("ships/trireme-32x32.bin",     SHIP_ADDR_TRIREME);
   loadVera("ships/dromon-32x32.bin",      SHIP_ADDR_DROMON);
   loadVera("ships/tartane-32x32.bin",     SHIP_ADDR_TARTANE);
   loadVera("ships/xebec-32x32.bin",       SHIP_ADDR_XEBEC);
   loadVera("ships/genoese-32x32.bin",     SHIP_ADDR_GENOESE);

   // settlements at 0x6000
   loadVera("people/camp-32x32-bw.bin",     PEOPLE_ADDR_CAMP);
   loadVera("people/village-32x32-bw.bin",  PEOPLE_ADDR_VILLAGE);
   loadVera("people/pueblo-32x32-bw.bin",   PEOPLE_ADDR_PUEBLO);
   loadVera("people/aztec-32x32-bw.bin",    PEOPLE_ADDR_AZTEC);
   loadVera("people/inca-32x32-bw.bin",     PEOPLE_ADDR_INCA);

   // terrain at 0x8000
   loadVera("terrain/ocean.bin",             LAND_ADDR_OCEAN);
   loadVera("terrain/island-desert-x.bin",   LAND_ADDR_DESERT);
   loadVera("terrain/island-savannah-x.bin", LAND_ADDR_SAVANNAH);
   loadVera("terrain/island-forest-x.bin",   LAND_ADDR_FOREST);
   loadVera("terrain/island-hills-x.bin",    LAND_ADDR_HILLS);
   loadVera("terrain/island-mountain-x.bin", LAND_ADDR_MOUNTAIN);
}

SpriteDefinition sprdef;
Position         pos;
int              dx, dy, x, y;

void initSprite()
{
   sprdef.mode            = SPRITE_MODE_8BPP;
   sprdef.block           = SHIP_ADDR_TARTANE;
   sprdef.collision_mask  = 0x0000;
   sprdef.layer           = SPRITE_LAYER_0;
   sprdef.dimensions      = SPRITE_32_BY_32;
   sprdef.palette_offset  = 0;
   sprdef.x               = SPRITE_X_SCALE(312);
   sprdef.y               = SPRITE_Y_SCALE(200);

   pos.x = sprdef.x;
   pos.y = sprdef.y;

   sprite_define(1, &sprdef);
}

byte ship_max_velocity = 12;
byte ship_acceleration = 3;
byte wind_vector       = 0;

void checkWind()
{
   if ((rand() % 100) == 0)
      wind_vector = rand() % 256;
}

void move()
{
   if (kbhit())
      switch(cgetc())
      {
         case 0x91: // up
         case 'i':
         case 'w': if (dy > -ship_max_velocity) dy -= ship_acceleration; break;

         case 0x11: // down
         case 'k':
         case 's': if (dy <  ship_max_velocity) dy += ship_acceleration; break;

         case 0x9d: // left
         case 'j':
         case 'a': if (dx > -ship_max_velocity) dx -= ship_acceleration; break;

         case 0x1d: // right
         case 'l':
         case 'd': if (dx <  ship_max_velocity) dx += ship_acceleration; break;
         case 'x':
            if (dx > 0) --dx;
            else if (dx < 0) ++dx;
            if (dy > 0) --dy;
            else if (dy < 0) ++dy;
            break;
      }

   if (dy < 0) map_north(-dy);
   if (dy > 0) map_south(dy);
   if (dx < 0) map_west(-dx);
   if (dx > 0) map_east(dx);

   //
   //  North-South wind is bits 0 and 1 (0x03).
   //  East-West wind is bits 2 and 3 (0x0c).
   //
   //    00 : north or east
   //    01 : no
   //    10 : no
   //    11 : south or west
   //
   if ((wind_vector & 0x03) == 0) map_north(1);
   if ((wind_vector & 0x03) == 3) map_south(1);
   if ((wind_vector & 0x0c) == 0) map_west(1);
   if ((wind_vector & 0x0c) == 12) map_east(1);

   map_calculate();
   map_show();
}

void main()
{
   unsigned char done;

   cbm_k_bsout(CH_FONT_UPPER);

   cprintf("press <return> to begin");
   done = cgetc();
   _randomize();

   done = 0;

   bgcolor(0); // lets background layer in
   clrscr();
   loadMapToBankedRAM();
   loadSpriteDataToVERA();
 
   vera_sprites_enable(1); // cx16.h 
   initSprite();
   map_init();
         
   while(!done)
   {
      cputsxy(19,50,theDate());
      move();
      checkWind();
   }
}
