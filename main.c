
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


#define  PLAYER_SPRITE                1

#define	LOAD_TO_MAIN_RAM	           0
#define	LOAD_TO_VERA	              2  

#define  SHIP_ADDR_START              0x4000
#define  SHIP_ADDR_DRAKKAR            0x4000
#define  SHIP_ADDR_DROMON             0x4400
#define  SHIP_ADDR_GENOESE            0x4800
#define  SHIP_ADDR_PENTEKONTOR        0x4c00
#define  SHIP_ADDR_SAMBUK             0x5000
#define  SHIP_ADDR_TARTANE            0x5400
#define  SHIP_ADDR_TRIREME            0x5800
#define  SHIP_ADDR_XEBEC              0x5c00

struct regs r;

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

void initSprite()
{
   sprdef.mode            = SPRITE_MODE_8BPP;
   sprdef.block           = SHIP_ADDR_SAMBUK; // SHIP_ADDR_TARTANE;
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

byte ship_max_velocity = 12;
byte ship_acceleration = 3;

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

         case 'm': // hey map!
            vera_sprites_enable(0);
            map_region();
            vera_sprites_enable(1);
            map_frame_draw();
            menus_init();
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

void main()
{
   cbm_k_bsout(CH_FONT_UPPER);
   splash();
   _randomize();

   bgcolor(COLOR_BLACK); 
   clrscr();
   loadMapToBankedRAM();
   loadSpriteDataToVERA();
 
   vera_sprites_enable(1); // cx16.h 
   initSprite();

   map_init();
   map_frame_draw();
   menus_init();

   while(1)
   {
      cputsxy(20,52,theDate());
      move();
   }
}
