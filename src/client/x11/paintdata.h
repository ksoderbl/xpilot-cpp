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

#ifndef PAINTDATA_H
#define PAINTDATA_H

#include "draw.h"
#include "types.h"

#include "xpaint.h"

#define RESET_FG() (current_foreground = -1)
#define SET_FG(PIXEL)                  \
    if ((PIXEL) == current_foreground) \
        ;                              \
    else                               \
        XSetForeground(dpy, gameGC, current_foreground = (PIXEL))

extern unsigned long current_foreground;

#define MAX_LINE_WIDTH 4

extern XRectangle *rect_ptr[MAX_COLORS];
extern int num_rect[MAX_COLORS], max_rect[MAX_COLORS];
extern XArc *arc_ptr[MAX_COLORS];
extern int num_arc[MAX_COLORS], max_arc[MAX_COLORS];
extern XSegment *seg_ptr[MAX_COLORS];
extern int num_seg[MAX_COLORS], max_seg[MAX_COLORS];

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
