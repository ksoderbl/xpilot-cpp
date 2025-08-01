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

#include "paint.h"

// from xinit.cpp
int draw_width, draw_height;

int num_spark_colors;

unsigned short team; /* What team is the player on? */

short ext_view_width;   /* Width of extended visible area */
short ext_view_height;  /* Height of extended visible area */
int active_view_width;  /* Width of active map area displayed. */
int active_view_height; /* Height of active map area displayed. */
int ext_view_x_offset;  /* Offset ext_view_width */
int ext_view_y_offset;  /* Offset ext_view_height */
u_byte debris_colors;   /* Number of debris intensities from server */

char modBankStr[NUM_MODBANKS][MAX_CHARS]; /* modifier banks */

int maxKeyDefs;
long loops = 0;
int maxMessages;      /* Max. number of messages to display */
int messagesToStdout; /* Send messages to standard output */
bool selectionAndHistory = false;

DFLOAT scaleFactor;
DFLOAT scaleFactor_s;
short scaleArray[SCALE_ARRAY_SIZE];
