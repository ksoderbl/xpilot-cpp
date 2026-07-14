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

#include "xperror.h"

#include "walls.h"
#include "walls1.h"
#include "walls2.h"

void Walls_init(void)
{
    warn("Walls_init: is_polygon_map: %s", is_polygon_map ? "true" : "false");
    if (!is_polygon_map)
        Walls_init1();
    else
        Walls_init2();
}

void Move_object(object_t *obj)
{
    warn("Move_object: is_polygon_map: %s", is_polygon_map ? "true" : "false");
    if (!is_polygon_map)
        Move_object1(obj);
    else
        Move_object2(obj);
}

void Move_player(player_t *pl)
{
    warn("Move_player: is_polygon_map: %s", is_polygon_map ? "true" : "false");
    if (!is_polygon_map)
        Move_player1(pl);
    else
        Move_player2(pl);
}

void Ball_line_init(void)
{
    warn("Ball_line_init: is_polygon_map: %s", is_polygon_map ? "true" : "false");
    if (!is_polygon_map)
        warn("NOP");
    else
        Ball_line_init2();
}

void Turn_player(player_t *pl, bool push)
{
    warn("Turn_player: is_polygon_map: %s", is_polygon_map ? "true" : "false");
    if (!is_polygon_map)
        Turn_player1(pl);
    else
        Turn_player2(pl, push);
}