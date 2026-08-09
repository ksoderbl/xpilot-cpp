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

#pragma once

#include <string>
#include <vector>

#include "types.h"
#include "click.h"
#include "const.h"
#include "xperror.h"

/*
 * Please don't change any of these maxima.
 * It will create incompatibilities and frustration.
 */
#define MIN_SHIP_PTS 3
#define MAX_SHIP_PTS 24
/* SSHACK needs to double the vertices */
#define MAX_SHIP_PTS2 (MAX_SHIP_PTS * 2)
#define MAX_GUN_PTS 3
#define MAX_LIGHT_PTS 3
#define MAX_RACK_PTS 4

typedef struct
{
    clpos_t *pts[MAX_SHIP_PTS2]; /* the shape rotated many ways */
    int num_points;              /* total points in object */
    int num_orig_points;         /* points before SSHACK */
    clpos_t cashed_pts[MAX_SHIP_PTS2];
    int cashed_dir;
} shape_t;

/* Defines wire-obj, i.e. ship */
class ShipShape
{
public:
    ShipShape();
    ~ShipShape();

    clpos_t *pts[MAX_SHIP_PTS];       /* the shape rotated many ways */
    int num_points;                   /* total points in object */
    clpos_t engine[ANGLE_RESOLUTION]; /* Engine position */
    clpos_t m_gun[ANGLE_RESOLUTION];  /* Main gun position */
    int num_l_gun,
        num_r_gun,
        num_l_rgun,
        num_r_rgun;              /* number of additional cannons */
    clpos_t *l_gun[MAX_GUN_PTS], /* Additional cannon positions, left*/
        *r_gun[MAX_GUN_PTS],     /* Additional cannon positions, right*/
        *l_rgun[MAX_GUN_PTS],    /* Additional rear cannon positions, left*/
        *r_rgun[MAX_GUN_PTS];    /* Additional rear cannon positions, right*/
    int num_l_light,             /* Number of lights */
        num_r_light;
    clpos_t *l_light[MAX_LIGHT_PTS], /* Left and right light positions */
        *r_light[MAX_LIGHT_PTS];
    int num_m_rack; /* Number of missile racks */
    clpos_t *m_rack[MAX_RACK_PTS];
    int shield_radius; /* Radius of shield used by client. */

#ifdef _NAMEDSHIPS
    char *name;
    char *author;
#endif
};

extern ShipShape *Default_ship(void);
extern void Free_ship_shape(ShipShape *ship);
extern ShipShape *Parse_shape_str(char *str);
extern ShipShape *Convert_shape_str(char *str);
extern int Calculate_shield_radius(ShipShape *ship);
extern int Validate_shape_str(char *str);
extern void Convert_ship_2_string(ShipShape *ship, char *buf, char *ext,
                                  unsigned shape_version);
extern void Rotate_point(clpos_t pt[ANGLE_RESOLUTION]);
extern void Rotate_position(position_t pt[ANGLE_RESOLUTION]);
extern clpos_t *Shape_get_points(shape_t *s, int dir);

static inline clpos_t
Ship_get_point_clpos(ShipShape *ship, int i, int dir)
{
    // warn("Ship_get_point_clpos: i=%d, dir=%d", i, dir);
    // clpos_t pos;
    // pos.cx = 0;
    // pos.cy = 0;
    // return pos;
    return ship->pts[i][dir];
}
static inline clpos_t
Ship_get_engine_clpos(ShipShape *ship, int dir)
{
    return ship->engine[dir];
}
static inline clpos_t
Ship_get_m_gun_clpos(ShipShape *ship, int dir)
{
    return ship->m_gun[dir];
}
static inline clpos_t
Ship_get_l_gun_clpos(ShipShape *ship, int gun, int dir)
{
    return ship->l_gun[gun][dir];
}
static inline clpos_t
Ship_get_r_gun_clpos(ShipShape *ship, int gun, int dir)
{
    return ship->r_gun[gun][dir];
}
static inline clpos_t
Ship_get_l_rgun_clpos(ShipShape *ship, int gun, int dir)
{
    return ship->l_rgun[gun][dir];
}
static inline clpos_t
Ship_get_r_rgun_clpos(ShipShape *ship, int gun, int dir)
{
    return ship->r_rgun[gun][dir];
}
static inline clpos_t
Ship_get_l_light_clpos(ShipShape *ship, int l, int dir)
{
    return ship->l_light[l][dir];
}
static inline clpos_t
Ship_get_r_light_clpos(ShipShape *ship, int l, int dir)
{
    return ship->r_light[l][dir];
}
static inline clpos_t
Ship_get_m_rack_clpos(ShipShape *ship, int rack, int dir)
{
    return ship->m_rack[rack][dir];
}

static inline position_t
Ship_get_point_position(ShipShape *ship, int i, int dir)
{
    return clpos2position(Ship_get_point_clpos(ship, i, dir));
}
static inline position_t
Ship_get_engine_position(ShipShape *ship, int dir)
{
    return clpos2position(Ship_get_engine_clpos(ship, dir));
}
static inline position_t
Ship_get_m_gun_position(ShipShape *ship, int dir)
{
    return clpos2position(Ship_get_m_gun_clpos(ship, dir));
}
static inline position_t
Ship_get_l_gun_position(ShipShape *ship, int gun, int dir)
{
    return clpos2position(Ship_get_l_gun_clpos(ship, gun, dir));
}
static inline position_t
Ship_get_r_gun_position(ShipShape *ship, int gun, int dir)
{
    return clpos2position(Ship_get_r_gun_clpos(ship, gun, dir));
}
static inline position_t
Ship_get_l_rgun_position(ShipShape *ship, int gun, int dir)
{
    return clpos2position(Ship_get_l_rgun_clpos(ship, gun, dir));
}
static inline position_t
Ship_get_r_rgun_position(ShipShape *ship, int gun, int dir)
{
    return clpos2position(Ship_get_r_rgun_clpos(ship, gun, dir));
}
static inline position_t
Ship_get_l_light_position(ShipShape *ship, int l, int dir)
{
    return clpos2position(Ship_get_l_light_clpos(ship, l, dir));
}
static inline position_t
Ship_get_r_light_position(ShipShape *ship, int l, int dir)
{
    return clpos2position(Ship_get_r_light_clpos(ship, l, dir));
}
static inline position_t
Ship_get_m_rack_position(ShipShape *ship, int rack, int dir)
{
    return clpos2position(Ship_get_m_rack_clpos(ship, rack, dir));
}
