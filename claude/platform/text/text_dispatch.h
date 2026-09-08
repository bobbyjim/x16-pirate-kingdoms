#ifndef _TEXT_DISPATCH_H_
#define _TEXT_DISPATCH_H_

typedef struct {
    void *ctx;

    void (*on_mode)(void *ctx, const char *mode_arg);
    void (*on_debug)(void *ctx);
    void (*on_game)(void *ctx);
    void (*on_help)(void *ctx);
    void (*on_load)(void *ctx, const char *path);
    void (*on_map)(void *ctx);
    void (*on_visible)(void *ctx);
    void (*on_center)(void *ctx, const char *x, const char *y);
    void (*on_goto)(void *ctx, const char *id);
    void (*on_pan)(void *ctx, const char *dir, const char *n);
    void (*on_cursor)(void *ctx, const char *row, const char *col);
    void (*on_move)(void *ctx, const char *dir, const char *n);
    void (*on_found)(void *ctx);
    void (*on_aid)(void *ctx);
    void (*on_levy)(void *ctx);
    void (*on_look)(void *ctx);
    void (*on_inspect)(void *ctx, const char *row, const char *col);
    void (*on_list)(void *ctx);
    void (*on_status)(void *ctx);
    void (*on_show)(void *ctx, const char *id);
    void (*on_tick)(void *ctx, const char *n);
    void (*on_event)(void *ctx, const char *name, const char *id);
    void (*on_build)(void *ctx, const char *id, const char *type);
    void (*on_nudge)(void *ctx, const char *id, const char *focus);
    void (*on_links)(void *ctx);
    void (*on_notes)(void *ctx);
    void (*on_unknown)(void *ctx, const char *cmd);
} TextDispatchHandlers;

/* Returns 0 when command requests exit, 1 otherwise. */
int text_dispatch_execute(char *line, const TextDispatchHandlers *handlers);

#endif
