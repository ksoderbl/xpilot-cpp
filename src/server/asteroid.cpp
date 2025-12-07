/*
 * XPilot, a multiplayer gravity war game.  Copyright (C) 1991-2001 by
 *
 *      Bjørn Stabell
 *      Ken Ronny Schouten
 *      Bert Gijsbers
 *      Dick Balaska
 *      Kimiko Koopman
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

// #include <iostream>

#include <algorithm>
#include <vector>

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <climits>

#include "server.h"

#define SERVER

#include "bit.h"
#include "click.h"
#include "const.h"
#include "keys.h"
#include "xpconfig.h"
#include "xpmath.h"

#include "asteroid.h"
#include "map.h"
#include "object.h"
#include "saudio.h"
#include "walls.h"

// #define SERVER
// #include "xpconfig.h"
// #include "serverconst.h"
// #include "list.h"
//
// #include "proto.h"
// #include "saudio.h"
// #include "bit.h"
// #include "objpos.h"
// #include "asteroid.h"
// #include "xpmath.h"

/* list containing pointers to all asteroids */
std::vector<wireobject_t *> Asteroid_vector;

/*
** Prototypes.
*/
static void Make_asteroid(clpos_t pos,
                          int size, int dir,
                          double speed);

/*
** Return the asteroid list.
*/
std::vector<wireobject_t *> &Asteroid_get_list(void)
{
    return Asteroid_vector;
}

static bool Asteroid_add_to_list(wireobject_t *ast)
{
    // TODO: handle errors/exceptions
    Asteroid_vector.push_back(ast);
    return true;
}

static bool Asteroid_remove_from_list(wireobject_t *ast)
{
    bool result = false;
    const auto it = std::find(Asteroid_vector.begin(), Asteroid_vector.end(), ast);
    if (it == Asteroid_vector.end())
    {
        return false;
    }
    Asteroid_vector.erase(it);
    return true;
}

/*
 * Breaks up an asteroid:
 * Asteroids of size > 1 break up into two asteroids of size n - 1
 * and some wreckage and debris.
 * Asteroids of size 1 break up into wreckage and debris only.
 * In both cases the sum of the masses of the smaller asteroids,
 * the wreckage and the debris should be about equal to the mass
 * of the original asteroid.
 */
void Break_asteroid(wireobject_t *asteroid)
{
    double mass, mass3;
    double speed, speed1, speed2, radius;
    int dir, dir1, dir2, split_dir;
    int x1, y1, x2, y2;

    if (asteroid->wire_size == 1)
    {
        mass = asteroid->mass / 2;
        Make_wreckage(asteroid->pos,
                      asteroid->vel,
                      NO_ID,
                      TEAM_NOT_SET,
                      mass / 20, mass / 3,
                      mass,
                      GRAVITY,
                      10,
                      0, RES - 1,
                      5.0, 10.0,
                      3.0, 10.0);
        Make_debris(asteroid->pos,
                    asteroid->vel,
                    NO_ID,
                    TEAM_NOT_SET,
                    OBJ_DEBRIS,
                    mass,
                    GRAVITY,
                    RED,
                    8,
                    20, 50,
                    (int)(20 + 30 * rfrac()),
                    0, RES - 1,
                    5.0, 10.0,
                    3.0, 10.0);
    }
    else
    {
        /* foo[12] refer to the mini-asteroids
           foo3 refers to the wreckage and debris */
        speed = VECTOR_LENGTH(asteroid->vel);
        dir = (int)findDir(asteroid->vel.x, asteroid->vel.y);
        mass3 = asteroid->mass * ASTEROID_DUST_FACT;
        mass = ASTEROID_MASS(asteroid->wire_size - 1);
        dir1 = MOD2((int)(dir - ASTEROID_DELTA_DIR / 4 - (rfrac() * ASTEROID_DELTA_DIR / 4)), RES);
        dir2 = MOD2((int)(dir + ASTEROID_DELTA_DIR / 4 + (rfrac() * ASTEROID_DELTA_DIR / 4)), RES);
        speed1 = (speed * (1 - ASTEROID_DUST_FACT)) / tcos(ABS(dir - dir1));
        speed2 = (speed * (1 - ASTEROID_DUST_FACT)) / tcos(ABS(dir2 - dir));
        /* velx1 = tcos(dir1) * speed1;
        vely1 = tsin(dir1) * speed1;
        velx2 = tcos(dir2) * speed2;
        vely2 = tsin(dir2) * speed2; */
        split_dir = MOD2(dir - RES / 4, RES);
        radius = ASTEROID_RADIUS(asteroid->wire_size - 1);
        x1 = WRAP_XPIXEL(asteroid->pix_pos.x + tcos(split_dir) * radius);
        y1 = WRAP_YPIXEL(asteroid->pix_pos.y + tsin(split_dir) * radius);
        x2 = WRAP_XPIXEL(asteroid->pix_pos.x - tcos(split_dir) * radius);
        y2 = WRAP_YPIXEL(asteroid->pix_pos.y - tsin(split_dir) * radius);
        clpos_t pos1, pos2;
        pos1.cx = PIXEL_TO_CLICK(x1);
        pos1.cy = PIXEL_TO_CLICK(y1);
        pos2.cx = PIXEL_TO_CLICK(x2);
        pos2.cy = PIXEL_TO_CLICK(y2);
        Make_asteroid(pos1, asteroid->wire_size - 1, dir1, speed1);
        Make_asteroid(pos2, asteroid->wire_size - 1, dir2, speed2);
        Make_wreckage(asteroid->pos,
                      asteroid->vel,
                      NO_ID,
                      TEAM_NOT_SET,
                      mass3 / 20, mass3 / 3,
                      mass3 / 2,
                      GRAVITY,
                      10,
                      0, RES - 1,
                      5.0, 10.0,
                      3.0, 10.0);
        Make_debris(asteroid->pos,
                    asteroid->vel,
                    NO_ID,
                    TEAM_NOT_SET,
                    OBJ_DEBRIS,
                    mass3 / 2,
                    GRAVITY,
                    RED,
                    8,
                    20, 50,
                    (int)(20 + 30 * rfrac()),
                    0, RES - 1,
                    5.0, 10.0,
                    3.0, 10.0);
    }

    if ((options.asteroidMaxItems > 0) && (rfrac() < options.asteroidItemProb))
    {
        int nitems = (int)(rfrac() * options.asteroidMaxItems) + 1;
        int i;
        int vx, vy;
        int item, item_dir, num_per_pack;
        double item_speed;
        long status;

        for (i = 0; i < nitems; i++)
        {
            vector_t vel;

            item = Choose_random_item();
            item_dir = (int)(rfrac() * RES);
            item_speed = rfrac() * 10;
            vel.x = asteroid->vel.x + item_speed * tcos(item_dir);
            vel.y = asteroid->vel.y + item_speed * tsin(item_dir);
            status = GRAVITY;
            if (rfrac() < options.randomItemProb)
                status |= RANDOM_ITEM;
            if (world->items[item].min_per_pack == world->items[item].max_per_pack)
                num_per_pack = world->items[item].max_per_pack;
            else
                num_per_pack = world->items[item].min_per_pack + (int)(rfrac() * (1 + world->items[item].max_per_pack - world->items[item].min_per_pack));

            Make_item(asteroid->pos, vel,
                      item, num_per_pack,
                      status);
        }
    }

    sound_play_sensors(asteroid->pos, ASTEROID_BREAK_SOUND);

    world->asteroids.num -= 1 << (asteroid->wire_size - 1);

    Asteroid_remove_from_list(asteroid);
}

/*
 * Creates an asteroid with the given characteristics.
 */
static void Make_asteroid(clpos_t pos, int size, int dir, double speed)
{
    wireobject_t *asteroid;
    double radius;
    int bx;
    int by;

    if (NumObjs >= MAX_TOTAL_SHOTS)
        return;

    if (size < 1 || size > ASTEROID_MAX_SIZE)
        return;

    if (BIT(world->rules->mode, WRAP_PLAY))
        pos = World_wrap_clpos(pos);

    if (!World_contains_clpos(pos))
        return;

    bx = CLICK_TO_BLOCK(pos.cx);
    by = CLICK_TO_BLOCK(pos.cy);
    if (BIT(world->block[bx][by], FILLED_BIT | FUEL_BIT | TARGET_BIT | TREASURE_BIT))
    {
        return;
    }
    else if (BIT(world->block[bx][by], REC_LU | REC_RU | REC_LD | REC_RD))
    {
        double cx_in_b = pos.cx - bx * BLOCK_CLICKS,
               cy_in_b = pos.cy - by * BLOCK_CLICKS;
        switch (world->block[bx][by])
        {
        case REC_LU:
            if (cx_in_b < cy_in_b)
                return;
            break;
        case REC_RU:
            if (cx_in_b + cy_in_b > BLOCK_CLICKS)
                return;
            break;
        case REC_LD:
            if (cx_in_b + cy_in_b < BLOCK_CLICKS)
                return;
            break;
        case REC_RD:
            if (cx_in_b > cy_in_b)
                return;
            break;
        }
    }

    asteroid = WIRE_PTR(Object_allocate());
    if (asteroid == NULL)
        return;

    asteroid->color = WHITE;
    asteroid->id = NO_ID;
    asteroid->team = TEAM_NOT_SET;
    asteroid->type = OBJ_ASTEROID;

    /* Position */
    Object_position_init_clpos(OBJ_PTR(asteroid), pos);

    asteroid->vel.x = tcos(dir) * speed;
    asteroid->vel.y = tsin(dir) * speed;
    asteroid->acc.x = asteroid->acc.y = 0;
    asteroid->mass = ASTEROID_MASS(size);
    asteroid->life = ASTEROID_LIFE;
    asteroid->wire_turnspeed = 0.02 + rfrac() * 0.05;
    asteroid->wire_rotation = (int)(rfrac() * RES);
    asteroid->wire_size = size;
    asteroid->wire_type = (uint8_t)(rfrac() * 256);
    radius = ASTEROID_RADIUS(size);
    asteroid->pl_range = radius;
    asteroid->pl_radius = radius;
    asteroid->fuselife = asteroid->life - 1;
    asteroid->obj_status = GRAVITY;
    CLEAR_MODS(asteroid->mods);

    if (Asteroid_add_to_list(asteroid))
    {
        world->asteroids.num += 1 << (size - 1);
        Cell_add_object(OBJ_PTR(asteroid));
    }
    else
        Object_free_ptr(OBJ_PTR(asteroid));
}

/*
 * Tries to place a new asteroid on the map.
 * Calls Make_asteroid() to actually create the new asteroid
 */
static void Place_asteroid(void)
{
    int place_count, dir, dist, i;
    int bx, by;
    unsigned space;
    bool okay = false;
    asteroid_concentrator_t *con;
    clpos_t pos;

    space = SPACE_BLOCKS;
    space &= ~(BASE_BIT | WORMHOLE_BIT);
    space |= FRICTION_BIT;
    /* would be dubious: space |= CANNON_BIT; */

    if (Num_asteroidConcs() > 0 && rfrac() < options.asteroidConcentratorProb)
        con = AsteroidConc_by_index((int)(rfrac() * Num_asteroidConcs()));
    else
        con = NULL;

    /*
     * We bail out after some unsuccessful attempts to avoid wasting
     * too much time on crowded maps.
     */
    okay = false;
    for (place_count = 0; !okay; place_count++)
    {
        if (place_count >= 10)
            return;

        if (con)
        {
            dir = (int)(rfrac() * RES);
            dist = (int)(rfrac() * ((options.asteroidConcentratorRadius * BLOCK_SZ) + 1));
            pos.cx = (int)(con->pos.cx + dist * tcos(dir) * PIXEL_CLICKS);
            pos.cy = (int)(con->pos.cy + dist * tsin(dir) * PIXEL_CLICKS);

            if (BIT(world->rules->mode, WRAP_PLAY))
                pos = World_wrap_clpos(pos);

            if (!World_contains_clpos(pos))
                continue;
        }
        else
            pos = World_get_random_clpos();

        bx = CLICK_TO_BLOCK(pos.cx);
        by = CLICK_TO_BLOCK(pos.cy);

        if (BIT(1U << world->block[bx][by], space))
        {
            int i, dcx, dcy, ocx, ocy;

            okay = true;

            for (i = 0; i < NumPlayers; i++)
            {
                player_t *pl = Player_by_index(i);

                if (Player_is_human(pl))
                {
                    ocx = OBJ_X_IN_CLICKS(pl);
                    ocy = OBJ_Y_IN_CLICKS(pl);
                    dcx = WRAP_XCLICK(pos.cx - ocx);
                    dcy = WRAP_YCLICK(pos.cy - ocy);
                    int dpx = CLICK_TO_PIXEL(dcx);
                    int dpy = CLICK_TO_PIXEL(dcy);
                    if (sqr(dpx) + sqr(dpy) < sqr(ASTEROID_MIN_DIST))
                    {
                        /* too close to player */
                        okay = false;
                        break;
                    }
                }
            }
        }
    }
    if (okay)
        Make_asteroid(pos,
                      (int)(1 + rfrac() * ASTEROID_MAX_SIZE),
                      (int)(rfrac() * RES),
                      (double)ASTEROID_START_SPEED);
}

static void Asteroid_move(wireobject_t *wireobj)
{
    Move_object(OBJ_PTR(wireobj));
}

static void Asteroid_rotate(wireobject_t *wireobj)
{
    wireobj->wire_rotation =
        (wireobj->wire_rotation + (int)(wireobj->wire_turnspeed * timeStep * RES)) % RES;
}

/*
 * Called once each frame update to update everything
 * related to asteroids including creation, destruction,
 * rotation and movement.
 */
void Asteroid_update(void)
{
    int num;
    wireobject_t *asteroid;

    {
        /*
         * if there are more asteroids than are wanted, mark
         * all asteroids to be removed (by Delete_shot()),
         * until enough of size 1 have been removed
         * (only breaking of size 1 actually lowers the
         * total number of asteroids)
         * one iteration may not remove enough asteroids
         * the rest are left until the next frame then
         */
        num = world->asteroids.num;
        // printf("Asteroid_update, number of asteroids: %d\n", num);
        if (num > world->asteroids.max)
        {
            for (wireobject_t *asteroid : Asteroid_vector)
            {
                if (asteroid->life > 0)
                {
                    asteroid->life = 0;
                    if (asteroid->wire_size == 1)
                        num--;
                }
                if (num <= world->asteroids.max)
                    break;
            }
        }

        /* rotate asteroids */
        for (wireobject_t *asteroid : Asteroid_vector)
        {
            if (asteroid->life > 0)
                Asteroid_rotate(asteroid);
        }

        /* move asteroids */
        for (wireobject_t *asteroid : Asteroid_vector)
        {
            if (asteroid->life > 0)
                Asteroid_move(asteroid);
        }
    }

    /* place new asteroid if room left */
    if (world->asteroids.chance > 0)
    {
        int incr = (1 << (ASTEROID_MAX_SIZE - 1));
        if (world->asteroids.num + incr < world->asteroids.max)
        {
            if ((rfrac() * world->asteroids.chance) < 1.0)
                Place_asteroid();
        }
    }
}

/* assume ASTEROID_MAX_SIZE == 4 */
shape_t asteroid_wire1;
shape_t asteroid_wire2;
shape_t asteroid_wire3;
shape_t asteroid_wire4;

void Asteroid_line_init(void)
{
    int i;
    static clpos_t coords1[MAX_SHIP_PTS];
    static clpos_t coords2[MAX_SHIP_PTS];
    static clpos_t coords3[MAX_SHIP_PTS];
    static clpos_t coords4[MAX_SHIP_PTS];

    asteroid_wire1.num_points = MAX_SHIP_PTS;
    for (i = 0; i < MAX_SHIP_PTS; i++)
    {
        asteroid_wire1.pts[i] = coords1 + i;
        coords1[i].cx = (click_t)(cos(i * 2 * PI / MAX_SHIP_PTS) * ASTEROID_RADIUS(1));
        coords1[i].cy = (click_t)(sin(i * 2 * PI / MAX_SHIP_PTS) * ASTEROID_RADIUS(1));
    }

    asteroid_wire2.num_points = MAX_SHIP_PTS;
    for (i = 0; i < MAX_SHIP_PTS; i++)
    {
        asteroid_wire2.pts[i] = coords2 + i;
        coords2[i].cx = (click_t)(cos(i * 2 * PI / MAX_SHIP_PTS) * ASTEROID_RADIUS(2));
        coords2[i].cy = (click_t)(sin(i * 2 * PI / MAX_SHIP_PTS) * ASTEROID_RADIUS(2));
    }

    asteroid_wire3.num_points = MAX_SHIP_PTS;
    for (i = 0; i < MAX_SHIP_PTS; i++)
    {
        asteroid_wire3.pts[i] = coords3 + i;
        coords3[i].cx = (click_t)(cos(i * 2 * PI / MAX_SHIP_PTS) * ASTEROID_RADIUS(3));
        coords3[i].cy = (click_t)(sin(i * 2 * PI / MAX_SHIP_PTS) * ASTEROID_RADIUS(3));
    }

    asteroid_wire4.num_points = MAX_SHIP_PTS;
    for (i = 0; i < MAX_SHIP_PTS; i++)
    {
        asteroid_wire4.pts[i] = coords4 + i;
        coords4[i].cx = (click_t)(cos(i * 2 * PI / MAX_SHIP_PTS) * ASTEROID_RADIUS(4));
        coords4[i].cy = (click_t)(sin(i * 2 * PI / MAX_SHIP_PTS) * ASTEROID_RADIUS(4));
    }

    return;
}
