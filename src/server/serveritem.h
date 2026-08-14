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

#include "item.h"
#include "player.h"

void Item_damage(Player *pl, double prob);
int Choose_random_item(void);
void Place_item(Player *pl, int item);
void Make_item(clpos_t pos, vector_t vel,
               int type, int num_per_pack, int status);
void Throw_items(Player *pl);
void Detonate_items(Player *pl);
void Tractor_beam(Player *pl);
void General_tractor_beam(int id, clpos_t pos,
                          int items, Player *victim, bool pressor);
void Do_deflector(Player *pl);
void Do_transporter(Player *pl);
void Do_general_transporter(int id, clpos_t pos,
                            Player *victim, int *itemp, double *amountp);
void do_hyperjump(Player *pl);
void do_lose_item(Player *pl);
void Fire_general_ecm(int id, int team, clpos_t pos);
void Fire_ecm(Player *pl);
Item_t Item_by_option_name(const char *name);
int IsOffensiveItem(enum Item i);
int IsDefensiveItem(enum Item i);
int CountOffensiveItems(Player *pl);
int CountDefensiveItems(Player *pl);
