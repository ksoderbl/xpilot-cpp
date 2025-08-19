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

#include "const.h"
#include "draw.h"

#include "netclient.h"
#include "paint.h"

/*
 * Globals.
 */
ipos_t world;
ipos_t realWorld;

// from xinit.cpp
int draw_width, draw_height;

int num_spark_colors;

unsigned short team; /* What team is the player on? */
bool players_exposed;

short ext_view_width;   /* Width of extended visible area */
short ext_view_height;  /* Height of extended visible area */
int active_view_width;  /* Width of active map area displayed. */
int active_view_height; /* Height of active map area displayed. */
int ext_view_x_offset;  /* Offset ext_view_width */
int ext_view_y_offset;  /* Offset ext_view_height */
uint8_t debris_colors;  /* Number of debris intensities from server */

char modBankStr[NUM_MODBANKS][MAX_CHARS]; /* modifier banks */

int maxKeyDefs;

long loops = 0;
long loopsSlow = 0; /* Proceeds slower than loops */
double timePerFrame = 0.0;
static double time_counter = 0.0;

double scaleFactor;
double scaleFactor_s;
short scaleArray[SCALE_ARRAY_SIZE];

int Check_view_dimensions(void)
{
    int width_wanted = draw_width;
    int height_wanted = draw_height;
    int srv_width, srv_height;

    width_wanted = (int)(width_wanted * scaleFactor + 0.5);
    height_wanted = (int)(height_wanted * scaleFactor + 0.5);

    srv_width = width_wanted;
    srv_height = height_wanted;
    LIMIT(srv_height, MIN_VIEW_SIZE, MAX_VIEW_SIZE);
    LIMIT(srv_width, MIN_VIEW_SIZE, MAX_VIEW_SIZE);
    if (ext_view_width != srv_width || ext_view_height != srv_height)
        Send_display();

    active_view_width = ext_view_width;
    active_view_height = ext_view_height;
    ext_view_x_offset = 0;
    ext_view_y_offset = 0;
    if (width_wanted > ext_view_width)
    {
        ext_view_width = width_wanted;
        ext_view_x_offset = (width_wanted - active_view_width) / 2;
    }
    if (height_wanted > ext_view_height)
    {
        ext_view_height = height_wanted;
        ext_view_y_offset = (height_wanted - active_view_height) / 2;
    }

    return 0;
}
