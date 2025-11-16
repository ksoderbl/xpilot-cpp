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

#ifndef OBJECT_H
#define OBJECT_H

#define SERVER

#include "bit.h"
#include "click.h"
#include "connection.h"
#include "item.h"
#include "keys.h"
#include "shipshape.h"

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
 * Weapons modifiers.
 */
typedef struct
{
    unsigned int nuclear : 2;  /* N  modifier */
    unsigned int warhead : 2;  /* CI modifier */
    unsigned int velocity : 2; /* V# modifier */
    unsigned int mini : 2;     /* X# modifier */
    unsigned int spread : 2;   /* Z# modifier */
    unsigned int power : 2;    /* B# modifier */
    unsigned int laser : 2;    /* LS LB modifier */
    unsigned int spare : 2;    /* padding for alignment */
} modifiers_t;

#define CLEAR_MODS(mods) memset(&(mods), 0, sizeof(modifiers_t))

#define MODS_NUCLEAR_MAX 2 /* - N FN */
#define NUCLEAR (1U << 0)
#define FULLNUCLEAR (1U << 1)

#define MODS_WARHEAD_MAX 3 /* - C I CI */
#define CLUSTER (1U << 0)
#define IMPLOSION (1U << 1)

#define MODS_VELOCITY_MAX 3 /* - V1 V2 V3 */
#define MODS_MINI_MAX 3     /* - X2 X3 X4 */
#define MODS_SPREAD_MAX 3   /* - Z1 Z2 Z3 */
#define MODS_POWER_MAX 3    /* - B1 B2 B3 */

#define MODS_LASER_MAX 2 /* - LS LB */
#define STUN (1U << 0)
#define BLIND (1U << 1)

#define LOCK_NONE 0x00    /* No lock */
#define LOCK_PLAYER 0x01  /* Locked on player */
#define LOCK_VISIBLE 0x02 /* Lock information was on HUD */
                          /* computed just before frame shown */
                          /* and client input checked */
#define LOCKBANK_MAX 4    /* Maximum number of locks in bank */

#define NOT_CONNECTED (-1)

#define OBJ_X_IN_CLICKS(obj) ((obj)->pos.cx)
#define OBJ_Y_IN_CLICKS(obj) ((obj)->pos.cy)
#define OBJ_X_IN_PIXELS(obj) CLICK_TO_PIXEL((obj)->pos.cx)
#define OBJ_Y_IN_PIXELS(obj) CLICK_TO_PIXEL((obj)->pos.cy)
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

#define OBJECT_BASE                                      \
    short id;            /* For shots => id of player */ \
    uint16_t team;       /* Team of player or cannon */  \
    clpos_t pos;         /* World coordinates */         \
    ipos_t pix_pos;      /* World pixel coordinates */   \
    clpos_t prevpos;     /* previous position */         \
    vector_t vel;        /* speed in x,y */              \
    vector_t acc;        /* acceleration in x,y */       \
    float mass;          /* mass in unigrams */          \
    modifiers_t mods;    /* Modifiers to this object */  \
    long life;           /* No of ticks left to live */  \
    int type;            /* one bit of OBJ_XXX */        \
    int count;           /* Misc timings */              \
    uint8_t color;       /* Color of object */           \
    uint8_t missile_dir; /* missile direction */         \
    uint32_t obj_status; /* gravity, etc. */

/* up to here all object types are the same as all player types. */

#define OBJECT_EXTEND                              \
    cell_node cell; /* node in cell linked list */ \
    long info;      /* Miscellaneous info */       \
    long fuselife;  /* fuse duration ticks */      \
    int pl_range;   /* distance for collision */   \
    int pl_radius;  /* distance for hit */         \
/* up to here all object types are the same. */

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

    int mine_owner;   /* Who's object is this ? */
    DFLOAT ecm_range; /* Range from last ecm center */
    int spread_left;  /* how much spread time left */

#define MINE_IND(ind) ((mineobject_t *)Obj[(ind)])
#define MINE_PTR(ptr) ((mineobject_t *)(ptr))
};

#define MISSILE_EXTEND                       \
    DFLOAT max_speed; /* speed limitation */ \
    DFLOAT turnspeed; /* how fast to turn */
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

    int new_info;     /* smart re-lock id */
    DFLOAT ecm_range; /* Range from last ecm center */

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

    int spread_left; /* how much spread time left */

#define TORP_IND(ind) ((torpobject_t *)Obj[(ind)])
#define TORP_PTR(ptr) ((torpobject_t *)(ptr))
};

/*
 * The ball object.
 */
typedef struct xp_ballobject ballobject_t;
struct xp_ballobject
{

    OBJECT_BASE

    OBJECT_EXTEND

    int ball_owner; /* Who's object is this ? */
    int treasure;   /* treasure for ball */
    DFLOAT length;  /* distance ball to player */

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

    DFLOAT turnspeed; /* how fast to turn */

    uint8_t wire_size; /* Size of object (wreckage) */
    uint8_t rotation;  /* Rotation direction */

#define WIRE_IND(ind) ((wireobject_t *)Obj[(ind)])
#define WIRE_PTR(obj) ((wireobject_t *)(obj))
};

/*
 * Any object type should be part of this union.
 */
typedef union _anyobject anyobject_t;
union _anyobject
{
    object_t obj;
    ballobject_t ball;
    mineobject_t mine;
    missileobject_t missile;
    smartobject_t smart;
    torpobject_t torp;
    wireobject_t wireobj;
};

/*
 * Structure holding the info for one pulse of a laser.
 */
typedef struct
{
    position_t pix_pos;
    // clpos_t pos; // TODO
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

#define Object_position_remember(o_)  \
    ((o_)->prevpos.cx = (o_)->pos.cx, \
     (o_)->prevpos.cy = (o_)->pos.cy)

#endif
