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

#ifndef DRAW_H
#define DRAW_H

/* need position */
#include "types.h"
#include "const.h"

/*
 * Abstract (non-display system specific) drawing definitions.
 *
 * This file should not contain any X window stuff.
 */

/*
 * The server supports only 4 colors, except for spark/debris, which
 * may have 8 different colors.
 */
#define NUM_COLORS 4

#define BLACK 0
#define WHITE 1
#define BLUE 2
#define RED 3

/*
 * The minimum and maximum playing window sizes supported by the server.
 */
#define MIN_VIEW_SIZE 384
#define MAX_VIEW_SIZE 1024
#define DEF_VIEW_SIZE 768

/*
 * Spark rand limits.
 */
#define MIN_SPARK_RAND 0    /* Not display spark */
#define MAX_SPARK_RAND 0x80 /* Always display spark */
#define DEF_SPARK_RAND 0x55 /* 66% */

#define DSIZE 4 /* Size of diamond (on radar) */

#endif
