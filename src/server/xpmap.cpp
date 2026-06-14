/*
 * XPilot, a multiplayer gravity war game.  Copyright (C) 1991-2001 by
 *
 *      Bjørn Stabell
 *      Ken Ronny Schouten
 *      Bert Gijsbers
 *      Dick Balaska
 *
 * Copyright (C) 2003-2004 Kristian Söderblom
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

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

#include "commonproto.h"

#include "cannon.h"

#define SERVER
#include "bit.h"
#include "commonmacros.h"
#include "const.h"
#include "setup.h"
#include "xpconfig.h"
#include "xperror.h"
#include "xpmap.h"

#include "map.h"
#include "polygon.h"
#include "server.h"
#include "wormhole.h"

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

static int Compress_map(uint8_t *map, size_t size);

static void Xpmap_treasure_to_polygon(int treasure_ind);
static void Xpmap_target_to_polygon(int target_ind);
static void Xpmap_cannon_to_polygon(int cannon_ind);
static void Xpmap_wormhole_to_polygon(int wormhole_ind);
static void Xpmap_friction_area_to_polygon(int fa_ind);

static bool compress_maps = true;

static void Map_extra_error(int line_num)
{
    static int prev_line_num, error_count;
    const int max_error = 5;

    if (line_num > prev_line_num)
    {
        prev_line_num = line_num;
        if (++error_count <= max_error)
        {
            printf("Map file contains extranous characters on line %d\n",
                   line_num);
        }
        else if (error_count - max_error == 1)
        {
            printf("And so on...\n");
        }
    }
}

static void Xpmap_extra_error(int line_num)
{
    static int prev_line_num, error_count;
    const int max_error = 5;

    if (line_num > prev_line_num)
    {
        prev_line_num = line_num;
        if (++error_count <= max_error)
            warn("Map file contains extraneous characters on line %d",
                 line_num);
        else if (error_count - max_error == 1)
            warn("And so on...");
    }
}

static void Xpmap_missing_error(int line_num)
{
    static int prev_line_num, error_count;
    const int max_error = 5;

    if (line_num > prev_line_num)
    {
        prev_line_num = line_num;
        if (++error_count <= max_error)
            warn("Not enough map data on map data line %d", line_num);
        else if (error_count - max_error == 1)
            warn("And so on...");
    }
}

/*
 * Compress the map data using a simple Run Length Encoding algorithm.
 * If there is more than one consecutive byte with the same type
 * then we set the high bit of the byte and then the next byte
 * gives the number of repetitions.
 * This works well for most maps which have lots of series of the
 * same map object and is simple enough to got implemented quickly.
 */
static int Compress_map(uint8_t *map, size_t size)
{
    int i, j, k;

    for (i = j = 0; i < (int)size; i++, j++)
    {
        if (i + 1 < (int)size && map[i] == map[i + 1])
        {
            for (k = 2; i + k < (int)size; k++)
            {
                if (map[i] != map[i + k])
                    break;
                if (k == 255)
                    break;
            }
            map[j] = (map[i] | SETUP_COMPRESSED);
            map[++j] = k;
            i += k - 1;
        }
        else
            map[j] = map[i];
    }
    return j;
}

void Create_blockmap_from_polygons(void)
{
    int i, h, type;
    blkpos_t blk;
    clpos_t pos;
    shape_t r_wire, u_wire, l_wire, d_wire;
    clpos_t r_coords[3], u_coords[3], l_coords[3], d_coords[3];

    r_wire.num_points = 3;
    u_wire.num_points = 3;
    l_wire.num_points = 3;
    d_wire.num_points = 3;

    for (i = 0; i < 3; i++)
        r_wire.pts[i] = &r_coords[i];
    for (i = 0; i < 3; i++)
        u_wire.pts[i] = &u_coords[i];
    for (i = 0; i < 3; i++)
        l_wire.pts[i] = &l_coords[i];
    for (i = 0; i < 3; i++)
        d_wire.pts[i] = &d_coords[i];

    /*
     * Block is divided to 4 parts, r, u, l and d, the middle of the block
     * looking like this. Each char represents a square click. The middle
     * of the block is at 'x', square clicks with 'x' or ' ' are considered
     * not part of any of the block parts r, u, l or d.
     *
     *  uuu
     * l U r
     * lLxRr
     * l D r
     *  ddd
     *
     */

    h = BLOCK_CLICKS / 2 - 2;

    /* right part of block */
    r_coords[0].cx = 0;
    r_coords[0].cy = 0; /* this is the R position in the block */
    r_coords[1].cx = h;
    r_coords[1].cy = -h;
    r_coords[2].cx = h;
    r_coords[2].cy = h;

    /* up part of block */
    u_coords[0].cx = 0;
    u_coords[0].cy = 0;
    u_coords[1].cx = h;
    u_coords[1].cy = h;
    u_coords[2].cx = -h;
    u_coords[2].cy = h;

    /* left part of block */
    l_coords[0].cx = 0;
    l_coords[0].cy = 0;
    l_coords[1].cx = -h;
    l_coords[1].cy = h;
    l_coords[2].cx = -h;
    l_coords[2].cy = -h;

    /* down part of block */
    d_coords[0].cx = 0;
    d_coords[0].cy = 0;
    d_coords[1].cx = -h;
    d_coords[1].cy = -h;
    d_coords[2].cx = h;
    d_coords[2].cy = -h;

    /*
     * Create blocks out of polygons.
     */
    for (blk.by = 0; blk.by < world->y; blk.by++)
        for (blk.bx = 0; blk.bx < world->x; blk.bx++)
            World_set_block(blk, SPACE);

    for (blk.by = 0; blk.by < world->bheight_floor; blk.by++)
    {
        for (blk.bx = 0; blk.bx < world->bwidth_floor; blk.bx++)
        {
            int num_inside = 0;
            bool r_inside = false, u_inside = false;
            bool l_inside = false, d_inside = false;

            pos = Block_get_center_clpos(blk);

            if (shape_is_inside(pos.cx + 1, pos.cy, 0, NULL, &r_wire, 0) == 0)
            {
                r_inside = true;
                num_inside++;
            }
            if (shape_is_inside(pos.cx, pos.cy + 1, 0, NULL, &u_wire, 0) == 0)
            {
                u_inside = true;
                num_inside++;
            }
            if (shape_is_inside(pos.cx - 1, pos.cy, 0, NULL, &l_wire, 0) == 0)
            {
                l_inside = true;
                num_inside++;
            }
            if (shape_is_inside(pos.cx, pos.cy - 1, 0, NULL, &d_wire, 0) == 0)
            {
                d_inside = true;
                num_inside++;
            }

            if (num_inside > 2)
                World_set_block(blk, FILLED);

            if (num_inside == 2)
            {
                if (r_inside && u_inside)
                    World_set_block(blk, REC_RU);
                if (u_inside && l_inside)
                    World_set_block(blk, REC_LU);
                if (l_inside && d_inside)
                    World_set_block(blk, REC_LD);
                if (d_inside && r_inside)
                    World_set_block(blk, REC_RD);
                if (u_inside && d_inside)
                    World_set_block(blk, FILLED);
                if (r_inside && l_inside)
                    World_set_block(blk, FILLED);
            }

            if (num_inside == 1)
            {
                if (r_inside)
                    World_set_block(blk, REC_RU);
                if (u_inside)
                    World_set_block(blk, REC_LU);
                if (l_inside)
                    World_set_block(blk, REC_LD);
                if (d_inside)
                    World_set_block(blk, REC_RD);
            }
        }
    }

    /*
     * Create blocks out of map objects. Note that some of these
     * may be in the same block, which might cause a client error.
     */
    for (i = 0; i < Num_fuels(); i++)
    {
        fuel_t *fs = Fuel_by_index(i);

        blk = Clpos_to_blkpos(fs->pos);
        World_set_block(blk, FUEL);
    }

    for (i = 0; i < Num_asteroidConcs(); i++)
    {
        asteroid_concentrator_t *aconc = AsteroidConc_by_index(i);

        blk = Clpos_to_blkpos(aconc->pos);
        World_set_block(blk, ASTEROID_CONCENTRATOR);
    }

    for (i = 0; i < Num_itemConcs(); i++)
    {
        item_concentrator_t *iconc = ItemConc_by_index(i);

        blk = Clpos_to_blkpos(iconc->pos);
        World_set_block(blk, ITEM_CONCENTRATOR);
    }

    for (i = 0; i < Num_wormholes(); i++)
    {
        wormhole_t *wh = Wormhole_by_index(i);

        blk = Clpos_to_blkpos(wh->pos);
        World_set_block(blk, WORMHOLE);
    }

    /* find balltargets */
    for (blk.by = 0; blk.by < world->bheight_floor; blk.by++)
    {
        for (blk.bx = 0; blk.bx < world->bwidth_floor; blk.bx++)
        {
            int group;
            group_t *gp;

            pos = Block_get_center_clpos(blk);
            group = shape_is_inside(pos.cx, pos.cy,
                                    BALL_BIT, NULL, &filled_wire, 0);
            if (group == NO_GROUP || group == 0)
                continue;
            gp = groupptr_by_id(group);
            if (gp == NULL)
                continue;
            if (gp->type == TREASURE && gp->hitmask == NONBALL_BIT)
                World_set_block(blk, TREASURE);
        }
    }

    /*
     * Handle bases. Note that on polygon maps there can be several
     * bases on the area of a block. To handle this so that old clients
     * won't get "Bad homebase index" error, we put excess bases somewhere
     * else than on the same block.
     */

    /*
     * First mark all blocks having a base.
     * We use a base attractor for this.
     */
    for (i = 0; i < Num_bases(); i++)
    {
        base_t *base = Base_by_index(i);

        blk = Clpos_to_blkpos(base->pos);
        type = World_get_block(blk);

        /* don't put the base on top of a fuel or treasure */
        if (type == FUEL || type == TREASURE)
            continue;
        World_set_block(blk, BASE_ATTRACTOR);
    }

    /*
     * Put bases where there are base attractors or somewhere else
     * if the block already has some other important type.
     */
    for (i = 0; i < Num_bases(); i++)
    {
        base_t *base = Base_by_index(i);
        bool done;

        blk = Clpos_to_blkpos(base->pos);
        type = World_get_block(blk);
        done = false;

        if (type == FUEL || type == TREASURE || type == BASE)
        {
            /*
             * The block where the base should be put already has some
             * important type. We need to put this base somewhere else.
             * Let's just line up excess bases close to the origin of the map.
             */
            for (blk.by = 0; blk.by < world->y; blk.by++)
            {
                for (blk.bx = 0; blk.bx < world->x; blk.bx++)
                {
                    type = World_get_block(blk);
                    /*
                     * Check for base attractor here too because we might
                     * have marked this block in the earlier loop over all
                     * bases.
                     */
                    if (type == FUEL || type == TREASURE || type == BASE || type == BASE_ATTRACTOR)
                        continue;
                    /* put base attractor here so that assert is happy */
                    type = BASE_ATTRACTOR;
                    World_set_block(blk, type);
                    done = true;
                    break;
                }
                if (done)
                    break;
            }

            /* this probably doesn't happen very often */
            if (!done)
                fatal("Create_blockmap_from_polygons:\n"
                      "Couldn't find any place on map to put base on "
                      "(%d, %d).",
                      base->pos.cx, base->pos.cy);
        }

        assert(type == BASE_ATTRACTOR);
        World_set_block(blk, BASE);
    }
}

setup_t *Xpmap_init_setup(void)
{
    int i, x, y, team, type = -1, dir, wtype;
    int wormhole_i = 0, treasure_i = 0, target_i = 0, base_i = 0, cannon_i = 0;
    uint8_t *mapdata, *mapptr;
    size_t size, numblocks;
    setup_t *setup;

    numblocks = world->x * world->y;
    if ((mapdata = XMALLOC(uint8_t, numblocks)) == NULL)
    {
        error("No memory for mapdata");
        return NULL;
    }
    memset(mapdata, SETUP_SPACE, numblocks);
    mapptr = mapdata;
    errno = 0;
    for (x = 0; x < world->x; x++)
    {
        for (y = 0; y < world->y; y++, mapptr++)
        {
            type = world->block[x][y];
            switch (type)
            {
            case ACWISE_GRAV:
            case CWISE_GRAV:
            case POS_GRAV:
            case NEG_GRAV:
            case UP_GRAV:
            case DOWN_GRAV:
            case RIGHT_GRAV:
            case LEFT_GRAV:
                if (!options.gravityVisible)
                    type = SPACE;
                break;
            case WORMHOLE:
                if (!options.wormholeVisible)
                    type = SPACE;
                break;
            case ITEM_CONCENTRATOR:
                if (!options.itemConcentratorVisible)
                    type = SPACE;
                break;
            case ASTEROID_CONCENTRATOR:
                if (!options.asteroidConcentratorVisible)
                    type = SPACE;
                break;
            case FRICTION:
                if (!options.blockFrictionVisible)
                    type = SPACE;
                else
                    type = DECOR_FILLED;
                break;
            default:
                break;
            }
            switch (type)
            {
            case SPACE:
                *mapptr = SETUP_SPACE;
                break;
            case FILLED:
                *mapptr = SETUP_FILLED;
                break;
            case REC_RU:
                *mapptr = SETUP_REC_RU;
                break;
            case REC_RD:
                *mapptr = SETUP_REC_RD;
                break;
            case REC_LU:
                *mapptr = SETUP_REC_LU;
                break;
            case REC_LD:
                *mapptr = SETUP_REC_LD;
                break;
            case FUEL:
                *mapptr = SETUP_FUEL;
                break;
            case ACWISE_GRAV:
                *mapptr = SETUP_ACWISE_GRAV;
                break;
            case CWISE_GRAV:
                *mapptr = SETUP_CWISE_GRAV;
                break;
            case POS_GRAV:
                *mapptr = SETUP_POS_GRAV;
                break;
            case NEG_GRAV:
                *mapptr = SETUP_NEG_GRAV;
                break;
            case UP_GRAV:
                *mapptr = SETUP_UP_GRAV;
                break;
            case DOWN_GRAV:
                *mapptr = SETUP_DOWN_GRAV;
                break;
            case RIGHT_GRAV:
                *mapptr = SETUP_RIGHT_GRAV;
                break;
            case LEFT_GRAV:
                *mapptr = SETUP_LEFT_GRAV;
                break;
            case ITEM_CONCENTRATOR:
                *mapptr = SETUP_ITEM_CONCENTRATOR;
                break;

            case ASTEROID_CONCENTRATOR:
                *mapptr = SETUP_ASTEROID_CONCENTRATOR;
                break;

            case DECOR_FILLED:
                *mapptr = SETUP_DECOR_FILLED;
                break;
            case DECOR_RU:
                *mapptr = SETUP_DECOR_RU;
                break;
            case DECOR_RD:
                *mapptr = SETUP_DECOR_RD;
                break;
            case DECOR_LU:
                *mapptr = SETUP_DECOR_LU;
                break;
            case DECOR_LD:
                *mapptr = SETUP_DECOR_LD;
                break;

            case WORMHOLE:
                if (wormhole_i >= Num_wormholes())
                {
                    /*
                     * This can happen on an xp2 map if the block mapdata
                     * contains more wormholes than is specified in the
                     * xml data.
                     */
                    warn("Too many wormholes in block mapdata.");
                    *mapptr = SETUP_SPACE;
                    break;
                }
                wtype = Wormhole_by_index(wormhole_i)->type;
                wormhole_i++;
                switch (wtype)
                {
                case WORM_NORMAL:
                case WORM_FIXED:
                    *mapptr = SETUP_WORM_NORMAL;
                    break;
                case WORM_IN:
                    *mapptr = SETUP_WORM_IN;
                    break;
                case WORM_OUT:
                    *mapptr = SETUP_WORM_OUT;
                    break;
                default:
                    warn("Bad wormhole (%d,%d).", x, y);
                    *mapptr = SETUP_SPACE;
                    break;
                }
                break;

            case TREASURE:
                if (treasure_i >= Num_treasures())
                {
                    warn("Too many treasures in block mapdata.");
                    *mapptr = SETUP_SPACE;
                    break;
                }
                team = Treasure_by_index(treasure_i)->team;
                treasure_i++;
                if (team == TEAM_NOT_SET)
                    team = 0;
                *mapptr = SETUP_TREASURE + team;
                break;

            case TARGET:
                if (target_i >= Num_targets())
                {
                    warn("Too many targets in block mapdata.");
                    *mapptr = SETUP_SPACE;
                    break;
                }
                team = Target_by_index(target_i)->team;
                target_i++;
                if (team == TEAM_NOT_SET)
                    team = 0;
                *mapptr = SETUP_TARGET + team;
                break;

            case BASE:
                if (base_i >= Num_bases())
                {
                    warn("Too many bases in block mapdata.");
                    *mapptr = SETUP_SPACE;
                    break;
                }
                team = Base_by_index(base_i)->team;
                if (team == TEAM_NOT_SET)
                    team = 0;
                dir = Base_by_index(base_i)->dir;
                base_i++;
                /* other code should take care of this */
                assert(dir >= 0);
                assert(dir < RES);
                /* round to nearest direction */
                dir = (((dir + (RES / 8)) / (RES / 4)) * (RES / 4)) % RES;
                assert(dir == DIR_UP || dir == DIR_RIGHT || dir == DIR_DOWN || dir == DIR_LEFT);
                switch (dir)
                {
                case DIR_UP:
                    *mapptr = SETUP_BASE_UP + team;
                    break;
                case DIR_RIGHT:
                    *mapptr = SETUP_BASE_RIGHT + team;
                    break;
                case DIR_DOWN:
                    *mapptr = SETUP_BASE_DOWN + team;
                    break;
                case DIR_LEFT:
                    *mapptr = SETUP_BASE_LEFT + team;
                    break;
                default:
                    /* should never happen */
                    warn("Bad base at (%d,%d), (dir = %d).", x, y, dir);
                    *mapptr = SETUP_BASE_UP + team;
                    break;
                }
                break;

            case CANNON:
                if (cannon_i >= Num_cannons())
                {
                    warn("Too many cannons in block mapdata.");
                    *mapptr = SETUP_SPACE;
                    break;
                }
                dir = Cannon_by_index(cannon_i)->dir;
                cannon_i++;
                switch (dir)
                {
                case DIR_UP:
                    *mapptr = SETUP_CANNON_UP;
                    break;
                case DIR_RIGHT:
                    *mapptr = SETUP_CANNON_RIGHT;
                    break;
                case DIR_DOWN:
                    *mapptr = SETUP_CANNON_DOWN;
                    break;
                case DIR_LEFT:
                    *mapptr = SETUP_CANNON_LEFT;
                    break;
                default:
                    warn("Bad cannon at (%d,%d), (dir = %d).", x, y, dir);
                    *mapptr = SETUP_CANNON_UP;
                    break;
                }
                break;

            case CHECK:
                for (i = 0; i < Num_checks(); i++)
                {
                    check_t *check = Check_by_index(i);
                    blkpos_t bpos = Clpos_to_blkpos(check->pos);

                    if (x != bpos.bx || y != bpos.by)
                        continue;
                    *mapptr = SETUP_CHECK + i;
                    break;
                }
                if (i >= Num_checks())
                {
                    warn("Bad checkpoint at (%d,%d).", x, y);
                    *mapptr = SETUP_SPACE;
                    break;
                }
                break;

            default:
                warn("Unknown map type (%d) at (%d,%d).", type, x, y);
                *mapptr = SETUP_SPACE;
                break;
            }
        }
    }
    if (!compress_maps)
    {
        type = SETUP_MAP_UNCOMPRESSED;
        size = numblocks;
    }
    else
    {
        type = SETUP_MAP_ORDER_XY;
        size = Compress_map(mapdata, numblocks);
        if (size <= 0 || size > numblocks)
        {
            warn("Map compression error (%d)", size);
            free(mapdata);
            return NULL;
        }
        if ((mapdata = XREALLOC(uint8_t, mapdata, size)) == NULL)
        {
            error("Cannot reallocate mapdata");
            return NULL;
        }
    }

    if (type != SETUP_MAP_UNCOMPRESSED)
        printf("%s Block map compression ratio is %-4.2f%%\n",
               showtime(), 100.0 * size / numblocks);

    if ((setup = (setup_t *)malloc(sizeof(setup_t) + size)) == NULL)
    {
        error("No memory to hold oldsetup");
        free(mapdata);
        return NULL;
    }
    memset(setup, 0, sizeof(setup_t) + size);
    memcpy(setup->map_data, mapdata, size);
    free(mapdata);
    setup->setup_size = ((char *)&setup->map_data[0] - (char *)setup) + size;
    setup->map_data_len = size;
    setup->map_order = type;
    setup->frames_per_second = FPS; // TODO: Remove?
    setup->lives = world->rules->lives;
    setup->mode = world->rules->mode;
    setup->x = world->x;
    setup->y = world->y;
    strlcpy(setup->name, world->name, sizeof(setup->name));
    strlcpy(setup->author, world->author, sizeof(setup->author));

    return setup;
}

/*
 * Grok block based map data.
 *
 * Create world->block using options.mapData.
 * Free options.mapData.
 */
void Xpmap_grok_map_data(void)
{
    int i, x, y, c;
    char *s;

    x = -1;
    y = world->y - 1;

    s = options.mapData;
    while (y >= 0)
    {

        x++;

        if (options.extraBorder && (x == 0 || x == world->x - 1 || y == 0 || y == world->y - 1))
        {
            if (x >= world->x)
            {
                x = -1;
                y--;
                continue;
            }
            else
                /* make extra border of solid rock */
                c = XPMAP_FILLED;
        }
        else
        {
            c = *s;
            if (c == '\0' || c == EOF)
            {
                if (x < world->x)
                {
                    /* not enough map data on this line */
                    Xpmap_missing_error(world->y - y);
                    c = XPMAP_SPACE;
                }
                else
                    c = '\n';
            }
            else
            {
                if (c == '\n' && x < world->x)
                {
                    /* not enough map data on this line */
                    Xpmap_missing_error(world->y - y);
                    c = XPMAP_SPACE;
                }
                else
                    s++;
            }
        }
        if (x >= world->x || c == '\n')
        {
            y--;
            x = -1;
            if (c != '\n')
            { /* Get rest of line */
                Xpmap_extra_error(world->y - y);
                while (c != '\n' && c != EOF)
                    c = *s++;
            }
            continue;
        }

        world->block[x][y] = c;
    }

    XFREE(options.mapData);
}

/*
 * Determining which team these belong to is done later,
 * in Find_closest_team().
 */
static void Xpmap_place_cannon(blkpos_t blk, int dir)
{
    clpos_t pos;
    int ind;

    switch (dir)
    {
    case DIR_UP:
        pos.cx = (click_t)((blk.bx + 0.5) * BLOCK_CLICKS);
        pos.cy = (click_t)((blk.by + 0.333) * BLOCK_CLICKS);
        break;
    case DIR_LEFT:
        pos.cx = (click_t)((blk.bx + 0.667) * BLOCK_CLICKS);
        pos.cy = (click_t)((blk.by + 0.5) * BLOCK_CLICKS);
        break;
    case DIR_RIGHT:
        pos.cx = (click_t)((blk.bx + 0.333) * BLOCK_CLICKS);
        pos.cy = (click_t)((blk.by + 0.5) * BLOCK_CLICKS);
        break;
    case DIR_DOWN:
        pos.cx = (click_t)((blk.bx + 0.5) * BLOCK_CLICKS);
        pos.cy = (click_t)((blk.by + 0.667) * BLOCK_CLICKS);
        break;
    default:
        /* can't happen */
        assert(0 && "Unknown cannon direction.");
        break;
    }

    World_set_block(blk, CANNON);
    ind = World_place_cannon(pos, dir, TEAM_NOT_SET);
    Cannon_init(Cannon_by_index(ind));
}

/*
 * The direction of the base should be so that it points
 * up with respect to the gravity in the region.  This
 * is fixed in Find_base_dir() when the gravity has
 * been computed.
 */
static void Xpmap_place_base(blkpos_t blk, int team)
{
    World_set_block(blk, BASE);
    World_place_base(Block_get_center_clpos(blk), DIR_UP, team, 0);
}

static void Xpmap_place_fuel(blkpos_t blk)
{
    World_set_block(blk, FUEL);
    World_place_fuel(Block_get_center_clpos(blk), TEAM_NOT_SET);
}

static void Xpmap_place_treasure(blkpos_t blk, bool empty)
{
    World_set_block(blk, TREASURE);
    World_place_treasure(Block_get_center_clpos(blk),
                         TEAM_NOT_SET, empty, 0xff);
}

static void Xpmap_place_wormhole(blkpos_t blk, wormtype_t type)
{
    World_set_block(blk, WORMHOLE);
    World_place_wormhole(Block_get_center_clpos(blk), type);
}

static void Xpmap_place_target(blkpos_t blk)
{
    World_set_block(blk, TARGET);
    World_place_target(Block_get_center_clpos(blk), TEAM_NOT_SET);
}

static void Xpmap_place_check(blkpos_t blk, int ind)
{
    if (!BIT(world->rules->mode, TIMING))
    {
        World_set_block(blk, SPACE);
        return;
    }

    World_set_block(blk, CHECK);
    World_place_check(Block_get_center_clpos(blk), ind);
}

static void Xpmap_place_item_concentrator(blkpos_t blk)
{
    World_set_block(blk, ITEM_CONCENTRATOR);
    World_place_item_concentrator(Block_get_center_clpos(blk));
}

static void Xpmap_place_asteroid_concentrator(blkpos_t blk)
{
    World_set_block(blk, ASTEROID_CONCENTRATOR);
    World_place_asteroid_concentrator(Block_get_center_clpos(blk));
}

static void Xpmap_place_grav(blkpos_t blk,
                             double force, int type)
{
    World_set_block(blk, type);
    World_place_grav(Block_get_center_clpos(blk), force, type);
}

static void Xpmap_place_friction_area(blkpos_t blk)
{
    World_set_block(blk, FRICTION);
    World_place_friction_area(Block_get_center_clpos(blk),
                              options.blockFriction);
}

static void Xpmap_place_block(blkpos_t blk, int type)
{
    World_set_block(blk, type);
}

void Xpmap_tags_to_internal_data(void)
{
    int i, x, y, c;
    char *s;

    // TODO
    // error("WARNING: map has no bases!");

    for (i = 0; i < MAX_TEAMS; i++)
    {
        world->teams[i].NumMembers = 0;
        world->teams[i].NumRobots = 0;
        world->teams[i].NumBases = 0;
        world->teams[i].NumTreasures = 0;
        world->teams[i].NumEmptyTreasures = 0;
        world->teams[i].TreasuresDestroyed = 0;
        world->teams[i].TreasuresLeft = 0;
    }

    /*
     * Change read tags to internal data, create objects
     */
    {
        int worm_in = 0,
            worm_out = 0,
            worm_norm = 0;

        for (x = 0; x < world->x; x++)
        {
            // uint8_t *line = world->block[x];
            // uint16_t *itemID = world->itemID[x];

            for (y = 0; y < world->y; y++)
            {
                // char c = line[y];
                char c = world->block[x][y];
                clpos_t pos;
                pos.cx = (x + 0.5) * BLOCK_CLICKS;
                pos.cy = (y + 0.5) * BLOCK_CLICKS;
                blkpos_t blk = Clpos_to_blkpos(pos);

                world->itemID[x][y] = (uint16_t)-1;

                // Default: space
                World_set_block(blk, SPACE);

                switch (c)
                {
                case XPMAP_SPACE:
                case XPMAP_SPACE_ALT:
                default:
                    Xpmap_place_block(blk, SPACE);
                    break;

                case XPMAP_FILLED:
                    Xpmap_place_block(blk, FILLED);
                    break;
                case XPMAP_REC_LU:
                    Xpmap_place_block(blk, REC_LU);
                    break;
                case XPMAP_REC_RU:
                    Xpmap_place_block(blk, REC_RU);
                    break;
                case XPMAP_REC_LD:
                    Xpmap_place_block(blk, REC_LD);
                    break;
                case XPMAP_REC_RD:
                    Xpmap_place_block(blk, REC_RD);
                    break;

                case XPMAP_CANNON_UP:
                    world->itemID[x][y] = world->cannons.size();
                    Xpmap_place_cannon(blk, DIR_UP);
                    break;
                case XPMAP_CANNON_LEFT:
                    world->itemID[x][y] = world->cannons.size();
                    Xpmap_place_cannon(blk, DIR_LEFT);
                    break;
                case XPMAP_CANNON_RIGHT:
                    world->itemID[x][y] = world->cannons.size();
                    Xpmap_place_cannon(blk, DIR_RIGHT);
                    break;
                case XPMAP_CANNON_DOWN:
                    world->itemID[x][y] = world->cannons.size();
                    Xpmap_place_cannon(blk, DIR_DOWN);
                    break;

                case XPMAP_FUEL:
                    world->block[x][y] = FUEL;
                    world->itemID[x][y] = world->fuels.size();
                    World_place_fuel(pos, TEAM_NOT_SET);
                    break;

                case XPMAP_TREASURE:
                case XPMAP_EMPTY_TREASURE:
                    world->block[x][y] = TREASURE;
                    world->itemID[x][y] = world->treasures.size();
                    // line[y] = TREASURE;
                    // itemID[y] = world->NumTreasures;
                    // world->treasures[world->NumTreasures].blk_pos.bx = x;
                    // world->treasures[world->NumTreasures].blk_pos.by = y;
                    // world->treasures[world->NumTreasures].pos.cx = cx;
                    // world->treasures[world->NumTreasures].pos.cy = (y * BLOCK_CLICKS) + 10 * PIXEL_CLICKS;
                    // world->treasures[world->NumTreasures].have = false;
                    // world->treasures[world->NumTreasures].destroyed = 0;
                    // world->treasures[world->NumTreasures].empty = (c == '^');
                    // /*
                    //  * Determining which team it belongs to is done later,
                    //  * in Find_closest_team().
                    //  */
                    // world->treasures[world->NumTreasures].team = 0;
                    // world->NumTreasures++;
                    pos.cx = (x + 0.5) * BLOCK_CLICKS;
                    pos.cy = (y * BLOCK_CLICKS) + 10 * PIXEL_CLICKS;
                    // bool empty = (c == '^');
                    World_place_treasure(pos, 0, (c == '^'), 0xff);
                    break;
                case XPMAP_TARGET:
                    world->block[x][y] = TARGET;
                    world->itemID[x][y] = world->targets.size();
                    // world->targets[world->NumTargets].blk_pos.bx = x;
                    // world->targets[world->NumTargets].blk_pos.by = y;
                    // world->targets[world->NumTargets].pos.cx = cx;
                    // world->targets[world->NumTargets].pos.cy = cy;
                    // /*
                    //  * Determining which team it belongs to is done later,
                    //  * in Find_closest_team().
                    //  */
                    // world->targets[world->NumTargets].team = 0;
                    // world->targets[world->NumTargets].dead_ticks = 0;
                    // world->targets[world->NumTargets].damage = TARGET_DAMAGE;
                    // world->targets[world->NumTargets].conn_mask = (unsigned)-1;
                    // world->targets[world->NumTargets].update_mask = 0;
                    // world->targets[world->NumTargets].last_change = frame_loops;
                    // world->NumTargets++;
                    // World_place_target(pos, 0);
                    // pos.cx = x * BLOCK_CLICKS;
                    // pos.cy = y * BLOCK_CLICKS;
                    Xpmap_place_target(blk);
                    break;
                case XPMAP_ITEM_CONCENTRATOR:
                    Xpmap_place_item_concentrator(blk);
                    break;
                case XPMAP_ASTEROID_CONCENTRATOR:
                    Xpmap_place_asteroid_concentrator(blk);
                    break;
                case XPMAP_BASE_ATTRACTOR:
                    Xpmap_place_block(blk, BASE_ATTRACTOR);
                    break;
                case XPMAP_BASE:
                    Xpmap_place_base(blk, TEAM_NOT_SET);
                    break;
                case XPMAP_BASE_TEAM_0:
                case XPMAP_BASE_TEAM_1:
                case XPMAP_BASE_TEAM_2:
                case XPMAP_BASE_TEAM_3:
                case XPMAP_BASE_TEAM_4:
                case XPMAP_BASE_TEAM_5:
                case XPMAP_BASE_TEAM_6:
                case XPMAP_BASE_TEAM_7:
                case XPMAP_BASE_TEAM_8:
                case XPMAP_BASE_TEAM_9:
                    Xpmap_place_base(blk, (int)(c - XPMAP_BASE_TEAM_0));
                    break;

                case XPMAP_POS_GRAV:
                    Xpmap_place_grav(blk, -GRAVS_POWER, POS_GRAV);
                    break;
                case XPMAP_NEG_GRAV:
                    Xpmap_place_grav(blk, GRAVS_POWER, NEG_GRAV);
                    break;
                case XPMAP_CWISE_GRAV:
                    Xpmap_place_grav(blk, GRAVS_POWER, CWISE_GRAV);
                    break;
                case XPMAP_ACWISE_GRAV:
                    Xpmap_place_grav(blk, -GRAVS_POWER, ACWISE_GRAV);
                    break;
                case XPMAP_UP_GRAV:
                    Xpmap_place_grav(blk, GRAVS_POWER, UP_GRAV);
                    break;
                case XPMAP_DOWN_GRAV:
                    Xpmap_place_grav(blk, -GRAVS_POWER, DOWN_GRAV);
                    break;
                case XPMAP_RIGHT_GRAV:
                    Xpmap_place_grav(blk, GRAVS_POWER, RIGHT_GRAV);
                    break;
                case XPMAP_LEFT_GRAV:
                    Xpmap_place_grav(blk, -GRAVS_POWER, LEFT_GRAV);
                    break;

                case XPMAP_WORMHOLE_NORMAL:
                    // Xpmap_place_wormhole(blk, WORM_NORMAL);
                    // world->itemID[x][y] = Num_wormholes();
                    // worm_norm++;
                    // break;
                case XPMAP_WORMHOLE_IN:
                    // Xpmap_place_wormhole(blk, WORM_IN);
                    // world->itemID[x][y] = Num_wormholes();
                    // worm_in++;
                    // break;
                case XPMAP_WORMHOLE_OUT:
                    // Xpmap_place_wormhole(blk, WORM_OUT);
                    // world->itemID[x][y] = Num_wormholes();
                    // worm_out++;
                    // break;

                    // world->block[x][y] = WORMHOLE;
                    // world->itemID[x][y] = world->NumWormholes;
                    // world->wormholes[world->NumWormholes].blk_pos = Clpos_to_blkpos(pos);
                    // world->wormholes[world->NumWormholes].pos = pos;
                    // world->wormholes[world->NumWormholes].countdown = 0;
                    // world->wormholes[world->NumWormholes].lastdest = -1;
                    // world->wormholes[world->NumWormholes].temporary = 0;
                    // world->wormholes[world->NumWormholes].lastblock = SPACE;
                    // world->wormholes[world->NumWormholes].lastID = -1;
                    // if (c == '@')
                    // {
                    //     world->wormholes[world->NumWormholes].type = WORM_NORMAL;
                    //     worm_norm++;
                    // }
                    // else if (c == '(')
                    // {
                    //     world->wormholes[world->NumWormholes].type = WORM_IN;
                    //     worm_in++;
                    // }
                    // else
                    // {
                    //     world->wormholes[world->NumWormholes].type = WORM_OUT;
                    //     worm_out++;
                    // }
                    // world->NumWormholes++;

                    // if (c == '@')
                    // {
                    //     World_place_wormhole(pos, WORM_NORMAL);
                    //     worm_norm++;
                    // }
                    // else if (c == '(')
                    // {
                    //     World_place_wormhole(pos, WORM_IN);
                    //     worm_in++;
                    // }
                    // else
                    // {
                    //     World_place_wormhole(pos, WORM_OUT);
                    //     worm_out++;
                    // }

                    break;

                case XPMAP_CHECK_0:
                case XPMAP_CHECK_1:
                case XPMAP_CHECK_2:
                case XPMAP_CHECK_3:
                case XPMAP_CHECK_4:
                case XPMAP_CHECK_5:
                case XPMAP_CHECK_6:
                case XPMAP_CHECK_7:
                case XPMAP_CHECK_8:
                case XPMAP_CHECK_9:
                case XPMAP_CHECK_10:
                case XPMAP_CHECK_11:
                case XPMAP_CHECK_12:
                case XPMAP_CHECK_13:
                case XPMAP_CHECK_14:
                case XPMAP_CHECK_15:
                case XPMAP_CHECK_16:
                case XPMAP_CHECK_17:
                case XPMAP_CHECK_18:
                case XPMAP_CHECK_19:
                case XPMAP_CHECK_20:
                case XPMAP_CHECK_21:
                case XPMAP_CHECK_22:
                case XPMAP_CHECK_23:
                case XPMAP_CHECK_24:
                case XPMAP_CHECK_25:
                    // if (BIT(world->rules->mode, TIMING))
                    // {
                    //     world->checks[c - 'A'].x = x;
                    //     world->checks[c - 'A'].y = y;
                    //     line[y] = CHECK;
                    // }
                    // else
                    // {
                    //     line[y] = SPACE;
                    // }
                    break;

                case XPMAP_FRICTION_AREA:
                    Xpmap_place_friction_area(blk);
                    break;

                case XPMAP_DECOR_FILLED:
                    Xpmap_place_block(blk, DECOR_FILLED);
                    break;
                case XPMAP_DECOR_LU:
                    Xpmap_place_block(blk, DECOR_LU);
                    break;
                case XPMAP_DECOR_RU:
                    Xpmap_place_block(blk, DECOR_RU);
                    break;
                case XPMAP_DECOR_LD:
                    Xpmap_place_block(blk, DECOR_LD);
                    break;
                case XPMAP_DECOR_RD:
                    Xpmap_place_block(blk, DECOR_RD);
                    break;
                }
            }
        }

        // printf("grok map: wormhole hacks\n");
        // /*
        //  * Verify that the wormholes are consistent, i.e. that if
        //  * we have no 'out' wormholes, make sure that we don't have
        //  * any 'in' wormholes, and (less critical) if we have no 'in'
        //  * wormholes, make sure that we don't have any 'out' wormholes.
        //  */
        // if ((worm_norm) ? (worm_norm + worm_out < 2)
        //     : (worm_in) ? (worm_out < 1)
        //                 : (worm_out > 0))
        // {

        //     int i;

        //     printf("Inconsistent use of wormholes, removing them.\n");
        //     for (i = 0; i < Num_wormholes(); i++)
        //     {
        //         world->block
        //             [world->wormholes[i].blk_pos.bx]
        //             [world->wormholes[i].blk_pos.by] = SPACE;
        //         world->itemID
        //             [world->wormholes[i].blk_pos.bx]
        //             [world->wormholes[i].blk_pos.by] = (uint16_t)-1;
        //     }
        //     world->NumWormholes = 0;
        // }

        // if (!options.wormTime)
        // {
        //     for (i = 0; i < Num_wormholes(); i++)
        //     {
        //         int j = (int)(rfrac() * Num_wormholes());
        //         while (world->wormholes[j].type == WORM_IN)
        //             j = (int)(rfrac() * Num_wormholes());
        //         world->wormholes[i].lastdest = j;
        //         // printf("Wormhole %d type is %d\n", i, world->wormholes[i].type);
        //     }
        // }

        if (BIT(world->rules->mode, TIMING) && Num_checks() == 0)
        {
            printf("No checkpoints found while race mode (timing) was set.\n");
            printf("Turning off race mode.\n");
            CLR_BIT(world->rules->mode, TIMING);
        }

        printf("grok map: teamplay hacks\n");
        /*
         * Determine which team a treasure belongs to.
         */
        if (BIT(world->rules->mode, TEAM_PLAY))
        {
            uint16_t team = TEAM_NOT_SET;
            for (i = 0; i < Num_treasures(); i++)
            {
                team = Find_closest_team(world->treasures[i].pos);
                world->treasures[i].team = team;
                if (team == TEAM_NOT_SET)
                {
                    error("Couldn't find a matching team for the treasure.");
                }
                else
                {
                    world->teams[team].NumTreasures++;
                    if (!world->treasures[i].empty)
                        world->teams[team].TreasuresLeft++;
                    else
                        world->teams[team].NumEmptyTreasures++;
                }
            }
            for (i = 0; i < Num_targets(); i++)
            {
                team = Find_closest_team(world->targets[i].pos);
                if (team == TEAM_NOT_SET)
                {
                    error("Couldn't find a matching team for the target.");
                }
                world->targets[i].team = team;
            }
            if (options.teamCannons)
            {
                for (i = 0; i < Num_cannons(); i++)
                {
                    team = Find_closest_team(world->cannons[i].pos);
                    if (team == TEAM_NOT_SET)
                    {
                        error("Couldn't find a matching team for the cannon.");
                    }
                    world->cannons[i].team = team;
                }
            }
            for (i = 0; i < Num_fuels(); i++)
            {
                team = Find_closest_team(world->fuels[i].pos);
                if (team == TEAM_NOT_SET)
                {
                    error("Couldn't find a matching team for fuelstation.");
                }
                world->fuels[i].team = team;
            }
        }
    }
}

void Xpmap_find_map_object_teams(void)
{
    int i;
    clpos_t pos = {0, 0};

    if (!BIT(world->rules->mode, TEAM_PLAY))
        return;

    /* This could return -1 */
    if (Find_closest_team(pos) == TEAM_NOT_SET)
    {
        warn("Broken map: Couldn't find teams for map objects.");
        return;
    }

    /*
     * Determine which team a stuff belongs to.
     */
    for (i = 0; i < Num_treasures(); i++)
    {
        treasure_t *treasure = Treasure_by_index(i);
        team_t *teamp;

        treasure->team = Find_closest_team(treasure->pos);
        teamp = Team_by_index(treasure->team);
        // assert(teamp != NULL);

        teamp->NumTreasures++;
        if (treasure->empty)
            teamp->NumEmptyTreasures++;
        else
            teamp->TreasuresLeft++;
    }

    for (i = 0; i < Num_targets(); i++)
    {
        target_t *targ = Target_by_index(i);

        targ->team = Find_closest_team(targ->pos);
    }

    if (options.teamCannons)
    {
        for (i = 0; i < Num_cannons(); i++)
        {
            cannon_t *cannon = Cannon_by_index(i);

            cannon->team = Find_closest_team(cannon->pos);
        }
    }

    for (i = 0; i < Num_fuels(); i++)
    {
        fuel_t *fs = Fuel_by_index(i);

        fs->team = Find_closest_team(fs->pos);
    }
}

/*
 * Find the correct direction of the base, according to the gravity in
 * the base region.
 *
 * If a base attractor is adjacent to a base then the base will point
 * to the attractor.
 */
void Xpmap_find_base_direction(void)
{
    int i;
    blkpos_t blk;

    for (i = 0; i < Num_bases(); i++)
    {
        base_t *base = Base_by_index(i);
        int x, y, dir, att;
        vector_t gravity = {0.0, 0.0};
        // vector_t gravity = World_gravity(base->pos);

        if (gravity.x == 0.0 && gravity.y == 0.0)
            /*
             * Undefined direction
             * Should be set to direction of gravity!
             */
            dir = DIR_UP;
        else
        {
            double a = findDir(-gravity.x, -gravity.y);
            dir = MOD2((int)(a + 0.5), RES);
            dir = ((dir + RES / 8) / (RES / 4)) * (RES / 4); /* round it */
            dir = MOD2(dir, RES);
        }
        att = -1;

        x = CLICK_TO_BLOCK(base->pos.cx);
        y = CLICK_TO_BLOCK(base->pos.cy);

        /* First check upwards attractor */
        if (y == world->y - 1 && world->block[x][0] == BASE_ATTRACTOR && BIT(world->rules->mode, WRAP_PLAY))
        {
            if (att == -1 || dir == DIR_UP)
                att = DIR_UP;
        }
        if (y < world->y - 1 && world->block[x][y + 1] == BASE_ATTRACTOR)
        {
            if (att == -1 || dir == DIR_UP)
                att = DIR_UP;
        }

        /* then downwards */
        if (y == 0 && world->block[x][world->y - 1] == BASE_ATTRACTOR && BIT(world->rules->mode, WRAP_PLAY))
        {
            if (att == -1 || dir == DIR_DOWN)
                att = DIR_DOWN;
        }
        if (y > 0 && world->block[x][y - 1] == BASE_ATTRACTOR)
        {
            if (att == -1 || dir == DIR_DOWN)
                att = DIR_DOWN;
        }
        /* then rightwards */
        if (x == world->x - 1 && world->block[0][y] == BASE_ATTRACTOR && BIT(world->rules->mode, WRAP_PLAY))
        {
            if (att == -1 || dir == DIR_RIGHT)
                att = DIR_RIGHT;
        }
        if (x < world->x - 1 && world->block[x + 1][y] == BASE_ATTRACTOR)
        {
            if (att == -1 || dir == DIR_RIGHT)
                att = DIR_RIGHT;
        }
        /* then leftwards */
        if (x == 0 && world->block[world->x - 1][y] == BASE_ATTRACTOR && BIT(world->rules->mode, WRAP_PLAY))
        {
            if (att == -1 || dir == DIR_LEFT)
                att = DIR_LEFT;
        }
        if (x > 0 && world->block[x - 1][y] == BASE_ATTRACTOR)
        {
            if (att == -1 || dir == DIR_LEFT)
                att = DIR_LEFT;
        }

        if (att != -1)
            dir = att;
        base->dir = dir;
    }
    for (blk.bx = 0; blk.bx < world->x; blk.bx++)
    {
        for (blk.by = 0; blk.by < world->y; blk.by++)
        {
            if (World_get_block(blk) == BASE_ATTRACTOR)
                World_set_block(blk, SPACE);
        }
    }
}

/*
 * The following functions is for converting the block based map data
 * to polygons.
 */

/* number of vertices in polygon */
#define N (2 + 12)
static void Xpmap_treasure_to_polygon(int treasure_ind)
{
    int cx, cy, i, r, n;
    double angle;
    int polystyle, edgestyle;
    treasure_t *treasure = Treasure_by_index(treasure_ind);
    clpos_t pos[N + 1];

    polystyle = P_get_poly_id("treasure_ps");
    edgestyle = P_get_edge_id("treasure_es");

    cx = treasure->pos.cx - BLOCK_CLICKS / 2;
    cy = treasure->pos.cy - BLOCK_CLICKS / 2;

    pos[0].cx = cx;
    pos[0].cy = cy;
    pos[1].cx = cx + (BLOCK_CLICKS - 1);
    pos[1].cy = cy;

    cx = treasure->pos.cx;
    cy = treasure->pos.cy;
    /* -1 is to ensure the vertices are inside the block */
    r = (BLOCK_CLICKS / 2) - 1;
    /* number of points in half circle */
    n = N - 2;

    for (i = 0; i < n; i++)
    {
        angle = (((double)i) / (n - 1)) * PI;
        pos[i + 2].cx = (click_t)(cx + r * cos(angle));
        pos[i + 2].cy = (click_t)(cy + r * sin(angle));
    }

    pos[N] = pos[0];

    /* create balltarget */
    P_start_balltarget(treasure->team, treasure_ind);
    P_start_polygon(pos[0], polystyle);
    for (i = 1; i <= N; i++)
        P_vertex(pos[i], edgestyle);
    P_end_polygon();
    P_end_balltarget();

    /* create ballarea */
    P_start_ballarea();
    P_start_polygon(pos[0], polystyle);
    for (i = 1; i <= N; i++)
        P_vertex(pos[i], edgestyle);
    P_end_polygon();
    P_end_ballarea();
}
#undef N

static void Xpmap_block_polygon(clpos_t bpos, int polystyle, int edgestyle,
                                int destroyed_style)
{
    clpos_t pos[5];
    int i;

    bpos.cx = CLICK_TO_BLOCK(bpos.cx) * BLOCK_CLICKS;
    bpos.cy = CLICK_TO_BLOCK(bpos.cy) * BLOCK_CLICKS;

    pos[0].cx = bpos.cx;
    pos[0].cy = bpos.cy;
    pos[1].cx = bpos.cx + (BLOCK_CLICKS - 1);
    pos[1].cy = bpos.cy;
    pos[2].cx = bpos.cx + (BLOCK_CLICKS - 1);
    pos[2].cy = bpos.cy + (BLOCK_CLICKS - 1);
    pos[3].cx = bpos.cx;
    pos[3].cy = bpos.cy + (BLOCK_CLICKS - 1);
    pos[4] = pos[0];

    P_start_polygon(pos[0], polystyle);
    if (destroyed_style >= 0)
        P_style("destroyed", destroyed_style);
    for (i = 1; i <= 4; i++)
        P_vertex(pos[i], edgestyle);
    P_end_polygon();
}

static void Xpmap_target_to_polygon(int target_ind)
{
    int ps, es, ds;
    target_t *targ = Target_by_index(target_ind);

    ps = P_get_poly_id("target_ps");
    es = P_get_edge_id("target_es");
    ds = P_get_poly_id("destroyed_ps");

    /* create target polygon */
    P_start_target(target_ind);
    Xpmap_block_polygon(targ->pos, ps, es, ds);
    P_end_target();
}

static void Xpmap_cannon_polygon(cannon_t *cannon,
                                 int polystyle, int edgestyle)
{
    clpos_t pos[4], cpos = cannon->pos;
    int i, ds;

    pos[0] = cannon->pos;

    cpos.cx = CLICK_TO_BLOCK(cpos.cx) * BLOCK_CLICKS;
    cpos.cy = CLICK_TO_BLOCK(cpos.cy) * BLOCK_CLICKS;

    switch (cannon->dir)
    {
    case DIR_RIGHT:
        pos[1].cx = cpos.cx;
        pos[1].cy = cpos.cy + (BLOCK_CLICKS - 1);
        pos[2].cx = cpos.cx;
        pos[2].cy = cpos.cy;
        break;
    case DIR_UP:
        pos[1].cx = cpos.cx;
        pos[1].cy = cpos.cy;
        pos[2].cx = cpos.cx + (BLOCK_CLICKS - 1);
        pos[2].cy = cpos.cy;
        break;
    case DIR_LEFT:
        pos[1].cx = cpos.cx + (BLOCK_CLICKS - 1);
        pos[1].cy = cpos.cy;
        pos[2].cx = cpos.cx + (BLOCK_CLICKS - 1);
        pos[2].cy = cpos.cy + (BLOCK_CLICKS - 1);
        break;
    case DIR_DOWN:
        pos[1].cx = cpos.cx + (BLOCK_CLICKS - 1);
        pos[1].cy = cpos.cy + (BLOCK_CLICKS - 1);
        pos[2].cx = cpos.cx;
        pos[2].cy = cpos.cy + (BLOCK_CLICKS - 1);
        break;
    default:
        /* can't happen */
        assert(0 && "Unknown cannon direction.");
        break;
    }
    pos[3] = pos[0];

    ds = P_get_poly_id("destroyed_ps");
    P_start_polygon(pos[0], polystyle);
    P_style("destroyed", ds);
    for (i = 1; i <= 3; i++)
        P_vertex(pos[i], edgestyle);
    P_end_polygon();
}

static void Xpmap_cannon_to_polygon(int cannon_ind)
{
    int ps, es;
    cannon_t *cannon = Cannon_by_index(cannon_ind);

    ps = P_get_poly_id("cannon_ps");
    es = P_get_edge_id("cannon_es");

    P_start_cannon(cannon_ind);
    Xpmap_cannon_polygon(cannon, ps, es);
    P_end_cannon();
}

#define N 12
static void Xpmap_wormhole_to_polygon(int wormhole_ind)
{
    int ps, es, i, r;
    double angle;
    wormhole_t *wormhole = Wormhole_by_index(wormhole_ind);
    clpos_t pos[N + 1], wpos;

    /* don't make a polygon for an out wormhole */
    if (wormhole->type == WORM_OUT)
        return;

    ps = P_get_poly_id("wormhole_ps");
    es = P_get_edge_id("wormhole_es");

    wpos = wormhole->pos;
    r = WORMHOLE_RADIUS;

    for (i = 0; i < N; i++)
    {
        angle = (((double)i) / N) * 2 * PI;
        pos[i].cx = (click_t)(wpos.cx + r * cos(angle));
        pos[i].cy = (click_t)(wpos.cy + r * sin(angle));
    }
    pos[N] = pos[0];

    P_start_wormhole(wormhole_ind);
    P_start_polygon(pos[0], ps);
    for (i = 1; i <= N; i++)
        P_vertex(pos[i], es);
    P_end_polygon();
    P_end_wormhole();
}

static void Xpmap_friction_area_to_polygon(int fa_ind)
{
    int ps, es;
    friction_area_t *fa = FrictionArea_by_index(fa_ind);

    ps = P_get_poly_id("fa_ps");
    es = P_get_edge_id("fa_es");

    P_start_friction_area(fa_ind);
    Xpmap_block_polygon(fa->pos, ps, es, -1);
    P_end_friction_area();
}

/*
 * Add a wall polygon
 *
 * The polygon consists of a start block and and endblock and possibly
 * some full wall/fuel blocks in between. A total number of numblocks
 * blocks are part of the polygon and must be 1 or more. If numblocks
 * is one, the startblock and endblock are the same block.
 *
 * The block coordinates of the first block is (bx, by)
 *
 * The polygon will have 3 or 4 vertices.
 *
 * Idea: first assume the polygon is a rectangle, then move
 * the vertices depending on the start and end blocks.
 *
 * The vertex index:
 * 0: upper left vertex
 * 1: lower left vertex
 * 2: lower right vertex
 * 3: upper right vertex
 * 4: upper left vertex, second time
 */
static void Xpmap_wall_poly(int bx, int by,
                            int startblock, int endblock, int numblocks,
                            int polystyle, int edgestyle)
{
    int i;
    clpos_t pos[5]; /* positions of vertices */

    if (numblocks < 1)
        return;

    /* first assume we have a rectangle */
    pos[0].cx = bx * BLOCK_CLICKS;
    pos[0].cy = (by + 1) * BLOCK_CLICKS - 1;
    pos[1].cx = bx * BLOCK_CLICKS;
    pos[1].cy = by * BLOCK_CLICKS;
    pos[2].cx = (bx + numblocks) * BLOCK_CLICKS - 1;
    pos[2].cy = by * BLOCK_CLICKS;
    pos[3].cx = (bx + numblocks) * BLOCK_CLICKS - 1;
    pos[3].cy = (by + 1) * BLOCK_CLICKS - 1;

    /* move the vertices depending on the startblock and endblock */
    switch (startblock)
    {
    case FILLED:
    case REC_LU:
    case REC_LD:
    case FUEL:
        /* no need to move the leftmost 2 vertices */
        break;
    case REC_RU:
        /* move lower left vertex to the right */
        pos[1].cx += (BLOCK_CLICKS - 1);
        break;
    case REC_RD:
        /* move upper left vertex to the right */
        pos[0].cx += (BLOCK_CLICKS - 1);
        break;
    default:
        return;
    }

    switch (endblock)
    {
    case FILLED:
    case FUEL:
    case REC_RU:
    case REC_RD:
        /* no need to move the rightmost 2 vertices */
        break;
    case REC_LU:
        pos[2].cx -= (BLOCK_CLICKS - 1);
        break;
    case REC_LD:
        pos[3].cx -= (BLOCK_CLICKS - 1);
        break;
    default:
        return;
    }

    /*
     * Since we want to form a closed loop of line segments, the
     * last vertex must equal the first.
     */
    pos[4] = pos[0];

    P_start_polygon(pos[0], polystyle);
    for (i = 1; i <= 4; i++)
        P_vertex(pos[i], edgestyle);
    P_end_polygon();
}

static void Xpmap_walls_to_polygons(void)
{
    int x, y, x0 = 0;
    int numblocks = 0;
    int inside = false;
    int startblock = 0, endblock = 0, block;
    int maxblocks = POLYGON_MAX_OFFSET / BLOCK_CLICKS;
    int ps, es;

    ps = P_get_poly_id("wall_ps");
    es = P_get_edge_id("wall_es");

    /*
     * x, FILLED = solid wall
     * s, REC_LU = wall triangle pointing left and up
     * a, REC_RU = wall triangle pointing right and up
     * w, REC_LD = wall triangle pointing left and down
     * q, REC_RD = wall triangle pointing right and down
     * #, FUEL   = fuel block
     */

    for (y = world->y - 1; y >= 0; y--)
    {
        for (x = 0; x < world->x; x++)
        {
            block = world->block[x][y];

            if (!inside)
            {
                switch (block)
                {
                case FILLED:
                case REC_RU:
                case REC_RD:
                case FUEL:
                    x0 = x;
                    startblock = endblock = block;
                    inside = true;
                    numblocks = 1;
                    break;

                case REC_LU:
                case REC_LD:
                    Xpmap_wall_poly(x, y, block, block, 1, ps, es);
                    break;
                default:
                    break;
                }
            }
            else
            {

                switch (block)
                {
                case FILLED:
                case FUEL:
                    numblocks++;
                    endblock = block;
                    break;

                case REC_RU:
                case REC_RD:
                    /* old polygon ends */
                    Xpmap_wall_poly(x0, y, startblock, endblock,
                                    numblocks, ps, es);
                    /* and a new one starts */
                    x0 = x;
                    startblock = endblock = block;
                    numblocks = 1;
                    break;

                case REC_LU:
                case REC_LD:
                    numblocks++;
                    endblock = block;
                    Xpmap_wall_poly(x0, y, startblock, endblock,
                                    numblocks, ps, es);
                    inside = false;
                    break;

                default:
                    /* none of the above, polygon ends */
                    Xpmap_wall_poly(x0, y, startblock, endblock,
                                    numblocks, ps, es);
                    inside = false;
                    break;
                }
            }

            /*
             * We don't want the polygon to have offsets that are too big.
             */
            if (inside && numblocks == maxblocks)
            {
                Xpmap_wall_poly(x0, y, startblock, endblock,
                                numblocks, ps, es);
                inside = false;
            }
        }

        /* end of row */
        if (inside)
        {
            Xpmap_wall_poly(x0, y, startblock, endblock,
                            numblocks, ps, es);
            inside = false;
        }
    }
}

void Xpmap_blocks_to_polygons(void)
{
    int i;

    /* create edgestyles and polystyles */
    P_edgestyle("wall_es", -1, 0x2244EE, 0);
    P_polystyle("wall_ps", 0x0033AA, 0, P_get_edge_id("wall_es"), 0);

    P_edgestyle("treasure_es", -1, 0xFF0000, 0);
    P_polystyle("treasure_ps", 0xFF0000, 0, P_get_edge_id("treasure_es"), 0);

    P_edgestyle("target_es", 3, 0xFF7700, 0);
    P_polystyle("target_ps", 0xFF7700, 3, P_get_edge_id("target_es"), 0);

    P_edgestyle("cannon_es", 3, 0xFFFFFF, 0);
    P_polystyle("cannon_ps", 0xFFFFFF, 2, P_get_edge_id("cannon_es"), 0);

    P_edgestyle("destroyed_es", 3, 0xFF0000, 0);
    P_polystyle("destroyed_ps", 0xFF0000, 2, P_get_edge_id("destroyed_es"),
                STYLE_INVISIBLE | STYLE_INVISIBLE_RADAR);

    P_edgestyle("wormhole_es", -1, 0x00FFFF, 0);
    P_polystyle("wormhole_ps", 0x00FFFF, 2, P_get_edge_id("wormhole_es"), 0);

    P_edgestyle("fa_es", 2, 0xFF1F00, 0);
    P_polystyle("fa_ps", 0xCF1F00, 2, P_get_edge_id("fa_es"), 0);

    Xpmap_walls_to_polygons();

    if (options.polygonMode)
        is_polygon_map = true;

    for (i = 0; i < Num_treasures(); i++)
        Xpmap_treasure_to_polygon(i);

    for (i = 0; i < Num_targets(); i++)
        Xpmap_target_to_polygon(i);

    for (i = 0; i < Num_cannons(); i++)
        Xpmap_cannon_to_polygon(i);

    for (i = 0; i < Num_wormholes(); i++)
        Xpmap_wormhole_to_polygon(i);

    for (i = 0; i < Num_frictionAreas(); i++)
        Xpmap_friction_area_to_polygon(i);

    /*printf("Created %d polygons.\n", num_polys);*/
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
    world->wormholes.clear();
}

static bool Xpmap_world_alloc(void)
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

    ccx = world->checks[0].pos.cx;
    ccy = world->checks[0].pos.cy;
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

bool Xpmap_grok_map2(void)
{
    warn("Grok_map: start");

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

    printf("grok map: alloc map\n");
    Xpmap_world_alloc();

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

    printf("grok map: reading mapdata\n");

    Xpmap_grok_map_data();

    printf("grok map: allocate objects\n");

    Xpmap_tags_to_internal_data();

    /* kps - what are these doing here ? */
    if (options.maxRobots == -1)
        options.maxRobots = Num_bases();

    if (options.minRobots == -1)
        options.minRobots = options.maxRobots;

    if (BIT(world->rules->mode, TIMING))
        Find_base_order();

    printf("World....: %s\nBases....: %d\nMapsize..: %dx%d\nTeam play: %s\n",
           world->name, Num_bases(), world->x, world->y,
           BIT(world->rules->mode, TEAM_PLAY) ? "on" : "off");

    D(Print_map());

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

    printf("grok map: returning true\n");

    return true;
}
