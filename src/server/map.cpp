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

static void Generate_random_map(void);

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
    world->pixel_hypotenuse = (int)LENGTH(world->width, world->height);

    world->cwidth = PIXEL_TO_CLICK(world->width);
    world->cheight = PIXEL_TO_CLICK(world->height);
    world->click_hypotenuse = LENGTH(world->cwidth, world->cheight);
}

static void Find_base_order(void);

static void Check_map_object_counters(void)
{
    int i;

    /*assert(world->NumCannons == 0);*/
    /*assert(world->NumGravs == 0);*/
    /*assert(world->NumWormholes == 0);*/
    /*assert(world->NumTreasures == 0);*/
    /*assert(world->NumTargets == 0);*/
    /*assert(world->NumBases == 0);*/
    /*assert(world->NumItemConcs == 0);
      assert(world->NumAsteroidConcs == 0);
      assert(world->NumFrictionAreas == 0);*/
    /*assert(world->NumEcms == 0);*/
    /*assert(world->NumTransporters == 0);*/

    for (i = 0; i < MAX_TEAMS; i++)
    {
        assert(world->teams[i].NumMembers == 0);
        assert(world->teams[i].NumRobots == 0);
        assert(world->teams[i].NumBases == 0);
        assert(world->teams[i].NumTreasures == 0);
        assert(world->teams[i].NumEmptyTreasures == 0);
        assert(world->teams[i].TreasuresDestroyed == 0);
        assert(world->teams[i].TreasuresLeft == 0);
        assert(world->teams[i].SwapperId == NO_ID);
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

static void Realloc_map_objects(void)
{
    /*SHRINK(cannon_t, world->cannons, world->NumCannons, world->MaxCannons);*/
    /*SHRINK(fuel_t, world->fuels, world->NumFuels, world->MaxFuels);*/
    /*SHRINK(grav_t, world->gravs, world->NumGravs, world->MaxGravs);*/
    /*SHRINK(wormhole_t, world->wormholes,
      world->NumWormholes, world->MaxWormholes);*/
    /*SHRINK(treasure_t, world->treasures,
      world->NumTreasures, world->MaxTreasures);*/
    /*SHRINK(target_t, world->targets, world->NumTargets, world->MaxTargets);*/
    /*SHRINK(base_t, world->bases, world->NumBases, world->MaxBases);*/
    /*SHRINK(item_concentrator_t, world->itemConcs,
      world->NumItemConcs, world->MaxItemConcs);
      SHRINK(asteroid_concentrator_t, world->asteroidConcs,
      world->NumAsteroidConcs, world->MaxAsteroidConcs);
      SHRINK(friction_area_t, world->frictionAreas,
      world->NumFrictionAreas, world->MaxFrictionAreas);*/
}

int World_place_cannon(clpos_t pos, int dir, int team)
{
    cannon_t t, *cannon;
    int ind = Num_cannons(), i;

    t.pos = pos;
    t.dir = dir;
    t.team = team;
    t.dead_ticks = 0;
    t.conn_mask = ~0;
    t.group = NO_GROUP;

    t.blk_pos = Clpos_to_blkpos(pos);
    t.pix_pos = Clpos_to_position(pos);

    // world->fuels[ind] = t;
    // world->NumFuels++;
    world->cannons.push_back(t);

    // world->cannons[ind] = t;
    Cannon_init(Cannon_by_index(ind));
    // world->NumCannons++;
    return ind;

    // cannon_t t, *cannon;
    // int ind = Num_cannons(), i;

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
}

int World_place_fuel(clpos_t pos, int team)
{
    fuel_t t;
    int ind = Num_fuels();

    t.pos = pos;
    t.fuel = START_STATION_FUEL;
    t.conn_mask = ~0;
    t.last_change = frame_loops;
    t.team = team;

    t.blk_pos = Clpos_to_blkpos(pos);
    t.pix_pos = Clpos_to_position(pos);

    world->fuels.push_back(t);

    return ind;
}

int World_place_base(clpos_t pos, int dir, int team, int order)
{
    base_t t;
    int ind = Num_bases(), i;

    t.pos = pos;
    // t.order = order;
    /*
     * The direction of the base should be so that it points
     * up with respect to the gravity in the region.  This
     * is fixed in Find_base_direction() when the gravity has
     * been computed.
     */
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
    treasure_t t;
    int ind = Num_treasures();
    t.blk_pos = Clpos_to_blkpos(pos);
    t.pos = pos;
    t.have = false;
    t.destroyed = 0;
    t.empty = empty;

    // Arraylist_add(world->treasures, &t);
    world->treasures[ind] = t;
    world->NumTreasures++;
    return ind;

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
}

int World_place_target(clpos_t pos, int team)
{
    target_t t;
    int ind = Num_targets();

    t.blk_pos = Clpos_to_blkpos(pos);
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
    // t.group = NO_GROUP;

    // world->targets[ind] = t;
    // world->NumTargets++;
    world->targets.push_back(t);

    return ind;
}

int World_place_wormhole(clpos_t pos, wormtype_t type)
{
    wormhole_t t;
    int ind = Num_wormholes();
    t.blk_pos = Clpos_to_blkpos(pos);
    t.pos = pos;
    t.countdown = 0;
    t.lastdest = -1;
    t.temporary = false;
    t.lastblock = SPACE;
    t.lastID = -1;

    // Arraylist_add(world->wormholes, &t);
    world->wormholes[ind] = t;
    world->NumWormholes++;
    return ind;

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
    item_concentrator_t t;
    int ind = Num_itemConcs();

    t.pos = pos;
    world->itemConcentrators.push_back(t);

    return ind;
}

int World_place_asteroid_concentrator(clpos_t pos)
{
    asteroid_concentrator_t t;
    int ind = Num_asteroidConcs();

    t.pos = pos;
    world->asteroidConcs.push_back(t);

    return ind;
}

int World_place_grav(clpos_t pos, double force, int type)
{
    grav_t t;
    int ind = Num_gravs();

    t.pos = pos;
    t.blk_pos = Clpos_to_blkpos(pos);
    t.force = force;
    world->gravs.push_back(t);

    return ind;
}

int World_place_friction_area(clpos_t pos, double fric)
{
    friction_area_t t;
    int ind = Num_frictionAreas();
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
    warn("World_init called");

    int i;

    memset(world, 0, sizeof(world_t));

#if 0
    if ((world->asteroidConcs = Arraylist_alloc(sizeof(asteroid_concentrator_t))) == NULL)
        return -1;
    if ((world->bases = Arraylist_alloc(sizeof(base_t))) == NULL)
        return -1;
    if ((world->cannons = Arraylist_alloc(sizeof(cannon_t))) == NULL)
        return -1;
    if ((world->ecms = Arraylist_alloc(sizeof(ecm_t))) == NULL)
        return -1;
    if ((world->frictionAreas = Arraylist_alloc(sizeof(friction_area_t))) == NULL)
        return -1;
    if ((world->fuels = Arraylist_alloc(sizeof(fuel_t))) == NULL)
        return -1;
    if ((world->itemConcs = Arraylist_alloc(sizeof(item_concentrator_t))) == NULL)
        return -1;
    if ((world->gravs = Arraylist_alloc(sizeof(grav_t))) == NULL)
        return -1;
    if ((world->targets = Arraylist_alloc(sizeof(target_t))) == NULL)
        return -1;
    if ((world->treasures = Arraylist_alloc(sizeof(treasure_t))) == NULL)
        return -1;
    if ((world->transporters = Arraylist_alloc(sizeof(transporter_t))) == NULL)
        return -1;
    if ((world->wormholes = Arraylist_alloc(sizeof(wormhole_t))) == NULL)
        return -1;
#endif

    for (i = 0; i < MAX_TEAMS; i++)
        Team_by_index(i)->SwapperId = NO_ID;

    Filled_wire_init();

    return 0;
}

static void Init_map(void)
{
    world->x = 256;
    world->y = 256;
    world->diagonal = (int)LENGTH(world->x, world->y);

    world->width = world->x * BLOCK_SZ;
    world->height = world->y * BLOCK_SZ;
    world->pixel_hypotenuse = (int)LENGTH(world->width, world->height);

    world->cwidth = PIXEL_TO_CLICK(world->width);
    world->cheight = PIXEL_TO_CLICK(world->height);
    world->click_hypotenuse = LENGTH(world->cwidth, world->cheight);

    // world->NumAsteroidConcs = 0;
    world->asteroidConcs.clear();

    world->NumBases = 0;
    // world->bases.clear();

    // world->NumCannons = 0;
    world->cannons.clear();

    world->NumEcms = 0;

    // world->NumFuels = 0;
    world->fuels.clear();

    // world->NumFrictionAreas = 0;
    world->frictionAreas.clear();

    // world->NumGravs = 0;
    world->gravs.clear();
    // world->NumItemConcentrators = 0;
    world->itemConcentrators.clear();

    // world->NumTargets = 0;
    world->targets.clear();

    world->NumTransporters = 0;
    world->NumTreasures = 0;
    world->NumWormholes = 0;
}

void World_free(void)
{
    XFREE(world->block);
    XFREE(world->itemID);
    XFREE(world->gravity);
    world->asteroidConcs.clear();
    XFREE(world->bases);
    world->cannons.clear();
    XFREE(world->ecms);
    world->fuels.clear();
    world->frictionAreas.clear();
    world->gravs.clear();
    world->itemConcentrators.clear();
    world->targets.clear();
    XFREE(world->transporters);
    XFREE(world->treasures);
    XFREE(world->wormholes);
}

static bool World_alloc(void)
{
    int x;
    uint8_t *map_line;
    uint8_t **map_pointer;
    uint16_t *item_line;
    uint16_t **item_pointer;
    vector_t *grav_line;
    vector_t **grav_pointer;

    assert(world->block == NULL);
    assert(world->gravity == NULL);

    // if (world->block || world->gravity)
    //     World_free();

    world->block = (uint8_t **)
        malloc(sizeof(uint8_t *) * world->x + world->x * sizeof(uint8_t) * world->y);
    world->itemID = (uint16_t **)
        malloc(sizeof(uint16_t *) * world->x + world->x * sizeof(uint16_t) * world->y);
    world->gravity = (vector_t **)
        malloc(sizeof(vector_t *) * world->x + world->x * sizeof(vector_t) * world->y);
    // world->gravs = NULL;
    world->bases = NULL;
    // world->fuels = NULL;
    // world->cannons = NULL;
    world->wormholes = NULL;
    // world->itemConcentrators = NULL;
    // world->asteroidConcs = NULL;
    world->ecms = NULL;
    // world->frictionAreas = NULL;
    world->transporters = NULL;

    /*assert(world->gravs == NULL);*/
    /*assert(world->bases == NULL);*/
    /*assert(world->fuels == NULL);*/
    /*assert(world->cannons == NULL);*/
    // assert(world->checks == NULL);
    /*assert(world->wormholes == NULL);*/
    /*assert(world->itemConcs == NULL);*/
    /*assert(world->asteroidConcs == NULL);*/

    if (world->block == NULL || world->itemID == NULL || world->gravity == NULL)
    {
        World_free();
        error("Couldn't allocate memory for map");
        return false;
    }

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

    return true;
}

/*
 * This function can be called after the map options have been read.
 */
static bool Grok_map_size(void)
{
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
    world->pixel_hypotenuse = LENGTH(world->width, world->height);

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

bool Grok_map(void)
{
    warn("Grok_map: start");

    int i, x, y, c;
    char *s;

    xpprintf("grok map: init map\n");
    Init_map();

    if (options.mapWidth <= 0 || options.mapWidth > OLD_MAX_MAP_SIZE ||
        options.mapHeight <= 0 || options.mapHeight > OLD_MAX_MAP_SIZE)
    {
        warn("mapWidth or mapHeight exceeds map size limit [1, %d]",
             OLD_MAX_MAP_SIZE);
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
    world->pixel_hypotenuse = (int)LENGTH(world->width, world->height);

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
    World_alloc();

    x = -1;
    y = world->y - 1;

    Set_world_rules();
    Set_world_items();
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

    xpprintf("World....: %s\nBases....: %d\nMapsize..: %dx%d\nTeam play: %s\n",
             world->name, Num_bases(), world->x, world->y,
             BIT(world->rules->mode, TEAM_PLAY) ? "on" : "off");

    D(Print_map());

    xpprintf("grok map: returning true\n");

    return true;
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
    if ((n = Num_bases()) <= 0)
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
