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

#define SERVER
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
world_t World;
bool is_polygon_map = false;

static void Find_base_order(void);

/*
 * Determine the order in which players are placed
 * on starting positions after race mode reset.
 */
static void Find_base_order(void)
{
    int i, j, k, n;
    int ccx, ccy;
    double dist;

    if (!BIT(World.rules->mode, TIMING))
    {
        World.baseorder = NULL;
        return;
    }
    if ((n = Num_bases()) <= 0)
    {
        error("Cannot support race mode in a map without bases");
        exit(-1);
    }

    if ((World.baseorder = (baseorder_t *)
             malloc(n * sizeof(baseorder_t))) == NULL)
    {
        error("Out of memory - baseorder");
        exit(-1);
    }

    ccx = World.checks[0].pos.cx;
    ccy = World.checks[0].pos.cy;
    for (i = 0; i < n; i++)
    {
        dist = Wrap_length(World.bases[i].pos.cx - ccx,
                           World.bases[i].pos.cy - ccy) /
               CLICK;
        for (j = 0; j < i; j++)
        {
            if (World.baseorder[j].dist > dist)
                break;
        }
        for (k = i - 1; k >= j; k--)
            World.baseorder[k + 1] = World.baseorder[k];

        World.baseorder[j].base_idx = i;
        World.baseorder[j].dist = dist;
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

    // World.fuels[ind] = t;
    // World.NumFuels++;
    World.cannons.push_back(t);

    // World.cannons[ind] = t;
    Cannon_init(Cannon_by_index(ind));
    // World.NumCannons++;
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
    // Arraylist_add(World.cannons, &t);
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

    World.fuels.push_back(t);

    return ind;
}

int World_place_base(clpos_t pos, int dir, int team, int order)
{
    base_t t;
    int ind = Num_bases(), i;

    t.blk_pos = Clpos_to_blkpos(pos);

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

    warn("World.rules = %p", World.rules);
    warn("World.rules->mode = %lx", World.rules->mode);

    if (BIT(World.rules->mode, TEAM_PLAY))
    {
        if (team < 0 || team >= MAX_TEAMS)
            team = 0;

        t.team = team;
        World.teams[team].NumBases++;
        if (World.teams[team].NumBases == 1)
            World.NumTeamBases++;
    }
    else
        t.team = TEAM_NOT_SET;
    t.ind = Num_bases();

    World.bases.push_back(t);

    return ind;
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
    t.team = team;
    t.empty = empty;

    // t.ball_style = ball_style;
    // if (team != TEAM_NOT_SET)
    // {
    //     World.teams[team].NumTreasures++;
    //     World.teams[team].TreasuresLeft++;
    // }
    World.treasures.push_back(t);

    return ind;
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
    t.group = NO_GROUP;

    World.targets.push_back(t);

    return ind;
}

int World_place_wormhole(clpos_t pos, wormtype_t type)
{
    wormhole_t t;
    int ind = Num_wormholes();
    t.blk_pos = Clpos_to_blkpos(pos);
    t.pos = pos;
    t.countdown = 0;
    t.lastdest = NO_IND;
    t.temporary = false;
    t.type = type;
    t.lastblock = SPACE;
    t.lastID = NO_ID;
    t.group = NO_GROUP;

    // World.wormholes[ind] = t;
    // World.NumWormholes++;
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
    //     STORE(check_t, World.checks, World.NumChecks, World.MaxChecks, t);

    // SHRINK(check_t, World.checks, World.NumChecks, World.MaxChecks);
    // World.NumChecks = 0;
}

int World_place_check(clpos_t pos, int ind)
{
    // check_t t;

    // if (!BIT(World.rules->mode, TIMING))
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

    //     if (World.NumChecks == 0)
    //         alloc_old_checks();

    //     /*
    //      * kps hack - we can't use Check_by_index because it might return
    //      * NULL since ind can here be >= World.NumChecks.
    //      */
    //     check = &World.checks[ind];
    //     if (World_contains_clpos(check->pos))
    //     {
    //         warn("Map contains too many '%c' checkpoints.", 'A' + ind);
    //         return NO_IND;
    //     }

    //     check->pos = pos;
    //     World.NumChecks++;
    //     return ind;
    // }

    // ind = World.NumChecks;
    // t.pos = pos;
    // STORE(check_t, World.checks, World.NumChecks, World.MaxChecks, t);
    // return ind;

    // TODO
    return -1;
}

int World_place_item_concentrator(clpos_t pos)
{
    item_concentrator_t t;
    int ind = Num_itemConcs();

    t.pos = pos;
    World.itemConcs.push_back(t);

    return ind;
}

int World_place_asteroid_concentrator(clpos_t pos)
{
    asteroid_concentrator_t t;
    int ind = Num_asteroidConcs();

    t.pos = pos;
    World.asteroidConcs.push_back(t);

    return ind;
}

int World_place_grav(clpos_t pos, double force, int type)
{
    grav_t t;
    int ind = Num_gravs();

    t.pos = pos;
    t.blk_pos = Clpos_to_blkpos(pos);
    t.force = force;
    World.gravs.push_back(t);

    return ind;
}

int World_place_friction_area(clpos_t pos, double fric)
{
    friction_area_t t;
    int ind = Num_frictionAreas();
    t.pos = pos;
    t.friction_setting = fric;
    World.frictionAreas.push_back(t);
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
    warn("World_init");

    int i;

    memset(&World, 0, sizeof(world_t));

    for (i = 0; i < MAX_TEAMS; i++)
        Team_by_index(i)->SwapperId = NO_ID;

    Filled_wire_init();

    return 0;
}

void World_free(void)
{
    XFREE(World.block);
    XFREE(World.itemID);
    XFREE(World.gravity);
    World.asteroidConcs.clear();
    World.bases.clear();
    World.cannons.clear();
    World.ecms.clear();
    World.fuels.clear();
    World.frictionAreas.clear();
    World.gravs.clear();
    World.itemConcs.clear();
    World.targets.clear();
    World.transporters.clear();
    World.treasures.clear();
    XFREE(World.wormholes);
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

    assert(World.block == NULL);
    assert(World.gravity == NULL);

    World.block = (uint8_t **)
        malloc(sizeof(uint8_t *) * World.x + World.x * sizeof(uint8_t) * World.y);
    World.itemID = (uint16_t **)
        malloc(sizeof(uint16_t *) * World.x + World.x * sizeof(uint16_t) * World.y);
    World.gravity = (vector_t **)
        malloc(sizeof(vector_t *) * World.x + World.x * sizeof(vector_t) * World.y);

    if (World.block == NULL || World.itemID == NULL || World.gravity == NULL)
    {
        World_free();
        error("Couldn't allocate memory for map");
        return false;
    }

    map_pointer = World.block;
    map_line = (uint8_t *)((uint8_t **)map_pointer + World.x);
    item_pointer = World.itemID;
    item_line = (uint16_t *)((uint16_t **)item_pointer + World.x);
    grav_pointer = World.gravity;
    grav_line = (vector_t *)((vector_t **)grav_pointer + World.x);

    for (x = 0; x < World.x; x++)
    {
        *map_pointer = map_line;
        map_pointer += 1;
        map_line += World.y;
        *item_pointer = item_line;
        item_pointer += 1;
        item_line += World.y;
        *grav_pointer = grav_line;
        grav_pointer += 1;
        grav_line += World.y;
    }

    return true;
}

// static bool Xpmap_world_alloc(void)
// {
//     int x;
//     uint8_t *map_line;
//     uint8_t **map_pointer;
//     uint16_t *item_line;
//     uint16_t **item_pointer;
//     vector_t *grav_line;
//     vector_t **grav_pointer;

//     assert(World.block == NULL);
//     assert(World.gravity == NULL);

//     // if (World.block || World.gravity)
//     //     World_free();

//     World.block = (uint8_t **)
//         malloc(sizeof(uint8_t *) * World.x + World.x * sizeof(uint8_t) * World.y);
//     World.itemID = (uint16_t **)
//         malloc(sizeof(uint16_t *) * World.x + World.x * sizeof(uint16_t) * World.y);
//     World.gravity = (vector_t **)
//         malloc(sizeof(vector_t *) * World.x + World.x * sizeof(vector_t) * World.y);

//     World.wormholes = NULL;

//     if (World.block == NULL || World.itemID == NULL || World.gravity == NULL)
//     {
//         World_free();
//         error("Couldn't allocate memory for map");
//         return false;
//     }

//     map_pointer = World.block;
//     map_line = (uint8_t *)((uint8_t **)map_pointer + World.x);
//     item_pointer = World.itemID;
//     item_line = (uint16_t *)((uint16_t **)item_pointer + World.x);
//     grav_pointer = World.gravity;
//     grav_line = (vector_t *)((vector_t **)grav_pointer + World.x);

//     for (x = 0; x < World.x; x++)
//     {
//         *map_pointer = map_line;
//         map_pointer += 1;
//         map_line += World.y;
//         *item_pointer = item_line;
//         item_pointer += 1;
//         item_line += World.y;
//         *grav_pointer = grav_line;
//         grav_pointer += 1;
//         grav_line += World.y;
//     }

//     return true;
// }

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
    World.width = w;
    World.height = h;
    if (!is_polygon_map && options.extraBorder)
    {
        World.width += 2 * BLOCK_SZ;
        World.height += 2 * BLOCK_SZ;
    }
    World.pixel_hypotenuse = LENGTH(World.width, World.height);

    /* click sizes */
    World.cwidth = World.width * CLICK;
    World.cheight = World.height * CLICK;

    /* block sizes */
    World.x = (World.width - 1) / BLOCK_SZ + 1; /* !@# */
    World.y = (World.height - 1) / BLOCK_SZ + 1;
    World.diagonal = LENGTH(World.x, World.y);
    World.bwidth_floor = World.width / BLOCK_SZ;
    World.bheight_floor = World.height / BLOCK_SZ;

    return true;
}

bool Grok_map_options(void)
{
    warn("Grok_map_options ----------------->");

    if (!Grok_map_size())
    {
        warn("Grok_map_options -----------------> Grok_map_size failed");
        return false;
    }

    strlcpy(World.name, options.mapName, sizeof(World.name));
    strlcpy(World.author, options.mapAuthor, sizeof(World.author));
    strlcpy(World.dataURL, options.dataURL, sizeof(World.dataURL));

    if (!World_alloc())
    {
        warn("Grok_map_options -----------------> Grok_alloc failed");
        return false;
    }

    Set_world_rules();
    Set_world_items();
    Set_world_asteroids();

    if (BIT(World.rules->mode, TEAM_PLAY | TIMING) == (TEAM_PLAY | TIMING))
    {
        warn("Cannot teamplay while in race mode -- ignoring teamplay");
        CLR_BIT(World.rules->mode, TEAM_PLAY);
    }

    warn("Grok_map_options -----------------> RETURNING OK");

    return true;
}

static void Init_map(void);

static void Init_map(void)
{
    World.x = 256;
    World.y = 256;
    World.diagonal = (int)LENGTH(World.x, World.y);

    World.width = World.x * BLOCK_SZ;
    World.height = World.y * BLOCK_SZ;
    World.pixel_hypotenuse = (int)LENGTH(World.width, World.height);

    World.cwidth = PIXEL_TO_CLICK(World.width);
    World.cheight = PIXEL_TO_CLICK(World.height);
    World.click_hypotenuse = LENGTH(World.cwidth, World.cheight);

    World.asteroidConcs.clear();
    World.bases.clear();
    World.cannons.clear();
    World.ecms.clear();
    World.fuels.clear();
    World.frictionAreas.clear();
    World.gravs.clear();
    World.itemConcs.clear();
    World.targets.clear();
    World.transporters.clear();
    World.treasures.clear();

    World.NumWormholes = 0;
}

static void Generate_random_map(void);

/*
 * Use wildmap to generate a random map.
 */
static void Generate_random_map(void)
{
    int width, height;

    options.edgeWrap = true;
    width = World.x;
    height = World.y;

    Wildmap(width, height, World.name, World.author, &options.mapData, &width, &height);

    World.x = width;
    World.y = height;
    World.diagonal = (int)LENGTH(World.x, World.y);

    World.width = World.x * BLOCK_SZ;
    World.height = World.y * BLOCK_SZ;
    World.pixel_hypotenuse = (int)LENGTH(World.width, World.height);

    World.cwidth = PIXEL_TO_CLICK(World.width);
    World.cheight = PIXEL_TO_CLICK(World.height);
    World.click_hypotenuse = LENGTH(World.cwidth, World.cheight);
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
    warn("Grok_map: ========================== START");
    warn("Grok_map: is_polygon_map: %s", is_polygon_map ? "true" : "false");

    if (!Grok_map_options())
        return false;

    if (!is_polygon_map)
    {
        Xpmap_grok_map_data2();
        Xpmap_tags_to_internal_data2();
        Xpmap_find_map_object_teams2();

        int i, x, y, c;
        char *s;

        printf("grok map: init map\n");
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
            World.x = options.mapWidth;
            World.y = options.mapHeight;
        }
        if (options.extraBorder)
        {
            World.x += 2;
            World.y += 2;
        }
        World.diagonal = (int)LENGTH(World.x, World.y);

        World.width = World.x * BLOCK_SZ;
        World.height = World.y * BLOCK_SZ;
        World.pixel_hypotenuse = (int)LENGTH(World.width, World.height);

        World.cwidth = PIXEL_TO_CLICK(World.width);
        World.cheight = PIXEL_TO_CLICK(World.height);
        World.click_hypotenuse = LENGTH(World.cwidth, World.cheight);

        strlcpy(World.name, options.mapName, sizeof(World.name));
        strlcpy(World.author, options.mapAuthor, sizeof(World.author));

        if (!options.mapData)
        {
            warn("Generating random map");
            Generate_random_map();
            if (!options.mapData)
                return false;
        }

        // printf("grok map: alloc map\n");
        // Xpmap_world_alloc();

        x = -1;
        y = World.y - 1;

        printf("grok map: reading mapdata\n");

        Xpmap_grok_map_data();

        printf("grok map: allocate objects\n");

        Xpmap_tags_to_internal_data();

        if (BIT(World.rules->mode, TIMING))
            Find_base_order();

        D(Print_map());
    }

    if (!Verify_wormhole_consistency())
        return false;

    if (BIT(World.rules->mode, TIMING) && Num_checks() == 0)
    {
        warn("No checkpoints found while race mode (timing) was set.");
        warn("Turning off race mode.");
        CLR_BIT(World.rules->mode, TIMING);
    }

    /* kps - what are these doing here ? */
    if (options.maxRobots == -1)
        options.maxRobots = Num_bases();

    if (options.minRobots == -1)
        options.minRobots = options.maxRobots;

    if (Num_bases() <= 0)
        fatal("Map has no bases!");

    printf("World....: %s\nBases....: %d\nMapsize..: %dx%d pixels\n"
           "Team play: %s\n",
           World.name, Num_bases(), World.width, World.height,
           BIT(World.rules->mode, TEAM_PLAY) ? "on" : "off");

    if (!is_polygon_map)
        Xpmap_blocks_to_polygons();

    Compute_gravity();
    Find_base_direction();

    // Print out amount of map objects.
    printf("===========\n");
    printf("Asteroid concentrators: %d\n", Num_asteroidConcs());
    printf("Bases.................: %d\n", Num_bases());
    printf("Cannons...............: %d\n", Num_cannons());
    printf("ECMs..................: %d\n", Num_ecms());
    printf("Fuels.................: %d\n", Num_fuels());
    printf("Friction areas........: %d\n", Num_frictionAreas());
    printf("Gravs.................: %d\n", Num_gravs());
    printf("Item concentrators....: %d\n", Num_itemConcs());
    printf("Targets...............: %d\n", Num_targets());
    printf("Transporters..........: %d\n", Num_transporters());
    printf("Treasures.............: %d\n", Num_treasures());
    printf("Wormholes.............: %d\n", Num_wormholes());

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
            team = World.bases[i].team;
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
