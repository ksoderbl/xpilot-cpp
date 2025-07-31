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

#include "client.h"

ipos        pos;
ipos        vel;
ipos        world;
ipos        realWorld;
short        heading;
short        nextCheckPoint;

u_byte        numItems[NUM_ITEMS];        /* Count of currently owned items */
u_byte        lastNumItems[NUM_ITEMS];/* Last item count shown */
int        numItemsTime[NUM_ITEMS];/* Number of frames to show this item count */
DFLOAT        showItemsTime;                /* How long to show changed item count for */
short        autopilotLight;

short        lock_id;                /* Id of player locked onto */
short        lock_dir;                /* Direction of lock */
short        lock_dist;                /* Distance to player locked onto */

other_t     *self;          /* player info */
short        selfVisible;                /* Are we alive and playing? */
short        damaged;                /* Damaged by ECM */
short        destruct;                /* If self destructing */
short        shutdown_delay;
short        shutdown_count;
short        thrusttime;
short        thrusttimemax;
short        shieldtime;
short        shieldtimemax;
short        phasingtime;
short        phasingtimemax;

int                        RadarHeight = 0;
int                        RadarWidth = 256;        /* must always be 256! */
int        map_point_distance;        /* spacing of navigation points */
int        map_point_size;                /* size of navigation points */
int        spark_size;                /* size of debris and spark */
int        shot_size;                /* size of shot */
int        teamshot_size;                /* size of team shot */
bool        showNastyShots = false;                /* show original flavor shots or the new "nasty shots" */
long        control_count;                /* Display control for how long? */
int                roundDelay;                        /* != 0 means we're in a delay */
int                roundDelayMax;                /* (not yet) used for graph of time remaining in delay */

u_byte        spark_rand;                /* Sparkling effect */
u_byte        old_spark_rand;                /* previous value of spark_rand */

long        fuelSum;                        /* Sum of fuel in all tanks */
long        fuelMax;                        /* How much fuel can you take? */
short        fuelCurrent;                        /* Number of currently used tank */
short        numTanks;                        /* Number of tanks */
long        fuelCount;                        /* Display fuel for how long? */
int        fuelLevel1;                        /* Fuel critical level */
int        fuelLevel2;                        /* Fuel warning level */
int        fuelLevel3;                        /* Fuel notify level */

char        *shipShape;                /* Shape of player's ship */
DFLOAT        power;                        /* Force of thrust */
DFLOAT        power_s;                /* Saved power fiks */
DFLOAT        turnspeed;                /* How fast player acc-turns */
DFLOAT        turnspeed_s;                /* Saved turnspeed */
DFLOAT        turnresistance;                /* How much is lost in % */
DFLOAT        turnresistance_s;        /* Saved (see above) */
DFLOAT        displayedPower;                /* What the server is sending us */
DFLOAT        displayedTurnspeed;        /* What the server is sending us */
DFLOAT        displayedTurnresistance;/* What the server is sending us */
DFLOAT        spark_prob;                /* Sparkling effect user configurable */
int     charsPerSecond;         /* Message output speed (configurable) */

DFLOAT        hud_move_fact;                /* scale the hud-movement (speed) */
DFLOAT        ptr_move_fact;                /* scale the speed pointer length */
long        instruments;                /* Instruments on screen (bitmask) */
char        mods[MAX_CHARS];        /* Current modifiers in effect */
int        packet_size;                /* Current frame update packet size */
int        packet_loss;                /* lost packets per second */
int        packet_drop;                /* dropped packets per second */
int        packet_lag;                /* approximate lag in frames */
char        *packet_measure;        /* packet measurement in a second */
long        packet_loop;                /* start of measurement */

bool        showRealName = false;        /* Show realname instead of nick name */
char        name[MAX_CHARS];        /* Nick-name of player */
char        realname[MAX_CHARS];        /* Real name of player */
char        servername[MAX_CHARS];        /* Name of server connecting to */
unsigned        version;        /* Version of the server */
bool        toggle_shield;          /* Are shields toggled by a press? */
int     shields = 1;            /* When shields are considered up */

bool    auto_shield = 1;        /* shield drops for fire */

int        maxFPS;                        /* Client's own FPS */
int        oldMaxFPS;

int        clientPortStart = 0;        /* First UDP port for clients */
int        clientPortEnd = 0;        /* Last one (these are for firewalls) */

u_byte        lose_item;                /* index for dropping owned item */
int        lose_item_active;        /* one of the lose keys is pressed */

#ifdef SOUND
char         sounds[MAX_CHARS];        /* audio mappings */
char         audioServer[MAX_CHARS];        /* audio server */
int         maxVolume;                /* maximum volume (in percent) */
#endif /* SOUND */