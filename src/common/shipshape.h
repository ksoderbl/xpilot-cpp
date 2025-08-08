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

#ifndef SHIPSHAPE_H
#define SHIPSHAPE_H

#include "types.h"
#include "const.h"

/*
 * Please don't change any of these maxima.
 * It will create incompatibilities and frustration.
 */
#define MIN_SHIP_PTS 3
#define MAX_SHIP_PTS 24
#define MAX_GUN_PTS 3
#define MAX_LIGHT_PTS 3
#define MAX_RACK_PTS 4

typedef struct
{                                  /* Defines wire-obj, i.e. ship */
    position_t *pts[MAX_SHIP_PTS]; /* the shape rotated many ways */
    int num_points;                /* total points in object */
    position_t engine[RES];        /* Engine position */
    position_t m_gun[RES];         /* Main gun position */
    int num_l_gun,
        num_r_gun,
        num_l_rgun,
        num_r_rgun;                 /* number of additional cannons */
    position_t *l_gun[MAX_GUN_PTS], /* Additional cannon positions, left*/
        *r_gun[MAX_GUN_PTS],        /* Additional cannon positions, right*/
        *l_rgun[MAX_GUN_PTS],       /* Additional rear cannon positions, left*/
        *r_rgun[MAX_GUN_PTS];       /* Additional rear cannon positions, right*/
    int num_l_light,                /* Number of lights */
        num_r_light;
    position_t *l_light[MAX_LIGHT_PTS], /* Left and right light positions */
        *r_light[MAX_LIGHT_PTS];
    int num_m_rack; /* Number of missile racks */
    position_t *m_rack[MAX_RACK_PTS];
    int shield_radius; /* Radius of shield used by client. */

#ifdef _NAMEDSHIPS
    char *name;
    char *author;
#endif
} shipshape_t;

extern shipshape_t *Default_ship(void);
extern void Free_ship_shape(shipshape_t *w);
extern shipshape_t *Parse_shape_str(char *str);
extern shipshape_t *Convert_shape_str(char *str);
extern void Calculate_shield_radius(shipshape_t *w);
extern int Validate_shape_str(char *str);
extern void Convert_ship_2_string(shipshape_t *w, char *buf, char *ext, unsigned shape_version);
void Rotate_point(position_t pt[RES]);

#endif
