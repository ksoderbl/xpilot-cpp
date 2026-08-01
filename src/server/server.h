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

#include <cstdio>
#include <cstdint>

#include "click.h"
#include "setup.h"
#include "shipshape.h"

#include "defaults.h"
#include "map.h"
#include "object.h"
#include "player.h"
#include "polygon.h"

#include "object.h"
#include "map.h"

#include "serveroption.h"

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

typedef struct
{
    char owner[80];
    char host[80];
} server_t;

extern shape_t ball_wire, wormhole_wire, filled_wire;

static inline vector_t World_gravity(world_t *world, clpos_t pos)
{
    vector_t vec = world->gravity[CLICK_TO_BLOCK(pos.cx)][CLICK_TO_BLOCK(pos.cy)];
    return vec;
}

/*
 * Global data.
 */
#define FPS options.framesPerSecond
// #define NumObjs (ObjCount + 0)

extern long frame_loops;
extern long frame_loops_slow;
extern double frame_time;
extern int NumPlayers;
extern int NumSpectators;
extern int NumOperators;
extern int NumPseudoPlayers;
extern int NumQueuedPlayers;
extern int ObjCount;
extern int NumAlliances;
extern int NumRobots;
extern int login_in_progress;
extern char ShutdownReason[];
extern sock_t contactSocket;
extern time_t serverStartTime;
extern server_t Server;

extern uint32_t LEGACY_KILL_BITS;
extern uint32_t DEF_HAVE;
extern uint32_t DEF_USED;
extern uint32_t USED_KILL;
extern uint32_t KILLING_SHOTS;

// extern int GetInd[];
extern int ShutdownServer, ShutdownDelay;
extern long main_loops;

extern int tagItPlayerId;
extern int mainLoopTime;
extern char *serverAddr;
extern bool updateScores;
extern int game_lock;
extern double friction;
extern int roundtime;
extern int roundsPlayed;

// extern unsigned SPACE_BLOCKS;
extern double coriolisCosine, coriolisSine;

extern char ShutdownReason[];

extern double timePerFrame;

extern double timeStep;

#define APPNAME "xpilot-cpp-server"

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

/*
 * Prototypes for map.c
 */
int World_init(void);
void World_free(void);
bool Grok_map(void);
bool Grok_map_options(void);

int World_place_base(world_t *world, clpos_t pos, int dir, int team, int order);
int World_place_cannon(world_t *world, clpos_t pos, int dir, int team);
int World_place_check(world_t *world, clpos_t pos, int ind);
int World_place_fuel(world_t *world, clpos_t pos, int team);
int World_place_grav(world_t *world, clpos_t pos, double force, int type);
int World_place_target(world_t *world, clpos_t pos, int team);
int World_place_treasure(world_t *world, clpos_t pos, int team, bool empty, int ball_style);
int World_place_wormhole(world_t *world, clpos_t pos, wormtype_t type);
int World_place_item_concentrator(world_t *world, clpos_t pos);
int World_place_asteroid_concentrator(world_t *world, clpos_t pos);
int World_place_friction_area(world_t *world, clpos_t pos, double fric);

void Find_base_direction(void);
void Compute_gravity(void);
double World_wrap_findDir(world_t *world, double dx, double dy);
double World_wrap_cfindDir(world_t *world, int dx, int dy);
double World_wrap_length(world_t *world, int dx, int dy);
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
 * Prototypes for xpmap.c
 */
void Create_blockmap_from_polygons(void);
setup_t *Xpmap_init_setup(void);
void Xpmap_print(void);
void Xpmap_grok_map_data(void);
void Xpmap_allocate_checks(void);
void Xpmap_tags_to_internal_data(void);
void Xpmap_find_map_object_teams(void);
void Xpmap_find_base_direction(void);
void Xpmap_blocks_to_polygons(void);

void Xpmap_grok_map_data2(void);
void Xpmap_tags_to_internal_data2(void);
void Xpmap_find_map_object_teams2(void);

/*
 * Prototypes for xp2map.c
 */
bool isXp2MapFile(FILE *ifile);
bool parseXp2MapFile(char *fname, optOrigin opt_origin);

/*
 * Prototypes for cmdline.c
 */
void tuner_none(void);
void tuner_dummy(void);
void Check_playerlimit(void);
void Timing_setup(void);
bool Init_options(void);
void Free_options(void);

/*
 * Prototypes for server.cpp
 */
void End_game(void);
int Pick_team(int pick_for_type);
void Server_info(char *str, size_t max_size);
void Log_game(const char *heading);
const char *Describe_game_status(void);
void Game_Over(void);
void Server_shutdown(const char *user_name, int delay, const char *reason);
void Server_log_admin_message(player_t *pl, const char *str);
int plock_server(bool on);
void Main_loop(void);

bool Friction_area_hitfunc(group_t *groupptr, const move_t *move);
