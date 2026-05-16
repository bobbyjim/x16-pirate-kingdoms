#ifndef _MAP_DISPLAY_H_
#define _MAP_DISPLAY_H_

#include "../../core/map_engine.h"

/* X16 map display adapter - renders visible map area using sprites */

#define MAP_VISIBLE_ROWS  7
#define MAP_VISIBLE_COLS  6

void map_display_init(void);
void map_display_calculate(void);
void map_display_frame_draw(void);
void map_display_region(byte dimension);

#endif
