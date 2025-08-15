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

#ifndef PLAYER_H
#define PLAYER_H

#define SERVER

#include "bit.h"
#include "click.h"
#include "connection.h"
#include "item.h"
#include "keys.h"
#include "shipshape.h"

#include "object.h"
#include "serverconst.h"

/* IMPORTANT
 *
 * This is the player structure, the first part MUST be similar to object_t,
 * this makes it possible to use the same basic operations on both of them
 * (mainly used in update.c).
 */
typedef struct player player_t;
struct player
{
    OBJECT_BASE

    /* up to here the player type should be the same as an object. */

    int type_ext; /* extended type info (tank, robot) */

    DFLOAT turnspeed; /* How fast player acc-turns */
    DFLOAT velocity;  /* Absolute speed */

    int kills;  /* Number of kills this round */
    int deaths; /* Number of deaths this round */

    long used; /** Items you use **/
    long have; /** Items you have **/

    int shield_time;         /* Shields if no playerShielding */
    pl_fuel_t fuel;          /* ship tanks and the stored fuel */
    DFLOAT emptymass;        /* Mass of empty ship */
    DFLOAT float_dir;        /* Direction, in float var */
    DFLOAT turnresistance;   /* How much is lost in % */
    DFLOAT turnvel;          /* Current velocity of turn (right) */
    DFLOAT oldturnvel;       /* Last velocity of turn (right) */
    DFLOAT turnacc;          /* Current acceleration of turn */
    int score;               /* Current score of player */
    int prev_score;          /* Last score that has been updated */
    int prev_life;           /* Last life that has been updated */
    shipshape_t *ship;       /* wire model of ship shape */
    DFLOAT power;            /* Force of thrust */
    DFLOAT power_s;          /* Saved power fiks */
    DFLOAT turnspeed_s;      /* Saved turnspeed */
    DFLOAT turnresistance_s; /* Saved (see above) */
    DFLOAT sensor_range;     /* Range of sensors (radar) */
    int shots;               /* Number of active shots by player */
    int missile_rack;        /* Next missile rack to be active */

    int num_pulses; /* Number of laser pulses in the air. */

    int emergency_thrust_left; /* how much emergency thrust left */
    int emergency_thrust_max;  /* maximum time left */
    int emergency_shield_left; /* how much emergency shield left */
    int emergency_shield_max;  /* maximum time left */
    int phasing_left;          /* how much time left */
    int phasing_max;           /* maximum time left */

    int item[NUM_ITEMS]; /* for each item type how many */
    int lose_item;       /* which item to drop */
    int lose_item_state; /* lose item key state, 2=up,1=down */

    DFLOAT auto_power_s;               /* autopilot saves of current */
                                       /* power, turnspeed and */
    DFLOAT auto_turnspeed_s;           /* turnresistance settings. Restored */
    DFLOAT auto_turnresistance_s;      /* when autopilot turned off */
    modifiers_t modbank[NUM_MODBANKS]; /* useful modifier settings */
    bool tractor_is_pressor;           /* on if tractor is pressor */
    int shot_max;                      /* Maximum number of shots active */
    long shot_time;                    /* Time of last shot fired by player */
    int repair_target;                 /* Repairing this target */
    int fs;                            /* Connected to fuel station fs */
    int check;                         /* Next check point to pass */
    int prev_check;                    /* Previous check point for score */
    int time;                          /* The time a player has used */
    int round;                         /* Number of rounds player have done */
    int prev_round;                    /* Previous rounds value for score */
    int best_lap;                      /* Players best lap time */
    int last_lap;                      /* Time on last pass */
    int last_lap_time;                 /* What was your last pass? */
    int last_check_dir;                /* player dir at last checkpoint */
    long last_wall_touch;              /* last time player touched a wall */

    int home_base; /* Num of home base */
    struct
    {
        int tagged;      /* Flag, what is tagged? */
        int pl_id;       /* Tagging player id */
        DFLOAT distance; /* Distance to object */
    } lock;
    int lockbank[LOCKBANK_MAX]; /* Saved player locks */

    uint8_t dir;                /* Direction of acceleration */
    uint8_t unused1;            /* padding for alignment */
    char mychar;                /* Special char for player */
    char prev_mychar;           /* Special char for player */
    char name[MAX_CHARS];       /* Nick-name of player */
    char realname[MAX_CHARS];   /* Real name of player */
    char hostname[MAX_CHARS];   /* Hostname of client player uses */
    unsigned short pseudo_team; /* Which team for detaching tanks */
    int alliance;               /* Member of which alliance? */
    int prev_alliance;          /* prev. alliance for score */
    int invite;                 /* Invitation for alliance */
    ballobject_t *ball;

    /*
     * Pointer to robot private data (dynamically allocated).
     * Only used in robot code.
     */
    struct robot_data *robot_data_ptr;

    /*
     * A record of who's been pushing me (a circular buffer).
     */
    shove_t shove_record[MAX_RECORDED_SHOVES];
    int shove_next;

    struct _visibility *visibility;

    int updateVisibility, forceVisible, damaged;
    int wormDrawCount, wormHoleHit, wormHoleDest;
    int stunned;

    int last_target_update;   /* index of last updated target */
    int last_cannon_update;   /* index of last updated cannon */
    int last_fuel_update;     /* index of last updated fuel */
    int last_wormhole_update; /* index of last updated wormhole */

    int ecmcount; /* number of active ecms */

    connection_t *conn; /* connection pointer, NULL if robot */
    unsigned version;   /* XPilot version number of client */

    BITV_DECL(last_keyv, NUM_KEYS); /* Keyboard state */
    BITV_DECL(prev_keyv, NUM_KEYS); /* Keyboard state */

    long frame_last_busy; /* When player touched keyboard. */

    void *audio; /* audio private data */

    int player_fps; /* FPS that this player can do */

    int isowner;    /* If player started this server. */
    int isoperator; /* If player has operator privileges. */

    int ind; /* Index in Players[] */
};

extern player_t **Players;

void Player_position_set_clicks(player_t *pl, int cx, int cy);
void Player_position_init_clicks(player_t *pl, int cx, int cy);

void Player_position_restore(player_t *pl);
void Player_position_limit(player_t *pl);
void Player_position_debug(player_t *pl, const char *msg);

#define Player_position_remember(p_) Object_position_remember(p_)

#endif
