#include <stdio.h>
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
    if (n != MAP_OBJECT_LIST_BYTES) {
        fclose(f);
        return -1;
    }

    n = fread(m->data, 1, MAP_DATA_BYTES, f);
    if (n != MAP_DATA_BYTES) {
        fclose(f);
        return -1;
    }

    fclose(f);
    m->object_count = count_objects(m);
    return 0;
}