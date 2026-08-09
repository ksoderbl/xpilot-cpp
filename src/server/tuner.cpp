/*
 * XPilot, a multiplayer gravity war game.  Copyright (C) 1991-2001 by
 *
 *      Bjørn Stabell
 *      Ken Ronny Schouten
 *      Bert Gijsbers
 *      Dick Balaska
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <cstdlib>
#include <ctime>

#include "commonproto.h"
#include "xperror.h"

#include "alliance.h"
#include "ship.h"

#include "cannon.h"
#include "server.h"
#include "serverconst.h"
#include "sched.h"
#include "move.h"
#include "robot.h"
#include "modifiers.h"

extern time_t gameOverTime;

void tuner_plock(void)
{
    options.pLockServer = (plock_server(options.pLockServer) == 1) ? true : false;
}

void tuner_shipmass(void)
{
    int i;

    for (i = 0; i < NumPlayers; i++)
        Player_by_index(i)->emptymass = options.shipMass;
}

void tuner_ballmass(void)
{
    int i;

    for (i = 0; i < NumObjs; i++)
    {
        if (Obj[i]->type == OBJ_BALL)
            Obj[i]->mass = options.ballMass;
    }
}

void tuner_maxrobots(void)
{
    world_t *world = &theWorld;

    if (options.maxRobots < 0)
        options.maxRobots = Num_bases(world);

    if (options.maxRobots < options.minRobots)
        options.minRobots = options.maxRobots;

    while (options.maxRobots < NumRobots)
        Robot_delete(nullptr, true);
}

void tuner_minrobots(void)
{
    if (options.minRobots < 0)
        options.minRobots = options.maxRobots;

    if (options.maxRobots < options.minRobots)
        options.maxRobots = options.minRobots;
}

void tuner_allowshields(void)
{
    int i;

    Set_world_rules();

    if (options.allowShields)
    {
        SET_BIT(DEF_HAVE, HAS_SHIELD);

        for (i = 0; i < NumPlayers; i++)
        {
            Player *pl_i = Player_by_index(i);

            if (!Player_is_tank(pl_i))
            {
                if (!BIT(pl_i->used, HAS_SHOT))
                    SET_BIT(pl_i->used, HAS_SHIELD);

                SET_BIT(pl_i->have, HAS_SHIELD);
                pl_i->shield_time = 0;
            }
        }
    }
    else
    {
        CLR_BIT(DEF_HAVE, HAS_SHIELD);

        for (i = 0; i < NumPlayers; i++)
            /* approx 2 seconds to get to safety */
            Player_by_index(i)->shield_time = SHIELD_TIME;
    }
}

void tuner_playerstartsshielded(void)
{
    if (options.allowShields)
        /* Doesn't make sense to turn off when shields are on. */
        options.playerStartsShielded = true;
}

void tuner_worldlives(void)
{
    world_t *world = &theWorld;

    if (options.worldLives < 0)
        options.worldLives = 0;

    Set_world_rules();

    if (Limited_lives(world))
    {
        Reset_all_players();
        if (options.gameDuration == -1)
            options.gameDuration = 0;
    }
}

void tuner_cannonsmartness(void)
{
    LIMIT(options.cannonSmartness, 0, CANNON_SMARTNESS_MAX);
}

void tuner_teamcannons(void)
{
    world_t *world = &theWorld;
    int i;
    int team;

    if (options.teamCannons)
    {
        for (i = 0; i < Num_cannons(world); i++)
        {
            cannon_t *cannon = Cannon_by_index(world, i);

            team = Find_closest_team(cannon->pos);
            if (team == TEAM_NOT_SET)
                warn("Couldn't find a matching team for the cannon.");
            cannon->team = team;
        }
    }
    else
    {
        for (i = 0; i < Num_cannons(world); i++)
            Cannon_by_index(world, i)->team = TEAM_NOT_SET;
    }
}

void tuner_mincannonshotlife(void)
{
    LIMIT(options.minCannonShotLife, 0, FLT_MAX);
    LIMIT(options.maxCannonShotLife, options.minCannonShotLife, FLT_MAX);
}

void tuner_maxcannonshotlife(void)
{
    LIMIT(options.maxCannonShotLife, 0, FLT_MAX);
    LIMIT(options.minCannonShotLife, 0, options.maxCannonShotLife);
}

void tuner_cannonsuseitems(void)
{
    world_t *world = &theWorld;
    int i, j;
    cannon_t *c;

    Move_init();

    for (i = 0; i < Num_cannons(world); i++)
    {
        c = &world->cannons[i];
        for (j = 0; j < NUM_ITEMS; j++)
        {
            c->item[j] = 0;

            if (options.cannonsUseItems)
                Cannon_add_item(c, j,
                                (int)(rfrac() * (world->items[j].initial + 1)));
        }
    }
}

void tuner_wormtime(void)
{
    world_t *world = &theWorld;
    int i;

    if (options.wormTime < 0)
        options.wormTime = 0;

    /* Make sure all wormholes get a new destination */
    if (options.wormTime)
    {
        for (i = 0; i < Num_wormholes(world); i++)
            world->wormholes[i].countdown = options.wormTime;
    }
    else
    {
        for (i = 0; i < Num_wormholes(world); i++)
        {
            if (world->wormholes[i].temporary)
                remove_temp_wormhole(i);
            else
                world->wormholes[i].countdown = WORMCOUNT;
        }
    }
}

void tuner_modifiers(void)
{
    int i;

    Set_world_rules();

    for (i = 0; i < NumPlayers; i++)
        Mods_filter(&(Player_by_index(i))->mods);
}

void tuner_minelife(void)
{
    int i;
    int life;

    if (options.mineLife < 0)
        options.mineLife = 0;

    for (i = 0; i < NumObjs; i++)
    {
        if (Obj[i]->type != OBJ_MINE_BIT)
            continue;

        if (!BIT(Obj[i]->obj_status, FROMCANNON))
        {
            // life = (options.mineLife ? options.mineLife : MINE_LIFETIME) / (Obj[i]->mods.mini + 1);
            life = (options.mineLife ? options.mineLife : MINE_LIFETIME) / (Mods_get(Obj[i]->mods, ModsMini) + 1);

            Obj[i]->obj_life = (int)(rfrac() * life);
            /* We wouldn't want all the mines
               to explode simultaneously, now
               would we? */
        }
    }
}

void tuner_missilelife(void)
{
    int i;
    int life;

    if (options.missileLife < 0)
        options.missileLife = 0;

    for (i = 0; i < NumObjs; i++)
    {
        if (Obj[i]->type != OBJ_SMART_SHOT_BIT &&
            Obj[i]->type != OBJ_HEAT_SHOT_BIT && Obj[i]->type != OBJ_TORPEDO_BIT)
            continue;

        if (!BIT(Obj[i]->obj_status, FROMCANNON))
        {
            // life = (options.mineLife ? options.mineLife : MISSILE_LIFETIME) / (Obj[i]->mods.mini + 1);
            life = (options.mineLife ? options.mineLife : MISSILE_LIFETIME) / (Mods_get(Obj[i]->mods, ModsMini) + 1);

            Obj[i]->obj_life = (int)(rfrac() * life);
            /* Maybe all the missiles are full
               nukes. Going off together might
               not be such a good idea. */
        }
    }
}

void tuner_gameduration(void)
{
    if (options.gameDuration <= 0.0)
        gameOverTime = time((time_t *)nullptr);
    else
        gameOverTime = (time_t)(options.gameDuration * 60) + time((time_t *)nullptr);
}

void tuner_racelaps(void)
{
    world_t *world = &theWorld;

    if (Timing(world))
    {
        Reset_all_players();
        if (options.gameDuration == -1)
            options.gameDuration = 0;
    }
}

void tuner_allowalliances(void)
{
    world_t *world = &theWorld;

    if (Team_play(world))
        CLR_BIT(world->rules.mode, ALLIANCES);

    if (!BIT(world->rules.mode, ALLIANCES) && NumAlliances > 0)
        Dissolve_all_alliances();
}

void tuner_announcealliances(void)
{
    updateScores = true;
}

void tuner_fps(void)
{
    install_timer_tick(nullptr, FPS);
}
