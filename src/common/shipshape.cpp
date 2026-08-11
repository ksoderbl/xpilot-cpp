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

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cctype>
#include <cmath>

#include "commonmacros.h"
#include "commonproto.h"
#include "shipshape.h"

#include "version.h"
#include "xpconfig.h"
#include "const.h"
#include "xperror.h"

static bool debugShapeParsing = false;
static bool verboseShapeParsing = true;
static bool shapeLimits = true;

ShipShape::ShipShape()
{
    // warn("ShipShape::ShipShape: Hello world!");
    // Assume rotate never gets negative angles like this.
    currentDir = INT_MIN;
}

ShipShape::~ShipShape()
{
    // warn("ShipShape::~ShipShape: Goodbye cruel world!");
}

static inline clpos_t My_rotate_clpos(clpos_t pos, int dir)
{
    clpos_t ret;

    ret.cx = tcos(dir) * pos.cx - tsin(dir) * pos.cy;
    ret.cy = tsin(dir) * pos.cx + tcos(dir) * pos.cy;

    return ret;
}

void ShipShape::rotateShip(int dir)
{
    if (dir == currentDir)
    {
        // currentDirCacheHits++;
        // double cacheHitRatio = 100.0 * (double)currentDirCacheHits / (double)(currentDirCacheMisses + currentDirCacheHits);
        // warn("rotate, dir is already %d (cache hits %d/%d =  %f%%)",
        //      dir, currentDirCacheHits, (currentDirCacheMisses + currentDirCacheHits), cacheHitRatio);

        return;
    }

    clpos_t pos;

    // pts
    points.clear();
    for (int i = 0; i < this->num_points; i++)
    {
        pos = this->pts[i][0];
        pos = My_rotate_clpos(pos, dir);
        points.push_back(pos);
    }

    // engine
    pos = this->engine;
    engineClickPosition = My_rotate_clpos(pos, dir);

    // main gun
    pos = this->mainGun;
    mainGunClickPosition = My_rotate_clpos(pos, dir);

    // left guns
    leftGunClickPositions.clear();
    for (int i = 0; i < this->num_l_gun; i++)
    {
        pos = this->leftGuns[i];
        pos = My_rotate_clpos(pos, dir);
        leftGunClickPositions.push_back(pos);
    }

    // right guns
    rightGunClickPositions.clear();
    for (int i = 0; i < this->num_r_gun; i++)
    {
        pos = this->rightGuns[i];
        pos = My_rotate_clpos(pos, dir);
        rightGunClickPositions.push_back(pos);
    }

    // left rear guns
    leftRearGunClickPositions.clear();
    for (int i = 0; i < this->num_l_rgun; i++)
    {
        pos = this->leftRearGuns[i];
        pos = My_rotate_clpos(pos, dir);
        leftRearGunClickPositions.push_back(pos);
    }

    // right rear guns
    rightRearGunClickPositions.clear();
    for (int i = 0; i < this->num_r_rgun; i++)
    {
        pos = this->rightRearGuns[i];
        pos = My_rotate_clpos(pos, dir);
        rightRearGunClickPositions.push_back(pos);
    }

    // left lights
    leftLightClickPositions.clear();
    for (int i = 0; i < this->num_l_light; i++)
    {
        pos = this->leftLights[i];
        pos = My_rotate_clpos(pos, dir);
        leftLightClickPositions.push_back(pos);
    }

    // right lights
    rightLightClickPositions.clear();
    for (int i = 0; i < this->num_r_light; i++)
    {
        pos = this->rightLights[i];
        pos = My_rotate_clpos(pos, dir);
        rightLightClickPositions.push_back(pos);
    }

    // missile racks
    missileRackClickPositions.clear();
    for (int i = 0; i < this->num_m_rack; i++)
    {
        pos = this->missileRacks[i];
        pos = My_rotate_clpos(pos, dir);
        missileRackClickPositions.push_back(pos);
    }

    currentDir = dir;

    // currentDirCacheMisses++;
    // double cacheHitRatio = 100.0 * (double)currentDirCacheHits / (double)(currentDirCacheMisses + currentDirCacheHits);
    // warn("rotate, new dir is %d (cache hits %d/%d =  %f%%)",
    //      dir, currentDirCacheHits, (currentDirCacheMisses + currentDirCacheHits), cacheHitRatio);
}

static int Get_shape_keyword(char *keyw);

static void Ship_set_point_ipos(ShipShape *ship, int i, ipos_t pos)
{
    ship->pts[i][0] = ipos2clpos(pos);
}

static void Ship_set_engine_ipos(ShipShape *ship, ipos_t pos)
{
    ship->engine = ipos2clpos(pos);
}

static void Ship_set_m_gun_ipos(ShipShape *ship, ipos_t pos)
{
    ship->mainGun = ipos2clpos(pos);
}

static void Ship_set_l_gun_ipos(ShipShape *ship, int i, ipos_t pos)
{
    ship->leftGuns[i] = ipos2clpos(pos);
}

static void Ship_set_r_gun_ipos(ShipShape *ship, int i, ipos_t pos)
{
    ship->rightGuns[i] = ipos2clpos(pos);
}

static void Ship_set_l_rgun_ipos(ShipShape *ship, int i, ipos_t pos)
{
    ship->leftRearGuns[i] = ipos2clpos(pos);
}

static void Ship_set_r_rgun_ipos(ShipShape *ship, int i, ipos_t pos)
{
    ship->rightRearGuns[i] = ipos2clpos(pos);
}

static void Ship_set_l_light_ipos(ShipShape *ship, int i, ipos_t pos)
{
    ship->leftLights[i] = ipos2clpos(pos);
}

static void Ship_set_r_light_ipos(ShipShape *ship, int i, ipos_t pos)
{
    ship->rightLights[i] = ipos2clpos(pos);
}

static void Ship_set_m_rack_ipos(ShipShape *ship, int i, ipos_t pos)
{
    ship->missileRacks[i] = ipos2clpos(pos);
}

clpos_t *Shape_get_points(shape_t *s, int dir)
{
    int i;

    /* kps - optimize if cashed_dir == dir */
    for (i = 0; i < s->num_points; i++)
        s->cashed_pts[i] = s->pts[i][dir];

    return s->cashed_pts;
}

void Rotate_point(clpos_t pt[ANGLE_RESOLUTION])
{
    int i;

    for (i = 1; i < ANGLE_RESOLUTION; i++)
    {
        // pt[i].x = tcos(i) * pt[0].x - tsin(i) * pt[0].y;
        // pt[i].y = tsin(i) * pt[0].x + tcos(i) * pt[0].y;
        double cx = tcos(i) * pt[0].cx - tsin(i) * pt[0].cy;
        double cy = tsin(i) * pt[0].cx + tcos(i) * pt[0].cy;
        pt[i].cx = round(cx);
        pt[i].cy = round(cy);
    }
}

void Rotate_position(position_t pt[ANGLE_RESOLUTION])
{
    int i;

    for (i = 1; i < ANGLE_RESOLUTION; i++)
    {
        pt[i].x = tcos(i) * pt[0].x - tsin(i) * pt[0].y;
        pt[i].y = tsin(i) * pt[0].x + tcos(i) * pt[0].y;
    }
}

void Rotate_ship(ShipShape *ship)
{
    int i;

    for (i = 0; i < ship->num_points; i++)
        Rotate_point(&ship->pts[i][0]);
}

/*
 * Return a pointer to a default ship.
 * This function should always succeed,
 * therefore no malloc()ed memory is used.
 */
ShipShape *Default_ship(void)
{
    static ShipShape sh{};
    static clpos_t pts[6][ANGLE_RESOLUTION];

    if (!sh.num_points)
    {
        ipos_t pos;

        sh.num_points = 3;

        sh.pts[0] = &pts[0][0];
        pos.x = 14;
        pos.y = 0;
        Ship_set_point_ipos(&sh, 0, pos);

        sh.pts[1] = &pts[1][0];
        pos.x = -8;
        pos.y = 8;
        Ship_set_point_ipos(&sh, 1, pos);

        sh.pts[2] = &pts[2][0];
        pos.x = -8;
        pos.y = -8;
        Ship_set_point_ipos(&sh, 2, pos);

        pos.x = -8;
        pos.y = 0;
        Ship_set_engine_ipos(&sh, pos);

        pos.x = 14;
        pos.y = 0;
        Ship_set_m_gun_ipos(&sh, pos);

        sh.num_l_light = 1;
        pos.x = -8;
        pos.y = 8;
        Ship_set_l_light_ipos(&sh, 0, pos);

        sh.num_r_light = 1;
        pos.x = -8;
        pos.y = -8;
        Ship_set_r_light_ipos(&sh, 0, pos);

        sh.num_m_rack = 1;
        pos.x = 14;
        pos.y = 0;
        Ship_set_m_rack_ipos(&sh, 0, pos);

        sh.num_l_gun = sh.num_r_gun = sh.num_l_rgun = sh.num_r_rgun = 0;

        Rotate_ship(&sh);
    }

    return &sh;
}

typedef struct
{
    int todo, done;
    uint8_t pt[32][32];
    ipos_t chk[32 * 32];
} grid_t;

grid_t grid;

/*
 * Macros to simplify limit-checking for ship points.
 * Until XPilot goes C++.
 */
#define GRID_PT(x, y) grid.pt[(x) + 15][(y) + 15]
#define GRID_ADD(ax, ay) (GRID_PT(ax, ay) = 2,               \
                          grid.chk[grid.todo].x = (ax) + 15, \
                          grid.chk[grid.todo].y = (ay) + 15, \
                          grid.todo++)
#define GRID_GET(ax, ay) ((ax) = (int)grid.chk[grid.done].x - 15, \
                          (ay) = (int)grid.chk[grid.done].y - 15, \
                          grid.done++)
// #define GRID_CHK(ax, ay) (GRID_PT(ax, ay) == 2)
#define GRID_READY() (grid.done >= grid.todo)
#define GRID_RESET() (memset(grid.pt, 0, sizeof grid.pt), \
                      grid.done = 0,                      \
                      grid.todo = 0)

static void Grid_set_value(grid_t *grid_p, int x, int y, int value)
{
    int ix = x + 15;
    int iy = y + 15;
    // TODO: check ix, iy is inside the grid
    grid_p->pt[ix][iy] = value;
}
static int Grid_get_value(grid_t *grid_p, int x, int y)
{
    int ix = x + 15;
    int iy = y + 15;
    // TODO: check ix, iy is inside the grid
    return grid_p->pt[ix][iy];
}

static bool Grid_point_is_outside_ship(grid_t *grid_p, ipos_t pt)
{
    int value = Grid_get_value(grid_p, pt.x, pt.y);
    // warn("Grid_get_value: %d,%d = %d", pt.x, pt.y, value);
    bool is_outside = value == 2;
    return is_outside;
}

static int shape2wire(std::string ship_shape_str, ShipShape *ship)
{
    int i, j, x, y, dx, dy, max, shape_version = 0;
    ipos_t pt[MAX_SHIP_PTS2], in, engine, m_gun;
    ipos_t l_light[MAX_LIGHT_PTS], r_light[MAX_LIGHT_PTS];
    ipos_t l_gun[MAX_GUN_PTS], r_gun[MAX_GUN_PTS];
    ipos_t l_rgun[MAX_GUN_PTS], r_rgun[MAX_GUN_PTS], m_rack[MAX_RACK_PTS];
    bool mainGunSet = false, engineSet = false;
    char *str, *teststr;
    char keyw[20], buf[MSG_LEN];

    ship->num_points = 0;
    ship->num_l_gun = 0;
    ship->num_r_gun = 0;
    ship->num_l_rgun = 0;
    ship->num_r_rgun = 0;
    ship->num_l_light = 0;
    ship->num_r_light = 0;
    ship->num_m_rack = 0;
#ifdef _NAMEDSHIPS
    ship->name = nullptr;
    ship->author = nullptr;
#endif

    if (debugShapeParsing)
        warn("parsing shape: %s", ship_shape_str);

    for (str = const_cast<char *>(ship_shape_str.c_str()); (str = strchr(str, '(')) != nullptr;)
    {
        str++;

        if (shape_version == 0)
        {
            if (isdigit(*str))
            {
                shape_version = 0x3100;
                if (verboseShapeParsing)
                    warn("===== Ship shape is in obsolete 3.1 format. =====");
                break;
            }
            else
                shape_version = 0x3200;
        }

        for (i = 0; (keyw[i] = str[i]) != '\0'; i++)
        {
            if (i == sizeof(keyw) - 1)
            {
                keyw[i] = '\0';
                break;
            }
            if (keyw[i] == ':')
            {
                keyw[i + 1] = '\0';
                break;
            }
        }
        if (str[i] != ':')
        {
            if (verboseShapeParsing)
                warn("Missing colon in ship shape: %s", keyw);
            continue;
        }
        for (teststr = &buf[++i]; (buf[i] = str[i]) != '\0'; i++)
        {
            if (buf[i] == ')')
            {
                buf[i++] = '\0';
                break;
            }
        }
        str += i;

        switch (Get_shape_keyword(keyw))
        {

        case 0: /* Keyword is 'shape' */
            while (teststr)
            {
                while (*teststr == ' ')
                    teststr++;
                if (sscanf(teststr, "%d,%d", &in.x, &in.y) != 2)
                {
                    if (verboseShapeParsing)
                        warn("Missing ship shape coordinate in: \"%s\"",
                             teststr);
                    break;
                }
                if (ship->num_points >= MAX_SHIP_PTS)
                {
                    if (verboseShapeParsing)
                        warn("Too many ship shape coordinates");
                }
                else
                {
                    pt[ship->num_points].x = in.x;
                    pt[ship->num_points].y = in.y;
                    ship->num_points++;
                    if (debugShapeParsing)
                        warn("ship point at %d,%d\n", in.x, in.y);
                }
                teststr = strchr(teststr, ' ');
            }
            break;

        case 1: /* Keyword is 'mainGun' */
            if (mainGunSet)
            {
                if (verboseShapeParsing)
                    warn("Ship shape keyword \"%s\" multiple defined", keyw);
                break;
            }
            while (*teststr == ' ')
                teststr++;
            if (sscanf(teststr, "%d,%d", &in.x, &in.y) != 2)
            {
                if (verboseShapeParsing)
                    warn("Missing main gun coordinate in: \"%s\"", teststr);
            }
            else
            {
                m_gun = in;
                mainGunSet = true;
                if (debugShapeParsing)
                    warn("main gun at %d,%d", in.x, in.y);
            }
            break;

        case 2: /* Keyword is 'leftGun' */
            while (teststr)
            {
                while (*teststr == ' ')
                    teststr++;
                if (sscanf(teststr, "%d,%d", &in.x, &in.y) != 2)
                {
                    if (verboseShapeParsing)
                        warn("Missing left gun coordinate in: \"%s\"",
                             teststr);
                    break;
                }
                if (ship->num_l_gun >= MAX_GUN_PTS)
                {
                    if (verboseShapeParsing)
                        warn("Too many left gun coordinates");
                }
                else
                {
                    l_gun[ship->num_l_gun++] = in;
                    if (debugShapeParsing)
                        warn("left gun at %d,%d", in.x, in.y);
                }
                teststr = strchr(teststr, ' ');
            }
            break;

        case 3: /* Keyword is 'rightGun' */
            while (teststr)
            {
                while (*teststr == ' ')
                    teststr++;
                if (sscanf(teststr, "%d,%d", &in.x, &in.y) != 2)
                {
                    if (verboseShapeParsing)
                        warn("Missing right gun coordinate in: \"%s\"",
                             teststr);
                    break;
                }
                if (ship->num_r_gun >= MAX_GUN_PTS)
                {
                    if (verboseShapeParsing)
                        warn("Too many right gun coordinates");
                }
                else
                {
                    r_gun[ship->num_r_gun++] = in;
                    if (debugShapeParsing)
                        warn("right gun at %d,%d", in.x, in.y);
                }
                teststr = strchr(teststr, ' ');
            }
            break;

        case 4: /* Keyword is 'leftLight' */
            while (teststr)
            {
                while (*teststr == ' ')
                    teststr++;
                if (sscanf(teststr, "%d,%d", &in.x, &in.y) != 2)
                {
                    if (verboseShapeParsing)
                        warn("Missing left light coordinate in: \"%s\"",
                             teststr);
                    break;
                }
                if (ship->num_l_light >= MAX_LIGHT_PTS)
                {
                    if (verboseShapeParsing)
                        warn("Too many left light coordinates");
                }
                else
                {
                    l_light[ship->num_l_light++] = in;
                    if (debugShapeParsing)
                        warn("left light at %d,%d", in.x, in.y);
                }
                teststr = strchr(teststr, ' ');
            }
            break;

        case 5: /* Keyword is 'rightLight' */
            while (teststr)
            {
                while (*teststr == ' ')
                    teststr++;
                if (sscanf(teststr, "%d,%d", &in.x, &in.y) != 2)
                {
                    if (verboseShapeParsing)
                        warn("Missing right light coordinate in: \"%s\"",
                             teststr);
                    break;
                }
                if (ship->num_r_light >= MAX_LIGHT_PTS)
                {
                    if (verboseShapeParsing)
                        warn("Too many right light coordinates");
                }
                else
                {
                    r_light[ship->num_r_light++] = in;
                    if (debugShapeParsing)
                        warn("right light at %d,%d", in.x, in.y);
                }
                teststr = strchr(teststr, ' ');
            }
            break;

        case 6: /* Keyword is 'engine' */
            if (engineSet)
            {
                if (verboseShapeParsing)
                    warn("Ship shape keyword \"%s\" multiple defined", keyw);
                break;
            }
            while (*teststr == ' ')
                teststr++;
            if (sscanf(teststr, "%d,%d", &in.x, &in.y) != 2)
            {
                if (verboseShapeParsing)
                    warn("Missing engine coordinate in: \"%s\"", teststr);
            }
            else
            {
                engine = in;
                engineSet = true;
                if (debugShapeParsing)
                    warn("engine at %d,%d", in.x, in.y);
            }
            break;

        case 7: /* Keyword is 'missileRack' */
            while (teststr)
            {
                while (*teststr == ' ')
                    teststr++;
                if (sscanf(teststr, "%d,%d", &in.x, &in.y) != 2)
                {
                    if (verboseShapeParsing)
                    {
                        warn("Missing missile rack coordinate in: \"%s\"",
                             teststr);
                    }
                    break;
                }
                if (ship->num_m_rack >= MAX_RACK_PTS)
                {
                    if (verboseShapeParsing)
                        warn("Too many missile rack coordinates");
                }
                else
                {
                    m_rack[ship->num_m_rack++] = in;
                    if (debugShapeParsing)
                        warn("missile rack at %d,%d", in.x, in.y);
                }
                teststr = strchr(teststr, ' ');
            }
            break;

        case 8: /* Keyword is 'name' */
#ifdef _NAMEDSHIPS
            ship->name = strdup(teststr);
            /* ship->name[strlen(ship->name)-1] = '\0'; */
#endif
            break;

        case 9: /* Keyword is 'author' */
#ifdef _NAMEDSHIPS
            ship->author = strdup(teststr);
            /* ship->author[strlen(ship->author)-1] = '\0'; */
#endif
            break;

        case 10: /* Keyword is 'leftRearGun' */
            while (teststr)
            {
                while (*teststr == ' ')
                    teststr++;
                if (sscanf(teststr, "%d,%d", &in.x, &in.y) != 2)
                {
                    if (verboseShapeParsing)
                        warn("Missing left rear gun coordinate in: \"%s\"",
                             teststr);
                    break;
                }
                if (ship->num_l_rgun >= MAX_GUN_PTS)
                {
                    if (verboseShapeParsing)
                        warn("Too many left rear gun coordinates");
                }
                else
                {
                    l_rgun[ship->num_l_rgun++] = in;
                    if (debugShapeParsing)
                        warn("left rear gun at %d,%d", in.x, in.y);
                }
                teststr = strchr(teststr, ' ');
            }
            break;

        case 11: /* Keyword is 'rightRearGun' */
            while (teststr)
            {
                while (*teststr == ' ')
                    teststr++;
                if (sscanf(teststr, "%d,%d", &in.x, &in.y) != 2)
                {
                    if (verboseShapeParsing)
                        warn("Missing right rear gun coordinate in: \"%s\"",
                             teststr);
                    break;
                }
                if (ship->num_r_rgun >= MAX_GUN_PTS)
                {
                    if (verboseShapeParsing)
                        warn("Too many right rear gun coordinates");
                }
                else
                {
                    r_rgun[ship->num_r_rgun++] = in;
                    if (debugShapeParsing)
                        warn("right rear gun at %d,%d", in.x, in.y);
                }
                teststr = strchr(teststr, ' ');
            }
            break;

        default:
            if (verboseShapeParsing)
                warn("Invalid ship shape keyword: \"%s\"", keyw);
            /* the good thing about this format is that we can just ignore
             * this.  it is likely to be a new extension we don't know
             * about yet. */
            break;
        }
    }

    if (shape_version == 0x3100)
    {
        int ofNum, ofLeft, ofRight; /* old format */

        str = const_cast<char *>(ship_shape_str.c_str());

        if (sscanf(str, "(%d,%d,%d)", &ofNum, &ofLeft, &ofRight) != 3 || ofNum < MIN_SHIP_PTS || ofNum > MAX_SHIP_PTS || ofLeft < 0 || ofLeft >= ofNum || ofRight < 0 || ofRight >= ofNum)
        {
            if (verboseShapeParsing)
            {
                warn("Invalid ship shape header: \"%s\"\n", str);
            }
            return -1;
        }

        for (i = 0; i < ofNum; i++)
        {
            str = strchr(str + 1, '(');
            if (!str)
            {
                if (verboseShapeParsing)
                    warn("Bad ship shape: "
                         "only %d points defined, %d expected\n",
                         i, ofNum);
                return -1;
            }
            if (sscanf(str, "(%d,%d)", &in.x, &in.y) != 2)
            {
                if (verboseShapeParsing)
                    warn("Bad ship shape: format error in point %d", i);
                return -1;
            }
            pt[i] = in;
        }

        ship->num_points = ofNum;

        m_gun = pt[0];
        mainGunSet = true;

        l_light[0] = pt[ofLeft];
        ship->num_l_light = 1;

        r_light[0] = pt[ofRight];
        ship->num_r_light = 1;

        engine.x = (pt[ofLeft].x + pt[ofRight].x) / 2;
        engine.y = (pt[ofLeft].y + pt[ofRight].y) / 2;
        engineSet = true;
    }

    /* Check for some things being set, and give them defaults if not */
    if (ship->num_points < 3)
    {
        if (verboseShapeParsing)
            warn("A shipshape must have at least 3 valid points.");
        return -1;
    }

    /* If no main gun set, put at foremost point */
    if (!mainGunSet)
    {
        max = 0;
        for (i = 1; i < ship->num_points; i++)
        {
            if (pt[i].x > pt[max].x || (pt[i].x == pt[max].x && ABS(pt[i].y) < ABS(pt[max].y)))
                max = i;
        }
        m_gun = pt[max];
        mainGunSet = true;
    }
    /* If no left light set, put at leftmost point */
    if (!ship->num_l_light)
    {
        max = 0;
        for (i = 1; i < ship->num_points; i++)
        {
            if (pt[i].y > pt[max].y || (pt[i].y == pt[max].y && pt[i].x <= pt[max].x))
                max = i;
        }
        l_light[0] = pt[max];
        ship->num_l_light = 1;
    }

    /* If no right light set, put at rightmost point */
    if (!ship->num_r_light)
    {
        max = 0;
        for (i = 1; i < ship->num_points; i++)
        {
            if (pt[i].y < pt[max].y || (pt[i].y == pt[max].y && pt[i].x <= pt[max].x))
                max = i;
        }
        r_light[0] = pt[max];
        ship->num_r_light = 1;
    }

    /* If no engine position, put at rear of ship */
    if (!engineSet)
    {
        max = 0;
        for (i = 1; i < ship->num_points; i++)
        {
            if (pt[i].x < pt[max].x)
                max = i;
        }
        /* this may lay outside of ship. */
        engine.x = pt[max].x;
        engine.y = 0;
        engineSet = true;
    }

    /* If no missile racks, put at main gun position*/
    if (!ship->num_m_rack)
    {
        m_rack[0] = m_gun;
        ship->num_m_rack = 1;
    }

    if (shapeLimits)
    {
        const int isLow = -8, isHi = 8, isLeft = 8, isRight = -8,
                  minLow = 1, minHi = 1, minLeft = 1, minRight = 1,
                  horMax = 15, verMax = 15, horMin = -15, verMin = -15,
                  minCount = 3, minSize = 22 + 16;
        int low = 0, hi = 0, left = 0, right = 0,
            count = 0, change,
            max = 0,
            lowest = 0, highest = 0,
            leftmost = 0, rightmost = 0;
        int invalid = 0;
        const int checkWidthAgainstLongestAxis = 1;

        for (i = 0; i < ship->num_points; i++)
        {
            x = pt[i].x;
            y = pt[i].y;
            change = 0;
            if (y >= isLeft)
            {
                change++, left++;
                if (y > leftmost)
                    leftmost = y;
            }
            if (y <= isRight)
            {
                change++, right++;
                if (y < rightmost)
                    rightmost = y;
            }
            if (x <= isLow)
            {
                change++, low++;
                if (x < lowest)
                    lowest = x;
            }
            if (x >= isHi)
            {
                change++, hi++;
                if (x > highest)
                    highest = x;
            }
            if (change)
                count++;
            if (y > horMax || y < horMin)
                max++;
            if (x > verMax || x < verMin)
                max++;
        }
        if (low < minLow || hi < minHi || left < minLeft || right < minRight || count < minCount)
        {
            if (verboseShapeParsing)
                warn("Ship shape does not meet size requirements "
                     "(%d,%d,%d,%d,%d)",
                     low, hi, left, right, count);

            return -1;
        }
        if (max)
        {
            if (verboseShapeParsing)
                warn("Ship shape exceeds size maxima.");
            return -1;
        }
        if (leftmost - rightmost + highest - lowest < minSize)
        {
            if (verboseShapeParsing)
                warn("Ship shape is not big enough.\n"
                     "The ship's width and height added together should\n"
                     "be at least %d.",
                     minSize);
            return -1;
        }

        if (checkWidthAgainstLongestAxis)
        {
            /*
             * For making sure the ship is the right width!
             */
            int pair[2] = {0, 0};
            int dist = 0, tmpDist = 0;
            double vec[2], width, dTmp;
            // const int minWidth = 12; // TODO: enable
            const int minWidth = 1;

            /*
             * Loop over all the points and find the two furthest apart
             */
            for (i = 0; i < ship->num_points; i++)
            {
                for (j = i + 1; j < ship->num_points; j++)
                {
                    /*
                     * Compare the points if they are not the same ones.
                     * Get this distance -- doesn't matter about sqrting
                     * it since only size is important.
                     */
                    if ((tmpDist = ((pt[i].x - pt[j].x) * (pt[i].x - pt[j].x) +
                                    (pt[i].y - pt[j].y) * (pt[i].y - pt[j].y))) > dist)
                    {
                        /*
                         * Set new separation thingy.
                         */
                        dist = tmpDist;
                        pair[0] = i;
                        pair[1] = j;
                    }
                }
            }

            /*
             * Now we know the vector that is _|_ to the one above
             * is simply found by (x,y) -> (y,-x) => dot-prod = 0
             */
            vec[0] = (double)(pt[pair[1]].y - pt[pair[0]].y);
            vec[1] = (double)(pt[pair[0]].x - pt[pair[1]].x);

            /*
             * Normalise
             */
            dTmp = LENGTH(vec[0], vec[1]);
            vec[0] /= dTmp;
            vec[1] /= dTmp;

            /*
             * Now check the width _|_ to the ship main line.
             */
            for (i = 0, width = dTmp = 0.0; i < ship->num_points; i++)
            {
                for (j = i + 1; j < ship->num_points; j++)
                {
                    /*
                     * Check the line if the points are not the same ones
                     */
                    width = fabs(vec[0] * (double)(pt[i].x - pt[j].x) +
                                 vec[1] * (double)(pt[i].y - pt[j].y));
                    if (width > dTmp)
                        dTmp = width;
                }
            }

            /*
             * And make sure it is nice and far away
             */
            if (((int)dTmp) < minWidth)
            {
                if (verboseShapeParsing)
                    warn("Ship shape is not big enough.\n"
                         "The ship's width should be at least %d.\n"
                         "Player's is %d",
                         minWidth, (int)dTmp);
                return -1;
            }
        }

        /*
         * Check that none of the special points are outside the
         * shape defined by the normal points.
         * First the shape is drawn on a grid.  Then all grid points
         * on the outside of the shape are marked.  Thusly for each
         * special point can be determined if it is outside the shape.
         */
        GRID_RESET();

        /* Draw the ship outline first. */
        for (i = 0; i < ship->num_points; i++)
        {
            j = i + 1;
            if (j == ship->num_points)
                j = 0;

            Grid_set_value(&grid, pt[i].x, pt[i].y, 1);

            dx = pt[j].x - pt[i].x;
            dy = pt[j].y - pt[i].y;
            if (ABS(dx) >= ABS(dy))
            {
                if (dx > 0)
                {
                    for (x = pt[i].x + 1; x < pt[j].x; x++)
                    {
                        y = pt[i].y + (dy * (x - pt[i].x)) / dx;
                        Grid_set_value(&grid, x, y, 1);
                    }
                }
                else
                {
                    for (x = pt[j].x + 1; x < pt[i].x; x++)
                    {
                        y = pt[j].y + (dy * (x - pt[j].x)) / dx;
                        Grid_set_value(&grid, x, y, 1);
                    }
                }
            }
            else
            {
                if (dy > 0)
                {
                    for (y = pt[i].y + 1; y < pt[j].y; y++)
                    {
                        x = pt[i].x + (dx * (y - pt[i].y)) / dy;
                        Grid_set_value(&grid, x, y, 1);
                    }
                }
                else
                {
                    for (y = pt[j].y + 1; y < pt[i].y; y++)
                    {
                        x = pt[j].x + (dx * (y - pt[j].y)) / dy;
                        Grid_set_value(&grid, x, y, 1);
                    }
                }
            }
        }

        /* Check the borders of the grid for blank points. */
        for (y = -15; y <= 15; y++)
        {
            for (x = -15; x <= 15; x += (y == -15 || y == 15) ? 1 : 2 * 15)
            {
                if (Grid_get_value(&grid, x, y) == 0)
                    GRID_ADD(x, y);
            }
        }

        /* Check from the borders of the grid to the centre. */
        while (!GRID_READY())
        {
            GRID_GET(x, y);
            if (x < 15 && Grid_get_value(&grid, x + 1, y) == 0)
                GRID_ADD(x + 1, y);
            if (x > -15 && Grid_get_value(&grid, x - 1, y) == 0)
                GRID_ADD(x - 1, y);
            if (y < 15 && Grid_get_value(&grid, x, y + 1) == 0)
                GRID_ADD(x, y + 1);
            if (y > -15 && Grid_get_value(&grid, x, y - 1) == 0)
                GRID_ADD(x, y - 1);
        }

        /*
         * Note that for the engine, old format shapes may well have the
         * engine position outside the ship, so this check not used for those.
         */

        if (Grid_point_is_outside_ship(&grid, m_gun))
        {
            if (verboseShapeParsing)
                warn("Main gun (at (%d,%d)) is outside ship.",
                     m_gun.x, m_gun.y);
            invalid++;
        }
        for (i = 0; i < ship->num_l_gun; i++)
        {
            if (Grid_point_is_outside_ship(&grid, l_gun[i]))
            {
                if (verboseShapeParsing)
                    warn("Left gun at (%d,%d) is outside ship.",
                         l_gun[i].x, l_gun[i].y);
                invalid++;
            }
        }
        for (i = 0; i < ship->num_r_gun; i++)
        {
            if (Grid_point_is_outside_ship(&grid, r_gun[i]))
            {
                if (verboseShapeParsing)
                    warn("Right gun at (%d,%d) is outside ship.",
                         r_gun[i].x, r_gun[i].y);
                invalid++;
            }
        }
        for (i = 0; i < ship->num_l_rgun; i++)
        {
            if (Grid_point_is_outside_ship(&grid, l_rgun[i]))
            {
                if (verboseShapeParsing)
                    warn("Left rear gun at (%d,%d) is outside ship.",
                         l_rgun[i].x, l_rgun[i].y);
                invalid++;
            }
        }
        for (i = 0; i < ship->num_r_rgun; i++)
        {
            if (Grid_point_is_outside_ship(&grid, r_rgun[i]))
            {
                if (verboseShapeParsing)
                    warn("Right rear gun at (%d,%d) is outside ship.",
                         r_rgun[i].x, r_rgun[i].y);
                invalid++;
            }
        }
        for (i = 0; i < ship->num_m_rack; i++)
        {
            if (Grid_point_is_outside_ship(&grid, m_rack[i]))
            {
                if (verboseShapeParsing)
                    warn("Missile rack at (%d,%d) is outside ship.",
                         m_rack[i].x, m_rack[i].y);
                invalid++;
            }
        }
        for (i = 0; i < ship->num_l_light; i++)
        {
            if (Grid_point_is_outside_ship(&grid, l_light[i]))
            {
                if (verboseShapeParsing)
                    warn("Left light at (%d,%d) is outside ship.",
                         l_light[i].x, l_light[i].y);
                invalid++;
            }
        }
        for (i = 0; i < ship->num_r_light; i++)
        {
            if (Grid_point_is_outside_ship(&grid, r_light[i]))
            {
                if (verboseShapeParsing)
                    warn("Right light at (%d,%d) is outside ship.",
                         r_light[i].x, r_light[i].y);
                invalid++;
            }
        }
        if (Grid_point_is_outside_ship(&grid, engine))
        {
            if (verboseShapeParsing)
                warn("Engine (at (%d,%d)) is outside ship.",
                     engine.x, engine.y);
            invalid++;
            /* this could happen in case of an old format ship shape. */
            if (shape_version == 0x3100 && invalid == 1)
            {
                ipos_t pt;
                /* move engine until it is legal. */
                for (pt.x = -15, pt.y = 0; pt.x <= 15; pt.x++)
                {
                    if (!Grid_point_is_outside_ship(&grid, pt))
                    {
                        engine = pt;
                        invalid--;
                        break;
                    }
                }
            }
        }

        if (debugShapeParsing)
        {
            for (i = -15; i <= 15; i++)
            {
                for (j = -15; j <= 15; j++)
                {
                    switch (Grid_get_value(&grid, j, i))
                    {
                    case 0:
                        putchar(' ');
                        break;
                    case 1:
                        putchar('*');
                        break;
                    case 2:
                        putchar('.');
                        break;
                    default:
                        putchar('?');
                        break;
                    }
                }
                putchar('\n');
            }
        }

        if (invalid > 0)
            return -1;
    }

    i = sizeof(position_t) * ANGLE_RESOLUTION;
    ship->pts[0] = (clpos_t *)malloc(ship->num_points * i);

    for (i = 1; i < ship->num_points; i++)
        ship->pts[i] = &ship->pts[i - 1][ANGLE_RESOLUTION];

    for (i = 0; i < ship->num_points; i++)
        Ship_set_point_ipos(ship, i, pt[i]);

    if (engineSet)
        Ship_set_engine_ipos(ship, engine);

    if (mainGunSet)
        Ship_set_m_gun_ipos(ship, m_gun);

    for (i = 0; i < ship->num_l_gun; i++)
        Ship_set_l_gun_ipos(ship, i, l_gun[i]);

    for (i = 0; i < ship->num_r_gun; i++)
        Ship_set_r_gun_ipos(ship, i, r_gun[i]);

    for (i = 0; i < ship->num_l_rgun; i++)
        Ship_set_l_rgun_ipos(ship, i, l_rgun[i]);

    for (i = 0; i < ship->num_r_rgun; i++)
        Ship_set_r_rgun_ipos(ship, i, r_rgun[i]);

    for (i = 0; i < ship->num_l_light; i++)
        Ship_set_l_light_ipos(ship, i, l_light[i]);

    for (i = 0; i < ship->num_r_light; i++)
        Ship_set_r_light_ipos(ship, i, r_light[i]);

    for (i = 0; i < ship->num_m_rack; i++)
        Ship_set_m_rack_ipos(ship, i, m_rack[i]);

    Rotate_ship(ship);

    return 0;
}

static ShipShape *do_parse_shape(std::string str)
{
    if (str.empty())
    {
        if (debugShapeParsing)
            warn("shape str not set");
        return Default_ship();
    }

    ShipShape *ship = new ShipShape{};
    if (shape2wire(str, ship) != 0)
    {
        delete ship;

        if (debugShapeParsing)
            warn("shape2wire failed");
        return Default_ship();
    }
    if (debugShapeParsing)
        warn("shape2wire succeeded");

    return (ship);
}

void Free_ship_shape(ShipShape *ship)
{
    if (ship != nullptr && ship != Default_ship())
    {
        if (ship->num_points > 0)
            XFREE(ship->pts[0]);
        delete ship;
    }
}

ShipShape *Parse_shape_str(std::string str)
{
    verboseShapeParsing = debugShapeParsing;
    shapeLimits = true;
    return do_parse_shape(str);
}

ShipShape *Convert_shape_str(std::string str)
{
    verboseShapeParsing = debugShapeParsing;
    shapeLimits = debugShapeParsing;
    return do_parse_shape(str);
}

/*
 * Returns 0 if ships is not valid, 1 if valid.
 */
int Validate_shape_str(std::string str)
{
    ShipShape *ship;

    verboseShapeParsing = true;
    shapeLimits = true;
    ship = do_parse_shape(str);
    Free_ship_shape(ship);
    return (ship && ship != Default_ship());
}

void Convert_ship_2_string(ShipShape *ship, char *buf, char *ext,
                           unsigned shape_version)
{
    char tmp[MSG_LEN];
    int i, buflen = 0, extlen, tmplen;

    ext[extlen = 0] = '\0';

    if (shape_version >= 0x3200)
    {
        position_t engine, m_gun;

        strcpy(buf, "(SH:");
        buflen = strlen(&buf[0]);
        std::vector<clpos_t> &points = ship->getPoints(0);
        for (i = 0; i < points.size() && i < MAX_SHIP_PTS; i++)
        {
            // position_t pt = Ship_get_point_position(ship, i, 0);
            position_t pt = clpos2position(points[i]);

            sprintf(&buf[buflen], " %d,%d", (int)pt.x, (int)pt.y);
            buflen += strlen(&buf[buflen]);
        }

        // engine = Ship_get_engine_position(ship, 0);
        engine = clpos2position(ship->getEngineClickPosition(0));
        m_gun = clpos2position(ship->getMainGunClickPosition(0));
        sprintf(&buf[buflen], ")(EN: %d,%d)(MG: %d,%d)",
                (int)engine.x, (int)engine.y,
                (int)m_gun.x, (int)m_gun.y);
        buflen += strlen(&buf[buflen]);

        /*
         * If the calculations are correct then only from here on
         * there is danger for overflowing the MSG_LEN size
         * of the buffer.  Therefore first copy a new pair of
         * parentheses into a temporary buffer and when the closing
         * parenthesis is reached then determine if there is enough
         * room in the main buffer or else copy it into the extended
         * buffer.  This scheme allows cooperation with versions which
         * didn't had the extended buffer yet for which the extended
         * buffer will simply be discarded.
         */
        if (ship->num_l_gun > 0)
        {
            strcpy(&tmp[0], "(LG:");
            tmplen = strlen(&tmp[0]);
            std::vector<clpos_t> l_guns = ship->getLeftGunClickPositions(0);
            for (i = 0; i < l_guns.size() && i < MAX_GUN_PTS; i++)
            {
                position_t l_gun = clpos2position(l_guns[i]);

                sprintf(&tmp[tmplen], " %d,%d",
                        (int)l_gun.x, (int)l_gun.y);
                tmplen += strlen(&tmp[tmplen]);
            }
            strcpy(&tmp[tmplen], ")");
            tmplen++;
            if (buflen + tmplen < MSG_LEN)
            {
                strcpy(&buf[buflen], tmp);
                buflen += tmplen;
            }
            else if (extlen + tmplen < MSG_LEN)
            {
                strcpy(&ext[extlen], tmp);
                extlen += tmplen;
            }
        }
        if (ship->num_r_gun > 0)
        {
            strcpy(&tmp[0], "(RG:");
            tmplen = strlen(&tmp[0]);
            std::vector<clpos_t> r_guns = ship->getRightGunClickPositions(0);
            for (i = 0; i < r_guns.size() && i < MAX_GUN_PTS; i++)
            {
                position_t r_gun = clpos2position(r_guns[i]);

                sprintf(&tmp[tmplen], " %d,%d",
                        (int)r_gun.x, (int)r_gun.y);
                tmplen += strlen(&tmp[tmplen]);
            }
            strcpy(&tmp[tmplen], ")");
            tmplen++;
            if (buflen + tmplen < MSG_LEN)
            {
                strcpy(&buf[buflen], tmp);
                buflen += tmplen;
            }
            else if (extlen + tmplen < MSG_LEN)
            {
                strcpy(&ext[extlen], tmp);
                extlen += tmplen;
            }
        }
        if (ship->num_l_rgun > 0)
        {
            strcpy(&tmp[0], "(LR:");
            tmplen = strlen(&tmp[0]);
            std::vector<clpos_t> l_rguns = ship->getLeftRearGunClickPositions(0);
            for (i = 0; i < l_rguns.size() && i < MAX_GUN_PTS; i++)
            {
                position_t l_rgun = clpos2position(l_rguns[i]);

                sprintf(&tmp[tmplen], " %d,%d",
                        (int)l_rgun.x, (int)l_rgun.y);
                tmplen += strlen(&tmp[tmplen]);
            }
            strcpy(&tmp[tmplen], ")");
            tmplen++;
            if (buflen + tmplen < MSG_LEN)
            {
                strcpy(&buf[buflen], tmp);
                buflen += tmplen;
            }
            else if (extlen + tmplen < MSG_LEN)
            {
                strcpy(&ext[extlen], tmp);
                extlen += tmplen;
            }
        }
        if (ship->num_r_rgun > 0)
        {
            strcpy(&tmp[0], "(RR:");
            tmplen = strlen(&tmp[0]);
            std::vector<clpos_t> r_rguns = ship->getRightRearGunClickPositions(0);
            for (i = 0; i < r_rguns.size() && i < MAX_GUN_PTS; i++)
            {
                position_t r_rgun = clpos2position(r_rguns[i]);

                sprintf(&tmp[tmplen], " %d,%d",
                        (int)r_rgun.x, (int)r_rgun.y);
                tmplen += strlen(&tmp[tmplen]);
            }
            strcpy(&tmp[tmplen], ")");
            tmplen++;
            if (buflen + tmplen < MSG_LEN)
            {
                strcpy(&buf[buflen], tmp);
                buflen += tmplen;
            }
            else if (extlen + tmplen < MSG_LEN)
            {
                strcpy(&ext[extlen], tmp);
                extlen += tmplen;
            }
        }
        if (ship->num_l_light > 0)
        {
            strcpy(&tmp[0], "(LL:");
            tmplen = strlen(&tmp[0]);
            std::vector<clpos_t> l_lights = ship->getLeftLightClickPositions(0);
            for (i = 0; i < l_lights.size() && i < MAX_LIGHT_PTS; i++)
            {
                position_t l_light = clpos2position(l_lights[i]);

                sprintf(&tmp[tmplen], " %d,%d",
                        (int)l_light.x, (int)l_light.y);
                tmplen += strlen(&tmp[tmplen]);
            }
            strcpy(&tmp[tmplen], ")");
            tmplen++;
            if (buflen + tmplen < MSG_LEN)
            {
                strcpy(&buf[buflen], tmp);
                buflen += tmplen;
            }
            else if (extlen + tmplen < MSG_LEN)
            {
                strcpy(&ext[extlen], tmp);
                extlen += tmplen;
            }
        }
        if (ship->num_r_light > 0)
        {
            strcpy(&tmp[0], "(RL:");
            tmplen = strlen(&tmp[0]);
            std::vector<clpos_t> r_lights = ship->getRightLightClickPositions(0);
            for (i = 0; i < r_lights.size() && i < MAX_LIGHT_PTS; i++)
            {
                position_t r_light = clpos2position(r_lights[i]);

                sprintf(&tmp[tmplen], " %d,%d",
                        (int)r_light.x, (int)r_light.y);
                tmplen += strlen(&tmp[tmplen]);
            }
            strcpy(&tmp[tmplen], ")");
            tmplen++;
            if (buflen + tmplen < MSG_LEN)
            {
                strcpy(&buf[buflen], tmp);
                buflen += tmplen;
            }
            else if (extlen + tmplen < MSG_LEN)
            {
                strcpy(&ext[extlen], tmp);
                extlen += tmplen;
            }
        }
        if (ship->num_m_rack > 0)
        {
            strcpy(&tmp[0], "(MR:");
            tmplen = strlen(&tmp[0]);
            std::vector<clpos_t> m_racks = ship->getMissileRackClickPositions(0);
            for (i = 0; i < m_racks.size() && i < MAX_RACK_PTS; i++)
            {
                position_t m_rack = clpos2position(m_racks[i]);

                sprintf(&tmp[tmplen], " %d,%d",
                        (int)m_rack.x, (int)m_rack.y);
                tmplen += strlen(&tmp[tmplen]);
            }
            strcpy(&tmp[tmplen], ")");
            tmplen++;
            if (buflen + tmplen < MSG_LEN)
            {
                strcpy(&buf[buflen], tmp);
                buflen += tmplen;
            }
            else if (extlen + tmplen < MSG_LEN)
            {
                strcpy(&ext[extlen], tmp);
                extlen += tmplen;
            }
        }
    }
    else
    {
        int ll, rl;

        /* 3.1 version had 16 points maximum.  just ignore the excess. */
        int num_points = MIN(ship->num_points, 16);
#if 0
        if (num_points < ship->num_points) {
            printf("Truncating ship to 16 points for old 3.1 server\n");
        }
#endif
        if (shape_version != 0x3100)
        {
            warn("Unknown ship shape version: %x", shape_version);
        }

        for (i = 1, ll = rl = 0; i < num_points; i++)
        {
            if (ship->pts[i][0].cy > ship->pts[ll][0].cy || (ship->pts[i][0].cy == ship->pts[ll][0].cy && ship->pts[i][0].cx < ship->pts[ll][0].cx))
                ll = i;

            if (ship->pts[i][0].cy < ship->pts[rl][0].cy || (ship->pts[i][0].cy == ship->pts[rl][0].cy && ship->pts[i][0].cx < ship->pts[rl][0].cx))
                rl = i;
        }
        sprintf(buf, "(%d,%d,%d)", num_points, ll, rl);
        buflen = strlen(buf);
        for (i = 0; i < num_points; i++)
        {
            sprintf(&buf[buflen], "(%d,%d)",
                    (int)ship->pts[i][0].cx, (int)ship->pts[i][0].cy);
            buflen += strlen(&buf[buflen]);
        }
    }
    if (buflen >= MSG_LEN || extlen >= MSG_LEN)
        warn("BUG: convert ship: buffer overflow (%d,%d)", buflen, extlen);

    if (debugShapeParsing)
        warn("ship 2 str: %s %s", buf, ext);
}

static int Get_shape_keyword(char *keyw)
{
#define NUM_SHAPE_KEYS 12

    static char shape_keys[NUM_SHAPE_KEYS][16] = {
        "shape:",
        "mainGun:",
        "leftGun:",
        "rightGun:",
        "leftLight:",
        "rightLight:",
        "engine:",
        "missileRack:",
        "name:",
        "author:",
        "leftRearGun:",
        "rightRearGun:",
    };
    static char abbrev_keys[NUM_SHAPE_KEYS][4] = {
        "SH:",
        "MG:",
        "LG:",
        "RG:",
        "LL:",
        "RL:",
        "EN:",
        "MR:",
        "NM:",
        "AU:",
        "LR:",
        "RR:",
    };
    int i;

    /* non-abbreviated keywords start with a lower case letter. */
    if (islower(*keyw))
    {
        for (i = 0; i < NUM_SHAPE_KEYS; i++)
        {
            if (!strcmp(keyw, shape_keys[i]))
                break;
        }
    }
    /* abbreviated keywords start with an upper case letter. */
    else if (isupper(*keyw))
    {
        for (i = 0; i < NUM_SHAPE_KEYS; i++)
        {
            if (!strcmp(keyw, abbrev_keys[i]))
                break;
        }
    }
    /* dunno what this is. */
    else
        i = -1;

    return (i);
}

int Calculate_shield_radius(ShipShape *ship)
{
    int i;
    int radius2, max_radius = 0;

    std::vector<clpos_t> &points = ship->getPoints(0);

    for (auto &pos : points)
    {
        position_t pti = clpos2position(pos);
        radius2 = (int)(sqr(pti.x) + sqr(pti.y));
        if (radius2 > max_radius)
            max_radius = radius2;
    }
    max_radius = (int)(2.0 * sqrt((double)max_radius));
    int shield_radius = (max_radius + 2 <= 34)
                            ? 34
                            : (max_radius + 2 - (max_radius & 1));
    return shield_radius;
}
