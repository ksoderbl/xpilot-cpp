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

#include "map.h"
#include "object.h"
#include "polygon.h"

void Target_update(void);

void Target_update(void);
void Object_hits_target1(object_t *obj, target_t *targ, double player_cost);
void Object_hits_target2(object_t *obj, target_t *targ, double player_cost);
hitmask_t Target_hitmask(target_t *targ);
void Target_set_hitmask(int group, target_t *targ);
void Target_init(void);
void World_restore_target(world_t *world, target_t *targ);
void World_remove_target(world_t *world, target_t *targ);
