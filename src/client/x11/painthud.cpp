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
#include <cmath>
#include <sys/types.h>

#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>

#include "strlcpy.h"

#include "client.h"
#include "messages.h"
#include "netclient.h"
#include "paint.h"
#include "paintdata.h"

#include "xpconfig.h"
#include "const.h"
#include "xperror.h"
#include "bit.h"
#include "types.h"
#include "keys.h"
#include "rules.h"
#include "setup.h"
#include "paintdata.h"
#include "record.h"
#include "xinit.h"
#include "protoclient.h"
#include "bitmaps.h"

extern setup_t *Setup;
extern int RadarHeight;
extern score_object_t score_objects[MAX_SCORE_OBJECTS];
extern int score_object;
extern XGCValues gcv;

int hudColor = BLUE;             /* Color index for HUD drawing */
int hudHLineColor = BLUE;        /* Color index for horiz. HUD line drawing */
int hudVLineColor = BLUE;        /* Color index for vert. HUD line drawing */
int hudItemsColor = BLUE;        /* Color index for HUD items drawing */
int hudRadarEnemyColor = BLUE;   /* Color index for enemy hudradar dots */
int hudRadarOtherColor = BLUE;   /* Color index for other hudradar dots */
int hudLockColor = BLUE;         /* Color index for lock on HUD drawing */
int fuelGaugeColor = BLUE;       /* Color index for fuel gauge drawing */
int dirPtrColor = BLUE;          /* Color index for dirptr drawing */
int messagesColor = BLUE;        /* Color index for messages */
int oldMessagesColor = BLUE;     /* Color index for old messages */
int msgScanBallColor = BLUE;     /* Color index for ball msg */
int msgScanSafeColor = BLUE;     /* Color index for safe msg */
int msgScanCoverColor = BLUE;    /* Color index for cover msg */
int msgScanPopColor = BLUE;      /* Color index for pop msg */
int fuelMeterColor = BLUE;       /* Color index for fuel meter */
int powerMeterColor = BLUE;      /* Color index for power meter */
int turnSpeedMeterColor = BLUE;  /* Color index for turnspeed meter */
int packetSizeMeterColor = BLUE; /* Color index for packet size meter */
int packetLossMeterColor = BLUE; /* Color index for packet loss meter */
int packetDropMeterColor = BLUE; /* Color index for packet drop meter */
int packetLagMeterColor = BLUE;  /* Color index for packet lag meter */
int temporaryMeterColor = BLUE;  /* Color index for temporary meter drawing */
int meterBorderColor = BLUE;     /* Color index for meter border drawing */
int scoreObjectColor = BLUE;     /* Color index for map score objects */

DFLOAT charsPerTick = 0.0; /* Output speed of messages */

static int meterWidth = 60;
static int meterHeight = 10;

/*
 * Draw a meter of some kind on screen.
 * When the x-offset is specified as a negative value then
 * the meter is drawn relative to the right side of the screen,
 * otherwise from the normal left side.
 */
static void Paint_meter(int xoff, int y, const char *title, int val, int max,
                        int meter_color)
{
    const int mw1_4 = meterWidth / 4,
              mw2_4 = meterWidth / 2,
              mw3_4 = 3 * meterWidth / 4,
              mw4_4 = meterWidth,
              BORDER = 5;
    int x, xstr;

    if (xoff >= 0)
    {
        x = xoff;
        xstr = WINSCALE(x + (int)meterWidth) + BORDER;
    }
    else
    {
        x = ext_view_width - ((int)meterWidth - xoff);
        xstr = WINSCALE(x) - (BORDER + XTextWidth(gameFont, title, (int)strlen(title)));
    }

    Rectangle_add(meter_color,
                  x + 2, y + 2,
                  (int)(((meterWidth - 3) * val) / (max ? max : 1)), meterHeight - 3);

    /* meterBorderColor = 0 obviously means no meter borders are drawn */
    if (meterBorderColor)
    {
        int color = meterBorderColor;

        SET_FG(colors[color].pixel);
        rd.drawRectangle(dpy, drawPixmap, gameGC,
                         WINSCALE(x), WINSCALE(y),
                         WINSCALE(meterWidth), WINSCALE(meterHeight));

        /* Paint scale levels(?) */
        Segment_add(color, x, y - 4, x, y + meterHeight + 4);
        Segment_add(color, x + mw4_4, y - 4, x + mw4_4, y + meterHeight + 4);
        Segment_add(color, x + mw2_4, y - 3, x + mw2_4, y + meterHeight + 3);
        Segment_add(color, x + mw1_4, y - 1, x + mw1_4, y + meterHeight + 1);
        Segment_add(color, x + mw3_4, y - 1, x + mw3_4, y + meterHeight + 1);
    }

    if (!meterBorderColor)
        SET_FG(colors[meter_color].pixel);

    rd.drawString(dpy, drawPixmap, gameGC,
                  xstr, WINSCALE(y) + (gameFont->ascent + meterHeight) / 2,
                  title, (int)strlen(title));

    /* texturedObjects - TODO */
    /*int width = WINSCALE((int)(((meterWidth-3)*val)/(max?max:1)));*/

    /*printf("TODO: implement paint meter\n");*/
    /*PaintMeter(drawPixmap, BM_METER,
      WINSCALE(x), WINSCALE(y),
      WINSCALE(meterWidth), WINSCALE(11),
      width);*/
    /*SET_FG(colors[color].pixel);*/
}

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

void Paint_score_objects(void)
{
    int i, x, y;

    for (i = 0; i < MAX_SCORE_OBJECTS; i++)
    {
        score_object_t *sobj = &score_objects[i];
        if (sobj->count > 0)
        {
            if (sobj->count % 3)
            {
                x = sobj->x * BLOCK_SZ + BLOCK_SZ / 2;
                y = sobj->y * BLOCK_SZ + BLOCK_SZ / 2;
                if (wrap(&x, &y))
                {
                    SET_FG(colors[hudColor].pixel);
                    x = WINSCALE(X(x)) - sobj->msg_width / 2,
                    y = WINSCALE(Y(y)) + gameFont->ascent / 2,
                    rd.drawString(dpy, drawPixmap, gameGC,
                                  x, y,
                                  sobj->msg,
                                  sobj->msg_len);
                }
            }
            sobj->count++;
            if (sobj->count > SCORE_OBJECT_COUNT)
            {
                sobj->count = 0;
                sobj->hud_msg_len = 0;
            }
        }
    }
}

void Paint_meters(void)
{
    int y = 20, color;

    if (fuelMeterColor)
        Paint_meter(-10, y += 20, "Fuel",
                    (int)fuelSum, (int)fuelMax, fuelMeterColor);

    if (powerMeterColor)
        color = powerMeterColor;
    else if (controlTime > 0.0)
        color = temporaryMeterColor;
    else
        color = 0;

    if (color)
        Paint_meter(-10, y += 20, "Power",
                    (int)displayedPower, (int)MAX_PLAYER_POWER, color);

    if (turnSpeedMeterColor)
        color = turnSpeedMeterColor;
    else if (controlTime > 0.0)
        color = temporaryMeterColor;
    else
        color = 0;

    if (color)
        Paint_meter(-10, y += 20, "Turnspeed",
                    (int)displayedTurnspeed, (int)MAX_PLAYER_TURNSPEED, color);

    if (controlTime > 0.0)
    {
        controlTime -= timePerFrame;
        if (controlTime <= 0.0)
            controlTime = 0.0;
    }

    if (packetSizeMeterColor)
        Paint_meter(-10, y += 20, "Packet",
                    (packet_size >= 4096) ? 4096 : packet_size, 4096,
                    packetSizeMeterColor);
    if (packetLossMeterColor)
        Paint_meter(-10, y += 20, "Loss", packet_loss, FPS,
                    packetLossMeterColor);
    if (packetDropMeterColor)
        Paint_meter(-10, y += 20, "Drop", packet_drop, FPS,
                    packetDropMeterColor);
    if (packetLagMeterColor)
        Paint_meter(-10, y += 20, "Lag", MIN(packet_lag, 1 * FPS), 1 * FPS,
                    packetLagMeterColor);

    if (temporaryMeterColor)
    {
        if (thrusttime >= 0 && thrusttimemax > 0)
            Paint_meter((ext_view_width - 300) / 2 - 32, 2 * ext_view_height / 3,
                        "Thrust Left",
                        (thrusttime >= thrusttimemax
                             ? thrusttimemax
                             : thrusttime),
                        thrusttimemax, temporaryMeterColor);

        if (shieldtime >= 0 && shieldtimemax > 0)
            Paint_meter((ext_view_width - 300) / 2 - 32, 2 * ext_view_height / 3 + 20,
                        "Shields Left",
                        (shieldtime >= shieldtimemax
                             ? shieldtimemax
                             : shieldtime),
                        shieldtimemax, temporaryMeterColor);

        if (phasingtime >= 0 && phasingtimemax > 0)
            Paint_meter((ext_view_width - 300) / 2 - 32, 2 * ext_view_height / 3 + 40,
                        "Phasing left",
                        (phasingtime >= phasingtimemax
                             ? phasingtimemax
                             : phasingtime),
                        phasingtimemax, temporaryMeterColor);

        if (destruct > 0)
            Paint_meter((ext_view_width - 300) / 2 - 32, 2 * ext_view_height / 3 + 60,
                        "Self destructing", destruct, (int)SELF_DESTRUCT_DELAY,
                        temporaryMeterColor);

        if (shutdown_count >= 0)
            Paint_meter((ext_view_width - 300) / 2 - 32, 2 * ext_view_height / 3 + 80,
                        "SHUTDOWN", shutdown_count, shutdown_delay,
                        temporaryMeterColor);
    }
}

static void Paint_lock(int hud_pos_x, int hud_pos_y)
{
    const int BORDER = 2;
    int x, y;
    int i, dir = 96;
    int hudShipColor = hudColor;
    other_t *target;
    shipshape_t *ship;
    char str[50];
    static int warningCount;
    static int mapdiag = 0;
    XPoint points[64];

    if (mapdiag == 0)
    {
        mapdiag = (int)LENGTH(Setup->x * BLOCK_SZ, Setup->y * BLOCK_SZ);
    }

    /*
     * Display direction arrow and miscellaneous target information.
     */
    if ((target = Other_by_id(lock_id)) == NULL)
    {
        return;
    }
    FIND_NAME_WIDTH(target);
    rd.drawString(dpy, drawPixmap, gameGC,
                  WINSCALE(hud_pos_x) - target->name_width / 2,
                  WINSCALE(hud_pos_y - HUD_SIZE + HUD_OFFSET - BORDER) - gameFont->descent,
                  target->nick_name, target->name_len);

    /* Only show the mini-ship for the locked player if it will be big enough
     * to even tell what the heck it is!  I choose the arbitrary size of
     * 10 pixels wide, which in practice is a scaleFactor <= 1.5.
     */
    if (
        10 * scaleFactor <= 15)
    {
        ship = Ship_by_id(lock_id);
        for (i = 0; i < ship->num_points; i++)
        {
            points[i].x = WINSCALE((int)(hud_pos_x + ship->pts[i][dir].x / 2 + 60));
            points[i].y = WINSCALE((int)(hud_pos_y + ship->pts[i][dir].y / 2 - 80));
        }
        points[i++] = points[0];
        SET_FG(colors[hudShipColor].pixel);
        rd.fillPolygon(dpy, drawPixmap, gameGC, points, i, Complex, CoordModeOrigin);
    }

    if (BIT(Setup->mode, LIMITED_LIVES))
    { /* lives left is a better info than distance in team games MM */
        sprintf(str, "%03d", target->life);
    }
    else
    {
        sprintf(str, "%03d", lock_dist / BLOCK_SZ);
    }

    if (BIT(Setup->mode, LIMITED_LIVES) || lock_dist != 0)
    {

        if (BIT(Setup->mode, LIMITED_LIVES) && target->life == 0)
            SET_FG(colors[RED].pixel);
        else
            SET_FG(colors[hudColor].pixel);

        rd.drawString(dpy, drawPixmap, gameGC,
                      WINSCALE(hud_pos_x + HUD_SIZE - HUD_OFFSET + BORDER),
                      WINSCALE(hud_pos_y - HUD_SIZE + HUD_OFFSET - BORDER) - gameFont->descent,
                      str, 3);
    }
    SET_FG(colors[hudColor].pixel);

    if (lock_dist != 0)
    {

        if (lock_dist > WARNING_DISTANCE || warningCount++ % 2 == 0)
        {
            int size = MIN(mapdiag / lock_dist, 10);

            if (size == 0)
            {
                size = 1;
            }
            if (self != NULL && ((self->team == target->team && BIT(Setup->mode, TEAM_PLAY)) || (self->alliance != ' ' && self->alliance == target->alliance)))
            {
                Arc_add(hudColor,
                        (int)(hud_pos_x + HUD_SIZE * 0.6 * tcos(lock_dir) - size * 0.5),
                        (int)(hud_pos_y - HUD_SIZE * 0.6 * tsin(lock_dir) - size * 0.5),
                        size, size, 0, 64 * 360);
            }
            else
            {
                SET_FG(colors[hudLockColor].pixel);
                x = (int)(hud_pos_x + HUD_SIZE * 0.6 * tcos(lock_dir) - size * 0.5),
                y = (int)(hud_pos_y - HUD_SIZE * 0.6 * tsin(lock_dir) - size * 0.5),
                rd.fillArc(dpy, drawPixmap, gameGC,
                           WINSCALE(x), WINSCALE(y),
                           WINSCALE(size), WINSCALE(size), 0, 64 * 360);
                SET_FG(colors[hudColor].pixel);
            }
        }
    }
}

void Paint_hudradar(void)
{
    int i;
    int hrscale = 3;
    int hrw = hrscale * 256;
    int hrh = hrscale * RadarHeight;
    float xf = (float)hrw / (float)Setup->width,
          yf = (float)hrh / (float)Setup->height;

    for (i = 0; i < num_radar; i++)
    {

        int sz = radar_ptr[i].size;

        /* skip non-enemy objects */
        if ((sz & 0x80) == 0)
        {

            int x = radar_ptr[i].x * hrscale -
                    (world.x + ext_view_width / 2) * xf;

            int y = radar_ptr[i].y * hrscale -
                    (world.y + ext_view_height / 2) * yf;

            /* skip objects that would be drawn over our ship */
            if (x < SHIP_SZ && x > -SHIP_SZ && y < SHIP_SZ && y > -SHIP_SZ)
                continue;

            if (BIT(Setup->mode, WRAP_PLAY))
            {
                if (x < 0)
                {
                    if (-x > hrw / 2)
                        x += hrw;
                }
                else
                {
                    if (x > hrw / 2)
                        x -= hrw;
                }

                if (y < 0)
                {
                    if (-y > hrh / 2)
                        y += hrh;
                }
                else
                {
                    if (y > hrh / 2)
                        y -= hrh;
                }
            }

            sz = (sz > 0) ? sz * hrscale : hrscale;

            Arc_add(hudColor,
                    x + ext_view_width / 2 - sz / 2,
                    -y + ext_view_height / 2 - sz / 2,
                    sz, sz, 0, 64 * 360);
        }
    }
}

void Paint_HUD(void)
{
    const int BORDER = 3;
    int vert_pos, horiz_pos, size;
    char str[50];
    int hud_pos_x;
    int hud_pos_y;
    int did_fuel = 0;
    int i, j, maxWidth = -1,
              rect_x, rect_y, rect_width, rect_height;
    static int vertSpacing = -1;
    static char autopilot[] = "Autopilot";

    int modlen = 0;

    /*
     * Show speed pointer
     */
    if (ptr_move_fact != 0.0 && selfVisible != 0 && (selfVel.x != 0 || selfVel.y != 0))
    {
        Segment_add(hudColor,
                    ext_view_width / 2,
                    ext_view_height / 2,
                    (int)(ext_view_width / 2 - ptr_move_fact * selfVel.x),
                    (int)(ext_view_height / 2 + ptr_move_fact * selfVel.y));
    }

    if (instruments.showHUDRadar)
        Paint_hudradar();

    if (!instruments.showHUD)
    {
        return;
    }

    /*
     * Display the HUD
     */
    SET_FG(colors[hudColor].pixel);

    hud_pos_x = (int)(ext_view_width / 2 - hud_move_fact * selfVel.x);
    hud_pos_y = (int)(ext_view_height / 2 + hud_move_fact * selfVel.y);

    /* HUD frame */
    gcv.line_style = LineOnOffDash;
    XChangeGC(dpy, gameGC, GCLineStyle | GCDashOffset, &gcv);

    if (instruments.horizontalHUDLine)
    {
        rd.drawLine(dpy, drawPixmap, gameGC,
                    WINSCALE(hud_pos_x - HUD_SIZE), WINSCALE(hud_pos_y - HUD_SIZE + HUD_OFFSET),
                    WINSCALE(hud_pos_x + HUD_SIZE), WINSCALE(hud_pos_y - HUD_SIZE + HUD_OFFSET));
        rd.drawLine(dpy, drawPixmap, gameGC,
                    WINSCALE(hud_pos_x - HUD_SIZE), WINSCALE(hud_pos_y + HUD_SIZE - HUD_OFFSET),
                    WINSCALE(hud_pos_x + HUD_SIZE), WINSCALE(hud_pos_y + HUD_SIZE - HUD_OFFSET));
    }
    if (instruments.verticalHUDLine)
    {
        rd.drawLine(dpy, drawPixmap, gameGC,
                    WINSCALE(hud_pos_x - HUD_SIZE + HUD_OFFSET), WINSCALE(hud_pos_y - HUD_SIZE),
                    WINSCALE(hud_pos_x - HUD_SIZE + HUD_OFFSET), WINSCALE(hud_pos_y + HUD_SIZE));
        rd.drawLine(dpy, drawPixmap, gameGC,
                    WINSCALE(hud_pos_x + HUD_SIZE - HUD_OFFSET), WINSCALE(hud_pos_y - HUD_SIZE),
                    WINSCALE(hud_pos_x + HUD_SIZE - HUD_OFFSET), WINSCALE(hud_pos_y + HUD_SIZE));
    }
    gcv.line_style = LineSolid;
    XChangeGC(dpy, gameGC, GCLineStyle, &gcv);

    /* Special itemtypes */
    if (vertSpacing < 0)
        vertSpacing = MAX(ITEM_SIZE, gameFont->ascent + gameFont->descent) + 1;
    /* find the scaled location, then work in pixels */
    vert_pos = WINSCALE(hud_pos_y - HUD_SIZE + HUD_OFFSET + BORDER);
    horiz_pos = WINSCALE(hud_pos_x - HUD_SIZE + HUD_OFFSET - BORDER);
    rect_width = 0;
    rect_height = 0;
    rect_x = horiz_pos;
    rect_y = vert_pos;

    for (i = 0; i < NUM_ITEMS; i++)
    {
        int num = numItems[i];

        if (i == ITEM_FUEL)
            continue;

        if (instruments.showItems)
        {
            lastNumItems[i] = num;
            if (num <= 0)
                num = -1;
        }
        else
        {
            if (num != lastNumItems[i])
            {
                numItemsTime[i] = (int)(showItemsTime * (float)FPS);
                lastNumItems[i] = num;
            }
            if (numItemsTime[i]-- <= 0)
            {
                numItemsTime[i] = 0;
                num = -1;
            }
        }

        if (num >= 0)
        {
            int len, width;

            /* Paint item symbol */
            Paint_item_symbol((uint8_t)i, drawPixmap, gameGC,
                              horiz_pos - ITEM_SIZE,
                              vert_pos,
                              ITEM_HUD);

            if (i == lose_item)
            {
                if (lose_item_active != 0)
                {
                    if (lose_item_active < 0)
                    {
                        lose_item_active++;
                    }
                    rd.drawRectangle(dpy, drawPixmap, gameGC,
                                     horiz_pos - ITEM_SIZE - 2,
                                     vert_pos - 2, ITEM_SIZE + 2, ITEM_SIZE + 2);
                }
            }

            /* Paint item count */
            sprintf(str, "%d", num);
            len = strlen(str);
            width = XTextWidth(gameFont, str, len);
            rd.drawString(dpy, drawPixmap, gameGC,
                          horiz_pos - ITEM_SIZE - BORDER - width,
                          vert_pos + ITEM_SIZE / 2 + gameFont->ascent / 2,
                          str, len);

            maxWidth = MAX(maxWidth, width + BORDER + ITEM_SIZE);
            vert_pos += vertSpacing;

            if (vert_pos + vertSpacing > WINSCALE(hud_pos_y + HUD_SIZE - HUD_OFFSET - BORDER))
            {
                rect_width += maxWidth + 2 * BORDER;
                rect_height = MAX(rect_height, vert_pos - rect_y);
                horiz_pos -= maxWidth + 2 * BORDER;
                vert_pos = WINSCALE(hud_pos_y - HUD_SIZE + HUD_OFFSET + BORDER);
                maxWidth = -1;
            }
        }
    }
    if (maxWidth != -1)
    {
        rect_width += maxWidth + BORDER;
    }
    if (rect_width > 0)
    {
        if (rect_height == 0)
        {
            rect_height = vert_pos - rect_y;
        }
        rect_x -= rect_width;
    }

    /* Fuel notify, HUD meter on */
    if (fuelCount || fuelSum < fuelLevel3)
    {
        did_fuel = 1;
        sprintf(str, "%04d", (int)fuelSum);
        rd.drawString(dpy, drawPixmap, gameGC,
                      WINSCALE(hud_pos_x + HUD_SIZE - HUD_OFFSET + BORDER),
                      WINSCALE(hud_pos_y + HUD_SIZE - HUD_OFFSET + BORDER) + gameFont->ascent,
                      str, strlen(str));
        if (numItems[ITEM_TANK])
        {
            if (fuelCurrent == 0)
                strcpy(str, "M ");
            else
                sprintf(str, "T%d", fuelCurrent);
            rd.drawString(dpy, drawPixmap, gameGC,
                          WINSCALE(hud_pos_x + HUD_SIZE - HUD_OFFSET + BORDER),
                          WINSCALE(hud_pos_y + HUD_SIZE - HUD_OFFSET + BORDER) + gameFont->descent + 2 * gameFont->ascent,
                          str, strlen(str));
        }
    }

    /* Update the lock display */
    Paint_lock(hud_pos_x, hud_pos_y);

    /* Draw last score on hud if it is an message attached to it */
    for (i = 0, j = 0; i < MAX_SCORE_OBJECTS; i++)
    {
        score_object_t *sobj = &score_objects[(i + score_object) % MAX_SCORE_OBJECTS];
        if (sobj->hud_msg_len > 0)
        {
            if (j == 0 &&
                sobj->hud_msg_width > WINSCALE(2 * HUD_SIZE - HUD_OFFSET * 2) &&
                (did_fuel || instruments.verticalHUDLine))
                ++j;
            rd.drawString(dpy, drawPixmap, gameGC,
                          WINSCALE(hud_pos_x) - sobj->hud_msg_width / 2,
                          WINSCALE(hud_pos_y + HUD_SIZE - HUD_OFFSET + BORDER) + gameFont->ascent + j * (gameFont->ascent + gameFont->descent),
                          sobj->hud_msg, sobj->hud_msg_len);
            j++;
        }
    }

    if (time_left > 0)
    {
        sprintf(str, "%3d:%02d", (int)(time_left / 60), (int)(time_left % 60));
        size = XTextWidth(gameFont, str, strlen(str));
        rd.drawString(dpy, drawPixmap, gameGC,
                      WINSCALE(hud_pos_x - HUD_SIZE + HUD_OFFSET - BORDER) - size,
                      WINSCALE(hud_pos_y - HUD_SIZE + HUD_OFFSET - BORDER) - gameFont->descent,
                      str, strlen(str));
    }

    /* Update the modifiers */
    modlen = strlen(mods);
    rd.drawString(dpy, drawPixmap, gameGC,
                  WINSCALE(hud_pos_x - HUD_SIZE + HUD_OFFSET - BORDER) - XTextWidth(gameFont, mods, modlen),
                  WINSCALE(hud_pos_y + HUD_SIZE - HUD_OFFSET + BORDER) + gameFont->ascent,
                  mods, strlen(mods));

    if (autopilotLight)
    {
        int text_width = XTextWidth(gameFont, autopilot, sizeof(autopilot) - 1);
        rd.drawString(dpy, drawPixmap, gameGC,
                      WINSCALE(hud_pos_x) - text_width / 2,
                      WINSCALE(hud_pos_y - HUD_SIZE + HUD_OFFSET - BORDER) - gameFont->descent * 2 - gameFont->ascent,
                      autopilot, sizeof(autopilot) - 1);
    }

    if (fuelCount > 0)
    {
        fuelCount--;
    }

    /* Fuel gauge, must be last */
    if (instruments.fuelGauge == 0 || !((fuelCount) || (fuelSum < fuelLevel3 && ((fuelSum < fuelLevel1 && (loops % 4) < 2) || (fuelSum < fuelLevel2 && fuelSum > fuelLevel1 && (loops % 8) < 4) || (fuelSum > fuelLevel2)))))
        return;

    rd.drawRectangle(dpy, drawPixmap, gameGC,
                     WINSCALE(hud_pos_x + HUD_SIZE - HUD_OFFSET + FUEL_GAUGE_OFFSET) - 1,
                     WINSCALE(hud_pos_y - HUD_SIZE + HUD_OFFSET + FUEL_GAUGE_OFFSET) - 1,
                     WINSCALE(HUD_OFFSET - (2 * FUEL_GAUGE_OFFSET)) + 3,
                     WINSCALE(HUD_FUEL_GAUGE_SIZE) + 3);

    size = (HUD_FUEL_GAUGE_SIZE * fuelSum) / fuelMax;
    rd.fillRectangle(dpy, drawPixmap, gameGC,
                     WINSCALE(hud_pos_x + HUD_SIZE - HUD_OFFSET + FUEL_GAUGE_OFFSET) + 1,
                     WINSCALE(hud_pos_y - HUD_SIZE + HUD_OFFSET + FUEL_GAUGE_OFFSET + HUD_FUEL_GAUGE_SIZE - size) + 1,
                     WINSCALE(HUD_OFFSET - (2 * FUEL_GAUGE_OFFSET)),
                     WINSCALE(size));
}

void Paint_messages(void)
{
    int i, x, y, top_y, bot_y, width, len;
    const int BORDER = 10,
              SPACING = messageFont->ascent + messageFont->descent + 1;
    message_t *msg;
    int msg_color;
    int last_msg_index = 0;

    if (charsPerTick <= 0.0)
        charsPerTick = (float)charsPerSecond / FPS;

    top_y = BORDER + messageFont->ascent;
    bot_y = WINSCALE(ext_view_height) - messageFont->descent - BORDER;

    /* get number of player messages */
    if (selectionAndHistory)
    {
        while (last_msg_index < maxMessages && TalkMsg[last_msg_index]->len != 0)
        {
            last_msg_index++;
        }
        last_msg_index--; /* make it an index */
    }

    for (i = 0; i < 2 * maxMessages; i++)
    {
        if (i < maxMessages)
        {
            msg = TalkMsg[i];
        }
        else
        {
            msg = GameMsg[i - maxMessages];
        }
        if (msg->len == 0)
            continue;

        /*
         * while there is something emphasized, freeze the life time counter
         * of a message if it is not drawn `flashed' (red) anymore
         */
        if (msg->life > MSG_FLASH || !selectionAndHistory || (selection.draw.state != SEL_PENDING && selection.draw.state != SEL_EMPHASIZED))
        {
            if (msg->life-- <= 0)
            {
                msg->txt[0] = '\0';
                msg->len = 0;
                msg->life = 0;
                continue;
            }
        }
        if (i < maxMessages)
        {
            x = BORDER;
            y = top_y;
            top_y += SPACING;
        }
        else
        {
            if (!instruments.showMessages)
            {
                continue;
            }
            x = BORDER;
            y = bot_y;
            bot_y -= SPACING;
        }
        len = (int)(charsPerTick * (MSG_DURATION - msg->life));
        len = MIN(msg->len, len);
        if (msg->life > MSG_FLASH)
        {
            msg_color = RED;
        }
        else
        {
            msg_color = oldMessagesColor;
        }

        /*
         * it's an emphasized talk message
         */
        if (selectionAndHistory && selection.draw.state == SEL_EMPHASIZED && i < maxMessages && TALK_MSG_SCREENPOS(last_msg_index, i) >= selection.draw.y1 && TALK_MSG_SCREENPOS(last_msg_index, i) <= selection.draw.y2)
        {

            /*
             * three strings (ptr), where they begin (xoff) and their
             * length (l):
             *   1st is an umemph. string to the left of a selection,
             *   2nd an emphasized part itself,
             *   3rd an unemph. part to the right of a selection.
             * set the according variables if a part exists.
             * e.g: a selection of several lines `stopping' somewhere in
             *   the middle of a line -> ptr2,ptr3 are needed to draw
             *   this line
             */
            char *ptr = NULL;
            int xoff = 0, l = 0;
            char *ptr2 = NULL;
            int xoff2 = 0, l2 = 0;
            char *ptr3 = NULL;
            int xoff3 = 0, l3 = 0;

            if (TALK_MSG_SCREENPOS(last_msg_index, i) > selection.draw.y1 && TALK_MSG_SCREENPOS(last_msg_index, i) < selection.draw.y2)
            {
                /* all emphasized on this line */
                /*xxxxxxxxx*/
                ptr2 = msg->txt;
                l2 = len;
                xoff2 = 0;
            }
            else if (TALK_MSG_SCREENPOS(last_msg_index, i) == selection.draw.y1)
            {
                /* first/only line */
                /*___xxx[___]*/
                ptr = msg->txt;
                xoff = 0;
                if (len < selection.draw.x1)
                {
                    l = len;
                }
                else
                {
                    /* at least two parts */
                    /*___xxx[___]*/
                    /*    ^      */
                    l = selection.draw.x1;
                    ptr2 = &(msg->txt[selection.draw.x1]);
                    xoff2 = XTextWidth(messageFont, msg->txt, selection.draw.x1);

                    if (TALK_MSG_SCREENPOS(last_msg_index, i) < selection.draw.y2)
                    {
                        /* first line */
                        /*___xxxxxx*/
                        /*     ^   */
                        l2 = len - selection.draw.x1;
                    }
                    else
                    {
                        /* only line */
                        /*___xxx___*/
                        if (len <= selection.draw.x2)
                        {
                            /*___xxx___*/
                            /*    ^    */
                            l2 = len - selection.draw.x1;
                        }
                        else
                        {
                            /*___xxx___*/
                            /*       ^ */
                            l2 = selection.draw.x2 - selection.draw.x1 + 1;
                            ptr3 = &(msg->txt[selection.draw.x2 + 1]);
                            xoff3 = XTextWidth(messageFont, msg->txt, selection.draw.x2 + 1);
                            l3 = len - selection.draw.x2 - 1;
                        }
                    } /* only line */
                } /* at least two parts */
            }
            else
            {
                /* last line */
                /*xxxxxx[___]*/
                ptr2 = msg->txt;
                xoff2 = 0;
                if (len <= selection.draw.x2 + 1)
                {
                    /* all blue */
                    /*xxxxxx[___]*/
                    /*  ^        */
                    l2 = len;
                }
                else
                {
                    /*xxxxxx___*/
                    /*       ^ */
                    l2 = selection.draw.x2 + 1;
                    ptr3 = &(msg->txt[selection.draw.x2 + 1]);
                    xoff3 = XTextWidth(messageFont, msg->txt, selection.draw.x2 + 1);
                    l3 = len - selection.draw.x2 - 1;
                }
            } /* last line */

            if (ptr)
            {
                XSetForeground(dpy, messageGC, colors[msg_color].pixel);
                rd.drawString(dpy, drawPixmap, messageGC, x + xoff, y, ptr, l);
            }
            if (ptr2)
            {
                XSetForeground(dpy, messageGC, colors[DRAW_EMPHASIZED].pixel);
                rd.drawString(dpy, drawPixmap, messageGC, x + xoff2, y, ptr2, l2);
            }
            if (ptr3)
            {
                XSetForeground(dpy, messageGC, colors[msg_color].pixel);
                rd.drawString(dpy, drawPixmap, messageGC, x + xoff3, y, ptr3, l3);
            }
        }
        else /* not emphasized */
        {
            XSetForeground(dpy, messageGC, colors[msg_color].pixel);
            rd.drawString(dpy, drawPixmap, messageGC, x, y, msg->txt, len);
        }

        if (len < msg->len)
        {
            width = XTextWidth(messageFont, msg->txt, len);
        }
        else
        {
            // TODO: Always calculate it here, remove msg->pixelLen
            width = msg->pixelLen;
        }
    }
}

void Paint_recording(void)
{
    int w = -1;
    int x, y;
    char buf[32];
    int mb, ck, len;
    long size;

    if (!recording || (loops % 16) < 8)
        return;

    SET_FG(colors[RED].pixel);
    size = Record_size();
    mb = size >> 20;
    ck = (10 * (size - ((long)mb << 20))) >> 20;
    sprintf(buf, "REC %d.%d", mb, ck);
    len = strlen(buf);
    w = XTextWidth(gameFont, buf, len);
    x = WINSCALE(ext_view_width) - 10 - w;
    y = 10 + gameFont->ascent;
    XDrawString(dpy, drawPixmap, gameGC, x, y, buf, len);
}

void Paint_HUD_values(void)
{
    int w, x, y, len, w2, len2, wmax;
    static char buf[32], buf2[32];

    SET_FG(colors[BLUE].pixel);

    sprintf(buf, "CL.FPS : %.3f", clientFPS);
    sprintf(buf2, "CL.LAG : %d us", clientLag);

    len = strlen(buf);
    w = XTextWidth(gameFont, buf, len);
    len2 = strlen(buf2);
    w2 = XTextWidth(gameFont, buf2, len2);

    wmax = MAX(w, w2);

    x = WINSCALE(ext_view_width) - 10 - wmax;
    y = 200 + gameFont->ascent;
    rd.drawString(dpy, drawPixmap, gameGC, x, y, buf, len);

    x = WINSCALE(ext_view_width) - 10 - wmax;
    y = 220 + gameFont->ascent;
    rd.drawString(dpy, drawPixmap, gameGC, x, y, buf2, len2);
}
