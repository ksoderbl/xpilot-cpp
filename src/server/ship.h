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

void Make_thrust_sparks(player_t *pl);
void Recoil(object_t *ship, object_t *shot);
void Record_shove(player_t *pl, player_t *pusher, long time);
void Delta_mv(object_t *ship, object_t *obj);
void Delta_mv_elastic(object_t *obj1, object_t *obj2);
void Delta_mv_partly_elastic(object_t *obj1, object_t *obj2, double elastic);
void Obj_repel(object_t *obj1, object_t *obj2, int repel_dist);
void Item_damage(player_t *pl, double prob);
void Tank_handle_detach(player_t *pl);
void Update_tanks(pl_fuel_t *);
void Player_add_fuel(player_t *pl, double amount);
void Place_item(player_t *pl, int type);
int Choose_random_item(void);
void Tractor_beam(player_t *pl);
void General_tractor_beam(int id, clpos_t pos,
                          int items, player_t *victim, bool pressor);
void Place_mine(player_t *pl);
void Place_moving_mine(player_t *pl);
void Place_general_mine(int id, int team, int status,
                        clpos_t pos, vector_t vel, modifiers_t mods);
void Detonate_mines(player_t *pl);
char *Describe_shot(int type, int status, modifiers_t mods, int hit);
void Fire_ecm(player_t *pl);
void Fire_general_ecm(int id, int team, clpos_t pos);
void Update_connector_force(ballobject_t *ball);
void Fire_shot(player_t *pl, int type, int dir);
void Fire_general_shot(int id, int team, bool cannon,
                       clpos_t pos, int type, int dir,
                       modifiers_t mods, int target_id);
void Fire_normal_shots(player_t *pl);
void Fire_main_shot(player_t *pl, int type, int dir);
void Fire_left_shot(player_t *pl, int type, int dir, int gun);
void Fire_right_shot(player_t *pl, int type, int dir, int gun);
void Fire_left_rshot(player_t *pl, int type, int dir, int gun);
void Fire_right_rshot(player_t *pl, int type, int dir, int gun);

void Team_immunity_init(void);
void Hitmasks_init(void);

void Delete_shot(int ind);
void Fire_laser(player_t *pl);
void Fire_general_laser(int id, int team, clpos_t pos, int dir, modifiers_t mods);
void Do_deflector(player_t *pl);
void Do_transporter(player_t *pl);
void Do_general_transporter(int id, clpos_t pos, player_t *victim, int *itemp, double *amount);
void do_hyperjump(player_t *pl);
void do_lose_item(player_t *pl);
void Update_torpedo(torpobject_t *torp);
void Update_missile(missileobject_t *shot);
void Update_mine(mineobject_t *mine);
void Make_debris(clpos_t pos,
                 vector_t vel,
                 int owner_id,
                 int owner_team,
                 int type,
                 double mass,
                 int status,
                 int color,
                 int radius,
                 int num_debris,
                 int min_dir, int max_dir,
                 double min_speed, double max_speed,
                 double min_life, double max_life);
void Make_wreckage(clpos_t pos,
                   vector_t vel,
                   int oner_id,
                   int owner_team,
                   double min_mass, double max_mass,
                   double total_mass,
                   int status,
                   int max_wreckage,
                   int min_dir, int max_dir,
                   double min_speed, double max_speed,
                   double min_life, double max_life);
void Make_item(clpos_t pos,
               vector_t vel,
               int type, int num_per_pack,
               int status);
void Explode_fighter(player_t *pl);
void Throw_items(player_t *pl);
void Detonate_items(player_t *pl);
void add_temp_wormholes(int xin, int yin, int xout, int yout);
void remove_temp_wormhole(int ind);