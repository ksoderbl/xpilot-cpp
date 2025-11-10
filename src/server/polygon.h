/*
 * XPilot, a multiplayer gravity war game.
 *
 * Copyright (C) 2000-2004 by
 *
 *      Uoti Urpala
 *      Kristian Söderblom
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

#ifndef POLYGON_H
#define POLYGON_H

#include <cstdint>

#include "click.h"
#include "object.h"

// From walls2

struct polystyle
{
    char id[100];
    int color;
    int texture_id;
    int defedge_id;
    int flags;
};

struct edgestyle
{
    char id[100];
    int width;
    int color;
    int style;
};

struct bmpstyle
{
    char id[100];
    char filename[32];
    int flags;
};

typedef struct
{
    int style;
    int current_style;
    int destroyed_style;
    int group;
    int edges;
    clpos_t pos;
    int num_points;
    int estyles_start;
    int num_echanges;
    int is_decor;
    unsigned update_mask;
    long last_change;
} poly_t;

extern poly_t *pdata;

#define ALL_BITS 0xffffffffU
#define BALL_BIT (1U << 11)
#define NONBALL_BIT (1U << 12)
#define NOTEAM_BIT (1U << 10)
#define HITMASK(team) ((team) == TEAM_NOT_SET ? NOTEAM_BIT : 1U << (team))
typedef uint32_t hitmask_t;

typedef struct move
{
    clvec_t start;
    clvec_t delta;
    hitmask_t hitmask;
    const object_t *obj;
} move_t;

typedef struct group group_t;

struct group
{
    int type;
    int team;
    hitmask_t hitmask;
    bool (*hitfunc)(group_t *groupptr, const move_t *move);
    int mapobj_ind;
};

extern struct polystyle pstyles[256];
extern struct edgestyle estyles[256];
extern struct bmpstyle bstyles[256];
// extern poly_t *pdata;
extern int *estyleptr;
extern int *edgeptr;
extern group_t *groups;
extern int num_groups, max_groups;

static inline group_t *groupptr_by_id(int group)
{
    if (group >= 0 && group < num_groups)
        return &groups[group];
    return NULL;
}

extern int num_polys, num_pstyles, num_estyles, num_bstyles;

/*
 * Prototypes for polygon.c
 */
void P_edgestyle(const char *id, int width, int color, int style);
void P_polystyle(const char *id, int color, int texture_id, int defedge_id,
                 int flags);
void P_bmpstyle(const char *id, const char *filename, int flags);
void P_start_polygon(clpos_t pos, int style);
void P_offset(clpos_t offset, int edgestyle);
void P_vertex(clpos_t pos, int edgestyle);
void P_style(const char *state, int style);
void P_end_polygon(void);
int P_start_ballarea(void);
void P_end_ballarea(void);
int P_start_balltarget(int team, int treasure_ind);
void P_end_balltarget(void);
int P_start_target(int target_ind);
void P_end_target(void);
int P_start_cannon(int cannon_ind);
void P_end_cannon(void);
int P_start_wormhole(int wormhole_ind);
void P_end_wormhole(void);
void P_start_decor(void);
void P_end_decor(void);
int P_start_friction_area(int fa_ind);
void P_end_friction_area(void);
int P_get_bmp_id(const char *s);
int P_get_edge_id(const char *s);
int P_get_poly_id(const char *s);
/*void P_grouphack(int type, void (*f)(int group, void *mapobj));*/
void P_set_hitmask(int group, hitmask_t hitmask);

#endif
