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

#pragma once

#include <string>

#include "player.h"

#define CANNON_SCORE -1436
#define WALL_SCORE 2000

#define RATE_SIZE 20
#define RATE_RANGE 1024

/* score.c */

void Score(Player *pl, double points, clpos_t pos, std::string msg);
int Rate(int winner, int loser);

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
void Score_players(Player *winner_pl, double winner_score,
                   std::string winner_msg, Player *loser_pl,
                   double loser_score, std::string loser_msg);

double Get_Score(Player *pl);

typedef enum
{
    SCORE_CANNON_KILL,
    SCORE_WALL_DEATH,
    SCORE_COLLISION,
    SCORE_ROADKILL,
    SCORE_BALL_KILL,
    SCORE_HIT_MINE,
    SCORE_EXPLOSION,
    SCORE_ASTEROID_KILL,
    SCORE_ASTEROID_DEATH,
    SCORE_SHOT_DEATH,
    SCORE_LASER,
    SCORE_TARGET,
    SCORE_TREASURE,
    SCORE_SELF_DESTRUCT,
    SCORE_SHOVE_KILL,
    SCORE_SHOVE_DEATH
} scoretype_t;

void Handle_Scoring(scoretype_t st, Player *killer, Player *victim,
                    void *extra, const char *somemsg);
