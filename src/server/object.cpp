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

/*
 * This file deals with low-level object structure manipulations.
 */

#include <cstdio>
#include <cstdlib>

#include "commonmacros.h"

#define SERVER
#include "xpconfig.h"
#include "types.h"
#include "serverconst.h"
#include "global.h"
#include "proto.h"
#include "xperror.h"
#include "portability.h"

/*
 * Global variables
 */
int ObjCount = 0;
int NumPulses = 0;
int NumEcms = 0;
int NumTransporters = 0;
object_t *Obj[MAX_TOTAL_SHOTS];
pulse_t *Pulses[MAX_TOTAL_PULSES];
ecm_t *Ecms[MAX_TOTAL_ECMS];
trans_t *Transporters[MAX_TOTAL_TRANSPORTERS];

static void Object_incr_count(void)
{
    ObjCount++;
}

static void Object_decr_count(void)
{
    ObjCount--;
}

object_t *Object_allocate(void)
{
    object_t *obj = OBJ_PTR(NULL);

    if (ObjCount < MAX_TOTAL_SHOTS)
    {
        obj = Obj[ObjCount];
        Object_incr_count();

        obj->type = OBJ_DEBRIS;
        obj->life = 0;
    }
    else
    {
        xpwarn("Object_allocate: MAX_TOTAL_SHOTS (ObjCount = %d) reached", ObjCount);
    }

    return obj;
}

void Object_free_ind(int ind)
{
    if ((0 <= ind) && (ind < ObjCount) && (ObjCount <= MAX_TOTAL_SHOTS))
    {
        object_t *obj = Obj[ind];
        Object_decr_count();
        Obj[ind] = Obj[ObjCount];
        Obj[ObjCount] = obj;
    }
    else
    {
        xpwarn("Cannot free object %d, when count = %d, and total = %d !",
               ind, ObjCount, MAX_TOTAL_SHOTS);
    }
}

void Object_free_ptr(object_t *obj)
{
    int i;

    for (i = ObjCount - 1; i >= 0; i--)
    {
        if (Obj[i] == obj)
        {
            Object_free_ind(i);
            break;
        }
    }
    if (i < 0)
    {
        xpwarn("Could NOT free object!");
    }
}

static anyobject_t *objArray;

#define SHOWTYPESIZE(T) warn("sizeof(" #T ") = %d", sizeof(T))

void Alloc_shots(int number)
{
    xpinfo("Alloc_shots: number = %d", number);

    anyobject_t *x;
    int i;

#if 1
    SHOWTYPESIZE(object_t);
    SHOWTYPESIZE(ballobject_t);
    SHOWTYPESIZE(mineobject_t);
    SHOWTYPESIZE(missileobject_t);
    SHOWTYPESIZE(smartobject_t);
    SHOWTYPESIZE(torpobject_t);
    // SHOWTYPESIZE(heatobject_t);
    SHOWTYPESIZE(wireobject_t);
    // SHOWTYPESIZE(pulseobject_t);
    // SHOWTYPESIZE(itemobject_t);
    SHOWTYPESIZE(anyobject_t);
    SHOWTYPESIZE(player_t);
#endif

    xpinfo("Alloc_shots: sizeof(anyobject_t) = %d", sizeof(anyobject_t));
    xpinfo("Alloc_shots: total allocation = %d bytes", number * sizeof(anyobject_t));

    // x = (anyobject_t *)calloc(number, sizeof(anyobject_t));
    x = XCALLOC(anyobject_t, number);
    if (!x)
    {
        xperror("Not enough memory for shots.");
        exit(1);
    }

    objArray = x;
    for (i = 0; i < number; i++)
    {
        Obj[i] = &(x->obj);
        MINE_PTR(Obj[i])->owner = NO_ID;
        Cell_init_object(Obj[i]);
        x++;
    }
}

void Free_shots(void)
{
    XFREE(objArray);
}

// TODO: Remove pixel positions, store only subpixel position (i.e. clicks)
void Object_position_set_clicks(object_t *obj, int cx, int cy)
{
    struct _objposition *pos = (struct _objposition *)&obj->pos;

#if 1
    if (cx < 0)
    {
        printf("BUG!  Illegal object position (cx < 0): (cx = %d, cy = %d)\n", cx, cy);
        // *(double *)(-1) = 4321.0;
        // abort();
    }
    if (cx >= World.click_width)
    {
        printf("BUG!  Illegal object position (cx > world width): (cx = %d, cy = %d)\n", cx, cy);
        // *(double *)(-1) = 4321.0;
        // abort();
    }
    if (cy < 0)
    {
        printf("BUG!  Illegal object position (cy < 0): (cx = %d, cy = %d)\n", cx, cy);
        // *(double *)(-1) = 4321.0;
        // abort();
    }
    if (cy >= World.click_height)
    {
        printf("BUG!  Illegal object position (cy > world height): (cx = %d, cy = %d)\n", cx, cy);
        // *(double *)(-1) = 4321.0;
        // abort();
    }
#endif
    pos->cx = cx;
    pos->x = CLICK_TO_PIXEL(cx);
    pos->cy = cy;
    pos->y = CLICK_TO_PIXEL(cy);
}

void Object_position_init_clicks(object_t *obj, int cx, int cy)
{
    Object_position_set_clicks(obj, cx, cy);
    Object_position_remember(obj);
}

void Player_position_restore(player_t *pl)
{
    Player_position_set_clicks(pl, pl->prevpos.cx, pl->prevpos.cy);
}

void Player_position_set_clicks(player_t *pl, int cx, int cy)
{
    struct _objposition *pos = (struct _objposition *)&pl->pos;

#if 1
    if (cx < 0)
    {
        printf("BUG!  Illegal player position (cx < 0): (cx = %d, cy = %d)\n", cx, cy);
        // *(double *)(-1) = 4321.0;
        // abort();
    }
    if (cx >= World.click_width)
    {
        printf("BUG!  Illegal player position (cx > world width): (cx = %d, cy = %d)\n", cx, cy);
        // *(double *)(-1) = 4321.0;
        // abort();
    }
    if (cy < 0)
    {
        printf("BUG!  Illegal player position (cy < 0): (cx = %d, cy = %d)\n", cx, cy);
        // *(double *)(-1) = 4321.0;
        // abort();
    }
    if (cy >= World.click_height)
    {
        printf("BUG!  Illegal player position (cy > world height): (cx = %d, cy = %d)\n", cx, cy);
        // *(double *)(-1) = 4321.0;
        // abort();
    }
#endif
    pos->cx = cx;
    pos->x = CLICK_TO_PIXEL(cx);
    pos->cy = cy;
    pos->y = CLICK_TO_PIXEL(cy);
}

void Player_position_init_clicks(player_t *pl, int cx, int cy)
{
    Player_position_set_clicks(pl, cx, cy);
    Player_position_remember(pl);
}

void Player_position_limit(player_t *pl)
{
    int x = pl->pos.x, ox = x;
    int y = pl->pos.y, oy = y;

    LIMIT(x, 0, World.width - 1);
    LIMIT(y, 0, World.height - 1);
    if (x != ox || y != oy)
    {
        Player_position_set_clicks(pl, PIXEL_TO_CLICK(x), PIXEL_TO_CLICK(y));
    }
}

void Player_position_debug(player_t *pl, const char *msg)
{
#if DEVELOPMENT
    int i;

    printf("pl %s pos dump: ", pl->name);
    if (msg)
        printf("(%s)", msg);
    printf("\n");
    printf("\tB %d, %d, P %d, %d, C %d, %d, O %d, %d\n",
           pl->pos.bx,
           pl->pos.by,
           pl->pos.x,
           pl->pos.y,
           pl->pos.cx,
           pl->pos.cy,
           pl->prevpos.x,
           pl->prevpos.y);
    for (i = 0; i < pl->ship->num_points; i++)
    {
        printf("\t%2d\tB %d, %d, P %d, %d, C %d, %d, O %d, %d\n",
               i,
               (int)((pl->pos.x + pl->ship->pts[i][pl->dir].x) / BLOCK_SZ),
               (int)((pl->pos.y + pl->ship->pts[i][pl->dir].y) / BLOCK_SZ),
               (int)(pl->pos.x + pl->ship->pts[i][pl->dir].x),
               (int)(pl->pos.y + pl->ship->pts[i][pl->dir].y),
               (int)(pl->pos.cx + FLOAT_TO_CLICK(pl->ship->pts[i][pl->dir].x)),
               (int)(pl->pos.cy + FLOAT_TO_CLICK(pl->ship->pts[i][pl->dir].y)),
               (int)(pl->prevpos.x + pl->ship->pts[i][pl->dir].x),
               (int)(pl->prevpos.y + pl->ship->pts[i][pl->dir].y));
    }
#endif
}
