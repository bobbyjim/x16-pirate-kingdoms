#ifndef _GAME_RULES_H_
#define _GAME_RULES_H_

#include "world.h"

/* Player-facing game-mode rules shared by any front end (CLI now, X16 UI
   later). Keep these values and transitions in the engine so UI layers only
   issue actions and render the outcome. */

#define GAME_START_SUPPLIES 100
#define GAME_MOVE_SUPPLY_COST 1
#define GAME_AID_SUPPLY_COST 6
#define GAME_LEVY_SUPPLY_GAIN 20
#define GAME_FOUND_SUPPLY_COST 30

typedef struct {
    int supplies;
    int reputation;
} GameRulesState;

typedef enum {
    GAME_RULE_OK = 0,
    GAME_RULE_NO_SETTLEMENT,
    GAME_RULE_SETTLEMENT_DEAD,
    GAME_RULE_NOT_ENOUGH_SUPPLIES,
    GAME_RULE_TILE_BLOCKED,
    GAME_RULE_TABLE_FULL,
    GAME_RULE_BAD_TERRAIN
} GameRuleResult;

void game_rules_init(GameRulesState *state);

/* Returns 1 if one move-step worth of supplies was spent, 0 otherwise. */
int game_rules_try_spend_move(GameRulesState *state);

/* Applies one settlement action and advances the world by one tick on success. */
GameRuleResult game_rules_apply_aid(World *world, GameRulesState *state, byte settlement_id);
GameRuleResult game_rules_apply_levy(World *world, GameRulesState *state, byte settlement_id);
GameRuleResult game_rules_apply_found(World *world, GameRulesState *state,
                                      byte x, byte y, byte owner,
                                      byte *new_settlement_id_out);

#endif
