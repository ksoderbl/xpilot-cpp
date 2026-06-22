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

#include "pack.h"

extern bool is_polygon_map;

#define MAGIC (is_polygon_map                       \
                   ? VERSION2MAGIC(POLYGON_VERSION) \
                   : VERSION2MAGIC(OLD_VERSION))

#define MY_VERSION MAGIC2VERSION(MAGIC)

/*
 * Which client versions can join this server.
 */
#define MIN_CLIENT_VERSION 0x4203
#define MAX_CLIENT_VERSION MY_VERSION
