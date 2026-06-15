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

#include "clientmap.h"

double teamscores[MAX_TEAMS];
cannontime_t *cannons = NULL;
int num_cannons = 0;
target_t *targets = NULL;
int num_targets = 0;

fuelstation_t *fuels = NULL;
int num_fuels = 0;
homebase_t *bases = NULL;
int num_bases = 0;
checkpoint_t *checks = NULL;
int num_checks = 0;
xp_polygon_t *polygons = NULL;
int num_polygons = 0;
edge_style_t *edge_styles = NULL;
int num_edge_styles = 0;
polygon_style_t *polygon_styles = NULL;
int num_polygon_styles = 0;

score_object_t score_objects[MAX_SCORE_OBJECTS];
int score_object = 0;

other_t *Others = NULL;
int num_others = 0, max_others = 0;
refuel_t *refuel_ptr;
int num_refuel, max_refuel;
connector_t *connector_ptr;
int num_connector, max_connector;
laser_t *laser_ptr;
int num_laser, max_laser;
missile_t *missile_ptr;
int num_missile, max_missile;
ball_t *ball_ptr;
int num_ball, max_ball;
ship_t *ship_ptr;
int num_ship, max_ship;
mine_t *mine_ptr;
int num_mine, max_mine;
itemtype_t *itemtype_ptr;
int num_itemtype, max_itemtype;
ecm_t *ecm_ptr;
int num_ecm, max_ecm;
trans_t *trans_ptr;
int num_trans, max_trans;
paused_t *paused_ptr;
int num_paused, max_paused;
appearing_t *appearing_ptr;
int num_appearing, max_appearing;
radar_t *radar_ptr;
int num_radar, max_radar;
vcannon_t *vcannon_ptr;
int num_vcannon, max_vcannon;
vfuel_t *vfuel_ptr;
int num_vfuel, max_vfuel;
vbase_t *vbase_ptr;
int num_vbase, max_vbase;
debris_t *debris_ptr[DEBRIS_TYPES];
int num_debris[DEBRIS_TYPES],
    max_debris[DEBRIS_TYPES];
debris_t *fastshot_ptr[DEBRIS_TYPES * 2];
int num_fastshot[DEBRIS_TYPES * 2],
    max_fastshot[DEBRIS_TYPES * 2];
vdecor_t *vdecor_ptr;
int num_vdecor, max_vdecor;
wreckage_t *wreckage_ptr;
int num_wreckage, max_wreckage;
asteroid_t *asteroid_ptr;
int num_asteroids, max_asteroids;
wormhole_t *wormhole_ptr;
int num_wormholes, max_wormholes;
