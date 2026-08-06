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

#include "bit.h"
#include "click.h"
#include "const.h"
#include "item.h"
#include "types.h"
#include "commonproto.h"

#include "serverrules.h"

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
#define DIR_UP (ANGLE_RESOLUTION / 4)
#define DIR_LEFT (ANGLE_RESOLUTION / 2)
#define DIR_DOWN (3 * ANGLE_RESOLUTION / 4)

typedef struct world world_t;
extern world_t World;
extern bool is_polygon_map;

typedef struct fuel
{
    clpos_t pos;
    double fuel;
    uint32_t conn_mask;
    long last_change;
    int team;
} fuel_t;

typedef struct grav
{
    clpos_t pos;
    double force;
    int type;
} grav_t;

typedef struct base
{
    clpos_t pos;
    int dir;
    int ind;
    int team;
    int order;
    int initial_items[NUM_ITEMS];
} base_t;

typedef struct baseorder
{
    int base_idx; /* Index in World.bases[] */
    double dist;  /* Distance to first checkpoint */
} baseorder_t;

typedef struct cannon
{
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
    clpos_t pos;
    bool have;      /* true if this treasure has ball in it */
    int team;       /* team of this treasure */
    int destroyed;  /* how often this treasure destroyed */
    bool empty;     /* true if this treasure never had a ball */
    int ball_style; /* polystyle to use for color */
} treasure_t;

typedef struct target
{
    clpos_t pos;
    int team;
    int dead_ticks;
    double damage;
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
    int x, y;            /* Size of world in blocks, rounded up */
    int bwidth_floor;    /* Width of world in blocks, rounded down */
    int bheight_floor;   /* Height of world in blocks, rounded down */
    double diagonal;     /* Diagonal length in blocks */
    int width, height;   /* Size of world in pixels (optimization) */
    double hypotenuse;   /* Diagonal length in pixels (optimization) */
    int cwidth, cheight; /* Size of world in clicks */

    rules_t rules;

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
    std::vector<asteroid_concentrator_t> asteroidConcs;
    std::vector<base_t> bases;
    std::vector<cannon_t> cannons;
    std::vector<ecm_t> ecms;
    std::vector<fuel_t> fuels;
    std::vector<friction_area_t> frictionAreas;
    std::vector<grav_t> gravs;
    std::vector<item_concentrator_t> itemConcs;
    std::vector<target_t> targets;
    std::vector<transporter_t> transporters;
    std::vector<treasure_t> treasures;

    int NumWormholes;
    wormhole_t *wormholes;

    std::vector<wormhole_t> wormholesVector;

    // Make sure we don't call Grok_map_options twice.
    bool have_options;
};

static inline void World_set_block(world_t *world, blkpos_t blk, int type)
{
    assert(!(blk.bx < 0 || blk.bx >= world->x || blk.by < 0 || blk.by >= world->y));
    world->block[blk.bx][blk.by] = type;
}

static inline int World_get_block(world_t *world, blkpos_t blk)
{
    assert(!(blk.bx < 0 || blk.bx >= world->x || blk.by < 0 || blk.by >= world->y));
    return world->block[blk.bx][blk.by];
}

static inline bool World_contains_clpos(world_t *world, clpos_t pos)
{
    if (pos.cx < 0 || pos.cx >= world->cwidth)
        return false;
    if (pos.cy < 0 || pos.cy >= world->cheight)
        return false;
    return true;
}

static inline bool World_contains_clicks(world_t *world, int cx, int cy)
{
    if (cx < 0 || cx >= world->cwidth)
        return false;
    if (cy < 0 || cy >= world->cheight)
        return false;
    return true;
}

static inline clpos_t World_get_random_clpos(world_t *world)
{
    clpos_t pos;

    pos.cx = (int)(rfrac() * world->cwidth);
    pos.cy = (int)(rfrac() * world->cheight);

    return pos;
}

static inline int World_wrap_xclick(world_t *world, int cx)
{
    while (cx < 0)
        cx += world->cwidth;
    while (cx >= world->cwidth)
        cx -= world->cwidth;

    return cx;
}

static inline int World_wrap_yclick(world_t *world, int cy)
{
    while (cy < 0)
        cy += world->cheight;
    while (cy >= world->cheight)
        cy -= world->cheight;

    return cy;
}

static inline clpos_t World_wrap_clpos(world_t *world, clpos_t pos)
{
    pos.cx = World_wrap_xclick(world, pos.cx);
    pos.cy = World_wrap_yclick(world, pos.cy);

    return pos;
}

/*
 * Two inline function for edge wrap of x and y coordinates measured
 * in clicks.
 *
 * Note that even when wrap play is off, ships will wrap around the map
 * if there is not walls that hinder it.
 */
static inline int WORLD_WRAP_XCLICK(world_t *world, int cx)
{
    // TODO: Check WRAP_PLAY ?
    return World_wrap_xclick(world, cx);
}

static inline int WORLD_WRAP_YCLICK(world_t *world, int cy)
{
    // TODO: Check WRAP_PLAY ?
    return World_wrap_yclick(world, cy);
}

/*
 * Two macros for edge wrap of differences in position.
 * If the absolute value of a difference is bigger than
 * half the map size then it is wrapped.
 */
#define WORLD_WRAP_DCX(world, dcx)              \
    (Wrap_play(world)                           \
         ? ((dcx) < -(world->cwidth >> 1)       \
                ? (dcx) + world->cwidth         \
                : ((dcx) > (world->cwidth >> 1) \
                       ? (dcx) - world->cwidth  \
                       : (dcx)))                \
         : (dcx))

#define WORLD_WRAP_DCY(world, dcy)               \
    (Wrap_play(world)                            \
         ? ((dcy) < -(world->cheight >> 1)       \
                ? (dcy) + world->cheight         \
                : ((dcy) > (world->cheight >> 1) \
                       ? (dcy) - world->cheight  \
                       : (dcy)))                 \
         : (dcy))

#define TWORLD_WRAP_XCLICK(world, x_) \
    ((x_) > 0 ? (x_) % World.cwidth : ((x_) % World.cwidth + World.cwidth))

#define TWORLD_WRAP_YCLICK(world, y_) \
    ((y_) > 0 ? (y_) % World.cheight : ((y_) % World.cheight + World.cheight))

#define CENTER_XCLICK(X) \
    (((X) < -(World.cwidth >> 1)) ? (X) + World.cwidth : (((X) >= (World.cwidth >> 1)) ? (X) - World.cwidth : (X)))

#define CENTER_YCLICK(X) \
    (((X) < -(World.cheight >> 1)) ? (X) + World.cheight : (((X) >= (World.cheight >> 1)) ? (X) - World.cheight : (X)))

static inline int Num_asteroidConcs()
{
    return World.asteroidConcs.size();
}

static inline int Num_bases()
{
    return World.bases.size();
}

static inline int Num_cannons()
{
    return World.cannons.size();
}

static inline int Num_ecms()
{
    return World.ecms.size();
}

static inline int Num_frictionAreas()
{
    return World.frictionAreas.size();
}

static inline int Num_fuels()
{
    return World.fuels.size();
}

static inline int Num_gravs()
{
    return World.gravs.size();
}

static inline int Num_itemConcs()
{
    return World.itemConcs.size();
}

static inline int Num_targets()
{
    return World.targets.size();
}

static inline int Num_transporters()
{
    return World.transporters.size();
}

static inline int Num_treasures()
{
    return World.treasures.size();
}

#define Num_wormholes() (World.NumWormholes)

static inline int Num_checks()
{
    return World.checks.size();
}

// by_index functions
static inline asteroid_concentrator_t *AsteroidConc_by_index(int i)
{
    return &World.asteroidConcs[i];
}

static inline base_t *Base_by_index(int i)
{
    return &World.bases[i];
}

static inline cannon_t *Cannon_by_index(int i)
{
    return &World.cannons[i];
}

static inline ecm_t *Ecm_by_index(int i)
{
    return &World.ecms[i];
}

static inline friction_area_t *FrictionArea_by_index(int i)
{
    return &World.frictionAreas[i];
}

static inline fuel_t *Fuel_by_index(int i)
{
    return &World.fuels[i];
}

static inline grav_t *Grav_by_index(int i)
{
    return &World.gravs[i];
}

static inline item_concentrator_t *ItemConc_by_index(int i)
{
    return &World.itemConcs[i];
}

static inline target_t *Target_by_index(int i)
{
    return &World.targets[i];
}

static inline treasure_t *Treasure_by_index(int i)
{
    return &World.treasures[i];
}

// static inline  wormhole_t *Wormhole_by_index(int i)
// {
//     return &World.wormholes[i];
// }

static inline transporter_t *Transporter_by_index(int i)
{
    return &World.transporters[i];
}

// #define Wormhole_by_index(i) ((wormhole_t *)(&World.wormholes[i]))

static inline wormhole_t *Wormhole_by_index(int i)
{
    if (i < 0 || i >= Num_wormholes())
        return nullptr;
    return ((wormhole_t *)(&World.wormholes[i]));
}

static inline check_t *Check_by_index(int i)
{
    return &World.checks[i];
}

/*
 * Here the index is the team number.
 */
static inline team_t *Team_by_index(int ind)
{
    if (ind >= 0 && ind < MAX_TEAMS)
        return &World.teams[ind];
    return nullptr;
}

// Rules
static inline bool Limited_lives(world_t *world)
{
    return BIT(world->rules.mode, LIMITED_LIVES);
}

static inline bool Team_play(world_t *world)
{
    return BIT(world->rules.mode, TEAM_PLAY);
}

static inline bool Timing(world_t *world)
{
    return BIT(world->rules.mode, TIMING);
}

static inline bool Wrap_play(world_t *world)
{
    return BIT(world->rules.mode, WRAP_PLAY);
}
