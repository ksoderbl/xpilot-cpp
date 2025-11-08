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
#include <cmath>
#include <climits>

#include "click.h"

#include "server.h"

#define SERVER
#include "version.h"
#include "xpconfig.h"
#include "serverconst.h"
#include "score.h"
#include "netserver.h"
#include "player.h"

void Score(player_t *pl, int points, clpos_t pos, const char *msg)
{
    pl->score += (points);

    if (pl->conn != NULL)
        Send_score_object(pl->conn, points, pos, msg);
    updateScores = true;
}

int Rate(int winner, int loser)
{
    int t;

    t = ((RATE_SIZE / 2) * RATE_RANGE) / (ABS(loser - winner) + RATE_RANGE);
    if (loser > winner)
        t = RATE_SIZE - t;
    return t;
}

/*
 * Cause 'winner' to get 'winner_score' points added with message
 * 'winner_msg', and similarly with the 'loser' and equivalent
 * variables.
 *
 * In general the winner_score should be positive, and the loser_score
 * negative, but this need not be true.
 *
 * If the winner and loser players are on the same team, the scores are
 * made negative, since you shouldn't gain points by killing team members,
 * or being killed by a team member (it is both players faults).
 *
 * KK 28-4-98: Same for killing your own tank.
 * KK 7-11-1: And for killing a member of your alliance
 */
void Score_players(player_t *winner_pl, int winner_score, char *winner_msg,
                   player_t *loser_pl, int loser_score, char *loser_msg)
{
    if (Players_are_teammates(winner_pl, loser_pl) ||
        (winner_pl->alliance != ALLIANCE_NOT_SET && winner_pl->alliance == loser_pl->alliance) ||
        (Player_is_tank(loser_pl) && loser_pl->lock.pl_id == winner_pl->id))
    {
        if (winner_score > 0)
            winner_score = -winner_score;
        if (loser_score > 0)
            loser_score = -loser_score;
    }
    Score(winner_pl, winner_score, loser_pl->pos, winner_msg);
    Score(loser_pl, loser_score, loser_pl->pos, loser_msg);
}
