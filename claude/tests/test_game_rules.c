#include "test.h"
#include "../engine/game_rules.h"

void run_game_rules_tests(void)
{
    World w;
    GameRulesState gs;
    Settlement *s;
    Settlement *founded;
    byte fx = 0, fy = 0, new_id = 0;
    byte found_open_land = 0;
    int i;
    int x, y;

    CHECK(world_load(&w, "tests/sample.map", 1) == 0);

    game_rules_init(&gs);
    CHECK(gs.supplies == GAME_START_SUPPLIES);
    CHECK(gs.reputation == 0);

    for (i = 0; i < GAME_START_SUPPLIES; i++) {
        CHECK(game_rules_try_spend_move(&gs) == 1);
    }
    CHECK(gs.supplies == 0);
    CHECK(game_rules_try_spend_move(&gs) == 0);

    game_rules_init(&gs);
    s = world_get_settlement(&w, 0);
    CHECK(s != NULL);

    /* levy: immediate supply gain + reputation loss + one tick */
    {
        int before_supply = gs.supplies;
        int before_rep = gs.reputation;
        unsigned long before_tick = w.tick;

        CHECK(game_rules_apply_levy(&w, &gs, 0) == GAME_RULE_OK);
        CHECK(gs.supplies == before_supply + GAME_LEVY_SUPPLY_GAIN);
        CHECK(gs.reputation == before_rep - 1);
        CHECK(w.tick == before_tick + 1);
    }

    /* aid: supply spend + reputation gain + one tick */
    {
        int before_supply = gs.supplies;
        int before_rep = gs.reputation;
        unsigned long before_tick = w.tick;

        CHECK(game_rules_apply_aid(&w, &gs, 0) == GAME_RULE_OK);
        CHECK(gs.supplies == before_supply - GAME_AID_SUPPLY_COST);
        CHECK(gs.reputation == before_rep + 2);
        CHECK(w.tick == before_tick + 1);
    }

    /* no supplies means aid is rejected and tick doesn't advance */
    {
        unsigned long before_tick = w.tick;
        gs.supplies = 0;
        CHECK(game_rules_apply_aid(&w, &gs, 0) == GAME_RULE_NOT_ENOUGH_SUPPLIES);
        CHECK(w.tick == before_tick);
    }

    /* invalid settlement id is rejected */
    CHECK(game_rules_apply_levy(&w, &gs, 250) == GAME_RULE_NO_SETTLEMENT);

    for (y = 0; y < MAP_HEIGHT && !found_open_land; y++) {
        for (x = 0; x < MAP_WIDTH && !found_open_land; x++) {
            if (map_get_terrain_at(&w.map, (byte)x, (byte)y) == TERRAIN_WATER) continue;
            if (map_has_object_at(&w.map, (byte)x, (byte)y)) continue;
            if (world_find_settlement_at(&w, (byte)x, (byte)y)) continue;
            fx = (byte)x;
            fy = (byte)y;
            found_open_land = 1;
        }
    }

    CHECK(found_open_land == 1);

    game_rules_init(&gs);
    {
        int before_supply = gs.supplies;
        int before_rep = gs.reputation;
        unsigned long before_tick = w.tick;
        word before_count = w.settlement_count;

        CHECK(game_rules_apply_found(&w, &gs, fx, fy, 0, &new_id) == GAME_RULE_OK);
        CHECK(gs.supplies == before_supply - GAME_FOUND_SUPPLY_COST);
        CHECK(gs.reputation == before_rep + 3);
        CHECK(w.tick == before_tick + 1);
        CHECK(w.settlement_count == before_count + 1);

        founded = world_get_settlement(&w, new_id);
        CHECK(founded != NULL);
        CHECK(founded->x == fx && founded->y == fy);
        CHECK(founded->alive == 1);
    }

    CHECK(game_rules_apply_found(&w, &gs, fx, fy, 0, &new_id) == GAME_RULE_TILE_BLOCKED);
    CHECK(game_rules_apply_found(&w, &gs, 0, 0, 0, &new_id) == GAME_RULE_BAD_TERRAIN);
}
