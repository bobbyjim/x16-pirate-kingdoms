#include <stdio.h>
#include <string.h>
#include "map.h"

static word count_objects(const Map *m)
{
    word i, count = 0;
    for (i = 0; i < MAP_MAX_OBJECTS; i++) {
        if (m->object_list[i * MAP_OBJECT_SIZE] != OBJ_EMPTY) count++;
    }
    return count;
}

int map_load(Map *m, const char *path)
{
    FILE *f = fopen(path, "rb");
    size_t n;

    if (!f) return -1;

    n = fread(m->object_list, 1, MAP_OBJECT_LIST_BYTES, f);
    if (n != MAP_OBJECT_LIST_BYTES) { fclose(f); return -1; }

    n = fread(m->data, 1, MAP_DATA_BYTES, f);
    if (n != MAP_DATA_BYTES) { fclose(f); return -1; }

    fclose(f);
    m->object_count = count_objects(m);
    return 0;
}

void map_init_empty(Map *m)
{
    memset(m->object_list, 0, MAP_OBJECT_LIST_BYTES);
    memset(m->data, 0, MAP_DATA_BYTES); /* terrain 0 = water, no flags */
    m->object_count = 0;
}

static word cell_offset(byte x, byte y)
{
    return ((word)y * MAP_WIDTH + x) * MAP_CELL_BYTES;
}

byte map_get_terrain_at(const Map *m, byte x, byte y)
{
    return m->data[cell_offset(x, y)] & TERRAIN_MASK;
}

byte map_get_travel_ease(const Map *m, byte x, byte y)
{
    return (m->data[cell_offset(x, y)] & TRAVEL_MASK) >> 4;
}

byte map_is_special_zone(const Map *m, byte x, byte y)
{
    return (m->data[cell_offset(x, y)] & FLAG_SPECIAL) != 0;
}

byte map_has_object_at(const Map *m, byte x, byte y)
{
    return (m->data[cell_offset(x, y)] & FLAG_OBJECT) != 0;
}

byte map_get_object_index_at(const Map *m, byte x, byte y)
{
    return m->data[cell_offset(x, y) + 1];
}

word map_object_count(const Map *m)
{
    return m->object_count;
}

void map_get_object(const Map *m, word index, MapObject *out)
{
    const byte *entry = &m->object_list[index * MAP_OBJECT_SIZE];
    out->type = entry[0];
    out->x = entry[1];
    out->y = entry[2];
    memcpy(out->data, &entry[3], 5);
}

word map_count_settlement_objects(const Map *m)
{
    word i, count = 0;
    for (i = 0; i < MAP_MAX_OBJECTS; i++) {
        if (m->object_list[i * MAP_OBJECT_SIZE] == OBJ_SETTLEMENT) count++;
    }
    return count;
}

static byte chebyshev_distance(byte ax, byte ay, byte bx, byte by)
{
    byte dx = (ax > bx) ? (ax - bx) : (bx - ax);
    byte dy = (ay > by) ? (ay - by) : (by - ay);
    return (dx > dy) ? dx : dy;
}

word map_find_nearby_settlements(const Map *m, byte x, byte y, byte radius,
                                  word *out_indices, word max_out)
{
    word i, found = 0;

    for (i = 0; i < MAP_MAX_OBJECTS && found < max_out; i++) {
        const byte *entry = &m->object_list[i * MAP_OBJECT_SIZE];
        byte ox, oy;

        if (entry[0] != OBJ_SETTLEMENT) continue;

        ox = entry[1];
        oy = entry[2];
        if (ox == x && oy == y) continue; /* skip self */

        if (chebyshev_distance(x, y, ox, oy) <= radius) {
            out_indices[found++] = i;
        }
    }
    return found;
}
