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

#define SHOW_HUD_INSTRUMENTS (1L << 0)
#define SHOW_HUD_VERTICAL (1L << 1)
#define SHOW_HUD_HORIZONTAL (1L << 2)
#define SHOW_FUEL_METER (1L << 3)
#define SHOW_FUEL_GAUGE (1L << 4)
#define SHOW_TURNSPEED_METER (1L << 5)
#define SHOW_POWER_METER (1L << 6)
#define SHOW_SHIP_NAME (1L << 7)
#define SHOW_SLIDING_RADAR (1L << 8)
#define SHOW_PACKET_SIZE_METER (1L << 10)
#define SHOW_PACKET_LOSS_METER (1L << 11)
#define SHOW_PACKET_DROP_METER (1L << 12)
#define SHOW_CLOCK (1L << 13)
#define SHOW_ITEMS (1L << 14)
#define SHOW_MESSAGES (1L << 15)
#define SHOW_MINE_NAME (1L << 16)
#define SHOW_OUTLINE_WORLD (1L << 17)
#define SHOW_FILLED_WORLD (1L << 18)
#define SHOW_TEXTURED_WALLS (1L << 19)
#define SHOW_DECOR (1L << 20)
#define SHOW_OUTLINE_DECOR (1L << 21)
#define SHOW_FILLED_DECOR (1L << 22)
#define SHOW_TEXTURED_DECOR (1L << 23)
#define SHOW_CLOCK_AMPM_FORMAT (1L << 24)
#define SHOW_TEXTURED_BALLS (1L << 25)
#define SHOW_REVERSE_SCROLL (1L << 26)
#define SHOW_HUD_RADAR (1L << 27)
#define SHOW_PACKET_LAG_METER (1L << 28)

#define PACKET_LOSS 0
#define PACKET_DROP 1
#define PACKET_DRAW 2

#define MAX_SCORE_OBJECTS 10

#define MAX_SPARK_SIZE 8
#define MIN_SPARK_SIZE 1
#define MAX_MAP_POINT_SIZE 8
#define MIN_MAP_POINT_SIZE 0
#define MAX_SHOT_SIZE 8
#define MIN_SHOT_SIZE 1
#define MAX_TEAMSHOT_SIZE 8
#define MIN_TEAMSHOT_SIZE 1

#define MIN_SHOW_ITEMS_TIME 0.0
#define MAX_SHOW_ITEMS_TIME 10.0

#define MIN_SCALEFACTOR 0.2
#define MAX_SCALEFACTOR 8.0

#define FIND_NAME_WIDTH(other)                                        \
    if ((other)->name_width == 0)                                     \
    {                                                                 \
        (other)->name_len = strlen((other)->name);                    \
        (other)->name_width = 2 + XTextWidth(gameFont, (other)->name, \
                                             (other)->name_len);      \
    }

/* macros begin */
#define X(co) ((int)((co) - world.x))
#define Y(co) ((int)(world.y + ext_view_height - (co)))
/* macros end */

typedef struct
{
    DFLOAT ratio;
    short id;
    short team;
    short score;
    short check;
    short round;
    short timing;
    long timing_loops;
    short life;
    short mychar;
    short alliance;
    short war_id;
    short name_width; /* In pixels */
    short name_len;   /* In bytes */
    shipobj *ship;
    char name[MAX_CHARS];
    char real[MAX_CHARS];
    char host[MAX_CHARS];
} other_t;

typedef struct
{
    int pos;   /* Block index */
    long fuel; /* Amount of fuel available */
} fuelstation_t;

typedef struct
{
    int pos;  /* Block index */
    short id, /* Id of owner or -1 */
        team; /* Team this base belongs to */
} homebase_t;

typedef struct
{
    int pos;         /* Block index */
    short dead_time, /* Frames inactive */
        dot;         /* Draw dot if inactive */
} cannontime_t;

typedef struct
{
    int pos;               /* Block index */
    short dead_time;       /* Frames inactive */
    unsigned short damage; /* Damage to target */
} target_t;

typedef struct
{
    int pos; /* Block index */
} checkpoint_t;

#define SCORE_OBJECT_COUNT 100
typedef struct
{
    int score,
        x,
        y,
        count,
        hud_msg_len,
        hud_msg_width,
        msg_width,
        msg_len;
    char msg[10],
        hud_msg[MAX_CHARS + 10];
} score_object_t;

/*
 * is a selection pending (in progress), done, drawn emphasized?
 */
#define SEL_NONE (1 << 0)
#define SEL_PENDING (1 << 1)
#define SEL_SELECTED (1 << 2)
#define SEL_EMPHASIZED (1 << 3)

/*
 * a selection (text, string indices, state,...)
 */
typedef struct
{
    /* a selection in the talk window */
    struct
    {
        int state; /* current state of the selection */
        int x1;    /* string indices */
        int x2;
        bool incl_nl; /* include a `\n'? */
    } talk;
    /* a selection in the draw window */
    struct
    {
        int state;
        int x1; /* string indices (for TalkMsg[].txt) */
        int x2; /* they are modified when the emphasized area */
        int y1; /* is scrolled down by new messages coming in */
        int y2;
    } draw;
    char *txt;    /* allocated when needed */
    int txt_size; /* size of txt buffer */
    int len;
    /* when a message `jumps' from talk window to the player messages: */
    bool keep_emphasizing;
} selection_t;

extern ipos pos;
extern ipos vel;
extern ipos world;
extern ipos realWorld;
extern short heading;
extern short nextCheckPoint;
extern u_byte numItems[NUM_ITEMS];
extern u_byte lastNumItems[NUM_ITEMS];
extern int numItemsTime[NUM_ITEMS];
extern DFLOAT showItemsTime;
extern short autopilotLight;

extern short lock_id;   /* Id of player locked onto */
extern short lock_dir;  /* Direction of lock */
extern short lock_dist; /* Distance to player locked onto */

extern other_t *self;     /* Player info */
extern short selfVisible; /* Are we alive and playing? */
extern short damaged;     /* Damaged by ECM */
extern short destruct;    /* If self destructing */
extern short shutdown_delay;
extern short shutdown_count;
extern short thrusttime;
extern short thrusttimemax;
extern short shieldtime;
extern short shieldtimemax;
extern short phasingtime;
extern short phasingtimemax;

extern int roundDelay;
extern int roundDelayMax;

extern int RadarWidth;
extern int RadarHeight;
extern int map_point_distance; /* spacing of navigation points */
extern int map_point_size;     /* size of navigation points */
extern int spark_size;         /* size of sparks and debris */
extern int shot_size;          /* size of shot */
extern int teamshot_size;      /* size of team shot */
extern bool showNastyShots;    /* show original flavor shots or the new "nasty shots" */
extern long control_count;     /* Display control for how long? */
extern u_byte spark_rand;      /* Sparkling effect */
extern u_byte old_spark_rand;  /* previous value of spark_rand */

extern long fuelSum;      /* Sum of fuel in all tanks */
extern long fuelMax;      /* How much fuel can you take? */
extern short fuelCurrent; /* Number of currently used tank */
extern short numTanks;    /* Number of tanks */
extern long fuelCount;    /* Display fuel for how long? */
extern int fuelLevel1;    /* Fuel critical level */
extern int fuelLevel2;    /* Fuel warning level */
extern int fuelLevel3;    /* Fuel notify level */

extern char *shipShape;                /* Shape of player's ship */
extern DFLOAT power;                   /* Force of thrust */
extern DFLOAT power_s;                 /* Saved power fiks */
extern DFLOAT turnspeed;               /* How fast player acc-turns */
extern DFLOAT turnspeed_s;             /* Saved turnspeed */
extern DFLOAT turnresistance;          /* How much is lost in % */
extern DFLOAT turnresistance_s;        /* Saved (see above) */
extern DFLOAT displayedPower;          /* What the server is sending us */
extern DFLOAT displayedTurnspeed;      /* What the server is sending us */
extern DFLOAT displayedTurnresistance; /* What the server is sending us */
extern DFLOAT spark_prob;              /* Sparkling effect configurable */
extern int charsPerSecond;             /* Message output speed (config) */

extern DFLOAT hud_move_fact; /* scale the hud-movement (speed) */
extern DFLOAT ptr_move_fact; /* scale the speed pointer length */
extern char mods[MAX_CHARS]; /* Current modifiers in effect */
extern long instruments;     /* Instruments on screen (bitmask) */
extern int packet_size;      /* Current frame update packet size */
extern int packet_loss;      /* lost packets per second */
extern int packet_drop;      /* dropped packets per second */
extern int packet_lag;       /* approximate lag in frames */
extern char *packet_measure; /* packet measurement in a second */
extern long packet_loop;     /* start of measurement */

extern bool showRealName;          /* Show realname instead of nickname */
extern char name[MAX_CHARS];       /* Nick-name of player */
extern char realname[MAX_CHARS];   /* Real name of player */
extern char servername[MAX_CHARS]; /* Name of server connecting to */
extern unsigned version;           /* Version of the server */
extern int scoresChanged;
extern bool toggle_shield;         /* Are shields toggled by a press? */
extern int shields;                /* When shields are considered up */
extern bool auto_shield;           /* drops shield for fire */
extern bool initialPointerControl; /* Start by using mouse for control? */
extern bool pointerControl;        /* current state of mouse ship flying */

extern int maxFPS; /* Client's own FPS */
extern int oldMaxFPS;

extern int clientPortStart; /* First UDP port for clients */
extern int clientPortEnd;   /* Last one (these are for firewalls) */

extern u_byte lose_item;     /* flag and index to drop item */
extern int lose_item_active; /* one of the lose keys is pressed */

#ifdef SOUND
extern char sounds[MAX_CHARS];      /* audio mappings */
extern char audioServer[MAX_CHARS]; /* audio server */
extern int maxVolume;               /* maximum volume (in percent) */
#endif                              /* SOUND */

extern int maxLinesInHistory; /* number of lines to save in history */
#define MAX_HIST_MSGS 128     /* maximum */

/*
 * Local types and data for painting.
 */

typedef struct
{
    short x0, y0, x1, y1;
} refuel_t;

typedef struct
{
    short x0, y0, x1, y1;
    u_byte tractor;
} connector_t;

typedef struct
{
    unsigned char color, dir;
    short x, y, len;
} laser_t;

typedef struct
{
    short x, y, dir;
    unsigned char len;
} missile_t;

typedef struct
{
    short x, y, id;
} ball_t;

typedef struct
{
    short x, y, id, dir;
    u_byte shield, cloak, eshield;
    u_byte phased, deflector;
} ship_t;

typedef struct
{
    short x, y, teammine, id;
} mine_t;

typedef struct
{
    short x, y, type;
} itemtype_t;

typedef struct
{
    short x, y, size;
} ecm_t;

typedef struct
{
    short x1, y1, x2, y2;
} trans_t;

typedef struct
{
    short x, y, count;
} paused_t;

typedef struct
{
    short x, y, size;
} radar_t;

typedef struct
{
    short x, y, type;
} vcannon_t;

typedef struct
{
    short x, y;
    long fuel;
} vfuel_t;

typedef struct
{
    short x, y, xi, yi, type;
} vbase_t;

typedef struct
{
    u_byte x, y;
} debris_t;

typedef struct
{
    short x, y, xi, yi, type;
} vdecor_t;

typedef struct
{
    short x, y;
    u_byte wrecktype, size, rotation;
} wreckage_t;

typedef struct
{
    short x, y;
    u_byte type, size, rotation;
} asteroid_t;

typedef struct
{
    short x, y;
} wormhole_t;

extern refuel_t *refuel_ptr;
extern int num_refuel, max_refuel;
extern connector_t *connector_ptr;
extern int num_connector, max_connector;
extern laser_t *laser_ptr;
extern int num_laser, max_laser;
extern missile_t *missile_ptr;
extern int num_missile, max_missile;
extern ball_t *ball_ptr;
extern int num_ball, max_ball;
extern ship_t *ship_ptr;
extern int num_ship, max_ship;
extern mine_t *mine_ptr;
extern int num_mine, max_mine;
extern itemtype_t *itemtype_ptr;
extern int num_itemtype, max_itemtype;
extern ecm_t *ecm_ptr;
extern int num_ecm, max_ecm;
extern trans_t *trans_ptr;
extern int num_trans, max_trans;
extern paused_t *paused_ptr;
extern int num_paused, max_paused;
extern radar_t *radar_ptr;
extern int num_radar, max_radar;
extern vcannon_t *vcannon_ptr;
extern int num_vcannon, max_vcannon;
extern vfuel_t *vfuel_ptr;
extern int num_vfuel, max_vfuel;
extern vbase_t *vbase_ptr;
extern int num_vbase, max_vbase;
extern debris_t *debris_ptr[DEBRIS_TYPES];
extern int num_debris[DEBRIS_TYPES],
    max_debris[DEBRIS_TYPES];
extern debris_t *fastshot_ptr[DEBRIS_TYPES * 2];
extern int num_fastshot[DEBRIS_TYPES * 2],
    max_fastshot[DEBRIS_TYPES * 2];
extern vdecor_t *vdecor_ptr;
extern int num_vdecor, max_vdecor;
extern wreckage_t *wreckage_ptr;
extern int num_wreckage, max_wreckage;
extern asteroid_t *asteroid_ptr;
extern int num_asteroids, max_asteroids;
extern wormhole_t *wormhole_ptr;
extern int num_wormholes, max_wormholes;

extern long start_loops, end_loops;
extern long time_left;

extern int eyesId;     /* Player we get frame updates for */
extern short snooping; /* are we snooping on someone else? */

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

#ifdef SOUND
extern void audioEvents();
#endif

extern int Init_playing_windows(void);
extern int Alloc_msgs(void);
extern int Startup_server_motd(void);

void Platform_specific_cleanup(void);

#endif
