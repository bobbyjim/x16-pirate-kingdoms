
#include <peekpoke.h>
#include <cx16.h>
#include <conio.h>

#include "sprite.h"
#include "memory.h"

// x and y are 15 bit uint's so we can toy with them just a bit.
#define     SPRITE_XH(x)             ((x>>5) & 0xff)
#define     SPRITE_XL(x)             (x>>13)
#define     SPRITE_YH(y)             ((y>>5) & 0xff)
#define     SPRITE_YL(y)             (y>>13)

/*
 *  Copies 1K of SPRITE_RAM_BANK into VERA starting at 1FC00. 
 */
void sprite_refresh()
{
   unsigned char* sourceAddress = ((unsigned char *)0xa000);
   //
   //  Point to our source bank
   //
   RAM_BANK = SPRITE_RAM_BANK;

   //
   //  Set Port 0 to point to sprite register
   //
   VERA.control = 0; // port 0
   VERA.address = 0xfc00;
   VERA.address_hi = VERA_INC_1 + 1; // from cx16.h

   //
   //  Set up R0, R1, and R2 for the MEMORY_COPY kernal routine.
   //
	asm("stz   $02");   
	asm("lda  #$a0");  
	asm("sta   $03"); 	// R0 = #$a000
	asm("lda  #$23"); 
	asm("sta   $04");   
	asm("lda  #$9f");    
	asm("sta   $05"); 	// R1 = #$9f23
	asm("stz   $06"); 
	asm("lda  #$04"); 
	asm("sta   $07"); 	// R2 = #$400 = 1K

   //
   //  Call tiny assembly program in SPRITE_RAM_BANK.
   //  It only has two jobs, in order:
   //   1. wait for vblank (WAI)
   //   2. jmp MEMORY_COPY.
   //
   //asm("jsr $bff8");

   // old C:
   //while(sourceAddress < ((unsigned char *)0xa400))
   //   VERA.data0 = *sourceAddress++;
}

void sprite_define(uint8_t spritenum, SpriteDefinition *sprdef)
{
   //
   //  Set Port 0 to point to sprite register
   //
   VERA.control = 0; // port 0
   VERA.address = SPRITE_REGISTERS(spritenum);
   VERA.address_hi = VERA_INC_1 + 1; // from cx16.h

   VERA.data0 = SPRITE_BLOCK_LO(sprdef->block); // lower 8 bits here
   VERA.data0 = sprdef->mode + SPRITE_BLOCK_HI(sprdef->block);  // upper 4 bits in the lower nybble here
   VERA.data0 = SPRITE_XH(sprdef->x);
   VERA.data0 = SPRITE_XL(sprdef->x);
   VERA.data0 = SPRITE_YH(sprdef->y);
   VERA.data0 = SPRITE_YL(sprdef->y);
   VERA.data0 = sprdef->layer;     // leave collision mask and flips alone for now
   VERA.data0 = sprdef->dimensions + sprdef->palette_offset;
}

void sprite_define_in_bank(uint8_t spritenum, SpriteDefinition *sprdef)
{
   unsigned char *address = ((unsigned char *)(0xa000 + spritenum * 8));
   RAM_BANK = SPRITE_RAM_BANK;

   address[0] = SPRITE_BLOCK_LO(sprdef->block); // lower 8 bits here
   address[1] = sprdef->mode + SPRITE_BLOCK_HI(sprdef->block);  // upper 4 bits in the lower nybble here
   address[2] = SPRITE_XH(sprdef->x);
   address[3] = SPRITE_XL(sprdef->x);
   address[4] = SPRITE_YH(sprdef->y);
   address[5] = SPRITE_YL(sprdef->y);
   address[6] = sprdef->layer;     // leave collision mask and flips alone for now
   address[7] = sprdef->dimensions + sprdef->palette_offset;
}

void sprite_pos(uint8_t spritenum, Position* pos)
{
   //
   //  Set Port 0 to point to sprite x,y registers
   //
   VERA.control = 0; // port 0
   VERA.address = SPRITE_REGISTERS(spritenum) + 2; // x
   VERA.address_hi = VERA_INC_1 + 1;

   VERA.data0 = SPRITE_XH(pos->x);
   VERA.data0 = SPRITE_XL(pos->x);
   VERA.data0 = SPRITE_YH(pos->y);
   VERA.data0 = SPRITE_YL(pos->y);
   VERA.data0 ^= pos->flip;
}

void sprite_horiz_flip(uint8_t spritenum)
{
   //
   //  Set Port 0 to point to sprite x,y registers
   //
   VERA.control = 0; // port 0
   VERA.address = SPRITE_REGISTERS(spritenum) + 6; // flip
   VERA.address_hi = 1;
   //VERA.data0 ^= (flip & 1);
   VERA.data0 |= 1;
}

void sprite_horiz_unflip(uint8_t spritenum)
{
   //
   //  Set Port 0 to point to sprite x,y registers
   //
   VERA.control = 0; // port 0
   VERA.address = SPRITE_REGISTERS(spritenum) + 6; // flip
   VERA.address_hi = 1;
   if (VERA.data0 & 1) VERA.data0--;
}

void sprite_changeBlock(uint8_t spritenum, SpriteDefinition *sprdef)
{
   //
   //  Set Port 0 to point to sprite register
   //
   VERA.control = 0; // port 0
   VERA.address = SPRITE_REGISTERS(spritenum);
   VERA.address_hi = VERA_INC_1 + 1; // from cx16.h

   VERA.data0 = SPRITE_BLOCK_LO(sprdef->block); // lower 8 bits here
   VERA.data0 = sprdef->mode + SPRITE_BLOCK_HI(sprdef->block);  // upper 4 bits in the lower nybble here
}
