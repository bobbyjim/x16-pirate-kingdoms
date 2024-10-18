
#include <stdio.h>

#include <6502.h>
#include <cbm.h>
#include <conio.h>
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
#include "menus.h"
#include "ship.h"


#define  PLAYER_SPRITE                1

#define	LOAD_TO_MAIN_RAM	           0
#define	LOAD_TO_VERA	              2  

#define  SHIP_ADDR_START              0x4000
//#define  SHIP_ADDR_DRAKKAR            0x4000
//#define  SHIP_ADDR_DROMON             0x4400
//#define  SHIP_ADDR_GENOESE            0x4800
//#define  SHIP_ADDR_PENTEKONTOR        0x4c00
//#define  SHIP_ADDR_SAMBUK             0x5000
//#define  SHIP_ADDR_TARTANE            0x5400
//#define  SHIP_ADDR_TRIREME            0x5800
//#define  SHIP_ADDR_XEBEC              0x5c00

struct regs r;

/*
char* title[] = {
"        ***** **                                           ",
"     ******  ****     *                        *           ",
"    **   *  *  ***   ***                      **           ",
"   *    *  *    ***   *                       **           ",
"       *  *      **      ***  ****          ********       ",
"      ** **      ** ***   **** **** * **** ******** ***    ",
"      ** **      **  ***   **   **** * ***  * **   * ***   ",
"    **** **      *    **   **       *   ****  **  *   ***  ",
"   * *** **     *     **   **      **    **   ** **    *** ",
"      ** *******      **   **      **    **   ** ********  ",
"      ** ******       **   **      **    **   ** *******   ",
"      ** **           **   **      **    **   ** **        ",
"      ** **           **   ***     **    **   ** ****    * ",
"      ** **           *** * ***     ***** **   ** *******  ",
" **   ** **            ***           ***   **      *****   ",
"***   *  *  ",
" ***    *   ",
"  ******    ",
"    ***     ",
"            ",
"            ",
"     *****                                    **  ",
"  ******             *                         ** ",
" **   *  *    *     ***                        ** ",
"*    *  *   *** *    *                         ** ",
"    *  *     ***                               **                          ***     ",
"   ** **    * *    ***  ***  **     ***     ** **    ***  *** *** ***     * *** * ",
"   ** **   *        ***  ******* * * *** * ******** *  *** *** *** **  * **  ***  ",
"   ** *****          **   **  *** *   *** **  **** **   *   **  *** *** ****      ",
"   ** ** ***         **   **   * **    ** **   **  **   *   **   *   *    ***     ",
"   ** **   ***       **   **   * **    ** **   **  **   *   **   *   *      ***    ",
"   *  **    **       **   **   * **    ** **   **  **   *   **   *   *        ***  ",
"      *      **      **   **   * **    ** **   **  **   *   **   *   *   ****  **  ",
"  ****        **     **   **   * **    ** **   **   ****    **   *   *  * **** *   ",
" *  *****      **  * ** * ***  ** *******  ****      ***    ***  **  **    ****    ",
"*    ***        ***   **   **  ***  ** **   **               ***  **  **           ",
"*                                       **   ",
" **                                ***   **  ",
"                                 ******  *   ",
"                                *    ****    "
};

void splash()
{
   byte i;

   bgcolor(COLOR_BLACK);
   clrscr();
   textcolor(COLOR_RED);
   for(i=0; i<40; ++i)
   {
      cputsxy(0,i+1,title[i]);
   }
   textcolor(COLOR_ORANGE);
   chlinexy(0,46,80);
   cputsxy(28,52,"press <return> to begin");
   cgetc();
}
*/

void loadMapToBankedRAM()
{
   setBank(MAP_RAM_BANK_START);

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
   loadVera("ships-32x32.bin", SHIP_ADDR_START);
   loadVera("people-32x32.bin", PEOPLE_ADDR_START);
   loadVera("terrain-64x64.bin", LAND_ADDR_START);
}

SpriteDefinition sprdef;
Position         pos;
int              dx, dy, x, y;
unsigned char    shipIndex;
ShipData*        ship;

//unsigned getShipAddr(unsigned char index)
//{
//   switch((unsigned)index)
//   {
//      case 1: return SHIP_ADDR_DRAKKAR;
//      case 2: return SHIP_ADDR_DROMON;
//      case 3: return SHIP_ADDR_GENOESE;
//      case 4: return SHIP_ADDR_PENTEKONTOR;
//      case 5: return SHIP_ADDR_SAMBUK;
//      case 6: return SHIP_ADDR_TARTANE;
//      case 7: return SHIP_ADDR_TRIREME;
//      default: return SHIP_ADDR_XEBEC;
//   }
//}

void initSprite()
{
   sprdef.mode            = SPRITE_MODE_8BPP;
   sprdef.block           = ship->address; // (shipIndex); // SHIP_ADDR_SAMBUK; // SHIP_ADDR_TARTANE;
   sprdef.collision_mask  = 0x0000;
   sprdef.layer           = SPRITE_LAYER_0;
   sprdef.dimensions      = SPRITE_32_BY_32;
   sprdef.palette_offset  = 0;
   sprdef.x               = SPRITE_X_SCALE(312);
   sprdef.y               = SPRITE_Y_SCALE(200);

   pos.x = sprdef.x;
   pos.y = sprdef.y;

   sprite_define(PLAYER_SPRITE, &sprdef);
}

void move()
{
   if (kbhit())
      switch(cgetc())
      {
         case 0x91: // up
         case 'i':
         case 'w': if (dy > -ship->speed) dy -= ship->acceleration; break;

         case 0x11: // down
         case 'k':
         case 's': if (dy <  ship->speed) dy += ship->acceleration; break;

         case 0x9d: // left
         case 'j':
         case 'a': if (dx > -ship->speed) dx -= ship->acceleration; break;

         case 0x1d: // right
         case 'l':
         case 'd': if (dx <  ship->speed) dx += ship->acceleration; break;

         case 'm': // hey map!
            vera_sprites_enable(0);
            vera_sprites_enable(1);
            map_region();
            map_frame_draw();
            menus_show();
            break;

         case 133: // f1 
            if (map_has_land_north())
               cputsxy(1,20,"n");
            else
               cputsxy(1,20," ");
      
            if (map_has_land_east())
               cputsxy(3,20,"e");
            else
               cputsxy(3,20," ");
      
            if (map_has_land_west())
               cputsxy(4,20,"w");
            else 
               cputsxy(4,20," ");
      
            if (map_has_land_south())
               cputsxy(2,20,"s");
            else  
               cputsxy(2,20," ");
            break;
            
         case 137: // f2
         case 134: // f3
         case 138: // f4
         case 135: // f5
         case 139: // f6
         case 136: // f7
         case 140: // f8
         case 16:  // f9
         case 21:  // f10
         case 22:  // f11
         case 23:  // f12
            break;
      }

   if (dy < 0) map_north(-dy);
   if (dy > 0) map_south(dy);
   if (dx < 0) 
   {
      map_west(-dx);
      sprite_horiz_flip(PLAYER_SPRITE);
   }
   
   if (dx > 0) 
   {
      map_east(dx);
      sprite_horiz_unflip(PLAYER_SPRITE);
   }

   map_calculate();
}

void setPETFont()
{
   // set PET font
   struct regs fontregs;
   fontregs.a = 4; // PET-like
   fontregs.pc = 0xff62;
   _sys(&fontregs);
}

void main()
{
   setPETFont();
   //cbm_k_bsout(CH_FONT_UPPER);
   //splash();
   _randomize();

   bgcolor(COLOR_BLACK); 
   clrscr();
   loadMapToBankedRAM();
   loadSpriteDataToVERA();

   shipIndex = rand() % 8;
   ship = getShipData( shipIndex );

   vera_sprites_enable(1); // cx16.h 
   initSprite();

   map_init();
   map_frame_draw();
   menus_show();
   gotoxy(20,52);
   cprintf("%-11s %2d/%1d %2d %3d/%-3d %2d/%-2d",
      ship->name,
      ship->speed,
      ship->acceleration,
      ship->hull,
      ship->people_capacity,
      ship->goods_capacity,
      ship->ballista_capacity,
      ship->greek_fire_capability
   );

   while(1)
   {
      updateDate();
      move();
   }
}
