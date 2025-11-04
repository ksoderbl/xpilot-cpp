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

#endif
