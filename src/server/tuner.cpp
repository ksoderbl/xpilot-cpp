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

#define SERVER
#include "serverconst.h"
#include "global.h"
#include "proto.h"
#include "xperror.h"
#include "xpmath.h"
#include "sched.h"
#include "walls.h"

extern time_t gameOverTime;

void tuner_plock(void)
{
    options.pLockServer = (plock_server(options.pLockServer) == 1) ? true : false;
}

void tuner_shotsmax(void)
{
    int i;

    for (i = 0; i < NumPlayers; i++)
    {
        Players[i]->shot_max = options.ShotsMax;
    }
}

void tuner_shipmass(void)
{
    int i;

    for (i = 0; i < NumPlayers; i++)
    {
        Players[i]->emptymass = options.ShipMass;
    }
}

void tuner_ballmass(void)
{
    int i;

    for (i = 0; i < NumObjs; i++)
    {
        if (BIT(Obj[i]->type, OBJ_BALL))
        {
            Obj[i]->mass = options.ballMass;
        }
    }
}

void tuner_maxrobots(void)
{
    if (options.maxRobots < 0)
    {
        options.maxRobots = World.NumBases;
    }

    if (options.maxRobots < options.minRobots)
    {
        options.minRobots = options.maxRobots;
    }

    while (options.maxRobots < NumRobots)
    {
        Robot_delete(-1, true);
    }
}

void tuner_minrobots(void)
{
    if (options.minRobots < 0)
    {
        options.minRobots = options.maxRobots;
    }

    if (options.maxRobots < options.minRobots)
    {
        options.maxRobots = options.minRobots;
    }
}

void tuner_playershielding(void)
{
    int i;

    Set_world_rules();

    if (options.playerShielding)
    {
        SET_BIT(DEF_HAVE, HAS_SHIELD);

        for (i = 0; i < NumPlayers; i++)
        {
            if (!IS_TANK_PTR(Players[i]))
            {
                if (!BIT(Players[i]->used, HAS_SHOT))
                    SET_BIT(Players[i]->used, HAS_SHIELD);

                SET_BIT(Players[i]->have, HAS_SHIELD);
                Players[i]->shield_time = 0;
            }
        }
    }
    else
    {
        CLR_BIT(DEF_HAVE, HAS_SHIELD);

        for (i = 0; i < NumPlayers; i++)
        {
            Players[i]->shield_time = 2 * FPS;
            /* 2 seconds to get to safety */
        }
    }
}

void tuner_playerstartsshielded(void)
{
    if (options.playerShielding)
    {
        options.playerStartsShielded = true; /* Doesn't make sense
                                    to turn off when
                                    shields are on. */
    }
}

void tuner_worldlives(void)
{
    if (options.worldLives < 0)
        options.worldLives = 0;

    Set_world_rules();

    if (BIT(World.rules->mode, LIMITED_LIVES))
    {
        Reset_all_players();
        if (options.gameDuration == -1)
            options.gameDuration = 0;
    }
}

void tuner_cannonsmartness(void)
{
    LIMIT(options.cannonSmartness, 0, 3);
}

void tuner_teamcannons(void)
{
    int i;
    int team;

    if (options.teamCannons)
    {
        for (i = 0; i < World.NumCannons; i++)
        {
            team = Find_closest_team(World.cannon[i].clk_pos.cx, World.cannon[i].clk_pos.cy);
            if (team == TEAM_NOT_SET)
            {
                error("Couldn't find a matching team for the cannon.");
            }
            World.cannon[i].team = team;
        }
    }
    else
    {
        for (i = 0; i < World.NumCannons; i++)
            World.cannon[i].team = TEAM_NOT_SET;
    }
}

void tuner_cannonsuseitems(void)
{
    int i, j;
    cannon_t *c;

    Move_init();

    for (i = 0; i < World.NumCannons; i++)
    {
        c = World.cannon + i;
        for (j = 0; j < NUM_ITEMS; j++)
        {
            c->item[j] = 0;

            if (options.cannonsUseItems)
                Cannon_add_item(i, j,
                                (int)(rfrac() * (World.items[j].initial + 1)));
        }
    }
}

void tuner_wormtime(void)
{
    int i;

    if (options.wormTime < 0)
        options.wormTime = 0;

    if (options.wormTime)
    {
        for (i = 0; i < World.NumWormholes; i++)
        {
            World.wormHoles[i].countdown = options.wormTime;
        }
    }
    else
    {
        for (i = 0; i < World.NumWormholes; i++)
        {
            if (World.wormHoles[i].temporary)
                remove_temp_wormhole(i);
            else
                World.wormHoles[i].countdown = WORMCOUNT;
        }
    }
}

void tuner_modifiers(void)
{
    int i;

    Set_world_rules();

    for (i = 0; i < NumPlayers; i++)
    {
        filter_mods(&Players[i]->mods);
    }
}

void tuner_minelife(void)
{
    int i;
    int life;

    if (options.mineLife < 0)
        options.mineLife = 0;

    for (i = 0; i < NumObjs; i++)
    {
        if (Obj[i]->type != OBJ_MINE)
            continue;

        if (!BIT(Obj[i]->status, FROMCANNON))
        {
            life =
                (options.mineLife ? options.mineLife : MINE_LIFETIME) / (Obj[i]->mods.mini +
                                                                         1);

            Obj[i]->life = (int)(rfrac() * life);
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
        if (Obj[i]->type != OBJ_SMART_SHOT &&
            Obj[i]->type != OBJ_HEAT_SHOT && Obj[i]->type != OBJ_TORPEDO)
            continue;

        if (!BIT(Obj[i]->status, FROMCANNON))
        {
            life =
                (options.mineLife ? options.mineLife : MISSILE_LIFETIME) / (Obj[i]->mods.mini +
                                                                            1);

            Obj[i]->life = (int)(rfrac() * life);
            /* Maybe all the missiles are full
               nukes. Going off together might
               not be such a good idea. */
        }
    }
}

void tuner_gameduration(void)
{
    if (options.gameDuration <= 0.0)
    {
        gameOverTime = time((time_t *)NULL);
    }

    else
        gameOverTime = (time_t)(options.gameDuration * 60) + time((time_t *)NULL);
}

void tuner_racelaps(void)
{
    if (BIT(World.rules->mode, TIMING))
    {
        Reset_all_players();
        if (options.gameDuration == -1)
            options.gameDuration = 0;
    }
}

void tuner_allowalliances(void)
{
    if (BIT(World.rules->mode, TEAM_PLAY))
    {
        CLR_BIT(World.rules->mode, ALLIANCES);
    }
    if (!BIT(World.rules->mode, ALLIANCES) && NumAlliances > 0)
    {
        Dissolve_all_alliances();
    }
}

void tuner_announcealliances(void)
{
    updateScores = true;
}

void tuner_fps(void)
{
    install_timer_tick(nullptr, FPS);
}
