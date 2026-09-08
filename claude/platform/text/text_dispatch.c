#include <string.h>

#include "text_dispatch.h"

static char *next_tok(void)
{
    return strtok(NULL, " \t\r\n");
}

int text_dispatch_execute(char *line, const TextDispatchHandlers *handlers)
{
    char *cmd = strtok(line, " \t\r\n");

    if (!handlers || !cmd) return 1;

    if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
        return 0;
    } else if (strcmp(cmd, "mode") == 0) {
        if (handlers->on_mode) handlers->on_mode(handlers->ctx, next_tok());
    } else if (strcmp(cmd, "debug") == 0) {
        if (handlers->on_debug) handlers->on_debug(handlers->ctx);
    } else if (strcmp(cmd, "game") == 0) {
        if (handlers->on_game) handlers->on_game(handlers->ctx);
    } else if (strcmp(cmd, "help") == 0) {
        if (handlers->on_help) handlers->on_help(handlers->ctx);
    } else if (strcmp(cmd, "load") == 0) {
        if (handlers->on_load) handlers->on_load(handlers->ctx, next_tok());
    } else if (strcmp(cmd, "map") == 0) {
        if (handlers->on_map) handlers->on_map(handlers->ctx);
    } else if (strcmp(cmd, "visible") == 0) {
        if (handlers->on_visible) handlers->on_visible(handlers->ctx);
    } else if (strcmp(cmd, "center") == 0) {
        if (handlers->on_center) handlers->on_center(handlers->ctx, next_tok(), next_tok());
    } else if (strcmp(cmd, "goto") == 0) {
        if (handlers->on_goto) handlers->on_goto(handlers->ctx, next_tok());
    } else if (strcmp(cmd, "pan") == 0) {
        if (handlers->on_pan) handlers->on_pan(handlers->ctx, next_tok(), next_tok());
    } else if (strcmp(cmd, "cursor") == 0) {
        if (handlers->on_cursor) handlers->on_cursor(handlers->ctx, next_tok(), next_tok());
    } else if (strcmp(cmd, "move") == 0) {
        if (handlers->on_move) handlers->on_move(handlers->ctx, next_tok(), next_tok());
    } else if (strcmp(cmd, "found") == 0) {
        if (handlers->on_found) handlers->on_found(handlers->ctx);
    } else if (strcmp(cmd, "aid") == 0) {
        if (handlers->on_aid) handlers->on_aid(handlers->ctx);
    } else if (strcmp(cmd, "levy") == 0) {
        if (handlers->on_levy) handlers->on_levy(handlers->ctx);
    } else if (strcmp(cmd, "look") == 0) {
        if (handlers->on_look) handlers->on_look(handlers->ctx);
    } else if (strcmp(cmd, "inspect") == 0) {
        if (handlers->on_inspect) handlers->on_inspect(handlers->ctx, next_tok(), next_tok());
    } else if (strcmp(cmd, "list") == 0) {
        if (handlers->on_list) handlers->on_list(handlers->ctx);
    } else if (strcmp(cmd, "status") == 0) {
        if (handlers->on_status) handlers->on_status(handlers->ctx);
    } else if (strcmp(cmd, "show") == 0) {
        if (handlers->on_show) handlers->on_show(handlers->ctx, next_tok());
    } else if (strcmp(cmd, "tick") == 0) {
        if (handlers->on_tick) handlers->on_tick(handlers->ctx, next_tok());
    } else if (strcmp(cmd, "event") == 0) {
        if (handlers->on_event) handlers->on_event(handlers->ctx, next_tok(), next_tok());
    } else if (strcmp(cmd, "build") == 0) {
        if (handlers->on_build) handlers->on_build(handlers->ctx, next_tok(), next_tok());
    } else if (strcmp(cmd, "nudge") == 0) {
        if (handlers->on_nudge) handlers->on_nudge(handlers->ctx, next_tok(), next_tok());
    } else if (strcmp(cmd, "links") == 0) {
        if (handlers->on_links) handlers->on_links(handlers->ctx);
    } else if (strcmp(cmd, "notes") == 0) {
        if (handlers->on_notes) handlers->on_notes(handlers->ctx);
    } else {
        if (handlers->on_unknown) handlers->on_unknown(handlers->ctx, cmd);
    }

    return 1;
}
