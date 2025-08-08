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

#ifndef CLIENT_H
#define CLIENT_H

#include "item.h"
#include "shipshape.h"
#include "types.h"

typedef struct
{
    bool talking;        /* Some talk window is open? */
    bool pointerControl; /* Pointer (mouse) control is on? */
    bool restorePointerControl;
    /* Pointer control should be restored later? */
    bool quitMode; /* Client is in quit mode? */
    double clientLag;
    double scaleFactor;
    double scale;
    float fscale;
    double altScaleFactor;
} client_data_t;

typedef struct
{
    bool blockProtocol;
    bool clientRanker;
    bool clock;
    bool clockAMPM;
    bool filledDecor;
    bool filledWorld;
    bool fuelGauge;
    bool fuelMeter;
    bool horizontalHUDLine;
    bool outlineDecor;
    bool outlineWorld;
    bool packetDropMeter;
    bool packetLagMeter;
    bool packetLossMeter;
    bool packetSizeMeter;
    bool powerMeter;
    bool showDecor;
    bool showHUD;
    bool showHUDRadar;
    bool showItems;
    bool showLivesByShip;
    bool showMessages;
    bool showMineName;
    bool showMyShipShape;
    bool showShipName;
    bool showShipShapes;
    bool showShipShapesHack;
    bool slidingRadar;
    bool texturedDecor;
    bool texturedWalls;
    bool turnSpeedMeter;
    bool verticalHUDLine;
} instruments_t;

typedef struct
{
    bool help;
    bool version;
    bool text;
    bool list_servers;               /* list */
    bool auto_connect;               /* join */
    char shutdown_reason[MAX_CHARS]; /* shutdown reason */
} xp_args_t;

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

#define MIN_SCALEFACTOR 0.1
#define MAX_SCALEFACTOR 20.0

#define CONTROL_DELAY 100

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
    shipshape_t *ship;
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

extern ipos_t selfPos;
extern ipos_t selfVel;
extern ipos_t world;
extern ipos_t realWorld;
extern short heading;
extern short nextCheckPoint;
extern uint8_t numItems[NUM_ITEMS];
extern uint8_t lastNumItems[NUM_ITEMS];
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
extern long control_count;     /* Display control for how long? */
extern uint8_t spark_rand;     /* Sparkling effect */
extern uint8_t old_spark_rand; /* previous value of spark_rand */

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

extern DFLOAT hud_move_fact;      /* scale the hud-movement (speed) */
extern DFLOAT ptr_move_fact;      /* scale the speed pointer length */
extern char mods[MAX_CHARS];      /* Current modifiers in effect */
extern instruments_t instruments; /* Instruments on screen */
extern int packet_size;           /* Current frame update packet size */
extern int packet_loss;           /* lost packets per second */
extern int packet_drop;           /* dropped packets per second */
extern int packet_lag;            /* approximate lag in frames */
extern char *packet_measure;      /* packet measurement in a second */
extern long packet_loop;          /* start of measurement */

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

extern double clientFPS;    /* FPS client is drawing at */
extern double timePerFrame; /* Time a frame is shown, unit s */
extern int clientLag;       /* Time to draw a frame, unit us */
extern bool newSecond;      /* Second changed this frame */
extern bool played_this_round;
extern long twelveHz; /* Attempt to increment this at 12Hz */

extern int clientPortStart; /* First UDP port for clients */
extern int clientPortEnd;   /* Last one (these are for firewalls) */

extern uint8_t lose_item;    /* flag and index to drop item */
extern int lose_item_active; /* one of the lose keys is pressed */

#ifdef SOUND
extern char sounds[MAX_CHARS];      /* audio mappings */
extern char audioServer[MAX_CHARS]; /* audio server */
extern int maxVolume;               /* maximum volume (in percent) */
#endif                              /* SOUND */

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
    uint8_t tractor;
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
    uint8_t shield, cloak, eshield;
    uint8_t phased, deflector;
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
    uint8_t x, y;
} debris_t;

typedef struct
{
    short x, y, xi, yi, type;
} vdecor_t;

typedef struct
{
    short x, y;
    uint8_t wrecktype, size, rotation;
} wreckage_t;

typedef struct
{
    short x, y;
    uint8_t type, size, rotation;
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
shipshape_t *Ship_by_id(int id);
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
