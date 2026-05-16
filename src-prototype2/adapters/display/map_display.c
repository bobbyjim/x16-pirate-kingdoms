#include <conio.h>
#include <peekpoke.h>
#include <cx16.h>
#include "map_display.h"
#include "sprite.h"
#include "memory.h"

/* X16 map display adapter - renders terrain and settlements using sprites */

extern SpriteDefinition mapSprite;
extern MapLocation map_loc;  /* Current map location from engine */

/* Map display constants */
#define TERRAIN_OVERLAP    0
#define HALF_VISIBLE_TILE_WIDTH  (32 - TERRAIN_OVERLAP)
#define MAP_OFFSET_SCALE   8

/* Terrain sprite addresses */
#define LAND_ADDR_OCEAN     0x7400
#define LAND_ADDR_DESERT    0x8400
#define LAND_ADDR_SAVANNAH  0x9400
#define LAND_ADDR_FOREST    0xa400
#define LAND_ADDR_HILLS     0xb400
#define LAND_ADDR_MOUNTAIN  0xc400

/* Settlement sprite addresses */
#define PEOPLE_ADDR_CAMP    0x6000
#define PEOPLE_ADDR_VILLAGE 0x6400
#define PEOPLE_ADDR_PUEBLO  0x6800
#define PEOPLE_ADDR_AZTEC   0x6c00
#define PEOPLE_ADDR_INCA    0x7000

/* Terrain format constants */
#define TERRAIN_MASK        0x0F
#define TRAVEL_MASK         0x30
#define FLAG_SPECIAL        0x40
#define FLAG_OBJECT         0x80

SpriteDefinition mapSprite;

/* Terrain display characters and colors */
char landchar[7] = "wgfhmdS";  /* water, grass, forest, hills, mountains, desert, swamp */
byte landcolor[7] = {
    COLOR_BLUE,         /* 0: water */
    COLOR_LIGHTGREEN,   /* 1: grass */
    COLOR_GREEN,        /* 2: forest */
    COLOR_BROWN,        /* 3: hills */
    COLOR_GRAY1,        /* 4: mountains */
    COLOR_YELLOW,       /* 5: desert */
    COLOR_CYAN          /* 6: swamp */
};

void map_display_init(void)
{
    /* Nothing to initialize for now */
}

void map_display_frame_draw(void)
{
    byte i, j;
    textcolor(COLOR_GRAY1);
    revers(1);
    
    for (i = 0; i < 7; ++i) {
        cputsxy(0, i, "                                                                                ");
        for (j = 4; j < 50; j += 6) {
            cputsxy(0, i + j, "                    ");
            cputsxy(60, i + j, "                    ");
        }
        cputsxy(0, i + 53, "                                                                                ");
    }
    
    revers(0);
    textcolor(COLOR_WHITE);
}

void map_display_calculate(void)
{
    byte row, col;
    byte terrain;
    byte terrainSprite = 2;
    MapLocation *loc;
    
    loc = map_engine_get_location();
    
    /* Render visible map area (7x6 tiles) */
    for (row = 0; row < MAP_VISIBLE_ROWS; ++row) {
        for (col = 0; col < MAP_VISIBLE_COLS; ++col) {
            terrain = map_engine_get_terrain_at(loc->x + col, loc->y + row);
            
            /* Draw terrain sprite (64x64) */
            mapSprite.mode = SPRITE_MODE_8BPP;
            mapSprite.layer = SPRITE_LAYER_0;
            mapSprite.dimensions = SPRITE_64_BY_64;
            mapSprite.palette_offset = 0;
            mapSprite.x = SPRITE_X_SCALE(loc->xoffset / MAP_OFFSET_SCALE + 128 + col * (64 - TERRAIN_OVERLAP));
            mapSprite.y = SPRITE_Y_SCALE(loc->yoffset / MAP_OFFSET_SCALE + 24 + row * (64 - TERRAIN_OVERLAP));

            /* Map terrain type to sprite address */
            switch (terrain) {
                case TERRAIN_WATER:
                    mapSprite.block = LAND_ADDR_OCEAN;
                    break;
                case TERRAIN_GRASS:
                    mapSprite.block = LAND_ADDR_SAVANNAH;
                    break;
                case TERRAIN_FOREST:
                    mapSprite.block = LAND_ADDR_FOREST;
                    break;
                case TERRAIN_HILLS:
                    mapSprite.block = LAND_ADDR_HILLS;
                    break;
                case TERRAIN_MOUNTAINS:
                    mapSprite.block = LAND_ADDR_MOUNTAIN;
                    break;
                case TERRAIN_DESERT:
                    mapSprite.block = LAND_ADDR_DESERT;
                    break;
                default:
                    mapSprite.block = LAND_ADDR_OCEAN;
                    break;
            }

            sprite_define(terrainSprite, &mapSprite);
            ++terrainSprite;
        }
    }
}

void map_display_region(byte dimension)
{
    byte row, r, col, c, terrain, bank, yy;
    byte half_dim = dimension / 2;
    byte start_col = (80 - dimension) / 2;  /* Center horizontally */
    byte start_row = (60 - dimension) / 2;  /* Center vertically */
    MapLocation *loc;
    
    loc = map_engine_get_location();
    
    revers(1);
    
    for (row = 0; row < dimension; ++row) {
        r = loc->y + row - half_dim;
        bank = r / 16;         /* 16 rows per bank */
        yy = r % 16;
        RAM_BANK = MAP_RAM_BANK_START + 1 + bank;  /* Bank 12+ has map data */

        for (col = 0; col < dimension; ++col) {
            c = loc->x + col - half_dim;
            /* Read from 2-byte format: terrain+flags is first byte */
            /* Map data starts at 0xa000 + 0x2000 (after object list) */
            terrain = PEEK(0xa000 + 0x2000 + yy * 512 + c * 2);
            terrain &= TERRAIN_MASK;  /* Extract terrain from lower 4 bits */

            textcolor(landcolor[terrain]);
            cputcxy(start_col + col, start_row + row, landchar[terrain]);
        }
    }
    revers(0);

    /* Mark player position (center tile of visible map is at loc->x+3, loc->y+3) */
    {
        byte px = loc->x + 3;
        byte py = loc->y + 3;
        byte marker_col = start_col + (px - (loc->x - half_dim));
        byte marker_row = start_row + (py - (loc->y - half_dim)) - 1;  /* nudge down one row */
        textcolor(COLOR_YELLOW);
        cputcxy(marker_col, marker_row, 'x');
    }
}

