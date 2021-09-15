
#include <stdio.h>

#include <6502.h>
#include <cbm.h>
#include <conio.h>
#include <peekpoke.h>
#include <cx16.h>
#include <time.h>

#include "sprite.h"
#include "map.h"

#define	TO_RAM	         0
#define	TO_VERA	         2

#define	SPRITE_SET_IMAGE	$FEF0
#define  SPRITE_MOVE		   $FEF3

struct regs r;

void loadMap()
{
   RAM_BANK = 10;  // the map goes here

   cbm_k_setnam("map.bin");
   cbm_k_setlfs(0,8,0);
   cbm_k_load(TO_RAM, 0xa000);
}

void loadVera(char *fname, unsigned int address)
{
   cbm_k_setnam(fname);
   cbm_k_setlfs(0,8,0);
   cbm_k_load(TO_VERA, address);
}

void initVera()
{
   // ships at 0x4000
   loadVera("ships/pentekontor-32x32.bin", 0x4000);
   loadVera("ships/drakkar-32x32.bin",     0x4400);
   loadVera("ships/trireme-32x32.bin",     0x4800);
   loadVera("ships/dromon-32x32.bin",      0x4c00);
   loadVera("ships/tartane-32x32.bin",     0x5000);
   loadVera("ships/xebec-32x32.bin",       0x5400);
   loadVera("ships/genoese-32x32.bin",     0x5800);

   // settlements at 0x6000
   loadVera("people/camp-32x32-bw.bin",     0x6000);
   loadVera("people/village-32x32-bw.bin",  0x6400);
   loadVera("people/pueblo-32x32-bw.bin",   0x6800);
   loadVera("people/aztec-32x32-bw.bin",    0x6c00);
   loadVera("people/inca-32x32-bw.bin",     0x7000);
   
   // terrain at 0x8000
   loadVera("terrain/ocean.bin",             0x8000);
   loadVera("terrain/island-desert-x.bin",   0x8400);
   loadVera("terrain/island-grass-x.bin",    0x8800);
   loadVera("terrain/island-savannah-x.bin", 0x8c00);
   loadVera("terrain/island-forest-x.bin",   0x9000);
   loadVera("terrain/island-hills-x.bin",    0x9400);
   loadVera("terrain/island-mountain-x.bin", 0x9800);
}

SpriteDefinition sprdef;
Position         pos;
SmallPosition    dxy;
MapLocation      map;

void initSprite()
{
   sprdef.mode            = SPRITE_MODE_8BPP;
   sprdef.block           = 0x5400;
   sprdef.collision_mask  = 0x0000;
   sprdef.layer           = SPRITE_LAYER_1;
   sprdef.dimensions      = SPRITE_32_BY_32;
   sprdef.palette_offset  = 0;
   sprdef.x               = 3200;
   sprdef.y               = 3200;

   pos.x = sprdef.x;
   pos.y = sprdef.y;

   vera_sprites_enable(1); // cx16.h
   sprite_define(1, &sprdef);
}

void moveSprite()
{
   if (kbhit())
      switch(cgetc())
      {
         case 'w': dxy.y -= 1; break;
         case 's': dxy.y += 1; break;
         case 'a': dxy.x -= 1; break;
         case 'd': dxy.x += 1; break;
      }

   if (dxy.x < -24) dxy.x = -24;
   if (dxy.x >  24) dxy.x =  24;
   if (dxy.y < -24) dxy.y = -24;
   if (dxy.y >  24) dxy.y =  24;

   pos.x += dxy.x;
   pos.y += dxy.y;
   sprite_pos(1, &pos);
}

void main()
{
   clock_t now = clock();
   unsigned char done = 0;

   map.x = 64;
   map.y = 64;

   cbm_k_bsout(CH_FONT_UPPER);

   loadMap();
   initVera();
 
   initSprite();

   while(!done)
   {
      while( clock() - now < 1 ); // tiny pause
      now  = clock();

      map_show(&map);
      moveSprite();
   }
}

