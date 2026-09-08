#ifndef _TEXT_ACTIONS_H_
#define _TEXT_ACTIONS_H_

#include "../../engine/common.h"
#include "text_dispatch.h"

#define TEXT_MODE_DEBUG 0
#define TEXT_MODE_GAME  1

typedef struct {
    void *ctx;

    int (*get_mode)(void *ctx);
    void (*set_mode)(void *ctx, int mode);
    void (*print_help)(void *ctx);
    void (*print_game_help)(void *ctx);

    void (*cmd_load)(void *ctx, const char *path);
    void (*cmd_map)(void *ctx);
    void (*cmd_visible)(void *ctx);
    void (*cmd_center)(void *ctx, byte x, byte y);
    void (*cmd_goto)(void *ctx, byte id);
    void (*cmd_pan)(void *ctx, const char *dir, int amount);
    void (*cmd_cursor)(void *ctx, const char *row, const char *col);
    void (*cmd_move)(void *ctx, const char *dir, int amount);
    void (*cmd_game_move)(void *ctx, const char *dir, int amount);
    void (*cmd_game_found)(void *ctx);
    void (*cmd_game_aid)(void *ctx);
    void (*cmd_game_levy)(void *ctx);
    void (*cmd_look)(void *ctx);
    void (*cmd_inspect)(void *ctx, const char *row, const char *col);
    void (*cmd_list)(void *ctx);
    void (*cmd_status)(void *ctx);
    void (*cmd_show)(void *ctx, byte id);
    void (*cmd_tick)(void *ctx, int n);
    void (*cmd_event)(void *ctx, const char *name, byte id);
    void (*cmd_build)(void *ctx, byte id, const char *type);
    void (*cmd_nudge)(void *ctx, byte id, const char *focus);
    void (*cmd_links)(void *ctx);
    void (*cmd_notes)(void *ctx);
} TextActionApi;

void text_actions_build_dispatch(const TextActionApi *api, TextDispatchHandlers *handlers);

#endif
