// Pirate Kingdoms - X16 Production Build
// Main entry point using hardware adapters

#include <stdio.h>
#include <6502.h>
#include <cbm.h>
#include <conio.h>
#include <cx16.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

// Core game logic
#include "core/common.h"
#include "core/ship.h"
#include "core/calendar_engine.h"
#include "core/map_engine.h"

// X16 display adapters
#include "adapters/display/sprite.h"
#include "adapters/display/memory.h"
#include "adapters/display/calendar.h"
#include "adapters/display/map_display.h"

// PSG sound adapter
#include "adapters/psg/ADSR.h"

#define PLAYER_SPRITE             1
#define LOAD_TO_MAIN_RAM          0
#define LOAD_TO_VERA              2  
#define SHIP_ADDR_START           0x4000

// Map display constants
#define PEOPLE_ADDR_START         0x6000
#define LAND_ADDR_START           0x7400
#define LAND_ADDR_OCEAN           0x7400
#define LAND_ADDR_DESERT          0x8400
#define LAND_ADDR_SAVANNAH        0x9400
#define LAND_ADDR_FOREST          0xa400
#define LAND_ADDR_HILLS           0xb400
#define LAND_ADDR_MOUNTAIN        0xc400

SpriteDefinition sprdef;
Position pos;
int dx, dy;
byte shipIndex;
ShipData* ship;

void loadMapToBankedRAM()
{
    RAM_BANK = MAP_RAM_BANK_START;
    cbm_k_setnam("assets/mapsarchipelago.map");
    cbm_k_setlfs(0, 8, 0);
    cbm_k_load(LOAD_TO_MAIN_RAM, 0xa000);
}

void loadVera(char *fname, unsigned int address)
{
    cbm_k_setnam(fname);
    cbm_k_setlfs(0, 8, 0);
    cbm_k_load(LOAD_TO_VERA, address);
}

void loadSpriteDataToVERA()
{
    loadVera("assets/sprites/ships-32x32.bin", SHIP_ADDR_START);
    loadVera("assets/sprites/people-32x32.bin", PEOPLE_ADDR_START);
    loadVera("assets/sprites/terrain-64x64.bin", LAND_ADDR_START);
}

void initSprite()
{
    sprdef.mode = SPRITE_MODE_8BPP;
    sprdef.block = ship->address;
    sprdef.collision_mask = 0x0000;
    sprdef.layer = SPRITE_LAYER_0;
    sprdef.dimensions = SPRITE_32_BY_32;
    sprdef.palette_offset = 0;
    sprdef.x = SPRITE_X_SCALE(312);
    sprdef.y = SPRITE_Y_SCALE(200);

    pos.x = sprdef.x;
    pos.y = sprdef.y;

    sprite_define(PLAYER_SPRITE, &sprdef);
}

void drawMapFrame()
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

void showMenus()
{
    cputsxy(1, 4, "b: build");
    cputsxy(1, 5, "m: map");
    cputsxy(70, 4, "f1: land check");
}

void move()
{
    if (kbhit()) {
        switch(cgetc()) {
            case 0x91: // up
            case 'i':
            case 'w': 
                if (dy > -ship->speed) dy -= ship->acceleration; 
                break;

            case 0x11: // down
            case 'k':
            case 's': 
                if (dy < ship->speed) dy += ship->acceleration; 
                break;

            case 0x9d: // left
            case 'j':
            case 'a': 
                if (dx > -ship->speed) dx -= ship->acceleration; 
                break;

            case 0x1d: // right
            case 'l':
            case 'd': 
                if (dx < ship->speed) dx += ship->acceleration; 
                break;

            case 'm': // map view
                vera_sprites_enable(0);
                vera_sprites_enable(1);
                map_display_region(50);
                cgetc(); // wait for keypress
				clrscr();
                map_display_frame_draw();
                showMenus();
                break;

            case 'b': // build settlement
            {
                byte cx, cy, terr;
                map_engine_get_center_tile(&cx, &cy);
                terr = map_engine_get_terrain_at(cx, cy);
                
                if (terr == TERRAIN_WATER) {
                    gotoxy(5, 55);
                    cprintf("cannot build at (%02x,%02x): water!     ", cx, cy);
                } else if (map_engine_has_object_at(cx, cy)) {
                    gotoxy(5, 55);
                    cprintf("cannot build at (%02x,%02x): occupied!  ", cx, cy);
                } else if (map_engine_has_settlement_nearby(cx, cy, 5)) {
                    gotoxy(5, 55);
                    cprintf("cannot build at (%02x,%02x): too close! ", cx, cy);
                } else {
                    // Calculate size based on terrain
                    byte size = 50 + ((cx * cy) % 30);
                    map_engine_place_settlement_at(cx, cy, size);
                    gotoxy(5, 55);
                    cprintf("settlement founded at (%02x,%02x)!      ", cx, cy);
                }
                break;
            }

            case 133: // f1 - land check
                cputsxy(1, 20, map_engine_has_land_north() ? "n" : " ");
                cputsxy(3, 20, map_engine_has_land_east() ? "e" : " ");
                cputsxy(4, 20, map_engine_has_land_west() ? "w" : " ");
                cputsxy(2, 20, map_engine_has_land_south() ? "s" : " ");
                break;
        }
    }

    // Update map engine position
    if (dy < 0) map_engine_move_north(-dy);
    if (dy > 0) map_engine_move_south(dy);
    if (dx < 0) {
        map_engine_move_west(-dx);
        sprite_horiz_flip(PLAYER_SPRITE);
    }
    if (dx > 0) {
        map_engine_move_east(dx);
        sprite_horiz_unflip(PLAYER_SPRITE);
    }

    map_display_calculate();
}

void setPETFont()
{
    struct regs fontregs;
    fontregs.a = 4; // PET-like
    fontregs.pc = 0xff62;
    _sys(&fontregs);
}

void main()
{
    setPETFont();
    _randomize();

    bgcolor(COLOR_BLACK); 
    clrscr();

    // Load resources
    loadMapToBankedRAM();
    loadSpriteDataToVERA();

    // Initialize core game logic
    map_engine_init();
    map_display_init();
    
    // Initialize ship
    shipIndex = 7; // Genoese
    ship = getShipData(shipIndex);

    // Initialize display
    vera_sprites_enable(1);
    initSprite();
    map_display_frame_draw();
    map_display_calculate();
    showMenus();

    // Display ship info
    gotoxy(20, 52);
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

    dx = 0;
    dy = 0;

    // Main game loop
    while (1) {
        waitvsync();
        calendar_display();
        move();
    }
}
