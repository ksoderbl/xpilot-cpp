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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cmath>

#include "bit.h"
#include "commonmacros.h"
#include "const.h"
#include "rules.h"
#include "setup.h"
#include "strlcpy.h"
#include "types.h"
#include "xpmath.h"

#include "client.h"
#include "netclient.h"
#include "paint.h"
#include "paintradar.h"
#include "protoclient.h"
#include "talk.h"

ipos pos;
ipos vel;
ipos world;
ipos realWorld;
short heading;
short nextCheckPoint;

u_byte numItems[NUM_ITEMS];     /* Count of currently owned items */
u_byte lastNumItems[NUM_ITEMS]; /* Last item count shown */
int numItemsTime[NUM_ITEMS];    /* Number of frames to show this item count */
DFLOAT showItemsTime;           /* How long to show changed item count for */
short autopilotLight;

short lock_id;   /* Id of player locked onto */
short lock_dir;  /* Direction of lock */
short lock_dist; /* Distance to player locked onto */

other_t *self;     /* player info */
short selfVisible; /* Are we alive and playing? */
short damaged;     /* Damaged by ECM */
short destruct;    /* If self destructing */
short shutdown_delay;
short shutdown_count;
short thrusttime;
short thrusttimemax;
short shieldtime;
short shieldtimemax;
short phasingtime;
short phasingtimemax;

int RadarHeight = 0;
int RadarWidth = 256;        /* must always be 256! */
int map_point_distance;      /* spacing of navigation points */
int map_point_size;          /* size of navigation points */
int spark_size;              /* size of debris and spark */
int shot_size;               /* size of shot */
int teamshot_size;           /* size of team shot */
bool showNastyShots = false; /* show original flavor shots or the new "nasty shots" */
long control_count;          /* Display control for how long? */
int roundDelay;              /* != 0 means we're in a delay */
int roundDelayMax;           /* (not yet) used for graph of time remaining in delay */

u_byte spark_rand;     /* Sparkling effect */
u_byte old_spark_rand; /* previous value of spark_rand */

long fuelSum;      /* Sum of fuel in all tanks */
long fuelMax;      /* How much fuel can you take? */
short fuelCurrent; /* Number of currently used tank */
short numTanks;    /* Number of tanks */
long fuelCount;    /* Display fuel for how long? */
int fuelLevel1;    /* Fuel critical level */
int fuelLevel2;    /* Fuel warning level */
int fuelLevel3;    /* Fuel notify level */

char *shipShape;                /* Shape of player's ship */
DFLOAT power;                   /* Force of thrust */
DFLOAT power_s;                 /* Saved power fiks */
DFLOAT turnspeed;               /* How fast player acc-turns */
DFLOAT turnspeed_s;             /* Saved turnspeed */
DFLOAT turnresistance;          /* How much is lost in % */
DFLOAT turnresistance_s;        /* Saved (see above) */
DFLOAT displayedPower;          /* What the server is sending us */
DFLOAT displayedTurnspeed;      /* What the server is sending us */
DFLOAT displayedTurnresistance; /* What the server is sending us */
DFLOAT spark_prob;              /* Sparkling effect user configurable */
int charsPerSecond;             /* Message output speed (configurable) */

DFLOAT hud_move_fact; /* scale the hud-movement (speed) */
DFLOAT ptr_move_fact; /* scale the speed pointer length */
long instruments;     /* Instruments on screen (bitmask) */
char mods[MAX_CHARS]; /* Current modifiers in effect */
int packet_size;      /* Current frame update packet size */
int packet_loss;      /* lost packets per second */
int packet_drop;      /* dropped packets per second */
int packet_lag;       /* approximate lag in frames */
char *packet_measure; /* packet measurement in a second */
long packet_loop;     /* start of measurement */

bool showRealName = false;  /* Show realname instead of nick name */
char name[MAX_CHARS];       /* Nick-name of player */
char realname[MAX_CHARS];   /* Real name of player */
char servername[MAX_CHARS]; /* Name of server connecting to */
unsigned version;           /* Version of the server */
bool toggle_shield;         /* Are shields toggled by a press? */
int shields = 1;            /* When shields are considered up */

bool auto_shield = 1; /* shield drops for fire */

int maxFPS; /* Client's own FPS */
int oldMaxFPS;

int clientPortStart = 0; /* First UDP port for clients */
int clientPortEnd = 0;   /* Last one (these are for firewalls) */

u_byte lose_item;     /* index for dropping owned item */
int lose_item_active; /* one of the lose keys is pressed */

#ifdef SOUND
char sounds[MAX_CHARS];      /* audio mappings */
char audioServer[MAX_CHARS]; /* audio server */
int maxVolume;               /* maximum volume (in percent) */
#endif                       /* SOUND */

refuel_t *refuel_ptr;
int num_refuel, max_refuel;
connector_t *connector_ptr;
int num_connector, max_connector;
laser_t *laser_ptr;
int num_laser, max_laser;
missile_t *missile_ptr;
int num_missile, max_missile;
ball_t *ball_ptr;
int num_ball, max_ball;
ship_t *ship_ptr;
int num_ship, max_ship;
mine_t *mine_ptr;
int num_mine, max_mine;
itemtype_t *itemtype_ptr;
int num_itemtype, max_itemtype;
ecm_t *ecm_ptr;
int num_ecm, max_ecm;
trans_t *trans_ptr;
int num_trans, max_trans;
paused_t *paused_ptr;
int num_paused, max_paused;
radar_t *radar_ptr;
int num_radar, max_radar;
vcannon_t *vcannon_ptr;
int num_vcannon, max_vcannon;
vfuel_t *vfuel_ptr;
int num_vfuel, max_vfuel;
vbase_t *vbase_ptr;
int num_vbase, max_vbase;
debris_t *debris_ptr[DEBRIS_TYPES];
int num_debris[DEBRIS_TYPES],
    max_debris[DEBRIS_TYPES];
debris_t *fastshot_ptr[DEBRIS_TYPES * 2];
int num_fastshot[DEBRIS_TYPES * 2],
    max_fastshot[DEBRIS_TYPES * 2];
vdecor_t *vdecor_ptr;
int num_vdecor, max_vdecor;
wreckage_t *wreckage_ptr;
int num_wreckage, max_wreckage;
asteroid_t *asteroid_ptr;
int num_asteroids, max_asteroids;
wormhole_t *wormhole_ptr;
int num_wormholes, max_wormholes;

long time_left = -1;
long start_loops, end_loops;

int eyesId;     /* Player we get frame updates for */
short snooping; /* are we snooping on someone else? */

static other_t *Others = 0;
static int num_others = 0,
           max_others = 0;

static fuelstation_t *fuels = 0;
static int num_fuels = 0;

static homebase_t *bases = 0;
static int num_bases = 0;

static cannontime_t *cannons = 0;
static int num_cannons = 0;

static target_t *targets = 0;
static int num_targets = 0;

#define MAX_CHECKPOINT 26

char *talk_fast_msgs[TALK_FAST_NR_OF_MSGS]; /* talk macros */

int scoresChanged = 0;

static checkpoint_t checks[MAX_CHECKPOINT];

score_object_t score_objects[MAX_SCORE_OBJECTS];
int score_object = 0;

/* provide cut&paste and message history */
extern selection_t selection;
static char *HistoryBlock = NULL;
extern char *HistoryMsg[MAX_HIST_MSGS];

int maxLinesInHistory;

int Handle_start(long server_loops)
{
    int i;

    start_loops = server_loops;

    num_refuel = 0;
    num_connector = 0;
    num_missile = 0;
    num_ball = 0;
    num_ship = 0;
    num_mine = 0;
    num_itemtype = 0;
    num_ecm = 0;
    num_trans = 0;
    num_paused = 0;
    num_radar = 0;
    num_vcannon = 0;
    num_vfuel = 0;
    num_vbase = 0;
    num_vdecor = 0;
    for (i = 0; i < DEBRIS_TYPES; i++)
    {
        num_debris[i] = 0;
    }

    damaged = 0;
    destruct = 0;
    shutdown_delay = 0;
    shutdown_count = -1;
    eyesId = (self != NULL) ? self->id : 0;
    thrusttime = -1;
    shieldtime = -1;
    phasingtime = -1;
    return 0;
}

int Handle_end(long server_loops)
{
    end_loops = server_loops;
    snooping = self && (eyesId != self->id);
    Paint_frame();
    return 0;
}

int Handle_self_items(u_byte *newNumItems)
{
    memcpy(numItems, newNumItems, NUM_ITEMS * sizeof(u_byte));
    return 0;
}

int Handle_self(int x, int y, int vx, int vy, int newHeading,
                float newPower, float newTurnspeed, float newTurnresistance,
                int newLockId, int newLockDist, int newLockBearing,
                int newNextCheckPoint, int newAutopilotLight,
                u_byte *newNumItems, int newCurrentTank,
                int newFuelSum, int newFuelMax, int newPacketSize)
{
    pos.x = x;
    pos.y = y;
    vel.x = vx;
    vel.y = vy;
    heading = newHeading;
    displayedPower = newPower;
    displayedTurnspeed = newTurnspeed;
    displayedTurnresistance = newTurnresistance;
    lock_id = newLockId;
    lock_dist = newLockDist;
    lock_dir = newLockBearing;
    nextCheckPoint = newNextCheckPoint;
    autopilotLight = newAutopilotLight;
    memcpy(numItems, newNumItems, NUM_ITEMS * sizeof(u_byte));
    fuelCurrent = newCurrentTank;
    if (newFuelSum > fuelSum && selfVisible != 0)
    {
        fuelCount = FUEL_NOTIFY;
    }
    fuelSum = newFuelSum;
    fuelMax = newFuelMax;
    selfVisible = 0;
    if (newPacketSize + 16 < packet_size)
    {
        packet_size -= 16;
    }
    else
    {
        packet_size = newPacketSize;
    }

    world.x = pos.x - (ext_view_width / 2);
    world.y = pos.y - (ext_view_height / 2);
    realWorld = world;
    if (BIT(Setup->mode, WRAP_PLAY))
    {
        if (world.x < 0 && world.x + ext_view_width < Setup->width)
        {
            world.x += Setup->width;
        }
        else if (world.x > 0 && world.x + ext_view_width >= Setup->width)
        {
            realWorld.x -= Setup->width;
        }
        if (world.y < 0 && world.y + ext_view_height < Setup->height)
        {
            world.y += Setup->height;
        }
        else if (world.y > 0 && world.y + ext_view_height >= Setup->height)
        {
            realWorld.y -= Setup->height;
        }
    }
    return 0;
}

int Handle_eyes(int id)
{
    eyesId = id;
    return 0;
}

int Handle_damaged(int dam)
{
    damaged = dam;
    return 0;
}

int Handle_modifiers(char *m)
{
    strlcpy(mods, m, MAX_CHARS);
    return 0;
}

int Handle_destruct(int count)
{
    destruct = count;
    return 0;
}

int Handle_shutdown(int count, int delay)
{
    shutdown_count = count;
    shutdown_delay = delay;
    return 0;
}

int Handle_thrusttime(int count, int max)
{
    thrusttime = count;
    thrusttimemax = max;
    return 0;
}

int Handle_shieldtime(int count, int max)
{
    shieldtime = count;
    shieldtimemax = max;
    return 0;
}

int Handle_phasingtime(int count, int max)
{
    phasingtime = count;
    phasingtimemax = max;
    return 0;
}

int Handle_rounddelay(int count, int max)
{
    roundDelay = count;
    roundDelayMax = max;
    return (0);
}

int Handle_refuel(int x0, int y0, int x1, int y1)
{
    refuel_t t;

    t.x0 = x0;
    t.x1 = x1;
    t.y0 = y0;
    t.y1 = y1;
    STORE(refuel_t, refuel_ptr, num_refuel, max_refuel, t);
    return 0;
}

int Handle_connector(int x0, int y0, int x1, int y1, int tractor)
{
    connector_t t;

    t.x0 = x0;
    t.x1 = x1;
    t.y0 = y0;
    t.y1 = y1;
    t.tractor = tractor;
    STORE(connector_t, connector_ptr, num_connector, max_connector, t);
    return 0;
}

int Handle_laser(int color, int x, int y, int len, int dir)
{
    laser_t t;

    t.color = color;
    t.x = x;
    t.y = y;
    t.len = len;
    t.dir = dir;
    STORE(laser_t, laser_ptr, num_laser, max_laser, t);
    return 0;
}

int Handle_missile(int x, int y, int len, int dir)
{
    missile_t t;

    t.x = x;
    t.y = y;
    t.dir = dir;
    t.len = len;
    STORE(missile_t, missile_ptr, num_missile, max_missile, t);
    return 0;
}

int Handle_ball(int x, int y, int id)
{
    ball_t t;

    t.x = x;
    t.y = y;
    t.id = id;
    STORE(ball_t, ball_ptr, num_ball, max_ball, t);
    return 0;
}

int Handle_ship(int x, int y, int id, int dir, int shield, int cloak, int eshield,
                int phased, int deflector)
{
    ship_t t;

    t.x = x;
    t.y = y;
    t.id = id;
    t.dir = dir;
    t.shield = shield;
    t.cloak = cloak;
    t.eshield = eshield;
    t.phased = phased;
    t.deflector = deflector;
    STORE(ship_t, ship_ptr, num_ship, max_ship, t);

    /* if we see a ship in the center of the display, we may be watching
     * it, especially if it's us!  consider any ship there to be our eyes
     * until we see a ship that really is us.
     * BG: XXX there was a bug here.  self was dereferenced at "self->id"
     * while self could be NULL here.
     */
    if (!selfVisible && ((x == pos.x && y == pos.y) || (self && id == self->id)))
    {
        int radarx, radary;
        eyesId = id;
        selfVisible = (self && (id == self->id));
        radarx = (int)((double)(x * RadarWidth) / Setup->width + 0.5);
        radary = (int)((double)(y * RadarHeight) / Setup->height + 0.5);
        return Handle_radar(radarx, radary, 3);
    }

    return 0;
}

int Handle_mine(int x, int y, int teammine, int id)
{
    mine_t t;

    t.x = x;
    t.y = y;
    t.teammine = teammine;
    t.id = id;
    STORE(mine_t, mine_ptr, num_mine, max_mine, t);
    return 0;
}

int Handle_item(int x, int y, int type)
{
    itemtype_t t;

    t.x = x;
    t.y = y;
    t.type = type;
    STORE(itemtype_t, itemtype_ptr, num_itemtype, max_itemtype, t);
    return 0;
}

#define STORE_DEBRIS(typ_e, _p, _n)                               \
    if (_n > max_)                                                \
    {                                                             \
        if (max_ == 0)                                            \
        {                                                         \
            ptr_ = (debris_t *)malloc(n * sizeof(*ptr_));         \
        }                                                         \
        else                                                      \
        {                                                         \
            ptr_ = (debris_t *)realloc(ptr_, _n * sizeof(*ptr_)); \
        }                                                         \
        if (ptr_ == NULL)                                         \
        {                                                         \
            xperror("No memory for debris");                      \
            num_ = max_ = 0;                                      \
            return -1;                                            \
        }                                                         \
        max_ = _n;                                                \
    }                                                             \
    else if (_n <= 0)                                             \
    {                                                             \
        printf("debris %d < 0\n", _n);                            \
        return 0;                                                 \
    }                                                             \
    num_ = _n;                                                    \
    memcpy(ptr_, _p, _n * sizeof(*ptr_));                         \
    return 0;

int Handle_fastshot(int type, u_byte *p, int n)
{
#define num_ (num_fastshot[type])
#define max_ (max_fastshot[type])
#define ptr_ (fastshot_ptr[type])
    STORE_DEBRIS(type, p, n);
#undef num_
#undef max_
#undef ptr_
}

int Handle_debris(int type, u_byte *p, int n)
{
#define num_ (num_debris[type])
#define max_ (max_debris[type])
#define ptr_ (debris_ptr[type])
    STORE_DEBRIS(type, p, n);
#undef num_
#undef max_
#undef ptr_
}

int Handle_wreckage(int x, int y, int wrecktype, int size, int rotation)
{
    wreckage_t t;

    t.x = x;
    t.y = y;
    t.wrecktype = wrecktype;
    t.size = size;
    t.rotation = rotation;
    STORE(wreckage_t, wreckage_ptr, num_wreckage, max_wreckage, t);
    return 0;
}

int Handle_asteroid(int x, int y, int type, int size, int rotation)
{
    asteroid_t t;

    t.x = x;
    t.y = y;
    t.type = type;
    t.size = size;
    t.rotation = rotation;
    STORE(asteroid_t, asteroid_ptr, num_asteroids, max_asteroids, t);
    return 0;
}

int Handle_wormhole(int x, int y)
{
    wormhole_t t;

    t.x = x - BLOCK_SZ / 2;
    t.y = y - BLOCK_SZ / 2;
    STORE(wormhole_t, wormhole_ptr, num_wormholes, max_wormholes, t);
    return 0;
}

int Handle_ecm(int x, int y, int size)
{
    ecm_t t;

    t.x = x;
    t.y = y;
    t.size = size;
    STORE(ecm_t, ecm_ptr, num_ecm, max_ecm, t);
    return 0;
}

int Handle_trans(int x1, int y1, int x2, int y2)
{
    trans_t t;

    t.x1 = x1;
    t.y1 = y1;
    t.x2 = x2;
    t.y2 = y2;
    STORE(trans_t, trans_ptr, num_trans, max_trans, t);
    return 0;
}

int Handle_paused(int x, int y, int count)
{
    paused_t t;

    t.x = x;
    t.y = y;
    t.count = count;
    STORE(paused_t, paused_ptr, num_paused, max_paused, t);
    return 0;
}

int Handle_radar(int x, int y, int size)
{
    radar_t t;

    t.x = x;
    t.y = y;
    t.size = size;
    STORE(radar_t, radar_ptr, num_radar, max_radar, t);
    return 0;
}

int Handle_message(char *msg)
{
    Add_message(msg);
    return 0;
}

// int Handle_time_left(long sec)
// {
//     if (sec >= 0 && sec < 10 && (time_left > sec || sec == 0))
//     {
//         XBell(dpy, 0);
//         XFlush(dpy);
//     }
//     time_left = (sec >= 0) ? sec : 0;
//     return 0;
// }

int Handle_vcannon(int x, int y, int type)
{
    vcannon_t t;

    t.x = x;
    t.y = y;
    t.type = type;
    STORE(vcannon_t, vcannon_ptr, num_vcannon, max_vcannon, t);
    return 0;
}

int Handle_vfuel(int x, int y, long fuel)
{
    vfuel_t t;

    t.x = x;
    t.y = y;
    t.fuel = fuel;
    STORE(vfuel_t, vfuel_ptr, num_vfuel, max_vfuel, t);
    return 0;
}

int Handle_vbase(int x, int y, int xi, int yi, int type)
{
    vbase_t t;

    t.x = x;
    t.y = y;
    t.xi = xi;
    t.yi = yi;
    t.type = type;
    STORE(vbase_t, vbase_ptr, num_vbase, max_vbase, t);
    return 0;
}

int Handle_vdecor(int x, int y, int xi, int yi, int type)
{
    vdecor_t t;

    t.x = x;
    t.y = y;
    t.xi = xi;
    t.yi = yi;
    t.type = type;
    STORE(vdecor_t, vdecor_ptr, num_vdecor, max_vdecor, t);
    return 0;
}

static fuelstation_t *Fuelstation_by_pos(int x, int y)
{
    int i, lo, hi, pos;

    lo = 0;
    hi = num_fuels - 1;
    pos = x * Setup->y + y;
    while (lo < hi)
    {
        i = (lo + hi) >> 1;
        if (pos > fuels[i].pos)
        {
            lo = i + 1;
        }
        else
        {
            hi = i;
        }
    }
    if (lo == hi && pos == fuels[lo].pos)
    {
        return &fuels[lo];
    }
    errno = 0;
    xperror("No fuelstation at (%d,%d)", x, y);
    return NULL;
}

int Fuel_by_pos(int x, int y)
{
    fuelstation_t *fuelp;

    if ((fuelp = Fuelstation_by_pos(x, y)) == NULL)
    {
        return 0;
    }
    return fuelp->fuel;
}

int Target_by_index(int ind, int *xp, int *yp, int *dead_time, int *damage)
{
    if (ind < 0 || ind >= num_targets)
    {
        return -1;
    }
    *xp = targets[ind].pos / Setup->y;
    *yp = targets[ind].pos % Setup->y;
    *dead_time = targets[ind].dead_time;
    *damage = targets[ind].damage;
    return 0;
}

int Target_alive(int x, int y, int *damage)
{
    int i, lo, hi, pos;

    lo = 0;
    hi = num_targets - 1;
    pos = x * Setup->y + y;
    while (lo < hi)
    {
        i = (lo + hi) >> 1;
        if (pos > targets[i].pos)
        {
            lo = i + 1;
        }
        else
        {
            hi = i;
        }
    }
    if (lo == hi && pos == targets[lo].pos)
    {
        *damage = targets[lo].damage;
        return targets[lo].dead_time;
    }
    errno = 0;
    xperror("No targets at (%d,%d)", x, y);
    return -1;
}

int Handle_fuel(int ind, int fuel)
{
    if (ind < 0 || ind >= num_fuels)
    {
        errno = 0;
        xperror("Bad fuelstation index (%d)", ind);
        return -1;
    }
    fuels[ind].fuel = fuel;
    return 0;
}

static cannontime_t *Cannon_by_pos(int x, int y)
{
    int i, lo, hi, pos;

    lo = 0;
    hi = num_cannons - 1;
    pos = x * Setup->y + y;
    while (lo < hi)
    {
        i = (lo + hi) >> 1;
        if (pos > cannons[i].pos)
        {
            lo = i + 1;
        }
        else
        {
            hi = i;
        }
    }
    if (lo == hi && pos == cannons[lo].pos)
    {
        return &cannons[lo];
    }
    errno = 0;
    xperror("No cannon at (%d,%d)", x, y);
    return NULL;
}

int Cannon_dead_time_by_pos(int x, int y, int *dot)
{
    cannontime_t *cannonp;

    if ((cannonp = Cannon_by_pos(x, y)) == NULL)
    {
        return -1;
    }
    *dot = cannonp->dot;
    return cannonp->dead_time;
}

int Handle_cannon(int ind, int dead_time)
{
    if (ind < 0 || ind >= num_cannons)
    {
        errno = 0;
        xperror("Bad cannon index (%d)", ind);
        return 0;
    }
    cannons[ind].dead_time = dead_time;
    return 0;
}

int Handle_target(int num, int dead_time, int damage)
{
    if (num < 0 || num >= num_targets)
    {
        errno = 0;
        xperror("Bad target index (%d)", num);
        return 0;
    }
    if (dead_time == 0 && (damage < 1 || damage > TARGET_DAMAGE))
    {
        printf("BUG target %d, dead %d, damage %d\n", num, dead_time, damage);
    }
    if (targets[num].dead_time > 0 && dead_time == 0)
    {
        int pos = targets[num].pos;
        Radar_show_target(pos / Setup->y, pos % Setup->y);
    }
    else if (targets[num].dead_time == 0 && dead_time > 0)
    {
        int pos = targets[num].pos;
        Radar_hide_target(pos / Setup->y, pos % Setup->y);
    }

    targets[num].dead_time = dead_time;
    targets[num].damage = damage;

    return 0;
}

static homebase_t *Homebase_by_pos(int x, int y)
{
    int i, lo, hi, pos;

    lo = 0;
    hi = num_bases - 1;
    pos = x * Setup->y + y;
    while (lo < hi)
    {
        i = (lo + hi) >> 1;
        if (pos > bases[i].pos)
        {
            lo = i + 1;
        }
        else
        {
            hi = i;
        }
    }
    if (lo == hi && pos == bases[lo].pos)
    {
        return &bases[lo];
    }
    errno = 0;
    xperror("No homebase at (%d,%d)", x, y);
    return NULL;
}

int Base_info_by_pos(int x, int y, int *idp, int *teamp)
{
    homebase_t *basep;

    if ((basep = Homebase_by_pos(x, y)) == NULL)
    {
        return -1;
    }
    *idp = basep->id;
    *teamp = basep->team;
    return 0;
}

int Handle_base(int id, int ind)
{
    int i;

    if (ind < 0 || ind >= num_bases)
    {
        errno = 0;
        xperror("Bad homebase index (%d)", ind);
        return -1;
    }
    for (i = 0; i < num_bases; i++)
    {
        if (bases[i].id == id)
        {
            bases[i].id = -1;
        }
    }
    bases[ind].id = id;

    return 0;
}

int Check_pos_by_index(int ind, int *xp, int *yp)
{
    if (ind < 0 || ind >= MAX_CHECKPOINT)
    {
        errno = 0;
        xperror("Bad checkpoint index (%d)", ind);
        *xp = 0;
        *yp = 0;
        return -1;
    }
    *xp = checks[ind].pos / Setup->y;
    *yp = checks[ind].pos % Setup->y;
    return 0;
}

int Check_index_by_pos(int x, int y)
{
    int i, pos;

    pos = x * Setup->y + y;
    for (i = 0; i < MAX_CHECKPOINT; i++)
    {
        if (pos == checks[i].pos)
        {
            return i;
        }
    }
    errno = 0;
    xperror("Can't find checkpoint (%d,%d)", x, y);
    return 0;
}

/*
 * Convert a `space' map block into a dot.
 */
static void Map_make_dot(unsigned char *data)
{
    if (*data == SETUP_SPACE)
    {
        *data = SETUP_SPACE_DOT;
    }
    else if (*data == SETUP_DECOR_FILLED)
    {
        *data = SETUP_DECOR_DOT_FILLED;
    }
    else if (*data == SETUP_DECOR_RU)
    {
        *data = SETUP_DECOR_DOT_RU;
    }
    else if (*data == SETUP_DECOR_RD)
    {
        *data = SETUP_DECOR_DOT_RD;
    }
    else if (*data == SETUP_DECOR_LU)
    {
        *data = SETUP_DECOR_DOT_LU;
    }
    else if (*data == SETUP_DECOR_LD)
    {
        *data = SETUP_DECOR_DOT_LD;
    }
}

/*
 * Optimize the drawing of all blue space dots by converting
 * certain map objects into a specialised form of their type.
 */
void Map_dots(void)
{
    int i,
        x,
        y,
        start;
    unsigned char dot[256];

    /*
     * Lookup table to recognize dots.
     */
    memset(dot, 0, sizeof dot);
    dot[SETUP_SPACE_DOT] = 1;
    dot[SETUP_DECOR_DOT_FILLED] = 1;
    dot[SETUP_DECOR_DOT_RU] = 1;
    dot[SETUP_DECOR_DOT_RD] = 1;
    dot[SETUP_DECOR_DOT_LU] = 1;
    dot[SETUP_DECOR_DOT_LD] = 1;

    /*
     * Restore the map to unoptimized form.
     */
    for (i = Setup->x * Setup->y; i-- > 0;)
    {
        if (dot[Setup->map_data[i]])
        {
            if (Setup->map_data[i] == SETUP_SPACE_DOT)
            {
                Setup->map_data[i] = SETUP_SPACE;
            }
            else if (Setup->map_data[i] == SETUP_DECOR_DOT_FILLED)
            {
                Setup->map_data[i] = SETUP_DECOR_FILLED;
            }
            else if (Setup->map_data[i] == SETUP_DECOR_DOT_RU)
            {
                Setup->map_data[i] = SETUP_DECOR_RU;
            }
            else if (Setup->map_data[i] == SETUP_DECOR_DOT_RD)
            {
                Setup->map_data[i] = SETUP_DECOR_RD;
            }
            else if (Setup->map_data[i] == SETUP_DECOR_DOT_LU)
            {
                Setup->map_data[i] = SETUP_DECOR_LU;
            }
            else if (Setup->map_data[i] == SETUP_DECOR_DOT_LD)
            {
                Setup->map_data[i] = SETUP_DECOR_LD;
            }
        }
    }

    /*
     * Lookup table to test for map data which can be turned into a dot.
     */
    memset(dot, 0, sizeof dot);
    dot[SETUP_SPACE] = 1;
    if (!BIT(instruments, SHOW_DECOR))
    {
        dot[SETUP_DECOR_FILLED] = 1;
        dot[SETUP_DECOR_RU] = 1;
        dot[SETUP_DECOR_RD] = 1;
        dot[SETUP_DECOR_LU] = 1;
        dot[SETUP_DECOR_LD] = 1;
    }

    /*
     * Optimize.
     */
    if (map_point_size > 0)
    {
        if (BIT(Setup->mode, WRAP_PLAY))
        {
            for (x = 0; x < Setup->x; x++)
            {
                if (dot[Setup->map_data[x * Setup->y]])
                {
                    Map_make_dot(&Setup->map_data[x * Setup->y]);
                }
            }
            for (y = 0; y < Setup->y; y++)
            {
                if (dot[Setup->map_data[y]])
                {
                    Map_make_dot(&Setup->map_data[y]);
                }
            }
            start = map_point_distance;
        }
        else
        {
            start = 0;
        }
        if (map_point_distance > 0)
        {
            for (x = start; x < Setup->x; x += map_point_distance)
            {
                for (y = start; y < Setup->y; y += map_point_distance)
                {
                    if (dot[Setup->map_data[x * Setup->y + y]])
                    {
                        Map_make_dot(&Setup->map_data[x * Setup->y + y]);
                    }
                }
            }
        }
        for (i = 0; i < num_cannons; i++)
        {
            x = cannons[i].pos / Setup->y;
            y = cannons[i].pos % Setup->y;
            if ((x == 0 || y == 0) && BIT(Setup->mode, WRAP_PLAY))
            {
                cannons[i].dot = 1;
            }
            else if (map_point_distance > 0 && x % map_point_distance == 0 && y % map_point_distance == 0)
            {
                cannons[i].dot = 1;
            }
            else
            {
                cannons[i].dot = 0;
            }
        }
    }
}

/*
 * Optimize the drawing of all blue map objects by converting
 * their map type to a bitmask with bits for each blue segment.
 */
void Map_restore(int startx, int starty, int width, int height)
{
    int i, j,
        x, y,
        map_index,
        type;

    /*
     * Restore an optimized map to its original unoptimized state.
     */
    x = startx;
    for (i = 0; i < width; i++, x++)
    {
        if (x < 0)
        {
            x += Setup->x;
        }
        else if (x >= Setup->x)
        {
            x -= Setup->x;
        }

        y = starty;
        for (j = 0; j < height; j++, y++)
        {
            if (y < 0)
            {
                y += Setup->y;
            }
            else if (y >= Setup->y)
            {
                y -= Setup->y;
            }

            map_index = x * Setup->y + y;

            type = Setup->map_data[map_index];
            if ((type & BLUE_BIT) == 0)
            {
                if (type == SETUP_FILLED_NO_DRAW)
                {
                    Setup->map_data[map_index] = SETUP_FILLED;
                }
            }
            else if ((type & BLUE_FUEL) == BLUE_FUEL)
            {
                Setup->map_data[map_index] = SETUP_FUEL;
            }
            else if (type & BLUE_OPEN)
            {
                if (type & BLUE_BELOW)
                {
                    Setup->map_data[map_index] = SETUP_REC_RD;
                }
                else
                {
                    Setup->map_data[map_index] = SETUP_REC_LU;
                }
            }
            else if (type & BLUE_CLOSED)
            {
                if (type & BLUE_BELOW)
                {
                    Setup->map_data[map_index] = SETUP_REC_LD;
                }
                else
                {
                    Setup->map_data[map_index] = SETUP_REC_RU;
                }
            }
            else
            {
                Setup->map_data[map_index] = SETUP_FILLED;
            }
        }
    }
}

void Map_blue(int startx, int starty, int width, int height)
{
    int i, j,
        x, y,
        map_index,
        type,
        newtype;
    unsigned char blue[256];
    const long outline_mask = SHOW_OUTLINE_WORLD | SHOW_FILLED_WORLD | SHOW_TEXTURED_WALLS;

    /*
     * Optimize the map for blue.
     */
    memset(blue, 0, sizeof blue);
    blue[SETUP_FILLED] = BLUE_LEFT | BLUE_UP | BLUE_RIGHT | BLUE_DOWN;
    blue[SETUP_FILLED_NO_DRAW] = blue[SETUP_FILLED];
    blue[SETUP_FUEL] = blue[SETUP_FILLED];
    blue[SETUP_REC_RU] = BLUE_RIGHT | BLUE_UP;
    blue[SETUP_REC_RD] = BLUE_RIGHT | BLUE_DOWN;
    blue[SETUP_REC_LU] = BLUE_LEFT | BLUE_UP;
    blue[SETUP_REC_LD] = BLUE_LEFT | BLUE_DOWN;
    blue[BLUE_BIT | BLUE_OPEN] =
        blue[BLUE_BIT | BLUE_OPEN | BLUE_LEFT] =
            blue[BLUE_BIT | BLUE_OPEN | BLUE_UP] =
                blue[BLUE_BIT | BLUE_OPEN | BLUE_LEFT | BLUE_UP] =
                    blue[SETUP_REC_LU];
    blue[BLUE_BIT | BLUE_OPEN | BLUE_BELOW] =
        blue[BLUE_BIT | BLUE_OPEN | BLUE_BELOW | BLUE_RIGHT] =
            blue[BLUE_BIT | BLUE_OPEN | BLUE_BELOW | BLUE_DOWN] =
                blue[BLUE_BIT | BLUE_OPEN | BLUE_BELOW | BLUE_RIGHT | BLUE_DOWN] =
                    blue[SETUP_REC_RD];
    blue[BLUE_BIT | BLUE_CLOSED] =
        blue[BLUE_BIT | BLUE_CLOSED | BLUE_RIGHT] =
            blue[BLUE_BIT | BLUE_CLOSED | BLUE_UP] =
                blue[BLUE_BIT | BLUE_CLOSED | BLUE_RIGHT | BLUE_UP] =
                    blue[SETUP_REC_RU];
    blue[BLUE_BIT | BLUE_CLOSED | BLUE_BELOW] =
        blue[BLUE_BIT | BLUE_CLOSED | BLUE_BELOW | BLUE_LEFT] =
            blue[BLUE_BIT | BLUE_CLOSED | BLUE_BELOW | BLUE_DOWN] =
                blue[BLUE_BIT | BLUE_CLOSED | BLUE_BELOW | BLUE_LEFT | BLUE_DOWN] =
                    blue[SETUP_REC_LD];
    for (i = BLUE_BIT; i < sizeof blue; i++)
    {
        if ((i & BLUE_FUEL) == BLUE_FUEL || (i & (BLUE_OPEN | BLUE_CLOSED)) == 0)
        {
            blue[i] = blue[SETUP_FILLED];
        }
    }

    x = startx;
    for (i = 0; i < width; i++, x++)
    {
        if (x < 0)
        {
            x += Setup->x;
        }
        else if (x >= Setup->x)
        {
            x -= Setup->x;
        }

        y = starty;
        for (j = 0; j < height; j++, y++)
        {
            if (y < 0)
            {
                y += Setup->y;
            }
            else if (y >= Setup->y)
            {
                y -= Setup->y;
            }

            map_index = x * Setup->y + y;

            type = Setup->map_data[map_index];
            newtype = 0;
            switch (type)
            {
            case SETUP_FILLED:
            case SETUP_FILLED_NO_DRAW:
            case SETUP_FUEL:
                newtype = BLUE_BIT;
                if (type == SETUP_FUEL)
                {
                    newtype |= BLUE_FUEL;
                }
                if ((x == 0)
                        ? (!BIT(Setup->mode, WRAP_PLAY) ||
                           !(blue[Setup->map_data[(Setup->x - 1) * Setup->y + y]] & BLUE_RIGHT))
                        : !(blue[Setup->map_data[(x - 1) * Setup->y + y]] & BLUE_RIGHT))
                    newtype |= BLUE_LEFT;
                if ((y == 0)
                        ? (!BIT(Setup->mode, WRAP_PLAY) ||
                           !(blue[Setup->map_data[x * Setup->y + Setup->y - 1]] & BLUE_UP))
                        : !(blue[Setup->map_data[x * Setup->y + (y - 1)]] & BLUE_UP))
                    newtype |= BLUE_DOWN;
                if (!BIT(instruments, outline_mask) || ((x == Setup->x - 1)
                                                            ? (!BIT(Setup->mode, WRAP_PLAY) || !(blue[Setup->map_data[y]] & BLUE_LEFT))
                                                            : !(blue[Setup->map_data[(x + 1) * Setup->y + y]] & BLUE_LEFT)))
                    newtype |= BLUE_RIGHT;
                if (!BIT(instruments, outline_mask) || ((y == Setup->y - 1)
                                                            ? (!BIT(Setup->mode, WRAP_PLAY) || !(blue[Setup->map_data[x * Setup->y]] & BLUE_DOWN))
                                                            : !(blue[Setup->map_data[x * Setup->y + (y + 1)]] & BLUE_DOWN)))
                    newtype |= BLUE_UP;
                break;

            case SETUP_REC_LU:
                newtype = BLUE_BIT | BLUE_OPEN;
                if (x == 0
                        ? (!BIT(Setup->mode, WRAP_PLAY) ||
                           !(blue[Setup->map_data[(Setup->x - 1) * Setup->y + y]] & BLUE_RIGHT))
                        : !(blue[Setup->map_data[(x - 1) * Setup->y + y]] & BLUE_RIGHT))
                    newtype |= BLUE_LEFT;
                if (!BIT(instruments, outline_mask) || ((y == Setup->y - 1)
                                                            ? (!BIT(Setup->mode, WRAP_PLAY) || !(blue[Setup->map_data[x * Setup->y]] & BLUE_DOWN))
                                                            : !(blue[Setup->map_data[x * Setup->y + (y + 1)]] & BLUE_DOWN)))
                    newtype |= BLUE_UP;
                break;

            case SETUP_REC_RU:
                newtype = BLUE_BIT | BLUE_CLOSED;
                if (!BIT(instruments, outline_mask) || ((x == Setup->x - 1)
                                                            ? (!BIT(Setup->mode, WRAP_PLAY) || !(blue[Setup->map_data[y]] & BLUE_LEFT))
                                                            : !(blue[Setup->map_data[(x + 1) * Setup->y + y]] & BLUE_LEFT)))
                    newtype |= BLUE_RIGHT;
                if (!BIT(instruments, outline_mask) || ((y == Setup->y - 1)
                                                            ? (!BIT(Setup->mode, WRAP_PLAY) || !(blue[Setup->map_data[x * Setup->y]] & BLUE_DOWN))
                                                            : !(blue[Setup->map_data[x * Setup->y + (y + 1)]] & BLUE_DOWN)))
                    newtype |= BLUE_UP;
                break;

            case SETUP_REC_LD:
                newtype = BLUE_BIT | BLUE_BELOW | BLUE_CLOSED;
                if ((x == 0)
                        ? (!BIT(Setup->mode, WRAP_PLAY) ||
                           !(blue[Setup->map_data[(Setup->x - 1) * Setup->y + y]] & BLUE_RIGHT))
                        : !(blue[Setup->map_data[(x - 1) * Setup->y + y]] & BLUE_RIGHT))
                    newtype |= BLUE_LEFT;
                if ((y == 0)
                        ? (!BIT(Setup->mode, WRAP_PLAY) ||
                           !(blue[Setup->map_data[x * Setup->y + Setup->y - 1]] & BLUE_UP))
                        : !(blue[Setup->map_data[x * Setup->y + (y - 1)]] & BLUE_UP))
                    newtype |= BLUE_DOWN;
                break;

            case SETUP_REC_RD:
                newtype = BLUE_BIT | BLUE_BELOW | BLUE_OPEN;
                if (!BIT(instruments, outline_mask) || ((x == Setup->x - 1)
                                                            ? (!BIT(Setup->mode, WRAP_PLAY) || !(blue[Setup->map_data[y]] & BLUE_LEFT))
                                                            : !(blue[Setup->map_data[(x + 1) * Setup->y + y]] & BLUE_LEFT)))
                    newtype |= BLUE_RIGHT;
                if ((y == 0)
                        ? (!BIT(Setup->mode, WRAP_PLAY) ||
                           !(blue[Setup->map_data[x * Setup->y + Setup->y - 1]] & BLUE_UP))
                        : !(blue[Setup->map_data[x * Setup->y + (y - 1)]] & BLUE_UP))
                    newtype |= BLUE_DOWN;
                break;

            default:
                continue;
            }
            if (newtype != 0)
            {
                if (newtype == BLUE_BIT)
                {
                    newtype = SETUP_FILLED_NO_DRAW;
                }
                Setup->map_data[map_index] = newtype;
            }
        }
    }
}

static int Map_init(void)
{
    int i,
        max,
        type;
    u_byte types[256];

    num_fuels = 0;
    num_bases = 0;
    num_cannons = 0;
    num_targets = 0;
    fuels = NULL;
    bases = NULL;
    cannons = NULL;
    targets = NULL;
    memset(types, 0, sizeof types);
    types[SETUP_FUEL] = 1;
    types[SETUP_CANNON_UP] = 2;
    types[SETUP_CANNON_RIGHT] = 2;
    types[SETUP_CANNON_DOWN] = 2;
    types[SETUP_CANNON_LEFT] = 2;
    for (i = SETUP_TARGET; i < SETUP_TARGET + 10; i++)
    {
        types[i] = 3;
    }
    for (i = SETUP_BASE_LOWEST; i <= SETUP_BASE_HIGHEST; i++)
    {
        types[i] = 4;
    }
    max = Setup->x * Setup->y;
    for (i = 0; i < max; i++)
    {
        switch (types[Setup->map_data[i]])
        {
        case 1:
            num_fuels++;
            break;
        case 2:
            num_cannons++;
            break;
        case 3:
            num_targets++;
            break;
        case 4:
            num_bases++;
            break;
        }
    }
    if (num_bases != 0)
    {
        bases = (homebase_t *)malloc(num_bases * sizeof(homebase_t));
        if (bases == NULL)
        {
            xperror("No memory for Map bases (%d)", num_bases);
            return -1;
        }
        num_bases = 0;
    }
    if (num_fuels != 0)
    {
        fuels = (fuelstation_t *)malloc(num_fuels * sizeof(fuelstation_t));
        if (fuels == NULL)
        {
            xperror("No memory for Map fuels (%d)", num_fuels);
            return -1;
        }
        num_fuels = 0;
    }
    if (num_targets != 0)
    {
        targets = (target_t *)malloc(num_targets * sizeof(target_t));
        if (targets == NULL)
        {
            xperror("No memory for Map targets (%d)", num_targets);
            return -1;
        }
        num_targets = 0;
    }
    if (num_cannons != 0)
    {
        cannons = (cannontime_t *)malloc(num_cannons * sizeof(cannontime_t));
        if (cannons == NULL)
        {
            xperror("No memory for Map cannons (%d)", num_cannons);
            return -1;
        }
        num_cannons = 0;
    }
    for (i = 0; i < MAX_CHECKPOINT; i++)
    {
        types[SETUP_CHECK + i] = 5;
    }
    for (i = 0; i < max; i++)
    {
        type = Setup->map_data[i];
        switch (types[type])
        {
        case 1:
            fuels[num_fuels].pos = i;
            fuels[num_fuels].fuel = MAX_STATION_FUEL;
            num_fuels++;
            break;
        case 2:
            cannons[num_cannons].pos = i;
            cannons[num_cannons].dead_time = 0;
            cannons[num_cannons].dot = 0;
            num_cannons++;
            break;
        case 3:
            targets[num_targets].pos = i;
            targets[num_targets].dead_time = 0;
            targets[num_targets].damage = TARGET_DAMAGE;
            num_targets++;
            break;
        case 4:
            bases[num_bases].pos = i;
            bases[num_bases].id = -1;
            bases[num_bases].team = type % 10;
            num_bases++;
            Setup->map_data[i] = type - (type % 10);
            break;
        case 5:
            checks[type - SETUP_CHECK].pos = i;
            Setup->map_data[i] = SETUP_CHECK;
            break;
        }
    }
    return 0;
}

static int Map_cleanup(void)
{
    if (num_bases > 0)
    {
        if (bases != NULL)
        {
            free(bases);
            bases = NULL;
        }
        num_bases = 0;
    }
    if (num_fuels > 0)
    {
        if (fuels != NULL)
        {
            free(fuels);
            fuels = NULL;
        }
        num_fuels = 0;
    }
    if (num_targets > 0)
    {
        if (targets != NULL)
        {
            free(targets);
            targets = NULL;
        }
        num_targets = 0;
    }
    if (num_cannons > 0)
    {
        if (cannons != NULL)
        {
            free(cannons);
            cannons = NULL;
        }
        num_cannons = 0;
    }
    return 0;
}

other_t *Other_by_id(int id)
{
    int i;

    if (id != -1)
    {
        for (i = 0; i < num_others; i++)
        {
            if (Others[i].id == id)
            {
                return &Others[i];
            }
        }
    }
    return NULL;
}

shipobj *Ship_by_id(int id)
{
    other_t *other;

    if ((other = Other_by_id(id)) == NULL)
    {
        return Parse_shape_str(NULL);
    }
    return other->ship;
}

int Handle_leave(int id)
{
    other_t *other;
    int i;
    char msg[MSG_LEN];

    if ((other = Other_by_id(id)) != NULL)
    {
        if (other == self)
        {
            errno = 0;
            xperror("Self left?!");
            self = NULL;
        }
        Free_ship_shape(other->ship);
        other->ship = NULL;
        /*
         * Silent about tanks and robots.
         */
        if (other->mychar != 'T' && other->mychar != 'R')
        {
            sprintf(msg, "%s left this world.", other->name);
            Add_message(msg);
        }
        num_others--;
        while (other < &Others[num_others])
        {
            *other = other[1];
            other++;
        }
        scoresChanged = 1;
    }
    for (i = 0; i < num_others; i++)
    {
        other = &Others[i];
        if (other->war_id == id)
        {
            other->war_id = -1;
            scoresChanged = 1;
        }
    }
    return 0;
}

int Handle_player(int id, int player_team, int mychar, char *player_name,
                  char *real_name, char *host_name, char *shape)
{
    other_t *other;

    if ((other = Other_by_id(id)) == NULL)
    {
        if (num_others >= max_others)
        {
            max_others += 5;
            if (num_others == 0)
            {
                Others = (other_t *)malloc(max_others * sizeof(other_t));
            }
            else
            {
                Others = (other_t *)realloc(Others,
                                            max_others * sizeof(other_t));
            }
            if (Others == NULL)
            {
                xperror("Not enough memory for player info");
                num_others = max_others = 0;
                self = NULL;
                return -1;
            }
            if (self != NULL)
            {
                /*
                 * We've made `self' the first member of Others[].
                 */
                self = &Others[0];
            }
        }
        other = &Others[num_others++];
    }
    if (self == NULL && strcmp(name, player_name) == 0)
    {
        if (other != &Others[0])
        {
            /*
             * Make `self' the first member of Others[].
             */
            *other = Others[0];
            other = &Others[0];
        }
        self = other;
        team = player_team;
    }
    other->id = id;
    other->team = player_team;
    other->score = 0;
    other->round = 0;
    other->check = 0;
    other->timing = 0;
    other->life = 0;
    other->mychar = mychar;
    other->war_id = -1;
    other->name_width = 0;
    strlcpy(other->name, player_name, sizeof(other->name));
    strlcpy(other->real, real_name, sizeof(other->real));
    strlcpy(other->host, host_name, sizeof(other->host));
    scoresChanged = 1;
    other->ship = Convert_shape_str(shape);
    Calculate_shield_radius(other->ship);

    return 0;
}

int Handle_war(int robot_id, int killer_id)
{
    other_t *robot,
        *killer;
    char msg[MSG_LEN];

    if ((robot = Other_by_id(robot_id)) == NULL)
    {
        errno = 0;
        xperror("Can't update war for non-existing player (%d,%d)", robot_id, killer_id);
        return 0;
    }
    if (killer_id == -1)
    {
        /*
         * Robot is no longer in war mode.
         */
        robot->war_id = -1;
        return 0;
    }
    if ((killer = Other_by_id(killer_id)) == NULL)
    {
        errno = 0;
        xperror("Can't update war against non-existing player (%d,%d)", robot_id, killer_id);
        return 0;
    }
    robot->war_id = killer_id;
    sprintf(msg, "%s declares war on %s.", robot->name, killer->name);
    Add_message(msg);
    scoresChanged = 1;

    return 0;
}

int Handle_seek(int programmer_id, int robot_id, int sought_id)
{
    other_t *programmer,
        *robot,
        *sought;
    char msg[MSG_LEN + 8];

    if ((programmer = Other_by_id(programmer_id)) == NULL || (robot = Other_by_id(robot_id)) == NULL || (sought = Other_by_id(sought_id)) == NULL)
    {
        errno = 0;
        xperror("Bad player seek (%d,%d,%d)",
                programmer_id, robot_id, sought_id);
        return 0;
    }
    robot->war_id = sought_id;
    sprintf(msg, "%s has programmed %s to seek %s.",
            programmer->name, robot->name, sought->name);
    Add_message(msg);
    scoresChanged = 1;

    return 0;
}

int Handle_score(int id, int score, int life, int mychar, int alliance)
{
    other_t *other;

    if ((other = Other_by_id(id)) == NULL)
    {
        errno = 0;
        xperror("Can't update score for non-existing player %d,%d,%d",
                id, score, life);
        return 0;
    }
    else if (other->score != score || other->life != life || other->mychar != mychar || other->alliance != alliance)
    {
        other->score = score;
        other->life = life;
        other->mychar = mychar;
        other->alliance = alliance;
        scoresChanged = 1;
    }

    return 0;
}

int Handle_timing(int id, int check, int round)
{
    other_t *other;

    if ((other = Other_by_id(id)) == NULL)
    {
        errno = 0;
        xperror("Can't update timing for non-existing player %d,%d,%d", id, check, round);
        return 0;
    }
    else if (other->check != check || other->round != round)
    {
        other->check = check;
        other->round = round;
        other->timing = round * MAX_CHECKS + check;
        other->timing_loops = last_loops;
        scoresChanged = 1;
    }

    return 0;
}

int Handle_score_object(int score, int x, int y, char *msg)
{
    score_object_t *sobj = &score_objects[score_object];

    sobj->score = score;
    sobj->x = x;
    sobj->y = y;
    sobj->count = 1;

    /* Initialize sobj->hud_msg (is shown on the HUD) */
    if (msg[0] != '\0')
    {
        sprintf(sobj->hud_msg, "%s %d", msg, score);
        sobj->hud_msg_len = strlen(sobj->hud_msg);
        // sobj->hud_msg_width = XTextWidth(gameFont,
        //                                  sobj->hud_msg, sobj->hud_msg_len);
        sobj->hud_msg_width = -1;
    }
    else
        sobj->hud_msg_len = 0;

    /* Initialize sobj->msg data (is shown on game area) */
    sprintf(sobj->msg, "%d", score);

    sobj->msg_len = strlen(sobj->msg);
    // sobj->msg_width = XTextWidth(gameFont, sobj->msg, sobj->msg_len);
    sobj->msg_width = -1;

    /* Update global index variable */
    score_object = (score_object + 1) % MAX_SCORE_OBJECTS;

    return 0;
}

void Client_score_table(void)
{
    struct team_score
    {
        int score;
        int life;
        int playing;
    };
    struct team_score team[MAX_TEAMS],
        *team_order[MAX_TEAMS];
    other_t *other,
        **order;
    int i, j, k, best = -1;
    DFLOAT ratio, best_ratio = -1e7;

    if (scoresChanged == 0)
    {
        return;
    }

    if (players_exposed == false)
    {
        return;
    }

    if (num_others < 1)
    {
        Paint_score_start();
        scoresChanged = 0;
        return;
    }

    if ((order = (other_t **)malloc(num_others * sizeof(other_t *))) == NULL)
    {
        xperror("No memory for score");
        return;
    }
    if (BIT(Setup->mode, TEAM_PLAY | TIMING) == TEAM_PLAY)
    {
        memset(&team[0], 0, sizeof team);
    }
    for (i = 0; i < num_others; i++)
    {
        other = &Others[i];
        if (BIT(Setup->mode, TIMING))
        {
            /*
             * Sort the score table on position in race.
             * Put paused and waiting players last as well as tanks.
             */
            if (strchr("PTW", other->mychar))
            {
                j = i;
            }
            else
            {
                for (j = 0; j < i; j++)
                {
                    if (order[j]->timing < other->timing)
                    {
                        break;
                    }
                    if (strchr("PTW", order[j]->mychar))
                    {
                        break;
                    }
                    if (order[j]->timing == other->timing)
                    {
                        if (order[j]->timing_loops > other->timing_loops)
                        {
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            if (BIT(Setup->mode, LIMITED_LIVES))
            {
                ratio = (float)other->score;
            }
            else
            {
                ratio = (float)other->score / (other->life + 1);
            }
            if (best == -1 || ratio > best_ratio)
            {
                best_ratio = ratio;
                best = i;
            }
            for (j = 0; j < i; j++)
            {
                if (order[j]->score < other->score)
                {
                    break;
                }
            }
        }
        for (k = i; k > j; k--)
        {
            order[k] = order[k - 1];
        }
        order[j] = other;

        if (BIT(Setup->mode, TEAM_PLAY | TIMING) == TEAM_PLAY)
        {
            switch (other->mychar)
            {
            case 'P':
            case 'W':
            case 'T':
                break;
            case ' ':
            case 'R':
                if (BIT(Setup->mode, LIMITED_LIVES))
                {
                    team[other->team].life += other->life + 1;
                }
                else
                {
                    team[other->team].life += other->life;
                }
                /*FALLTHROUGH*/
            default:
                team[other->team].playing++;
                team[other->team].score += other->score;
                break;
            }
        }
    }
    Paint_score_start();
    if (BIT(Setup->mode, TIMING))
    {
        best = order[0] - Others;
    }
    for (i = 0; i < num_others; i++)
    {
        other = order[i];
        j = other - Others;
        Paint_score_entry(i, other, (j == best) ? true : false);
    }
    if (BIT(Setup->mode, TEAM_PLAY | TIMING) == TEAM_PLAY)
    {
        int pos = num_others + 1;
        int num_playing_teams = 0;
        for (i = 0; i < MAX_TEAMS; i++)
        {
            if (team[i].playing)
            {
                for (j = 0; j < num_playing_teams; j++)
                {
                    if (team[i].score > team_order[j]->score || (team[i].score == team_order[j]->score && ((BIT(Setup->mode, LIMITED_LIVES))
                                                                                                               ? (team[i].life > team_order[j]->life)
                                                                                                               : (team[i].life < team_order[j]->life))))
                    {
                        for (k = i; k > j; k--)
                        {
                            team_order[k] = team_order[k - 1];
                        }
                        break;
                    }
                }
                team_order[j] = &team[i];
                num_playing_teams++;
            }
        }
        for (i = 0; i < num_playing_teams; i++)
        {
            other_t tmp;
            tmp.id = -1;
            tmp.team = team_order[i] - &team[0];
            tmp.war_id = -1;
            tmp.name_width = 0;
            tmp.ship = NULL;
            sprintf(tmp.name, "Team %d", tmp.team);
            strcpy(tmp.real, tmp.name);
            strcpy(tmp.host, "");
            if (BIT(Setup->mode, LIMITED_LIVES) && team_order[i]->life == 0)
            {
                tmp.mychar = 'D';
            }
            else
            {
                tmp.mychar = ' ';
            }
            tmp.score = team_order[i]->score;
            tmp.life = team_order[i]->life;
            Paint_score_entry(pos++, &tmp, false);
        }
    }

    free(order);

    scoresChanged = 0;
}

static int Alloc_history(void)
{
    char *hist_ptr;
    int i;

    /* maxLinesInHistory is a runtime constant */
    if ((hist_ptr = (char *)malloc(maxLinesInHistory * MAX_CHARS)) == NULL)
    {
        xperror("No memory for history");
        return -1;
    }
    HistoryBlock = hist_ptr;

    for (i = 0; i < maxLinesInHistory; i++)
    {
        HistoryMsg[i] = hist_ptr;
        hist_ptr[0] = '\0';
        hist_ptr += MAX_CHARS;
    }
    return 0;
}

static void Free_selectionAndHistory(void)
{
    if (HistoryBlock)
    {
        free(HistoryBlock);
        HistoryBlock = NULL;
    }
    if (selection.txt)
    {
        free(selection.txt);
        selection.txt = NULL;
    }
}

int Client_init(char *server, unsigned server_version)
{
    version = server_version;

    Make_table();
    Init_scale_array();

    if (Init_wreckage() == -1)
    {
        return -1;
    }

    if (Init_asteroids() == -1)
    {
        return -1;
    }

    strlcpy(servername, server, sizeof(servername));

    return 0;
}

int Client_setup(void)
{
    if (Map_init() == -1)
    {
        return -1;
    }
    Map_dots();
    Map_restore(0, 0, Setup->x, Setup->y);
    Map_blue(0, 0, Setup->x, Setup->y);

    RadarHeight = (RadarWidth * Setup->y) / Setup->x;

    if (Init_playing_windows() == -1)
    {
        return -1;
    }
    if (Alloc_msgs() == -1)
    {
        return -1;
    }
    if (Alloc_history() == -1)
    {
        return -1;
    }

    /* Old servers can't deal with 0.0 turnresistance, so swap to
     * the alternate bank, and hope there's something better there. */
    /* HACK: Hanging Gardens runs an old server (version code 0x4101)
     * which happens to have the turnresistance patch. */
    if (turnresistance == 0.0 && version < 0x4200 && version != 0x4101)
    {
        DFLOAT tmp;
#define SWAP(a, b) (tmp = (a), (a) = (b), (b) = tmp)
        SWAP(power, power_s);
        SWAP(turnspeed, turnspeed_s);
        SWAP(turnresistance, turnresistance_s);
#undef SWAP
        control_count = CONTROL_DELAY;
        Add_message("Old server can't handle turnResistance=0.0; "
                    "swapping to alternate settings [*Client message*]");
    }

    return 0;
}

int Client_fps_request(void)
{
    LIMIT(maxFPS, 1, MAX_SUPPORTED_FPS);
    oldMaxFPS = maxFPS;
    return Send_fps_request(maxFPS);
}

int Check_client_fps(void)
{
    if (oldMaxFPS != maxFPS)
    {
        return Client_fps_request();
    }
    return 0;
}

int Client_power(void)
{
    int i;

    if (Send_power(power) == -1 ||
        Send_power_s(power_s) == -1 ||
        Send_turnspeed(turnspeed) == -1 ||
        Send_turnspeed_s(turnspeed_s) == -1 ||
        Send_turnresistance(turnresistance) == -1 ||
        Send_turnresistance_s(turnresistance_s) == -1 ||
        Send_display() == -1 ||
        Startup_server_motd() == -1)
    {
        return -1;
    }
    for (i = 0; i < NUM_MODBANKS; i++)
    {
        if (Send_modifier_bank(i) == -1)
        {
            return -1;
        }
    }

    return 0;
}

int Client_start(void)
{
    Key_init();

    return 0;
}

static void clientCleanup(void)
{
    if (max_refuel > 0 && refuel_ptr)
    {
        max_refuel = 0;
        free(refuel_ptr);
        refuel_ptr = 0;
    }
    if (max_connector > 0 && connector_ptr)
    {
        max_connector = 0;
        free(connector_ptr);
        connector_ptr = 0;
    }
    if (max_laser > 0 && laser_ptr)
    {
        max_laser = 0;
        free(laser_ptr);
        laser_ptr = 0;
    }
    if (max_missile > 0 && missile_ptr)
    {
        max_missile = 0;
        free(missile_ptr);
        missile_ptr = 0;
    }
    if (max_ball > 0 && ball_ptr)
    {
        max_ball = 0;
        free(ball_ptr);
        ball_ptr = 0;
    }
    if (max_ship > 0 && ship_ptr)
    {
        max_ship = 0;
        free(ship_ptr);
        ship_ptr = 0;
    }
    if (max_mine > 0 && mine_ptr)
    {
        max_mine = 0;
        free(mine_ptr);
        mine_ptr = 0;
    }
    if (max_ecm > 0 && ecm_ptr)
    {
        max_ecm = 0;
        free(ecm_ptr);
        ecm_ptr = 0;
    }
    if (max_trans > 0 && trans_ptr)
    {
        max_trans = 0;
        free(trans_ptr);
        trans_ptr = 0;
    }
    if (max_paused > 0 && paused_ptr)
    {
        max_paused = 0;
        free(paused_ptr);
        paused_ptr = 0;
    }
    if (max_radar > 0 && radar_ptr)
    {
        max_radar = 0;
        free(radar_ptr);
        radar_ptr = 0;
    }
    if (max_vcannon > 0 && vcannon_ptr)
    {
        max_vcannon = 0;
        free(vcannon_ptr);
        vcannon_ptr = 0;
    }
    if (max_vfuel > 0 && vfuel_ptr)
    {
        max_vfuel = 0;
        free(vfuel_ptr);
        vfuel_ptr = 0;
    }
    if (max_vbase > 0 && vbase_ptr)
    {
        max_vbase = 0;
        free(vbase_ptr);
        vbase_ptr = 0;
    }
    if (max_vdecor > 0 && vdecor_ptr)
    {
        max_vdecor = 0;
        free(vdecor_ptr);
        vdecor_ptr = 0;
    }
    if (max_itemtype > 0 && itemtype_ptr)
    {
        max_itemtype = 0;
        free(itemtype_ptr);
        itemtype_ptr = 0;
    }
    if (max_wreckage > 0 && wreckage_ptr)
    {
        max_wreckage = 0;
        free(wreckage_ptr);
        wreckage_ptr = 0;
    }
    if (max_asteroids > 0 && asteroid_ptr)
    {
        max_asteroids = 0;
        free(asteroid_ptr);
        asteroid_ptr = 0;
    }
    if (max_wormholes > 0 && wormhole_ptr)
    {
        max_wormholes = 0;
        free(wormhole_ptr);
        wormhole_ptr = 0;
    }
}

void Client_cleanup(void)
{
    int i;

    Platform_specific_cleanup();
    Free_selectionAndHistory();
    if (max_others > 0)
    {
        for (i = 0; i < num_others; i++)
        {
            other_t *other = &Others[i];
            Free_ship_shape(other->ship);
        }
        free(Others);
        num_others = 0;
        max_others = 0;
    }
    clientCleanup();
    Map_cleanup();
}

int Client_wrap_mode(void)
{
    return (BIT(Setup->mode, WRAP_PLAY) != 0);
}

void Init_scale_array(void)
{
    int i, start, end, n;
    double scaleMultFactor;

    if (scaleFactor == 0.0)
        scaleFactor = 1.0;
    if (scaleFactor < 0.1)
        scaleFactor = 0.1;
    if (scaleFactor > 10.0)
        scaleFactor = 10.0;
    scaleMultFactor = 1.0 / scaleFactor;

    scaleArray[0] = 0;

    for (i = 1; i < NELEM(scaleArray); i++)
    {
        n = (int)floor(i * scaleMultFactor + 0.5);
        if (n == 0)
        {
            /* keep values for non-zero indices at least 1. */
            scaleArray[i] = 1;
        }
        else
        {
            break;
        }
    }
    start = i;

    for (i = NELEM(scaleArray) - 1; i >= 0; i--)
    {
        n = (int)floor(i * scaleMultFactor + 0.5);
        if (n > 32767)
        {
            /* keep values lower or equal to max short. */
            scaleArray[i] = 32767;
        }
        else
        {
            break;
        }
    }
    end = i;

    for (i = start; i <= end; i++)
    {
        scaleArray[i] = (int)floor(i * scaleMultFactor + 0.5);
    }

    /* verify correct calculations, because of reported gcc optimization bugs. */
    for (i = 1; i < NELEM(scaleArray); i++)
    {
        if (scaleArray[i] < 1)
        {
            break;
        }
    }

    if (i != SCALE_ARRAY_SIZE)
    {
        fprintf(stderr,
                "Error: Illegal value %d in scaleArray[%d].\n"
                "\tThis error may be due to a bug in your compiler.\n"
                "\tEither try a lower optimization level,\n"
                "\tor a different compiler version or vendor.\n",
                scaleArray[i], i);
        exit(1);
    }
}
