#include <stdio.h>

#include "text_render.h"

#include "../../engine/events.h"
#include "../../engine/settlement.h"

char text_terrain_glyph(byte terrain)
{
    switch (terrain) {
        case TERRAIN_WATER:     return '.';
        case TERRAIN_GRASS:     return 'g';
        case TERRAIN_FOREST:    return 'f';
        case TERRAIN_HILLS:     return 'h';
        case TERRAIN_MOUNTAINS: return 'm';
        case TERRAIN_DESERT:    return 'd';
        case TERRAIN_SWAMP:     return 'w';
        default:                return '?';
    }
}

const char *text_terrain_name(byte terrain)
{
    switch (terrain) {
        case TERRAIN_WATER:     return "water";
        case TERRAIN_GRASS:     return "grass";
        case TERRAIN_FOREST:    return "forest";
        case TERRAIN_HILLS:     return "hills";
        case TERRAIN_MOUNTAINS: return "mountains";
        case TERRAIN_DESERT:    return "desert";
        case TERRAIN_SWAMP:     return "swamp";
        default:                return "unknown";
    }
}

const char *text_travel_ease_name(byte travel_ease)
{
    switch (travel_ease) {
        case 0: return "rough";
        case 1: return "trail";
        case 2: return "road";
        case 3: return "road";
        default: return "unknown";
    }
}

char text_object_glyph(byte type)
{
    switch (type) {
        case OBJ_SETTLEMENT: return 'X';
        case OBJ_TOWER:      return 'T';
        case OBJ_SHRINE:     return 'S';
        case OBJ_RUINS:      return 'R';
        case OBJ_MINE:       return 'M';
        case OBJ_STELA:      return 'A';
        case OBJ_PORTAL:     return 'P';
        case OBJ_CAVE:       return 'C';
        case OBJ_MONUMENT:   return 'O';
        case OBJ_LIGHTHOUSE: return 'L';
        case OBJ_BRIDGE:     return 'B';
        case OBJ_SHIP:       return 's';
        case OBJ_GROUP:      return 'G';
        default:             return '?';
    }
}

const char *text_object_type_name(byte type)
{
    switch (type) {
        case OBJ_SETTLEMENT: return "settlement";
        case OBJ_TOWER:      return "tower";
        case OBJ_SHRINE:     return "shrine";
        case OBJ_RUINS:      return "ruins";
        case OBJ_MINE:       return "mine";
        case OBJ_STELA:      return "stela";
        case OBJ_PORTAL:     return "portal";
        case OBJ_CAVE:       return "cave";
        case OBJ_MONUMENT:   return "monument";
        case OBJ_LIGHTHOUSE: return "lighthouse";
        case OBJ_BRIDGE:     return "bridge";
        case OBJ_SHIP:       return "ship";
        case OBJ_GROUP:      return "group";
        default:             return "unknown";
    }
}

static const char *STRUCT_NAMES[STRUCT_TYPE_COUNT] = {
    "dock", "warehouse", "fort", "townhall", "monument"
};

void text_print_settlement_header(void)
{
    puts("#ID  (xxx,yyy) stats pop wea res inf def TRA AGR GRO SEC event   structures                        ");
    puts("---------------------------------------------------------------------------------------------------");
}

void text_print_settlement(const Settlement *s)
{
    byte culture[CULTURE_COUNT];
    byte i;

    settlement_culture_vector(s, culture);

    printf("#%-3u (%3u,%3u) %s %3u %3u %3u %3u %3u "
           "%3u %3u %3u %3u %-12s : ",
           s->id, s->x, s->y, s->alive ? "alive" : "DEAD ",
           settlement_population_support(s), settlement_wealth_potential(s),
           settlement_reserve_potential(s), settlement_infrastructure_resilience(s),
           settlement_defense_posture(s),
           culture[CULTURE_TRA], culture[CULTURE_AGR], culture[CULTURE_GRO], culture[CULTURE_SEC],
           s->event_status == EVENT_STATUS_NONE ? "-" : event_name((EventType)s->event_status));

    for (i = 0; i < MAX_STRUCTURE_SLOTS; i++) {
        if (s->structures[i].type == STRUCT_EMPTY || s->structures[i].type >= STRUCT_TYPE_COUNT) continue;
        printf(" %c.%x", STRUCT_NAMES[s->structures[i].type][0]-32, s->structures[i].condition);
    }
    printf("\n");
}

static const char *text_link_type_name(byte type)
{
    switch (type) {
        case TRADE_LINK_CARAVAN: return "caravan";
        case TRADE_LINK_FLEET:   return "fleet";
        default:                 return "?";
    }
}

void text_print_trade_link(const TradeLink *l)
{
    printf("#%-3u %-7s #%u <-> #%u  health=%3u throughput=%3u risk=%3u  flags=%s%s%s%s  last_event=%s\n",
           l->link_id, text_link_type_name(l->type), l->from_settlement_id, l->to_settlement_id,
           l->health, l->throughput, l->risk,
           (l->status_flags & TRADE_LINK_ACTIVE) ? "A" : "-",
           (l->status_flags & TRADE_LINK_DISRUPTED) ? "D" : "-",
           (l->status_flags & TRADE_LINK_BLOCKED) ? "B" : "-",
           (l->status_flags & TRADE_LINK_RECOVERING) ? "R" : "-",
           l->last_event_tag == TRADE_LINK_EVENT_NONE ? "-" : event_name((EventType)l->last_event_tag));
}
