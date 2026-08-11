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

    clpos_t *pts[MAX_SHIP_PTS2]; /* the shape rotated many ways */
    int num_points = 0;          /* total points in object */
    int num_orig_points = 0;     /* points before SSHACK */
    clpos_t engine;              /* Engine position */
    clpos_t mainGun;             /* Main gun position */
    int num_l_gun = 0,
        num_r_gun = 0,
        num_l_rgun = 0,
        num_r_rgun = 0;                 /* number of additional cannons */
    clpos_t leftGuns[MAX_GUN_PTS];      /* Additional cannon positions, left*/
    clpos_t rightGuns[MAX_GUN_PTS];     /* Additional cannon positions, right*/
    clpos_t leftRearGuns[MAX_GUN_PTS];  /* Additional rear cannon positions, left*/
    clpos_t rightRearGuns[MAX_GUN_PTS]; /* Additional rear cannon positions, right*/
    int num_l_light = 0,                /* Number of lights */
        num_r_light = 0;
    clpos_t leftLights[MAX_LIGHT_PTS]; /* Left and right light positions */
    clpos_t rightLights[MAX_LIGHT_PTS];
    int num_m_rack = 0; /* Number of missile racks */
    clpos_t missileRacks[MAX_RACK_PTS];
    int shield_radius = 0; /* Radius of shield used by client. */

#ifdef _NAMEDSHIPS
    char *name;
    char *author;
#endif

    std::vector<clpos_t> &getPoints(int dir)
    {
        // warn("getPoints, dir %d", dir);
        rotateShip(dir);
        return points;
    }

    clpos_t getEngineClickPosition(int dir)
    {
        rotateShip(dir);
        return engineClickPosition;
    }
    clpos_t getMainGunClickPosition(int dir)
    {
        rotateShip(dir);
        return mainGunClickPosition;
    }
    std::vector<clpos_t> &getLeftGunClickPositions(int dir)
    {
        rotateShip(dir);
        return leftGunClickPositions;
    }
    std::vector<clpos_t> &getRightGunClickPositions(int dir)
    {
        rotateShip(dir);
        return rightGunClickPositions;
    }
    std::vector<clpos_t> &getLeftRearGunClickPositions(int dir)
    {
        rotateShip(dir);
        return leftRearGunClickPositions;
    }
    std::vector<clpos_t> &getRightRearGunClickPositions(int dir)
    {
        rotateShip(dir);
        return rightRearGunClickPositions;
    }
    std::vector<clpos_t> &getLeftLightClickPositions(int dir)
    {
        rotateShip(dir);
        return leftLightClickPositions;
    }
    std::vector<clpos_t> &getRightLightClickPositions(int dir)
    {
        rotateShip(dir);
        return rightLightClickPositions;
    }
    std::vector<clpos_t> &getMissileRackClickPositions(int dir)
    {
        rotateShip(dir);
        return missileRackClickPositions;
    }

    void rotateShip(int dir);

    std::vector<clpos_t> points;
    clpos_t engineClickPosition;
    clpos_t mainGunClickPosition;
    std::vector<clpos_t> leftGunClickPositions;
    std::vector<clpos_t> rightGunClickPositions;
    std::vector<clpos_t> leftRearGunClickPositions;
    std::vector<clpos_t> rightRearGunClickPositions;
    std::vector<clpos_t> leftLightClickPositions;
    std::vector<clpos_t> rightLightClickPositions;
    std::vector<clpos_t> missileRackClickPositions;

    int currentDir = -1; // current rotated direction
    int currentDirCacheHits = 0;
    int currentDirCacheMisses = 0;
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
