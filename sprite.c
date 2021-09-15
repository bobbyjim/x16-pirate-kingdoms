
#include <peekpoke.h>
#include <cx16.h>

#include "sprite.h"
#include "vpoke.h"

/*
POKE $9F29,($40 OR PEEK($9F29)) :REM ENABLE SPRITES
S0 = $fc10
VPOKE $F, S0+0, %00000000+ls :REM LSB
VPOKE $F, S0+1, %10000000+bk :REM R...BBBB
VPOKE $F, S0+2, X AND 255    :REM X
VPOKE $F, S0+3, X   / 256    :REM X MSB
VPOKE $F, S0+4, Y AND 255    :REM Y
VPOKE $F, S0+5, Y   / 256    :REM Y MSB
VPOKE $F, S0+6, %00001100+FL :REM coll,ZZ,FLIP
VPOKE $F, S0+7, %10100000+PO :REM HHWW..PO
*/

#define 	SPRITE_REGISTERS(spritenum)	((spritenum << 3) + 0xfc00)

// sprite blocks are in 32 byte chunks.
#define		SPRITE_BLOCK(addr)			(addr >> 5)

// x and y are 15 bit uint's so we can toy with them just a bit.
#define     SPRITE_X(x)                (x>>5)
#define     SPRITE_Y(y)                (y>>5)

void sprite_define(uint8_t spritenum, SpriteDefinition *sprdef)
{
   int block   = SPRITE_BLOCK(sprdef->block); // blocksize is in 32 byte chunks

   //
   //  Set Port 0 to point to sprite register
   //
   VERA.control = 0; // port 0
   VERA.address = SPRITE_REGISTERS(spritenum);
   VERA.address_hi = VERA_INC_1 + 1;

   VERA.data0 = (block & 0xff);              // lower 8 bits here
   VERA.data0 = sprdef->mode + ((block >> 8) & 0x1f);  // upper 4 bits in the lower nybble here
   VERA.data0 = SPRITE_X(sprdef->x) & 0xff;
   VERA.data0 = SPRITE_X(sprdef->x) >> 8;
   VERA.data0 = SPRITE_Y(sprdef->y) & 0xff;
   VERA.data0 = SPRITE_Y(sprdef->y) >> 8;
   VERA.data0 = sprdef->layer;     // leave collision mask and flips alone for now
   VERA.data0 = sprdef->dimensions + sprdef->palette_offset;
}

void sprite_pos(uint8_t spritenum, Position* pos)
{
   //
   //  Set Port 0 to point to sprite x,y registers
   //
   VERA.control = 0; // port 0
   VERA.address = SPRITE_REGISTERS(spritenum) + 2; // x
   VERA.address_hi = VERA_INC_1 + 1;

   VERA.data0 = SPRITE_X(pos->x) & 0xff;
   VERA.data0 = SPRITE_X(pos->x) >> 8;
   VERA.data0 = SPRITE_Y(pos->y) & 0xff;
   VERA.data0 = SPRITE_Y(pos->y) >> 8;
}
