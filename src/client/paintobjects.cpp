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
    char *name = nullptr;

    if (clMap.mines.size() > 0)
    {
        for (const auto &mine : clMap.mines)
        {
            int x = mine.x;
            int y = mine.y;

            if (wrap(&x, &y))
            {
                /*
                 * Determine if the name of the player who is safe
                 * from the mine should be drawn.
                 * Mines unsafe to all players have the name "Expired"
                 * We do not know who is safe for mines sent with id==0
                 */
                name = nullptr;
                if (mine.id != 0)
                {
                    Other *other;
                    if (mine.id == EXPIRED_MINE_ID)
                    {
                        static char expired_name[] = "Expired";
                        name = expired_name;
                    }
                    else if ((other = Other_by_id(mine.id)) != nullptr)
                        name = const_cast<char *>(other->nick_name.c_str());
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

/* adjusted for "sparkColors" option: */
static inline int Debris_color(int color)
{
    return ((num_spark_colors > 4) ? ((((color & 1) << 2) | (color >> 1))) : (color));
}

static void Paint_debris(int x_areas, int y_areas, int areas, int max_i)
{
    for (int i = 0; i < max_i; i++)
    {
        auto &debrisList = clMap.debrisTypes[i];

        if (debrisList.empty())
            continue;

        const int x = BASE_X(i);
        const int y = BASE_Y(i);
        const int color = Debris_color(COLOR(i));

        for (const debris_t &debris : debrisList)
        {
            Gui_paint_spark(color,
                            x + debris.x,
                            y - debris.y);
        }

        debrisList.clear();
    }
}

static void Paint_wreckages(void)
{
    if (clMap.wreckages.size() > 0)
    {
        for (const auto &wreckage : clMap.wreckages)
        {
            int x = wreckage.x;
            int y = wreckage.y;
            if (wrap(&x, &y))
            {
                bool deadly = (wreckage.wrecktype & 0x80);

                int wtype = (wreckage.wrecktype & 0x7F) % NUM_WRECKAGE_SHAPES;
                int rot = wreckage.rotation;
                int size = wreckage.size;

                Gui_paint_wreck(x, y, deadly, wtype, rot, size);
            }
        }
        clMap.wreckages.clear();
    }
}

static void Paint_asteroids(void)
{
    if (clMap.asteroids.size() > 0)
    {
        Gui_paint_asteroids_begin();
        for (const auto &asteroid : clMap.asteroids)
        {
            int x = asteroid.x;
            int y = asteroid.y;
            if (wrap(&x, &y))
            {
                int type = asteroid.type;
                int rot = asteroid.rotation;
                int size = asteroid.size;

                Gui_paint_asteroid(x, y, type, rot, size);
            }
        }
        Gui_paint_asteroids_end();
        clMap.asteroids.clear();
    }
}

static void Paint_wormholes(void)
{
    if (clMap.wormholes.size() > 0)
    {
        for (const auto &wormhole : clMap.wormholes)
        {
            int x = wormhole.x;
            int y = wormhole.y;
            if (wrap(&x, &y))
                Gui_paint_setup_worm(x, y);
        }
        clMap.wormholes.clear();
    }
}

static void Paint_missiles(void)
{
    if (clMap.missiles.size() > 0)
    {
        Gui_paint_missiles_begin();

        for (const auto &missile : clMap.missiles)
        {
            int x = missile.x;
            int y = missile.y;
            int dir = missile.dir;
            int len = MISSILE_LEN;
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
    if (clMap.lasers.size() > 0)
    {
        Gui_paint_lasers_begin();

        for (const auto &laser : clMap.lasers)
        {
            int x1 = laser.x;
            int y1 = laser.y;
            int len = laser.len;
            int dir = laser.dir;
            int color = laser.color;

            if (wrap(&x1, &y1))
                Gui_paint_laser(color, x1, y1, len, dir);
        }
        Gui_paint_lasers_end();
        clMap.lasers.clear();
    }
}

static void Paint_fastshots(int i, int x_areas, int y_areas, int areas)
{
    auto &fastshotList = clMap.fastshotTypes[i];

    if (fastshotList.empty())
        return;

    const int x = BASE_X(i);
    const int y = BASE_Y(i);
    int color = COLOR(i);
    if (color != WHITE && color != BLUE)
        color = WHITE;

    for (const fastshot_t &fastshot : fastshotList)
    {
        Gui_paint_fastshot(color,
                           x + fastshot.x,
                           y - fastshot.y);
    }

    fastshotList.clear();
}

static void Paint_teamshots(int i, int x_areas, int y_areas, int areas)
{
    auto &teamshotList = clMap.teamshotTypes[i];

    if (teamshotList.empty())
        return;

    const int x = BASE_X(i);
    const int y = BASE_Y(i);

    for (const teamshot_t &teamshot : teamshotList)
    {
        Gui_paint_teamshot(x + teamshot.x,
                           y - teamshot.y);
    }

    teamshotList.clear();
}

void Paint_shots(void)
{
    Paint_items();
    Paint_balls();
    Paint_mines();

    int x_areas = (active_view_width + 255) >> 8;
    int y_areas = (active_view_height + 255) >> 8;
    int areas = x_areas * y_areas;
    int max_i = areas * (num_spark_colors >= 3 ? num_spark_colors : 4);

    // warn("Paint_shots: x_areas: %d", x_areas);
    // warn("Paint_shots: y_areas: %d", y_areas);
    // warn("Paint_shots: areas_: %d", areas);
    // warn("Paint_shots: max_i: %d", max_i);

    Paint_debris(x_areas, y_areas, areas, max_i);

    Paint_wreckages();
    Paint_asteroids();
    Paint_wormholes();

    for (int i = 0; i < max_i; i++)
    {
        Paint_fastshots(i, x_areas, y_areas, areas);
        Paint_teamshots(i, x_areas, y_areas, areas);
    }

    Paint_missiles();
    Paint_lasers();
}

static void Paint_paused(void)
{
    if (clMap.pausers.size() > 0)
    {
        for (const auto &paused : clMap.pausers)
        {
            int x = paused.x;
            int y = paused.y;
            if (wrap(&x, &y))
                Gui_paint_paused(x, y, paused.count);
        }
        clMap.pausers.clear();
    }
}

static void Paint_appearing(void)
{
    if (clMap.appearers.size() > 0)
    {
        for (const auto &appearing : clMap.appearers)
        {
            int x = appearing.x;
            int y = appearing.y;
            if (wrap(&x, &y))
                Gui_paint_appearing(x, y, appearing.id,
                                    appearing.count);
        }
        clMap.appearers.clear();
    }
}

static void Paint_ecm(void)
{
    if (clMap.ecms.size() > 0)
    {
        for (const auto &ecm : clMap.ecms)
        {
            int size = ecm.size;

            if (size > 0)
            {
                int x = ecm.x;
                int y = ecm.y;
                if (wrap(&x, &y))
                    Gui_paint_ecm(x, y, size);
            }
        }
        clMap.ecms.clear();
    }
}

static void Paint_all_ships(void)
{
    if (clMap.ships.size() > 0)
    {
        for (const auto &ship : clMap.ships)
        {
            int x = ship.x;
            int y = ship.y;
            if (!wrap(&x, &y))
                continue;

            /*
             * ship in the center? (svenska-hack)
             */
            if (abs(X(x) - ext_view_width / 2) <= 1 && abs(Y(y) - ext_view_height / 2) <= 1 && Other_by_id(ship.id) != nullptr)
            {
                eyesId = ship.id;
                eyes = Other_by_id(eyesId);
                if (eyes != nullptr)
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
    if (clMap.refuels.size() > 0)
    {
        for (const auto &refuel : clMap.refuels)
        {
            int x0 = refuel.x0;
            int y0 = refuel.y0;
            int x1 = refuel.x1;
            int y1 = refuel.y1;
            if (wrap(&x0, &y0) && wrap(&x1, &y1))
                Gui_paint_refuel(x0, y0, x1, y1);
        }
        clMap.refuels.clear();
    }
}

static void Paint_connectors(void)
{
    if (clMap.connectors.size() > 0)
    {
        for (const auto &connector : clMap.connectors)
        {
            int x0 = connector.x0;
            int y0 = connector.y0;
            int x1 = connector.x1;
            int y1 = connector.y1;
            if (wrap(&x0, &y0) && wrap(&x1, &y1))
                Gui_paint_connector(x0, y0, x1, y1,
                                    connector.tractor);
        }
        clMap.connectors.clear();
    }
}

static void Paint_transporters(void)
{
    if (clMap.transporters.size() > 0)
    {
        for (const auto &transporter : clMap.transporters)
        {
            int x0 = transporter.x1;
            int y0 = transporter.y1;
            int x1 = transporter.x2;
            int y1 = transporter.y2;
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
    point_size = sizeof(position_t) * ANGLE_RESOLUTION;
    total_size = point_size * NUM_WRECKAGE_POINTS * NUM_WRECKAGE_SHAPES;
    if ((dynmem = (char *)malloc(total_size)) == nullptr)
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
