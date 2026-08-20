#ifndef _MAP_H_
#define _MAP_H_

#include "common.h"

/* Portable re-implementation of src-prototype1/map.c's on-disk map format.
   Same layout, same bit meanings -- just read from a plain in-memory
   buffer instead of banked X16 RAM (no PEEK/RAM_BANK/sprite code here). */

#define MAP_WIDTH             256
#define MAP_HEIGHT            256

#define MAP_OBJECT_LIST_BYTES 8192   /* 1024 objects * 8 bytes */
#define MAP_OBJECT_SIZE       8
#define MAP_MAX_OBJECTS       (MAP_OBJECT_LIST_BYTES / MAP_OBJECT_SIZE)

/* Terrain grid: 2 bytes/cell (terrain+flags, object index), matching the
   real archipelago.map/MAP.BIN files (65536 cells * 2 bytes = 131072). */
#define MAP_CELL_BYTES        2
#define MAP_DATA_BYTES        (MAP_WIDTH * MAP_HEIGHT * MAP_CELL_BYTES)

#define MAP_FILE_BYTES        (MAP_OBJECT_LIST_BYTES + MAP_DATA_BYTES)

/* Cell byte 0 bit layout (from src-prototype1/map.c) */
#define TERRAIN_MASK   0x0F   /* bits 0-3 */
#define TRAVEL_MASK    0x30   /* bits 4-5 */
#define FLAG_SPECIAL   0x40   /* bit 6 */
#define FLAG_OBJECT    0x80   /* bit 7 */

typedef enum {
    TERRAIN_WATER = 0,
    TERRAIN_GRASS = 1,
    TERRAIN_FOREST = 2,
    TERRAIN_HILLS = 3,
    TERRAIN_MOUNTAINS = 4,
    TERRAIN_DESERT = 5,
    TERRAIN_SWAMP = 6
} Terrain;

/* Object list entry types (from WORLD-CREATION.md) */
typedef enum {
    OBJ_EMPTY = 0x00,
    OBJ_SETTLEMENT = 0x01,
    OBJ_SHIP = 0x02,
    OBJ_GROUP = 0x03,
    OBJ_TOWER = 0x04,
    OBJ_SHRINE = 0x05,
    OBJ_RUINS = 0x06,
    OBJ_MINE = 0x07,
    OBJ_STELA = 0x08,
    OBJ_PORTAL = 0x09,
    OBJ_LIGHTHOUSE = 0x0A,
    OBJ_BRIDGE = 0x0B,
    OBJ_CAVE = 0x0C,
    OBJ_MONUMENT = 0x0D
} MapObjectType;

typedef struct {
    byte type;
    byte x;
    byte y;
    byte data[5];   /* type-specific; byte0 of data is settlement size for OBJ_SETTLEMENT */
} MapObject;

typedef struct {
    byte object_list[MAP_OBJECT_LIST_BYTES];
    byte data[MAP_DATA_BYTES];
    word object_count;   /* number of non-empty entries, cached at load time */
} Map;

/* Loading */
int  map_load(Map *m, const char *path);           /* 0 on success, -1 on error */
void map_init_empty(Map *m);                        /* all water, no objects */

/* Terrain / cell queries */
byte map_get_terrain_at(const Map *m, byte x, byte y);
byte map_get_travel_ease(const Map *m, byte x, byte y);
byte map_is_special_zone(const Map *m, byte x, byte y);
byte map_has_object_at(const Map *m, byte x, byte y);
byte map_get_object_index_at(const Map *m, byte x, byte y); /* 1-based, 0 = none */

/* Object list access */
word map_object_count(const Map *m);
void map_get_object(const Map *m, word index, MapObject *out); /* 0-based index */

/* Settlement-focused helpers (object type == OBJ_SETTLEMENT) */
word map_count_settlement_objects(const Map *m);

/* Finds up to max_out settlement object indices (0-based, into the object
   list) within `radius` tiles (Chebyshev distance) of (x,y), excluding any
   object located exactly at (x,y). Returns the number found. */
word map_find_nearby_settlements(const Map *m, byte x, byte y, byte radius,
                                  word *out_indices, word max_out);

#endif
