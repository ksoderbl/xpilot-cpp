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

#include "polygon.h"

#include <vector>

#include <cassert>
#include <climits>
#include <cstring>

#include "commonmacros.h"
#include "commonproto.h"
#include "xperror.h"

#include "cannon.h"
#include "server.h"
#include "map.h"
#include "target.h"
#include "treasure.h"
#include "wormhole.h"

/* polygon map format related stuff */
std::vector<poly_t> pdata;
std::vector<int> edgeptr;
std::vector<int> estyleptr;

static int ptscount = -1;

struct polystyle pstyles[256];
struct edgestyle estyles[256] =
    {{"internal", 0, 0, 0}}; /* Style 0 is always this special style */
struct bmpstyle bstyles[256];

int num_pstyles, num_bstyles, num_estyles = 1; /* "Internal" edgestyle */
int max_bases, max_balls;                      /* !@# make static after testing done */
static int current_estyle, current_group, is_decor;

static int Create_group(int type, int team, hitmask_t hitmask,
                        bool (*hitfunc)(group_t *gp, const move_t *move),
                        int mapobj_ind)
{
    group_t gp;

    gp.type = type;
    gp.team = team;
    gp.hitmask = hitmask;
    gp.hitfunc = hitfunc;
    gp.mapobj_ind = mapobj_ind;

    if (current_group != 0)
    {
        warn("Broken map: map object defined inside another.");
        exit(1);
    }

    current_group = groups.size();
    groups.push_back(gp);

    return current_group;
}

void Groups_init(void)
{
    Create_group(FILLED, TEAM_NOT_SET, 0, nullptr, NO_IND);
    current_group = 0;
}

void P_edgestyle(const char *id, int width, int color, int style)
{
    if (num_estyles > 255)
    {
        warn("Too many edgestyles");
        exit(1);
    }

    strlcpy(estyles[num_estyles].id, id, sizeof(estyles[0].id));
    estyles[num_estyles].color = color;
    estyles[num_estyles].width = width;
    estyles[num_estyles].style = style;
    num_estyles++;
}

void P_polystyle(const char *id, int color, int texture_id, int defedge_id,
                 int flags)
{
    if (num_pstyles > 255)
    {
        warn("Too many polygon styles");
        exit(1);
    }
    if (defedge_id == 0)
    {
        warn("Polygon default edgestyle cannot be omitted or set "
             "to 'internal'!");
        exit(1);
    }

    strlcpy(pstyles[num_pstyles].id, id, sizeof(pstyles[0].id));
    pstyles[num_pstyles].color = color;
    pstyles[num_pstyles].texture_id = texture_id;
    pstyles[num_pstyles].defedge_id = defedge_id;
    pstyles[num_pstyles].flags = flags;
    num_pstyles++;
}

void P_bmpstyle(const char *id, const char *filename, int flags)
{
    if (num_bstyles > 255)
    {
        warn("Too many bitmap styles");
        exit(1);
    }
    strlcpy(bstyles[num_bstyles].id, id, sizeof(bstyles[0].id));
    strlcpy(bstyles[num_bstyles].filename, filename,
            sizeof(bstyles[0].filename));
    bstyles[num_bstyles].flags = flags;
    num_bstyles++;
}

/* current vertex */
static clpos_t P_cv;

void P_start_polygon(clpos_t pos, int style)
{
    world_t *world = &theWorld;
    poly_t t;

    if (!World_contains_clpos(world, pos))
    {
        warn("Polygon start point (%d, %d) is not inside the map"
             "(0 <= x < %d, 0 <= y < %d)",
             pos.cx, pos.cy, world->cwidth, world->cheight);
        exit(1);
    }
    if (style == -1)
    {
        warn("Currently you must give polygon style, no default");
        exit(1);
    }

    ptscount = 0;
    P_cv = pos;
    t.pos = pos;
    t.group = current_group;
    t.edges = edgeptr.size();
    t.style = style;
    t.current_style = style;
    t.destroyed_style = style; /* may be changed */
    t.estyles_start = estyleptr.size();
    t.is_decor = is_decor;

    t.update_mask = 0;
    t.last_change = frame_loops;

    current_estyle = pstyles[style].defedge_id;

    pdata.push_back(t);
}

void P_offset(clpos_t offset, int edgestyle)
{
    int i, offcx = offset.cx, offcy = offset.cy;

    if (ptscount < 0)
    {
        warn("Can't have <Offset> outside <Polygon>.");
        exit(1);
    }

    if (offcx == 0 && offcy == 0)
    {
        /*
         * Don't warn about zero length edges for xp maps, since
         * the conversion creates such edges.
         */
        if (is_polygon_map)
            warn("Edge with zero length");
        if (edgestyle != -1 && edgestyle != current_estyle)
        {
            warn("Refusing to change edgestyle with zero-length edge");
            exit(1);
        }
        return;
    }

    if (edgestyle != -1 && edgestyle != current_estyle)
    {
        estyleptr.push_back(ptscount);
        estyleptr.push_back(edgestyle);
        current_estyle = edgestyle;
    }

    P_cv.cx += offcx;
    P_cv.cy += offcy;

    i = (MAX(ABS(offcx), ABS(offcy)) - 1) / POLYGON_MAX_OFFSET + 1;
    for (; i > 0; i--)
    {
        edgeptr.push_back(offcx / i);
        edgeptr.push_back(offcy / i);
        offcx -= offcx / i;
        offcy -= offcy / i;
        ptscount++;
    }
}

void P_vertex(clpos_t pos, int edgestyle)
{
    clpos_t offset;

    offset.cx = pos.cx - P_cv.cx;
    offset.cy = pos.cy - P_cv.cy;

    P_offset(offset, edgestyle);
}

void P_style(const char *state, int style)
{
    if (style == -1)
    {
        warn("<Style> needs a style id.");
        exit(1);
    }

    if (ptscount < 0)
    {
        warn("Can't have <Style> outside <Polygon>.");
        exit(1);
    }

    if (!strcmp(state, "destroyed"))
    {
        int lastInd = pdata.size() - 1;
        pdata[lastInd].destroyed_style = style;
    }
    else
    {
        warn("<Style> does not support state \"%s\".", state);
        exit(1);
    }
}

void P_end_polygon(void)
{
    int lastInd = pdata.size() - 1;

    if (ptscount < 3)
    {
        warn("Polygon with less than 3 edges?? (start %d, %d)",
             pdata[lastInd].pos.cx, pdata[lastInd].pos.cy);
        exit(1);
    }

    /* kps - add check that e.g. cannons have "destroyed" state <Style> ? */

    pdata[lastInd].num_points = ptscount;
    int ecount = estyleptr.size();
    pdata[lastInd].num_echanges = ecount - pdata[lastInd].estyles_start;
    estyleptr.push_back(INT_MAX);
    ptscount = -1;
}

int P_start_ballarea(void)
{
    return Create_group(TREASURE,
                        TEAM_NOT_SET,
                        BALL_BIT,
                        nullptr,
                        NO_IND);
}

void P_end_ballarea(void)
{
    current_group = 0;
}

int P_start_balltarget(int team, int treasure_ind)
{
    return Create_group(TREASURE,
                        team,
                        NONBALL_BIT,
                        Balltarget_hitfunc,
                        treasure_ind);
}

void P_end_balltarget(void)
{
    current_group = 0;
}

int P_start_target(int target_ind)
{
    world_t *world = &theWorld;
    target_t *targ = Target_by_index(world, target_ind);

    targ->group = Create_group(TARGET,
                               targ->team,
                               Target_hitmask(targ),
                               nullptr,
                               target_ind);
    return targ->group;
}

void P_end_target(void)
{
    current_group = 0;
}

int P_start_cannon(int cannon_ind)
{
    world_t *world = &theWorld;
    cannon_t *cannon = Cannon_by_index(world, cannon_ind);

    cannon->group = Create_group(CANNON,
                                 cannon->team,
                                 Cannon_hitmask(cannon),
                                 Cannon_hitfunc,
                                 cannon_ind);
    return cannon->group;
}

void P_end_cannon(void)
{
    current_group = 0;
}

int P_start_wormhole(int wormhole_ind)
{
    world_t *world = &theWorld;
    wormhole_t *wormhole = Wormhole_by_index(world, wormhole_ind);

    // warn("P_start_wormhole, wormhole: %p", wormhole);
    if (wormhole == nullptr)
    {
        warn("wormhole at index %d is nullptr", wormhole_ind);
        return -1;
    }

    wormhole->group = Create_group(WORMHOLE,
                                   TEAM_NOT_SET,
                                   Wormhole_hitmask(wormhole),
                                   Wormhole_hitfunc,
                                   wormhole_ind);
    return wormhole->group;
}

void P_end_wormhole(void)
{
    current_group = 0;
}

int P_start_friction_area(int fa_ind)
{
    world_t *world = &theWorld;
    friction_area_t *fa = FrictionArea_by_index(world, fa_ind);

    fa->group = Create_group(FRICTION,
                             TEAM_NOT_SET,
                             0,
                             Friction_area_hitfunc,
                             fa_ind);
    return fa->group;
}

void P_end_friction_area(void)
{
    current_group = 0;
}

void P_start_decor(void)
{
    is_decor = 1;
}

void P_end_decor(void)
{
    is_decor = 0;
}

int P_get_bmp_id(const char *s)
{
    int i;

    for (i = 0; i < num_bstyles; i++)
        if (!strcmp(bstyles[i].id, s))
            return i;
    warn("Broken map: Undeclared bmpstyle %s", s);
    exit(1);
}

int P_get_edge_id(const char *s)
{
    int i;

    for (i = 0; i < num_estyles; i++)
        if (!strcmp(estyles[i].id, s))
            return i;
    warn("Broken map: Undeclared edgestyle %s", s);
    exit(1);
}

int P_get_poly_id(const char *s)
{
    int i;

    for (i = 0; i < num_pstyles; i++)
        if (!strcmp(pstyles[i].id, s))
            return i;
    warn("Broken map: Undeclared polystyle %s", s);
    exit(1);
}

void P_set_hitmask(int group, hitmask_t hitmask)
{
    assert(group >= 0);
    assert(group < groups.size());
    groups[group].hitmask = hitmask;
}
