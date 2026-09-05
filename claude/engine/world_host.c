#include "world.h"

int world_load(World *w, const char *map_path, unsigned long seed)
{
    if (map_load(&w->map, map_path) != 0) return -1;
    return world_bootstrap_loaded_map(w, seed);
}