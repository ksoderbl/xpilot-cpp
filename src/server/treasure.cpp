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
#include <cstdio>
#include <climits>
#include <cmath>

#include "server.h"

#define SERVER
#include "xpconfig.h"
#include "serverconst.h"
#include "global.h"
#include "saudio.h"
#include "score.h"
#include "object.h"
#include "xpmath.h"

int Punish_team(player_t *pl, treasure_t *td, treasure_t *tt)
{
    static char msg[MSG_LEN];
    // treasure_t *td = &world->treasures[t_destroyed];
    // treasure_t *tt = &world->treasures[t_target];
    // player_t *pl = PlayersArray[ind];
    int i;
    int win_score = 0, lose_score = 0;
    int win_team_members = 0, lose_team_members = 0;
    int somebody_flag = 0;
    int sc, por;

    Check_team_members(td->team);
    if (td->team == pl->team)
        return 0;

    if (BIT(world->rules->mode, TEAM_PLAY))
    {
        for (i = 0; i < NumPlayers; i++)
        {
            player_t *pl_i = Player_by_index(i);

            if (Player_is_tank(pl_i) || (BIT(pl_i->obj_status, PAUSE) && pl_i->count <= 0) || (BIT(pl_i->obj_status, GAME_OVER) && pl_i->mychar == 'W' && pl_i->score == 0))
                continue;
            if (pl_i->team == td->team)
            {
                lose_score += pl_i->score;
                lose_team_members++;
                if (BIT(pl_i->obj_status, GAME_OVER) == 0)
                    somebody_flag = 1;
            }
            else if (pl_i->team == tt->team)
            {
                win_score += pl_i->score;
                win_team_members++;
            }
        }
    }

    sound_play_all(DESTROY_BALL_SOUND);
    sprintf(msg, " < %s's (%d) team has destroyed team %d treasure >",
            pl->name, pl->team, td->team);
    Set_message(msg);

    if (!somebody_flag)
    {
        Score(pl, Rate(pl->score, CANNON_SCORE) / 2,
              tt->pos, "Treasure:");
        return 0;
    }

    td->destroyed++;
    world->teams[td->team].TreasuresLeft--;
    world->teams[tt->team].TreasuresDestroyed++;

    sc = 3 * Rate(win_score, lose_score);
    por = (sc * lose_team_members) / (2 * win_team_members + 1);

    for (i = 0; i < NumPlayers; i++)
    {
        player_t *pl_i = Player_by_index(i);

        if (Player_is_tank(pl_i) ||
            (BIT(pl_i->obj_status, PAUSE) && pl_i->count <= 0) ||
            (BIT(pl_i->obj_status, GAME_OVER) && pl_i->mychar == 'W' && pl_i->score == 0))
            continue;
        if (pl_i->team == td->team)
        {
            Score(pl_i, -sc, tt->pos, "Treasure: ");
            if (options.treasureKillTeam)
                SET_BIT(pl_i->obj_status, KILLED);
        }
        else if (pl_i->team == tt->team &&
                 (pl_i->team != TEAM_NOT_SET || pl_i->id == pl->id))
            Score(pl_i, (pl_i->id == pl->id ? 3 * por : 2 * por), tt->pos, "Treasure: ");
    }

    if (options.treasureKillTeam)
    {
        pl->kills++;
    }

    updateScores = true;

    return 1;
}
