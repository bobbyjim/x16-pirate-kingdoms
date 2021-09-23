#ifndef _SPRITE_H_
#define _SPRITE_H_

#include <stdint.h>

#define		SPRITE_MODE_8BPP			128
#define		SPRITE_MODE_4BPP			0

#define		SPRITE_32_BY_32				(128 + 32)
#define		SPRITE_64_BY_64				(192 + 48)

#define		SPRITE_DISABLED				0
#define	    SPRITE_LAYER_BACKGROUND		(1 << 2)
#define		SPRITE_LAYER_0				(2 << 2)
#define		SPRITE_LAYER_1				(3 << 2)

#define	    SPRITE_SET_IMAGE	        0xFEF0
#define     SPRITE_MOVE		            0xFEF3
#define     SPRITE_X_SCALE(x)           (x<<5)
#define     SPRITE_Y_SCALE(y)           (y<<5)


//
//  Stuff you shouldn't care about
//
#define 	SPRITE_REGISTERS(spritenum)	((spritenum << 3) + 0xfc00)
#define     SPRITE_BLOCK_HI(addr)      ((addr >> 13) & 0x1f)
#define     SPRITE_BLOCK_LO(addr)      ((addr >> 5) & 0xff)

//
//  A note on positions:
//  15 bits is an abuse, unless you use it fractionally.
//  Let's shift it by 5 bits before using it.
//

typedef struct {
	uint16_t block; 
	uint8_t  mode;
    uint8_t  collision_mask;
    uint8_t  layer;
	uint8_t  dimensions;      
	int8_t  palette_offset;
	int     x : 15;
	int     y : 15;
} SpriteDefinition;

typedef struct {
	int x    : 15;
	int flip : 1;
	int y    : 15;
} Position;

/*
typedef struct {
	int x;
	int y;
} SmallPosition;
*/

void sprite_define(uint8_t spritenum, SpriteDefinition *sprdef);
void sprite_pos(uint8_t spritenum, Position* pos);

#endif
