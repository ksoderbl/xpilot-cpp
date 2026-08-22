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

#include <string>

#include "shipshape.h"
#include "item.h"

#include "clientoption.h"
#include "types.h"
#include "clientmap.h"
#include "other.h"
#include "msg-parser.h"
#include "clientoptions.h"

typedef struct
{
    bool talking = false;               /* Some talk window is open? */
    bool pointerControl = false;        /* Pointer (mouse) control is on? */
    bool restorePointerControl = false; /* Pointer control should be restored later? */
    bool quitMode = false;              /* Client is in quit mode? */
    double clientLag = 0.0;
    double scaleFactor = 1.0;
    double scale = 1.0; // 1.0 / scaleFactor;
    float fscale = 1.0;
    double altScaleFactor = 1.0;
} client_data_t;

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

// #define MAX_SCORE_OBJECTS 10

#define MAX_SPARK_SIZE 8
#define MIN_SPARK_SIZE 1
#define MAX_MAP_POINT_SIZE 8
#define MIN_MAP_POINT_SIZE 0
#define MAX_SHOT_SIZE 20
#define MIN_SHOT_SIZE 1
#define MAX_TEAMSHOT_SIZE 20
#define MIN_TEAMSHOT_SIZE 1

#define MIN_SHOW_ITEMS_TIME 0.0
#define MAX_SHOW_ITEMS_TIME 300.0

#define MIN_SCALEFACTOR 0.1
#define MAX_SCALEFACTOR 20.0

#define FUEL_NOTIFY_TIME 3.0
#define CONTROL_TIME 8.0

#define MAX_MSGS 15       /* Max. messages displayed ever */
#define MAX_HIST_MSGS 128 /* Max. messages in history */

#define MSG_LIFE_TIME 120.0  /* Seconds */
#define MSG_FLASH_TIME 105.0 /* Old messages have life time less \
                than this */
#define MAX_POINTER_BUTTONS 5

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
        bool state; /* current state of the selection */
        size_t x1;  /* string indices */
        size_t x2;
        bool incl_nl; /* include a '\n'? */
    } talk;
    /* a selection in the draw window */
    struct
    {
        bool state;
        int x1; /* string indices (for TalkMsg[].txt) */
        int x2; /* they are modified when the emphasized area */
        int y1; /* is scrolled down by new messages coming in */
        int y2;
    } draw;
    char *txt;       /* allocated when needed */
    size_t txt_size; /* size of txt buffer */
    size_t len;
    /* when a message 'jumps' from talk window to the player messages: */
    bool keep_emphasizing;
} selection_t;

typedef struct
{
    char txt[MSG_LEN];
    size_t len;
    /*short        pixelLen;*/
    double lifeTime;
    msg_bms_t bmsinfo;
} message_t;
/* typedefs end */

extern client_data_t clData;

extern char *geometry;
extern xp_args_t xpArgs;
// extern ConnectParam connectParam;
extern message_t *TalkMsg[];
extern message_t *GameMsg[];
extern message_t *TalkMsg_pending[]; /* store incoming messages */
extern message_t *GameMsg_pending[]; /* while a cut is pending */
extern char *HistoryMsg[];           /* talk window history */

extern int maxLinesInHistory; /* lines to save in history */
extern selection_t selection; /* in talk/draw window */
extern int maxMessages;
extern int messagesToStdout;

extern char *talk_fast_msgs[]; /* talk macros */

extern score_object_t score_objects[MAX_SCORE_OBJECTS];
extern int score_object;

extern bool oldServer; /* Compatibility mode for old block-based servers */
extern ipos_t selfPos;
extern ipos_t selfVel;
extern ipos_t world;
extern ipos_t realWorld;
extern short heading;
extern short nextCheckPoint;
extern uint8_t numItems[NUM_ITEMS];
extern uint8_t lastNumItems[NUM_ITEMS];
extern int numItemsTime[NUM_ITEMS];
extern double showItemsTime;
extern short autopilotLight;
extern int showScoreDecimals;
extern double scoreObjectTime; /* How long to show score objects */

extern short lock_id;   /* Id of player locked onto */
extern short lock_dir;  /* Direction of lock */
extern short lock_dist; /* Distance to player locked onto */

extern int eyesId;    /* Player we get frame updates for */
extern Other *eyes;   /* Player we get frame updates for */
extern bool snooping; /* are we snooping on someone else? */
extern int eyeTeam;   /* Team of player we get updates for */

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

extern bool UpdateRadar;
constexpr int ServerRadarWidth = 256; /* radar width at the server */
extern int RadarWidth;
extern int RadarHeight;
extern int backgroundPointDist; /* spacing of navigation points */
extern int backgroundPointSize; /* size of navigation points */
extern int sparkSize;           /* size of sparks and debris */
extern int shotSize;            /* size of shot */
extern int teamShotSize;        /* size of team shot */

extern double controlTime;     /* Display control for how long? */
extern uint8_t spark_rand;     /* Sparkling effect */
extern uint8_t old_spark_rand; /* previous value of spark_rand */

extern double fuelSum;      /* Sum of fuel in all tanks */
extern double fuelMax;      /* How much fuel can you take? */
extern short fuelCurrent;   /* Number of currently used tank */
extern short numTanks;      /* Number of tanks */
extern double fuelTime;     /* Display fuel for how long? */
extern double fuelCritical; /* Fuel critical level */
extern double fuelWarning;  /* Fuel warning level */
extern double fuelNotify;   /* Fuel notify level */

extern char *shipShape;                /* Shape of player's ship */
extern double power;                   /* Force of thrust */
extern double power_s;                 /* Saved power fiks */
extern double turnspeed;               /* How fast player acc-turns */
extern double turnspeed_s;             /* Saved turnspeed */
extern double turnresistance;          /* How much is lost in % */
extern double turnresistance_s;        /* Saved (see above) */
extern double displayedPower;          /* What the server is sending us */
extern double displayedTurnspeed;      /* What the server is sending us */
extern double displayedTurnresistance; /* What the server is sending us */
extern double sparkProb;               /* Sparkling effect configurable */
extern int charsPerSecond;             /* Message output speed (config) */

extern double hud_move_fact; /* scale the hud-movement (speed) */
extern double ptr_move_fact; /* scale the speed pointer length */
extern char mods[MAX_CHARS]; /* Current modifiers in effect */
// extern instruments_t instruments; /* Instruments on screen */
extern int packet_size;      /* Current frame update packet size */
extern int packet_loss;      /* lost packets per second */
extern int packet_drop;      /* dropped packets per second */
extern int packet_lag;       /* approximate lag in frames */
extern char *packet_measure; /* packet measurement in a second */
extern long packet_loop;     /* start of measurement */

extern bool showUserName;          /* Show username instead of nickname */
extern char servername[MAX_CHARS]; /* Name of server connecting to */
extern unsigned version;           /* Version of the server */
extern bool scoresChanged;
extern bool shields; /* When shields are considered up */

extern int maxFPS; /* Max FPS player wants from server */
extern int oldMaxFPS;

extern double clientFPS;    /* FPS client is drawing at */
extern double timePerFrame; /* Time a frame is shown, unit s */
extern int clientLag;       /* Time to draw a frame, unit us */
extern long twelveHz;       /* Attempt to increment this at 12Hz */

extern int recordFPS;      /* What FPS to record at */
extern time_t currentTime; /* Current value of time() */
extern bool newSecond;     /* Did time() increment this frame? */
extern int maxMouseTurnsPS;
extern int mouseMovementInterval;
extern int cumulativeMouseMovement;

extern char modBankStr[][MAX_CHARS]; /* modifier banks strings */

extern int baseWarningType; /* Which type of base warning you prefer */
extern int maxCharsInNames;
extern int lose_item;        /* flag and index to drop item */
extern int lose_item_active; /* one of the lose keys is pressed */

extern char sounds[MAX_CHARS];      /* audio mappings */
extern char audioServer[MAX_CHARS]; /* audio server */
extern int maxVolume;               /* maximum volume (in percent) */

// extern int num_playing_teams;

extern long start_loops, end_loops;
extern long time_left;

extern bool roundend;
extern bool played_this_round;
extern int protocolVersion;

/*
 * somewhere
 */
std::string Program_name(void);
int Bitmap_add(std::string filename, int count, bool scalable);

/*
 * Platform specific code needs to implement these.
 */
void Platform_specific_pointer_control_set_state(bool on);
void Platform_specific_talk_set_state(bool on);
void Record_toggle(void);
void Toggle_fullscreen(void);
void Toggle_radar_and_scorelist(void);

/*
 * event.c
 */
void Pointer_control_set_state(bool on);
void Talk_set_state(bool on);

void Pointer_button_pressed(int button);
void Pointer_button_released(int button);
void Keyboard_button_pressed(xp_keysym_t ks);
void Keyboard_button_released(xp_keysym_t ks);

int Key_init(void);
int Key_update(void);
void Key_clear_counts(void);
bool Key_press(keys_t key);
bool Key_release(keys_t key);
void Set_auto_shield(bool on);
void Set_toggle_shield(bool on);

// was in xinit.cpp
void Scale_dashes(void);

/*
 * client.cpp
 */
int Client_init(char *server, unsigned server_version);
int Client_setup(void);
void Client_cleanup(void);
int Client_start(void);
int Client_fps_request(void);
int Client_power(void);
int Client_pointer_move(int movement);
int Client_check_pointer_move_interval(void);
void Client_exit(int status);

int Init_playing_windows(void);

void Raise_window(void);
void Reset_shields(void);
void Set_toggle_shield(bool on);
void Set_auto_shield(bool on);
void Platform_specific_cleanup(void);

void xpilotShutdown(void);

int x_event(int);

int Key_init(void);
int Key_update(void);
int Check_client_fps(void);

extern void audioEvents();

extern int Init_playing_windows(void);
extern int Alloc_msgs(void);
extern int Startup_server_motd(void);

void Platform_specific_cleanup(void);

extern void Colors_init_style_colors(void);

/*
 * about.c
 */
extern int Handle_motd(long off, char *buf, int len, long filesize);
extern void aboutCleanup(void);

extern int motd_viewer;
extern int keys_viewer;

/*
 * default.c
 */
extern void Store_default_options(void);
extern void defaultCleanup(void); /* memory cleanup */
extern void Get_xpilotrc_file(char *, unsigned);

extern bool Set_scaleFactor(xp_option_t *opt, double val);
extern bool Set_altScaleFactor(xp_option_t *opt, double val);

/*
 * event.c
 */
extern void Store_key_options(void);

/*
 * join.c
 */
extern int Join(ConnectParam *conpar);
extern void xpilotShutdown(void);

/*
 * mapdata.c
 */
extern int Mapdata_setup(const std::string &urlstr);

/*
 * paintdata.c
 */
extern void paintdataCleanup(void); /* memory cleanup */

/*
 * paintobjects.c
 */
extern int Init_wreckage(void);
extern int Init_asteroids(void);

/*
 * query.c
 */
extern int Query_all(sock_t *sockfd, int port, char *msg, size_t msglen);

/*
 * sim.c
 */
extern void Simulate(bool on);

/*
 * textinterface.c
 */
extern int Connect_to_server(int auto_connect, int list_servers,
                             int auto_shutdown, char *shutdown_reason,
                             ConnectParam *conpar);
extern int Contact_servers(int count, char **servers,
                           int auto_connect, int list_servers,
                           int auto_shutdown, char *shutdown_message,
                           int find_max, int *num_found,
                           char **server_addresses, char **server_names,
                           unsigned *server_versions,
                           ConnectParam *conpar);

/*
 * welcome.c
 */
extern int Welcome_screen(ConnectParam *conpar);

/*
 * widget.c
 */
extern void Widget_cleanup(void);
