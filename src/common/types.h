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

#ifndef        TYPES_H
#define        TYPES_H

typedef unsigned char        u_byte;

/*
 * On some systems an enum is smaller than an int.
 * On others bool is already a builtin type.
 * Using preprocessor macros to circumvent both situations.
 */
#ifndef __cplusplus
#define false        0
#define true        1
#define bool        int
#endif

typedef        float        DFLOAT;

typedef struct { DFLOAT x, y; }        vector;
typedef vector                        position;
typedef struct { int x, y; }        ivec;
typedef ivec                        ipos;

#endif
