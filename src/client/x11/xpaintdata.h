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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef XPAINTDATA_H
#define XPAINTDATA_H

#include "draw.h"
#include "types.h"

#include "xpaint.h"

/*
 * Local types and data for painting.
 */

typedef struct
{
    short x0, y0, x1, y1;
} refuel_t;

typedef struct
{
    short x0, y0, x1, y1;
    u_byte tractor;
} connector_t;

typedef struct
{
    unsigned char color, dir;
    short x, y, len;
} laser_t;

typedef struct
{
    short x, y, dir;
    unsigned char len;
} missile_t;

typedef struct
{
    short x, y, id;
} ball_t;

typedef struct
{
    short x, y, id, dir;
    u_byte shield, cloak, eshield;
    u_byte phased, deflector;
} ship_t;

typedef struct
{
    short x, y, teammine, id;
} mine_t;

typedef struct
{
    short x, y, type;
} itemtype_t;

typedef struct
{
    short x, y, size;
} ecm_t;

typedef struct
{
    short x1, y1, x2, y2;
} trans_t;

typedef struct
{
    short x, y, count;
} paused_t;

typedef struct
{
    short x, y, size;
} radar_t;

typedef struct
{
    short x, y, type;
} vcannon_t;

typedef struct
{
    short x, y;
    long fuel;
} vfuel_t;

typedef struct
{
    short x, y, xi, yi, type;
} vbase_t;

typedef struct
{
    u_byte x, y;
} debris_t;

typedef struct
{
    short x, y, xi, yi, type;
} vdecor_t;

typedef struct
{
    short x, y;
    u_byte wrecktype, size, rotation;
} wreckage_t;

typedef struct
{
    short x, y;
    u_byte type, size, rotation;
} asteroid_t;

typedef struct
{
    short x, y;
} wormhole_t;

extern refuel_t *refuel_ptr;
extern int num_refuel, max_refuel;
extern connector_t *connector_ptr;
extern int num_connector, max_connector;
extern laser_t *laser_ptr;
extern int num_laser, max_laser;
extern missile_t *missile_ptr;
extern int num_missile, max_missile;
extern ball_t *ball_ptr;
extern int num_ball, max_ball;
extern ship_t *ship_ptr;
extern int num_ship, max_ship;
extern mine_t *mine_ptr;
extern int num_mine, max_mine;
extern itemtype_t *itemtype_ptr;
extern int num_itemtype, max_itemtype;
extern ecm_t *ecm_ptr;
extern int num_ecm, max_ecm;
extern trans_t *trans_ptr;
extern int num_trans, max_trans;
extern paused_t *paused_ptr;
extern int num_paused, max_paused;
extern radar_t *radar_ptr;
extern int num_radar, max_radar;
extern vcannon_t *vcannon_ptr;
extern int num_vcannon, max_vcannon;
extern vfuel_t *vfuel_ptr;
extern int num_vfuel, max_vfuel;
extern vbase_t *vbase_ptr;
extern int num_vbase, max_vbase;
extern debris_t *debris_ptr[DEBRIS_TYPES];
extern int num_debris[DEBRIS_TYPES],
    max_debris[DEBRIS_TYPES];
extern debris_t *fastshot_ptr[DEBRIS_TYPES * 2];
extern int num_fastshot[DEBRIS_TYPES * 2],
    max_fastshot[DEBRIS_TYPES * 2];
extern vdecor_t *vdecor_ptr;
extern int num_vdecor, max_vdecor;
extern wreckage_t *wreckage_ptr;
extern int num_wreckage, max_wreckage;
extern asteroid_t *asteroid_ptr;
extern int num_asteroids, max_asteroids;
extern wormhole_t *wormhole_ptr;
extern int num_wormholes, max_wormholes;

extern long start_loops, end_loops;
extern long time_left;

#define RESET_FG() (current_foreground = -1)
#define SET_FG(PIXEL)                  \
    if ((PIXEL) == current_foreground) \
        ;                              \
    else                               \
        XSetForeground(dpy, gc, current_foreground = (PIXEL))

extern unsigned long current_foreground;

#define MAX_LINE_WIDTH 4

extern XRectangle *rect_ptr[MAX_COLORS];
extern int num_rect[MAX_COLORS], max_rect[MAX_COLORS];
extern XArc *arc_ptr[MAX_COLORS];
extern int num_arc[MAX_COLORS], max_arc[MAX_COLORS];
extern XSegment *seg_ptr[MAX_COLORS];
extern int num_seg[MAX_COLORS], max_seg[MAX_COLORS];

extern int eyesId;     /* Player we get frame updates for */
extern short snooping; /* are we snooping on someone else? */

extern void Rectangle_start(void);
extern void Rectangle_end(void);
extern int Rectangle_add(int color, int x, int y, int width, int height);
extern void Arc_start(void);
extern void Arc_end(void);
extern int Arc_add(int color,
                   int x, int y,
                   int width, int height,
                   int angle1, int angle2);
extern void Segment_start(void);
extern void Segment_end(void);
extern int Segment_add(int color, int x1, int y1, int x2, int y2);

#endif
