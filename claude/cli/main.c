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

static World world;
static byte world_ready = 0;

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
    printf("loaded %s: %u settlement(s), seed=%lu\n", path, world.settlement_count, seed);
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
    for (i = 0; i < world.settlement_count; i++) print_settlement(&world.settlements[i]);
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
    word i, alive = 0, links_active = 0, links_disrupted = 0;
    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }
    for (i = 0; i < world.settlement_count; i++) {
        if (world.settlements[i].alive) alive++;
    }
    for (i = 0; i < world.trade_link_count; i++) {
        byte flags = world.trade_links[i].status_flags;
        if (flags & TRADE_LINK_ACTIVE) links_active++;
        if (flags & TRADE_LINK_DISRUPTED) links_disrupted++;
    }
    printf("tick=%lu  settlements=%u alive / %u total (baseline %u)  event weather=%u%% (range %d-%d%%)\n"
           "trade links=%u (%u active, %u disrupted)\n",
           world.tick, alive, world.settlement_count, world.initial_settlement_count,
           world.event_chance_pct, EVENT_CHANCE_MIN, EVENT_CHANCE_MAX,
           world.trade_link_count, links_active, links_disrupted);
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
    if (world.trade_link_count == 0) { printf("no trade links yet\n"); return; }
    for (i = 0; i < world.trade_link_count; i++) print_trade_link(&world.trade_links[i]);
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
