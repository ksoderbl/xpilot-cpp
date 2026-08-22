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

#include <array>
#include <vector>
#include <cstring>
#include <new>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cmath>
#include <ctime>

#include <sys/time.h>

#include "bit.h"
#include "commonmacros.h"
#include "commonproto.h"
#include "const.h"
#include "rules.h"
#include "clientsetup.h"
#include "shipshape.h"
#include "types.h"

#include "client.h"
#include "clientmap.h"
#include "clientcommand.h"
#include "clientoptions.h" // 2026
#include "messages.h"
#include "netclient.h"
#include "paint.h"
#include "paintradar.h"
#include "talk.h"

client_data_t clData;

char *geometry;
xp_args_t xpArgs;
// ConnectParam connectParam;

int baseWarningType = 2; /* Which type of base warning you prefer */
int maxCharsInNames;
int hudRadarDotSize;            /* Size for hudradar dot drawing */
double hudRadarScale = 3.0;     /* Scale for hudradar drawing */
double hudRadarLimit = 0.05;    /* Hudradar dots are not drawn if closer to
                your ship than this factor of visible
                range */
int hudSize = 3 * MIN_HUD_SIZE; /* Size for HUD drawing, depends on hudScale */

bool is_server = false; /* used in common code */

bool scoresChanged = true;
int RadarHeight = 0;
int RadarWidth = ServerRadarWidth; /* radar width at the server */
bool UpdateRadar = false;          /* radar update because of polystyle changes? */

bool oldServer;
ipos_t selfPos;
ipos_t selfVel;
short heading;
short nextCheckPoint;

uint8_t numItems[NUM_ITEMS];     /* Count of currently owned items */
uint8_t lastNumItems[NUM_ITEMS]; /* Last item count shown */
int numItemsTime[NUM_ITEMS];     /* Number of frames to show this item count */
double showItemsTime;            /* How long to show changed item count for */
double scoreObjectTime;          /* How long to flash score objects */

short autopilotLight;

int showScoreDecimals;

short lock_id;   /* Id of player locked onto */
short lock_dir;  /* Direction of lock */
short lock_dist; /* Distance to player locked onto */

int eyesId;            /* Player we get frame updates for */
Other *eyes = nullptr; /* Player we get frame updates for */
bool snooping;         /* are we snooping on someone else? */
int eyeTeam = TEAM_NOT_SET;

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

int roundDelay;    /* != 0 means we're in a delay */
int roundDelayMax; /* (not yet) used for graph of time
                      remaining in delay */

int backgroundPointDist; /* spacing of navigation points */
int backgroundPointSize; /* size of navigation points */
int sparkSize;           /* size of debris and spark */
int shotSize;            /* size of shot */
int teamShotSize;        /* size of team shot */
double controlTime;      /* Display control for how long? */
uint8_t spark_rand;      /* Sparkling effect */
uint8_t old_spark_rand;  /* previous value of spark_rand */

double fuelSum;      /* Sum of fuel in all tanks */
double fuelMax;      /* How much fuel can you take? */
short fuelCurrent;   /* Number of currently used tank */
short numTanks;      /* Number of tanks */
double fuelTime;     /* Display fuel for how long? */
double fuelCritical; /* Fuel critical level */
double fuelWarning;  /* Fuel warning level */
double fuelNotify;   /* Fuel notify level */

char *shipShape = nullptr;      /* Shape of player's ship */
double power;                   /* Force of thrust */
double power_s;                 /* Saved power fiks */
double turnspeed;               /* How fast player acc-turns */
double turnspeed_s;             /* Saved turnspeed */
double turnresistance;          /* How much is lost in % */
double turnresistance_s;        /* Saved (see above) */
double displayedPower;          /* What the server is sending us */
double displayedTurnspeed;      /* What the server is sending us */
double displayedTurnresistance; /* What the server is sending us */
double sparkProb;               /* Sparkling effect user configurable */
int charsPerSecond;             /* Message output speed (configurable) */

double hud_move_fact; /* scale the hud-movement (speed) */
double ptr_move_fact; /* scale the speed pointer length */
char mods[MAX_CHARS]; /* Current modifiers in effect */
int packet_size;      /* Current frame update packet size */
int packet_loss;      /* lost packets per second */
int packet_drop;      /* dropped packets per second */
int packet_lag;       /* approximate lag in frames */
char *packet_measure; /* packet measurement in a second */
long packet_loop;     /* start of measurement */

bool showUserName = false;                /* Show user name instead of nick name */
char servername[MAX_CHARS];               /* Name of server connecting to */
unsigned version;                         /* Version of the server */
bool shields = true;                      /* When shields are considered up */
char modBankStr[NUM_MODBANKS][MAX_CHARS]; /* modifier banks */

int maxFPS; /* Max FPS player wants from server */
int oldMaxFPS = 0;
double clientFPS = 1.0; /* FPS client is drawing at */
// double timePerFrame = 0.0; /* Time a frame is shown, unit seconds */
int clientLag = 0;
long twelveHz = 0; /* We attempt to increment this at 12 Hz */

int recordFPS = 0;      /* What FPS to record at */
time_t currentTime = 0; /* Current value of time() */
bool newSecond = false; /* Did time() increment this frame? */

int maxMouseTurnsPS = 0;
int mouseMovementInterval = 0;
int cumulativeMouseMovement = 0;

int lose_item;        /* index for dropping owned item */
int lose_item_active; /* one of the lose keys is pressed */

char sounds[MAX_CHARS];      /* audio mappings */
char audioServer[MAX_CHARS]; /* audio server */

// score_object_t score_objects[MAX_SCORE_OBJECTS];
// int score_object = 0;

int num_playing_teams = 0;
long time_left = -1;
long start_loops, end_loops;

int Client_init(char *server, unsigned server_version)
{
    warn("Client_init, server %s, server_version 0x%08x", server, server_version);

    version = server_version;
    if (server_version < 0x4F09)
        oldServer = true;
    else
        oldServer = false;

    if (Paint_init() == -1)
        return -1;

    strlcpy(servername, server, sizeof(servername));

    return 0;
}

int Client_setup(void)
{
    if (Map_init() == -1)
        return -1;

    if (oldServer)
    {
        Map_dots();
        Map_restore(0, 0, Setup->x, Setup->y);
        Map_blue(0, 0, Setup->x, Setup->y);
        /* kps -remove this, you shouldn't change options this way */
        /* No one wants this on old-style maps anyway, so turn it off.
         * I do, so turn it on.
         * This allows people to turn it on in their .xpilotrc for new maps
         * without affecting old ones. It's still possible to turn in on
         * from the config menu during play for old maps.
         * -- But doesn't seem to work anyway if turned on? Well who cares */
        clientOptions.instruments.texturedWalls = false;
    }

    RadarHeight = (RadarWidth * Setup->height) / Setup->width;

    if (Init_playing_windows() == -1)
        return -1;

    if (Alloc_msgs() == -1)
        return -1;

    if (Alloc_history() == -1)
        return -1;

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
        return Client_fps_request();
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
        Send_turnresistance_s(turnresistance_s) == -1)
        return -1;

    // if (Check_view_dimensions() == -1)
    //     return -1;

    if (Send_display1() == -1)
        return -1;

    for (i = 0; i < NUM_MODBANKS; i++)
    {
        if (Send_modifier_bank(i) == -1)
            return -1;
    }

    return 0;
}

int Client_start(void)
{
    Key_init();

    return 0;
}

void Client_cleanup(void)
{
    int i;

    Pointer_control_set_state(false);
    Platform_specific_cleanup();
    Free_selectionAndHistory();
    Free_msgs();

    if (others.size() > 0)
    {
        for (i = 0; i < others.size(); i++)
        {
            Other *other = others[i];
            Free_ship_shape(other->ship);
        }
        others.resize(0);
    }

    clMap.refuels.clear();
    clMap.connectors.clear();
    clMap.lasers.clear();
    clMap.missiles.clear();
    clMap.balls.clear();
    clMap.ships.clear();
    clMap.mines.clear();
    clMap.ecms.clear();
    clMap.transporters.clear();
    clMap.pausers.clear();
    clMap.appearers.clear();
    clMap.radarObjects.clear();
    clMap.vcannons.clear();
    clMap.vfuels.clear();
    clMap.vbases.clear();
    clMap.vdecors.clear();
    clMap.itemtypes.clear();
    clMap.wreckages.clear();
    clMap.asteroids.clear();
    clMap.wormholes.clear();

    Map_cleanup();
    Paint_cleanup();
}

int Client_pointer_move(int movement)
{
    if (maxMouseTurnsPS == 0)
        return Send_pointer_move(movement);

    /*
     * maxMouseTurnsPS is not 0: player wants to limit amount
     * of pointer move packets sent to server.
     */
    cumulativeMouseMovement += movement;

    return 0;
}

/*
 * Check if there is any pointer move we need to send to server.
 * Returns how many microseconds to wait in select().
 */
int Client_check_pointer_move_interval(void)
{
    struct timeval now;
    static int last_send_interval_num = -1;
    int interval_num; /* 0 ... maxMouseTurnsPS - 1 */
    int next_interval_start;

    assert(maxMouseTurnsPS > 0);

    /*
     * Let's see if we've sent any pointer move this interval,
     * if not and there is something to send, do that now.
     */
    gettimeofday(&now, nullptr);
    interval_num = ((int)now.tv_usec) / mouseMovementInterval;
    if (interval_num != last_send_interval_num && cumulativeMouseMovement != 0)
    {
        Send_pointer_move(cumulativeMouseMovement);
        cumulativeMouseMovement = 0;
        last_send_interval_num = interval_num;
    }

    if (cumulativeMouseMovement != 0)
    {
        /* calculate how long to wait to next interval */
        next_interval_start = (interval_num + 1) * mouseMovementInterval;
        return next_interval_start - (int)now.tv_usec;
    }

    return 1000000;
}

/*
 * Exit the entire client.
 */
void Client_exit(int status)
{
    Net_cleanup();
    Client_cleanup();
    exit(status);
}
