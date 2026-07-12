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

#include <vector>
#include <array>

#include "const.h"
#include "other.h"
#include "shipshape.h"

#define MAX_SCORE_OBJECTS 10

typedef struct
{
    int pos;       /* Block index */
    double fuel;   /* Amount of fuel available */
    irec_t bounds; /* Location on map */
} fuelstation_t;

typedef struct
{
    int pos;         /* Block index */
    short id;        /* Id of owner or -1 */
    uint16_t team;   /* Team this base belongs to */
    irec_t bounds;   /* Location on map */
    int type;        /* orientation */
    long appeartime; /* For base warning */
} homebase_t;

typedef struct
{
    int pos;         /* Block index */
    short dead_time, /* Frames inactive */
        dot;         /* Draw dot if inactive */
} cannontime_t;

typedef struct
{
    int pos;         /* Block index */
    short dead_time; /* Frames inactive */
    double damage;   /* Damage to target */
} target_t;

typedef struct
{
    int pos;       /* Block index */
    irec_t bounds; /* Location on map */
} checkpoint_t;

typedef struct
{
    int width;           /* Line width, -1 means no line */
    unsigned long color; /* Line color */
    int rgb;             /* RGB values corresponding to color */
    int style;           /* 0=LineSolid, 1=LineOnOffDash, 2=LineDoubleDash */
} edge_style_t;

typedef struct
{
    unsigned long color; /* The color if drawn in filled mode */
    int rgb;             /* RGB values corresponding to color */
    int texture;         /* The texture if drawn in texture mode */
    int flags;           /* Flags about this style (see draw.h) */
    int def_edge_style;  /* The default style for edges */
} polygon_style_t;

typedef struct
{
    ipos_t *points;   /* points[0] is absolute, rest are relative */
    int num_points;   /* number of points */
    irec_t bounds;    /* bounding box for the polygon */
    int *edge_styles; /* optional array of indexes to edge_styles */
    int style;        /* index to polygon_styles array */
} xp_polygon_t;

/*
 * Types for dynamic game data
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
    uint8_t color, dir;
    short x, y, len;
} laser_t;

typedef struct
{
    short x, y, dir;
    uint8_t len;
} missile_t;

typedef struct
{
    short x, y, id;
    uint8_t style;
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
    short x, y, id, count;
} appearing_t;

typedef enum
{
    RadarEnemy,
    RadarFriend
} radar_type_t;

typedef struct
{
    short x, y, size;
    radar_type_t type;
} radar_t;

typedef struct
{
    short x, y, type;
} vcannon_t;

typedef struct
{
    short x, y;
    double fuel;
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
    uint8_t x, y;
} fastshot_t;

typedef struct
{
    uint8_t x, y;
} teamshot_t;

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

#define SCORE_OBJECT_COUNT 100
typedef struct
{
    double score,
        life_time;
    int x,
        y,
        count,
        hud_msg_len,
        hud_msg_width,
        msg_width,
        msg_len;
    char msg[10],
        hud_msg[MAX_CHARS + 10];
} score_object_t;

/* mapdata accessible to outside world */

extern int num_playing_teams;

class ClientMap
{
public:
    std::vector<cannontime_t> cannons;
    std::vector<target_t> targets;
    std::vector<fuelstation_t> fuels;
    std::vector<homebase_t> bases;
    std::vector<refuel_t> refuels;
    std::vector<connector_t> connectors;
    std::vector<laser_t> lasers;
    std::vector<missile_t> missiles;
    std::vector<ball_t> balls;
    std::vector<ship_t> ships;
    std::vector<mine_t> mines;
    std::vector<itemtype_t> itemtypes;
    std::vector<ecm_t> ecms;
    std::vector<trans_t> transporters;
    std::vector<paused_t> pausers;
    std::vector<appearing_t> appearers;
    std::vector<radar_t> radarObjects;
    std::vector<vcannon_t> vcannons;
    std::vector<vfuel_t> vfuels;
    std::vector<vbase_t> vbases;
    std::vector<vdecor_t> vdecors;
    std::vector<wreckage_t> wreckages;
    std::vector<asteroid_t> asteroids;
    std::vector<wormhole_t> wormholes;

    // array with DEBRIS_TYPE number of vectors containing debris_t
    std::array<std::vector<debris_t>, DEBRIS_TYPES> debrisTypes;
    std::array<std::vector<fastshot_t>, DEBRIS_TYPES> fastshotTypes;
    std::array<std::vector<teamshot_t>, DEBRIS_TYPES> teamshotTypes;

    std::vector<xp_polygon_t> polygons;
};

extern ClientMap clMap;

extern checkpoint_t *checks;
extern int num_checks;
// extern xp_polygon_t *polygons;
// extern int num_polygons, max_polygons;
extern edge_style_t *edge_styles;
extern int num_edge_styles, max_edge_styles;
extern polygon_style_t *polygon_styles;
extern int num_polygon_styles, max_polygon_styles;

double Fuel_by_pos(int x, int y);
int Target_alive(int x, int y, double *damage);
int Target_by_index(int ind, int *xp, int *yp, int *dead_time, double *damage);
int Handle_fuel(int ind, double fuel);
int Cannon_dead_time_by_pos(int x, int y, int *dot);
int Handle_cannon(int ind, int dead_time);
int Handle_target(int num, int dead_time, double damage);
int Base_info_by_pos(int x, int y, int *id, int *team);
int Handle_base(int id, int ind);
int Check_pos_by_index(int ind, int *xp, int *yp);
int Check_index_by_pos(int x, int y);
homebase_t *Homebase_by_id(int id);
int Handle_leave(int id);
int Handle_player(int id, int team, int mychar,
                  char *nick_name, char *user_name, char *host_name,
                  char *shape, int myself);
int Handle_team(int id, int pl_team);
int Handle_score(int id, double score, int life, int mychar, int alliance);
int Handle_score_object(double score, int x, int y, char *msg);
int Handle_team_score(int team, double score);
int Handle_timing(int id, int check, int round, long loops);
int Handle_seek(int programmer_id, int robot_id, int sought_id);
int Handle_start(long server_loops);
int Handle_end(long server_loops);
int Handle_self(int x, int y, int vx, int vy, int newHeading,
                double newPower, double newTurnspeed, double newTurnresistance,
                int newLockId, int newLockDist, int newLockBearing,
                int newNextCheckPoint, int newAutopilotLight,
                uint8_t *newNumItems, int newCurrentTank,
                double newFuelSum, double newFuelMax, int newPacketSize,
                int status);
int Handle_self_items(uint8_t *newNumItems);
int Handle_modifiers(char *m);
int Handle_damaged(int dam);
int Handle_destruct(int count);
int Handle_shutdown(int count, int delay);
int Handle_thrusttime(int count, int max);
int Handle_shieldtime(int count, int max);
int Handle_phasingtime(int count, int max);
int Handle_rounddelay(int count, int max);
int Handle_refuel(int x0, int y0, int x1, int y1);
int Handle_connector(int x0, int y0, int x1, int y1, int tractor);
int Handle_laser(int color, int x, int y, int len, int dir);
int Handle_missile(int x, int y, int dir, int len);
int Handle_ball(int x, int y, int id, int style);
int Handle_ship(int x, int y, int id, int dir, int shield, int cloak,
                int eshield, int phased, int deflector);
int Handle_mine(int x, int y, int teammine, int id);
int Handle_item(int x, int y, int type);
int Handle_fastshot(int type, uint8_t *p, int n);
int Handle_teamshot(int type, uint8_t *p, int n);
int Handle_debris(int type, uint8_t *p, int n);
int Handle_wreckage(int x, int y, int wrecktype, int size, int rotation);
int Handle_asteroid(int x, int y, int type, int size, int rotation);
int Handle_wormhole(int x, int y);
int Handle_polystyle(int polyind, int newstyle);
int Handle_ecm(int x, int y, int size);
int Handle_trans(int x1, int y1, int x2, int y2);
int Handle_paused(int x, int y, int count);
int Handle_appearing(int x, int y, int id, int count);
int Handle_radar(int x, int y, int size);
int Handle_fastradar(int x, int y, int size);
int Handle_vcannon(int x, int y, int type);
int Handle_vfuel(int x, int y, double fuel);
int Handle_vbase(int x, int y, int xi, int yi, int type);
int Handle_vdecor(int x, int y, int xi, int yi, int type);
int Handle_message(char *msg);
int Handle_eyes(int id);
int Handle_time_left(long sec);
void Map_dots(void);
void Map_restore(int startx, int starty, int width, int height);
void Map_blue(int startx, int starty, int width, int height);
bool Using_score_decimals(void);
int Map_init(void);
int Map_cleanup(void);
