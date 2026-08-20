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

static void print_settlement(const Settlement *s)
{
    printf("#%-3u (%3u,%3u) %s  pop=%2u wealth=%2u reserves=%2u infra=%2u def=%2u  "
           "culture[TRA=%3u AGR=%3u GRO=%3u SEC=%3u]  event=%s\n",
           s->id, s->x, s->y, s->alive ? "alive" : "DEAD ",
           s->population, s->wealth, s->reserves, s->infrastructure, s->defense,
           s->culture[CULTURE_TRA], s->culture[CULTURE_AGR],
           s->culture[CULTURE_GRO], s->culture[CULTURE_SEC],
           s->event_status == EVENT_STATUS_NONE ? "-" : event_name((EventType)s->event_status));
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

static void cmd_list(void)
{
    word i;
    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }
    for (i = 0; i < world.settlement_count; i++) print_settlement(&world.settlements[i]);
}

static void cmd_show(byte id)
{
    Settlement *s = world_get_settlement(&world, id);
    if (!s) { printf("no such settlement #%u\n", id); return; }
    print_settlement(s);
}

static void cmd_tick(int n)
{
    int i;
    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }
    for (i = 0; i < n; i++) world_tick(&world);
    printf("advanced %d tick(s); world tick=%lu, event weather=%u%%\n",
           n, world.tick, world.event_chance_pct);
}

static void cmd_status(void)
{
    word i, alive = 0;
    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }
    for (i = 0; i < world.settlement_count; i++) {
        if (world.settlements[i].alive) alive++;
    }
    printf("tick=%lu  settlements=%u alive / %u total (baseline %u)  event weather=%u%% (range %d-%d%%)\n",
           world.tick, alive, world.settlement_count, world.initial_settlement_count,
           world.event_chance_pct, EVENT_CHANCE_MIN, EVENT_CHANCE_MAX);
}

static void cmd_event(const char *event_name_arg, byte id)
{
    int type;
    if (!world_ready) { printf("no world loaded (use: load <path>)\n"); return; }
    type = event_from_name(event_name_arg);
    if (type < 0) { printf("unknown event '%s'\n", event_name_arg); return; }
    if (!world_get_settlement(&world, id)) { printf("no such settlement #%u\n", id); return; }
    world_force_event(&world, id, (EventType)type);
    printf("applied %s to #%u:\n", event_name(type), id);
    print_settlement(world_get_settlement(&world, id));
}

static void cmd_setfocus(byte id, const char *focus_arg, int value)
{
    Settlement *s = world_get_settlement(&world, id);
    int focus;
    if (!s) { printf("no such settlement #%u\n", id); return; }
    focus = focus_from_name(focus_arg);
    if (focus < 0) { printf("unknown focus '%s' (use TRA/AGR/GRO/SEC)\n", focus_arg); return; }
    settlement_shift_culture(s, (CultureFocus)focus, value);
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
        "  setfocus <id> <TRA|AGR|GRO|SEC> <delta>   shift a settlement's cultural focus\n"
        "  help                         show this text\n"
        "  quit                         exit\n");
}

int main(int argc, char **argv)
{
    char line[256];

    printf("Pirate Kingdoms simulation CLI. Type 'help' for commands.\n");
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
        } else if (strcmp(cmd, "setfocus") == 0) {
            char *id = strtok(NULL, " \t\r\n");
            char *focus = strtok(NULL, " \t\r\n");
            char *delta = strtok(NULL, " \t\r\n");
            if (!id || !focus || !delta) { printf("usage: setfocus <id> <TRA|AGR|GRO|SEC> <delta>\n"); continue; }
            cmd_setfocus((byte)atoi(id), focus, atoi(delta));
        } else {
            printf("unknown command '%s' (try 'help')\n", cmd);
        }
    }

    return 0;
}
