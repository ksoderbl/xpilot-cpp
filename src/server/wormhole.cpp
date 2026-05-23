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

#include "server.h"

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

void Object_hits_wormhole(object_t *obj, int ind)
{
    SET_BIT(obj->obj_status, WARPING);
    // obj->wormHoleHit = ind;//TODO
}
