#include <string.h>
#include "events.h"

static const char *EVENT_NAMES[EVENT_COUNT] = {
    "drought", "plague", "storm", "pirates", "market_crash", "civil_war"
};

const char *event_name(EventType type)
{
    if (type < 0 || type >= EVENT_COUNT) return "unknown";
    return EVENT_NAMES[type];
}

int event_from_name(const char *name)
{
    int i;
    for (i = 0; i < EVENT_COUNT; i++) {
        if (strcmp(name, EVENT_NAMES[i]) == 0) return i;
    }
    return -1;
}

/* Higher tested-stat / mitigating-focus values reduce severity; a weak
   settlement (low stat, low focus) takes the worst of it. */
static int severity_roll(Rng *rng, byte tested_value, byte focus_value)
{
    int base = 6;
    int reduction = (tested_value / 3) + (focus_value / 64);
    int roll = (int)rng_range(rng, 4);
    int severity = base - reduction + roll;
    if (severity < 1) severity = 1;
    if (severity > 15) severity = 15;
    return severity;
}

void event_apply(Settlement *s, EventType type, Rng *rng, EventResult *result)
{
    StatField primary, secondary;
    CultureFocus mitigator;
    int severity;

    result->collapsed = 0;
    result->spawned_faction = 0;

    switch (type) {
        case EVENT_DROUGHT:
            primary = STAT_RESERVES; mitigator = CULTURE_GRO; secondary = STAT_POPULATION;
            break;
        case EVENT_PLAGUE:
            primary = STAT_INFRASTRUCTURE; mitigator = CULTURE_SEC; secondary = STAT_POPULATION;
            break;
        case EVENT_STORM:
            primary = STAT_INFRASTRUCTURE; mitigator = CULTURE_GRO; secondary = STAT_DEFENSE;
            break;
        case EVENT_PIRATES:
            primary = STAT_DEFENSE; mitigator = CULTURE_SEC; secondary = STAT_WEALTH;
            break;
        case EVENT_MARKET_CRASH:
            primary = STAT_RESERVES; mitigator = CULTURE_TRA; secondary = STAT_WEALTH;
            break;
        case EVENT_CIVIL_WAR:
            primary = STAT_DEFENSE; mitigator = CULTURE_SEC; secondary = STAT_POPULATION;
            break;
        default:
            return;
    }

    severity = severity_roll(rng, settlement_get_stat(s, primary), s->culture[mitigator]);
    settlement_add_stat(s, primary, -severity);

    /* Once the tested stat bottoms out, the strain spills onto the
       secondary resource harder than if the settlement merely weakened. */
    if (settlement_get_stat(s, primary) == 0) {
        settlement_add_stat(s, secondary, -(severity / 2 + 1));
    } else {
        settlement_add_stat(s, secondary, -(severity / 3));
    }

    if (type == EVENT_CIVIL_WAR &&
        settlement_get_stat(s, STAT_DEFENSE) == 0 &&
        settlement_get_stat(s, STAT_POPULATION) > 1) {
        result->spawned_faction = 1;
    }

    s->event_status = (byte)type;
    result->collapsed = settlement_is_collapsed(s);
}
