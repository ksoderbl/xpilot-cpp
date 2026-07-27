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

#pragma once

#define SERVER

#include "bit.h"
#include "click.h"
#include "connection.h"
#include "item.h"
#include "keys.h"
#include "shipshape.h"

#include "map.h"
#include "option.h"
#include "modifiers.h"
#include "serverconst.h"

/*
 * Different types of objects, including player.
 * Robots and tanks are players but have an additional type_ext field.
 * Smart missile, heatseeker and torpedo can be merged into missile.
 */
#define OBJ_TYPEBIT(type) (1U << (type))

/*
 * Different types of objects, including player.
 * Robots and tanks are players but have an additional type_ext field.
 * Smart missile, heatseeker and torpedoe can be merged into missile.
 * ECM doesn't really need an object type.
 */
#define OBJ_PLAYER (1U << 0)
#define OBJ_DEBRIS (1U << 1)
#define OBJ_SPARK (1U << 2)
#define OBJ_BALL (1U << 3)
#define OBJ_SHOT (1U << 4)
#define OBJ_SMART_SHOT (1U << 5)
#define OBJ_MINE (1U << 6)
#define OBJ_TORPEDO (1U << 7)
#define OBJ_HEAT_SHOT (1U << 8)
#define OBJ_PULSE (1U << 9)
#define OBJ_ITEM (1U << 10)
#define OBJ_WRECKAGE (1U << 11)
#define OBJ_ASTEROID (1U << 12)
#define OBJ_CANNON_SHOT (1U << 13)

/* BITS */
#define OBJ_PLAYER_BIT (1U << 0)
#define OBJ_DEBRIS_BIT (1U << 1)
#define OBJ_SPARK_BIT (1U << 2)
#define OBJ_BALL_BIT (1U << 3)
#define OBJ_SHOT_BIT (1U << 4)
#define OBJ_SMART_SHOT_BIT (1U << 5)
#define OBJ_MINE_BIT (1U << 6)
#define OBJ_TORPEDO_BIT (1U << 7)
#define OBJ_HEAT_SHOT_BIT (1U << 8)
#define OBJ_PULSE_BIT (1U << 9)
#define OBJ_ITEM_BIT (1U << 10)
#define OBJ_WRECKAGE_BIT (1U << 11)
#define OBJ_ASTEROID_BIT (1U << 12)
#define OBJ_CANNON_SHOT_BIT (1U << 13)

/*
 * Possible object status bits.
 */
// #define GRAVITY (1U << 0)
// #define WARPING (1U << 1)
// #define WARPED (1U << 2)
// #define CONFUSED (1U << 3)
// #define FROMCANNON (1U << 4)     /* Object from cannon */
// #define RECREATE (1U << 5)       /* Recreate ball */
// #define THRUSTING (1U << 6)      /* Engine is thrusting */
// #define OWNERIMMUNE (1U << 7)    /* Owner is immune to object */
// #define NOEXPLOSION (1U << 8)    /* No recreate explosion */
// #define COLLISIONSHOVE (1U << 9) /* Collision counts as shove */
// #define RANDOM_ITEM (1U << 10)   /* Item shows up as random */

#define LEGACY_PLAYING (1L << 0)   /* Not returning to base */
#define LEGACY_PAUSE (1L << 1)     /* Must stay below 8 */
#define LEGACY_GAME_OVER (1L << 2) /* Must stay below 8 */
#define LEGACY_KILLED (1L << 10)

#define THRUSTING (1L << 3)     /* not used by client? */
#define SELF_DESTRUCT (1L << 4) /* not used by client? */
#define WANT_AUDIO (1L << 5)    /* whether client has SOUND */

#define GRAVITY (1L << 11)
#define WARPING (1L << 12)
#define WARPED (1L << 13)
#define CONFUSED (1L << 14)
#define FROMCANNON (1L << 15)     /* Object from cannon */
#define HOVERPAUSE (1L << 16)     /* Hovering pause */
#define RECREATE (1L << 17)       /* Recreate ball */
#define FROMBOUNCE (1L << 18)     /* Spark from wall bounce */
#define OWNERIMMUNE (1L << 19)    /* Owner is immune to object */
#define REPROGRAM (1L << 20)      /* Player reprogramming */
#define NOEXPLOSION (1L << 21)    /* No ball recreate explosion */
#define COLLISIONSHOVE (1L << 22) /* Collision counts as shove */
#define FINISH (1L << 23)         /* Reached race finish */
#define RACE_OVER (1L << 24)      /* After finished and score. */
#define RANDOM_ITEM (1L << 25)    /* If an item shows up as random */

#define LOCK_NONE 0x00    /* No lock */
#define LOCK_PLAYER 0x01  /* Locked on player */
#define LOCK_VISIBLE 0x02 /* Lock information was on HUD */
                          /* computed just before frame shown */
                          /* and client input checked */
#define LOCKBANK_MAX 4    /* Maximum number of locks in bank */

#define NOT_CONNECTED (-1)

#define OBJ_X_IN_BLOCKS(obj) CLICK_TO_BLOCK((obj)->pos.cx)
#define OBJ_Y_IN_BLOCKS(obj) CLICK_TO_BLOCK((obj)->pos.cy)

/*
 * Node within a Cell list.
 */
typedef struct cell_node cell_node_t;
struct cell_node
{
    cell_node_t *next;
    cell_node_t *prev;
};

#define OBJECT_BASE                                                                                \
    short id;            /* For shots => id of player */                                           \
    uint16_t team;       /* Team of player or cannon */                                            \
                         /* Object position pos must only be changed with the proper functions! */ \
    clpos_t pos;         /* World coordinates */                                                   \
    clpos_t prevpos;     /* previous position */                                                   \
    clpos_t extmove;     /* For collision detection */                                             \
    double wall_time;    /* bounce/crash time within frame */                                      \
    vector_t vel;        /* speed in x,y */                                                        \
    vector_t acc;        /* acceleration in x,y */                                                 \
    double mass;         /* mass in unigrams */                                                    \
    double obj_life;     /* No of ticks left to live */                                            \
    modifiers_t mods;    /* Modifiers to this object */                                            \
    int type;            /* one of OBJ_XXX */                                                      \
    uint8_t color;       /* Color of object */                                                     \
    uint8_t collmode;    /* collision checking mode */                                             \
    uint8_t missile_dir; /* missile direction */                                                   \
    uint32_t obj_status; /* gravity, etc. */

/* up to here all object types are the same as all player types. */

#define OBJECT_EXTEND                                \
    cell_node_t cell; /* node in cell linked list */ \
    int pl_range;     /* distance for collision */   \
    int pl_radius;    /* distance for hit */         \
    long fuselife;    /* fuse duration ticks */      \
    double fuse;      /* ticks until fused, TODO */

/*
 * Generic object
 */
typedef struct xp_object object_t;
struct xp_object
{
    OBJECT_BASE

    OBJECT_EXTEND

#define OBJ_IND(ind) (Obj[(ind)])
#define OBJ_PTR(ptr) ((object_t *)(ptr))
};

/*
 * Mine object
 */
typedef struct xp_mineobject mineobject_t;
struct xp_mineobject
{
    OBJECT_BASE

    OBJECT_EXTEND

    double mine_count;                       /* Misc snafus */
    long mine_info; /* Miscellaneous info */ // TODO: REMOVE

    int mine_owner;       /* Who's object is this ? */
    double ecm_range;     /* Range from last ecm center */
    int mine_spread_left; /* how much spread time left */

#define MINE_IND(ind) ((mineobject_t *)Obj[(ind)])
#define MINE_PTR(ptr) ((mineobject_t *)(ptr))
};

#define MISSILE_EXTEND                               \
    double missile_max_speed; /* speed limitation */ \
    double missile_turnspeed; /* how fast to turn */

/* up to here all missiles types are the same. */

/*
 * Generic missile object
 */
typedef struct xp_missileobject missileobject_t;
struct xp_missileobject
{
    OBJECT_BASE

    OBJECT_EXTEND

    MISSILE_EXTEND

#define MISSILE_IND(ind) ((missileobject_t *)Obj[(ind)])
#define MISSILE_PTR(ptr) ((missileobject_t *)(ptr))
};

/*
 * Smart missile is a generic missile with extras.
 */
typedef struct xp_smartobject smartobject_t;
struct xp_smartobject
{
    OBJECT_BASE

    OBJECT_EXTEND

    MISSILE_EXTEND

    // TODO: Remove these
    int new_info; /* smart re-lock id */

    long info; /* Miscellaneous info */ // TODO: REMOVE
    int count;                          // TODO: REMOVE

    double smart_ecm_range; /* Range from last ecm center*/
    double smart_count;     /* Misc snafus */
    short smart_lock_id;    /* snafu */
    short smart_relock_id;  /* smart re-lock id */

#define SMART_IND(ind) ((smartobject_t *)Obj[(ind)])
#define SMART_PTR(ptr) ((smartobject_t *)(ptr))
};

/*
 * Torpedo is a generic missile with extras
 */
typedef struct xp_torpobject torpobject_t;
struct xp_torpobject
{
    OBJECT_BASE

    OBJECT_EXTEND

    MISSILE_EXTEND

    int count; /* Misc timings */       // TODO: REMOVE
    long info; /* Miscellaneous info */ // TODO: REMOVE

    int torp_spread_left; /* how much spread time left: TODO: double */
    double torp_count;    /* Misc snafus */

#define TORP_IND(ind) ((torpobject_t *)Obj[(ind)])
#define TORP_PTR(ptr) ((torpobject_t *)(ptr))
};

/*
 * Heat-seeker is a generic missile with extras
 */
typedef struct xp_heatobject heatobject_t;
struct xp_heatobject
{
    OBJECT_BASE

    OBJECT_EXTEND

    MISSILE_EXTEND

    int count; /* Misc timings */       // TODO: REMOVE
    long info; /* Miscellaneous info */ // TODO: REMOVE

    double heat_count;  /* Misc snafus */
    short heat_lock_id; /* snafu */

#define HEAT_IND(ind) ((heatobject_t *)Obj[(ind)])
#define HEAT_PTR(ptr) ((heatobject_t *)(ptr))
};

/*
 * The ball object.
 */
typedef struct xp_ballobject ballobject_t;
struct xp_ballobject
{
    OBJECT_BASE

    OBJECT_EXTEND

    int ball_count; /* Misc timings */ // TODO: REMOVE

    double ball_loose_ticks;

    int dummy1;
    int dummy2;

    // There's a bug here with these pointers, the dummy ints protect against it.
    treasure_t *ball_treasure;      /* treasure for ball */
    treasure_t *ball_treasure_copy; /* treasure for ball */

    int dummy3;
    int dummy4;

    short ball_owner; /* Who's object is this ? */
    short ball_style; /* What polystyle to use */

#define BALL_IND(ind) ((ballobject_t *)Obj[(ind)])
#define BALL_PTR(obj) ((ballobject_t *)(obj))
};

/*
 * Object with a wireframe representation.
 */
typedef struct xp_wireobject wireobject_t;
struct xp_wireobject
{
    OBJECT_BASE

    OBJECT_EXTEND

    double wire_turnspeed; /* how fast to turn */

    uint8_t wire_type;     /* Type of object */
    uint8_t wire_size;     /* Size of object */
    uint8_t wire_rotation; /* Rotation direction */

#define WIRE_IND(ind) ((wireobject_t *)Obj[(ind)])
#define WIRE_PTR(obj) ((wireobject_t *)(obj))
};

/*
 * Pulse object used for laser pulses.
 */
typedef struct xp_pulseobject pulseobject_t;
struct xp_pulseobject
{
    OBJECT_BASE

    OBJECT_EXTEND

    int count; /* Misc timings */ // TODO: REMOVE

    double pulse_len;  /* Length of the pulse */
    uint8_t pulse_dir; /* Direction of the pulse */
    bool pulse_refl;   /* Pulse was reflected ? */

#define PULSE_IND(ind) ((pulseobject_t *)Obj[(ind)])
#define PULSE_PTR(obj) ((pulseobject_t *)(obj))
};

/*
 * Item object.
 */
typedef struct xp_itemobject itemobject_t;
struct xp_itemobject
{
    OBJECT_BASE

    OBJECT_EXTEND

    long info; /* Miscellaneous info */ // TODO: REMOVE
    int count; /* Misc timings */       // TODO: REMOVE

    int item_type;  /* One of ITEM_* */
    int item_count; /* Misc snafus */

#define ITEM_IND(ind) ((itemobject_t *)Obj[(ind)])
#define ITEM_PTR(obj) ((itemobject_t *)(obj))
};

/*
 * Any object type should be part of this union.
 */
typedef union xp_anyobject anyobject_t;
union xp_anyobject
{
    object_t obj;
    ballobject_t ball;
    mineobject_t mine;
    missileobject_t missile;
    smartobject_t smart;
    torpobject_t torp;
    heatobject_t heat;
    wireobject_t wireobj;
    pulseobject_t pulse;
    itemobject_t item;
};

/*
 * Structure holding the info for one pulse of a laser.
 */
typedef struct
{
    clpos_t pos;
    int dir;
    int len;
    int life;
    int id;
    uint16_t team;
    modifiers_t mods;
    bool refl;
} pulse_t;

/*
 * Shove-information.
 *
 * This is for keeping a record of the last N times the player was shoved,
 * for assigning wall-smash-blame, where N=MAX_RECORDED_SHOVES.
 */
#define MAX_RECORDED_SHOVES 4

typedef struct
{
    int pusher_id;
    int time;
} shove_t;

struct robot_data;

#define NumObjs (ObjCount + 0)

extern object_t *Obj[];
extern pulse_t *Pulses[];
// extern ecm_t *Ecms[];
// extern transporter_t *Transporters[];

extern int NumPlayers;
extern int NumPseudoPlayers;
extern int ObjCount;
extern int NumPulses;
// extern int NumEcms;
// extern int NumTransporters;
extern int NumAlliances;
extern int NumRobots;

void Object_position_set_clpos(object_t *obj, clpos_t pos);
void Object_position_init_clpos(object_t *obj, clpos_t pos);

static inline void Object_position_remember(object_t *obj)
{
    obj->prevpos = obj->pos;
}

const char *Object_typename(object_t *obj);

static inline void Object_position_set_clvec(object_t *obj, clvec_t vec)
{
    clpos_t pos;

    pos.cx = vec.cx;
    pos.cy = vec.cy;

    Object_position_set_clpos(obj, pos);
}

// #define SHOT_MULT(o)                                                           \
//     ((BIT((o)->mods.nuclear, MODS_NUCLEAR) && BIT((o)->mods.warhead, CLUSTER)) \
//          ? options.nukeClusterDamage                                           \
//          : 1.0)

static inline double SHOT_MULT(object_t *obj)
{
    int nuclear = Mods_get(obj->mods, ModsNuclear);
    int cluster = Mods_get(obj->mods, ModsCluster);
    if (nuclear && cluster)
        return options.nukeClusterDamage;
    return 1.0;
}

object_t *Object_allocate(void);
void Object_free_ind(int ind);
void Object_free_ptr(object_t *obj);
void Alloc_shots(int number);
void Free_shots(void);
