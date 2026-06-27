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

ClientMap clMap;

checkpoint_t *checks = NULL;
int num_checks = 0;
// xp_polygon_t *polygons = NULL;
// int num_polygons = 0;
edge_style_t *edge_styles = NULL;
int num_edge_styles = 0;
polygon_style_t *polygon_styles = NULL;
int num_polygon_styles = 0;

score_object_t score_objects[MAX_SCORE_OBJECTS];
int score_object = 0;

other_t *Others = NULL;
int num_others = 0, max_others = 0;
