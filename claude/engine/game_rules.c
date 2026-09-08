#include "game_rules.h"

void game_rules_init(GameRulesState *state)
{
    if (!state) return;
    state->supplies = GAME_START_SUPPLIES;
    state->reputation = 0;
}

int game_rules_try_spend_move(GameRulesState *state)
{
    if (!state) return 0;
    if (state->supplies < GAME_MOVE_SUPPLY_COST) return 0;
    state->supplies -= GAME_MOVE_SUPPLY_COST;
    return 1;
}

static GameRuleResult lookup_live_settlement(World *world, byte settlement_id, Settlement **out)
{
    Settlement *s;

    if (!world || !out) return GAME_RULE_NO_SETTLEMENT;
    s = world_get_settlement(world, settlement_id);
    if (!s) return GAME_RULE_NO_SETTLEMENT;
    if (!s->alive) return GAME_RULE_SETTLEMENT_DEAD;

    *out = s;
    return GAME_RULE_OK;
}

GameRuleResult game_rules_apply_aid(World *world, GameRulesState *state, byte settlement_id)
{
    Settlement *s;
    GameRuleResult check;

    if (!state) return GAME_RULE_NOT_ENOUGH_SUPPLIES;
    if (state->supplies < GAME_AID_SUPPLY_COST) return GAME_RULE_NOT_ENOUGH_SUPPLIES;

    check = lookup_live_settlement(world, settlement_id, &s);
    if (check != GAME_RULE_OK) return check;

    state->supplies -= GAME_AID_SUPPLY_COST;
    settlement_repair_characteristic(s, CHAR_POPULATION, 2);
    settlement_repair_characteristic(s, CHAR_COMMERCE, 2);
    settlement_repair_characteristic(s, CHAR_DEFENSE, 1);
    settlement_nudge_focus(s, CULTURE_SEC);
    state->reputation += 2;

    world_tick(world);
    return GAME_RULE_OK;
}

GameRuleResult game_rules_apply_levy(World *world, GameRulesState *state, byte settlement_id)
{
    Settlement *s;
    GameRuleResult check;

    if (!state) return GAME_RULE_NO_SETTLEMENT;

    check = lookup_live_settlement(world, settlement_id, &s);
    if (check != GAME_RULE_OK) return check;

    settlement_damage_characteristic(s, CHAR_INDUSTRY, 2);
    settlement_damage_characteristic(s, CHAR_COMMERCE, 1);
    state->supplies += GAME_LEVY_SUPPLY_GAIN;
    state->reputation -= 1;

    world_tick(world);
    return GAME_RULE_OK;
}

GameRuleResult game_rules_apply_found(World *world, GameRulesState *state,
                                      byte x, byte y, byte owner,
                                      byte *new_settlement_id_out)
{
    Settlement *s;
    word i;

    if (!world || !state) return GAME_RULE_TILE_BLOCKED;
    if (state->supplies < GAME_FOUND_SUPPLY_COST) return GAME_RULE_NOT_ENOUGH_SUPPLIES;

    if (map_get_terrain_at(&world->map, x, y) == TERRAIN_WATER) return GAME_RULE_BAD_TERRAIN;

    for (i = 0; i < world->settlement_count; i++) {
        Settlement *o = &world->settlements[i];
        if (!o->alive) continue;
        if (o->x == x && o->y == y) return GAME_RULE_TILE_BLOCKED;
    }

    if (map_has_object_at(&world->map, x, y)) return GAME_RULE_TILE_BLOCKED;

    s = NULL;
    for (i = 0; i < world->settlement_count; i++) {
        if (!world->settlements[i].alive) {
            s = &world->settlements[i];
            break;
        }
    }
    if (!s) {
        if (world->settlement_count >= MAX_SETTLEMENTS) return GAME_RULE_TABLE_FULL;
        s = &world->settlements[world->settlement_count++];
    }

    settlement_init(s, (byte)(s - world->settlements), x, y, owner, 2);
    settlement_build(s, STRUCT_TOWNHALL);

    state->supplies -= GAME_FOUND_SUPPLY_COST;
    state->reputation += 3;

    if (new_settlement_id_out) *new_settlement_id_out = s->id;

    world_tick(world);
    return GAME_RULE_OK;
}
