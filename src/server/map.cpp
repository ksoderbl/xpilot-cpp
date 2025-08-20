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

#include "strlcpy.h"

#define SERVER
#include "xpconfig.h"
#include "serverconst.h"
#include "global.h"
#include "proto.h"
#include "map.h"
#include "bit.h"
#include "xperror.h"
#include "xpmath.h"

#define GRAV_RANGE 10

/*
 * Globals.
 */
world_t World, *world = &World;

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

static void Init_map(void)
{
    world->x = 256;
    world->y = 256;
    world->diagonal = (int)LENGTH(world->x, world->y);

    world->width = world->x * BLOCK_SZ;
    world->height = world->y * BLOCK_SZ;
    world->hypotenuse = (int)LENGTH(world->width, world->height);

    world->click_width = PIXEL_TO_CLICK(world->width);
    world->click_height = PIXEL_TO_CLICK(world->height);
    world->click_hypotenuse = LENGTH(world->click_width, world->click_height);

    world->NumFuels = 0;
    world->NumBases = 0;
    world->NumGravs = 0;
    world->NumCannons = 0;
    world->NumWormholes = 0;
    world->NumTreasures = 0;
    world->NumTargets = 0;
    world->NumItemConcentrators = 0;
    world->NumAsteroidConcs = 0;
}

void Free_map(void)
{
    if (world->block)
    {
        free(world->block);
        world->block = NULL;
    }
    if (world->itemID)
    {
        free(world->itemID);
        world->itemID = NULL;
    }
    if (world->gravity)
    {
        free(world->gravity);
        world->gravity = NULL;
    }
    if (world->grav)
    {
        free(world->grav);
        world->grav = NULL;
    }
    if (world->base)
    {
        free(world->base);
        world->base = NULL;
    }
    if (world->cannon)
    {
        free(world->cannon);
        world->cannon = NULL;
    }
    if (world->fuel)
    {
        free(world->fuel);
        world->fuel = NULL;
    }
    if (world->wormHoles)
    {
        free(world->wormHoles);
        world->wormHoles = NULL;
    }
    if (world->itemConcentrators)
    {
        free(world->itemConcentrators);
        world->itemConcentrators = NULL;
    }
    if (world->asteroidConcs)
    {
        free(world->asteroidConcs);
        world->asteroidConcs = NULL;
    }
}

static void Alloc_map(void)
{
    int x;

    if (world->block || world->gravity)
        Free_map();

    world->block =
        (uint8_t **)malloc(sizeof(uint8_t *) * world->x + world->x * sizeof(uint8_t) * world->y);
    world->itemID =
        (unsigned short **)malloc(sizeof(unsigned short *) * world->x + world->x * sizeof(unsigned short) * world->y);
    world->gravity =
        (vector_t **)malloc(sizeof(vector_t *) * world->x + world->x * sizeof(vector_t) * world->y);
    world->grav = NULL;
    world->base = NULL;
    world->fuel = NULL;
    world->cannon = NULL;
    world->wormHoles = NULL;
    world->itemConcentrators = NULL;
    world->asteroidConcs = NULL;
    if (world->block == NULL || world->itemID == NULL || world->gravity == NULL)
    {
        Free_map();
        error("Couldn't allocate memory for map (%d bytes)",
              world->x * (world->y * (sizeof(uint8_t) + sizeof(vector_t)) + sizeof(vector_t *) + sizeof(uint8_t *)));
        exit(-1);
    }
    else
    {
        uint8_t *map_line;
        uint8_t **map_pointer;
        unsigned short *item_line;
        unsigned short **item_pointer;
        vector_t *grav_line;
        vector_t **grav_pointer;

        map_pointer = world->block;
        map_line = (uint8_t *)((uint8_t **)map_pointer + world->x);
        item_pointer = world->itemID;
        item_line = (unsigned short *)((unsigned short **)item_pointer + world->x);
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

static void Map_extra_error(int line_num)
{
#ifndef SILENT
    static int prev_line_num, error_count;
    const int max_error = 5;

    if (line_num > prev_line_num)
    {
        prev_line_num = line_num;
        if (++error_count <= max_error)
        {
            xpprintf("Map file contains extranous characters on line %d\n",
                     line_num);
        }
        else if (error_count - max_error == 1)
        {
            xpprintf("And so on...\n");
        }
    }
#endif
}

static void Map_missing_error(int line_num)
{
#ifndef SILENT
    static int prev_line_num, error_count;
    const int max_error = 5;

    if (line_num > prev_line_num)
    {
        prev_line_num = line_num;
        if (++error_count <= max_error)
        {
            xpprintf("Not enough map data on map data line %d\n", line_num);
        }
        else if (error_count - max_error == 1)
        {
            xpprintf("And so on...\n");
        }
    }
#endif
}

bool Grok_map(void)
{
    int i, x, y, c;
    char *s;

    Init_map();

    if (options.mapWidth <= 0 || options.mapWidth > MAX_MAP_SIZE ||
        options.mapHeight <= 0 || options.mapHeight > MAX_MAP_SIZE)
    {
        errno = 0;
        error("mapWidth or mapHeight exceeds map size limit [1, %d]",
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

    world->click_width = PIXEL_TO_CLICK(world->width);
    world->click_height = PIXEL_TO_CLICK(world->height);
    world->click_hypotenuse = LENGTH(world->click_width, world->click_height);

    strlcpy(world->name, options.mapName, sizeof(world->name));
    strlcpy(world->author, options.mapAuthor, sizeof(world->author));

    if (!options.mapData)
    {
        errno = 0;
        error("Generating random map");
        Generate_random_map();
        if (!options.mapData)
        {
            return false;
        }
    }

    Alloc_map();

    x = -1;
    y = world->y - 1;

    Set_world_rules();
    Set_world_items();
    Set_world_asteroids();

    if (BIT(world->rules->mode, TEAM_PLAY | TIMING) == (TEAM_PLAY | TIMING))
    {
        error("Cannot teamplay while in race mode -- ignoring teamplay");
        CLR_BIT(world->rules->mode, TEAM_PLAY);
    }

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
            {
                /* make extra border of solid rock */
                c = 'x';
            }
        }
        else
        {
            c = *s;
            if (c == '\0' || c == EOF)
            {
                if (x < world->x)
                {
                    /* not enough map data on this line */
                    Map_missing_error(world->y - y);
                    c = ' ';
                }
                else
                {
                    c = '\n';
                }
            }
            else
            {
                if (c == '\n' && x < world->x)
                {
                    /* not enough map data on this line */
                    Map_missing_error(world->y - y);
                    c = ' ';
                }
                else
                {
                    s++;
                }
            }
        }
        if (x >= world->x || c == '\n')
        {
            y--;
            x = -1;
            if (c != '\n')
            { /* Get rest of line */
                Map_extra_error(world->y - y);
                while (c != '\n' && c != EOF)
                {
                    c = *s++;
                }
            }
            continue;
        }

        switch (world->block[x][y] = c)
        {
        case 'r':
        case 'd':
        case 'f':
        case 'c':
            world->NumCannons++;
            break;
        case '*':
        case '^':
            world->NumTreasures++;
            break;
        case '#':
            world->NumFuels++;
            break;
        case '!':
            world->NumTargets++;
            break;
        case '%':
            world->NumItemConcentrators++;
            break;
        case '&':
            world->NumAsteroidConcs++;
            break;
        case '_':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            world->NumBases++;
            break;
        case '+':
        case '-':
        case '>':
        case '<':
        case 'i':
        case 'm':
        case 'j':
        case 'k':
            world->NumGravs++;
            break;
        case '@':
        case '(':
        case ')':
            world->NumWormholes++;
            break;
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'H':
        case 'I':
        case 'J':
        case 'K':
        case 'L':
        case 'M':
        case 'N':
        case 'O':
        case 'P':
        case 'Q':
        case 'R':
        case 'S':
        case 'T':
        case 'U':
        case 'V':
        case 'W':
        case 'X':
        case 'Y':
        case 'Z':
            if (BIT(world->rules->mode, TIMING))
                world->NumChecks++;
            break;
        default:
            break;
        }
    }

    free(options.mapData);
    options.mapData = NULL;

    /*
     * Get space for special objects.
     */
    if (world->NumCannons > 0 && (world->cannon = (cannon_t *)
                                      malloc(world->NumCannons * sizeof(cannon_t))) == NULL)
    {
        error("Out of memory - cannons");
        exit(-1);
    }
    if (world->NumFuels > 0 && (world->fuel = (fuel_t *)
                                    malloc(world->NumFuels * sizeof(fuel_t))) == NULL)
    {
        error("Out of memory - fuel depots");
        exit(-1);
    }
    if (world->NumGravs > 0 && (world->grav = (grav_t *)
                                    malloc(world->NumGravs * sizeof(grav_t))) == NULL)
    {
        error("Out of memory - gravs");
        exit(-1);
    }
    if (world->NumWormholes > 0 && (world->wormHoles = (wormhole_t *)
                                        malloc(world->NumWormholes * sizeof(wormhole_t))) == NULL)
    {
        error("Out of memory - wormholes");
        exit(-1);
    }
    if (world->NumTreasures > 0 && (world->treasures = (treasure_t *)
                                        malloc(world->NumTreasures * sizeof(treasure_t))) == NULL)
    {
        error("Out of memory - treasures");
        exit(-1);
    }
    if (world->NumTargets > 0 && (world->targets = (target_t *)
                                      malloc(world->NumTargets * sizeof(target_t))) == NULL)
    {
        error("Out of memory - targets");
        exit(-1);
    }
    if (world->NumItemConcentrators > 0 && (world->itemConcentrators = (item_concentrator_t *)
                                                malloc(world->NumItemConcentrators * sizeof(item_concentrator_t))) == NULL)
    {
        error("Out of memory - item concentrators");
        exit(-1);
    }
    if (world->NumAsteroidConcs > 0 && (world->asteroidConcs = (asteroid_concentrator_t *)
                                            malloc(world->NumAsteroidConcs * sizeof(asteroid_concentrator_t))) == NULL)
    {
        error("Out of memory - asteroid concentrators");
        exit(-1);
    }
    if (world->NumBases > 0)
    {
        if ((world->base = (base_t *)
                 malloc(world->NumBases * sizeof(base_t))) == NULL)
        {
            error("Out of memory - bases");
            exit(-1);
        }
    }
    else
    {
        error("WARNING: map has no bases!");
    }

    /*
     * Now reset all counters since we will recount everything
     * and reuse these counters while inserting the objects
     * into structures.
     */
    world->NumCannons = 0;
    world->NumFuels = 0;
    world->NumGravs = 0;
    world->NumWormholes = 0;
    world->NumTreasures = 0;
    world->NumTargets = 0;
    world->NumBases = 0;
    world->NumItemConcentrators = 0;
    world->NumAsteroidConcs = 0;

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
            uint8_t *line = world->block[x];
            unsigned short *itemID = world->itemID[x];

            for (y = 0; y < world->y; y++)
            {
                char c = line[y];
                int cx = (x + 0.5) * BLOCK_CLICKS;
                int cy = (y + 0.5) * BLOCK_CLICKS;

                itemID[y] = (unsigned short)-1;

                switch (c)
                {
                case ' ':
                case '.':
                default:
                    line[y] = SPACE;
                    break;

                case 'x':
                    line[y] = FILLED;
                    break;
                case 's':
                    line[y] = REC_LU;
                    break;
                case 'a':
                    line[y] = REC_RU;
                    break;
                case 'w':
                    line[y] = REC_LD;
                    break;
                case 'q':
                    line[y] = REC_RD;
                    break;

                case 'r':
                    line[y] = CANNON;
                    itemID[y] = world->NumCannons;
                    world->cannon[world->NumCannons].dir = DIR_UP;
                    world->cannon[world->NumCannons].blk_pos.x = x;
                    world->cannon[world->NumCannons].blk_pos.y = y;
                    world->cannon[world->NumCannons].pix_pos.x = (x + 0.5) * BLOCK_SZ;
                    world->cannon[world->NumCannons].pix_pos.y = (y + 0.333) * BLOCK_SZ;
                    world->cannon[world->NumCannons].clk_pos.cx = cx;
                    world->cannon[world->NumCannons].clk_pos.cy = (y + 0.333) * BLOCK_CLICKS;
                    world->cannon[world->NumCannons].dead_time = 0;
                    world->cannon[world->NumCannons].conn_mask = (unsigned)-1;
                    world->cannon[world->NumCannons].team = TEAM_NOT_SET;
                    Cannon_init(world->NumCannons);
                    world->NumCannons++;
                    break;
                case 'd':
                    line[y] = CANNON;
                    itemID[y] = world->NumCannons;
                    world->cannon[world->NumCannons].dir = DIR_LEFT;
                    world->cannon[world->NumCannons].blk_pos.x = x;
                    world->cannon[world->NumCannons].blk_pos.y = y;
                    world->cannon[world->NumCannons].pix_pos.x = (x + 0.667) * BLOCK_SZ;
                    world->cannon[world->NumCannons].pix_pos.y = (y + 0.5) * BLOCK_SZ;
                    world->cannon[world->NumCannons].clk_pos.cx = (x + 0.667) * BLOCK_CLICKS;
                    world->cannon[world->NumCannons].clk_pos.cy = cy;
                    world->cannon[world->NumCannons].dead_time = 0;
                    world->cannon[world->NumCannons].conn_mask = (unsigned)-1;
                    world->cannon[world->NumCannons].team = TEAM_NOT_SET;
                    Cannon_init(world->NumCannons);
                    world->NumCannons++;
                    break;
                case 'f':
                    line[y] = CANNON;
                    itemID[y] = world->NumCannons;
                    world->cannon[world->NumCannons].dir = DIR_RIGHT;
                    world->cannon[world->NumCannons].blk_pos.x = x;
                    world->cannon[world->NumCannons].blk_pos.y = y;
                    world->cannon[world->NumCannons].pix_pos.x = (x + 0.333) * BLOCK_SZ;
                    world->cannon[world->NumCannons].pix_pos.y = (y + 0.5) * BLOCK_SZ;
                    world->cannon[world->NumCannons].clk_pos.cx = (x + 0.333) * BLOCK_CLICKS;
                    world->cannon[world->NumCannons].clk_pos.cy = cy;
                    world->cannon[world->NumCannons].dead_time = 0;
                    world->cannon[world->NumCannons].conn_mask = (unsigned)-1;
                    world->cannon[world->NumCannons].team = TEAM_NOT_SET;
                    Cannon_init(world->NumCannons);
                    world->NumCannons++;
                    break;
                case 'c':
                    line[y] = CANNON;
                    itemID[y] = world->NumCannons;
                    world->cannon[world->NumCannons].dir = DIR_DOWN;
                    world->cannon[world->NumCannons].blk_pos.x = x;
                    world->cannon[world->NumCannons].blk_pos.y = y;
                    world->cannon[world->NumCannons].pix_pos.x = (x + 0.5) * BLOCK_SZ;
                    world->cannon[world->NumCannons].pix_pos.y = (y + 0.667) * BLOCK_SZ;
                    world->cannon[world->NumCannons].clk_pos.cx = cx;
                    world->cannon[world->NumCannons].clk_pos.cy = (y + 0.667) * BLOCK_CLICKS;
                    world->cannon[world->NumCannons].dead_time = 0;
                    world->cannon[world->NumCannons].conn_mask = (unsigned)-1;
                    world->cannon[world->NumCannons].team = TEAM_NOT_SET;
                    Cannon_init(world->NumCannons);
                    world->NumCannons++;
                    break;

                case '#':
                    line[y] = FUEL;
                    itemID[y] = world->NumFuels;
                    world->fuel[world->NumFuels].blk_pos.x = x;
                    world->fuel[world->NumFuels].blk_pos.y = y;
                    world->fuel[world->NumFuels].pix_pos.x = (x + 0.5) * BLOCK_SZ;
                    world->fuel[world->NumFuels].pix_pos.y = (y + 0.5) * BLOCK_SZ;
                    world->fuel[world->NumFuels].clk_pos.cx = cx;
                    world->fuel[world->NumFuels].clk_pos.cy = cy;
                    world->fuel[world->NumFuels].fuel = START_STATION_FUEL;
                    world->fuel[world->NumFuels].conn_mask = (unsigned)-1;
                    world->fuel[world->NumFuels].last_change = frame_loops;
                    world->fuel[world->NumFuels].team = TEAM_NOT_SET;
                    world->NumFuels++;
                    break;

                case '*':
                case '^':
                    line[y] = TREASURE;
                    itemID[y] = world->NumTreasures;
                    world->treasures[world->NumTreasures].blk_pos.x = x;
                    world->treasures[world->NumTreasures].blk_pos.y = y;
                    world->treasures[world->NumTreasures].clk_pos.cx = cx;
                    world->treasures[world->NumTreasures].clk_pos.cy = (y * BLOCK_CLICKS) + 10 * PIXEL_CLICKS;
                    world->treasures[world->NumTreasures].have = false;
                    world->treasures[world->NumTreasures].destroyed = 0;
                    world->treasures[world->NumTreasures].empty = (c == '^');
                    /*
                     * Determining which team it belongs to is done later,
                     * in Find_closest_team().
                     */
                    world->treasures[world->NumTreasures].team = 0;
                    world->NumTreasures++;
                    break;
                case '!':
                    line[y] = TARGET;
                    itemID[y] = world->NumTargets;
                    world->targets[world->NumTargets].blk_pos.x = x;
                    world->targets[world->NumTargets].blk_pos.y = y;
                    world->targets[world->NumTargets].clk_pos.cx = cx;
                    world->targets[world->NumTargets].clk_pos.cy = cy;
                    /*
                     * Determining which team it belongs to is done later,
                     * in Find_closest_team().
                     */
                    world->targets[world->NumTargets].team = 0;
                    world->targets[world->NumTargets].dead_time = 0;
                    world->targets[world->NumTargets].damage = TARGET_DAMAGE;
                    world->targets[world->NumTargets].conn_mask = (unsigned)-1;
                    world->targets[world->NumTargets].update_mask = 0;
                    world->targets[world->NumTargets].last_change = frame_loops;
                    world->NumTargets++;
                    break;
                case '%':
                    line[y] = ITEM_CONCENTRATOR;
                    itemID[y] = world->NumItemConcentrators;
                    world->itemConcentrators[world->NumItemConcentrators].blk_pos.x = x;
                    world->itemConcentrators[world->NumItemConcentrators].blk_pos.y = y;
                    world->itemConcentrators[world->NumItemConcentrators].clk_pos.cx = cx;
                    world->itemConcentrators[world->NumItemConcentrators].clk_pos.cy = cy;
                    world->NumItemConcentrators++;
                    break;
                case '&':
                    line[y] = ASTEROID_CONCENTRATOR;
                    itemID[y] = world->NumAsteroidConcs;
                    world->asteroidConcs[world->NumAsteroidConcs].blk_pos.x = x;
                    world->asteroidConcs[world->NumAsteroidConcs].blk_pos.y = y;
                    world->asteroidConcs[world->NumAsteroidConcs].clk_pos.cx = cx;
                    world->asteroidConcs[world->NumAsteroidConcs].clk_pos.cy = cy;
                    world->NumAsteroidConcs++;
                    break;
                case '$':
                    line[y] = BASE_ATTRACTOR;
                    break;
                case '_':
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    line[y] = BASE;
                    itemID[y] = world->NumBases;
                    world->base[world->NumBases].blk_pos.x = x;
                    world->base[world->NumBases].blk_pos.y = y;
                    world->base[world->NumBases].clk_pos.cx = cx;
                    world->base[world->NumBases].clk_pos.cy = cy;
                    /*
                     * The direction of the base should be so that it points
                     * up with respect to the gravity in the region.  This
                     * is fixed in Find_base_dir() when the gravity has
                     * been computed.
                     */
                    world->base[world->NumBases].dir = DIR_UP;
                    if (BIT(world->rules->mode, TEAM_PLAY))
                    {
                        if (c >= '0' && c <= '9')
                        {
                            world->base[world->NumBases].team = c - '0';
                        }
                        else
                        {
                            world->base[world->NumBases].team = 0;
                        }
                        world->teams[world->base[world->NumBases].team].NumBases++;
                        if (world->teams[world->base[world->NumBases].team].NumBases == 1)
                            world->NumTeamBases++;
                    }
                    else
                    {
                        world->base[world->NumBases].team = TEAM_NOT_SET;
                    }
                    world->NumBases++;
                    break;

                case '+':
                    line[y] = POS_GRAV;
                    itemID[y] = world->NumGravs;
                    world->grav[world->NumGravs].blk_pos.x = x;
                    world->grav[world->NumGravs].blk_pos.y = y;
                    world->grav[world->NumGravs].clk_pos.cx = cx;
                    world->grav[world->NumGravs].clk_pos.cy = cy;
                    world->grav[world->NumGravs].force = -GRAVS_POWER;
                    world->NumGravs++;
                    break;
                case '-':
                    line[y] = NEG_GRAV;
                    itemID[y] = world->NumGravs;
                    world->grav[world->NumGravs].blk_pos.x = x;
                    world->grav[world->NumGravs].blk_pos.y = y;
                    world->grav[world->NumGravs].clk_pos.cx = cx;
                    world->grav[world->NumGravs].clk_pos.cy = cy;
                    world->grav[world->NumGravs].force = GRAVS_POWER;
                    world->NumGravs++;
                    break;
                case '>':
                    line[y] = CWISE_GRAV;
                    itemID[y] = world->NumGravs;
                    world->grav[world->NumGravs].blk_pos.x = x;
                    world->grav[world->NumGravs].blk_pos.y = y;
                    world->grav[world->NumGravs].clk_pos.cx = cx;
                    world->grav[world->NumGravs].clk_pos.cy = cy;
                    world->grav[world->NumGravs].force = GRAVS_POWER;
                    world->NumGravs++;
                    break;
                case '<':
                    line[y] = ACWISE_GRAV;
                    itemID[y] = world->NumGravs;
                    world->grav[world->NumGravs].blk_pos.x = x;
                    world->grav[world->NumGravs].blk_pos.y = y;
                    world->grav[world->NumGravs].clk_pos.cx = cx;
                    world->grav[world->NumGravs].clk_pos.cy = cy;
                    world->grav[world->NumGravs].force = -GRAVS_POWER;
                    world->NumGravs++;
                    break;
                case 'i':
                    line[y] = UP_GRAV;
                    itemID[y] = world->NumGravs;
                    world->grav[world->NumGravs].blk_pos.x = x;
                    world->grav[world->NumGravs].blk_pos.y = y;
                    world->grav[world->NumGravs].clk_pos.cx = cx;
                    world->grav[world->NumGravs].clk_pos.cy = cy;
                    world->grav[world->NumGravs].force = GRAVS_POWER;
                    world->NumGravs++;
                    break;
                case 'm':
                    line[y] = DOWN_GRAV;
                    itemID[y] = world->NumGravs;
                    world->grav[world->NumGravs].blk_pos.x = x;
                    world->grav[world->NumGravs].blk_pos.y = y;
                    world->grav[world->NumGravs].clk_pos.cx = cx;
                    world->grav[world->NumGravs].clk_pos.cy = cy;
                    world->grav[world->NumGravs].force = -GRAVS_POWER;
                    world->NumGravs++;
                    break;
                case 'k':
                    line[y] = RIGHT_GRAV;
                    itemID[y] = world->NumGravs;
                    world->grav[world->NumGravs].blk_pos.x = x;
                    world->grav[world->NumGravs].blk_pos.y = y;
                    world->grav[world->NumGravs].clk_pos.cx = cx;
                    world->grav[world->NumGravs].clk_pos.cy = cy;
                    world->grav[world->NumGravs].force = GRAVS_POWER;
                    world->NumGravs++;
                    break;
                case 'j':
                    line[y] = LEFT_GRAV;
                    itemID[y] = world->NumGravs;
                    world->grav[world->NumGravs].blk_pos.x = x;
                    world->grav[world->NumGravs].blk_pos.y = y;
                    world->grav[world->NumGravs].clk_pos.cx = cx;
                    world->grav[world->NumGravs].clk_pos.cy = cy;
                    world->grav[world->NumGravs].force = -GRAVS_POWER;
                    world->NumGravs++;
                    break;

                case '@':
                case '(':
                case ')':
                    world->wormHoles[world->NumWormholes].blk_pos.x = x;
                    world->wormHoles[world->NumWormholes].blk_pos.y = y;
                    world->wormHoles[world->NumWormholes].clk_pos.cx = cx;
                    world->wormHoles[world->NumWormholes].clk_pos.cy = cy;
                    world->wormHoles[world->NumWormholes].countdown = 0;
                    world->wormHoles[world->NumWormholes].lastdest = -1;
                    world->wormHoles[world->NumWormholes].temporary = 0;
                    world->wormHoles[world->NumWormholes].lastblock = SPACE;
                    world->wormHoles[world->NumWormholes].lastID = -1;
                    if (c == '@')
                    {
                        world->wormHoles[world->NumWormholes].type = WORM_NORMAL;
                        worm_norm++;
                    }
                    else if (c == '(')
                    {
                        world->wormHoles[world->NumWormholes].type = WORM_IN;
                        worm_in++;
                    }
                    else
                    {
                        world->wormHoles[world->NumWormholes].type = WORM_OUT;
                        worm_out++;
                    }
                    line[y] = WORMHOLE;
                    itemID[y] = world->NumWormholes;
                    world->NumWormholes++;
                    break;

                case 'A':
                case 'B':
                case 'C':
                case 'D':
                case 'E':
                case 'F':
                case 'G':
                case 'H':
                case 'I':
                case 'J':
                case 'K':
                case 'L':
                case 'M':
                case 'N':
                case 'O':
                case 'P':
                case 'Q':
                case 'R':
                case 'S':
                case 'T':
                case 'U':
                case 'V':
                case 'W':
                case 'X':
                case 'Y':
                case 'Z':
                    if (BIT(world->rules->mode, TIMING))
                    {
                        world->check[c - 'A'].x = x;
                        world->check[c - 'A'].y = y;
                        line[y] = CHECK;
                    }
                    else
                    {
                        line[y] = SPACE;
                    }
                    break;

                case 'z':
                    line[y] = FRICTION;
                    break;

                case 'b':
                    line[y] = DECOR_FILLED;
                    break;
                case 'h':
                    line[y] = DECOR_LU;
                    break;
                case 'g':
                    line[y] = DECOR_RU;
                    break;
                case 'y':
                    line[y] = DECOR_LD;
                    break;
                case 't':
                    line[y] = DECOR_RD;
                    break;
                }
            }
        }

        /*
         * Verify that the wormholes are consistent, i.e. that if
         * we have no 'out' wormholes, make sure that we don't have
         * any 'in' wormholes, and (less critical) if we have no 'in'
         * wormholes, make sure that we don't have any 'out' wormholes.
         */
        if ((worm_norm) ? (worm_norm + worm_out < 2)
            : (worm_in) ? (worm_out < 1)
                        : (worm_out > 0))
        {

            int i;

            xpprintf("Inconsistent use of wormholes, removing them.\n");
            for (i = 0; i < world->NumWormholes; i++)
            {
                world->block
                    [world->wormHoles[i].blk_pos.x]
                    [world->wormHoles[i].blk_pos.y] = SPACE;
                world->itemID
                    [world->wormHoles[i].blk_pos.x]
                    [world->wormHoles[i].blk_pos.y] = (unsigned short)-1;
            }
            world->NumWormholes = 0;
        }

        if (!options.wormTime)
        {
            for (i = 0; i < world->NumWormholes; i++)
            {
                int j = (int)(rfrac() * world->NumWormholes);
                while (world->wormHoles[j].type == WORM_IN)
                    j = (int)(rfrac() * world->NumWormholes);
                world->wormHoles[i].lastdest = j;
            }
        }

        if (BIT(world->rules->mode, TIMING) && world->NumChecks == 0)
        {
            xpprintf("No checkpoints found while race mode (timing) was set.\n");
            xpprintf("Turning off race mode.\n");
            CLR_BIT(world->rules->mode, TIMING);
        }

        /*
         * Determine which team a treasure belongs to.
         */
        if (BIT(world->rules->mode, TEAM_PLAY))
        {
            unsigned short team = TEAM_NOT_SET;
            for (i = 0; i < world->NumTreasures; i++)
            {
                team = Find_closest_team(world->treasures[i].clk_pos.cx, world->treasures[i].clk_pos.cy);
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
            for (i = 0; i < world->NumTargets; i++)
            {
                team = Find_closest_team(world->targets[i].clk_pos.cx, world->targets[i].clk_pos.cy);
                if (team == TEAM_NOT_SET)
                {
                    error("Couldn't find a matching team for the target.");
                }
                world->targets[i].team = team;
            }
            if (options.teamCannons)
            {
                for (i = 0; i < world->NumCannons; i++)
                {
                    team = Find_closest_team(world->cannon[i].clk_pos.cx, world->cannon[i].clk_pos.cy);
                    if (team == TEAM_NOT_SET)
                    {
                        error("Couldn't find a matching team for the cannon.");
                    }
                    world->cannon[i].team = team;
                }
            }
            for (i = 0; i < world->NumFuels; i++)
            {
                team = Find_closest_team(world->fuel[i].clk_pos.cx, world->fuel[i].clk_pos.cy);
                if (team == TEAM_NOT_SET)
                {
                    error("Couldn't find a matching team for fuelstation.");
                }
                world->fuel[i].team = team;
            }
        }
    }

    if (options.maxRobots == -1)
    {
        options.maxRobots = world->NumBases;
    }
    if (options.minRobots == -1)
    {
        options.minRobots = options.maxRobots;
    }
    if (BIT(world->rules->mode, TIMING))
    {
        Find_base_order();
    }

#ifndef SILENT
    xpprintf("world->...: %s\nBases....: %d\nMapsize..: %dx%d\nTeam play: %s\n",
             world->name, world->NumBases, world->x, world->y,
             BIT(world->rules->mode, TEAM_PLAY) ? "on" : "off");
#endif

    D(Print_map());

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

    world->click_width = PIXEL_TO_CLICK(world->width);
    world->click_height = PIXEL_TO_CLICK(world->height);
    world->click_hypotenuse = LENGTH(world->click_width, world->click_height);
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

    for (i = 0; i < world->NumBases; i++)
    {
        int x = world->base[i].blk_pos.x,
            y = world->base[i].blk_pos.y,
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
        world->base[i].dir = dir;
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
 * Return the team that is closest to this position.
 */
unsigned short Find_closest_team(int cx, int cy)
{
    unsigned short team = TEAM_NOT_SET;
    int i;
    DFLOAT closest = FLT_MAX, l;

    for (i = 0; i < world->NumBases; i++)
    {
        if (world->base[i].team == TEAM_NOT_SET)
            continue;

        l = Wrap_length(CLICK_TO_FLOAT(cx - world->base[i].clk_pos.cx),
                        CLICK_TO_FLOAT(cy - world->base[i].clk_pos.cy));

        if (l < closest)
        {
            team = world->base[i].team;
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
    DFLOAT cx, cy, dist;

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

    cx = world->check[0].x * BLOCK_SZ;
    cy = world->check[0].y * BLOCK_SZ;
    for (i = 0; i < n; i++)
    {
        dist = Wrap_length(world->base[i].blk_pos.x * BLOCK_SZ - cx,
                           world->base[i].blk_pos.y * BLOCK_SZ - cy);
        for (j = 0; j < i; j++)
        {
            if (world->baseorder[j].dist > dist)
            {
                break;
            }
        }
        for (k = i - 1; k >= j; k--)
        {
            world->baseorder[k + 1] = world->baseorder[k];
        }
        world->baseorder[j].base_idx = i;
        world->baseorder[j].dist = dist;
    }
}

// TODO: add click version
DFLOAT Wrap_findDir(DFLOAT dx, DFLOAT dy)
{
    dx = WRAP_DX(dx);
    dy = WRAP_DY(dy);
    return findDir(dx, dy);
}

// TODO: add click version
DFLOAT Wrap_length(DFLOAT dx, DFLOAT dy)
{
    dx = WRAP_DX(dx);
    dy = WRAP_DY(dy);
    return LENGTH(dx, dy);
}

static void Compute_global_gravity(void)
{
    int xi, yi, dx, dy;
    DFLOAT xforce, yforce, strength;
    double theta;
    vector_t *grav;

    if (options.gravityPointSource == false)
    {
        theta = (options.gravityAngle * PI) / 180.0;
        xforce = cos(theta) * options.Gravity;
        yforce = sin(theta) * options.Gravity;
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
                strength = options.Gravity / LENGTH(dx, dy);
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
    DFLOAT force, fx, fy;
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
        gx = world->grav[g].blk_pos.x;
        gy = world->grav[g].blk_pos.y;
        force = world->grav[g].force;

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

    if ((wwhtemp = (wormhole_t *)realloc(world->wormHoles,
                                         (world->NumWormholes + 2) * sizeof(wormhole_t))) == NULL)
    {
        error("No memory for temporary wormholes.");
        return;
    }
    world->wormHoles = wwhtemp;

    inhole.blk_pos.x = xin;
    inhole.blk_pos.y = yin;
    outhole.blk_pos.x = xout;
    outhole.blk_pos.y = yout;
    inhole.countdown = outhole.countdown = options.wormTime;
    inhole.lastdest = world->NumWormholes + 1;
    inhole.temporary = outhole.temporary = 1;
    inhole.type = WORM_IN;
    outhole.type = WORM_OUT;
    inhole.lastblock = world->block[xin][yin];
    outhole.lastblock = world->block[xout][yout];
    inhole.lastID = world->itemID[xin][yin];
    outhole.lastID = world->itemID[xout][yout];
    world->wormHoles[world->NumWormholes] = inhole;
    world->wormHoles[world->NumWormholes + 1] = outhole;
    world->block[xin][yin] = world->block[xout][yout] = WORMHOLE;
    world->itemID[xin][yin] = world->NumWormholes;
    world->itemID[xout][yout] = world->NumWormholes + 1;
    world->NumWormholes += 2;
}

void remove_temp_wormhole(int ind)
{
    wormhole_t hole;

    hole = world->wormHoles[ind];
    world->block[hole.blk_pos.x][hole.blk_pos.y] = hole.lastblock;
    world->itemID[hole.blk_pos.x][hole.blk_pos.y] = hole.lastID;
    world->NumWormholes--;
    if (ind != world->NumWormholes)
    {
        world->wormHoles[ind] = world->wormHoles[world->NumWormholes];
    }
    world->wormHoles = (wormhole_t *)realloc(world->wormHoles,
                                             world->NumWormholes * sizeof(wormhole_t));
}
