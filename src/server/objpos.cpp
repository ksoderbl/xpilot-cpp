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

#include "objpos.h"

// TODO: Remove pixel positions, store only subpixel position (i.e. clicks)
void Object_position_set_clpos(object_t *obj, clpos_t pos)
{
#if 1
    if (pos.cx < 0)
    {
        printf("BUG!  Illegal object position (cx < 0): (cx = %d, cy = %d)\n", pos.cx, pos.cy);
        // *(double *)(-1) = 4321.0;
        // abort();
    }
    if (pos.cx >= World.cwidth)
    {
        printf("BUG!  Illegal object position (cx > world width): (cx = %d, cy = %d)\n", pos.cx, pos.cy);
        // *(double *)(-1) = 4321.0;
        // abort();
    }
    if (pos.cy < 0)
    {
        printf("BUG!  Illegal object position (cy < 0): (cx = %d, cy = %d)\n", pos.cx, pos.cy);
        // *(double *)(-1) = 4321.0;
        // abort();
    }
    if (pos.cy >= World.cheight)
    {
        printf("BUG!  Illegal object position (cy > world height): (cx = %d, cy = %d)\n", pos.cx, pos.cy);
        // *(double *)(-1) = 4321.0;
        // abort();
    }
#endif
    obj->pos = pos;
}

void Object_position_init_clpos(object_t *obj, clpos_t pos)
{
    Object_position_set_clpos(obj, pos);
    Object_position_remember(obj);
}

void Player_position_restore(player_t *pl)
{
    Player_position_set_clicks(pl, pl->prevpos);
}

void Player_position_set_clicks(player_t *pl, clpos_t pos)
{
#if 1
    if (pos.cx < 0)
    {
        printf("BUG!  Illegal player position (cx < 0): (cx = %d, cy = %d)\n", pos.cx, pos.cy);
        // *(double *)(-1) = 4321.0;
        // abort();
    }
    if (pos.cx >= World.cwidth)
    {
        printf("BUG!  Illegal player position (cx > world width): (cx = %d, cy = %d)\n", pos.cx, pos.cy);
        // *(double *)(-1) = 4321.0;
        // abort();
    }
    if (pos.cy < 0)
    {
        printf("BUG!  Illegal player position (cy < 0): (cx = %d, cy = %d)\n", pos.cx, pos.cy);
        // *(double *)(-1) = 4321.0;
        // abort();
    }
    if (pos.cy >= World.cheight)
    {
        printf("BUG!  Illegal player position (cy > world height): (cx = %d, cy = %d)\n", pos.cx, pos.cy);
        // *(double *)(-1) = 4321.0;
        // abort();
    }
#endif
    pl->pos = pos;
}

void Player_position_init_clpos(player_t *pl, clpos_t pos)
{
    Player_position_set_clicks(pl, pos);
    Player_position_remember(pl);
}

void Player_position_limit(player_t *pl)
{
    clpos_t pos = pl->pos;
    clpos_t opos = pos;

    LIMIT(pos.cx, 0, World.cwidth - 1);
    LIMIT(pos.cy, 0, World.cheight - 1);
    if (pos.cx != opos.cx || pos.cy != opos.cy)
    {
        Player_position_set_clicks(pl, pos);
    }
}

void Player_position_debug(player_t *pl, const char *msg)
{
#if DEVELOPMENT
    int i;

    printf("pl %s pos dump: ", pl->name.c_str());
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
