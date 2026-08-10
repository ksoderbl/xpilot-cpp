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

#include "bit.h"
#include "commonmacros.h"
#include "commonproto.h"
#include "xperror.h"

#include "xpconfig.h"

#include "cannon.h"
#include "map.h"
#include "server.h"
#include "serverconst.h"
#include "wormhole.h"

#define GRAV_RANGE 10

/*
 * Globals.
 */
world_t theWorld;
bool is_polygon_map = false;

static void Find_base_order(void);

/*
 * Determine the order in which players are placed
 * on starting positions after race mode reset.
 */
static void Find_base_order(void)
{
    world_t *world = &theWorld;
    int i, j, k, n;
    int ccx, ccy;
    double dist;

    if (!Timing(world))
    {
        world->baseorder = nullptr;
        return;
    }
    if ((n = Num_bases(world)) <= 0)
    {
        error("Cannot support race mode in a map without bases");
        exit(-1);
    }

    if ((world->baseorder = (baseorder_t *)
             malloc(n * sizeof(baseorder_t))) == nullptr)
    {
        error("Out of memory - baseorder");
        exit(-1);
    }

    ccx = world->checks[0].pos.cx;
    ccy = world->checks[0].pos.cy;
    for (i = 0; i < n; i++)
    {
        dist = World_wrap_length(
                   world,
                   world->bases[i].pos.cx - ccx,
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

static void shrink(void **pp, size_t size)
{
    void *p;

    p = realloc(*pp, size);
    if (!p)
    {
        warn("Realloc failed!");
        exit(1);
    }
    *pp = p;
}

#define SHRINK(T, P, N, M)                          \
    {                                               \
        if ((M) > (N))                              \
        {                                           \
            shrink((void **)&(P), (N) * sizeof(T)); \
            M = (N);                                \
        }                                           \
    }

int World_place_cannon(world_t *world, clpos_t pos, int dir, int team)
{
    cannon_t t, *cannon;
    int ind = Num_cannons(world), i;

    t.pos = pos;
    t.dir = dir;
    t.team = team;
    t.dead_ticks = 0;
    t.conn_mask = ~0;
    t.group = NO_GROUP;

    // world->fuels[ind] = t;
    // world->NumFuels++;
    world->cannons.push_back(t);

    // world->cannons[ind] = t;
    Cannon_init(Cannon_by_index(world, ind));
    // world->NumCannons++;
    return ind;

    // cannon_t t, *cannon;
    // int ind = Num_cannons(world), i;

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
    // cannon = Cannon_by_index(world, ind);
    // assert(Cannon_by_id(t.id) == cannon);

    // return ind;
}

int World_place_fuel(world_t *world, clpos_t pos, int team)
{
    fuel_t t;
    int ind = Num_fuels(world);

    t.pos = pos;
    t.fuel = START_STATION_FUEL;
    t.conn_mask = ~0;
    t.last_change = frame_loops;
    t.team = team;

    world->fuels.push_back(t);

    return ind;
}

int World_place_base(world_t *world, clpos_t pos, int dir, int team, int order)
{
    base_t t;
    int ind = Num_bases(world), i;

    t.pos = pos;
    t.order = order;
    /*
     * The direction of the base should be so that it points
     * up with respect to the gravity in the region.  This
     * is fixed in Find_base_direction() when the gravity has
     * been computed.
     */
    if (dir < 0 || dir >= ANGLE_RESOLUTION)
    {
        warn("Base with direction %d in map.", dir);
        warn("Valid base directions are from 0 to %d.", ANGLE_RESOLUTION - 1);
        while (dir < 0)
            dir += ANGLE_RESOLUTION;
        while (dir >= ANGLE_RESOLUTION)
            dir -= ANGLE_RESOLUTION;
        warn("Using direction %d for this base.", dir);
    }

    t.dir = dir;

    if (Team_play(world))
    {
        if (team < 0 || team >= MAX_TEAMS)
            team = 0;

        t.team = team;
        world->teams[team].NumBases++;
        if (world->teams[team].NumBases == 1)
            world->NumTeamBases++;
    }
    else
        t.team = TEAM_NOT_SET;
    t.ind = Num_bases(world);

    world->bases.push_back(t);

    return ind;
}

int World_place_treasure(world_t *world, clpos_t pos, int team, bool empty,
                         int ball_style)
{
    treasure_t t;
    int ind = Num_treasures(world);

    t.pos = pos;
    t.have = false;
    t.destroyed = 0;
    t.team = team;
    t.empty = empty;

    // t.ball_style = ball_style;
    // if (team != TEAM_NOT_SET)
    // {
    //     world->teams[team].NumTreasures++;
    //     world->teams[team].TreasuresLeft++;
    // }
    world->treasures.push_back(t);

    return ind;
}

int World_place_target(world_t *world, clpos_t pos, int team)
{
    target_t t;
    int ind = Num_targets(world);

    t.pos = pos;
    /*
     * If we have a block based map, the team is determined in
     * in Xpmap_find_map_object_teams().
     */
    t.team = team;
    t.dead_ticks = 0;
    t.damage = TARGET_DAMAGE;
    t.conn_mask = ~0;
    t.update_mask = 0;
    t.last_change = frame_loops;
    t.group = NO_GROUP;

    world->targets.push_back(t);

    return ind;
}

int World_place_wormhole(world_t *world, clpos_t pos, wormtype_t type)
{
    wormhole_t t;
    int ind = Num_wormholes(world);

    t.pos = pos;
    t.countdown = 0;
    t.lastdest = NO_IND;
    t.temporary = false;
    t.type = type;
    t.lastblock = SPACE;
    t.lastID = NO_ID;
    t.group = NO_GROUP;

    world->wormholesVector.push_back(t);

    return ind;
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

int World_place_check(world_t *world, clpos_t pos, int ind)
{
    // check_t t;

    // if (!Timing(world))
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
    //      * nullptr since ind can here be >= world->NumChecks.
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

int World_place_item_concentrator(world_t *world, clpos_t pos)
{
    item_concentrator_t t;
    int ind = Num_itemConcs(world);

    t.pos = pos;
    world->itemConcs.push_back(t);

    return ind;
}

int World_place_asteroid_concentrator(world_t *world, clpos_t pos)
{
    asteroid_concentrator_t t;
    int ind = Num_asteroidConcs(world);

    t.pos = pos;
    world->asteroidConcs.push_back(t);

    return ind;
}

int World_place_grav(world_t *world, clpos_t pos, double force, int type)
{
    grav_t t;
    int ind = Num_gravs(world);

    t.pos = pos;
    t.force = force;
    world->gravs.push_back(t);

    return ind;
}

int World_place_friction_area(world_t *world, clpos_t pos, double fric)
{
    friction_area_t t;
    int ind = Num_frictionAreas(world);

    t.pos = pos;
    t.friction_setting = fric;
    world->frictionAreas.push_back(t);

    return ind;
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

int World_init(void)
{
    world_t *world = &theWorld;
    warn("World_init");

    int i;

    // TODO: World class
    memset(&theWorld, 0, sizeof(world_t));

    for (i = 0; i < MAX_TEAMS; i++)
        Team_by_index(world, i)->SwapperId = NO_ID;

    Filled_wire_init();

    return 0;
}

void World_free(void)
{
    world_t *world = &theWorld;
    XFREE(world->block);
    XFREE(world->gravity);
    world->asteroidConcs.clear();
    world->bases.clear();
    world->cannons.clear();
    world->ecms.clear();
    world->fuels.clear();
    world->frictionAreas.clear();
    world->gravs.clear();
    world->itemConcs.clear();
    world->targets.clear();
    world->transporters.clear();
    world->treasures.clear();
    XFREE(world->wormholes);
}

static bool World_alloc(void)
{
    world_t *world = &theWorld;
    int x;
    uint8_t *map_line;
    uint8_t **map_pointer;
    vector_t *grav_line;
    vector_t **grav_pointer;

    assert(world->block == nullptr);
    assert(world->gravity == nullptr);

    world->block = (uint8_t **)
        malloc(sizeof(uint8_t *) * world->x + world->x * sizeof(uint8_t) * world->y);
    world->gravity = (vector_t **)
        malloc(sizeof(vector_t *) * world->x + world->x * sizeof(vector_t) * world->y);

    if (world->block == nullptr || world->gravity == nullptr)
    {
        World_free();
        error("Couldn't allocate memory for map");
        return false;
    }

    map_pointer = world->block;
    map_line = (uint8_t *)((uint8_t **)map_pointer + world->x);
    grav_pointer = world->gravity;
    grav_line = (vector_t *)((vector_t **)grav_pointer + world->x);

    for (x = 0; x < world->x; x++)
    {
        *map_pointer = map_line;
        map_pointer += 1;
        map_line += world->y;
        *grav_pointer = grav_line;
        grav_pointer += 1;
        grav_line += world->y;
    }

    return true;
}

/*
 * This function can be called after the map options have been read.
 */
static bool Grok_map_size(void)
{
    world_t *world = &theWorld;
    bool bad = false;
    int w = options.mapWidth, h = options.mapHeight;

    if (!is_polygon_map)
    {
        w *= BLOCK_SZ;
        h *= BLOCK_SZ;
    }

    if (w < NEW_MIN_MAP_SIZE)
    {
        warn("mapWidth too small, minimum is %d pixels (%d blocks).\n",
             NEW_MIN_MAP_SIZE, NEW_MIN_MAP_SIZE / BLOCK_SZ + 1);
        bad = true;
    }
    if (w > NEW_MAX_MAP_SIZE)
    {
        warn("mapWidth too big, maximum is %d pixels (%d blocks).\n",
             NEW_MAX_MAP_SIZE, NEW_MAX_MAP_SIZE / BLOCK_SZ);
        bad = true;
    }

    if (h < NEW_MIN_MAP_SIZE)
    {
        warn("mapHeight too small, minimum is %d pixels (%d blocks).\n",
             NEW_MIN_MAP_SIZE, NEW_MIN_MAP_SIZE / BLOCK_SZ + 1);
        bad = true;
    }
    if (h > NEW_MAX_MAP_SIZE)
    {
        warn("mapWidth too big, maximum is %d pixels (%d blocks).\n",
             NEW_MAX_MAP_SIZE, NEW_MAX_MAP_SIZE / BLOCK_SZ);
        bad = true;
    }

    if (bad)
        return false;

    /* pixel sizes */
    world->width = w;
    world->height = h;
    if (!is_polygon_map && options.extraBorder)
    {
        world->width += 2 * BLOCK_SZ;
        world->height += 2 * BLOCK_SZ;
    }
    world->hypotenuse = LENGTH(world->width, world->height);

    /* click sizes */
    world->cwidth = world->width * CLICK;
    world->cheight = world->height * CLICK;

    /* block sizes */
    world->x = (world->width - 1) / BLOCK_SZ + 1; /* !@# */
    world->y = (world->height - 1) / BLOCK_SZ + 1;
    world->diagonal = LENGTH(world->x, world->y);
    world->bwidth_floor = world->width / BLOCK_SZ;
    world->bheight_floor = world->height / BLOCK_SZ;

    return true;
}

bool Grok_map_options(void)
{
    world_t *world = &theWorld;

    if (world->have_options)
        return true;

    warn("Grok_map_options ----------------->");

    if (!Grok_map_size())
    {
        warn("Grok_map_options -----------------> Grok_map_size failed");
        return false;
    }

    strlcpy(world->name, options.mapName, sizeof(world->name));
    strlcpy(world->author, options.mapAuthor, sizeof(world->author));
    strlcpy(world->dataURL, options.dataURL, sizeof(world->dataURL));

    if (!World_alloc())
    {
        warn("Grok_map_options -----------------> Grok_alloc failed");
        return false;
    }

    Set_world_rules();
    Set_world_items();
    Set_world_asteroids();

    if (BIT(world->rules.mode, TEAM_PLAY | TIMING) == (TEAM_PLAY | TIMING))
    {
        warn("Cannot teamplay while in race mode -- ignoring teamplay");
        CLR_BIT(world->rules.mode, TEAM_PLAY);
    }

    warn("Grok_map_options -----------------> RETURNING OK");

    world->have_options = true;

    return true;
}

static void Init_map(void);

static void Init_map(void)
{
    world_t *world = &theWorld;
    world->x = 256;
    world->y = 256;
    world->diagonal = (int)LENGTH(world->x, world->y);

    world->width = world->x * BLOCK_SZ;
    world->height = world->y * BLOCK_SZ;
    world->hypotenuse = (int)LENGTH(world->width, world->height);

    world->cwidth = PIXEL_TO_CLICK(world->width);
    world->cheight = PIXEL_TO_CLICK(world->height);

    world->asteroidConcs.clear();
    world->bases.clear();
    world->cannons.clear();
    world->ecms.clear();
    world->fuels.clear();
    world->frictionAreas.clear();
    world->gravs.clear();
    world->itemConcs.clear();
    world->targets.clear();
    world->transporters.clear();
    world->treasures.clear();

    world->NumWormholes = 0;
}

static void Generate_random_map(void);

/*
 * Use wildmap to generate a random map.
 */
static void Generate_random_map(void)
{
    world_t *world = &theWorld;
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
}

void Xpmap_grok_map_data2(void)
{
}

void Xpmap_tags_to_internal_data2(void)
{
}

void Xpmap_find_map_object_teams2(void)
{
}

bool Grok_map(void)
{
    world_t *world = &theWorld;

    warn("Grok_map: ========================== START");
    warn("Grok_map: is_polygon_map: %s", is_polygon_map ? "true" : "false");

    if (!Grok_map_options())
        return false;

    if (!is_polygon_map)
    {
        Xpmap_grok_map_data2();
        Xpmap_tags_to_internal_data2();
        Xpmap_find_map_object_teams2();

        printf("grok map: init map\n");
        Init_map();

        if (options.mapWidth <= 0 || options.mapWidth > OLD_MAX_MAP_SIZE ||
            options.mapHeight <= 0 || options.mapHeight > OLD_MAX_MAP_SIZE)
        {
            warn("mapWidth or mapHeight exceeds map size limit [1, %d]",
                 OLD_MAX_MAP_SIZE);
            free(options.mapData);
            options.mapData = nullptr;
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

        strlcpy(world->name, options.mapName, sizeof(world->name));
        strlcpy(world->author, options.mapAuthor, sizeof(world->author));

        if (!options.mapData)
        {
            warn("Generating random map");
            Generate_random_map();
            if (!options.mapData)
                return false;
        }

        // printf("grok map: alloc map\n");
        // Xpmap_world_alloc();

        printf("grok map: reading mapdata\n");

        Xpmap_grok_map_data();

        printf("grok map: allocate objects\n");

        Xpmap_tags_to_internal_data();

        if (Timing(world))
            Find_base_order();

        D(Print_map());
    }

    if (!Verify_wormhole_consistency())
        return false;

    if (Timing(world) && Num_checks(world) == 0)
    {
        warn("No checkpoints found while race mode (timing) was set.");
        warn("Turning off race mode.");
        CLR_BIT(world->rules.mode, TIMING);
    }

    /* kps - what are these doing here ? */
    if (options.maxRobots == -1)
        options.maxRobots = Num_bases(world);

    if (options.minRobots == -1)
        options.minRobots = options.maxRobots;

    if (Num_bases(world) <= 0)
        fatal("Map has no bases!");

    printf("World....: %s\nBases....: %d\nMapsize..: %dx%d pixels\n"
           "Team play: %s\n",
           world->name, Num_bases(world), world->width, world->height,
           Team_play(world) ? "on" : "off");

    if (!is_polygon_map)
        Xpmap_blocks_to_polygons();

    Compute_gravity();
    Find_base_direction();

    // Print out amount of map objects.
    printf("===========\n");
    printf("Asteroid concentrators: %d\n", Num_asteroidConcs(world));
    printf("Bases.................: %d\n", Num_bases(world));
    printf("Cannons...............: %d\n", Num_cannons(world));
    printf("ECMs..................: %d\n", Num_ecms(world));
    printf("Fuels.................: %d\n", Num_fuels(world));
    printf("Friction areas........: %d\n", Num_frictionAreas(world));
    printf("Gravs.................: %d\n", Num_gravs(world));
    printf("Item concentrators....: %d\n", Num_itemConcs(world));
    printf("Targets...............: %d\n", Num_targets(world));
    printf("Transporters..........: %d\n", Num_transporters(world));
    printf("Treasures.............: %d\n", Num_treasures(world));
    printf("Wormholes.............: %d\n", Num_wormholes(world));

    return true;
}

/*
 * Return the team that is closest to this click position.
 */
int Find_closest_team(clpos_t pos)
{
    world_t *world = &theWorld;
    int team = TEAM_NOT_SET, i;
    double closest = FLT_MAX, l;

    for (i = 0; i < Num_bases(world); i++)
    {
        base_t *base = Base_by_index(world, i);

        if (base->team == TEAM_NOT_SET)
            continue;

        l = World_wrap_length(
            world,
            pos.cx - base->pos.cx,
            pos.cy - base->pos.cy);
        if (l < closest)
        {
            team = world->bases[i].team;
            closest = l;
        }
    }

    return team;
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
    /* kps - this might go wrong if we run in -options.polygonMode ? */
    if (!is_polygon_map)
        Xpmap_find_base_direction();
}

double World_wrap_findDir(world_t *world, double dx, double dy)
{
    dx = WORLD_WRAP_DX(world, dx);
    dy = WORLD_WRAP_DY(world, dy);
    return findDir(dx, dy);
}

double World_wrap_cfindDir(world_t *world, int dcx, int dcy)
{
    dcx = WORLD_WRAP_DCX(world, dcx);
    dcy = WORLD_WRAP_DCY(world, dcy);
    return findDir((double)dcx, (double)dcy);
}

double World_wrap_length(world_t *world, int dcx, int dcy)
{
    dcx = WORLD_WRAP_DCX(world, dcx);
    dcy = WORLD_WRAP_DCY(world, dcy);
    return LENGTH(dcx, dcy);
}
