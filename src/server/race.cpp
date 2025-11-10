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
#include <cstring>
#include <cmath>
#include <cstdio>
#include <cerrno>

#include "commonmacros.h"
#include "const.h"

#include "server.h"

#define SERVER
#include "xpconfig.h"
#include "serverconst.h"
#include "map.h"
#include "score.h"
#include "bit.h"
#include "netserver.h"
#include "saudio.h"
#include "xperror.h"
#include "object.h"
#include "xpmath.h"
#include "robot.h"

void Race_game_over(void)
{
    player_t *pl;
    int i,
        j,
        k,
        bestlap = 0,
        num_best_players = 0,
        num_active_players = 0,
        num_ordered_players = 0;
    int *order;
    char msg[MSG_LEN];

    /*
     * Reassign players's starting posisitions based upon
     * personal best lap times.
     */
    if ((order = (int *)malloc(NumPlayers * sizeof(int))) != NULL)
    {
        for (i = 0; i < NumPlayers; i++)
        {
            pl = Player_by_index(i);
            if (Player_is_tank(pl))
            {
                continue;
            }
            if (BIT(pl->obj_status, PAUSE) || (BIT(pl->obj_status, GAME_OVER) && pl->mychar == 'W') || pl->best_lap <= 0)
            {
                j = i;
            }
            else
            {
                for (j = 0; j < i; j++)
                {
                    if (pl->best_lap < PlayersArray[order[j]]->best_lap)
                    {
                        break;
                    }
                    if (BIT(PlayersArray[order[j]]->obj_status, PAUSE) || (BIT(PlayersArray[order[j]]->obj_status, GAME_OVER) && PlayersArray[order[j]]->mychar == 'W'))
                    {
                        break;
                    }
                }
            }
            for (k = i - 1; k >= j; k--)
            {
                order[k + 1] = order[k];
            }
            order[j] = i;
            num_ordered_players++;
        }
        for (i = 0; i < num_ordered_players; i++)
        {
            pl = PlayersArray[order[i]];
            if (pl->home_base != world->baseorder[i].base_idx)
            {
                pl->home_base = world->baseorder[i].base_idx;
                for (j = 0; j < NumPlayers; j++)
                {
                    if (PlayersArray[j]->conn != NULL)
                    {
                        Send_base(PlayersArray[j]->conn,
                                  pl->id,
                                  pl->home_base);
                    }
                }
                if (Player_is_paused(pl))
                    Go_home(pl);
            }
        }
        free(order);
    }

    for (i = NumPlayers - 1; i >= 0; i--)
    {
        pl = Player_by_index(i);
        CLR_BIT(pl->obj_status, RACE_OVER | FINISH);
        if (BIT(pl->obj_status, PAUSE) || (BIT(pl->obj_status, GAME_OVER) && pl->mychar == 'W') || Player_is_tank(pl))
        {
            continue;
        }
        num_active_players++;

        /* Kill any remaining players */
        if (!BIT(pl->obj_status, GAME_OVER))
            Kill_player(pl, false);
        else
            Player_death_reset(pl, false);
        if (pl != Player_by_index(i))
            continue;

        if ((pl->best_lap < bestlap || bestlap == 0) &&
            pl->best_lap > 0)
        {
            bestlap = pl->best_lap;
            num_best_players = 0;
        }
        if (pl->best_lap == bestlap)
            num_best_players++;
    }

    /* If someone completed a lap */
    if (bestlap > 0)
    {
        for (i = 0; i < NumPlayers; i++)
        {
            pl = Player_by_index(i);
            if (BIT(pl->obj_status, PAUSE) || (BIT(pl->obj_status, GAME_OVER) && pl->mychar == 'W') || Player_is_tank(pl))
            {
                continue;
            }
            if (pl->best_lap == bestlap)
            {
                Set_message_f("%s %s the best lap time of %.2fs",
                              pl->name,
                              (num_best_players == 1) ? "had" : "shares",
                              (double)bestlap / FPS);
                Score(pl, 5 + num_active_players, pl->pos,
                      (num_best_players == 1) ? "[Fastest lap]" : "[Joint fastest lap]");
            }
        }

        updateScores = true;
    }
    else if (num_active_players > NumRobots)
        Set_message("No-one even managed to complete one lap, you should be "
                    "ashamed of yourselves.");

    Count_rounds();
    Reset_all_players();
}

void Player_reset_timing(player_t *pl)
{
    pl->round = 0;
    pl->check = 0;
    pl->time = 0;
    pl->best_lap = 0;
    pl->last_lap = 0;
    pl->last_lap_time = 0;
}

void Player_pass_checkpoint(player_t *pl)
{
    int j;

    if (pl->check == 0)
    {
        pl->round++;
        pl->last_lap_time = pl->time - pl->last_lap;
        if ((pl->best_lap > pl->last_lap_time || pl->best_lap == 0) && pl->time != 0 && pl->round != 1)
        {
            pl->best_lap = pl->last_lap_time;
        }
        pl->last_lap = pl->time;
        if (pl->round > options.raceLaps)
        {
            if (options.ballrace)
            {
                /* Balls are made unowned when their owner finishes the race
                   This way, they can be reused by other players */
                for (j = 0; j < NumObjs; j++)
                {
                    if (Obj[j]->type == OBJ_BALL)
                    {
                        ballobject_t *ball = BALL_PTR(Obj[j]);

                        if (ball->ball_owner == pl->id)
                            ball->ball_owner = NO_ID;
                    }
                }
            }
            Player_death_reset(pl, false);
            pl->mychar = 'D';
            SET_BIT(pl->obj_status, GAME_OVER | FINISH);
            Set_message_f("%s finished the race. Last lap time: %.2fs. "
                          "Personal race best lap time: %.2fs.",
                          pl->name,
                          (double)pl->last_lap_time / FPS,
                          (double)pl->best_lap / FPS);
        }
        else if (pl->round > 1)
        {
            Set_message_f("%s completes lap %d in %.2fs. "
                          "Personal race best lap time: %.2fs.",
                          pl->name,
                          pl->round - 1,
                          (double)pl->last_lap_time / FPS,
                          (double)pl->best_lap / FPS);
        }
        else
            Set_message_f("%s starts lap 1 of %d.",
                          pl->name, options.raceLaps);
    }

    if (++pl->check == world->NumChecks)
        pl->check = 0;
    pl->last_check_dir = pl->dir;

    updateScores = true;
}
