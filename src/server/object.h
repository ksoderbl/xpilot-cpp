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

/*
 * Different types of attributes a player can have.
 * These are the bits of the player->have and player->used fields.
 */
#define HAS_EMERGENCY_THRUST (1U << 30)
#define HAS_AUTOPILOT (1U << 29)
#define HAS_TRACTOR_BEAM (1U << 28)
#define HAS_LASER (1U << 27)
#define HAS_CLOAKING_DEVICE (1U << 26)
#define HAS_SHIELD (1U << 25)
#define HAS_REFUEL (1U << 24)
#define HAS_REPAIR (1U << 23)
#define HAS_COMPASS (1U << 22)
#define HAS_AFTERBURNER (1U << 21)
#define HAS_CONNECTOR (1U << 20)
#define HAS_EMERGENCY_SHIELD (1U << 19)
#define HAS_DEFLECTOR (1U << 18)
#define HAS_PHASING_DEVICE (1U << 17)
#define HAS_MIRROR (1U << 16)
#define HAS_ARMOR (1U << 15)
#define HAS_SHOT (1U << 4)
#define HAS_BALL (1U << 3)

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

/*
 * Object position is non-modifiable, except at one place.
 */
typedef const struct _objposition objposition_t;
struct _objposition
{
    int cx, cy; /* object position in clicks. */
    int x, y;   /* object position in pixels. */
};

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
    unsigned short team; /* Team of player or cannon */  \
    objposition_t pos;   /* World coordinates */         \
    clpos_t prevpos;     /* previous position */         \
    vector_t vel;        /* speed in x,y */              \
    vector_t acc;        /* acceleration in x,y */       \
    DFLOAT mass;         /* mass in unigrams */          \
    long life;           /* No of ticks left to live */  \
    long status;         /* gravity, etc. */             \
    int type;            /* one bit of OBJ_XXX */        \
    int count;           /* Misc timings */              \
    modifiers_t mods;    /* Modifiers to this object */  \
    uint8_t color;       /* Color of object */           \
    uint8_t missile_dir; /* missile direction */         \
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
typedef struct _object object_t;
struct _object
{

    OBJECT_BASE

    OBJECT_EXTEND

    // #ifdef __cplusplus
    //                         _object() {}
    // #endif

#define OBJ_IND(ind) (Obj[(ind)])
#define OBJ_PTR(ptr) ((object_t *)(ptr))
};

/*
 * Mine object
 */
typedef struct _mineobject mineobject_t;
struct _mineobject
{

    OBJECT_BASE

    OBJECT_EXTEND

    int owner;        /* Who's object is this ? */
    DFLOAT ecm_range; /* Range from last ecm center */
    int spread_left;  /* how much spread time left */

    // #ifdef __cplusplus
    //                         _mineobject() {}
    // #endif

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
typedef struct _missileobject missileobject_t;
struct _missileobject
{

    OBJECT_BASE

    OBJECT_EXTEND

    MISSILE_EXTEND

    // #ifdef __cplusplus
    //                         _missileobject() {}
    // #endif

#define MISSILE_IND(ind) ((missileobject_t *)Obj[(ind)])
#define MISSILE_PTR(ptr) ((missileobject_t *)(ptr))
};

/*
 * Smart missile is a generic missile with extras.
 */
typedef struct _smartobject smartobject_t;
struct _smartobject
{

    OBJECT_BASE

    OBJECT_EXTEND

    MISSILE_EXTEND

    int new_info;     /* smart re-lock id */
    DFLOAT ecm_range; /* Range from last ecm center */

    // #ifdef __cplusplus
    //                         _smartobject() {}
    // #endif

#define SMART_IND(ind) ((smartobject_t *)Obj[(ind)])
#define SMART_PTR(ptr) ((smartobject_t *)(ptr))
};

/*
 * Torpedo is a generic missile with extras
 */
typedef struct _torpobject torpobject_t;
struct _torpobject
{

    OBJECT_BASE

    OBJECT_EXTEND

    MISSILE_EXTEND

    int spread_left; /* how much spread time left */

    // #ifdef __cplusplus
    //                         _torpobject() {}
    // #endif

#define TORP_IND(ind) ((torpobject_t *)Obj[(ind)])
#define TORP_PTR(ptr) ((torpobject_t *)(ptr))
};

/*
 * The ball object.
 */
typedef struct _ballobject ballobject_t;
struct _ballobject
{

    OBJECT_BASE

    OBJECT_EXTEND

    int owner;     /* Who's object is this ? */
    int treasure;  /* treasure for ball */
    DFLOAT length; /* distance ball to player */

    // #ifdef __cplusplus
    //                         _ballobject() {}
    // #endif

#define BALL_IND(ind) ((ballobject_t *)Obj[(ind)])
#define BALL_PTR(obj) ((ballobject_t *)(obj))
};

/*
 * Object with a wireframe representation.
 */
typedef struct _wireobject wireobject_t;
struct _wireobject
{

    OBJECT_BASE

    OBJECT_EXTEND

    DFLOAT turnspeed; /* how fast to turn */

    uint8_t size;     /* Size of object (wreckage) */
    uint8_t rotation; /* Rotation direction */

    // #ifdef __cplusplus
    //                         _wireobject() {}
    // #endif

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
 * Fuel structure, used by player
 */
typedef struct
{
    long sum;                 /* Sum of fuel in all tanks */
    long max;                 /* How much fuel can you take? */
    int current;              /* Number of currently used tank */
    int num_tanks;            /* Number of tanks */
    long tank[1 + MAX_TANKS]; /* main fixed tank + extra tanks. */
    long l1;                  /* Fuel critical level */
    long l2;                  /* Fuel warning level */
    long l3;                  /* Fuel notify level */
} pl_fuel_t;

struct _visibility
{
    int canSee;
    long lastChange;
};

#define MAX_PLAYER_ECMS 8 /* Maximum simultaneous per player */
typedef struct
{
    int size;
    // position_t pos;
    clpos_t clk_pos;
    int id;
} ecm_t;

/*
 * Structure holding the info for one pulse of a laser.
 */
typedef struct
{
    position_t pos;
    // clpos_t clk_pos; // TODO
    int dir;
    int len;
    int life;
    int id;
    unsigned short team;
    modifiers_t mods;
    bool refl;
} pulse_t;

/*
 * Transporter info.
 */
typedef struct
{
    // position_t pos;
    clpos_t clk_pos;
    int target;
    int id;
    int count;
} trans_t;

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
extern ecm_t *Ecms[];
extern trans_t *Transporters[];

extern int NumPlayers;
extern int NumPseudoPlayers;
extern int ObjCount;
extern int NumPulses;
extern int NumEcms;
extern int NumTransporters;
extern int NumAlliances;
extern int NumRobots;

void Object_position_set_clicks(object_t *obj, int cx, int cy);
void Object_position_init_clicks(object_t *obj, int cx, int cy);

#define Object_position_remember(o_)  \
    ((o_)->prevpos.cx = (o_)->pos.cx, \
     (o_)->prevpos.cy = (o_)->pos.cy)

#endif
