#ifndef _TEXT_COMMANDS_H_
#define _TEXT_COMMANDS_H_

#include "../../engine/common.h"

int text_parse_hex_nibble(const char *s, int max_value_exclusive);
int text_focus_from_name(const char *name);
int text_struct_type_from_name(const char *name);

#endif
