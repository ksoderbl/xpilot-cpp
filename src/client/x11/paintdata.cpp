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

#include <array>
#include <vector>

#include <X11/Xlib.h>

#include "paint.h"

#include "paintdata.h"
#include "record.h"
#include "xinit.h"

std::array<std::vector<XRectangle>, MAX_COLORS> rectanglesArray;
std::array<std::vector<XArc>, MAX_COLORS> arcsArray;
std::array<std::vector<XSegment>, MAX_COLORS> segmentsArray;

typedef struct
{
    unsigned long color;
    XArc arc;
} rgb_arc_t;

std::vector<rgb_arc_t> rgbArcsVector;

unsigned long current_foreground;

void Rectangle_start(void)
{
    int i;

    for (i = 0; i < clientOptions.maxColors; i++)
        rectanglesArray[i].clear();
}

void Rectangle_end(void)
{
    int i;

    for (i = 0; i < clientOptions.maxColors; i++)
    {
        if (rectanglesArray[i].size() > 0)
        {
            SET_FG(colors[i].pixel);
            auto &rectangles = rectanglesArray[i];
            rd.fillRectangles(dpy, drawPixmap, gameGC,
                              &rectangles[0], rectangles.size());
            rectanglesArray[i].clear();
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
    rectanglesArray[color].push_back(t);

    return 0;
}

void Arc_start(void)
{
    int i;

    for (i = 0; i < clientOptions.maxColors; i++)
        arcsArray[i].clear();
    rgbArcsVector.clear();
}

void Arc_end(void)
{
    int i;

    for (i = 0; i < clientOptions.maxColors; i++)
    {
        if (arcsArray[i].size() > 0)
        {
            SET_FG(colors[i].pixel);
            auto &arcs = arcsArray[i];
            rd.drawArcs(dpy, drawPixmap, gameGC, &arcs[0], arcs.size());
            arcsArray[i].clear();
        }
    }

    /* fullcolor arcs */
    for (rgb_arc_t &rgbArc : rgbArcsVector)
    {
        SET_FG(rgbArc.color);
        rd.drawArc(dpy, drawPixmap, gameGC,
                   rgbArc.arc.x, rgbArc.arc.y,
                   rgbArc.arc.width, rgbArc.arc.height,
                   rgbArc.arc.angle1, rgbArc.arc.angle2);
    }
    if (rgbArcsVector.size() > 0)
        rgbArcsVector.clear();
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
    arcsArray[color].push_back(t);

    return 0;
}

int Arc_add_rgb(unsigned long color,
                int fallback_color,
                int x, int y,
                int width, int height,
                int angle1, int angle2)
{
    rgb_arc_t t;

    /* hack */
    if (!clientOptions.fullColor)
        return Arc_add(fallback_color, x, y, width, height, angle1, angle2);

    t.color = color;
    t.arc.x = WINSCALE(x);
    t.arc.y = WINSCALE(y);
    t.arc.width = WINSCALE(width + x) - t.arc.x;
    t.arc.height = WINSCALE(height + y) - t.arc.y;

    t.arc.angle1 = angle1;
    t.arc.angle2 = angle2;
    rgbArcsVector.push_back(t);

    return 0;
}

void Segment_start(void)
{
    int i;

    for (i = 0; i < clientOptions.maxColors; i++)
        segmentsArray[i].clear();
}

void Segment_end(void)
{
    int i;

    for (i = 0; i < clientOptions.maxColors; i++)
    {
        if (segmentsArray[i].size() > 0)
        {
            SET_FG(colors[i].pixel);
            // SET_FG(randomMT());
            auto &segments = segmentsArray[i];
            rd.drawSegments(dpy, drawPixmap, gameGC, &segments[0], segments.size());
            segmentsArray[i].clear();
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
    segmentsArray[color].push_back(t);

    return 0;
}

void paintdataCleanup(void)
{
    int i;

    for (i = 0; i < MAX_COLORS; i++)
    {
        rectanglesArray[i].clear();
        arcsArray[i].clear();
        segmentsArray[i].clear();
        rgbArcsVector.clear();
    }
}
