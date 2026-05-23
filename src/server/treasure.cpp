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

#include "xperror.h"

#include "server.h"

#define SERVER
#include "xpconfig.h"
#include "serverconst.h"

#include "saudio.h"
#include "score.h"
#include "object.h"
#include "xpmath.h"

void Make_treasure_ball(treasure_t *t)
{
    warn("Make_treasure_ball: treasure is %p", t);

    ballobject_t *ball;
    clpos_t pos = t->pos;

    if (t->empty)
        return;
    if (t->have)
    {
        xpprintf("%s Failed Make_treasure_ball(treasure=%p):\n",
                 showtime(), (long)t);
        xpprintf("\ttreasure: destroyed = %d, team = %d, have = %d\n",
                 t->destroyed, t->team, t->have);
        return;
    }

    if ((ball = BALL_PTR(Object_allocate())) == NULL)
        return;

    ball->life = LONG_MAX;
    ball->mass = options.ballMass;
    ball->vel.x = 0; /* make the ball stuck a little */
    ball->vel.y = 0; /* longer to the ground */
    ball->acc.x = 0;
    ball->acc.y = 0;
    Object_position_init_clpos(OBJ_PTR(ball), pos);
    ball->id = NO_ID;
    ball->ball_owner = NO_ID;
    ball->team = t->team;
    ball->type = OBJ_BALL;
    ball->color = WHITE;
    ball->pl_range = BALL_RADIUS;
    ball->pl_radius = BALL_RADIUS;
    Mods_clear(&ball->mods);
    ball->obj_status = RECREATE;
    ball->ball_treasure = t;
    ball->ball_treasure_copy = t;
    warn("Make_treasure_ball: ball_treasure is      %p", ball->ball_treasure);
    warn("Make_treasure_ball: ball_treasure_copy is %p", ball->ball_treasure_copy);
    Cell_add_object(OBJ_PTR(ball));
    ball->ball_loose_ticks = 0;
    ball->ball_style = t->ball_style;
    t->have = true;
}

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

            if (Player_is_tank(pl_i) || (BIT(pl_i->obj_status, PAUSE) && pl_i->count <= 0) || (BIT(pl_i->obj_status, GAME_OVER) && pl_i->mychar == 'W' && Get_Score(pl_i) == 0))
                continue;
            if (pl_i->team == td->team)
            {
                lose_score += Get_Score(pl_i);
                lose_team_members++;
                if (BIT(pl_i->obj_status, GAME_OVER) == 0)
                    somebody_flag = 1;
            }
            else if (pl_i->team == tt->team)
            {
                win_score += Get_Score(pl_i);
                win_team_members++;
            }
        }
    }

    sound_play_all(DESTROY_BALL_SOUND);
    Set_message_f(" < %s's (%d) team has destroyed team %d treasure >",
                  pl->name, pl->team, td->team);

    if (!somebody_flag)
    {
        Score(pl, Rate(Get_Score(pl), CANNON_SCORE) / 2,
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
            (BIT(pl_i->obj_status, GAME_OVER) && pl_i->mychar == 'W' && Get_Score(pl_i) == 0))
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
        pl->kills++;

    updateScores = true;

    return 1;
}

void Ball_hits_goal(ballobject_t *ball, group_t *gp)
{
}

/*
 * Here follows some hit functions, used in the walls code to determine
 * if some object can hit some polygon. The arguments for a hit function
 * are: the pointer to the polygon group that the polygon belongs to and
 * the pointer to the move. The hit function checks if the move can hit
 * the polygon group.
 * NOTE: hit functions should not have any side effects (i.e. change
 * anything) unless you know what you are doing.
 */

// extern bool in_legacy_mode_ball_hack;
/*
 * This function is called when something would hit a balltarget.
 * The function determines if it hits or not.
 */
bool Balltarget_hitfunc(group_t *gp, const move_t *move)
{
    const ballobject_t *ball = NULL;

    /* this can happen if is_inside is called for a balltarget with
       a NULL obj */
    if (move->obj == NULL)
        return true;

    assert(move->obj->type == OBJ_BALL);

    ball = (const ballobject_t *)move->obj;

    if (ball->ball_owner == NO_ID)
        return true;

    // TODO

    // if (in_legacy_mode_ball_hack)
    //     return true;

    // if (BIT(world->rules->mode, TEAM_PLAY))
    // {
    //     /*
    //      * The only case a ball does not hit a balltarget is when
    //      * the ball and the target are of the same team, but the
    //      * owner is not.
    //      */
    //     if (ball->ball_treasure->team != options.specialBallTeam /* kps - ? */
    //         /* khs - "special" ball and "special" treasure have the same "team" */
    //         /* the player/team that gets this ball into this treasure scores    */
    //         /* against all other teams, therefore the ball must be able to hit - ok? */
    //         && ball->ball_treasure->team == gp->team && Player_by_id(ball->ball_owner)->team != gp->team)
    //         return false;
    //     return true;
    // }

    // /* not teamplay */

    // /* kps - fix this */

    /* allow grabbing of ball */
    return false;
}
