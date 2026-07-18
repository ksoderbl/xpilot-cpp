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

#include "wormhole.h"

#include "xperror.h"

#include "server.h"

/*
 * Initialization functions.
 */

void Wormhole_line_init(void)
{
}

bool Verify_wormhole_consistency(void)
{
    return false;
}

/*
 * Functions used in game.
 */

hitmask_t Wormhole_hitmask(wormhole_t *wormhole)
{
    if (wormhole->type == WORM_OUT)
        return ALL_BITS;
    return 0;
}

bool Wormhole_hitfunc(group_t *gp, const move_t *move)
{
    const object_t *obj = move->obj;
    wormhole_t *wormhole = Wormhole_by_index(gp->mapobj_ind);

    if (wormhole->type == WORM_OUT)
        return false;

    if (obj == NULL)
        return true;

    if (BIT(obj->obj_status, WARPED | WARPING))
        return false;

    return true;
}

void Object_hits_wormhole1(object_t *obj, int ind)
{
    SET_BIT(obj->obj_status, WARPING);
    // obj->wormHoleHit = ind;//TODO
}

void Object_hits_wormhole2(object_t *obj, int ind)
{
    SET_BIT(obj->obj_status, WARPING);
    // obj->wormHoleHit = ind;//TODO
}

/*
 * Warp balls connected to warped player.
 */
static void Warp_balls(player_t *pl, clpos_t dest)
{
}

static int Find_wormhole_dest(int wh_hit_ind)
{
    int wh_ind = 0;

    return wh_ind;
}

/*
 * Move player trough wormhole.
 */
static void Traverse_wormhole(player_t *pl)
{
}

/*
 * Returns true if warp status was achieved.
 */
bool Initiate_hyperjump(player_t *pl)
{
    return false;
}

/*
 * Player has used hyperjump item.
 */
static void Hyperjump(player_t *pl)
{
}

void Player_warp(player_t *pl)
{
}

void Player_finish_warp(player_t *pl)
{
}

void Object_warp(object_t *obj)
{
}

void Object_finish_warp(object_t *obj)
{
}

void add_temp_wormholes(int xin, int yin, int xout, int yout)
{
    wormhole_t inhole, outhole, *wwhtemp;

    if ((wwhtemp = (wormhole_t *)realloc(world->wormholes,
                                         (world->NumWormholes + 2) * sizeof(wormhole_t))) == NULL)
    {
        error("No memory for temporary wormholes.");
        return;
    }
    world->wormholes = wwhtemp;

    inhole.blk_pos.bx = xin;
    inhole.blk_pos.by = yin;
    outhole.blk_pos.bx = xout;
    outhole.blk_pos.by = yout;
    inhole.countdown = outhole.countdown = options.wormTime;
    inhole.lastdest = world->NumWormholes + 1;
    inhole.temporary = outhole.temporary = 1;
    inhole.type = WORM_IN;
    outhole.type = WORM_OUT;
    inhole.lastblock = world->block[xin][yin];
    outhole.lastblock = world->block[xout][yout];
    inhole.lastID = world->itemID[xin][yin];
    outhole.lastID = world->itemID[xout][yout];
    world->wormholes[world->NumWormholes] = inhole;
    world->wormholes[world->NumWormholes + 1] = outhole;
    world->block[xin][yin] = world->block[xout][yout] = WORMHOLE;
    world->itemID[xin][yin] = world->NumWormholes;
    world->itemID[xout][yout] = world->NumWormholes + 1;
    world->NumWormholes += 2;
}

void remove_temp_wormhole(int ind)
{
    wormhole_t hole;

    hole = world->wormholes[ind];
    world->block[hole.blk_pos.bx][hole.blk_pos.by] = hole.lastblock;
    world->itemID[hole.blk_pos.bx][hole.blk_pos.by] = hole.lastID;
    world->NumWormholes--;
    if (ind != world->NumWormholes)
    {
        world->wormholes[ind] = world->wormholes[world->NumWormholes];
    }
    world->wormholes = (wormhole_t *)realloc(world->wormholes,
                                             world->NumWormholes * sizeof(wormhole_t));
}
