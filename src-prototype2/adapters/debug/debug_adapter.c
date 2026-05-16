#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "debug_adapter.h"
#include "../../core/ship.h"
#include "../../core/calendar_engine.h"
#include "../../core/map_engine.h"

// Debug adapter: text-based output for development and testing

void debug_init()
{
    printf("=== Pirate Kingdoms Debug Mode ===\n");
    printf("Commands: ship <n>, calendar, map, help\n");
    
    // Initialize map engine
    map_engine_init();
}

void debug_print_ship_state(int ship_index)
{
    ShipData *ship = getShipData(ship_index);
    if (!ship) {
        printf("ERROR: Invalid ship index %d\n", ship_index);
        return;
    }
    
    printf("\n--- Ship %d: %s ---\n", ship_index, ship->name);
    printf("  Speed: %d | Acceleration: %d | Hull: %d\n", 
        ship->speed, ship->acceleration, ship->hull);
    printf("  Capacity - People: %d | Goods: %d\n",
        ship->people_capacity, ship->goods_capacity);
    printf("  Capacity - Ballista: %d | Greek Fire: %d\n",
        ship->ballista_capacity, ship->greek_fire_capability);
    printf("  Mission: %s\n", ship->mission);
}

void debug_print_calendar_state()
{
    CalendarDate date;
    CalendarHaab haab;
    
    calendar_get_long_count(&date);
    calendar_get_haab(&haab);
    
    printf("\n--- Calendar ---\n");
    printf("  Long Count: %d.%d.%d.%d\n",
        date.baktun, date.katun, date.winal, date.kin);
    
    printf("  Haab: %s %d %s\n",
        calendar_get_haab_name(haab.haab_month),
        haab.trecena + 1,
        calendar_get_tzolkin_name(haab.kin));
}

void debug_print_map_state()
{
    MapLocation *loc = map_engine_get_location();
    byte cx, cy;
    
    map_engine_get_center_tile(&cx, &cy);
    
    printf("\n--- Map ---\n");
    printf("  Position: (%d, %d) | Offset: (%d, %d)\n",
        loc->x, loc->y, loc->xoffset, loc->yoffset);
    printf("  Center tile: (%d, %d)\n", cx, cy);
    printf("  Terrain at center: %d\n", 
        map_engine_get_terrain_at(cx, cy));
    printf("  Settlements: %d\n", map_engine_get_settlement_count());
    printf("  Land checks: N=%d S=%d E=%d W=%d\n",
        map_engine_has_land_north(),
        map_engine_has_land_south(),
        map_engine_has_land_east(),
        map_engine_has_land_west());
}

void debug_print_map_region(int dimension)
{
    char landchar[7] = ".gfhmdS";
    MapLocation *loc = map_engine_get_location();
    byte row, col, r, c, terrain;
    byte half_dim = dimension / 2;
    byte px = loc->x + 3;
    byte py = loc->y + 3;
    
    printf("\n--- Map Region (dimension=%d) ---\n", dimension);
    printf("Player at: (%d, %d)\n\n", px, py);
    
    for (row = 0; row < dimension; ++row) {
        r = loc->y + row - half_dim;
        for (col = 0; col < dimension; ++col) {
            c = loc->x + col - half_dim;
            terrain = map_engine_get_terrain_at(c, r);
            
            /* Mark player position */
            if (c == px && r == py) {
                printf("X");
            } else {
                printf("%c", landchar[terrain & 0x0F]);
            }
        }
        printf("\n");
    }
}

void debug_print_map_full(void)
{
    char landchar[7] = ".gfhmdS";
    int row, col;
    byte terrain;
    
    printf("\n--- Full Map (256x256, sampling every 4th tile) ---\n");
    printf("(64x64 display)\n\n");
    
    for (row = 0; row < 64; ++row) {
        for (col = 0; col < 64; ++col) {
            terrain = map_engine_get_terrain_at(col * 4, row * 4);
            printf("%c", landchar[terrain & 0x0F]);
        }
        printf("\n");
    }
}



void debug_input_command(char *cmd)
{
    if (strncmp(cmd, "ship", 4) == 0) {
        int index = atoi(&cmd[5]);
        debug_print_ship_state(index);
    } else if (strncmp(cmd, "calendar", 8) == 0) {
        debug_print_calendar_state();
    } else if (strncmp(cmd, "mapfull", 7) == 0) {
        debug_print_map_full();
    } else if (strncmp(cmd, "mapregion", 9) == 0) {
        int dimension = atoi(&cmd[10]);
        if (dimension <= 0) dimension = 50;
        debug_print_map_region(dimension);
    } else if (strncmp(cmd, "map", 3) == 0) {
        debug_print_map_state();
    } else {
        printf("Unknown command: %s\n", cmd);
    }
}
