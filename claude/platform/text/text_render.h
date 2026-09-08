#ifndef _TEXT_RENDER_H_
#define _TEXT_RENDER_H_

#include "../../engine/common.h"
#include "../../engine/world.h"

char text_terrain_glyph(byte terrain);
const char *text_terrain_name(byte terrain);
const char *text_travel_ease_name(byte travel_ease);
char text_object_glyph(byte type);
const char *text_object_type_name(byte type);

void text_print_settlement_header(void);
void text_print_settlement(const Settlement *s);
void text_print_trade_link(const TradeLink *l);

#endif
