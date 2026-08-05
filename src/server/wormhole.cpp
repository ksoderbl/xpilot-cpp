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

#include "cell.h"
#include "server.h"
#include "ship.h"

#include "saudio.h"

#include "walls.h"

shape_t wormhole_wire;

/*
 * Initialization functions.
 */

void Wormhole_line_init(void)
{
    int i;
    static clpos_t coords[MAX_SHIP_PTS];

    wormhole_wire.num_points = MAX_SHIP_PTS;
    for (i = 0; i < MAX_SHIP_PTS; i++)
    {
        wormhole_wire.pts[i] = coords + i;
        coords[i].cx = (int)(cos(i * 2 * PI / MAX_SHIP_PTS) * WORMHOLE_RADIUS);
        coords[i].cy = (int)(sin(i * 2 * PI / MAX_SHIP_PTS) * WORMHOLE_RADIUS);
    }

    return;
}

bool Verify_wormhole_consistency(void)
{
    int i, worm_in = 0, worm_out = 0, worm_norm = 0;

    /* count wormhole types */
    for (i = 0; i < Num_wormholes(); i++)
    {
        int type = Wormhole_by_index(i)->type;

        if (type == WORM_NORMAL)
            worm_norm++;
        else if (type == WORM_IN)
            worm_in++;
        else if (type == WORM_OUT)
            worm_out++;
    }

    /*
     * Verify that the wormholes are consistent, i.e. that if
     * we have no 'out' wormholes, make sure that we don't have
     * any 'in' wormholes, and (less critical) if we have no 'in'
     * wormholes, make sure that we don't have any 'out' wormholes.
     */
    if ((worm_norm) ? (worm_norm + worm_out < 2)
        : (worm_in) ? (worm_out < 1)
                    : (worm_out > 0))
    {

        int i;

        printf("Inconsistent use of wormholes, removing them.\n");
        for (i = 0; i < World.NumWormholes; i++)
        {
            wormhole_t *wormhole = Wormhole_by_index(i);
            blkpos_t blkpos = Clpos_to_blkpos(wormhole->pos);

            World.block[blkpos.bx][blkpos.by] = SPACE;
            World.itemID[blkpos.bx][blkpos.by] = (uint16_t)-1;
        }
        World.NumWormholes = 0;
    }

    return true;
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

    if (obj == nullptr)
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
    world_t *world = &World;

    /*
     * Don't connect to balls while warping.
     */
    if (Player_uses_connector(pl))
        pl->ball = nullptr;

    if (BIT(pl->have, HAS_BALL))
    {
        /*
         * Warp every ball associated with player.
         * NB. the connector can cross a wall boundary this is
         * allowed, so long as the ball itself doesn't collide.
         */
        int k;

        for (k = 0; k < NumObjs; k++)
        {
            object_t *b = Obj[k];

            if (b->type == OBJ_BALL && b->id == pl->id)
            {
                clpos_t ballpos;
                ballpos.cx = b->pos.cx + dest.cx - pl->pos.cx;
                ballpos.cy = b->pos.cy + dest.cy - pl->pos.cy;
                ballpos = World_wrap_clpos(world, ballpos);
                if (!World_contains_clpos(world, ballpos))
                {
                    b->obj_life = 0.0;
                    continue;
                }

                Object_position_set_clpos(b, ballpos);
                Object_position_remember(b);
                b->vel.x *= WORM_BRAKE_FACTOR;
                b->vel.y *= WORM_BRAKE_FACTOR;
                Cell_add_object(b);
            }
        }
    }
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
    if (pl->item[ITEM_HYPERJUMP] <= 0)
        return false;

    if (pl->fuel.sum < -ED_HYPERJUMP)
        return false;

    pl->item[ITEM_HYPERJUMP]--;
    Player_add_fuel(pl, ED_HYPERJUMP);
    SET_BIT(pl->obj_status, WARPING);
    pl->wormHoleHit = -1;

    return true;
}

/*
 * Player has used hyperjump item.
 */
static void Hyperjump(player_t *pl)
{
}

void Do_warp(player_t *pl)
{
    world_t *world = &World;
    position_t w;
    int wx, wy, proximity,
        nearestFront, nearestRear,
        proxFront, proxRear, j;

    if (pl->wormHoleHit >= Num_wormholes())
    {
        /* could happen if the player hit a temporary wormhole
           that was removed while the player was warping */
        CLR_BIT(pl->obj_status, WARPING);
        return;
    }

    if (pl->wormHoleHit != -1)
    {
        wormhole_t *wormhole = nullptr;

        if (World.wormholes[pl->wormHoleHit].countdown > 0)
        {
            j = World.wormholes[pl->wormHoleHit].lastdest;
        }
        else if (rfrac() < 0.10)
        {
            do
                j = (int)(rfrac() * Num_wormholes());
            while (World.wormholes[j].type == WORM_IN || pl->wormHoleHit == j || World.wormholes[j].temporary);
        }
        else
        {
            nearestFront = nearestRear = -1;
            proxFront = proxRear = 10000000;

            for (j = 0; j < Num_wormholes(); j++)
            {
                wormhole = Wormhole_by_index(j);

                if (j == pl->wormHoleHit || wormhole->type == WORM_IN || wormhole->temporary)
                    continue;

                // wx = (wormhole->blk_pos.bx - World.wormholes[pl->wormHoleHit].blk_pos.bx) * BLOCK_SZ;
                // wy = (wormhole->blk_pos.by - World.wormholes[pl->wormHoleHit].blk_pos.by) * BLOCK_SZ;
                wx = CLICK_TO_PIXEL(wormhole->pos.cx - World.wormholes[pl->wormHoleHit].pos.cx);
                wy = CLICK_TO_PIXEL(wormhole->pos.cy - World.wormholes[pl->wormHoleHit].pos.cy);
                wx = WORLD_WRAP_DX(world, wx);
                wy = WORLD_WRAP_DY(world, wy);

                proximity = (int)(pl->vel.y * wx + pl->vel.x * wy);
                proximity = ABS(proximity);

                if (pl->vel.x * wx + pl->vel.y * wy < 0)
                {
                    if (proximity < proxRear)
                    {
                        nearestRear = j;
                        proxRear = proximity;
                    }
                }
                else if (proximity < proxFront)
                {
                    nearestFront = j;
                    proxFront = proximity;
                }
            }

#define RANDOM_REAR_WORM
#ifndef RANDOM_REAR_WORM
            j = nearestFront < 0 ? nearestRear : nearestFront;
#else  /* RANDOM_REAR_WORM */
            if (nearestFront >= 0)
            {
                j = nearestFront;
            }
            else
            {
                do
                {
                    j = (int)(rfrac() * Num_wormholes());
                    wormhole = Wormhole_by_index(j);
                } while (wormhole->type == WORM_IN || j == pl->wormHoleHit);
            }
#endif /* RANDOM_REAR_WORM */
        }

        sound_play_sensors(pl->pos, WORM_HOLE_SOUND);

        wormhole = Wormhole_by_index(j);
        warn("Wormhole index is %d", j);

        // w.x = (World.wormholes[j].blk_pos.bx + 0.5) * BLOCK_SZ;
        // w.y = (World.wormholes[j].blk_pos.by + 0.5) * BLOCK_SZ;
        w.x = CLICK_TO_PIXEL(wormhole->pos.cx);
        w.y = CLICK_TO_PIXEL(wormhole->pos.cy);
    }
    else
    {
        /* wormHoleHit == -1 */
        int counter;
        for (counter = 20; counter > 0; counter--)
        {
            w.x = (int)(rfrac() * World.width);
            w.y = (int)(rfrac() * World.height);
            if (BIT(1U << World.block[(int)(w.x / BLOCK_SZ)]
                                     [(int)(w.y / BLOCK_SZ)],
                    SPACE_BLOCKS))
            {
                break;
            }
        }
        if (!counter)
        {
            w.x = CLICK_TO_PIXEL(pl->pos.cx);
            w.y = CLICK_TO_PIXEL(pl->pos.cy);
        }
        if (counter &&
            options.wormTime &&
            BIT(1U << World.block[OBJ_X_IN_BLOCKS(pl)][OBJ_Y_IN_BLOCKS(pl)], SPACE_BIT) &&
            BIT(1U << World.block[(int)(w.x / BLOCK_SZ)][(int)(w.y / BLOCK_SZ)], SPACE_BIT))
        {
            add_temp_wormholes(OBJ_X_IN_BLOCKS(pl),
                               OBJ_Y_IN_BLOCKS(pl),
                               (int)(w.x / BLOCK_SZ),
                               (int)(w.y / BLOCK_SZ));
        }
        j = -2;
        sound_play_sensors(pl->pos, HYPERJUMP_SOUND);
    }

    // /*
    //  * Don't connect to balls while warping.
    //  */
    // if (BIT(pl->used, USES_CONNECTOR))
    //     pl->ball = nullptr;

    // if (BIT(pl->have, HAS_BALL))
    // {
    // /*
    //  * Take every ball associated with player through worm hole.
    //  * NB. the connector can cross a wall boundary this is
    //  * allowed, so long as the ball itself doesn't collide.
    //  */
    // int k;
    // for (k = 0; k < NumObjs; k++)
    // {
    //     object_t *b = Obj[k];
    //     if (BIT(b->type, OBJ_BALL_BIT) && b->id == pl->id)
    //     {
    //         position_t ballpos;
    //         ballpos.x = b->pix_pos.x + (w.x - pl->pix_pos.x);
    //         ballpos.y = b->pix_pos.y + (w.y - pl->pix_pos.y);
    //         ballpos.x = WRAP_XPIXEL(ballpos.x);
    //         ballpos.y = WRAP_YPIXEL(ballpos.y);
    //         if (ballpos.x < 0 || ballpos.x >= World.width || ballpos.y < 0 || ballpos.y >= World.height)
    //         {
    //             b->obj_life = 0.0;
    //         }
    //         else
    //         {
    //             clpos_t ball_clpos;
    //             ball_clpos.cx = FLOAT_TO_CLICK(ballpos.x);
    //             ball_clpos.cy = FLOAT_TO_CLICK(ballpos.y);
    //             Object_position_set_clpos(b, ball_clpos);
    //             Object_position_remember(b);
    //             b->vel.x *= WORM_BRAKE_FACTOR;
    //             b->vel.y *= WORM_BRAKE_FACTOR;
    //             Cell_add_object(b);
    //         }
    //     }
    // }
    clpos_t dest;
    dest.cx = PIXEL_TO_CLICK(wx);
    dest.cy = PIXEL_TO_CLICK(wy);
    Warp_balls(pl, dest);
    // }

    pl->wormHoleDest = j;
    clpos_t pos;
    pos.cx = FLOAT_TO_CLICK(w.x);
    pos.cy = FLOAT_TO_CLICK(w.y);
    Player_position_init_clpos(pl, pos);
    pl->vel.x *= WORM_BRAKE_FACTOR;
    pl->vel.y *= WORM_BRAKE_FACTOR;
    pl->forceVisible += 15;

    if ((j != pl->wormHoleHit) && (pl->wormHoleHit != -1))
    {
        World.wormholes[pl->wormHoleHit].lastdest = j;
        if (!World.wormholes[j].temporary)
        {
            World.wormholes[pl->wormHoleHit].countdown = (options.wormTime ? options.wormTime : WORMCOUNT);
        }
    }

    CLR_BIT(pl->obj_status, WARPING);
    SET_BIT(pl->obj_status, WARPED);

    sound_play_sensors(pl->pos, WORM_HOLE_SOUND);
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

    if ((wwhtemp = (wormhole_t *)realloc(World.wormholes,
                                         (World.NumWormholes + 2) * sizeof(wormhole_t))) == nullptr)
    {
        error("No memory for temporary wormholes.");
        return;
    }
    World.wormholes = wwhtemp;

    blkpos_t inblk, outblk;

    inblk.bx = xin;
    inblk.by = yin;
    outblk.bx = xout;
    outblk.by = yout;

    inhole.pos = Block_get_center_clpos(inblk);
    outhole.pos = Block_get_center_clpos(outblk);

    inhole.countdown = outhole.countdown = options.wormTime;
    inhole.lastdest = World.NumWormholes + 1;
    inhole.temporary = outhole.temporary = 1;
    inhole.type = WORM_IN;
    outhole.type = WORM_OUT;
    inhole.lastblock = World.block[xin][yin];
    outhole.lastblock = World.block[xout][yout];
    inhole.lastID = World.itemID[xin][yin];
    outhole.lastID = World.itemID[xout][yout];
    World.wormholes[World.NumWormholes] = inhole;
    World.wormholes[World.NumWormholes + 1] = outhole;
    World.block[xin][yin] = World.block[xout][yout] = WORMHOLE;
    World.itemID[xin][yin] = World.NumWormholes;
    World.itemID[xout][yout] = World.NumWormholes + 1;
    World.NumWormholes += 2;
}

void remove_temp_wormhole(int ind)
{
    wormhole_t hole;

    hole = World.wormholes[ind];
    blkpos_t blkpos = Clpos_to_blkpos(hole.pos);
    World.block[blkpos.bx][blkpos.by] = hole.lastblock;
    World.itemID[blkpos.bx][blkpos.by] = hole.lastID;
    World.NumWormholes--;
    if (ind != World.NumWormholes)
    {
        World.wormholes[ind] = World.wormholes[World.NumWormholes];
    }
    World.wormholes = (wormhole_t *)realloc(World.wormholes,
                                            World.NumWormholes * sizeof(wormhole_t));
}

// Returns pointer to wormhole at block with coordinates (x, y).
// This assumes the map is block based.
wormhole_t *wormholeXY(int x, int y)
{
    for (int i = 0; i < Num_wormholes(); i++)
    {
        wormhole_t *wormhole = Wormhole_by_index(i);
        blkpos_t blk = Clpos_to_blkpos(wormhole->pos);
        if (blk.bx == x && blk.by == y)
            return wormhole;
    }
    return nullptr;
}

// Return index of wormhole in world's wormhole data structure.
// This function should be eventually removed.
int Index_by_wormhole(world_t *world, wormhole_t *wormhole)
{
    for (int i = 0; i < Num_wormholes(); i++)
    {
        if (Wormhole_by_index(i) == wormhole)
            return i;
    }
    return NO_IND;
}
