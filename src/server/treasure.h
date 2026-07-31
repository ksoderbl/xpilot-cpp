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

// TODO: Define treasure_t or Treasure here
#include "map.h"
#include "object.h"
#include "polygon.h"

void Treasure_init(void);
void Make_treasure_ball(treasure_t *t);
void Ball_hits_goal2(ballobject_t *ball, group_t *groupptr);
void Ball_is_replaced2(ballobject_t *ball);
void Ball_is_destroyed2(ballobject_t *ball);
bool Balltarget_hitfunc(group_t *groupptr, const move_t *move);

int Punish_team1(player_t *pl, treasure_t *td, clpos_t pos);
void Ball_is_replaced1(ballobject_t *ball);
void Ball_is_destroyed1(ballobject_t *ball);
treasure_t *treasureXY(int x, int y);
