#include <stdlib.h>
#include <strings.h>

#include "text_commands.h"
#include "../../engine/settlement.h"

int text_parse_hex_nibble(const char *s, int max_value_exclusive)
{
    char *end;
    unsigned long value;

    if (!s || s[0] == '\0') return -1;
    value = strtoul(s, &end, 16);
    if (*end != '\0' || (int)value >= max_value_exclusive) return -1;
    return (int)value;
}

int text_focus_from_name(const char *name)
{
    if (strcasecmp(name, "TRA") == 0) return CULTURE_TRA;
    if (strcasecmp(name, "AGR") == 0) return CULTURE_AGR;
    if (strcasecmp(name, "GRO") == 0) return CULTURE_GRO;
    if (strcasecmp(name, "SEC") == 0) return CULTURE_SEC;
    return -1;
}

int text_struct_type_from_name(const char *name)
{
    static const char *STRUCT_NAMES[STRUCT_TYPE_COUNT] = {
        "dock", "warehouse", "fort", "townhall", "monument"
    };
    int i;

    for (i = 0; i < STRUCT_TYPE_COUNT; i++) {
        if (strcasecmp(name, STRUCT_NAMES[i]) == 0) return i;
    }
    return -1;
}
