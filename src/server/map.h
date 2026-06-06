/*
 * XPilot, a multiplayer gravity war game.  Copyright (C) 1991-2001 by
 *
 *      Bjørn Stabell
 *      Ken Ronny Schouten
 *      Bert Gijsbers
 *      Dick Balaska
 *
 * Copyright (C) 2000-2004 by
 *
 *      Uoti Urpala
 *      Kristian Söderblom
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

#include <cassert>
#include <vector>

#include "click.h"
#include "const.h"
#include "item.h"
#include "rules.h"
#include "types.h"
#include "xpmath.h"

#define SPACE 0
#define BASE 1
#define FILLED 2
#define REC_LU 3
#define REC_LD 4
#define REC_RU 5
#define REC_RD 6
#define FUEL 7
#define CANNON 8
#define CHECK 9
#define POS_GRAV 10
#define NEG_GRAV 11
#define CWISE_GRAV 12
#define ACWISE_GRAV 13
#define WORMHOLE 14
#define TREASURE 15
#define TARGET 16
#define ITEM_CONCENTRATOR 17
#define DECOR_FILLED 18
#define DECOR_LU 19
#define DECOR_LD 20
#define DECOR_RU 21
#define DECOR_RD 22
#define UP_GRAV 23
#define DOWN_GRAV 24
#define RIGHT_GRAV 25
#define LEFT_GRAV 26
#define FRICTION 27
#define ASTEROID_CONCENTRATOR 28
#define BASE_ATTRACTOR 127

#define SPACE_BIT (1 << SPACE)
#define BASE_BIT (1 << BASE)
#define FILLED_BIT (1 << FILLED)
#define REC_LU_BIT (1 << REC_LU)
#define REC_LD_BIT (1 << REC_LD)
#define REC_RU_BIT (1 << REC_RU)
#define REC_RD_BIT (1 << REC_RD)
#define FUEL_BIT (1 << FUEL)
#define CANNON_BIT (1 << CANNON)
#define CHECK_BIT (1 << CHECK)
#define POS_GRAV_BIT (1 << POS_GRAV)
#define NEG_GRAV_BIT (1 << NEG_GRAV)
#define CWISE_GRAV_BIT (1 << CWISE_GRAV)
#define ACWISE_GRAV_BIT (1 << ACWISE_GRAV)
#define WORMHOLE_BIT (1 << WORMHOLE)
#define TREASURE_BIT (1 << TREASURE)
#define TARGET_BIT (1 << TARGET)
#define ITEM_CONCENTRATOR_BIT (1 << ITEM_CONCENTRATOR)
#define DECOR_FILLED_BIT (1 << DECOR_FILLED)
#define DECOR_LU_BIT (1 << DECOR_LU)
#define DECOR_LD_BIT (1 << DECOR_LD)
#define DECOR_RU_BIT (1 << DECOR_RU)
#define DECOR_RD_BIT (1 << DECOR_RD)
#define UP_GRAV_BIT (1 << UP_GRAV)
#define DOWN_GRAV_BIT (1 << DOWN_GRAV)
#define RIGHT_GRAV_BIT (1 << RIGHT_GRAV)
#define LEFT_GRAV_BIT (1 << LEFT_GRAV)
#define FRICTION_BIT (1 << FRICTION)
#define ASTEROID_CONCENTRATOR_BIT (1 << ASTEROID_CONCENTRATOR)

#define DIR_RIGHT 0
#define DIR_UP (RES / 4)
#define DIR_LEFT (RES / 2)
#define DIR_DOWN (3 * RES / 4)

typedef struct world world_t;
extern world_t World, *world;
extern bool is_polygon_map;

typedef struct fuel
{
    blkpos_t blk_pos;
    position_t pix_pos;
    clpos_t pos;
    long fuel;
    uint32_t conn_mask;
    long last_change;
    int team;
} fuel_t;

typedef struct grav
{
    blkpos_t blk_pos;
    clpos_t pos;
    double force;
} grav_t;

typedef struct base
{
    blkpos_t blk_pos;
    clpos_t pos;
    int dir;
    int ind;
    int team;
} base_t;

typedef struct baseorder
{
    int base_idx; /* Index in world->bases[] */
    double dist;  /* Distance to first checkpoint */
} baseorder_t;

typedef struct cannon
{
    blkpos_t blk_pos;
    position_t pix_pos;
    clpos_t pos;
    int dir;
    int dead_ticks;
    uint32_t conn_mask;
    long last_change;
    int item[NUM_ITEMS];
    int damaged;
    int tractor_target_id;
    int tractor_count;
    bool tractor_is_pressor;
    uint16_t team;
    long used;
    int emergency_shield_left;
    int phasing_left;
    // short id;

    int group;

    short smartness;
    float shot_speed;
    int initial_items[NUM_ITEMS];
} cannon_t;

typedef struct check
{
    clpos_t pos;
} check_t;

typedef struct item
{
    double prob;        /* Probability [0..1] for item to appear */
    int max;            /* Max on world at a given time */
    int num;            /* Number active right now */
    int chance;         /* Chance [0..127] for this item to appear */
    double cannonprob;  /* Relative probability for item to appear */
    int min_per_pack;   /* minimum number of elements per item. */
    int max_per_pack;   /* maximum number of elements per item. */
    int initial;        /* initial number of elements per player. */
    int cannon_initial; /* initial number of elements per cannon. */
    int limit;          /* max number of elements per player/cannon. */
} item_t;

typedef struct asteroid
{
    double prob; /* Probability [0..1] for asteroid to appear */
    int max;     /* Max on world at a given time */
    int num;     /* Number active right now */
    int chance;  /* Chance [0..127] for asteroid to appear */
} asteroid_t;

typedef enum
{
    WORM_NORMAL,
    WORM_IN,
    WORM_OUT,
    WORM_FIXED
} wormtype_t;

typedef struct wormhole
{
    blkpos_t blk_pos;
    clpos_t pos;
    int lastdest;   /* last destination wormhole */
    int countdown;  /* if >0 warp to lastdest else random */
    bool temporary; /* wormhole was left by hyperjump */
    wormtype_t type;
    int lastID;
    int lastblock; /* block it occluded */
    int group;
} wormhole_t;

typedef struct treasure
{
    blkpos_t blk_pos;
    clpos_t pos;
    bool have;      /* true if this treasure has ball in it */
    int team;       /* team of this treasure */
    int destroyed;  /* how often this treasure destroyed */
    bool empty;     /* true if this treasure never had a ball */
    int ball_style; /* polystyle to use for color */
} treasure_t;

typedef struct target
{
    blkpos_t blk_pos;
    clpos_t pos;
    int team;
    int dead_ticks;
    int damage;
    uint32_t conn_mask;
    uint32_t update_mask;
    long last_change;
    int group;
} target_t;

typedef struct team
{
    int NumMembers;         /* Number of current members */
    int NumRobots;          /* Number of robot players */
    int NumBases;           /* Number of bases owned */
    int NumTreasures;       /* Number of treasures owned */
    int NumEmptyTreasures;  /* Number of empty treasures owned */
    int TreasuresDestroyed; /* Number of destroyed treasures */
    int TreasuresLeft;      /* Number of treasures left */
    int SwapperId;          /* Player swapping to this full team */
} team_t;

typedef struct item_concentrator
{
    clpos_t pos;
} item_concentrator_t;

typedef struct asteroid_concentrator
{
    clpos_t pos;
} asteroid_concentrator_t;

typedef struct friction_area
{
    clpos_t pos;
    double friction_setting; /* Setting from map */
    double friction;         /* Changes with gameSpeed */
    int group;
} friction_area_t;

#define MAX_PLAYER_ECMS 8 /* Maximum simultaneous per player */
typedef struct
{
    double size;
    clpos_t pos;
    int id;
} ecm_t;

/*
 * Transporter info.
 */
typedef struct
{
    clpos_t pos;
    int victim_id;
    int id;
    double count;
} transporter_t;

extern bool is_polygon_map;

struct world
{
    int x, y;                /* Size of world in blocks, rounded up */
    int bwidth_floor;        /* Width of world in blocks, rounded down */
    int bheight_floor;       /* Height of world in blocks, rounded down */
    double diagonal;         /* Diagonal length in blocks */
    int width, height;       /* Size of world in pixels (optimization) */
    int pixel_hypotenuse;    /* Diagonal length in pixels (optimization) */
    int cwidth, cheight;     /* Size of world in clicks */
    double click_hypotenuse; /* Diagonal length in clicks (optimization) */

    rules_t *rules;
    char name[MAX_CHARS];
    char author[MAX_CHARS];
    char dataURL[MAX_CHARS];

    uint8_t **block; /* type of item in each block */

    /* index into mapobject depending on value of corresponding block,
    ** -1 for space, walls, etc */
    uint16_t **itemID;

    vector_t **gravity;

    item_t items[NUM_ITEMS];

    asteroid_t asteroids;

    team_t teams[MAX_TEAMS];

    int NumTeamBases; /* How many 'different' teams are allowed */
    baseorder_t *baseorder;

    // int NumChecks;
    // ipos_t checks[OLD_MAX_CHECKS];

    std::vector<check_t> checks;

    // int NumAsteroidConcs;
    // asteroid_concentrator_t *asteroidConcs;
    std::vector<asteroid_concentrator_t> asteroidConcs;

    int NumBases;
    base_t *bases;
    // std::vector<base_t> bases;

    // int NumCannons;
    // cannon_t *cannons;
    std::vector<cannon_t> cannons;

    int NumEcms;
    ecm_t *ecms;

    // int NumFuels;
    // fuel_t *fuels;
    std::vector<fuel_t> fuels;

    // int NumFrictionAreas;
    // friction_area_t *frictionAreas;
    std::vector<friction_area_t> frictionAreas;

    // int NumGravs;
    // grav_t *gravs;
    std::vector<grav_t> gravs;

    // int NumItemConcentrators;
    // item_concentrator_t *itemConcentrators;
    std::vector<item_concentrator_t> itemConcentrators;

    // int NumTargets;
    // target_t *targets;
    std::vector<target_t> targets;

    int NumTransporters;
    transporter_t *transporters;
    // int NumTreasures;
    // treasure_t *treasures;
    std::vector<treasure_t> treasures;

    int NumWormholes;
    wormhole_t *wormholes;

    bool have_options;
};

static inline void World_set_block(blkpos_t blk, int type)
{
    assert(!(blk.bx < 0 || blk.bx >= world->x || blk.by < 0 || blk.by >= world->y));
    world->block[blk.bx][blk.by] = type;
}

static inline int World_get_block(blkpos_t blk)
{
    assert(!(blk.bx < 0 || blk.bx >= world->x || blk.by < 0 || blk.by >= world->y));
    return world->block[blk.bx][blk.by];
}

static inline bool World_contains_clpos(clpos_t pos)
{
    if (pos.cx < 0 || pos.cx >= world->cwidth)
        return false;
    if (pos.cy < 0 || pos.cy >= world->cheight)
        return false;
    return true;
}

static inline bool World_contains_clicks(int cx, int cy)
{
    if (cx < 0 || cx >= world->cwidth)
        return false;
    if (cy < 0 || cy >= world->cheight)
        return false;
    return true;
}

static inline clpos_t World_get_random_clpos(void)
{
    clpos_t pos;

    pos.cx = (int)(rfrac() * world->cwidth);
    pos.cy = (int)(rfrac() * world->cheight);

    return pos;
}

static inline int World_wrap_xclick(int cx)
{
    while (cx < 0)
        cx += world->cwidth;
    while (cx >= world->cwidth)
        cx -= world->cwidth;

    return cx;
}

static inline int World_wrap_yclick(int cy)
{
    while (cy < 0)
        cy += world->cheight;
    while (cy >= world->cheight)
        cy -= world->cheight;

    return cy;
}

static inline clpos_t World_wrap_clpos(clpos_t pos)
{
    pos.cx = World_wrap_xclick(pos.cx);
    pos.cy = World_wrap_yclick(pos.cy);

    return pos;
}

/*
 * Two macros for edge wrap of x and y coordinates measured in clicks.
 * Note that the correction needed should never be bigger than the size of the map.
 */
// #define WRAP_XCLICK(x_)                      \
//     (BIT(World.rules->mode, WRAP_PLAY)       \
//          ? ((x_) < 0                         \
//                 ? (x_) + World.cwidth        \
//                 : ((x_) >= World.cwidth      \
//                        ? (x_) - World.cwidth \
//                        : (x_)))              \
//          : (x_))

// #define WRAP_YCLICK(y_)                       \
//     (BIT(World.rules->mode, WRAP_PLAY)        \
//          ? ((y_) < 0                          \
//                 ? (y_) + World.cheight        \
//                 : ((y_) >= World.cheight      \
//                        ? (y_) - World.cheight \
//                        : (y_)))               \
//          : (y_))

/*
 * Two inline function for edge wrap of x and y coordinates measured
 * in clicks.
 *
 * Note that even when wrap play is off, ships will wrap around the map
 * if there is not walls that hinder it.
 */
static inline int WRAP_XCLICK(int cx)
{
    // TODO: Check WRAP_PLAY ?
    return World_wrap_xclick(cx);
}

static inline int WRAP_YCLICK(int cy)
{
    // TODO: Check WRAP_PLAY ?
    return World_wrap_yclick(cy);
}

/*
 * Two macros for edge wrap of differences in position.
 * If the absolute value of a difference is bigger than
 * half the map size then it is wrapped.
 */
#define WRAP_DCX(dcx)                           \
    (BIT(world->rules->mode, WRAP_PLAY)         \
         ? ((dcx) < -(world->cwidth >> 1)       \
                ? (dcx) + world->cwidth         \
                : ((dcx) > (world->cwidth >> 1) \
                       ? (dcx) - world->cwidth  \
                       : (dcx)))                \
         : (dcx))

#define WRAP_DCY(dcy)                            \
    (BIT(world->rules->mode, WRAP_PLAY)          \
         ? ((dcy) < -(world->cheight >> 1)       \
                ? (dcy) + world->cheight         \
                : ((dcy) > (world->cheight >> 1) \
                       ? (dcy) - world->cheight  \
                       : (dcy)))                 \
         : (dcy))

#define TWRAP_XCLICK(x_) \
    ((x_) > 0 ? (x_) % world->cwidth : ((x_) % world->cwidth + world->cwidth))

#define TWRAP_YCLICK(y_) \
    ((y_) > 0 ? (y_) % world->cheight : ((y_) % world->cheight + world->cheight))

#define CENTER_XCLICK(X) \
    (((X) < -(world->cwidth >> 1)) ? (X) + world->cwidth : (((X) >= (world->cwidth >> 1)) ? (X) - world->cwidth : (X)))

#define CENTER_YCLICK(X) \
    (((X) < -(world->cheight >> 1)) ? (X) + world->cheight : (((X) >= (world->cheight >> 1)) ? (X) - world->cheight : (X)))

// #define Num_asteroidConcs() (world->NumAsteroidConcs)
static inline int Num_asteroidConcs()
{
    return world->asteroidConcs.size();
}

#define Num_bases() (world->NumBases)
// static inline int Num_bases()
// {
//     return Num_bases();
// }

// #define Num_cannons() (world->NumCannons)
static inline int Num_cannons()
{
    return world->cannons.size();
}

#define Num_ecms() (world->NumEcms)

// #define Num_frictionAreas() (world->NumFrictionAreas)
static inline int Num_frictionAreas()
{
    return world->frictionAreas.size();
}

// #define Num_fuels() (world->NumFuels)
static inline int Num_fuels()
{
    return world->fuels.size();
}

// #define Num_gravs() (world->NumGravs)
static inline int Num_gravs()
{
    return world->gravs.size();
}

// #define Num_itemConcs() (world->NumItemConcentrators)
static inline int Num_itemConcs()
{
    return world->itemConcentrators.size();
}

// #define Num_targets() (world->NumTargets)
static inline int Num_targets()
{
    return world->targets.size();
}

#define Num_transporters() (world->NumTransporters)

// #define Num_treasures() (world->NumTreasures)
static inline int Num_treasures()
{
    return world->treasures.size();
}

#define Num_wormholes() (world->NumWormholes)

static inline int Num_checks()
{
    return world->checks.size();
}

#define AsteroidConc_by_index(i) ((asteroid_concentrator_t *)(&world->asteroidConcs[i]))
#define Base_by_index(i) ((base_t *)(&world->bases[i]))
#define Cannon_by_index(i) ((cannon_t *)(&world->cannons[i]))
#define Ecm_by_index(i) ((ecm_t *)(&world->ecms[i]))
#define FrictionArea_by_index(i) ((friction_area_t *)(&world->frictionAreas[i]))
#define Fuel_by_index(i) ((fuel_t *)(&world->fuels[i]))
#define Grav_by_index(i) ((grav_t *)(&world->gravs[i]))
#define ItemConc_by_index(i) ((item_concentrator_t *)(&world->itemConcentrators[i]))
// #define Target_by_index(i) ((target_t *)(&world->targets[i]))
static target_t *Target_by_index(int i)
{
    return &world->targets[i];
}
#define Treasure_by_index(i) ((treasure_t *)(&world->treasures[i]))
#define Wormhole_by_index(i) ((wormhole_t *)(&world->wormholes[i]))
#define Transporter_by_index(i) ((transporter_t *)(&world->transporters[i]))

static inline check_t *Check_by_index(int i)
{
    return &world->checks[i];
}

/*
 * Here the index is the team number.
 */
static inline team_t *Team_by_index(int ind)
{
    if (ind >= 0 && ind < MAX_TEAMS)
        return &world->teams[ind];
    return NULL;
}
