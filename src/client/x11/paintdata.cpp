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

// #include <cstdlib>
// #include <cstdio>
// #include <cstring>
// #include <cerrno>
// #include <cmath>

// #include <unistd.h>
#include <X11/Xlib.h>

#include "commonmacros.h"
// #include "const.h"
// #include "strlcpy.h"

#include "paint.h"

// #include "xpconfig.h"
// #include "const.h"
// #include "xperror.h"
// #include "bit.h"
// #include "setup.h"
// #include "rules.h"
// #include "xpaint.h"
#include "paintdata.h"
#include "record.h"
// #include "xinit.h"
// #include "protoclient.h"
// #include "dbuff.h"

XRectangle *rect_ptr[MAX_COLORS];
int num_rect[MAX_COLORS], max_rect[MAX_COLORS];
XArc *arc_ptr[MAX_COLORS];
int num_arc[MAX_COLORS], max_arc[MAX_COLORS];
XSegment *seg_ptr[MAX_COLORS];
int num_seg[MAX_COLORS], max_seg[MAX_COLORS];

unsigned long current_foreground;

void Rectangle_start(void)
{
    int i;

    for (i = 0; i < maxColors; i++)
    {
        num_rect[i] = 0;
    }
}

void Rectangle_end(void)
{
    int i;

    for (i = 0; i < maxColors; i++)
    {
        if (num_rect[i] > 0)
        {
            SET_FG(colors[i].pixel);
            rd.fillRectangles(dpy, p_draw, gc, rect_ptr[i], num_rect[i]);
            RELEASE(rect_ptr[i], num_rect[i], max_rect[i]);
        }
    }
}

int Rectangle_add(int color, int x, int y, int width, int height)
{
    XRectangle t;

    t.x = WINSCALE(x);
    t.y = WINSCALE(y);
    t.width = WINSCALE(width);
    t.height = WINSCALE(height);

    STORE(XRectangle, rect_ptr[color], num_rect[color], max_rect[color], t);
    return 0;
}

void Arc_start(void)
{
    int i;

    for (i = 0; i < maxColors; i++)
    {
        num_arc[i] = 0;
    }
}

void Arc_end(void)
{
    int i;

    for (i = 0; i < maxColors; i++)
    {
        if (num_arc[i] > 0)
        {
            SET_FG(colors[i].pixel);
            rd.drawArcs(dpy, p_draw, gc, arc_ptr[i], num_arc[i]);
            RELEASE(arc_ptr[i], num_arc[i], max_arc[i]);
        }
    }
}

int Arc_add(int color,
            int x, int y,
            int width, int height,
            int angle1, int angle2)
{
    XArc t;

    t.x = WINSCALE(x);
    t.y = WINSCALE(y);
    t.width = WINSCALE(width + x) - t.x;
    t.height = WINSCALE(height + y) - t.y;

    t.angle1 = angle1;
    t.angle2 = angle2;
    STORE(XArc, arc_ptr[color], num_arc[color], max_arc[color], t);
    return 0;
}

void Segment_start(void)
{
    int i;

    for (i = 0; i < maxColors; i++)
    {
        num_seg[i] = 0;
    }
}

void Segment_end(void)
{
    int i;

    for (i = 0; i < maxColors; i++)
    {
        if (num_seg[i] > 0)
        {
            SET_FG(colors[i].pixel);
            rd.drawSegments(dpy, p_draw, gc,
                            seg_ptr[i], num_seg[i]);
            RELEASE(seg_ptr[i], num_seg[i], max_seg[i]);
        }
    }
}

int Segment_add(int color, int x1, int y1, int x2, int y2)
{
    XSegment t;

    t.x1 = WINSCALE(x1);
    t.y1 = WINSCALE(y1);
    t.x2 = WINSCALE(x2);
    t.y2 = WINSCALE(y2);
    STORE(XSegment, seg_ptr[color], num_seg[color], max_seg[color], t);
    return 0;
}

int Handle_time_left(long sec)
{
    if (sec >= 0 && sec < 10 && (time_left > sec || sec == 0))
    {
        XBell(dpy, 0);
        XFlush(dpy);
    }
    time_left = (sec >= 0) ? sec : 0;
    return 0;
}

void paintdataCleanup(void)
{
    int i;

    for (i = 0; i < MAX_COLORS; i++)
    {
        if (max_rect[i] > 0 && rect_ptr[i])
        {
            max_rect[i] = 0;
            free(rect_ptr[i]);
        }
        if (max_arc[i] > 0 && arc_ptr[i])
        {
            max_arc[i] = 0;
            free(arc_ptr[i]);
        }
        if (max_seg[i] > 0 && seg_ptr[i])
        {
            max_seg[i] = 0;
            free(seg_ptr[i]);
        }
    }
}
