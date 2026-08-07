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

#include "xpconfig.h"
#include "commonproto.h"
#include "xperror.h"

#include "cell.h"
#include "frame.h"
#include "server.h"

#include "serverconst.h"

#include "saudio.h"
#include "score.h"
#include "object.h"
#include "rank.h"

void Make_treasure_ball(treasure_t *t)
{
    // warn("Make_treasure_ball: treasure is %p", t);

    ballobject_t *ball;
    clpos_t pos = t->pos;

    if (t->empty)
        return;
    if (t->have)
    {
        printf("%s Failed Make_treasure_ball(treasure=%ld):\n",
               showtime(), (long)t);
        printf("\ttreasure: destroyed = %d, team = %d, have = %d\n",
               t->destroyed, t->team, t->have);
        return;
    }

    if ((ball = BALL_PTR(Object_allocate())) == nullptr)
        return;

    ball->obj_life = LONG_MAX;
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
    // warn("Make_treasure_ball: ball_treasure is      %p", ball->ball_treasure);
    // warn("Make_treasure_ball: ball_treasure_copy is %p", ball->ball_treasure_copy);
    ball->ball_loose_ticks = 0;
    ball->ball_style = t->ball_style;
    Cell_add_object(OBJ_PTR(ball));

    t->have = true;
}

void Treasure_init(void)
{
    int i;
    for (i = 0; i < Num_treasures(); i++)
        Make_treasure_ball(Treasure_by_index(i));
}

/*
 * Ball has been replaced back in the hoop from whence
 * it came. The player must be from the same team as the ball,
 * otherwise Bounce_object() wouldn't have been called. It
 * should be replaced into the hoop without exploding and
 * the player gets some points.
 */
void Ball_is_replaced2(ballobject_t *ball)
{
    player_t *pl = Player_by_id(ball->ball_owner);

    ball->obj_life = 0.0;
    SET_BIT(ball->obj_status, (NOEXPLOSION | RECREATE));

    if (!options.zeroSumScoring)
        Score(pl, 2.0, ball->pos, "Treasure: ");
    Set_message_f(" < %s (team %d) has replaced the treasure >",
                  pl->name.c_str(), pl->team);
    Rank_saved_ball(pl);
}

/*
 * Ball has been brought back to home treasure.
 * The team should be punished.
 */
void Ball_is_destroyed2(ballobject_t *ball)
{
    player_t *pl = Player_by_id(ball->ball_owner);
    double ticks = ball->ball_loose_ticks;
    int frames = (int)(ticks / timeStep + .5);
    // double seconds = ticks / options.gameSpeed; // TODO
    double seconds = 0.0; // TODO

    Set_message_f(" < The ball was loose for %d frames / "
                  "%.2f ticks (best %.2f) / %.2fs >",
                  frames, ticks, Rank_get_best_ballrun(pl), seconds);
    Rank_ballrun(pl, ticks);
}

// Punish_team1 = xpilot 4.5.5, Punish_team2 = NG
static int Punish_team2(player_t *pl, treasure_t *td, clpos_t pos)
{
    world_t *world = &World;
    double win_score = 0.0, lose_score = 0.0;
    int i, win_team_members = 0, lose_team_members = 0;
    bool somebody = false;

    Check_team_members(td->team);
    if (td->team == pl->team)
        return 0;

    Handle_Scoring(SCORE_TREASURE, pl, nullptr, td, nullptr);

    if (Team_play(world))
    {
        for (i = 0; i < NumPlayers; i++)
        {
            player_t *pl_i = Player_by_index(i);

            if (Player_is_tank(pl_i) ||
                (Player_is_paused(pl_i) && pl_i->pause_count <= 0) ||
                Player_is_waiting(pl_i))
                continue;
            if (pl_i->team == td->team)
            {
                lose_score += Get_Score(pl_i);
                lose_team_members++;
                if (!Player_is_dead(pl_i))
                    somebody = true;
            }
            else if (pl_i->team == pl->team)
            {
                win_score += Get_Score(pl_i);
                win_team_members++;
            }
        }
    }

    if (!somebody)
    {
        return 0;
    }
    sound_play_all(DESTROY_BALL_SOUND);
    Set_message_f(" < %s's (%d) team has destroyed team %d treasure >",
                  pl->name.c_str(), pl->team, td->team);
    updateScores = true;

    return 1;
}

void Ball_hits_goal2(ballobject_t *ball, group_t *gp)
{
    world_t *world = &World;
    player_t *owner;
    treasure_t *td;
    int i;

    if (gp->type != TREASURE)
    {
        warn("Ball_hits_goal2: not a treasure! Possible map bug.");
        return;
    }

    /*
     * Player already quit ?
     */
    if (ball->ball_owner == NO_ID)
    {
        SET_BIT(ball->obj_status, (NOEXPLOSION | RECREATE));
        return;
    }

    /*
     * If it's not team play, nothing interesting happens.
     */
    if (!Team_play(world))
        return;

    td = ball->ball_treasure;
    owner = Player_by_id(ball->ball_owner);
    if (td->team == gp->team && td->team == owner->team)
    {
        Ball_is_replaced2(ball);
        return;
    }
    if (gp->team == owner->team &&
        td->team != options.specialBallTeam)
    {
        treasure_t *tt = Treasure_by_index(gp->mapobj_ind);

        Ball_is_destroyed2(ball);

        if (options.captureTheFlag && !tt->have && !tt->empty)
            Set_message(" < The treasure must be safe before you "
                        "can cash an opponent's! >");
        else if (Punish_team2(owner, td, ball->pos))
            CLR_BIT(ball->obj_status, RECREATE);
        return;
    }

    /* {KS} mods for team 'options.specialBallTeam' treasures start here*/
    /* Ball and box have to be in 'specialBallTeam'. If such a ball     */
    /* is scored by any player, it counts as if he had scored one ball */
    /* against every single opponent team */
    /* This means that (nonlinear scoring) the first team is punished */
    /* slightly more then the last team in the loop - fix for that?*/

    /* gp->team: team of treasure box     td->team: team of ball */
    if (gp->team == options.specialBallTeam &&
        td->team == options.specialBallTeam)
    {
        int opponent_teams = 0;
        Ball_is_destroyed2(ball);

        for (i = 0; i < MAX_TEAMS; i++)
        {

            if (World.teams[i].NumMembers == 0 || i == owner->team)
                continue;
            opponent_teams++;
            td->team = i; /* give ball to team that has to be punished*/
            if (Punish_team2(owner, td, ball->pos))
            {
                CLR_BIT(ball->obj_status, RECREATE);
                /*undo treasure counts from Punish_team so we don't
                  have to touch that function and possibly break it*/
                World.teams[owner->team].TreasuresDestroyed--;
                World.teams[td->team].TreasuresLeft++;
            }
        }
        td->team = options.specialBallTeam;
        World.teams[options.specialBallTeam].TreasuresLeft--;
        World.teams[owner->team].TreasuresDestroyed++;
        World.teams[options.specialBallTeam].TreasuresDestroyed++;

        if (!opponent_teams)
        {
            SET_BIT(ball->obj_status, RECREATE);
            if (Punish_team2(owner, td, ball->pos))
                World.teams[options.specialBallTeam].TreasuresLeft++;
        }
        return;
    }

    if (td->team == options.specialBallTeam)
    {
        Ball_is_destroyed2(ball);
        td->team = gp->team; /* give ball to team that has to be punished*/
        if (Punish_team2(owner, td, ball->pos))
        {
            CLR_BIT(ball->obj_status, RECREATE);
            /*undo treasure counts from Punish_team so we don't
              have to touch that function and possibly break it*/
            World.teams[td->team].TreasuresLeft++;
            World.teams[options.specialBallTeam].TreasuresLeft--;
        }

        td->team = options.specialBallTeam;
        return;
    }
    /* {KS} mods for special treasures stop here*/
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
    const ballobject_t *ball = nullptr;

    /* this can happen if is_inside is called for a balltarget with
       a nullptr obj */
    if (move->obj == nullptr)
        return true;

    assert(move->obj->type == OBJ_BALL);

    ball = (const ballobject_t *)move->obj;

    if (ball->ball_owner == NO_ID)
        return true;

    // TODO

    // if (in_legacy_mode_ball_hack)
    //     return true;

    // if (Team_play(world))
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

/*
 * Ball has been replaced back in the hoop from whence
 * it came.  If the player is on the same team as the
 * hoop, then it should be replaced into the hoop without
 * exploding and gets the player some points.  Otherwise
 * nothing interesting happens.
 */
void Ball_is_replaced1(ballobject_t *ball)
{
    player_t *pl = Player_by_id(ball->ball_owner);

    ball->obj_life = 0.0;
    SET_BIT(ball->obj_status, (NOEXPLOSION | RECREATE));

    Score(pl, 5, ball->pos, "Treasure: ");
    Set_message_f(" < %s (team %d) has replaced the treasure >", pl->name.c_str(), pl->team);
    Rank_saved_ball(pl);
}

/*
 * Ball has been brought back to home treasure.
 * The team should be punished.
 */
void Ball_is_destroyed1(ballobject_t *ball)
{
    player_t *pl = Player_by_id(ball->ball_owner);
    double ticks = ball->ball_loose_ticks;

    // int frames = (int)(ticks / timeStep + .5);
    // warn("LONG_MAX = %ld", LONG_MAX);
    // warn("ball->obj_life = %f", ball->obj_life);
    // warn("ball->ball_loose_ticks = %f", ball->ball_loose_ticks);

    Set_message_f(" < The ball was loose for %f frames >", ball->ball_loose_ticks);
    Rank_ballrun(pl, ticks);
}

// pl = player who cashed ball
// td  = destroyed treasure
// pos = ball position
int Punish_team1(player_t *pl, treasure_t *td, clpos_t pos)
{
    world_t *world = &World;
    static char msg[MSG_LEN];
    int i;
    int win_score = 0, lose_score = 0;
    int win_team_members = 0, lose_team_members = 0;
    int sc, por;
    bool somebody = false;

    Check_team_members(td->team);
    if (td->team == pl->team)
        return 0;

    if (Team_play(world))
    {
        for (i = 0; i < NumPlayers; i++)
        {
            player_t *pl_i = Player_by_index(i);

            if (Player_is_tank(pl_i) ||
                (Player_is_paused(pl_i) && pl_i->pause_count <= 0) ||
                Player_is_waiting(pl_i))
                continue;
            if (pl_i->team == td->team)
            {
                lose_score += Get_Score(pl_i);
                lose_team_members++;
                if (!Player_is_dead(pl_i))
                    somebody = true;
            }
            else if (pl_i->team == pl->team)
            {
                win_score += Get_Score(pl_i);
                win_team_members++;
            }
        }
    }

    sound_play_all(DESTROY_BALL_SOUND);
    Set_message_f(" < %s's (%d) team has destroyed team %d treasure >",
                  pl->name.c_str(), pl->team, td->team);

    if (!somebody)
    {
        Score(pl, Rate(Get_Score(pl), CANNON_SCORE) / 2, pos, "Treasure:");
        return 0;
    }

    td->destroyed++;
    World.teams[td->team].TreasuresLeft--;
    World.teams[pl->team].TreasuresDestroyed++;

    sc = 3 * Rate(win_score, lose_score);
    por = (sc * lose_team_members) / (2 * win_team_members + 1);

    for (i = 0; i < NumPlayers; i++)
    {
        player_t *pl_i = Player_by_index(i);

        if (Player_is_tank(pl_i) ||
            (Player_is_paused(pl_i) && pl_i->pause_count <= 0) ||
            Player_is_waiting(pl_i))
            continue;
        if (pl_i->team == td->team)
        {
            Score(pl_i, -sc, pos, "Treasure: ");
            if (options.treasureKillTeam)
                Player_set_state(pl_i, PL_STATE_KILLED);
        }
        else if (pl_i->team == pl->team &&
                 (pl_i->team != TEAM_NOT_SET || pl_i->id == pl->id))
            Score(pl_i, (pl_i->id == pl->id ? 3 * por : 2 * por), pos, "Treasure: ");
    }

    if (options.treasureKillTeam)
        pl->kills++;

    updateScores = true;

    return 1;
}

// Returns pointer to treasure at block with coordinates (x, y).
// This assumes the map is block based.
treasure_t *treasureXY(int x, int y)
{
    for (int i = 0; i < Num_treasures(); i++)
    {
        treasure_t *treasure = Treasure_by_index(i);
        blkpos_t blk = Clpos_to_blkpos(treasure->pos);
        if (blk.bx == x && blk.by == y)
            return treasure;
    }
    return nullptr;
}
