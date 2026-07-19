/*
 * XPilot, a multiplayer gravity war game.
 *
 * Copyright (C) 1991-2001 by
 *
 *      Bjørn Stabell
 *      Ken Ronny Schouten
 *      Bert Gijsbers
 *      Dick Balaska
 *
 * Copyright (C) 2000-2004 Uoti Urpala
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

#include "click.h"
#include "object.h"
#include "player.h"
#include "polygon.h"
#include "move.h"

void Move_point(const move_t *move, struct collans *answer);

void Groups_init(void);
void Walls_init2(void);
void Move_init2(void);
void Move_object2(object_t *obj);
void Move_player2(player_t *pl);
void Turn_player2(player_t *pl, bool push);
int is_inside(int x, int y, hitmask_t hitmask, const object_t *obj);
int shape_is_inside(int cx, int cy, hitmask_t hitmask, const object_t *obj,
                    shape_t *s, int dir);
int Polys_to_client(uint8_t **);
void Player_crash2(player_t *pl, int crashtype, int mapobj_ind, int pt);
void Object_crash2(object_t *obj, int crashtype, int mapobj_ind);
void Ball_line_init2(void);
