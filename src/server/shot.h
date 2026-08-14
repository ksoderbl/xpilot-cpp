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

void Place_mine(Player *pl);
void Place_moving_mine(Player *pl);
void Place_general_mine(int id, int team, int status,
                        clpos_t pos, vector_t vel, modifiers_t mods);
void Detonate_mines(Player *pl);
char *Describe_shot(int type, int status, modifiers_t mods, int hit);
void Update_connector_force(ballobject_t *ball);
void Fire_shot(Player *pl, int type, int dir);
void Fire_general_shot(int id, int team, bool cannon,
                       clpos_t pos, int type, int dir,
                       modifiers_t mods, int target_id);
void Fire_normal_shots(Player *pl);
void Fire_main_shot(Player *pl, int type, int dir);
void Fire_left_shot(Player *pl, int type, int dir, int gun);
void Fire_right_shot(Player *pl, int type, int dir, int gun);
void Fire_left_rshot(Player *pl, int type, int dir, int gun);
void Fire_right_rshot(Player *pl, int type, int dir, int gun);

void Delete_shot(int ind);