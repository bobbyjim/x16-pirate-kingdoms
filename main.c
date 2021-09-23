
#include <stdio.h>

#include <6502.h>
#include <cbm.h>
#include <conio.h>
#include <peekpoke.h>
#include <cx16.h>
#include <time.h>

#include "sprite.h"
#include "map.h"
#include "calendar.h"


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
#define  LAND_ADDR_DESERT             0x9000
#define  LAND_ADDR_GRASS              0xa000
#define  LAND_ADDR_SAVANNAH           0xb000
#define  LAND_ADDR_FOREST             0xc000
#define  LAND_ADDR_HILLS              0xd000
#define  LAND_ADDR_MOUNTAIN           0xe000

struct regs r;

uint16_t ships[] = {
   SHIP_ADDR_PENTEKONTOR, SHIP_ADDR_DRAKKAR, SHIP_ADDR_TRIREME, SHIP_ADDR_DROMON,
   SHIP_ADDR_TARTANE, SHIP_ADDR_XEBEC, SHIP_ADDR_GENOESE
};

void loadMapToBankedRAM()
{
   RAM_BANK = 10;  // macro in cx16.h sets the destination bank

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
   loadVera("terrain/island-grass-x.bin",    LAND_ADDR_GRASS);
   loadVera("terrain/island-savannah-x.bin", LAND_ADDR_SAVANNAH);
   loadVera("terrain/island-forest-x.bin",   LAND_ADDR_FOREST);
   loadVera("terrain/island-hills-x.bin",    LAND_ADDR_HILLS);
   loadVera("terrain/island-mountain-x.bin", LAND_ADDR_MOUNTAIN);
}

SpriteDefinition sprdef;
Position         pos, landpos;
int dx, dy;

void initSprite()
{
   sprdef.mode            = SPRITE_MODE_8BPP;
   sprdef.block           = SHIP_ADDR_GENOESE;
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

#define MAX_V     2

void move()
{
   if (kbhit())
      switch(cgetc())
      {
         case 'w': --dy; break;
         case 's': ++dy; break;
         case 'a': --dx; break;
         case 'd': ++dx; break;
      }

   if (dy >  MAX_V)  dy =  MAX_V;
   if (dy < -MAX_V)  dy = -MAX_V;
   if (dx >  MAX_V)  dx =  MAX_V;
   if (dx < -MAX_V)  dx = -MAX_V;

   if (dy < 0) map_north(-dy);
   if (dy > 0) map_south(dy);
   if (dx < 0) map_west(-dx);
   if (dx > 0) map_east(dx);
   map_show();
}

void main()
{
   clock_t now = clock();
   unsigned char done = 0;

   cbm_k_bsout(CH_FONT_UPPER);

   bgcolor(0);
   clrscr();
   loadMapToBankedRAM();
   loadSpriteDataToVERA();
 
   vera_sprites_enable(1); // cx16.h 
   initSprite();
   map_init();
   //initLand();

   while(!done)
   {
      cputsxy(20,50,theDate());
      nextHour();
      while( clock() - now < 1 ); // tiny pause
      now  = clock();
      move();
   }
}
