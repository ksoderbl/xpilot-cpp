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

#ifndef PAINT_H
#define PAINT_H

#include "types.h"

#include "client.h"

/* constants begin */
#define MAX_COLORS 16    /* Max. switched colors ever */
#define MAX_COLOR_LEN 32 /* Max. length of a color name */

// #define MAX_MSGS 15 /* Max. messages displayed ever */

#define NUM_DASHES 2
#define NUM_CDASHES 2
#define DASHES_LENGTH 12

#define HUD_SIZE 90   /* Size/2 of HUD lines */
#define HUD_OFFSET 20 /* Hud line offset */
#define FUEL_GAUGE_OFFSET 6
#define HUD_FUEL_GAUGE_SIZE (2 * (HUD_SIZE - HUD_OFFSET - FUEL_GAUGE_OFFSET))
#define FUEL_NOTIFY (3 * FPS)

#define WARNING_DISTANCE (VISIBILITY_DISTANCE * 0.8)

#define SCALE_ARRAY_SIZE 32768
/* constants end */

// /* typedefs begin */
// typedef struct
// {
//     char txt[MSG_LEN];
//     short len;
//     short pixelLen;
//     int life;
// } message_t;
// /* typedefs end */

/* which index a message actually has (consider SHOW_REVERSE_SCROLL) */
#define TALK_MSG_SCREENPOS(_total, _pos) (_pos)

/* how to draw a selection */
#define DRAW_EMPHASIZED BLUE

extern int draw_width, draw_height;

extern char dashes[NUM_DASHES];
extern char cdashes[NUM_CDASHES];

extern int num_spark_colors;

extern unsigned short team;  /* What team is the player on? */
extern bool players_exposed; /* Is score window exposed? */

extern short ext_view_width;   /* Width of extended visible area */
extern short ext_view_height;  /* Height of extended visible area */
extern int active_view_width;  /* Width of active map area displayed. */
extern int active_view_height; /* Height of active map area displayed. */
extern int ext_view_x_offset;  /* Offset of ext_view_width */
extern int ext_view_y_offset;  /* Offset of ext_view_height */
extern uint8_t debris_colors;  /* Number of debris intensities */

extern char modBankStr[][MAX_CHARS]; /* modifier banks strings */

extern int maxKeyDefs;
extern long loops;

extern DFLOAT scaleFactor; /* scale the draw (main playfield) window */
extern DFLOAT scaleFactor_s;
extern short scaleArray[SCALE_ARRAY_SIZE];
extern void Init_scale_array(void);
#define WINSCALE(__n) ((__n) >= 0 ? scaleArray[(__n)] : -scaleArray[-(__n)])

int Handle_start(long server_loops);
int Handle_end(long server_loops);
int Handle_self(int x, int y, int vx, int vy, int dir,
                float power, float turnspeed, float turnresistance,
                int lock_id, int lock_dist, int lock_dir,
                int nextCheckPoint, int autopilotLight,
                uint8_t *newNumItems,
                int currentTank, int fuel_sum, int fuel_max, int packet_size);
int Handle_self_items(uint8_t *newNumItems);
int Handle_modifiers(char *m);
int Handle_damaged(int damaged);
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
int Handle_ball(int x, int y, int id);
int Handle_ship(int x, int y, int id, int dir, int shield, int cloak, int eshield, int phased, int deflector);
int Handle_mine(int x, int y, int teammine, int id);
int Handle_item(int x, int y, int type);
int Handle_fastshot(int type, uint8_t *p, int n);
int Handle_debris(int type, uint8_t *p, int n);
int Handle_wreckage(int x, int y, int wrecktype, int size, int rotation);
int Handle_asteroid(int x, int y, int type, int size, int rotation);
int Handle_wormhole(int x, int y);
int Handle_ecm(int x, int y, int size);
int Handle_trans(int x1, int y1, int x2, int y2);
int Handle_paused(int x, int y, int count);
int Handle_radar(int x, int y, int size);
int Handle_vcannon(int x, int y, int type);
int Handle_vfuel(int x, int y, long fuel);
int Handle_vbase(int x, int y, int xi, int yi, int type);
int Handle_vdecor(int x, int y, int xi, int yi, int type);
int Handle_message(char *msg);
int Handle_eyes(int id);
// void Paint_item_symbol(uint8_t type, Drawable d, GC mygc, int x, int y, int color);
// void Paint_item(uint8_t type, Drawable d, GC mygc, int x, int y);
void Paint_shots(void);
void Paint_ships(void);
void Paint_radar(void);
void Paint_sliding_radar(void);
void Paint_world_radar(void);
void Paint_radar_block(int, int, int);
void Paint_vcannon(void);
void Paint_vfuel(void);
void Paint_vbase(void);
void Paint_vdecor(void);
void Paint_world(void);
void Paint_score_entry(int entry_num, other_t *other, bool best);
void Paint_score_start(void);
void Paint_score_objects(void);
void Paint_meters(void);
void Paint_HUD(void);
void Paint_HUD_values(void);
void Paint_messages(void);
void Paint_recording(void);
void Paint_frame(void);
int Handle_time_left(long sec);
void Game_over_action(uint8_t stat);

/*
 * Prototype from blockbitmaps.c
 */
int Block_bitmaps_create(void);

#endif
