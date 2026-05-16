#include <conio.h>
#include <peekpoke.h>
#include <cx16.h>
#include <cbm.h>

#include "map.h"
#include "sprite.h"
#include "memory.h"
#include "common.h"

SpriteDefinition mapSprite;
MapLocation map;
byte map_needs_redraw = 1;  // Flag to track if map needs recalculation

// Settlement cache for fast lookup (max 256 settlements)
typedef struct {
    byte x;
    byte y;
    byte size;
} SettlementCache;

SettlementCache settlement_cache[256];
byte settlement_count = 0;

// Cache for visible map area (7 rows × 6 cols)
typedef struct {
    byte terrain;
    byte obj_index;
} MapCell;

MapCell visible_map[MAP_VISIBLE_ROWS][MAP_VISIBLE_COLS];

// New map format constants
#define         TERRAIN_MASK                    0x0F  // Bits 0-3
#define         TRAVEL_MASK                     0x30  // Bits 4-5
#define         FLAG_SPECIAL                    0x40  // Bit 6
#define         FLAG_OBJECT                     0x80  // Bit 7
#define         MAP_OFFSET_SCALE                8
#define         TERRAIN_OVERLAP                 0
#define         HALF_VISIBLE_TILE_WIDTH       (32 - TERRAIN_OVERLAP)   

void map_init()
{
   unsigned int i, obj_addr;
   byte obj_type, obj_x, obj_y, obj_size;

   map.x = 0x4c;
   map.y = 0x64;

   map.xoffset = 0;
   map.yoffset = 0;
   
   // Build settlement cache from object list
   
   settlement_count = 0;
   RAM_BANK = MAP_RAM_BANK_START;
   
   for (i = 0; i < 1024 && settlement_count < 256; i++)
   {
       obj_addr = 0xa000 + (i * 8);
       obj_type = PEEK(obj_addr);
       
       if (obj_type == 1)  // Settlement
       {
           obj_x = PEEK(obj_addr + 1);
           obj_y = PEEK(obj_addr + 2);
           obj_size = PEEK(obj_addr + 3);
           
           settlement_cache[settlement_count].x = obj_x;
           settlement_cache[settlement_count].y = obj_y;
           settlement_cache[settlement_count].size = obj_size;
           settlement_count++;
       }
   }
}

void map_frame_draw()
{
   byte i, j;
   textcolor(COLOR_GRAY1);
   revers(1);
   for(i=0;i<7;++i)
   {
      cputsxy(0,i,"                                                                                ");
      for (j=4; j<50; j += 6)
      {
          cputsxy(0, i+j,"                    ");
          cputsxy(60,i+j,"                    ");
      }
      cputsxy(0,i+53,"                                                                                ");
   }
   revers(0);
   textcolor(COLOR_WHITE);
}

void map_south(byte v)
{
    map.yoffset -= v;
    if (map.yoffset < -HALF_VISIBLE_TILE_WIDTH * MAP_OFFSET_SCALE)
    {
        map.yoffset = HALF_VISIBLE_TILE_WIDTH * MAP_OFFSET_SCALE;
        ++map.y;
        map_needs_redraw = 1;
        gotoxy(20,7);
        cprintf("%2x,%2x", map.x, map.y);
    }
}

void map_north(byte v)
{
    map.yoffset += v;
    if (map.yoffset > HALF_VISIBLE_TILE_WIDTH * MAP_OFFSET_SCALE)
    {
        map.yoffset = -HALF_VISIBLE_TILE_WIDTH * MAP_OFFSET_SCALE;
        --map.y;
        map_needs_redraw = 1;
        gotoxy(20,7);
        cprintf("%2x,%2x", map.x, map.y);
    }
}

void map_east(byte v)
{
    map.xoffset -= v;
    if (map.xoffset < -HALF_VISIBLE_TILE_WIDTH * MAP_OFFSET_SCALE)
    {
        map.xoffset = HALF_VISIBLE_TILE_WIDTH * MAP_OFFSET_SCALE;
        ++map.x;
        map_needs_redraw = 1;
        gotoxy(20,7);
        cprintf("%2x,%2x", map.x, map.y);
    }
}

void map_west(byte v)
{
    map.xoffset += v;
    if (map.xoffset > HALF_VISIBLE_TILE_WIDTH * MAP_OFFSET_SCALE)
    {
        map.xoffset = -HALF_VISIBLE_TILE_WIDTH * MAP_OFFSET_SCALE;
        --map.x;
        map_needs_redraw = 1;
        gotoxy(20,7);
        cprintf("%2x,%2x", map.x, map.y);
    }
}

byte get_map(byte x, byte y)
{
    byte y3 = y/32;
    byte yy = y%32;
    RAM_BANK = MAP_RAM_BANK_START + y3;
    // Map data starts at 8KB offset (after object list)
    // Each cell is 2 bytes: terrain+flags, object_index
    return PEEK(0xa000 + 0x2000 + yy * 512 + x * 2);
}

byte get_map_object_index(byte x, byte y)
{
    byte y3 = y/32;
    byte yy = y%32;
    RAM_BANK = MAP_RAM_BANK_START + y3;
    // Second byte of each cell
    return PEEK(0xa000 + 0x2000 + yy * 512 + x * 2 + 1);
}

byte map_has_land_north()
{
    byte land = get_map(map.x+3, map.y-2);
    land &= TERRAIN_MASK;
    return land;
}

byte map_has_land_south()
{
    byte land = get_map(map.x+3, map.y+4);
    land &= TERRAIN_MASK;
    return land;
}

byte map_has_land_east()
{
    byte land = get_map(map.x+4, map.y+3);
    land &= TERRAIN_MASK;
    return land;
}

byte map_has_land_west()
{
    byte land = get_map(map.x-2, map.y+3);
    land &= TERRAIN_MASK;
    return land;
}

void map_calculate()
{
    byte row, col;
    byte y3, yy;
    unsigned int map_addr;
    byte terrainSprite = 2;
    byte settlementSprite = 50;
    byte terrain;
    byte obj_index;
    
    // Read map data from banked RAM only when view changes
    if (map_needs_redraw)
    {
        map_needs_redraw = 0;
        
        for(row=0; row<MAP_VISIBLE_ROWS; ++row)
        {
            byte actual_y = map.y + row;
            y3 = actual_y / 16;
            RAM_BANK = MAP_RAM_BANK_START + 1 + y3;
            yy = actual_y % 16;
            
            for(col=0; col<MAP_VISIBLE_COLS; ++col)
            {
                map_addr = 0xa000 + yy * 512 + (col + map.x) * 2;
                visible_map[row][col].terrain = PEEK(map_addr) & TERRAIN_MASK;
                visible_map[row][col].obj_index = PEEK(map_addr + 1);
            }
        }
    }
    
    // Update sprite positions every frame (smooth scrolling)
    
    for(row=0; row<MAP_VISIBLE_ROWS; ++row)
    {
        for(col=0; col<MAP_VISIBLE_COLS; ++col)
        {
            terrain = visible_map[row][col].terrain;
            obj_index = visible_map[row][col].obj_index;
            
            // Draw terrain sprite (64x64)
            mapSprite.mode           = SPRITE_MODE_8BPP;
            mapSprite.layer          = SPRITE_LAYER_0;
            mapSprite.dimensions     = SPRITE_64_BY_64;
            mapSprite.palette_offset = 0;
            mapSprite.x              = SPRITE_X_SCALE(map.xoffset/MAP_OFFSET_SCALE + 128 + col * (64-TERRAIN_OVERLAP));
            mapSprite.y              = SPRITE_Y_SCALE(map.yoffset/MAP_OFFSET_SCALE + 24  + row * (64-TERRAIN_OVERLAP));

            // Terrain types: 0=water, 1=grass, 2=forest, 3=hills, 4=mountains, 5=desert, 6=swamp
            switch(terrain)
            {
                case 0:  // water
                     mapSprite.block = LAND_ADDR_OCEAN; 
                     break;
                case 1:  // grass
                     mapSprite.block = LAND_ADDR_SAVANNAH; 
                     break;
                case 2:  // forest
                     mapSprite.block = LAND_ADDR_FOREST; 
                     break;
                case 3:  // hills
                     mapSprite.block = LAND_ADDR_HILLS; 
                     break;
                case 4:  // mountains
                     mapSprite.block = LAND_ADDR_MOUNTAIN; 
                     break;
                case 5:  // desert
                     mapSprite.block = LAND_ADDR_DESERT; 
                     break;
                case 6:  // swamp (placeholder)
                     mapSprite.block = LAND_ADDR_SAVANNAH; 
                     break;
                default:
                     mapSprite.block = LAND_ADDR_OCEAN; 
                     break;
            }

            sprite_define(terrainSprite, &mapSprite);
            ++terrainSprite;

            // Draw settlement sprite using direct object index lookup
            if (obj_index > 0)
            {
                byte size = settlement_cache[obj_index - 1].size;  // Index is 1-based
                
                mapSprite.dimensions = SPRITE_32_BY_32;
                mapSprite.layer      = SPRITE_LAYER_1;  // above terrain
                
                // Map size ranges to sprite types
                if (size < 40)
                {
                    mapSprite.block = PEOPLE_ADDR_CAMP;
                }
                else if (size < 70)
                {
                    mapSprite.block = PEOPLE_ADDR_VILLAGE;
                }
                else if (size < 100)
                {
                    mapSprite.block = PEOPLE_ADDR_PUEBLO;
                }
                else if (size < 120)
                {
                    mapSprite.block = PEOPLE_ADDR_AZTEC;
                }
                else
                {
                    mapSprite.block = PEOPLE_ADDR_INCA;
                }
                
                sprite_define(settlementSprite, &mapSprite);
                ++settlementSprite;
            }
        }
    }
}

char* landchar = "wgfhmdS";  // water, grass, forest, hills, mountains, desert, swamp
// Terrain color mapping for new format: 0=water, 1=grass, 2=forest, 3=hills, 4=mountains, 5=desert, 6=swamp
byte  landcolor[] = {
    COLOR_BLUE,    // 0: TERRAIN_WATER
    COLOR_LIGHTGREEN,   // 1: TERRAIN_GRASS
    COLOR_GREEN,   // 2: TERRAIN_FOREST
    COLOR_BROWN,   // 3: TERRAIN_HILLS
    COLOR_GRAY1,   // 4: TERRAIN_MOUNTAINS
    COLOR_YELLOW,  // 5: TERRAIN_DESERT
    COLOR_CYAN     // 6: TERRAIN_SWAMP
};

void map_region(byte dimension)
{
    byte row, r, col, c, terrain, bank, yy;
    byte half_dim = dimension / 2;
    byte start_col = (80 - dimension) / 2;  // Center horizontally
    byte start_row = (60 - dimension) / 2;  // Center vertically
    
    clrscr();

    revers(1);
    
    for(row=0; row<dimension; ++row)
    {
        r = map.y + row - half_dim;
        bank = r / 16;         // 16 rows per bank
        RAM_BANK = MAP_RAM_BANK_START + 1 + bank;  // Bank 12+ has map data
        yy = r % 16;

        for(col=0; col<dimension; ++col)
        {
           c = map.x + col - half_dim;
           // Read from 2-byte format: terrain+flags is first byte
           terrain = PEEK(0xa000 + yy * 512 + c * 2);
           terrain &= TERRAIN_MASK;  // Extract terrain from lower 4 bits

           textcolor(landcolor[terrain]);
           cputcxy(start_col + col, start_row + row, '.');
        }
    }
    revers(0);

    // Mark player position (center tile of visible map is at map.x+3, map.y+3)
    {
        byte px = map.x + 3;
        byte py = map.y + 3;
        byte marker_col = start_col + (px - (map.x - half_dim));
        byte marker_row = start_row + (py - (map.y - half_dim)) - 1;  // nudge down one row
        textcolor(COLOR_YELLOW);
        cputcxy(marker_col, marker_row, 'x');
    }
    cgetc();
    clrscr();
}

// Helper functions for settlement placement
byte map_get_terrain_at(byte x, byte y)
{
    byte bank = y / 16;
    byte yy = y % 16;
    RAM_BANK = MAP_RAM_BANK_START + 1 + bank;  // Bank 12+ has map data
    return PEEK(0xa000 + yy * 512 + x * 2) & TERRAIN_MASK;
}

byte map_has_object_at(byte x, byte y)
{
    byte bank = y / 16;
    byte yy = y % 16;
    RAM_BANK = MAP_RAM_BANK_START + 1 + bank;  // Bank 12+ has map data
    return PEEK(0xa000 + yy * 512 + x * 2 + 1) != 0;
}

void map_place_settlement_at(byte x, byte y)
{
    byte bank = y / 16;
    byte yy = y % 16;
    unsigned int addr = 0xa000 + yy * 512 + x * 2;
    byte v;
    byte terrain;

    RAM_BANK = MAP_RAM_BANK_START + 1 + bank;  // Bank 12+ has map data
    
    // Read current terrain
    v = PEEK(addr);
    terrain = v & TERRAIN_MASK;
    
    // Set flags on terrain byte
    v |= FLAG_OBJECT;   // mark object
    v |= FLAG_SPECIAL;  // special marker
    POKE(addr, v);
    
    // Add to settlement cache and write object index
    if (settlement_count < 256)
    {
        settlement_cache[settlement_count].x = x;
        settlement_cache[settlement_count].y = y;
        
        // Assign size based on terrain (matching build-world.pl logic)
        switch(terrain)
        {
            case 1:  // grass
                settlement_cache[settlement_count].size = 80 + (x * y) % 40;  // 80-119
                break;
            case 2:  // forest
                settlement_cache[settlement_count].size = 40 + (x * y) % 30;  // 40-69
                break;
            case 5:  // desert
                settlement_cache[settlement_count].size = 20 + (x * y) % 20;  // 20-39
                break;
            default:
                settlement_cache[settlement_count].size = 50 + (x * y) % 30;  // 50-79
                break;
        }
        
        settlement_count++;
        
        // Write 1-based object index to second byte
        POKE(addr + 1, settlement_count);  // settlement_count was just incremented, so it's correct
    }
}

byte map_has_settlement_nearby(byte x, byte y, byte radius)
{
    byte dx, dy;
    byte startX = (x > radius) ? (x - radius) : 0;
    byte endX = (x + radius < 255) ? (x + radius) : 255;
    byte startY = (y > radius) ? (y - radius) : 0;
    byte endY = (y + radius < 255) ? (y + radius) : 255;
    
    for (dy = startY; dy <= endY; ++dy)
    {
        for (dx = startX; dx <= endX; ++dx)
        {
            // Skip the center tile itself
            if (dx == x && dy == y) continue;
            
            // Check for settlement
            if (map_has_object_at(dx, dy))
            {
                return 1;  // Found a settlement nearby
            }
        }
    }
    
    return 0;  // No settlements nearby
}

void map_get_center_tile(byte *x, byte *y)
{
    *x = map.x + 3;
    *y = map.y + 3;
}

// Find settlement object at given coordinates and return its size (0 if not found)
byte map_get_settlement_size(byte x, byte y)
{
    byte i;
    
    // Search the cached settlement list
    for (i = 0; i < settlement_count; i++)
    {
        if (settlement_cache[i].x == x && settlement_cache[i].y == y)
        {
            return settlement_cache[i].size;
        }
    }
    
    return 0;  // No settlement found at this location
}