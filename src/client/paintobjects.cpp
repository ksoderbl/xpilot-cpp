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

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <ctime>
#include <climits>
#include <sys/types.h>

#include <unistd.h>

#include "commonmacros.h"

#include "client.h"
#include "paint.h"

#include "xpconfig.h"
#include "const.h"
#include "xperror.h"
#include "bit.h"
#include "types.h"
#include "keys.h"
#include "rules.h"
#include "clientsetup.h"
#include "portability.h"
#include "guiobjects.h"
#include "guimap.h"
#include "wreckshape.h"
#include "astershape.h"

#define COLOR(i) (i / areas)
#define BASE_X(i) (((i % x_areas) << 8) + ext_view_x_offset)
#define BASE_Y(i) ((ext_view_height - 1 - (((i / x_areas) % y_areas) << 8)) - ext_view_y_offset)

static int wreckageRawShapes[NUM_WRECKAGE_SHAPES][NUM_WRECKAGE_POINTS][2] = {
    {WRECKAGE_SHAPE_0},
    {WRECKAGE_SHAPE_1},
    {WRECKAGE_SHAPE_2},
};

position_t *wreckageShapes[NUM_WRECKAGE_SHAPES][NUM_WRECKAGE_POINTS];

bool markingLights;

static int wrap(int *xp, int *yp)
{
    int x = *xp, y = *yp;

    if (x < world.x || x > world.x + ext_view_width)
    {
        if (x < realWorld.x || x > realWorld.x + ext_view_width)
            return 0;
        *xp += world.x - realWorld.x;
    }
    if (y < world.y || y > world.y + ext_view_height)
    {
        if (y < realWorld.y || y > realWorld.y + ext_view_height)
            return 0;
        *yp += world.y - realWorld.y;
    }
    return 1;
}

static void Paint_items(void)
{
    int i, x, y;

    if (clMap.itemtypes.size() > 0)
    {
        for (const auto &item : clMap.itemtypes)
        {
            x = item.x;
            y = item.y;
            if (wrap(&x, &y))
                Gui_paint_item_object(item.type, x, y);
        }
        clMap.itemtypes.clear();
    }
}

static void Paint_balls(void)
{
    int i, j, id, style, x, y, xs, ys;

    if (clMap.balls.size() > 0)
    {
        for (const auto &ball : clMap.balls)
        {
            x = ball.x;
            y = ball.y;
            id = ball.id;
            style = ball.style;

            if (wrap(&x, &y))
            {
                Gui_paint_ball(x, y, style);

                if (id == -1)
                    continue;

                // Also paint connectors for ship that has connected to this ball.
                // This code makes implies that only one ship can be connected to a ball at a time.
                for (j = 0; j < clMap.ships.size() && clMap.ships[j].id != id; j++)
                {
                    if (clMap.ships[j].id == id)
                        break;
                }

                if (j >= clMap.ships.size())
                    continue;

                xs = clMap.ships[j].x;
                ys = clMap.ships[j].y;

                if (wrap(&xs, &ys))
                    Gui_paint_ball_connector(x, y, xs, ys);
            }
        }
        clMap.balls.clear();
    }
}

static void Paint_mines(void)
{
    int i, x, y;
    char *name = NULL;

    if (clMap.mines.size() > 0)
    {
        for (const auto &mine : clMap.mines)
        {
            x = mine.x;
            y = mine.y;

            if (wrap(&x, &y))
            {
                /*
                 * Determine if the name of the player who is safe
                 * from the mine should be drawn.
                 * Mines unsafe to all players have the name "Expired"
                 * We do not know who is safe for mines sent with id==0
                 */
                name = NULL;
                if (mine.id != 0)
                {
                    other_t *other;
                    if (mine.id == EXPIRED_MINE_ID)
                    {
                        static char expired_name[] = "Expired";
                        name = expired_name;
                    }
                    else if ((other = Other_by_id(mine.id)) != NULL)
                        name = other->nick_name;
                    else
                    {
                        static char unknown_name[] = "Not of this world!";
                        name = unknown_name;
                    }
                }
                Gui_paint_mine(x, y, mine.teammine, name);
            }
        }
        clMap.mines.clear();
    }
}

static void Paint_debris(int x_areas, int y_areas, int areas, int max_)
{
    int color, i, j, x, y;

#if 0
/* before "sparkColors" option: */
#define DEBRIS_COLOR(color) \
    ((num_spark_colors > 4) ? (5 + (((color & 1) << 2) | (color >> 1))) : ((num_spark_colors >= 3) ? (5 + color) : (color)))
#else
/* adjusted for "sparkColors" option: */
#define DEBRIS_COLOR(color) \
    ((num_spark_colors > 4) ? ((((color & 1) << 2) | (color >> 1))) : (color))
#endif

    for (i = 0; i < max_; i++)
    {
        if (num_debris[i] > 0)
        {
            x = BASE_X(i);
            y = BASE_Y(i);
            color = COLOR(i);
            color = DEBRIS_COLOR(color);
            for (j = 0; j < num_debris[i]; j++)
                Gui_paint_spark(color,
                                x + debris_ptr[i][j].x,
                                y - debris_ptr[i][j].y);
            RELEASE(debris_ptr[i], num_debris[i], max_debris[i]);
        }
    }
}

static void Paint_wreckages(void)
{
    int i, x, y;
    int wtype, size, rot;
    bool deadly;

    if (num_wreckage > 0)
    {
        for (i = 0; i < num_wreckage; i++)
        {
            x = wreckage_ptr[i].x;
            y = wreckage_ptr[i].y;
            if (wrap(&x, &y))
            {
                deadly = (wreckage_ptr[i].wrecktype & 0x80);

                wtype = (wreckage_ptr[i].wrecktype & 0x7F) % NUM_WRECKAGE_SHAPES;
                rot = wreckage_ptr[i].rotation;
                size = wreckage_ptr[i].size;

                Gui_paint_wreck(x, y, deadly, wtype, rot, size);
            }
        }
        RELEASE(wreckage_ptr, num_wreckage, max_wreckage);
    }
}

static void Paint_asteroids(void)
{
    int i, x, y;
    int type, size, rot;

    if (num_asteroids > 0)
    {
        Gui_paint_asteroids_begin();
        for (i = 0; i < num_asteroids; i++)
        {
            x = asteroid_ptr[i].x;
            y = asteroid_ptr[i].y;
            if (wrap(&x, &y))
            {
                type = asteroid_ptr[i].type;
                rot = asteroid_ptr[i].rotation;
                size = asteroid_ptr[i].size;

                Gui_paint_asteroid(x, y, type, rot, size);
            }
        }
        Gui_paint_asteroids_end();
        RELEASE(asteroid_ptr, num_asteroids, max_asteroids);
    }
}

static void Paint_wormholes(void)
{
    int i, x, y;

    if (num_wormholes > 0)
    {
        for (i = 0; i < num_wormholes; i++)
        {
            x = wormhole_ptr[i].x;
            y = wormhole_ptr[i].y;
            if (wrap(&x, &y))
                Gui_paint_setup_worm(x, y);
        }
        RELEASE(wormhole_ptr, num_wormholes, max_wormholes);
    }
}

static void Paint_missiles(void)
{
    int i, x, y;
    int len, dir;

    if (clMap.missiles.size() > 0)
    {
        Gui_paint_missiles_begin();

        for (const auto &missile : clMap.missiles)
        {
            x = missile.x;
            y = missile.y;
            dir = missile.dir;
            len = MISSILE_LEN;
            if (missile.len > 0)
                len = missile.len;

            if (wrap(&x, &y))
                Gui_paint_missile(x, y, len, dir);
        }
        Gui_paint_missiles_end();
        clMap.missiles.clear();
    }
}

static void Paint_lasers(void)
{
    int color, i, x1, y1, len, dir;

    if (clMap.lasers.size() > 0)
    {
        Gui_paint_lasers_begin();

        for (const auto &laser : clMap.lasers)
        {
            x1 = laser.x;
            y1 = laser.y;
            len = laser.len;
            dir = laser.dir;
            color = laser.color;

            if (wrap(&x1, &y1))
                Gui_paint_laser(color, x1, y1, len, dir);
        }
        Gui_paint_lasers_end();
        clMap.lasers.clear();
    }
}

static void Paint_fastshots(int i, int x_areas, int y_areas, int areas)
{
    int x, y, j, color;

    if (num_fastshot[i] > 0)
    {
        x = BASE_X(i);
        y = BASE_Y(i);
        color = COLOR(i);
        if (color != WHITE && color != BLUE)
            color = WHITE;
        for (j = 0; j < num_fastshot[i]; j++)
            Gui_paint_fastshot(color,
                               x + fastshot_ptr[i][j].x,
                               y - fastshot_ptr[i][j].y);
        RELEASE(fastshot_ptr[i], num_fastshot[i], max_fastshot[i]);
    }
}

static void Paint_teamshots(int i, int t_, int x_areas, int y_areas, int areas)
{
    int x, y, j /*, color */;

    (void)areas;
    /*
     * Teamshots are in range DEBRIS_TYPES to DEBRIS_TYPES*2-1 in fastshot.
     */
    if (num_fastshot[t_] > 0)
    {
        x = BASE_X(i);
        y = BASE_Y(i);
        /*color = COLOR(i);*/
        for (j = 0; j < num_fastshot[t_]; j++)
            Gui_paint_teamshot(x + fastshot_ptr[t_][j].x,
                               y - fastshot_ptr[t_][j].y);
        RELEASE(fastshot_ptr[t_], num_fastshot[t_], max_fastshot[t_]);
    }
}

void Paint_shots(void)
{
    int i, t_;
    int x_areas, y_areas, areas, max_;

    Paint_items();
    Paint_balls();
    Paint_mines();

    x_areas = (active_view_width + 255) >> 8;
    y_areas = (active_view_height + 255) >> 8;
    areas = x_areas * y_areas;
    max_ = areas * (num_spark_colors >= 3 ? num_spark_colors : 4);

    Paint_debris(x_areas, y_areas, areas, max_);

    Paint_wreckages();
    Paint_asteroids();
    Paint_wormholes();

    for (i = 0; i < max_; i++)
    {
        t_ = i + DEBRIS_TYPES;
        Paint_fastshots(i, x_areas, y_areas, areas);
        Paint_teamshots(i, t_, x_areas, y_areas, areas);
    }

    Paint_missiles();
    Paint_lasers();
}

static void Paint_paused(void)
{
    int i, x, y;

    if (clMap.pausers.size() > 0)
    {
        for (const auto &paused : clMap.pausers)
        {
            x = paused.x;
            y = paused.y;
            if (wrap(&x, &y))
                Gui_paint_paused(x, y, paused.count);
        }
        clMap.pausers.clear();
    }
}

static void Paint_appearing(void)
{
    int i, x, y;

    if (num_appearing > 0)
    {
        for (i = 0; i < num_appearing; i++)
        {
            x = appearing_ptr[i].x;
            y = appearing_ptr[i].y;
            if (wrap(&x, &y))
                Gui_paint_appearing(x, y, appearing_ptr[i].id,
                                    appearing_ptr[i].count);
        }
        RELEASE(appearing_ptr, num_appearing, max_appearing);
    }
}

static void Paint_ecm(void)
{
    int i, x, y, size;

    if (clMap.ecms.size() > 0)
    {
        for (const auto &ecm : clMap.ecms)
        {
            if ((size = ecm.size) > 0)
            {
                x = ecm.x;
                y = ecm.y;
                if (wrap(&x, &y))
                    Gui_paint_ecm(x, y, size);
            }
        }
        clMap.ecms.clear();
    }
}

static void Paint_all_ships(void)
{
    int i, x, y;

    if (clMap.ships.size() > 0)
    {
        for (const auto &ship : clMap.ships)
        {
            x = ship.x;
            y = ship.y;
            if (!wrap(&x, &y))
                continue;

            /*
             * ship in the center? (svenska-hack)
             */
            if (abs(X(x) - ext_view_width / 2) <= 1 && abs(Y(y) - ext_view_height / 2) <= 1 && Other_by_id(ship.id) != NULL)
            {
                eyesId = ship.id;
                eyes = Other_by_id(eyesId);
                if (eyes != NULL)
                    eyeTeam = eyes->team;
            }

            Gui_paint_ship(x, y,
                           ship.dir, ship.id,
                           ship.cloak, ship.phased,
                           ship.shield,
                           ship.deflector, ship.eshield);
        }
        clMap.ships.clear();
    }
}

static void Paint_refuel(void)
{
    int i, x0, y0, x1, y1;

    if (clMap.refuels.size() > 0)
    {
        for (const auto &refuel : clMap.refuels)
        {
            x0 = refuel.x0;
            y0 = refuel.y0;
            x1 = refuel.x1;
            y1 = refuel.y1;
            if (wrap(&x0, &y0) && wrap(&x1, &y1))
                Gui_paint_refuel(x0, y0, x1, y1);
        }
        clMap.refuels.clear();
    }
}

static void Paint_connectors(void)
{
    int i, x0, y0, x1, y1;

    if (clMap.connectors.size() > 0)
    {
        for (const auto &connector : clMap.connectors)
        {
            x0 = connector.x0;
            y0 = connector.y0;
            x1 = connector.x1;
            y1 = connector.y1;
            if (wrap(&x0, &y0) && wrap(&x1, &y1))
                Gui_paint_connector(x0, y0, x1, y1,
                                    connector.tractor);
        }
        clMap.connectors.clear();
    }
}

static void Paint_transporters(void)
{
    int i, x0, y0, x1, y1;

    if (clMap.transporters.size() > 0)
    {
        for (const auto &transporter : clMap.transporters)
        {
            x0 = transporter.x1;
            y0 = transporter.y1;
            x1 = transporter.x2;
            y1 = transporter.y2;
            if (wrap(&x0, &y0) && wrap(&x1, &y1))
                Gui_paint_transporter(x0, y0, x1, y1);
        }
        clMap.transporters.clear();
    }
}

static void Paint_all_connectors(void)
{

    if (clMap.refuels.size() > 0 ||
        clMap.connectors.size() > 0 ||
        clMap.transporters.size() > 0)
    {

        Gui_paint_all_connectors_begin();
        Paint_refuel();
        Paint_connectors();
        Paint_transporters();
    }
}

void Paint_ships(void)
{
    Gui_paint_ships_begin();

    Paint_paused();
    Paint_appearing();
    Paint_ecm();
    Paint_all_ships();
    Paint_all_connectors();

    Gui_paint_ships_end();
}

int Init_wreckage(void)
{
    int shp, i;
    size_t point_size;
    size_t total_size;
    char *dynmem;

    /*
     * Allocate memory for all the wreckage points.
     */
    point_size = sizeof(position_t) * RES;
    total_size = point_size * NUM_WRECKAGE_POINTS * NUM_WRECKAGE_SHAPES;
    if ((dynmem = (char *)malloc(total_size)) == NULL)
    {
        error("Not enough memory for wreckage shapes");
        return -1;
    }

    /*
     * For each wreckage-shape rotate all points.
     */
    for (shp = 0; shp < NUM_WRECKAGE_SHAPES; shp++)
    {
        for (i = 0; i < NUM_WRECKAGE_POINTS; i++)
        {
            wreckageShapes[shp][i] = (position_t *)dynmem;
            dynmem += point_size;
            wreckageShapes[shp][i][0].x = wreckageRawShapes[shp][i][0];
            wreckageShapes[shp][i][0].y = wreckageRawShapes[shp][i][1];
            Rotate_position(&wreckageShapes[shp][i][0]);
        }
    }

    return 0;
}
