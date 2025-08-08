/*
 * XMapEdit, the XPilot Map Editor.  Copyright (C) 1993 by
 *
 *      Aaron Averill
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
 *
 * Modifications to XMapEdit
 * 1996:
 *      Robert Templeman
 * 1997:
 *      William Docter
 */

typedef struct
{
   int color;
   int num_points;
   float x[5], y[5];
} segment_t;

typedef struct
{
   float x, y, width, height;
   int start, end;
} arc_t;

extern segment_t mapicon_seg[35];
extern segment_t mapicondet_seg[4];
extern int mapicon_ptr[91];
extern char iconmenu[36];

extern int smlmap_x, smlmap_y, smlmap_width, smlmap_height;
extern float smlmap_xscale, smlmap_yscale;
