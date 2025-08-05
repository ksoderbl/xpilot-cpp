/* $Id: dbuff.h,v 5.0 2001/04/07 20:00:58 dik Exp $
 *
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

#ifndef DBUFF_H
#define DBUFF_H

typedef enum
{
    PIXMAP_COPY
} dbuff_t;

typedef struct
{
    Display *display;
    dbuff_t type;
    Colormap xcolormap;
    unsigned long drawing_planes;
    int colormap_index;
    XColor *colormaps[2];
    int colormap_size;
    unsigned long drawing_plane_masks[2];
    unsigned long *planes;
    unsigned long pixel;
} dbuff_state_t;

extern dbuff_state_t *dbuf_state; /* Holds current dbuff state */

dbuff_state_t *start_dbuff(Display *display, Colormap cmap,
                           dbuff_t type,
                           int num_planes, XColor *colors);
void dbuff_switch(dbuff_state_t *state);
void end_dbuff(dbuff_state_t *state);

#endif
