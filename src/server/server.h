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

#ifndef SERVER_H
#define SERVER_H

#include <cstdint>

#include "click.h"
#include "list.h"
#include "shipshape.h"

#include "map.h"
#include "object.h"
#include "player.h"
#include "polygon.h"

extern shape_t ball_wire, wormhole_wire, filled_wire;

static inline vector_t World_gravity(clpos_t pos)
{
    return world->gravity[CLICK_TO_BLOCK(pos.cx)][CLICK_TO_BLOCK(pos.cy)];
}

enum TeamPickType
{
    PickForHuman = 1,
    PickForRobot = 2
};

#define APPNAME "xpilot-cpp-server"

extern object_t *Obj[];

extern int NumPlayers;
extern int NumOperators;
extern int NumPseudoPlayers;
extern int NumQueuedPlayers;
extern int ObjCount;
extern int NumAlliances;
extern int NumRobots;

extern char ShutdownReason[];

extern double timePerFrame;
extern const double timeStep;

/*
 * Prototypes for cell.c
 */
void Free_cells(void);
void Alloc_cells(void);
void Cell_init_object(object_t *obj);
void Cell_add_object(object_t *obj);
void Cell_remove_object(object_t *obj);
void Cell_get_objects(clpos_t pos, int r, int max, object_t ***list, int *count);

/*
 * Prototypes for collision.c
 */
void Check_collision(void);
int wormXY(int x, int y);
int IsOffensiveItem(enum Item i);
int IsDefensiveItem(enum Item i);
int CountOffensiveItems(player_t *pl);
int CountDefensiveItems(player_t *pl);

/*
 * Prototypes for id.c
 */
int peek_ID(void);
int request_ID(void);
void release_ID(int id);

/*
 * Prototypes for event.c
 */
int Handle_keyboard(player_t *pl);
void Pause_player(player_t *pl, bool on);
int Player_lock_closest(player_t *pl, bool next);
bool team_dead(int team);
void filter_mods(modifiers_t *mods);

/*
 * Prototypes for map.c
 */
int World_place_base(clpos_t pos, int dir, int team, int order);
int World_place_cannon(clpos_t pos, int dir, int team);
int World_place_check(clpos_t pos, int ind);
int World_place_fuel(clpos_t pos, int team);
int World_place_grav(clpos_t pos, double force, int type);
int World_place_target(clpos_t pos, int team);
int World_place_treasure(clpos_t pos, int team, bool empty, int ball_style);
int World_place_wormhole(clpos_t pos, wormtype_t type);
int World_place_item_concentrator(clpos_t pos);
int World_place_asteroid_concentrator(clpos_t pos);
int World_place_friction_area(clpos_t pos, double fric);

void World_free(void);
bool Grok_map(void);
void Find_base_direction(void);
void Compute_gravity(void);
double Wrap_findDir(double dx, double dy);
double Wrap_cfindDir(double dcx, double dcy);
double Wrap_length(double dx, double dy);
int Find_closest_team(clpos_t pos);

int Wildmap(
    int width,
    int height,
    char *name,
    char *author,
    char **data,
    int *width_ptr,
    int *height_ptr);

/*
 * Prototypes for cmdline.c
 */
void tuner_none(void);
void tuner_dummy(void);
void Timing_setup(void);
bool Init_options(void);
void Free_options(void);

/*
 * Prototypes for player.c
 */
void Thrust(player_t *pl);
void Recoil(object_t *ship, object_t *shot);
void Record_shove(player_t *pl, player_t *pusher, long time);
void Delta_mv(object_t *ship, object_t *obj);
void Delta_mv_elastic(object_t *obj1, object_t *obj2);
void Obj_repel(object_t *obj1, object_t *obj2, int repel_dist);
void Item_damage(player_t *pl, double prob);
void Tank_handle_detach(player_t *pl);
void Update_tanks(pl_fuel_t *);

void Add_fuel(pl_fuel_t *ft, double fuel);

static inline void Player_add_fuel(player_t *pl, double amount)
{
    Add_fuel(&(pl->fuel), amount);
}

void Place_item(int type, player_t *pl);
int Choose_random_item(void);
void Tractor_beam(player_t *pl);
void General_tractor_beam(int id, clpos_t pos, int items, player_t *victim, bool pressor);
void Place_mine(player_t *pl);
void Place_moving_mine(player_t *pl);
void Place_general_mine(int id, int team, long status, clpos_t pos,
                        vector_t vel, modifiers_t mods);
void Detonate_mines(player_t *pl);
char *Describe_shot(int type, long status, modifiers_t mods, int hit);
void Fire_ecm(player_t *pl);
void Fire_general_ecm(int id, int team, clpos_t pos);
void Move_ball(int ind);
void Fire_general_shot(int id, int team,
                       clpos_t pos, int type, int dir,
                       modifiers_t mods, int target_id);
void Fire_normal_shots(player_t *pl);
void Fire_main_shot(player_t *pl, int type, int dir);
void Fire_shot(player_t *pl, int type, int dir);
void Fire_left_shot(player_t *pl, int type, int dir, int gun);
void Fire_right_shot(player_t *pl, int type, int dir, int gun);
void Fire_left_rshot(player_t *pl, int type, int dir, int gun);
void Fire_right_rshot(player_t *pl, int type, int dir, int gun);
void Make_treasure_ball(int treasure);
int Punish_team(int ind, int t_destroyed, int t_target);
void Delete_shot(int ind);
void Fire_laser(player_t *pl);
void Fire_general_laser(int id, int team, clpos_t pos, int dir, modifiers_t mods);
void Do_deflector(player_t *pl);
void Do_transporter(player_t *pl);
void Do_general_transporter(int id, clpos_t pos, player_t *victim, int *item, double *amount);
void do_hyperjump(player_t *pl);
void do_lose_item(player_t *pl);
void Update_torpedo(torpobject_t *torp);
void Update_missile(missileobject_t *missile);
void Update_mine(mineobject_t *mine);
void Make_debris(
    clpos_t pos,
    vector_t vel,
    int id,
    int team,
    int type,
    double mass,
    long status,
    int color,
    int radius,
    int min_debris, int max_debris,
    int min_dir, int max_dir,
    double min_speed, double max_speed,
    int min_life, int max_life);
void Make_wreckage(
    clpos_t pos,
    vector_t vel,
    int id,
    int team,
    double min_mass, double max_mass,
    double total_mass,
    long status,
    int color,
    int max_wreckage,
    int min_dir, int max_dir,
    double min_speed, double max_speed,
    int min_life, int max_life);
void Make_item(clpos_t pos,
               vector_t vel,
               int item_type, int num_per_pack,
               long status);
void Explode_fighter(player_t *pl);
void Throw_items(player_t *pl);
void Detonate_items(player_t *pl);
void add_temp_wormholes(int xin, int yin, int xout, int yout);
void remove_temp_wormhole(int ind);

/*
 * Prototypes for cannon.c
 */
void Cannon_init(cannon_t *cannon);
void Cannon_add_item(cannon_t *cannon, int item, int amount);
void Cannon_throw_items(cannon_t *cannon);
void Cannon_check_defense(cannon_t *cannon);
void Cannon_check_fire(cannon_t *cannon);

/*
 * Prototypes for command.c
 */
void Handle_player_command(player_t *pl, char *cmd);
player_t *Get_player_by_name(const char *str,
                             int *errcode, const char **errorstr_p);
void Send_info_about_player(player_t *pl);
void Set_swapper_state(player_t *pl);

/*
 * Prototypes for race.c
 */
void Race_compute_game_status(void);
void Race_game_over(void);
void Player_reset_timing(player_t *pl);
void Player_pass_checkpoint(player_t *pl);
void PlayerCheckpointCollision(player_t *pl);

/*
 * Prototypes for rules.c
 */
void Tune_item_probs(void);
void Tune_item_packs(void);
void Set_initial_resources(void);
void Set_world_items(void);
void Set_world_rules(void);
void Set_world_asteroids(void);
void Set_misc_item_limits(void);
void Tune_asteroid_prob(void);

/*
 * Prototypes for server.c
 */
void End_game(void);
int Pick_team(int pick_for_type);
void Server_info(char *str, unsigned max_size);
void Log_game(const char *heading);
void Game_Over(void);
void Server_shutdown(const char *user_name, int delay, const char *reason);
void Server_log_admin_message(player_t *pl, const char *str);
int plock_server(bool on);
void Main_loop(void);

/*
 * Prototypes for contact.c
 */
void Contact_cleanup(void);
bool Contact_init(void);
void Contact(int fd, void *arg);
void Queue_loop(void);
int Queue_advance_player(char *name, char *msg);
int Queue_show_list(char *qmsg, size_t size);
void Set_deny_hosts(void);

/*
 * Prototypes for metaserver.c
 */
void Meta_send(char *mesg, int len);
int Meta_from(char *addr, int port);
void Meta_gone(void);
void Meta_init(void);
void Meta_update(int change);

/*
 * Prototypes for frame.c
 */
void Frame_update(void);
void Set_message(const char *message);
void Set_player_message(player_t *pl, const char *message);
void Set_message_f(const char *format, ...);
void Set_player_message_f(player_t *pl, const char *format, ...);

/*
 * Prototypes for update.c
 */
void Update_objects(void);
void Autopilot(player_t *pl, bool on);
void Cloak(player_t *pl, bool on);
void Deflector(player_t *pl, bool on);
void Emergency_thrust(player_t *pl, bool on);
void Emergency_shield(player_t *pl, bool on);
void Phasing(player_t *pl, bool on);
void Thrust(player_t *pl, bool on);

/*
 * Prototypes for option.c
 */
void Options_parse(void);
void Options_free(void);
bool Convert_string_to_int(const char *value_str, int *int_ptr);
bool Convert_string_to_float(const char *value_str, double *float_ptr);
bool Convert_string_to_bool(const char *value_str, bool *bool_ptr);
void Convert_list_to_string(list_t list, char **string);
void Convert_string_to_list(const char *value, list_t *list_ptr);

/*
 * Prototypes for parser.c
 */
int Parser_list_option(int *index, char *buf);
bool Parser(int argc, char **argv);
int Tune_option(char *name, char *val);
int Get_option_value(const char *name, char *value, unsigned size);

/*
 * Prototypes for fileparser.c
 */
bool parseDefaultsFile(const char *filename);
bool parsePasswordFile(const char *filename);
bool parseMapFile(const char *filename);
void expandKeyword(const char *keyword);

/*
 * Prototypes for laser.c
 */
void Laser_pulse_collision(void);

/*
 * Prototypes for alliance.c
 */
int Invite_player(player_t *pl, player_t *ally);
int Cancel_invitation(player_t *pl);
int Refuse_alliance(player_t *pl, player_t *ally);
int Refuse_all_alliances(player_t *pl);
int Accept_alliance(player_t *pl, player_t *ally);
int Accept_all_alliances(player_t *pl);
int Get_alliance_member_count(int id);
void Player_join_alliance(player_t *pl, player_t *ally);
void Dissolve_all_alliances(void);
int Leave_alliance(player_t *pl);
void Alliance_player_list(player_t *pl);

/*
 * Prototypes for object.c
 */
object_t *Object_allocate(void);
void Object_free_ind(int ind);
void Object_free_ptr(object_t *obj);
void Alloc_shots(int number);
void Free_shots(void);

/*
 * Prototypes for polygon.c
 */
void P_edgestyle(const char *id, int width, int color, int style);
void P_polystyle(const char *id, int color, int texture_id, int defedge_id,
                 int flags);
void P_bmpstyle(const char *id, const char *filename, int flags);
void P_start_polygon(clpos_t pos, int style);
void P_offset(clpos_t offset, int edgestyle);
void P_vertex(clpos_t pos, int edgestyle);
void P_style(const char *state, int style);
void P_end_polygon(void);
int P_start_ballarea(void);
void P_end_ballarea(void);
int P_start_balltarget(int team, int treasure_ind);
void P_end_balltarget(void);
int P_start_target(int target_ind);
void P_end_target(void);
int P_start_cannon(int cannon_ind);
void P_end_cannon(void);
int P_start_wormhole(int wormhole_ind);
void P_end_wormhole(void);
void P_start_decor(void);
void P_end_decor(void);
int P_start_friction_area(int fa_ind);
void P_end_friction_area(void);
int P_get_bmp_id(const char *s);
int P_get_edge_id(const char *s);
int P_get_poly_id(const char *s);
/*void P_grouphack(int type, void (*f)(int group, void *mapobj));*/
void P_set_hitmask(int group, hitmask_t hitmask);

/*
 * Prototypes for showtime.c
 */
char *showtime(void);

#endif
