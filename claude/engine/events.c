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

/* Higher tested-characteristic / mitigating-focus values reduce severity;
   a weak settlement (little built toward the tested characteristic, low
   focus) takes the worst of it. Characteristic totals run well past the
   old 0-15 stat range (multiple structures can stack), hence the /8
   divisor here versus the old model's /3. */
static int severity_roll(Rng *rng, word tested_value, byte focus_value)
{
    int base = 6;
    int reduction = (int)(tested_value / 8) + (focus_value / 64);
    int roll = (int)rng_range(rng, 4);
    int severity = base - reduction + roll;
    if (severity < 1) severity = 1;
    if (severity > STAT_MAX) severity = STAT_MAX;
    return severity;
}

void event_apply(Settlement *s, EventType type, Rng *rng, EventResult *result)
{
    CharacteristicField target;
    CultureFocus mitigator;
    byte culture[CULTURE_COUNT];
    word tested;
    int severity;

    result->collapsed = 0;
    result->spawned_faction = 0;

    switch (type) {
        case EVENT_DROUGHT:
            target = CHAR_INDUSTRY; mitigator = CULTURE_GRO;
            break;
        case EVENT_PLAGUE:
            target = CHAR_POPULATION; mitigator = CULTURE_SEC;
            break;
        case EVENT_STORM:
            target = CHAR_COMMERCE; mitigator = CULTURE_GRO;
            break;
        case EVENT_PIRATES:
            target = CHAR_DEFENSE; mitigator = CULTURE_SEC;
            break;
        case EVENT_MARKET_CRASH:
            target = CHAR_COMMERCE; mitigator = CULTURE_TRA;
            break;
        case EVENT_CIVIL_WAR:
            target = CHAR_DEFENSE; mitigator = CULTURE_SEC;
            break;
        default:
            return;
    }

    tested = settlement_characteristic(s, target);
    settlement_culture_vector(s, culture);
    severity = severity_roll(rng, tested, culture[mitigator]);

    /* Damaging every structure that feeds `target` naturally spills onto
       whatever else those same structures feed (e.g. damaging a Fort for
       a Defense-tested event also erodes its Population contribution) --
       there's no separate "secondary stat" step needed anymore. */
    settlement_damage_characteristic(s, target, severity);

    if (type == EVENT_CIVIL_WAR &&
        settlement_defense_posture(s) == 0 &&
        settlement_population_support(s) > 0) {
        result->spawned_faction = 1;
    }

    s->event_status = (byte)type;
    result->collapsed = settlement_is_collapsed(s);
}
