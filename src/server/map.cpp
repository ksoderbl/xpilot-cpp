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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#include "commonmacros.h"
#include "strlcpy.h"

#include "cannon.h"
#include "server.h"

#define SERVER
#include "xpconfig.h"
#include "serverconst.h"

#include "map.h"
#include "bit.h"
#include "xperror.h"
#include "xpmath.h"

#define GRAV_RANGE 10

/*
 * Globals.
 */
world_t World, *world = &World;
bool is_polygon_map = false;

static void Init_map(void);
static void Alloc_map(void);
static void Generate_random_map(void);

static void Find_base_order(void);

#ifdef DEBUG
static void Print_map(void) /* Debugging only. */
{
    int x, y;

    for (y = world->y - 1; y >= 0; y--)
    {
        for (x = 0; x < world->x; x++)
            switch (world->block[x][y])
            {
            case SPACE:
                putchar(' ');
                break;
            case BASE:
                putchar('_');
                break;
            default:
                putchar('X');
                break;
            }
        putchar('\n');
    }
}
#endif

int World_place_cannon(clpos_t pos, int dir, int team)
{
    // cannon_t t, *cannon;
    // int ind = Num_cannons(), i;

    // t.pos = pos;
    // t.dir = dir;
    // t.team = team;
    // t.dead_ticks = 0;
    // t.conn_mask = ~0;
    // t.group = NO_GROUP;
    // t.score = CANNON_SCORE;
    // t.id = ind + MIN_CANNON_ID;
    // assert(Is_cannon_id(t.id));
    // if (t.id > MAX_CANNON_ID)
    // {
    //     warn("The server supports only %d cannons per map.", NUM_CANNON_IDS);
    //     exit(1);
    // }
    // for (i = 0; i < NUM_ITEMS; i++)
    //     t.initial_items[i] = -1;
    // t.shot_speed = -1;
    // t.smartness = -1;
    // Arraylist_add(world->cannons, &t);
    // cannon = Cannon_by_index(ind);
    // assert(Cannon_by_id(t.id) == cannon);

    // return ind;

    // TODO
    return -1;
}

int World_place_fuel(clpos_t pos, int team)
{
    // fuel_t t;
    // int ind = Num_fuels();

    // t.pos = pos;
    // t.fuel = START_STATION_FUEL;
    // t.conn_mask = ~0;
    // t.last_change = frame_loops;
    // t.team = team;
    // Arraylist_add(world->fuels, &t);

    // return ind;

    // TODO
    return -1;
}

int World_place_base(clpos_t pos, int dir, int team, int order)
{
    // base_t t;
    // int ind = world->NumBases, i;

    // t.pos = pos;
    // t.order = order;
    // /*
    //  * The direction of the base should be so that it points
    //  * up with respect to the gravity in the region.  This
    //  * is fixed in Find_base_direction() when the gravity has
    //  * been computed.
    //  */
    // if (dir < 0 || dir >= RES)
    // {
    //     warn("Base with direction %d in map.", dir);
    //     warn("Valid base directions are from 0 to %d.", RES - 1);
    //     while (dir < 0)
    //         dir += RES;
    //     while (dir >= RES)
    //         dir -= RES;
    //     warn("Using direction %d for this base.", dir);
    // }

    // t.dir = dir;
    // if (BIT(world->rules->mode, TEAM_PLAY))
    // {
    //     if (team < 0 || team >= MAX_TEAMS)
    //         team = 0;
    //     t.team = team;
    //     world->teams[team].NumBases++;
    //     if (world->teams[team].NumBases == 1)
    //         world->NumTeamBases++;
    // }
    // else
    //     t.team = TEAM_NOT_SET;
    // t.ind = world->NumBases;

    // for (i = 0; i < NUM_ITEMS; i++)
    //     t.initial_items[i] = -1;
    // Arraylist_add(world->bases, &t);

    // return ind;

    // TODO
    return -1;
}

int World_place_treasure(clpos_t pos, int team, bool empty,
                         int ball_style)
{
    // treasure_t t;
    // int ind = Num_treasures();

    // t.pos = pos;
    // t.have = false;
    // t.destroyed = 0;
    // t.team = team;
    // t.empty = empty;
    // t.ball_style = ball_style;
    // if (team != TEAM_NOT_SET)
    // {
    //     world->teams[team].NumTreasures++;
    //     world->teams[team].TreasuresLeft++;
    // }
    // Arraylist_add(world->treasures, &t);

    // return ind;

    // TODO
    return -1;
}

int World_place_target(clpos_t pos, int team)
{
    // target_t t;
    // int ind = Num_targets();

    // t.pos = pos;
    // /*
    //  * If we have a block based map, the team is determined in
    //  * in Xpmap_find_map_object_teams().
    //  */
    // t.team = team;
    // t.dead_ticks = 0;
    // t.damage = TARGET_DAMAGE;
    // t.conn_mask = ~0;
    // t.update_mask = 0;
    // t.last_change = frame_loops;
    // t.group = NO_GROUP;
    // Arraylist_add(world->targets, &t);

    // return ind;

    // TODO
    return -1;
}

int World_place_wormhole(clpos_t pos, wormtype_t type)
{
    // wormhole_t t;
    // int ind = Num_wormholes();

    // t.pos = pos;
    // t.countdown = 0;
    // t.lastdest = NO_IND;
    // t.type = type;
    // t.lastblock = SPACE;
    // t.lastID = NO_ID;
    // t.group = NO_GROUP;
    // Arraylist_add(world->wormholes, &t);

    // return ind;

    // TODO
    return -1;
}

/*
 * Allocate checkpoints for an xp map.
 */
static void alloc_old_checks(void)
{
    // int i;
    // check_t t;
    // clpos_t pos = {-1, -1};

    // t.pos = pos;

    // for (i = 0; i < OLD_MAX_CHECKS; i++)
    //     STORE(check_t, world->checks, world->NumChecks, world->MaxChecks, t);

    // SHRINK(check_t, world->checks, world->NumChecks, world->MaxChecks);
    // world->NumChecks = 0;
}

int World_place_check(clpos_t pos, int ind)
{
    // check_t t;

    // if (!BIT(world->rules->mode, TIMING))
    // {
    //     warn("Checkpoint on map with no timing.");
    //     return NO_IND;
    // }

    // /* kps - need to do this for other map object types ? */
    // if (!World_contains_clpos(pos))
    // {
    //     warn("Checkpoint outside world, ignoring.");
    //     return NO_IND;
    // }

    // /*
    //  * On xp maps we can have only 26 checkpoints.
    //  */
    // if (ind >= 0 && ind < OLD_MAX_CHECKS)
    // {
    //     check_t *check;

    //     if (world->NumChecks == 0)
    //         alloc_old_checks();

    //     /*
    //      * kps hack - we can't use Check_by_index because it might return
    //      * NULL since ind can here be >= world->NumChecks.
    //      */
    //     check = &world->checks[ind];
    //     if (World_contains_clpos(check->pos))
    //     {
    //         warn("Map contains too many '%c' checkpoints.", 'A' + ind);
    //         return NO_IND;
    //     }

    //     check->pos = pos;
    //     world->NumChecks++;
    //     return ind;
    // }

    // ind = world->NumChecks;
    // t.pos = pos;
    // STORE(check_t, world->checks, world->NumChecks, world->MaxChecks, t);
    // return ind;

    // TODO
    return -1;
}

int World_place_item_concentrator(clpos_t pos)
{
    // item_concentrator_t t;
    // int ind = Num_itemConcs();

    // t.pos = pos;
    // Arraylist_add(world->itemConcs, &t);

    // return ind;

    // TODO
    return -1;
}

int World_place_asteroid_concentrator(clpos_t pos)
{
    // asteroid_concentrator_t t;
    // int ind = Num_asteroidConcs();

    // t.pos = pos;
    // Arraylist_add(world->asteroidConcs, &t);

    // return ind;

    // TODO
    return -1;
}

int World_place_grav(clpos_t pos, double force, int type)
{
    // grav_t t;
    // int ind = Num_gravs();

    // t.pos = pos;
    // t.force = force;
    // t.type = type;
    // Arraylist_add(world->gravs, &t);

    // return ind;

    // TODO
    return -1;
}

int World_place_friction_area(clpos_t pos, double fric)
{
    // friction_area_t t;
    // int ind = Num_frictionAreas();

    // t.pos = pos;
    // t.friction_setting = fric;
    // /*t.friction = ... ; handled in timing setup */
    // Arraylist_add(world->frictionAreas, &t);

    // return ind;

    // TODO
    return -1;
}

shape_t filled_wire;
clpos_t filled_coords[4];

static void Filled_wire_init(void)
{
    int i, h;

    filled_wire.num_points = 4;

    for (i = 0; i < 4; i++)
        filled_wire.pts[i] = &filled_coords[i];

    h = BLOCK_CLICKS / 2;

    /* whole (filled) block */
    filled_coords[0].cx = -h;
    filled_coords[0].cy = -h;
    filled_coords[1].cx = h - 1;
    filled_coords[1].cy = -h;
    filled_coords[2].cx = h - 1;
    filled_coords[2].cy = h - 1;
    filled_coords[3].cx = -h;
    filled_coords[3].cy = h - 1;
}

static void Init_map(void)
{
    world->x = 256;
    world->y = 256;
    world->diagonal = (int)LENGTH(world->x, world->y);

    world->width = world->x * BLOCK_SZ;
    world->height = world->y * BLOCK_SZ;
    world->hypotenuse = (int)LENGTH(world->width, world->height);

    world->cwidth = PIXEL_TO_CLICK(world->width);
    world->cheight = PIXEL_TO_CLICK(world->height);
    world->click_hypotenuse = LENGTH(world->cwidth, world->cheight);

    world->NumAsteroidConcs = 0;
    world->NumBases = 0;
    world->NumCannons = 0;
    world->NumEcms = 0;
    world->NumFuels = 0;
    world->NumFrictionAreas = 0;
    world->NumGravs = 0;
    world->NumItemConcentrators = 0;
    world->NumTargets = 0;
    world->NumTransporters = 0;
    world->NumTreasures = 0;
    world->NumWormholes = 0;
}

void World_free(void)
{
    XFREE(world->block);
    XFREE(world->itemID);
    XFREE(world->gravity);

    XFREE(world->asteroidConcs);
    XFREE(world->bases);
    XFREE(world->cannons);
    XFREE(world->ecms);
    XFREE(world->fuels);
    XFREE(world->frictionAreas);
    XFREE(world->gravs);
    XFREE(world->itemConcentrators);
    XFREE(world->targets);
    XFREE(world->transporters);
    XFREE(world->treasures);
    XFREE(world->wormholes);
}

static void Alloc_map(void)
{
    int x;

    if (world->block || world->gravity)
        World_free();

    world->block =
        (uint8_t **)malloc(sizeof(uint8_t *) * world->x + world->x * sizeof(uint8_t) * world->y);
    world->itemID =
        (uint16_t **)malloc(sizeof(uint16_t *) * world->x + world->x * sizeof(uint16_t) * world->y);
    world->gravity =
        (vector_t **)malloc(sizeof(vector_t *) * world->x + world->x * sizeof(vector_t) * world->y);
    world->gravs = NULL;
    world->bases = NULL;
    world->fuels = NULL;
    world->cannons = NULL;
    world->wormholes = NULL;
    world->itemConcentrators = NULL;
    world->asteroidConcs = NULL;
    world->ecms = NULL;
    world->frictionAreas = NULL;
    world->transporters = NULL;
    if (world->block == NULL || world->itemID == NULL || world->gravity == NULL)
    {
        World_free();
        error("Couldn't allocate memory for map (%d bytes)",
              world->x * (world->y * (sizeof(uint8_t) + sizeof(vector_t)) + sizeof(vector_t *) + sizeof(uint8_t *)));
        exit(-1);
    }
    else
    {
        uint8_t *map_line;
        uint8_t **map_pointer;
        uint16_t *item_line;
        uint16_t **item_pointer;
        vector_t *grav_line;
        vector_t **grav_pointer;

        map_pointer = world->block;
        map_line = (uint8_t *)((uint8_t **)map_pointer + world->x);
        item_pointer = world->itemID;
        item_line = (uint16_t *)((uint16_t **)item_pointer + world->x);
        grav_pointer = world->gravity;
        grav_line = (vector_t *)((vector_t **)grav_pointer + world->x);

        for (x = 0; x < world->x; x++)
        {
            *map_pointer = map_line;
            map_pointer += 1;
            map_line += world->y;
            *item_pointer = item_line;
            item_pointer += 1;
            item_line += world->y;
            *grav_pointer = grav_line;
            grav_pointer += 1;
            grav_line += world->y;
        }
    }
}

bool Grok_map(void)
{
    int i, x, y, c;
    char *s;

    xpprintf("grok map: init map\n");
    Init_map();

    if (options.mapWidth <= 0 || options.mapWidth > MAX_MAP_SIZE ||
        options.mapHeight <= 0 || options.mapHeight > MAX_MAP_SIZE)
    {
        warn("mapWidth or mapHeight exceeds map size limit [1, %d]",
             MAX_MAP_SIZE);
        free(options.mapData);
        options.mapData = NULL;
    }
    else
    {
        world->x = options.mapWidth;
        world->y = options.mapHeight;
    }
    if (options.extraBorder)
    {
        world->x += 2;
        world->y += 2;
    }
    world->diagonal = (int)LENGTH(world->x, world->y);

    world->width = world->x * BLOCK_SZ;
    world->height = world->y * BLOCK_SZ;
    world->hypotenuse = (int)LENGTH(world->width, world->height);

    world->cwidth = PIXEL_TO_CLICK(world->width);
    world->cheight = PIXEL_TO_CLICK(world->height);
    world->click_hypotenuse = LENGTH(world->cwidth, world->cheight);

    strlcpy(world->name, options.mapName, sizeof(world->name));
    strlcpy(world->author, options.mapAuthor, sizeof(world->author));

    if (!options.mapData)
    {
        warn("Generating random map");
        Generate_random_map();
        if (!options.mapData)
            return false;
    }

    xpprintf("grok map: alloc map\n");
    Alloc_map();

    x = -1;
    y = world->y - 1;

    xpprintf("grok map: set world rules\n");
    Set_world_rules();
    xpprintf("grok map: set world items\n");
    Set_world_items();
    xpprintf("grok map: set world asteroids\n");
    Set_world_asteroids();

    if (BIT(world->rules->mode, TEAM_PLAY | TIMING) == (TEAM_PLAY | TIMING))
    {
        warn("Cannot teamplay while in race mode -- ignoring teamplay");
        CLR_BIT(world->rules->mode, TEAM_PLAY);
    }

    xpprintf("grok map: reading mapdata\n");

    Xpmap_grok_map_data();

    xpprintf("grok map: allocate objects\n");

    Xpmap_tags_to_internal_data();

    /* kps - what are these doing here ? */
    if (options.maxRobots == -1)
        options.maxRobots = Num_bases();
    if (options.minRobots == -1)
        options.minRobots = options.maxRobots;
    if (BIT(world->rules->mode, TIMING))
        Find_base_order();

#ifndef SILENT
    xpprintf("World....: %s\nBases....: %d\nMapsize..: %dx%d\nTeam play: %s\n",
             world->name, world->NumBases, world->x, world->y,
             BIT(world->rules->mode, TEAM_PLAY) ? "on" : "off");
#endif

    D(Print_map());

    xpprintf("grok map: returning true\n");

    return true;
}

/*
 * Use wildmap to generate a random map.
 */
static void Generate_random_map(void)
{
    int width, height;

    options.edgeWrap = true;
    width = world->x;
    height = world->y;

    Wildmap(width, height, world->name, world->author, &options.mapData, &width, &height);

    world->x = width;
    world->y = height;
    world->diagonal = (int)LENGTH(world->x, world->y);

    world->width = world->x * BLOCK_SZ;
    world->height = world->y * BLOCK_SZ;
    world->hypotenuse = (int)LENGTH(world->width, world->height);

    world->cwidth = PIXEL_TO_CLICK(world->width);
    world->cheight = PIXEL_TO_CLICK(world->height);
    world->click_hypotenuse = LENGTH(world->cwidth, world->cheight);
}

/*
 * Find the correct direction of the base, according to the gravity in
 * the base region.
 *
 * If a base attractor is adjacent to a base then the base will point
 * to the attractor.
 */
void Find_base_direction(void)
{
    int i;

    for (i = 0; i < Num_bases(); i++)
    {
        int x = world->bases[i].blk_pos.bx,
            y = world->bases[i].blk_pos.by,
            dir,
            att;
        double dx = world->gravity[x][y].x,
               dy = world->gravity[x][y].y;

        if (dx == 0.0 && dy == 0.0)
        {                 /* Undefined direction? */
            dir = DIR_UP; /* Should be set to direction of gravity! */
        }
        else
        {
            dir = (int)findDir(-dx, -dy);
            dir = ((dir + RES / 8) / (RES / 4)) * (RES / 4); /* round it */
            dir = MOD2(dir, RES);
        }
        att = -1;
        /*BASES SNAP TO UPWARDS ATTRACTOR FIRST*/
        if (y == world->y - 1 && world->block[x][0] == BASE_ATTRACTOR && BIT(world->rules->mode, WRAP_PLAY))
        { /*check wrapped*/
            if (att == -1 || dir == DIR_UP)
            {
                att = DIR_UP;
            }
        }
        if (y < world->y - 1 && world->block[x][y + 1] == BASE_ATTRACTOR)
        {
            if (att == -1 || dir == DIR_UP)
            {
                att = DIR_UP;
            }
        }
        /*THEN DOWNWARDS ATTRACTORS*/
        if (y == 0 && world->block[x][world->y - 1] == BASE_ATTRACTOR && BIT(world->rules->mode, WRAP_PLAY))
        { /*check wrapped*/
            if (att == -1 || dir == DIR_DOWN)
            {
                att = DIR_DOWN;
            }
        }
        if (y > 0 && world->block[x][y - 1] == BASE_ATTRACTOR)
        {
            if (att == -1 || dir == DIR_DOWN)
            {
                att = DIR_DOWN;
            }
        }
        /*THEN RIGHTWARDS ATTRACTORS*/
        if (x == world->x - 1 && world->block[0][y] == BASE_ATTRACTOR && BIT(world->rules->mode, WRAP_PLAY))
        { /*check wrapped*/
            if (att == -1 || dir == DIR_RIGHT)
            {
                att = DIR_RIGHT;
            }
        }
        if (x < world->x - 1 && world->block[x + 1][y] == BASE_ATTRACTOR)
        {
            if (att == -1 || dir == DIR_RIGHT)
            {
                att = DIR_RIGHT;
            }
        }
        /*THEN LEFTWARDS ATTRACTORS*/
        if (x == 0 && world->block[world->x - 1][y] == BASE_ATTRACTOR && BIT(world->rules->mode, WRAP_PLAY))
        { /*check wrapped*/
            if (att == -1 || dir == DIR_LEFT)
            {
                att = DIR_LEFT;
            }
        }
        if (x > 0 && world->block[x - 1][y] == BASE_ATTRACTOR)
        {
            if (att == -1 || dir == DIR_LEFT)
            {
                att = DIR_LEFT;
            }
        }
        if (att != -1)
        {
            dir = att;
        }
        world->bases[i].dir = dir;
    }
    for (i = 0; i < world->x; i++)
    {
        int j;
        for (j = 0; j < world->y; j++)
        {
            if (world->block[i][j] == BASE_ATTRACTOR)
            {
                world->block[i][j] = SPACE;
            }
        }
    }
}

/*
 * Return the team that is closest to this click position.
 */
int Find_closest_team(clpos_t pos)
{
    int team = TEAM_NOT_SET, i;
    double closest = FLT_MAX, l;

    for (i = 0; i < Num_bases(); i++)
    {
        base_t *base = Base_by_index(i);

        if (base->team == TEAM_NOT_SET)
            continue;

        l = Wrap_length(pos.cx - base->pos.cx, pos.cy - base->pos.cy);
        if (l < closest)
        {
            team = world->bases[i].team;
            closest = l;
        }
    }

    return team;
}

/*
 * Determine the order in which players are placed
 * on starting positions after race mode reset.
 */
static void Find_base_order(void)
{
    int i, j, k, n;
    int ccx, ccy;
    double dist;

    if (!BIT(world->rules->mode, TIMING))
    {
        world->baseorder = NULL;
        return;
    }
    if ((n = world->NumBases) <= 0)
    {
        error("Cannot support race mode in a map without bases");
        exit(-1);
    }

    if ((world->baseorder = (baseorder_t *)
             malloc(n * sizeof(baseorder_t))) == NULL)
    {
        error("Out of memory - baseorder");
        exit(-1);
    }

    ccx = world->checks[0].x * BLOCK_CLICKS;
    ccy = world->checks[0].y * BLOCK_CLICKS;
    for (i = 0; i < n; i++)
    {
        dist = Wrap_length(world->bases[i].pos.cx - ccx,
                           world->bases[i].pos.cy - ccy) /
               CLICK;
        for (j = 0; j < i; j++)
        {
            if (world->baseorder[j].dist > dist)
                break;
        }
        for (k = i - 1; k >= j; k--)
            world->baseorder[k + 1] = world->baseorder[k];

        world->baseorder[j].base_idx = i;
        world->baseorder[j].dist = dist;
    }
}

static void Compute_global_gravity(void)
{
    int xi, yi, dx, dy;
    double xforce, yforce, strength;
    double theta;
    vector_t *grav;

    if (options.gravityPointSource == false)
    {
        theta = (options.gravityAngle * PI) / 180.0;
        xforce = cos(theta) * options.gravity;
        yforce = sin(theta) * options.gravity;
        for (xi = 0; xi < world->x; xi++)
        {
            grav = world->gravity[xi];

            for (yi = 0; yi < world->y; yi++, grav++)
            {
                grav->x = xforce;
                grav->y = yforce;
            }
        }
    }
    else
    {
        for (xi = 0; xi < world->x; xi++)
        {
            grav = world->gravity[xi];
            dx = (xi - options.gravityPoint.x) * BLOCK_SZ;
            dx = WRAP_DX(dx);

            for (yi = 0; yi < world->y; yi++, grav++)
            {
                dy = (yi - options.gravityPoint.y) * BLOCK_SZ;
                dy = WRAP_DX(dy);

                if (dx == 0 && dy == 0)
                {
                    grav->x = 0.0;
                    grav->y = 0.0;
                    continue;
                }
                strength = options.gravity / LENGTH(dx, dy);
                if (options.gravityClockwise)
                {
                    grav->x = dy * strength;
                    grav->y = -dx * strength;
                }
                else if (options.gravityAnticlockwise)
                {
                    grav->x = -dy * strength;
                    grav->y = dx * strength;
                }
                else
                {
                    grav->x = dx * strength;
                    grav->y = dy * strength;
                }
            }
        }
    }
}

static void Compute_grav_tab(vector_t grav_tab[GRAV_RANGE + 1][GRAV_RANGE + 1])
{
    int x, y;
    double strength;

    grav_tab[0][0].x = grav_tab[0][0].y = 0;
    for (x = 0; x < GRAV_RANGE + 1; x++)
    {
        for (y = (x == 0); y < GRAV_RANGE + 1; y++)
        {
            strength = pow((double)(sqr(x) + sqr(y)), -1.5);
            grav_tab[x][y].x = x * strength;
            grav_tab[x][y].y = y * strength;
        }
    }
}

static void Compute_local_gravity(void)
{
    int xi, yi, g, gx, gy, ax, ay, dx, dy, gtype;
    int first_xi, last_xi, first_yi, last_yi, mod_xi, mod_yi;
    int min_xi, max_xi, min_yi, max_yi;
    double force, fx, fy;
    vector_t *v, *grav, *tab, grav_tab[GRAV_RANGE + 1][GRAV_RANGE + 1];

    Compute_grav_tab(grav_tab);

    min_xi = 0;
    max_xi = world->x - 1;
    min_yi = 0;
    max_yi = world->y - 1;
    if (BIT(world->rules->mode, WRAP_PLAY))
    {
        min_xi -= MIN(GRAV_RANGE, world->x);
        max_xi += MIN(GRAV_RANGE, world->x);
        min_yi -= MIN(GRAV_RANGE, world->y);
        max_yi += MIN(GRAV_RANGE, world->y);
    }
    for (g = 0; g < world->NumGravs; g++)
    {
        gx = world->gravs[g].blk_pos.bx;
        gy = world->gravs[g].blk_pos.by;
        force = world->gravs[g].force;

        if ((first_xi = gx - GRAV_RANGE) < min_xi)
            first_xi = min_xi;
        if ((last_xi = gx + GRAV_RANGE) > max_xi)
            last_xi = max_xi;
        if ((first_yi = gy - GRAV_RANGE) < min_yi)
            first_yi = min_yi;
        if ((last_yi = gy + GRAV_RANGE) > max_yi)
            last_yi = max_yi;
        gtype = world->block[gx][gy];
        mod_xi = (first_xi < 0) ? (first_xi + world->x) : first_xi;
        dx = gx - first_xi;
        fx = force;
        for (xi = first_xi; xi <= last_xi; xi++, dx--)
        {
            if (dx < 0)
            {
                fx = -force;
                ax = -dx;
            }
            else
            {
                ax = dx;
            }
            mod_yi = (first_yi < 0) ? (first_yi + world->y) : first_yi;
            dy = gy - first_yi;
            grav = &world->gravity[mod_xi][mod_yi];
            tab = grav_tab[ax];
            fy = force;
            for (yi = first_yi; yi <= last_yi; yi++, dy--)
            {
                if (dx || dy)
                {
                    if (dy < 0)
                    {
                        fy = -force;
                        ay = -dy;
                    }
                    else
                    {
                        ay = dy;
                    }
                    v = &tab[ay];
                    if (gtype == CWISE_GRAV || gtype == ACWISE_GRAV)
                    {
                        grav->x -= fy * v->y;
                        grav->y += fx * v->x;
                    }
                    else if (gtype == UP_GRAV || gtype == DOWN_GRAV)
                    {
                        grav->y += force * v->x;
                    }
                    else if (gtype == RIGHT_GRAV || gtype == LEFT_GRAV)
                    {
                        grav->x += force * v->y;
                    }
                    else
                    {
                        grav->x += fx * v->x;
                        grav->y += fy * v->y;
                    }
                }
                else
                {
                    if (gtype == UP_GRAV || gtype == DOWN_GRAV)
                    {
                        grav->y += force;
                    }
                    else if (gtype == LEFT_GRAV || gtype == RIGHT_GRAV)
                    {
                        grav->x += force;
                    }
                }
                mod_yi++;
                grav++;
                if (mod_yi >= world->y)
                {
                    mod_yi = 0;
                    grav = world->gravity[mod_xi];
                }
            }
            if (++mod_xi >= world->x)
            {
                mod_xi = 0;
            }
        }
    }
    /*
     * We may want to free the world->gravity memory here
     * as it is not used anywhere else.
     * e.g.: free(world->gravity);
     *       world->gravity = NULL;
     *       world->NumGravs = 0;
     * Some of the more modern maps have quite a few gravity symbols.
     */
}

void Compute_gravity(void)
{
    Compute_global_gravity();
    Compute_local_gravity();
}

void add_temp_wormholes(int xin, int yin, int xout, int yout)
{
    wormhole_t inhole, outhole, *wwhtemp;

    if ((wwhtemp = (wormhole_t *)realloc(world->wormholes,
                                         (world->NumWormholes + 2) * sizeof(wormhole_t))) == NULL)
    {
        error("No memory for temporary wormholes.");
        return;
    }
    world->wormholes = wwhtemp;

    inhole.blk_pos.bx = xin;
    inhole.blk_pos.by = yin;
    outhole.blk_pos.bx = xout;
    outhole.blk_pos.by = yout;
    inhole.countdown = outhole.countdown = options.wormTime;
    inhole.lastdest = world->NumWormholes + 1;
    inhole.temporary = outhole.temporary = 1;
    inhole.type = WORM_IN;
    outhole.type = WORM_OUT;
    inhole.lastblock = world->block[xin][yin];
    outhole.lastblock = world->block[xout][yout];
    inhole.lastID = world->itemID[xin][yin];
    outhole.lastID = world->itemID[xout][yout];
    world->wormholes[world->NumWormholes] = inhole;
    world->wormholes[world->NumWormholes + 1] = outhole;
    world->block[xin][yin] = world->block[xout][yout] = WORMHOLE;
    world->itemID[xin][yin] = world->NumWormholes;
    world->itemID[xout][yout] = world->NumWormholes + 1;
    world->NumWormholes += 2;
}

void remove_temp_wormhole(int ind)
{
    wormhole_t hole;

    hole = world->wormholes[ind];
    world->block[hole.blk_pos.bx][hole.blk_pos.by] = hole.lastblock;
    world->itemID[hole.blk_pos.bx][hole.blk_pos.by] = hole.lastID;
    world->NumWormholes--;
    if (ind != world->NumWormholes)
    {
        world->wormholes[ind] = world->wormholes[world->NumWormholes];
    }
    world->wormholes = (wormhole_t *)realloc(world->wormholes,
                                             world->NumWormholes * sizeof(wormhole_t));
}

double Wrap_findDir(double dx, double dy)
{
    dx = WRAP_DX(dx);
    dy = WRAP_DY(dy);
    return findDir(dx, dy);
}

double Wrap_cfindDir(int dcx, int dcy)
{
    dcx = WRAP_DCX(dcx);
    dcy = WRAP_DCY(dcy);
    return findDir((double)dcx, (double)dcy);
}

double Wrap_length(int dcx, int dcy)
{
    dcx = WRAP_DCX(dcx);
    dcy = WRAP_DCY(dcy);
    return LENGTH(dcx, dcy);
}
