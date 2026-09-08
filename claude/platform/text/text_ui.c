/* Host-side CLI/debug harness for the Pirate Kingdoms simulation engine.
   This is deliberately host-only (stdio, malloc-free but interactive) --
   it is NOT the X16 UI, just the "text based" adapter the retro-beast-mode
   agent notes asked for so the engine can be exercised without hardware. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> /* strcasecmp: host CLI only, not used by the engine */

#include "../../engine/common.h"
#include "../../engine/world.h"
#include "../../engine/calendar.h"
#include "../../engine/game_rules.h"
#include "../../present/present.h"

#include "text_actions.h"
#include "text_commands.h"
#include "text_dispatch.h"
#include "text_render.h"
#include "text_ui.h"

#define MAP_WINDOW_SIZE 16

enum CliMode {
    CLI_MODE_DEBUG = 0,
    CLI_MODE_GAME  = 1
};

static World world;
static byte world_ready = 0;
static byte view_center_x = MAP_WIDTH / 2;
static byte view_center_y = MAP_HEIGHT / 2;
static byte cursor_row = MAP_WINDOW_SIZE / 2;
static byte cursor_col = MAP_WINDOW_SIZE / 2;
static int current_mode = CLI_MODE_DEBUG;
static word game_subtick = 0;
#define GAME_TICK_THRESHOLD 240
static GameRulesState game_state;

static void print_game_help(void);

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

static int glyph_for_tile(const WorldTileInfo *tile)
{
    if (world_find_settlement_at(&world, tile->x, tile->y)) return 'X';
    if (tile->object_index > 0) return text_object_glyph(tile->object.type);
    if (tile->travel_ease == 3) return '=';
    return text_terrain_glyph(tile->terrain);
}

static void print_cursor_status(void)
{
    WorldTileInfo tile;

    world_get_tile_info(&world, current_cursor_world_x(), current_cursor_world_y(), &tile);
    printf("cursor=(%X,%X) world=(%u,%u) tile=%c",
           cursor_row, cursor_col, tile.x, tile.y, glyph_for_tile(&tile));
    if (tile.object_index > 0) {
        printf(" object=%s#%u", text_object_type_name(tile.object.type), tile.object_index);
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
            row, col, tile.x, tile.y, text_terrain_glyph(tile.terrain), tile.travel_ease,
           tile.is_special_zone ? "yes" : "no", tile.has_object ? "yes" : "no");

    if (tile.object_index > 0) {
        printf("object #%u: %s at (%u,%u)",
               tile.object_index, text_object_type_name(tile.object.type), tile.object.x, tile.object.y);
        if (tile.object.type == OBJ_SETTLEMENT) printf(" size=%u", tile.object.data[0]);
        printf("\n");

        if (tile.object.type == OBJ_SETTLEMENT) {
            const Settlement *s = world_find_settlement_at(&world, tile.x, tile.y);
            if (s) {
                text_print_settlement_header();
                text_print_settlement(s);
            } else {
                printf("note: settlement object exists in the map, but no live settlement record is currently aligned to that tile\n");
            }
        }
    } else {
        const Settlement *s = world_find_settlement_at(&world, tile.x, tile.y);
        if (s) {
            printf("dynamic settlement #%u at (%u,%u)\n", s->id, s->x, s->y);
            text_print_settlement_header();
            text_print_settlement(s);
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

static void recenter_view_on_cursor(void)
{
    byte world_x = current_cursor_world_x();
    byte world_y = current_cursor_world_y();

    view_center_x = clamp_view_center(world_x);
    view_center_y = clamp_view_center(world_y);
    sync_cursor_to_world(world_x, world_y);
}

static void cmd_cursor(const char *row_arg, const char *col_arg)
{
    int row = text_parse_hex_nibble(row_arg, MAP_WINDOW_SIZE);
    int col = text_parse_hex_nibble(col_arg, MAP_WINDOW_SIZE);

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
    int row = text_parse_hex_nibble(row_arg, MAP_WINDOW_SIZE);
    int col = text_parse_hex_nibble(col_arg, MAP_WINDOW_SIZE);

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

static void cmd_list(void)
{
    word i;
    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }
    text_print_settlement_header();
    for (i = 0; i < world_settlement_count(&world); i++) {
        const Settlement *s = world_get_settlement_const(&world, (byte)i);
        if (s) text_print_settlement(s);
    }
}

static void cmd_show(byte id)
{
    Settlement *s = world_get_settlement(&world, id);
    if (!s) { printf("no such settlement #%u\n", id); return; }
    text_print_settlement_header();
    text_print_settlement(s);
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
    char date[32];

    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }
    alive = world_alive_settlement_count(&world);
    world_trade_link_status_counts(&world, &links_active, &links_disrupted);
    calendar_format(&world.calendar, date, sizeof(date));
            printf("tick=%lu  date=%s  subtick=%u/%d  supplies=%d rep=%d  settlements=%u alive / %u total (baseline %u)  event weather=%u%% (range %d-%d%%)\n"
           "trade links=%u (%u active, %u disrupted)\n",
                world.tick, date, game_subtick, GAME_TICK_THRESHOLD, game_state.supplies, game_state.reputation,
           alive, world_settlement_count(&world), world.initial_settlement_count,
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
    text_print_settlement_header();
    text_print_settlement(world_get_settlement(&world, id));
}

static void cmd_build(byte id, const char *type_arg)
{
    Settlement *s = world_get_settlement(&world, id);
    int type;
    int slot;
    if (!s) { printf("no such settlement #%u\n", id); return; }
    type = text_struct_type_from_name(type_arg);
    if (type < 0) { printf("unknown structure '%s' (use dock/warehouse/fort/townhall/monument)\n", type_arg); return; }
    slot = settlement_build(s, (StructureType)type);
    if (slot < 0) { printf("no empty slot within capacity\n"); return; }
    text_print_settlement_header();
    text_print_settlement(s);
}

static void cmd_links(void)
{
    word i;
    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }
    if (world_trade_link_count(&world) == 0) { printf("no trade links yet\n"); return; }
    for (i = 0; i < world_trade_link_count(&world); i++) {
        const TradeLink *l = world_get_trade_link_const(&world, i);
        if (l) text_print_trade_link(l);
    }
}

static void cmd_nudge(byte id, const char *focus_arg)
{
    Settlement *s = world_get_settlement(&world, id);
    int focus;
    if (!s) { printf("no such settlement #%u\n", id); return; }
    focus = text_focus_from_name(focus_arg);
    if (focus < 0) { printf("unknown focus '%s' (use TRA/AGR/GRO/SEC)\n", focus_arg); return; }
    settlement_nudge_focus(s, (CultureFocus)focus);
    text_print_settlement_header();
    text_print_settlement(s);
}

static word terrain_subtick_cost(byte travel_ease)
{
    if (travel_ease >= 3) return 40; /* road / higher ease */
    return 60; /* rough land */
}

static void advance_game_subtick_for_move(void)
{
    WorldTileInfo tile;
    word cost;

    world_get_tile_info(&world, current_cursor_world_x(), current_cursor_world_y(), &tile);
    cost = terrain_subtick_cost(tile.travel_ease);
    game_subtick += cost;

    while (game_subtick >= GAME_TICK_THRESHOLD) {
        world_tick(&world);
        present_drain_notes(&world);
        game_subtick -= GAME_TICK_THRESHOLD;
    }
}

static void cmd_game_move(const char *dir, int amount)
{
    int dx = 0, dy = 0;
    int i;
    int moved = 0;
    char date[32];
    WorldTileInfo tile;
    word last_cost = 0;
    word total_cost = 0;

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

    for (i = 0; i < amount; i++) {
        byte x, y;

        if (!game_rules_try_spend_move(&game_state)) {
            printf("out of supplies: reach a settlement and use 'levy'\n");
            break;
        }

        move_cursor_step(dx, dy);
        recenter_view_on_cursor();
        x = current_cursor_world_x();
        y = current_cursor_world_y();
        world_get_tile_info(&world, x, y, &tile);
        last_cost = terrain_subtick_cost(tile.travel_ease);
        total_cost += last_cost;
        advance_game_subtick_for_move();
        moved++;

        printf("move %s -> (%u,%u) %s (%s, cost %u), subtick=%u/%d\n",
             dir, x, y, text_terrain_name(tile.terrain), text_travel_ease_name(tile.travel_ease),
               last_cost, game_subtick, GAME_TICK_THRESHOLD);
    }

    calendar_format(&world.calendar, date, sizeof(date));
    printf("summary: moved %s %d/%d step(s), total cost=%u, tick=%lu date=%s subtick=%u/%d supplies=%d rep=%d\n",
           dir, moved, amount, total_cost, world.tick, date, game_subtick, GAME_TICK_THRESHOLD,
            game_state.supplies, game_state.reputation);
    if (world.note_count > 0) {
        cmd_notes();
    }
    cmd_map();
}

static void cmd_game_aid(void)
{
    const Settlement *cursor_settlement;
    Settlement *s;
    GameRuleResult result;
    byte id;

    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }
    cursor_settlement = world_find_settlement_at(&world, current_cursor_world_x(), current_cursor_world_y());
    if (!cursor_settlement || !cursor_settlement->alive) {
        printf("aid requires the cursor to be on a live settlement tile\n");
        return;
    }

    id = cursor_settlement->id;
    result = game_rules_apply_aid(&world, &game_state, id);
    if (result == GAME_RULE_NOT_ENOUGH_SUPPLIES) {
        printf("not enough supplies: need %d, have %d\n", GAME_AID_SUPPLY_COST, game_state.supplies);
        return;
    } else if (result != GAME_RULE_OK) {
        printf("aid could not be applied\n");
        return;
    }

    present_drain_notes(&world);
    s = world_get_settlement(&world, id);

    printf("aided settlement #%u: supplies=%d rep=%d tick=%lu\n",
           id, game_state.supplies, game_state.reputation, world.tick);
    text_print_settlement_header();
    if (s) text_print_settlement(s);
    if (world.note_count > 0) cmd_notes();
}

static void cmd_game_levy(void)
{
    const Settlement *cursor_settlement;
    Settlement *s;
    GameRuleResult result;
    byte id;

    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }
    cursor_settlement = world_find_settlement_at(&world, current_cursor_world_x(), current_cursor_world_y());
    if (!cursor_settlement || !cursor_settlement->alive) {
        printf("levy requires the cursor to be on a live settlement tile\n");
        return;
    }

    id = cursor_settlement->id;
    result = game_rules_apply_levy(&world, &game_state, id);
    if (result != GAME_RULE_OK) {
        printf("levy could not be applied\n");
        return;
    }

    present_drain_notes(&world);
    s = world_get_settlement(&world, id);

    printf("levied settlement #%u: +%d supplies -> %d, rep=%d, tick=%lu\n",
           id, GAME_LEVY_SUPPLY_GAIN, game_state.supplies, game_state.reputation, world.tick);
    text_print_settlement_header();
    if (s) text_print_settlement(s);
    if (world.note_count > 0) cmd_notes();
}

static void cmd_game_found(void)
{
    byte x, y;
    byte new_id = 0;
    GameRuleResult result;
    Settlement *s;

    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }

    x = current_cursor_world_x();
    y = current_cursor_world_y();
    result = game_rules_apply_found(&world, &game_state, x, y, 0, &new_id);

    if (result == GAME_RULE_NOT_ENOUGH_SUPPLIES) {
        printf("not enough supplies: need %d, have %d\n", GAME_FOUND_SUPPLY_COST, game_state.supplies);
        return;
    }
    if (result == GAME_RULE_BAD_TERRAIN) {
        printf("cannot found on water\n");
        return;
    }
    if (result == GAME_RULE_TILE_BLOCKED) {
        printf("cannot found here: tile already occupied or blocked\n");
        return;
    }
    if (result == GAME_RULE_TABLE_FULL) {
        printf("cannot found: settlement table is full\n");
        return;
    }
    if (result != GAME_RULE_OK) {
        printf("found could not be applied\n");
        return;
    }

    present_drain_notes(&world);
    s = world_get_settlement(&world, new_id);
    printf("founded settlement #%u at (%u,%u): supplies=%d rep=%d tick=%lu\n",
           new_id, x, y, game_state.supplies, game_state.reputation, world.tick);
    text_print_settlement_header();
    if (s) text_print_settlement(s);
    if (world.note_count > 0) cmd_notes();
}

static void set_mode(int mode)
{
    current_mode = mode;
    if (mode == CLI_MODE_GAME) {
        game_subtick = 0;
        game_rules_init(&game_state);
        printf("entered game mode. use 'debug' to return to debug mode.\n");
        print_game_help();
    } else {
        printf("entered debug mode.\n");
    }
}

static void print_game_help(void)
{
    printf(
        "game mode:\n"
        "  move <n|s|e|w> [n]      recenter view, add terrain cost; tick fires at 240\n"
        "  found                   found a new settlement at the cursor\n"
        "  aid                     spend supplies to strengthen this settlement\n"
        "  levy                    gain supplies by straining this settlement\n"
        "  look                    inspect the tile under the cursor\n"
        "  map                     show the current 16x16 map window\n"
        "  tick [n]                advance the world n ticks\n"
        "  status                  show date + simulation status + supplies\n"
        "  notes                   dump notes from the last tick\n"
        "  debug                   return to debug mode\n"
        "  help                    show this text\n"
        "  quit                    exit the program\n");
}

static void print_help(void)
{
    const char *left[] = {
        "load <path>",
        "map",
        "visible",
        "center <x> <y>",
        "goto <id>",
        "pan <n|s|e|w> [n]",
        "cursor <row> <col>",
        "move <n|s|e|w> [n]",
        "look",
        "inspect <row> <col>",
        "list",
        "show <id>",
        "event <name> <id>",
        "build <id> <structure>",
        "nudge <id> <TRA|AGR|GRO|SEC>",
        "links",
        "tick [n]",
        "status",
        "notes",
        "mode game",
        "mode debug",
        "help",
        "quit"
    };
    const char *right[] = {
        "load a .map file",
        "draw the map window",
        "show visible settlements",
        "center the view",
        "jump to a settlement",
        "pan the map window",
        "place the cursor",
        "move the cursor",
        "inspect the tile under the cursor",
        "inspect a tile in-window",
        "list all settlements",
        "show one settlement",
        "force an event",
        "build a structure",
        "nudge a focus",
        "show trade links",
        "advance the world",
        "show tick + weather summary",
        "dump recent notes",
        "enter game mode",
        "return to debug mode",
        "show this help",
        "exit the CLI"
    };
    int i;

    printf("commands:\n");
    printf("  %-32s %-32s\n", "map / view", "world / simulation");
    printf("  %-32s %-32s\n", "--------------------------------", "--------------------------------");
    for (i = 0; i < 10; i++) {
        printf("  %-32s %-32s\n", left[i], right[i]);
    }
    printf("\n");
    printf("  %-32s %-32s\n", "settlements", "mode / help");
    printf("  %-32s %-32s\n", "--------------------------------", "--------------------------------");
    for (i = 10; i < 22; i++) {
        printf("  %-32s %-32s\n", left[i], right[i]);
    }
    printf("\n");
}

static int ui_get_mode(void *ctx)
{
    (void)ctx;
    return current_mode;
}

static void ui_set_mode(void *ctx, int mode)
{
    (void)ctx;
    set_mode(mode);
}

static void ui_print_help(void *ctx)
{
    (void)ctx;
    print_help();
}

static void ui_print_game_help(void *ctx)
{
    (void)ctx;
    print_game_help();
}

static void ui_cmd_load(void *ctx, const char *path)
{
    (void)ctx;
    cmd_load(path);
}

static void ui_cmd_map(void *ctx)
{
    (void)ctx;
    cmd_map();
}

static void ui_cmd_visible(void *ctx)
{
    (void)ctx;
    cmd_visible();
}

static void ui_cmd_center(void *ctx, byte x, byte y)
{
    (void)ctx;
    cmd_center(x, y);
}

static void ui_cmd_goto(void *ctx, byte id)
{
    (void)ctx;
    cmd_goto(id);
}

static void ui_cmd_pan(void *ctx, const char *dir, int amount)
{
    (void)ctx;
    cmd_pan(dir, amount);
}

static void ui_cmd_cursor(void *ctx, const char *row, const char *col)
{
    (void)ctx;
    cmd_cursor(row, col);
}

static void ui_cmd_move(void *ctx, const char *dir, int amount)
{
    (void)ctx;
    cmd_move(dir, amount);
}

static void ui_cmd_game_move(void *ctx, const char *dir, int amount)
{
    (void)ctx;
    cmd_game_move(dir, amount);
}

static void ui_cmd_game_found(void *ctx)
{
    (void)ctx;
    cmd_game_found();
}

static void ui_cmd_game_aid(void *ctx)
{
    (void)ctx;
    cmd_game_aid();
}

static void ui_cmd_game_levy(void *ctx)
{
    (void)ctx;
    cmd_game_levy();
}

static void ui_cmd_look(void *ctx)
{
    (void)ctx;
    cmd_look();
}

static void ui_cmd_inspect(void *ctx, const char *row, const char *col)
{
    (void)ctx;
    cmd_inspect(row, col);
}

static void ui_cmd_list(void *ctx)
{
    (void)ctx;
    cmd_list();
}

static void ui_cmd_status(void *ctx)
{
    (void)ctx;
    cmd_status();
}

static void ui_cmd_show(void *ctx, byte id)
{
    (void)ctx;
    cmd_show(id);
}

static void ui_cmd_tick(void *ctx, int n)
{
    (void)ctx;
    cmd_tick(n);
}

static void ui_cmd_event(void *ctx, const char *name, byte id)
{
    (void)ctx;
    cmd_event(name, id);
}

static void ui_cmd_build(void *ctx, byte id, const char *type)
{
    (void)ctx;
    cmd_build(id, type);
}

static void ui_cmd_nudge(void *ctx, byte id, const char *focus)
{
    (void)ctx;
    cmd_nudge(id, focus);
}

static void ui_cmd_links(void *ctx)
{
    (void)ctx;
    cmd_links();
}

static void ui_cmd_notes(void *ctx)
{
    (void)ctx;
    cmd_notes();
}

int text_ui_run(int argc, char **argv)
{
    char line[256];
    TextDispatchHandlers dispatch;
    TextActionApi action_api;

    action_api.ctx = NULL;
    action_api.get_mode = ui_get_mode;
    action_api.set_mode = ui_set_mode;
    action_api.print_help = ui_print_help;
    action_api.print_game_help = ui_print_game_help;
    action_api.cmd_load = ui_cmd_load;
    action_api.cmd_map = ui_cmd_map;
    action_api.cmd_visible = ui_cmd_visible;
    action_api.cmd_center = ui_cmd_center;
    action_api.cmd_goto = ui_cmd_goto;
    action_api.cmd_pan = ui_cmd_pan;
    action_api.cmd_cursor = ui_cmd_cursor;
    action_api.cmd_move = ui_cmd_move;
    action_api.cmd_game_move = ui_cmd_game_move;
    action_api.cmd_game_found = ui_cmd_game_found;
    action_api.cmd_game_aid = ui_cmd_game_aid;
    action_api.cmd_game_levy = ui_cmd_game_levy;
    action_api.cmd_look = ui_cmd_look;
    action_api.cmd_inspect = ui_cmd_inspect;
    action_api.cmd_list = ui_cmd_list;
    action_api.cmd_status = ui_cmd_status;
    action_api.cmd_show = ui_cmd_show;
    action_api.cmd_tick = ui_cmd_tick;
    action_api.cmd_event = ui_cmd_event;
    action_api.cmd_build = ui_cmd_build;
    action_api.cmd_nudge = ui_cmd_nudge;
    action_api.cmd_links = ui_cmd_links;
    action_api.cmd_notes = ui_cmd_notes;

    text_actions_build_dispatch(&action_api, &dispatch);

    printf("Pirate Kingdoms simulation CLI. Type 'help' for commands.\n");
    present_init();
    if (argc > 1) cmd_load(argv[1]);

    while (fgets(line, sizeof(line), stdin)) {
        if (!text_dispatch_execute(line, &dispatch)) break;
    }

    return 0;
}
