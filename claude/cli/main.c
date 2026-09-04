/* Host-side CLI/debug harness for the Pirate Kingdoms simulation engine.
   This is deliberately host-only (stdio, malloc-free but interactive) --
   it is NOT the X16 UI, just the "text based" adapter the retro-beast-mode
   agent notes asked for so the engine can be exercised without hardware. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> /* strcasecmp: host CLI only, not used by the engine */

#include "../engine/common.h"
#include "../engine/world.h"
#include "../present/present.h"

#define MAP_WINDOW_SIZE 16

static World world;
static byte world_ready = 0;
static byte view_center_x = MAP_WIDTH / 2;
static byte view_center_y = MAP_HEIGHT / 2;
static byte cursor_row = MAP_WINDOW_SIZE / 2;
static byte cursor_col = MAP_WINDOW_SIZE / 2;

static void print_settlement(const Settlement *s);
static void print_settlement_header(void);

static byte clamp_view_center(int v)
{
    if (v < MAP_WINDOW_SIZE / 2) return MAP_WINDOW_SIZE / 2;
    if (v > MAP_WIDTH - (MAP_WINDOW_SIZE / 2)) return MAP_WIDTH - (MAP_WINDOW_SIZE / 2);
    return (byte)v;
}

static byte map_window_origin(byte center)
{
    int origin = center - (MAP_WINDOW_SIZE / 2);
    if (origin < 0) origin = 0;
    if (origin > MAP_WIDTH - MAP_WINDOW_SIZE) origin = MAP_WIDTH - MAP_WINDOW_SIZE;
    return (byte)origin;
}

static byte clamp_window_index(int v)
{
    if (v < 0) return 0;
    if (v >= MAP_WINDOW_SIZE) return MAP_WINDOW_SIZE - 1;
    return (byte)v;
}

static byte current_origin_x(void)
{
    return map_window_origin(view_center_x);
}

static byte current_origin_y(void)
{
    return map_window_origin(view_center_y);
}

static byte current_cursor_world_x(void)
{
    return (byte)(current_origin_x() + cursor_col);
}

static byte current_cursor_world_y(void)
{
    return (byte)(current_origin_y() + cursor_row);
}

static void sync_cursor_to_world(byte x, byte y)
{
    cursor_col = clamp_window_index((int)x - (int)current_origin_x());
    cursor_row = clamp_window_index((int)y - (int)current_origin_y());
}

static void reset_cursor(void)
{
    cursor_row = MAP_WINDOW_SIZE / 2;
    cursor_col = MAP_WINDOW_SIZE / 2;
}

static void reset_view_center(void)
{
    word i;

    for (i = 0; i < world.settlement_count; i++) {
        const Settlement *s = world_get_settlement(&world, (byte)i);
        if (!s || !s->alive) continue;
        view_center_x = clamp_view_center(s->x);
        view_center_y = clamp_view_center(s->y);
        sync_cursor_to_world(s->x, s->y);
        return;
    }

    view_center_x = MAP_WIDTH / 2;
    view_center_y = MAP_HEIGHT / 2;
    reset_cursor();
}

static char terrain_glyph(byte terrain)
{
    switch (terrain) {
        case TERRAIN_WATER:     return '.';
        case TERRAIN_GRASS:     return 'g';
        case TERRAIN_FOREST:    return 'f';
        case TERRAIN_HILLS:     return 'h';
        case TERRAIN_MOUNTAINS: return 'm';
        case TERRAIN_DESERT:    return 'd';
        case TERRAIN_SWAMP:     return 'w';
        default:                return '?';
    }
}

static char object_glyph(byte type)
{
    switch (type) {
        case OBJ_SETTLEMENT: return 'X';
        case OBJ_TOWER:      return 'T';
        case OBJ_SHRINE:     return 'S';
        case OBJ_RUINS:      return 'R';
        case OBJ_MINE:       return 'M';
        case OBJ_STELA:      return 'A';
        case OBJ_PORTAL:     return 'P';
        case OBJ_CAVE:       return 'C';
        case OBJ_MONUMENT:   return 'O';
        case OBJ_LIGHTHOUSE: return 'L';
        case OBJ_BRIDGE:     return 'B';
        case OBJ_SHIP:       return 's';
        case OBJ_GROUP:      return 'G';
        default:             return '?';
    }
}

static const char *object_type_name(byte type)
{
    switch (type) {
        case OBJ_SETTLEMENT: return "settlement";
        case OBJ_TOWER:      return "tower";
        case OBJ_SHRINE:     return "shrine";
        case OBJ_RUINS:      return "ruins";
        case OBJ_MINE:       return "mine";
        case OBJ_STELA:      return "stela";
        case OBJ_PORTAL:     return "portal";
        case OBJ_CAVE:       return "cave";
        case OBJ_MONUMENT:   return "monument";
        case OBJ_LIGHTHOUSE: return "lighthouse";
        case OBJ_BRIDGE:     return "bridge";
        case OBJ_SHIP:       return "ship";
        case OBJ_GROUP:      return "group";
        default:             return "unknown";
    }
}

static int parse_hex_nibble(const char *s)
{
    char *end;
    unsigned long value;

    if (!s || s[0] == '\0') return -1;
    value = strtoul(s, &end, 16);
    if (*end != '\0' || value >= MAP_WINDOW_SIZE) return -1;
    return (int)value;
}

static int glyph_for_tile(const WorldTileInfo *tile)
{
    if (tile->object_index > 0) return object_glyph(tile->object.type);
    if (tile->travel_ease == 3) return '=';
    return terrain_glyph(tile->terrain);
}

static void print_cursor_status(void)
{
    WorldTileInfo tile;

    world_get_tile_info(&world, current_cursor_world_x(), current_cursor_world_y(), &tile);
    printf("cursor=(%X,%X) world=(%u,%u) tile=%c",
           cursor_row, cursor_col, tile.x, tile.y, glyph_for_tile(&tile));
    if (tile.object_index > 0) {
        printf(" object=%s#%u", object_type_name(tile.object.type), tile.object_index);
    }
    printf("\n");
}
static void cmd_visible(void)
{
    byte ids[MAP_WINDOW_SIZE * MAP_WINDOW_SIZE];
    byte origin_x, origin_y;
    word found, i;

    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }

    origin_x = current_origin_x();
    origin_y = current_origin_y();
    found = world_find_settlements_in_rect(&world, origin_x, origin_y,
                                           MAP_WINDOW_SIZE, MAP_WINDOW_SIZE,
                                           ids, MAP_WINDOW_SIZE * MAP_WINDOW_SIZE);

    if (found == 0) {
        printf("no settlements visible in the current 16x16 window\n");
        return;
    }

    printf("visible settlements (%u)\n", found);
    for (i = 0; i < found; i++) {
        const Settlement *s = world_get_settlement_const(&world, ids[i]);
        if (!s) continue;
        printf("  %X,%X  #%u  (%u,%u)  %s  %s\n",
               (byte)(s->y - origin_y), (byte)(s->x - origin_x),
               s->id, s->x, s->y,
               s->alive ? "alive" : "DEAD ",
               s->event_status == EVENT_STATUS_NONE ? "-" : event_name((EventType)s->event_status));
    }
}

static void inspect_window_cell(byte row, byte col)
{
    byte world_x = (byte)(current_origin_x() + col);
    byte world_y = (byte)(current_origin_y() + row);
    WorldTileInfo tile;

    world_get_tile_info(&world, world_x, world_y, &tile);

    printf("tile %X,%X -> world (%u,%u): terrain=%c travel=%u special=%s object=%s\n",
           row, col, tile.x, tile.y, terrain_glyph(tile.terrain), tile.travel_ease,
           tile.is_special_zone ? "yes" : "no", tile.has_object ? "yes" : "no");

    if (tile.object_index > 0) {
        printf("object #%u: %s at (%u,%u)",
               tile.object_index, object_type_name(tile.object.type), tile.object.x, tile.object.y);
        if (tile.object.type == OBJ_SETTLEMENT) printf(" size=%u", tile.object.data[0]);
        printf("\n");

        if (tile.object.type == OBJ_SETTLEMENT) {
            const Settlement *s = world_find_settlement_at(&world, tile.x, tile.y);
            if (s) {
                print_settlement_header();
                print_settlement(s);
            } else {
                printf("note: settlement object exists in the map, but no live settlement record is currently aligned to that tile\n");
            }
        }
    }
}

static void cmd_map(void)
{
    byte origin_x, origin_y, row, col;

    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }

    origin_x = map_window_origin(view_center_x);
    origin_y = map_window_origin(view_center_y);

    printf("    0123456789ABCDEF\n");
    for (row = 0; row < MAP_WINDOW_SIZE; row++) {
        printf("%X | ", row);
        for (col = 0; col < MAP_WINDOW_SIZE; col++) {
            WorldTileInfo tile;
            char glyph;

            world_get_tile_info(&world, (byte)(origin_x + col), (byte)(origin_y + row), &tile);
            if (row == cursor_row && col == cursor_col) {
                glyph = '@';
            } else {
                glyph = (char)glyph_for_tile(&tile);
            }

            putchar(glyph);
        }
        printf("\n");
    }

    printf("window origin=(%u,%u) center=(%u,%u)\n", origin_x, origin_y, view_center_x, view_center_y);
    printf("legend: .=water g=grass f=forest h=hills m=mountains d=desert w=swamp =road X=settlement\n");
    print_cursor_status();
}

static void cmd_center(byte x, byte y)
{
    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }
    view_center_x = clamp_view_center(x);
    view_center_y = clamp_view_center(y);
    sync_cursor_to_world(x, y);
    cmd_map();
}

static void cmd_pan(const char *dir, int amount)
{
    int dx = 0, dy = 0;

    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }
    if (amount < 1) amount = 1;

    if (strcasecmp(dir, "n") == 0 || strcasecmp(dir, "north") == 0) dy = -amount;
    else if (strcasecmp(dir, "s") == 0 || strcasecmp(dir, "south") == 0) dy = amount;
    else if (strcasecmp(dir, "e") == 0 || strcasecmp(dir, "east") == 0) dx = amount;
    else if (strcasecmp(dir, "w") == 0 || strcasecmp(dir, "west") == 0) dx = -amount;
    else {
        printf("unknown direction '%s' (use n/s/e/w)\n", dir);
        return;
    }

    view_center_x = clamp_view_center((int)view_center_x + dx);
    view_center_y = clamp_view_center((int)view_center_y + dy);
    sync_cursor_to_world(current_cursor_world_x(), current_cursor_world_y());
    cmd_map();
}

static void cmd_goto(byte id)
{
    const Settlement *s = world_get_settlement(&world, id);

    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }
    if (!s) { printf("no such settlement #%u\n", id); return; }

    view_center_x = clamp_view_center(s->x);
    view_center_y = clamp_view_center(s->y);
    sync_cursor_to_world(s->x, s->y);
    cmd_map();
}

static void move_cursor_step(int dx, int dy)
{
    int next_col = (int)cursor_col + dx;
    int next_row = (int)cursor_row + dy;

    if (next_col < 0) {
        view_center_x = clamp_view_center((int)view_center_x - 1);
        next_col = 0;
    } else if (next_col >= MAP_WINDOW_SIZE) {
        view_center_x = clamp_view_center((int)view_center_x + 1);
        next_col = MAP_WINDOW_SIZE - 1;
    }

    if (next_row < 0) {
        view_center_y = clamp_view_center((int)view_center_y - 1);
        next_row = 0;
    } else if (next_row >= MAP_WINDOW_SIZE) {
        view_center_y = clamp_view_center((int)view_center_y + 1);
        next_row = MAP_WINDOW_SIZE - 1;
    }

    cursor_col = clamp_window_index(next_col);
    cursor_row = clamp_window_index(next_row);
}

static void cmd_cursor(const char *row_arg, const char *col_arg)
{
    int row = parse_hex_nibble(row_arg);
    int col = parse_hex_nibble(col_arg);

    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }
    if (row < 0 || col < 0) {
        printf("usage: cursor <row-hex> <col-hex>\n");
        return;
    }

    cursor_row = (byte)row;
    cursor_col = (byte)col;
    cmd_map();
}

static void cmd_move(const char *dir, int amount)
{
    int dx = 0, dy = 0;
    int i;

    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }
    if (amount < 1) amount = 1;

    if (strcasecmp(dir, "n") == 0 || strcasecmp(dir, "north") == 0) dy = -1;
    else if (strcasecmp(dir, "s") == 0 || strcasecmp(dir, "south") == 0) dy = 1;
    else if (strcasecmp(dir, "e") == 0 || strcasecmp(dir, "east") == 0) dx = 1;
    else if (strcasecmp(dir, "w") == 0 || strcasecmp(dir, "west") == 0) dx = -1;
    else {
        printf("unknown direction '%s' (use n/s/e/w)\n", dir);
        return;
    }

    for (i = 0; i < amount; i++) move_cursor_step(dx, dy);
    cmd_map();
}

static void cmd_inspect(const char *row_arg, const char *col_arg)
{
    int row = parse_hex_nibble(row_arg);
    int col = parse_hex_nibble(col_arg);

    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }
    if (row < 0 || col < 0) {
        printf("usage: inspect <row-hex> <col-hex>   (0-F within the current 16x16 map window)\n");
        return;
    }

    inspect_window_cell((byte)row, (byte)col);
}

static void cmd_look(void)
{
    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }
    inspect_window_cell(cursor_row, cursor_col);
}

static int focus_from_name(const char *name)
{
    if (strcasecmp(name, "TRA") == 0) return CULTURE_TRA;
    if (strcasecmp(name, "AGR") == 0) return CULTURE_AGR;
    if (strcasecmp(name, "GRO") == 0) return CULTURE_GRO;
    if (strcasecmp(name, "SEC") == 0) return CULTURE_SEC;
    return -1;
}

static const char *STRUCT_NAMES[STRUCT_TYPE_COUNT] = {
    "dock", "warehouse", "fort", "townhall", "monument"
};

static int struct_type_from_name(const char *name)
{
    int i;
    for (i = 0; i < STRUCT_TYPE_COUNT; i++) {
        if (strcasecmp(name, STRUCT_NAMES[i]) == 0) return i;
    }
    return -1;
}

static void print_settlement(const Settlement *s)
{
    byte culture[CULTURE_COUNT];
    byte i;

    settlement_culture_vector(s, culture);

//    printf("#%-3u (%3u,%3u) %s  pop=%3u wealth=%3u reserve=%3u infra=%2u def=%3u  "
//           "culture[TRA=%3u AGR=%3u GRO=%3u SEC=%3u]  event=%s\n",
      printf("#%-3u (%3u,%3u) %s %3u %3u %3u %3u %3u "
           "%3u %3u %3u %3u %-12s : ",
           s->id, s->x, s->y, s->alive ? "alive" : "DEAD ",
           settlement_population_support(s), settlement_wealth_potential(s),
           settlement_reserve_potential(s), settlement_infrastructure_resilience(s),
           settlement_defense_posture(s),
           culture[CULTURE_TRA], culture[CULTURE_AGR], culture[CULTURE_GRO], culture[CULTURE_SEC],
           s->event_status == EVENT_STATUS_NONE ? "-" : event_name((EventType)s->event_status));

//    printf("    structures (capacity %u):", s->capacity);
    for (i = 0; i < MAX_STRUCTURE_SLOTS; i++) {
        if (s->structures[i].type == STRUCT_EMPTY || s->structures[i].type >= STRUCT_TYPE_COUNT) continue;
//        printf(" [%u]%s@%u", i, STRUCT_NAMES[s->structures[i].type], s->structures[i].condition);
        printf(" %c.%x", STRUCT_NAMES[s->structures[i].type][0]-32, s->structures[i].condition);
    }
    printf("\n");
}

static void cmd_load(const char *path)
{
    unsigned long seed = 1;
    if (world_load(&world, path, seed) != 0) {
        printf("error: could not load map '%s'\n", path);
        return;
    }
    world_ready = 1;
    reset_cursor();
    reset_view_center();
    printf("loaded %s: %u settlement(s), seed=%lu\n", path, world_settlement_count(&world), seed);
}

static void print_settlement_header() {
    puts("#ID  (xxx,yyy) stats pop wea res inf def TRA AGR GRO SEC event   structures                        ");
    puts("---------------------------------------------------------------------------------------------------");
}

static void cmd_list(void)
{
    word i;
    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }
    print_settlement_header();
    for (i = 0; i < world_settlement_count(&world); i++) {
        const Settlement *s = world_get_settlement_const(&world, (byte)i);
        if (s) print_settlement(s);
    }
}

static void cmd_show(byte id)
{
    Settlement *s = world_get_settlement(&world, id);
    if (!s) { printf("no such settlement #%u\n", id); return; }
    print_settlement_header();
    print_settlement(s);
}

static void cmd_tick(int n)
{
    int i;
    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }
    for (i = 0; i < n; i++) {
        world_tick(&world);
        present_drain_notes(&world); /* notes are per-tick -- drain before the next */
    }
    printf("advanced %d tick(s); world tick=%lu, event weather=%u%%\n",
           n, world.tick, world.event_chance_pct);
}

static int note_is_link(byte type)
{
    return type == NOTE_LINK_FORMED || type == NOTE_LINK_DISRUPTED ||
           type == NOTE_LINK_BLOCKED || type == NOTE_CARAVAN_ARRIVED;
}

/* Dump the raw Note buffer from the most recent tick / forced event -- the
   same data present_drain_notes() consumes, shown undecorated. */
static void cmd_notes(void)
{
    word i;
    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }
    if (world.note_count == 0) { printf("no notes from the last tick\n"); return; }
    for (i = 0; i < world.note_count; i++) {
        const Note *n = &world.notes[i];
        if (note_is_link(n->type)) {
            printf("  %-20s #%u <-> #%u", note_type_name((NoteType)n->type),
                   NOTE_LINK_FROM(n->ref), NOTE_LINK_TO(n->ref));
        } else {
            printf("  %-20s #%u", note_type_name((NoteType)n->type), n->ref);
        }
        if (n->type == NOTE_EVENT_STRUCK || n->type == NOTE_LINK_DISRUPTED) {
            printf(" (%s)", event_name((EventType)n->aux));
        }
        printf("\n");
    }
    if (world.notes_overflowed) printf("  ... buffer overflowed (MAX_NOTES=%d)\n", MAX_NOTES);
}

static void cmd_status(void)
{
    word alive, links_active, links_disrupted;
    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }
    alive = world_alive_settlement_count(&world);
    world_trade_link_status_counts(&world, &links_active, &links_disrupted);
    printf("tick=%lu  settlements=%u alive / %u total (baseline %u)  event weather=%u%% (range %d-%d%%)\n"
           "trade links=%u (%u active, %u disrupted)\n",
           world.tick, alive, world_settlement_count(&world), world.initial_settlement_count,
           world.event_chance_pct, EVENT_CHANCE_MIN, EVENT_CHANCE_MAX,
           world_trade_link_count(&world), links_active, links_disrupted);
}

static void cmd_event(const char *event_name_arg, byte id)
{
    int type;
    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }
    type = event_from_name(event_name_arg);
    if (type < 0) { printf("unknown event '%s'\n", event_name_arg); return; }
    if (!world_get_settlement(&world, id)) { printf("no such settlement #%u\n", id); return; }
    world_force_event(&world, id, (EventType)type);
    present_drain_notes(&world);
    printf("applied %s to #%u:\n", event_name(type), id);
    print_settlement_header();
    print_settlement(world_get_settlement(&world, id));
}

static void cmd_build(byte id, const char *type_arg)
{
    Settlement *s = world_get_settlement(&world, id);
    int type;
    int slot;
    if (!s) { printf("no such settlement #%u\n", id); return; }
    type = struct_type_from_name(type_arg);
    if (type < 0) { printf("unknown structure '%s' (use dock/warehouse/fort/townhall/monument)\n", type_arg); return; }
    slot = settlement_build(s, (StructureType)type);
    if (slot < 0) { printf("no empty slot within capacity\n"); return; }
    print_settlement_header();
    print_settlement(s);
}

static const char *link_type_name(byte type)
{
    switch (type) {
        case TRADE_LINK_CARAVAN: return "caravan";
        case TRADE_LINK_FLEET:   return "fleet";
        default:                 return "?";
    }
}

static void print_trade_link(const TradeLink *l)
{
    printf("#%-3u %-7s #%u <-> #%u  health=%3u throughput=%3u risk=%3u  flags=%s%s%s%s  last_event=%s\n",
           l->link_id, link_type_name(l->type), l->from_settlement_id, l->to_settlement_id,
           l->health, l->throughput, l->risk,
           (l->status_flags & TRADE_LINK_ACTIVE) ? "A" : "-",
           (l->status_flags & TRADE_LINK_DISRUPTED) ? "D" : "-",
           (l->status_flags & TRADE_LINK_BLOCKED) ? "B" : "-",
           (l->status_flags & TRADE_LINK_RECOVERING) ? "R" : "-",
           l->last_event_tag == TRADE_LINK_EVENT_NONE ? "-" : event_name((EventType)l->last_event_tag));
}

static void cmd_links(void)
{
    word i;
    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }
    if (world_trade_link_count(&world) == 0) { printf("no trade links yet\n"); return; }
    for (i = 0; i < world_trade_link_count(&world); i++) {
        const TradeLink *l = world_get_trade_link_const(&world, i);
        if (l) print_trade_link(l);
    }
}

static void cmd_nudge(byte id, const char *focus_arg)
{
    Settlement *s = world_get_settlement(&world, id);
    int focus;
    if (!s) { printf("no such settlement #%u\n", id); return; }
    focus = focus_from_name(focus_arg);
    if (focus < 0) { printf("unknown focus '%s' (use TRA/AGR/GRO/SEC)\n", focus_arg); return; }
    settlement_nudge_focus(s, (CultureFocus)focus);
    print_settlement_header();
    print_settlement(s);
}

static void print_help(void)
{
    printf(
        "commands:\n"
        "  load <path>                  load a .map file (e.g. ../src-prototype1/archipelago.map)\n"
        "  map                          draw a 16x16 ASCII window around the current view center\n"
        "  visible                      list settlement ids currently inside the 16x16 map window\n"
        "  center <x> <y>               move the map window center to world coordinates 0..255\n"
        "  goto <id>                    center the map window on a settlement id\n"
        "  pan <n|s|e|w> [n]            move the map window center by n tiles (default 1)\n"
        "  cursor <row> <col>           place the cursor at 0-F,0-F inside the current map window\n"
        "  move <n|s|e|w> [n]           move the cursor by n tiles, panning the window at edges\n"
        "  look                         inspect the tile under the current cursor\n"
        "  inspect <row> <col>          inspect tile 0-F,0-F inside the current map window\n"
        "  list                         list all settlements\n"
        "  show <id>                    show one settlement\n"
        "  tick [n]                     advance the simulation n ticks (default 1)\n"
        "  status                       show tick, settlement count, event weather\n"
        "  event <name> <id>            force an event (drought/plague/storm/pirates/market_crash/civil_war)\n"
        "  build <id> <structure>       build dock/warehouse/fort/townhall/monument in the first empty slot\n"
        "  nudge <id> <TRA|AGR|GRO|SEC> nudge a settlement toward a cultural focus (structural investment)\n"
        "  links                        list all trade links (health/throughput/risk/flags)\n"
        "  notes                        dump the transient-event buffer from the last tick/event\n"
        "  help                         show this text\n"
        "  quit                         exit\n");
}

int main(int argc, char **argv)
{
    char line[256];

    printf("Pirate Kingdoms simulation CLI. Type 'help' for commands.\n");
    present_init();
    if (argc > 1) cmd_load(argv[1]);

    while (fgets(line, sizeof(line), stdin)) {
        char *cmd = strtok(line, " \t\r\n");
        if (!cmd) continue;

        if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            break;
        } else if (strcmp(cmd, "help") == 0) {
            print_help();
        } else if (strcmp(cmd, "load") == 0) {
            char *path = strtok(NULL, " \t\r\n");
            if (!path) { printf("usage: load <path>\n"); continue; }
            cmd_load(path);
        } else if (strcmp(cmd, "map") == 0) {
            cmd_map();
        } else if (strcmp(cmd, "visible") == 0) {
            cmd_visible();
        } else if (strcmp(cmd, "center") == 0) {
            char *x = strtok(NULL, " \t\r\n");
            char *y = strtok(NULL, " \t\r\n");
            if (!x || !y) { printf("usage: center <x> <y>\n"); continue; }
            cmd_center((byte)atoi(x), (byte)atoi(y));
        } else if (strcmp(cmd, "goto") == 0) {
            char *id = strtok(NULL, " \t\r\n");
            if (!id) { printf("usage: goto <id>\n"); continue; }
            cmd_goto((byte)atoi(id));
        } else if (strcmp(cmd, "pan") == 0) {
            char *dir = strtok(NULL, " \t\r\n");
            char *n = strtok(NULL, " \t\r\n");
            if (!dir) { printf("usage: pan <n|s|e|w> [n]\n"); continue; }
            cmd_pan(dir, n ? atoi(n) : 1);
        } else if (strcmp(cmd, "cursor") == 0) {
            char *row = strtok(NULL, " \t\r\n");
            char *col = strtok(NULL, " \t\r\n");
            if (!row || !col) { printf("usage: cursor <row-hex> <col-hex>\n"); continue; }
            cmd_cursor(row, col);
        } else if (strcmp(cmd, "move") == 0) {
            char *dir = strtok(NULL, " \t\r\n");
            char *n = strtok(NULL, " \t\r\n");
            if (!dir) { printf("usage: move <n|s|e|w> [n]\n"); continue; }
            cmd_move(dir, n ? atoi(n) : 1);
        } else if (strcmp(cmd, "look") == 0) {
            cmd_look();
        } else if (strcmp(cmd, "inspect") == 0) {
            char *row = strtok(NULL, " \t\r\n");
            char *col = strtok(NULL, " \t\r\n");
            if (!row || !col) { printf("usage: inspect <row-hex> <col-hex>\n"); continue; }
            cmd_inspect(row, col);
        } else if (strcmp(cmd, "list") == 0) {
            cmd_list();
        } else if (strcmp(cmd, "status") == 0) {
            cmd_status();
        } else if (strcmp(cmd, "show") == 0) {
            char *id = strtok(NULL, " \t\r\n");
            if (!id) { printf("usage: show <id>\n"); continue; }
            cmd_show((byte)atoi(id));
        } else if (strcmp(cmd, "tick") == 0) {
            char *n = strtok(NULL, " \t\r\n");
            cmd_tick(n ? atoi(n) : 1);
        } else if (strcmp(cmd, "event") == 0) {
            char *name = strtok(NULL, " \t\r\n");
            char *id = strtok(NULL, " \t\r\n");
            if (!name || !id) { printf("usage: event <name> <id>\n"); continue; }
            cmd_event(name, (byte)atoi(id));
        } else if (strcmp(cmd, "build") == 0) {
            char *id = strtok(NULL, " \t\r\n");
            char *type = strtok(NULL, " \t\r\n");
            if (!id || !type) { printf("usage: build <id> <structure>\n"); continue; }
            cmd_build((byte)atoi(id), type);
        } else if (strcmp(cmd, "nudge") == 0) {
            char *id = strtok(NULL, " \t\r\n");
            char *focus = strtok(NULL, " \t\r\n");
            if (!id || !focus) { printf("usage: nudge <id> <TRA|AGR|GRO|SEC>\n"); continue; }
            cmd_nudge((byte)atoi(id), focus);
        } else if (strcmp(cmd, "links") == 0) {
            cmd_links();
        } else if (strcmp(cmd, "notes") == 0) {
            cmd_notes();
        } else {
            printf("unknown command '%s' (try 'help')\n", cmd);
        }
    }

    return 0;
}
