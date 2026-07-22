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

#include "click.h"
#include "object.h"
#include "player.h"
#include "polygon.h"

#define WORMHOLE_RADIUS ((BLOCK_CLICKS / 2) - 1)

/*
 * Prototypes for wormhole.cpp
 */
bool Initiate_hyperjump(player_t *pl);
void Do_warp(player_t *pl);
void Player_warp(player_t *pl);
void Player_finish_warp(player_t *pl);
void Object_warp(object_t *obj);
void Object_finish_warp(object_t *obj);
void Object_hits_wormhole1(object_t *obj, int ind);
void Object_hits_wormhole2(object_t *obj, int ind);
hitmask_t Wormhole_hitmask(wormhole_t *wormhole);
bool Wormhole_hitfunc(group_t *groupptr, const move_t *move);
bool Verify_wormhole_consistency(void);
