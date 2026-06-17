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

#include "const.h"
#include "shipshape.h"

#define MAX_SCORE_OBJECTS 10

typedef struct
{
    double ratio;
    double score;
    short id;
    uint16_t team;
    short check;
    short round;
    long timing_loops;
    short timing;
    short life;
    short mychar;
    short alliance;
    short name_width;         /* In pixels */
    short name_len;           /* In bytes */
    short max_chars_in_names; /* name_width was calculated
                     for this value of maxCharsInNames */
    short ignorelevel;
    shipshape_t *ship;
    char nick_name[MAX_CHARS];
    char user_name[MAX_CHARS];
    char host_name[MAX_CHARS];
    char id_string[MAX_CHARS];
} other_t;

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

extern double teamscores[MAX_TEAMS];

extern cannontime_t *cannons;
extern int num_cannons;
extern target_t *targets;
extern int num_targets;

extern int num_playing_teams;

class ClientMap
{
public:
    // fuelstation_t *fuels;
    // int num_fuels;
    std::vector<fuelstation_t> fuels;
    // homebase_t *bases;
    // int num_bases;
    std::vector<homebase_t> bases;
};

extern ClientMap clMap;

extern checkpoint_t *checks;
extern int num_checks;
extern xp_polygon_t *polygons;
extern int num_polygons, max_polygons;
extern edge_style_t *edge_styles;
extern int num_edge_styles, max_edge_styles;
extern polygon_style_t *polygon_styles;
extern int num_polygon_styles, max_polygon_styles;

/* dynamic global game data */

// TODO: use e.g. std::vectors here

extern other_t *Others;
extern int num_others, max_others;
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
extern appearing_t *appearing_ptr;
extern int num_appearing, max_appearing;
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
