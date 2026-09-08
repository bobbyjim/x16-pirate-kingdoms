#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "text_actions.h"

static const TextActionApi *g_api;

static void on_mode(void *ctx, const char *mode_arg)
{
    (void)ctx;
    if (!mode_arg) { printf("usage: mode <debug|game>\n"); return; }
    if (strcasecmp(mode_arg, "game") == 0) {
        g_api->set_mode(g_api->ctx, TEXT_MODE_GAME);
    } else if (strcasecmp(mode_arg, "debug") == 0) {
        g_api->set_mode(g_api->ctx, TEXT_MODE_DEBUG);
    } else {
        printf("unknown mode '%s' (use debug or game)\n", mode_arg);
    }
}

static void on_debug(void *ctx)
{
    (void)ctx;
    g_api->set_mode(g_api->ctx, TEXT_MODE_DEBUG);
}

static void on_game(void *ctx)
{
    (void)ctx;
    g_api->set_mode(g_api->ctx, TEXT_MODE_GAME);
}

static void on_help(void *ctx)
{
    (void)ctx;
    if (g_api->get_mode(g_api->ctx) == TEXT_MODE_GAME) g_api->print_game_help(g_api->ctx);
    else g_api->print_help(g_api->ctx);
}

static void on_load(void *ctx, const char *path)
{
    (void)ctx;
    if (!path) { printf("usage: load <path>\n"); return; }
    g_api->cmd_load(g_api->ctx, path);
}

static void on_map(void *ctx) { (void)ctx; g_api->cmd_map(g_api->ctx); }
static void on_visible(void *ctx) { (void)ctx; g_api->cmd_visible(g_api->ctx); }

static void on_center(void *ctx, const char *x, const char *y)
{
    (void)ctx;
    if (!x || !y) { printf("usage: center <x> <y>\n"); return; }
    g_api->cmd_center(g_api->ctx, (byte)atoi(x), (byte)atoi(y));
}

static void on_goto(void *ctx, const char *id)
{
    (void)ctx;
    if (!id) { printf("usage: goto <id>\n"); return; }
    g_api->cmd_goto(g_api->ctx, (byte)atoi(id));
}

static void on_pan(void *ctx, const char *dir, const char *n)
{
    (void)ctx;
    if (!dir) { printf("usage: pan <n|s|e|w> [n]\n"); return; }
    g_api->cmd_pan(g_api->ctx, dir, n ? atoi(n) : 1);
}

static void on_cursor(void *ctx, const char *row, const char *col)
{
    (void)ctx;
    if (!row || !col) { printf("usage: cursor <row-hex> <col-hex>\n"); return; }
    g_api->cmd_cursor(g_api->ctx, row, col);
}

static void on_move(void *ctx, const char *dir, const char *n)
{
    (void)ctx;
    if (!dir) { printf("usage: move <n|s|e|w> [n]\n"); return; }
    if (g_api->get_mode(g_api->ctx) == TEXT_MODE_GAME) {
        g_api->cmd_game_move(g_api->ctx, dir, n ? atoi(n) : 1);
    } else {
        g_api->cmd_move(g_api->ctx, dir, n ? atoi(n) : 1);
    }
}

static void on_found(void *ctx)
{
    (void)ctx;
    if (g_api->get_mode(g_api->ctx) != TEXT_MODE_GAME) {
        printf("'found' is game-mode only (use: mode game)\n");
    } else {
        g_api->cmd_game_found(g_api->ctx);
    }
}

static void on_aid(void *ctx)
{
    (void)ctx;
    if (g_api->get_mode(g_api->ctx) != TEXT_MODE_GAME) {
        printf("'aid' is game-mode only (use: mode game)\n");
    } else {
        g_api->cmd_game_aid(g_api->ctx);
    }
}

static void on_levy(void *ctx)
{
    (void)ctx;
    if (g_api->get_mode(g_api->ctx) != TEXT_MODE_GAME) {
        printf("'levy' is game-mode only (use: mode game)\n");
    } else {
        g_api->cmd_game_levy(g_api->ctx);
    }
}

static void on_look(void *ctx) { (void)ctx; g_api->cmd_look(g_api->ctx); }

static void on_inspect(void *ctx, const char *row, const char *col)
{
    (void)ctx;
    if (!row || !col) { printf("usage: inspect <row-hex> <col-hex>\n"); return; }
    g_api->cmd_inspect(g_api->ctx, row, col);
}

static void on_list(void *ctx) { (void)ctx; g_api->cmd_list(g_api->ctx); }
static void on_status(void *ctx) { (void)ctx; g_api->cmd_status(g_api->ctx); }

static void on_show(void *ctx, const char *id)
{
    (void)ctx;
    if (!id) { printf("usage: show <id>\n"); return; }
    g_api->cmd_show(g_api->ctx, (byte)atoi(id));
}

static void on_tick(void *ctx, const char *n)
{
    (void)ctx;
    g_api->cmd_tick(g_api->ctx, n ? atoi(n) : 1);
}

static void on_event(void *ctx, const char *name, const char *id)
{
    (void)ctx;
    if (!name || !id) { printf("usage: event <name> <id>\n"); return; }
    g_api->cmd_event(g_api->ctx, name, (byte)atoi(id));
}

static void on_build(void *ctx, const char *id, const char *type)
{
    (void)ctx;
    if (!id || !type) { printf("usage: build <id> <structure>\n"); return; }
    g_api->cmd_build(g_api->ctx, (byte)atoi(id), type);
}

static void on_nudge(void *ctx, const char *id, const char *focus)
{
    (void)ctx;
    if (!id || !focus) { printf("usage: nudge <id> <TRA|AGR|GRO|SEC>\n"); return; }
    g_api->cmd_nudge(g_api->ctx, (byte)atoi(id), focus);
}

static void on_links(void *ctx) { (void)ctx; g_api->cmd_links(g_api->ctx); }
static void on_notes(void *ctx) { (void)ctx; g_api->cmd_notes(g_api->ctx); }

static void on_unknown(void *ctx, const char *cmd)
{
    (void)ctx;
    printf("unknown command '%s' (try 'help')\n", cmd);
}

void text_actions_build_dispatch(const TextActionApi *api, TextDispatchHandlers *handlers)
{
    g_api = api;

    handlers->ctx = NULL;
    handlers->on_mode = on_mode;
    handlers->on_debug = on_debug;
    handlers->on_game = on_game;
    handlers->on_help = on_help;
    handlers->on_load = on_load;
    handlers->on_map = on_map;
    handlers->on_visible = on_visible;
    handlers->on_center = on_center;
    handlers->on_goto = on_goto;
    handlers->on_pan = on_pan;
    handlers->on_cursor = on_cursor;
    handlers->on_move = on_move;
    handlers->on_found = on_found;
    handlers->on_aid = on_aid;
    handlers->on_levy = on_levy;
    handlers->on_look = on_look;
    handlers->on_inspect = on_inspect;
    handlers->on_list = on_list;
    handlers->on_status = on_status;
    handlers->on_show = on_show;
    handlers->on_tick = on_tick;
    handlers->on_event = on_event;
    handlers->on_build = on_build;
    handlers->on_nudge = on_nudge;
    handlers->on_links = on_links;
    handlers->on_notes = on_notes;
    handlers->on_unknown = on_unknown;
}
