#ifndef _SPRITE_H_
#define _SPRITE_H_

#include <stdint.h>

#define		SPRITE_MODE_8BPP			128

#define		SPRITE_32_BY_32				(128 + 32)
#define		SPRITE_64_BY_64				(196 + 48)

#define		SPRITE_DISABLED				0
#define	    SPRITE_LAYER_BACKGROUND		(1 << 2)
#define		SPRITE_LAYER_0				(2 << 2)
#define		SPRITE_LAYER_1				(3 << 2)

//
//  A note on positions:
//  15 bits is an abuse, unless you use it fractionally.
//  Let's shift it by 5 bits before using it.
//

typedef struct {
	int8_t  mode;
	int16_t block; 
    int8_t  collision_mask;
    int8_t  layer;
	int8_t  dimensions;      
	int8_t  palette_offset;
	int     x : 15;
	int     y : 15;
} SpriteDefinition;

typedef struct {
	int x    : 15;
	int y    : 15;
	int flip : 1;
} Position;

typedef struct {
	int x;
	int y;
} SmallPosition;

void sprite_define(uint8_t spritenum, SpriteDefinition *sprdef);
void sprite_pos(uint8_t spritenum, Position* pos);

#endif

/*
POKE $9F29,($40 OR PEEK($9F29)) :REM ENABLE SPRITES
rem ship
s0 = $fc10
x = px + 164
y = py + 164
bk = 2
ls = int(rnd(1)*8) * 32
fl = 0
po = 0 :rem pallette offset

VPOKE $F, S0+0, %00000000+ls :REM LSB
VPOKE $F, S0+1, %10000000+bk :REM R...BBBB
VPOKE $F, S0+2, X AND 255    :REM X
VPOKE $F, S0+3, X  / 256     :REM X MSB
VPOKE $F, S0+4, Y AND 255    :REM Y
VPOKE $F, S0+5, Y  / 256     :REM Y MSB
VPOKE $F, S0+6, %00001100+FL :REM coll,ZZ,FLIP
VPOKE $F, S0+7, %10100000+PO :REM HHWW..PO
*/
