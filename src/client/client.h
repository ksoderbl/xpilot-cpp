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

#ifndef CLIENT_H
#define CLIENT_H

#include "draw.h"
#include "item.h"
#include "types.h"

#define SHOW_HUD_INSTRUMENTS        (1L << 0)
#define SHOW_HUD_VERTICAL        (1L << 1)
#define SHOW_HUD_HORIZONTAL        (1L << 2)
#define SHOW_FUEL_METER                (1L << 3)
#define SHOW_FUEL_GAUGE                (1L << 4)
#define SHOW_TURNSPEED_METER        (1L << 5)
#define SHOW_POWER_METER        (1L << 6)
#define SHOW_SHIP_NAME                (1L << 7)
#define SHOW_SLIDING_RADAR        (1L << 8)
#define SHOW_PACKET_SIZE_METER        (1L << 10)
#define SHOW_PACKET_LOSS_METER        (1L << 11)
#define SHOW_PACKET_DROP_METER        (1L << 12)
#define SHOW_CLOCK                (1L << 13)
#define SHOW_ITEMS                (1L << 14)
#define SHOW_MESSAGES                (1L << 15)
#define SHOW_MINE_NAME                (1L << 16)
#define SHOW_OUTLINE_WORLD        (1L << 17)
#define SHOW_FILLED_WORLD        (1L << 18)
#define SHOW_TEXTURED_WALLS        (1L << 19)
#define SHOW_DECOR                (1L << 20)
#define SHOW_OUTLINE_DECOR        (1L << 21)
#define SHOW_FILLED_DECOR        (1L << 22)
#define SHOW_TEXTURED_DECOR        (1L << 23)
#define SHOW_CLOCK_AMPM_FORMAT        (1L << 24)
#define SHOW_TEXTURED_BALLS        (1L << 25)
#define SHOW_REVERSE_SCROLL        (1L << 26)
#define SHOW_HUD_RADAR          (1L << 27)
#define SHOW_PACKET_LAG_METER        (1L << 28)

#define PACKET_LOSS                0
#define PACKET_DROP                1
#define PACKET_DRAW                2

#define MAX_SCORE_OBJECTS        10

#define MAX_SPARK_SIZE                8
#define MIN_SPARK_SIZE                1
#define MAX_MAP_POINT_SIZE        8
#define MIN_MAP_POINT_SIZE        0
#define MAX_SHOT_SIZE                8
#define MIN_SHOT_SIZE                1
#define MAX_TEAMSHOT_SIZE        8
#define MIN_TEAMSHOT_SIZE        1

#define MIN_SHOW_ITEMS_TIME        0.0
#define MAX_SHOW_ITEMS_TIME        10.0

#define MIN_SCALEFACTOR                0.2
#define MAX_SCALEFACTOR                8.0

/* macros begin */
#define X(co)        ((int) ((co) - world.x))
#define Y(co)        ((int) (world.y + ext_view_height - (co)))
/* macros end */

typedef struct {
    DFLOAT        ratio;
    short        id;
    short        team;
    short        score;
    short        check;
    short        round;
    short        timing;
    long        timing_loops;
    short        life;
    short        mychar;
    short        alliance;
    short        war_id;
    short        name_width;        /* In pixels */
    short        name_len;        /* In bytes */
    shipobj        *ship;
    char        name[MAX_CHARS];
    char        real[MAX_CHARS];
    char        host[MAX_CHARS];
} other_t;


extern ipos        pos;
extern ipos        vel;
extern ipos        world;
extern ipos        realWorld;
extern short        heading;
extern short        nextCheckPoint;
extern u_byte        numItems[NUM_ITEMS];
extern u_byte        lastNumItems[NUM_ITEMS];
extern int        numItemsTime[NUM_ITEMS];
extern DFLOAT        showItemsTime;
extern short        autopilotLight;

extern short        lock_id;                /* Id of player locked onto */
extern short        lock_dir;                /* Direction of lock */
extern short        lock_dist;                /* Distance to player locked onto */

extern other_t*        self;                        /* Player info */
extern short        selfVisible;                /* Are we alive and playing? */
extern short        damaged;                /* Damaged by ECM */
extern short        destruct;                /* If self destructing */
extern short        shutdown_delay;
extern short        shutdown_count;
extern short        thrusttime;
extern short        thrusttimemax;
extern short        shieldtime;
extern short        shieldtimemax;
extern short        phasingtime;
extern short        phasingtimemax;

extern int                roundDelay;
extern int                roundDelayMax;

extern int        RadarWidth;
extern int        RadarHeight;

extern u_byte        spark_rand;                /* Sparkling effect */
extern u_byte        old_spark_rand;                /* previous value of spark_rand */

extern char        *shipShape;                /* Shape of player's ship */

extern long        instruments;                /* Instruments on screen (bitmask) */
extern int        packet_size;                /* Current frame update packet size */
extern int        packet_loss;                /* lost packets per second */
extern int        packet_drop;                /* dropped packets per second */
extern int        packet_lag;                /* approximate lag in frames */
extern char        *packet_measure;        /* packet measurement in a second */
extern long        packet_loop;                /* start of measurement */

extern unsigned        version;                /* Version of the server */

extern int        clientPortStart;        /* First UDP port for clients */
extern int        clientPortEnd;                /* Last one (these are for firewalls) */

extern u_byte        lose_item;                /* flag and index to drop item */


int Fuel_by_pos(int x, int y);
int Target_alive(int x, int y, int *damage);
int Target_by_index(int ind, int *xp, int *yp, int *dead_time, int *damage);
int Handle_fuel(int ind, int fuel);
int Cannon_dead_time_by_pos(int x, int y, int *dot);
int Handle_cannon(int ind, int dead_time);
int Handle_target(int num, int dead_time, int damage);
int Base_info_by_pos(int x, int y, int *id, int *team);
int Handle_base(int id, int ind);
int Check_pos_by_index(int ind, int *xp, int *yp);
int Check_index_by_pos(int x, int y);
other_t *Other_by_id(int id);
shipobj *Ship_by_id(int id);
int Handle_leave(int id);
int Handle_player(int id, int team, int mychar, char *player_name,
                  char *real_name, char *host_name, char *shape);
int Handle_score(int id, int score, int life, int mychar, int alliance);
int Handle_score_object(int score, int x, int y, char *msg);
int Handle_timing(int id, int check, int round);
int Handle_war(int robot_id, int killer_id);
int Handle_seek(int programmer_id, int robot_id, int sought_id);
void Map_dots(void);
void Map_restore(int startx, int starty, int width, int height);
void Map_blue(int startx, int starty, int width, int height);
void Client_score_table(void);
int Client_init(char *server, unsigned server_version);
int Client_setup(void);
void Client_cleanup(void);
int Client_start(void);
int Client_fps_request(void);
int Client_power(void);
int Client_input(int);
int Client_wrap_mode(void);
void Reset_shields(void);
void Set_toggle_shield(bool on);
void Set_auto_shield(bool on);

#ifdef XlibSpecificationRelease
void Key_event(XEvent *event);
#endif
int x_event(int);

int Key_init(void);
int Key_update(void);
int Check_client_fps(void);

#ifdef        SOUND
extern        void audioEvents();
#endif

#endif
