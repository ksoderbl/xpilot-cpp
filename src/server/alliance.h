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

#include "player.h"

int Invite_player(Player *pl, Player *ally);
int Cancel_invitation(Player *pl);
int Refuse_alliance(Player *pl, Player *ally);
int Refuse_all_alliances(Player *pl);
int Accept_alliance(Player *pl, Player *ally);
int Accept_all_alliances(Player *pl);
int Get_alliance_member_count(int id);
void Player_join_alliance(Player *pl, Player *ally);
void Dissolve_all_alliances(void);
int Leave_alliance(Player *pl);
void Alliance_player_list(Player *pl);
