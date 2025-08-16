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
#include <cmath>
#include <cerrno>
#include <ctime>
#include <climits>
#include <sys/types.h>

#include <unistd.h>
#include <sys/param.h>

#include "commonmacros.h"
#include "draw.h"
#include "strlcpy.h"

#define SERVER
#include "xpconfig.h"
#include "serverconst.h"
#include "global.h"
#include "proto.h"
#include "bit.h"
#include "netserver.h"
#include "saudio.h"
#include "xperror.h"
#include "xpmath.h"
#include "player.h"

#define MAX_SHUFFLE_INDEX 65535

typedef unsigned short shuffle_t;

/*
 * Structure for calculating if a pixel is visible by a player.
 * The following always holds:
 *        (world.x >= realWorld.x && world.y >= realWorld.y)
 */
typedef struct
{
    position_t world;     /* Lower left hand corner is this */
                          /* world coordinate */
    position_t realWorld; /* If the player is on the edge of
                     the screen, these are the world
                     coordinates before adjustment... */
} pixel_visibility_t;

/*
 * Structure with player position info measured in blocks instead of pixels.
 * Used for map state info updating.
 */
// typedef struct
// {
//     ipos_t world;
//     ipos_t realWorld;
// } block_visibility_t;

typedef struct
{
    clpos_t world;
    clpos_t realWorld;
} click_visibility_t;

typedef struct
{
    uint8_t x, y;
} debris_t;

typedef struct
{
    short x, y, size;
} radar_t;

extern time_t gameOverTime;
long frame_loops = 1;
static long last_frame_shuffle;
static shuffle_t *object_shuffle_ptr;
static int num_object_shuffle;
static int max_object_shuffle;
static shuffle_t *player_shuffle_ptr;
static int num_player_shuffle;
static int max_player_shuffle;
static radar_t *radar_ptr;
static int num_radar, max_radar;

static pixel_visibility_t pv;
static click_visibility_t cv;
static int view_width,
    view_height,
    view_click_width,
    view_click_height,
    horizontal_blocks,
    vertical_blocks,
    debris_x_areas,
    debris_y_areas,
    debris_areas,
    debris_colors,
    spark_rand;
static debris_t *debris_ptr[DEBRIS_TYPES];
static unsigned debris_num[DEBRIS_TYPES],
    debris_max[DEBRIS_TYPES];
static debris_t *fastshot_ptr[DEBRIS_TYPES * 2];
static unsigned fastshot_num[DEBRIS_TYPES * 2],
    fastshot_max[DEBRIS_TYPES * 2];

// #define inview(x_, y_)                                                  \
//     ((((x_) > pv.world.x && (x_) < pv.world.x + view_width) ||          \
//       ((x_) > pv.realWorld.x && (x_) < pv.realWorld.x + view_width)) && \
//      (((y_) > pv.world.y && (y_) < pv.world.y + view_height) ||         \
//       ((y_) > pv.realWorld.y && (y_) < pv.realWorld.y + view_height)))

static bool click_inview(click_visibility_t &cv, int cx, int cy)
{
    return (((cx > cv.world.cx && cx < cv.world.cx + view_click_width) ||
             (cx > cv.realWorld.cx && cx < cv.realWorld.cx + view_click_width)) &&
            ((cy > cv.world.cy && cy < cv.world.cy + view_click_height) ||
             (cy > cv.realWorld.cy && cy < cv.realWorld.cy + view_click_height)));
}

// static int block_inview(block_visibility_t *bv, int x, int y)
// {
//     return ((x > bv->world.x && x < bv->world.x + horizontal_blocks) ||
//             (x > bv->realWorld.x && x < bv->realWorld.x + horizontal_blocks)) &&
//            ((y > bv->world.y && y < bv->world.y + vertical_blocks) ||
//             (y > bv->realWorld.y && y < bv->realWorld.y + vertical_blocks));
// }

static void fastshot_store(int xf, int yf, int color, int offset)
{
    int i;
    if (xf < 0)
    {
        xf += world->width;
    }
    if (yf < 0)
    {
        yf += world->height;
    }
    if ((unsigned)xf >= (unsigned)view_width || (unsigned)yf >= (unsigned)view_height)
    {
        /*
         * There's some rounding error or so somewhere.
         * Should be possible to resolve it.
         */
        return;
    }

    i = offset + color * debris_areas + (((yf >> 8) % debris_y_areas) * debris_x_areas) + ((xf >> 8) % debris_x_areas);

    if ((fastshot_num[i]) >= 255)
    {
        return;
    }
    if ((fastshot_num[i]) >= (fastshot_max[i]))
    {
        if ((fastshot_num[i]) == 0)
        {
            (fastshot_ptr[i]) = (debris_t *)malloc(((fastshot_max[i]) = 16) * sizeof(*(fastshot_ptr[i])));
        }
        else
        {
            (fastshot_ptr[i]) = (debris_t *)realloc((fastshot_ptr[i]), ((fastshot_max[i]) += (fastshot_max[i])) * sizeof(*(fastshot_ptr[i])));
        }
        if ((fastshot_ptr[i]) == 0)
        {
            xperror("No memory for debris");
            (fastshot_num[i]) = 0;
            return;
        }
    }
    (fastshot_ptr[i])[(fastshot_num[i])].x = (uint8_t)xf;
    (fastshot_ptr[i])[(fastshot_num[i])].y = (uint8_t)yf;
    (fastshot_num[i])++;
}

static void debris_store(int xf, int yf, int color)
{
    int i;
    int offset = 0;
    if (xf < 0)
    {
        xf += world->width;
    }
    if (yf < 0)
    {
        yf += world->height;
    }
    if ((unsigned)xf >= (unsigned)view_width || (unsigned)yf >= (unsigned)view_height)
    {
        /*
         * There's some rounding error or so somewhere.
         * Should be possible to resolve it.
         */
        return;
    }

    i = offset + color * debris_areas + (((yf >> 8) % debris_y_areas) * debris_x_areas) + ((xf >> 8) % debris_x_areas);

    if ((debris_num[i]) >= 255)
    {
        return;
    }
    if ((debris_num[i]) >= (debris_max[i]))
    {
        if ((debris_num[i]) == 0)
        {
            (debris_ptr[i]) = (debris_t *)malloc(((debris_max[i]) = 16) * sizeof(*(debris_ptr[i])));
        }
        else
        {
            (debris_ptr[i]) = (debris_t *)realloc((debris_ptr[i]), ((debris_max[i]) += (debris_max[i])) * sizeof(*(debris_ptr[i])));
        }
        if ((debris_ptr[i]) == 0)
        {
            xperror("No memory for debris");
            (debris_num[i]) = 0;
            return;
        }
    }
    (debris_ptr[i])[(debris_num[i])].x = (uint8_t)xf;
    (debris_ptr[i])[(debris_num[i])].y = (uint8_t)yf;
    (debris_num[i])++;
}

static void fastshot_end(connection_t *conn)
{
    int i;

    for (i = 0; i < DEBRIS_TYPES * 2; i++)
    {
        if (fastshot_num[i] != 0)
        {
            Send_fastshot(conn, i,
                          (uint8_t *)fastshot_ptr[i],
                          fastshot_num[i]);
            fastshot_num[i] = 0;
        }
    }
}

static void debris_end(connection_t *conn)
{
    int i;

    for (i = 0; i < DEBRIS_TYPES; i++)
    {
        if (debris_num[i] != 0)
        {
            Send_debris(conn, i,
                        (uint8_t *)debris_ptr[i],
                        debris_num[i]);
            debris_num[i] = 0;
        }
    }
}

static void Frame_radar_buffer_reset(void)
{
    num_radar = 0;
}

static void Frame_radar_buffer_add(int cx, int cy, int s)
{
    radar_t *p;

    EXPAND(radar_ptr, num_radar, max_radar, radar_t, 1);
    p = &radar_ptr[num_radar++];
    p->x = CLICK_TO_PIXEL(cx);
    p->y = CLICK_TO_PIXEL(cy);
    p->size = s;
}

static void Frame_radar_buffer_send(connection_t *conn)
{
    int i;
    int dest;
    int tmp;
    radar_t *p;
    const int radar_width = 256;
    int radar_height = (radar_width * world->y) / world->x;
    int radar_x;
    int radar_y;
    int send_x;
    int send_y;
    shuffle_t *radar_shuffle;
    size_t shuffle_bufsize;

    if (num_radar > MIN(256, MAX_SHUFFLE_INDEX))
        num_radar = MIN(256, MAX_SHUFFLE_INDEX);
    shuffle_bufsize = (num_radar * sizeof(shuffle_t));
    radar_shuffle = (shuffle_t *)malloc(shuffle_bufsize);
    if (radar_shuffle == (shuffle_t *)NULL)
        return;
    for (i = 0; i < num_radar; i++)
        radar_shuffle[i] = i;
    /* permute. */
    for (i = 0; i < num_radar; i++)
    {
        dest = (int)(rfrac() * num_radar);
        tmp = radar_shuffle[i];
        radar_shuffle[i] = radar_shuffle[dest];
        radar_shuffle[dest] = tmp;
    }

    if (Get_conn_version(conn) <= 0x4400)
    {
        for (i = 0; i < num_radar; i++)
        {
            p = &radar_ptr[radar_shuffle[i]];
            radar_x = (radar_width * p->x) / world->width;
            radar_y = (radar_height * p->y) / world->height;
            send_x = (world->width * radar_x) / radar_width;
            send_y = (world->height * radar_y) / radar_height;
            Send_radar(conn, send_x, send_y, p->size);
        }
    }
    else
    {
        uint8_t buf[3 * 256];
        int buf_index = 0;
        int fast_count = 0;

        if (num_radar > 256)
            num_radar = 256;
        for (i = 0; i < num_radar; i++)
        {
            p = &radar_ptr[radar_shuffle[i]];
            radar_x = (radar_width * p->x) / world->width;
            radar_y = (radar_height * p->y) / world->height;
            if (radar_y >= 1024)
                continue;
            buf[buf_index++] = (uint8_t)(radar_x);
            buf[buf_index++] = (uint8_t)(radar_y & 0xFF);
            buf[buf_index] = (uint8_t)((radar_y >> 2) & 0xC0);
            if (p->size & 0x80)
                buf[buf_index] |= (uint8_t)(0x20);
            buf[buf_index] |= (uint8_t)(p->size & 0x07);
            buf_index++;
            fast_count++;
        }
        if (fast_count > 0)
            Send_fastradar(conn, buf, fast_count);
    }

    free(radar_shuffle);
}

static void Frame_radar_buffer_free(void)
{
    XFREE(radar_ptr);
    num_radar = 0;
    max_radar = 0;
}

/*
 * Fast conversion of `num' into `str' starting at position `i', returns
 * index of character after converted number.
 */
static int num2str(int num, char *str, int i)
{
    int digits, t;

    if (num < 0)
    {
        str[i++] = '-';
        num = -num;
    }
    if (num < 10)
    {
        str[i++] = '0' + num;
        return i;
    }
    for (t = num, digits = 0; t; t /= 10, digits++)
        ;
    for (t = i + digits - 1; t >= 0; t--)
    {
        str[t] = num % 10;
        num /= 10;
    }
    return i + digits;
}

static int Frame_status(connection_t *conn, int ind)
{
    static char mods[MAX_CHARS];
    player_t *pl = Players[ind];
    int n,
        lock_ind,
        lock_id = NO_ID,
        lock_dist = 0,
        lock_dir = 0,
        i,
        showautopilot;

    /*
     * Don't make lock visible during this frame if;
     * 0) we are not player locked or compass is not on.
     * 1) we have limited visibility and the player is out of range.
     * 2) the player is invisible and he's not in our team.
     * 3) he's not actively playing.
     * 4) we have blind mode and he's not on the visible screen.
     * 5) his distance is zero.
     */

    CLR_BIT(pl->lock.tagged, LOCK_VISIBLE);
    if (BIT(pl->lock.tagged, LOCK_PLAYER) && BIT(pl->used, HAS_COMPASS))
    {
        lock_id = pl->lock.pl_id;
        lock_ind = GetInd[lock_id];

        if ((!BIT(world->rules->mode, LIMITED_VISIBILITY) || pl->lock.distance <= pl->sensor_range)
#ifndef SHOW_CLOAKERS_RANGE
            && (pl->visibility[lock_ind].canSee ||
                Player_owns_tank(pl, Players[lock_ind]) ||
                Players_are_teammates(pl, Players[lock_ind]) ||
                Players_are_allies(pl, Players[lock_ind]))
#endif
            && BIT(Players[lock_ind]->status, PLAYING | GAME_OVER) == PLAYING &&
            (options.playersOnRadar || click_inview(cv, Players[lock_ind]->pos.cx, Players[lock_ind]->pos.cy)) &&
            pl->lock.distance != 0)
        {
            SET_BIT(pl->lock.tagged, LOCK_VISIBLE);
            lock_dir = (int)Wrap_findDir((int)(Players[lock_ind]->pos.x - pl->pos.x),
                                         (int)(Players[lock_ind]->pos.y - pl->pos.y));
            lock_dist = (int)pl->lock.distance;
        }
    }

    if (BIT(pl->status, HOVERPAUSE))
        showautopilot = (pl->count <= 0 || (frame_loops % 8) < 4);
    else if (BIT(pl->used, HAS_AUTOPILOT))
        showautopilot = (frame_loops % 8) < 4;
    else
        showautopilot = 0;

    /*
     * Don't forget to modify Receive_modifier_bank() in netserver.c
     */
    i = 0;
    if (BIT(pl->mods.nuclear, FULLNUCLEAR))
        mods[i++] = 'F';
    if (BIT(pl->mods.nuclear, NUCLEAR))
        mods[i++] = 'N';
    if (BIT(pl->mods.warhead, CLUSTER))
        mods[i++] = 'C';
    if (BIT(pl->mods.warhead, IMPLOSION))
        mods[i++] = 'I';
    if (pl->mods.velocity)
    {
        if (i)
            mods[i++] = ' ';
        mods[i++] = 'V';
        i = num2str(pl->mods.velocity, mods, i);
    }
    if (pl->mods.mini)
    {
        if (i)
            mods[i++] = ' ';
        mods[i++] = 'X';
        i = num2str(pl->mods.mini + 1, mods, i);
    }
    if (pl->mods.spread)
    {
        if (i)
            mods[i++] = ' ';
        mods[i++] = 'Z';
        i = num2str(pl->mods.spread, mods, i);
    }
    if (pl->mods.power)
    {
        if (i)
            mods[i++] = ' ';
        mods[i++] = 'B';
        i = num2str(pl->mods.power, mods, i);
    }
    if (pl->mods.laser)
    {
        if (i)
            mods[i++] = ' ';
        mods[i++] = 'L';
        mods[i++] = (BIT(pl->mods.laser, STUN) ? 'S' : 'B');
    }
    mods[i] = '\0';
    n = Send_self(conn,
                  pl,
                  lock_id,
                  lock_dist,
                  lock_dir,
                  showautopilot,
                  Players[GetInd[Get_player_id(conn)]]->status,
                  mods);
    if (n <= 0)
    {
        return 0;
    }

    if (BIT(pl->used, HAS_EMERGENCY_THRUST))
        Send_thrusttime(conn,
                        pl->emergency_thrust_left,
                        pl->emergency_thrust_max);
    if (BIT(pl->used, HAS_EMERGENCY_SHIELD))
        Send_shieldtime(conn,
                        pl->emergency_shield_left,
                        pl->emergency_shield_max);
    if (BIT(pl->status, SELF_DESTRUCT) && pl->count > 0)
        Send_destruct(conn, pl->count);
    if (BIT(pl->used, HAS_PHASING_DEVICE))
        Send_phasingtime(conn,
                         pl->phasing_left,
                         pl->phasing_max);
    if (ShutdownServer != -1)
        Send_shutdown(conn, ShutdownServer, ShutdownDelay);

    if (round_delay_send > 0)
        Send_rounddelay(conn, round_delay, options.roundDelaySeconds * FPS);

    return 1;
}

static void Frame_map(connection_t *conn, player_t *pl)
{
    int i, k;
    // int bx, by;
    int conn_bit = (1 << conn->conn_index);
    // block_visibility_t bv;
    const int fuel_packet_size = 5;
    const int cannon_packet_size = 5;
    const int target_packet_size = 7;
    const int wormhole_packet_size = 5;
    int bytes_left = 2000;
    int max_packet;
    int packet_count;

    // bx = OBJ_X_IN_BLOCKS(pl);
    // by = OBJ_Y_IN_BLOCKS(pl);

    // bv.world.x = bx - (horizontal_blocks >> 1);
    // bv.world.y = by - (vertical_blocks >> 1);
    // bv.realWorld = bv.world;
    // if (BIT(world->rules->mode, WRAP_PLAY))
    // {
    //     if (bv.world.x < 0 && bv.world.x + horizontal_blocks < world->x)
    //         bv.world.x += world->x;
    //     else if (bv.world.x > 0 && bv.world.x + horizontal_blocks > world->x)
    //         bv.realWorld.x -= world->x;
    //     if (bv.world.y < 0 && bv.world.y + vertical_blocks < world->y)
    //         bv.world.y += world->y;
    //     else if (bv.world.y > 0 && bv.world.y + vertical_blocks > world->y)
    //         bv.realWorld.y -= world->y;
    // }

    packet_count = 0;
    max_packet = MAX(5, bytes_left / target_packet_size);
    i = MAX(0, pl->last_target_update);
    for (k = 0; k < world->NumTargets; k++)
    {
        target_t *targ;
        if (++i >= world->NumTargets)
            i = 0;
        targ = &world->targets[i];
        if (BIT(targ->update_mask, conn_bit) || (BIT(targ->conn_mask, conn_bit) == 0 &&
                                                 click_inview(cv, targ->clk_pos.cx, targ->clk_pos.cy)))
        {
            Send_target(conn, i, targ->dead_time, targ->damage);
            pl->last_target_update = i;
            bytes_left -= target_packet_size;
            if (++packet_count >= max_packet)
                break;
        }
    }

    packet_count = 0;
    max_packet = MAX(5, bytes_left / cannon_packet_size);
    i = MAX(0, pl->last_cannon_update);
    for (k = 0; k < world->NumCannons; k++)
    {
        if (++i >= world->NumCannons)
            i = 0;
        if (click_inview(cv,
                         world->cannon[i].clk_pos.cx,
                         world->cannon[i].clk_pos.cy))
        {
            if (BIT(world->cannon[i].conn_mask, conn_bit) == 0)
            {
                Send_cannon(conn, i, world->cannon[i].dead_time);
                pl->last_cannon_update = i;
                bytes_left -= max_packet * cannon_packet_size;
                if (++packet_count >= max_packet)
                    break;
            }
        }
    }

    packet_count = 0;
    max_packet = MAX(5, bytes_left / fuel_packet_size);
    i = MAX(0, pl->last_fuel_update);
    for (k = 0; k < world->NumFuels; k++)
    {
        if (++i >= world->NumFuels)
            i = 0;
        if (BIT(world->fuel[i].conn_mask, conn_bit) == 0)
        {
            if (world->block[world->fuel[i].blk_pos.x]
                            [world->fuel[i].blk_pos.y] == FUEL)
            {
                if (click_inview(cv,
                                 world->fuel[i].clk_pos.cx,
                                 world->fuel[i].clk_pos.cy))
                {
                    Send_fuel(conn, i, (int)world->fuel[i].fuel);
                    pl->last_fuel_update = i;
                    bytes_left -= max_packet * fuel_packet_size;
                    if (++packet_count >= max_packet)
                    {
                        break;
                    }
                }
            }
        }
    }

    packet_count = 0;
    max_packet = MAX(5, bytes_left / wormhole_packet_size);
    i = MAX(0, pl->last_wormhole_update);
    for (k = 0; k < world->NumWormholes; k++)
    {
        wormhole_t *worm;
        if (++i >= world->NumWormholes)
            i = 0;
        worm = &world->wormHoles[i];
        if (options.wormholeVisible &&
            worm->temporary &&
            (worm->type == WORM_IN || worm->type == WORM_NORMAL) &&
            click_inview(cv, worm->clk_pos.cx, worm->clk_pos.cy))
        {
            int cx = worm->clk_pos.cx;
            int cy = worm->clk_pos.cy;
            Send_wormhole(conn, CLICK_TO_PIXEL(cx), CLICK_TO_PIXEL(cy));
            pl->last_wormhole_update = i;
            bytes_left -= max_packet * wormhole_packet_size;
            if (++packet_count >= max_packet)
                break;
        }
    }
}

static void Frame_shuffle_objects(void)
{
    int i;
    size_t memsize;

    num_object_shuffle = MIN(NumObjs, options.maxVisibleObject);

    if (max_object_shuffle < num_object_shuffle)
    {
        XFREE(object_shuffle_ptr);
        max_object_shuffle = num_object_shuffle;
        memsize = max_object_shuffle * sizeof(shuffle_t);
        object_shuffle_ptr = (shuffle_t *)malloc(memsize);
        if (object_shuffle_ptr == NULL)
            max_object_shuffle = 0;
    }

    if (max_object_shuffle < num_object_shuffle)
        num_object_shuffle = max_object_shuffle;

    for (i = 0; i < num_object_shuffle; i++)
        object_shuffle_ptr[i] = i;

    /* permute. */
    for (i = num_object_shuffle - 1; i >= 0; --i)
    {
        if (object_shuffle_ptr[i] == i)
        {
            int j = (int)(rfrac() * i);
            shuffle_t tmp = object_shuffle_ptr[j];
            object_shuffle_ptr[j] = object_shuffle_ptr[i];
            object_shuffle_ptr[i] = tmp;
        }
    }
}

static void Frame_shuffle_players(void)
{
    int i;
    size_t memsize;

    num_player_shuffle = MIN(NumPlayers, MAX_SHUFFLE_INDEX);

    if (max_player_shuffle < num_player_shuffle)
    {
        XFREE(player_shuffle_ptr);
        max_player_shuffle = num_player_shuffle;
        memsize = max_player_shuffle * sizeof(shuffle_t);
        player_shuffle_ptr = (shuffle_t *)malloc(memsize);
        if (player_shuffle_ptr == NULL)
            max_player_shuffle = 0;
    }

    if (max_player_shuffle < num_player_shuffle)
        num_player_shuffle = max_player_shuffle;

    for (i = 0; i < num_player_shuffle; i++)
        player_shuffle_ptr[i] = i;

    /* permute. */
    for (i = 0; i < num_player_shuffle; i++)
    {
        int j = (int)(rfrac() * num_player_shuffle);
        shuffle_t tmp = player_shuffle_ptr[j];
        player_shuffle_ptr[j] = player_shuffle_ptr[i];
        player_shuffle_ptr[i] = tmp;
    }
}

static void Frame_shuffle(void)
{
    if (last_frame_shuffle != frame_loops)
    {
        last_frame_shuffle = frame_loops;
        Frame_shuffle_objects();
        Frame_shuffle_players();
    }
}

static void Frame_shots(connection_t *conn, int ind)
{
    player_t *pl = Players[ind];
    int x, y, cx, cy;
    int i, k, color;
    int fuzz = 0, teamshot, len;
    int obj_count;
    object_t *shot;
    object_t **obj_list;
    int hori_blocks, vert_blocks;

    hori_blocks = (view_width + (BLOCK_SZ - 1)) / (2 * BLOCK_SZ);
    vert_blocks = (view_height + (BLOCK_SZ - 1)) / (2 * BLOCK_SZ);
    Cell_get_objects(pl->pos.cx, pl->pos.cy,
                     MAX(hori_blocks, vert_blocks), num_object_shuffle,
                     &obj_list,
                     &obj_count);
    for (k = 0; k < num_object_shuffle; k++)
    {
        i = object_shuffle_ptr[k];
        if (i >= obj_count)
            continue;
        shot = obj_list[i];
        x = shot->pos.x;
        y = shot->pos.y;
        cx = shot->pos.cx;
        cy = shot->pos.cy;
        if (!click_inview(cv, cx, cy))
            continue;

        if ((color = shot->color) == BLACK)
        {
            xpprintf("black %d,%d\n", shot->type, shot->id);
            color = WHITE;
        }
        switch (shot->type)
        {
        case OBJ_SPARK:
        case OBJ_DEBRIS:
            if ((fuzz >>= 7) < 0x40)
                fuzz = randomMT();
            if ((fuzz & 0x7F) >= spark_rand)
                /*
                 * produce a sparkling effect by not displaying
                 * particles every frame.
                 */
                break;
            /*
             * The number of colors which the client
             * uses for displaying debris is bigger than 2
             * then the color used denotes the temperature
             * of the debris particles.
             * Higher color number means hotter debris.
             */
            if (debris_colors >= 3)
            {
                if (debris_colors > 4)
                {
                    if (color == BLUE)
                        color = (shot->life >> 1);
                    else
                        color = (shot->life >> 2);
                }
                else
                {
                    if (color == BLUE)
                        color = (shot->life >> 2);
                    else
                        color = (shot->life >> 3);
                }
                if (color >= debris_colors)
                    color = debris_colors - 1;
            }

            debris_store((int)(shot->pos.x - pv.world.x),
                         (int)(shot->pos.y - pv.world.y),
                         color);
            break;

        case OBJ_WRECKAGE:
            if (spark_rand != 0 || options.wreckageCollisionMayKill)
            {
                wireobject_t *wreck = WIRE_PTR(shot);
                Send_wreckage(conn, x, y, (uint8_t)wreck->info,
                              wreck->size, wreck->rotation);
            }
            break;

        case OBJ_ASTEROID:
        {
            wireobject_t *ast = WIRE_PTR(shot);
            Send_asteroid(conn, x, y,
                          (uint8_t)ast->info, ast->size, ast->rotation);
        }
        break;

        case OBJ_SHOT:
        case OBJ_CANNON_SHOT:
            if (Team_immune(shot->id, pl->id) || (shot->id != NO_ID && BIT(Players[GetInd[shot->id]]->status, PAUSE)) || (shot->id == NO_ID && BIT(world->rules->mode, TEAM_PLAY) && shot->team == pl->team))
            {
                color = BLUE;
                teamshot = DEBRIS_TYPES;
            }
            else if (shot->id == pl->id && options.selfImmunity)
            {
                color = BLUE;
                teamshot = DEBRIS_TYPES;
            }
            else if (shot->mods.nuclear && (frame_loops & 2))
            {
                color = RED;
                teamshot = DEBRIS_TYPES;
            }
            else
                teamshot = 0;

            fastshot_store((int)(shot->pos.x - pv.world.x),
                           (int)(shot->pos.y - pv.world.y),
                           color, teamshot);
            break;

        case OBJ_TORPEDO:
            len = options.distinguishMissiles ? TORPEDO_LEN : MISSILE_LEN;
            Send_missile(conn, x, y, len, shot->missile_dir);
            break;
        case OBJ_SMART_SHOT:
            len = options.distinguishMissiles ? SMART_SHOT_LEN : MISSILE_LEN;
            Send_missile(conn, x, y, len, shot->missile_dir);
            break;
        case OBJ_HEAT_SHOT:
            len = options.distinguishMissiles ? HEAT_SHOT_LEN : MISSILE_LEN;
            Send_missile(conn, x, y, len, shot->missile_dir);
            break;
        case OBJ_BALL:
            Send_ball(conn, x, y, shot->id);
            break;
        case OBJ_MINE:
        {
            int id = 0;
            int laid_by_team = 0;
            int confused = 0;
            mineobject_t *mine = MINE_PTR(shot);

            /* calculate whether ownership of mine can be determined */
            if (options.identifyMines && (Wrap_length(pl->pos.cx - mine->pos.cx,
                                                      pl->pos.cy - mine->pos.cy) /
                                              CLICK <
                                          (SHIP_SZ + MINE_SENSE_BASE_RANGE + pl->item[ITEM_SENSOR] * MINE_SENSE_RANGE_FACTOR)))
            {
                id = mine->id;
                if (id == NO_ID)
                    id = EXPIRED_MINE_ID;
                if (BIT(mine->status, CONFUSED))
                    confused = 1;
            }
            if (mine->id != NO_ID && BIT(Players[GetInd[mine->id]]->status, PAUSE))
                laid_by_team = 1;
            else
            {
                laid_by_team = (Team_immune(mine->id, pl->id) || (BIT(mine->status, OWNERIMMUNE) && mine->owner == pl->id));
                if (confused)
                {
                    id = 0;
                    laid_by_team = (rfrac() < 0.5f);
                }
            }
            Send_mine(conn, x, y, laid_by_team, id);
        }
        break;

        case OBJ_ITEM:
        {
            int item_type = shot->info;

            if (BIT(shot->status, RANDOM_ITEM))
                item_type = Choose_random_item();

            Send_item(conn, x, y, item_type);
        }
        break;

        default:
            xperror("Frame_shots: Shot type %d not defined.", shot->type);
            break;
        }
    }
}

static void Frame_ships(connection_t *conn, int ind)
{
    player_t *pl = Players[ind],
             *pl_i;
    pulse_t *pulse;
    int i, j, k, color, dir;
    int cx, cy;

    for (j = 0; j < NumPulses; j++)
    {
        pulse = Pulses[j];
        if (pulse->len <= 0)
            continue;
        cx = FLOAT_TO_CLICK(pulse->pos.x);
        cy = FLOAT_TO_CLICK(pulse->pos.y);
        if (BIT(world->rules->mode, WRAP_PLAY))
        {
            if (cx < 0)
                cx += world->click_width;
            else if (cx >= world->click_width)
                cx -= world->click_width;
            if (cy < 0)
                cy += world->click_height;
            else if (cy >= world->click_height)
                cy -= world->click_height;
        }

        double x = CLICK_TO_FLOAT(cx);
        double y = CLICK_TO_FLOAT(cy);

        if (click_inview(cv, cx, cy))
            dir = pulse->dir;
        else
        {
            x += tcos(pulse->dir) * pulse->len;
            y += tsin(pulse->dir) * pulse->len;
            if (BIT(world->rules->mode, WRAP_PLAY))
            {
                if (x < 0)
                    x += world->width;
                else if (x >= world->width)
                    x -= world->width;
                if (y < 0)
                    y += world->height;
                else if (y >= world->height)
                    y -= world->height;
            }
            cx = FLOAT_TO_CLICK(x);
            cy = FLOAT_TO_CLICK(y);
            if (click_inview(cv, cx, cy))
                dir = MOD2(pulse->dir + RES / 2, RES);
            else
                continue;
        }
        if (Team_immune(pulse->id, pl->id))
            color = BLUE;
        else if (pulse->id == pl->id && options.selfImmunity)
            color = BLUE;
        else
            color = RED;
        Send_laser(conn, color, (int)x, (int)y, pulse->len, dir);
    }
    for (i = 0; i < NumEcms; i++)
    {
        ecm_t *ecm = Ecms[i];
        Send_ecm(conn, CLICK_TO_PIXEL(ecm->clk_pos.cx), CLICK_TO_PIXEL(ecm->clk_pos.cy), ecm->size);
    }
    for (i = 0; i < NumTransporters; i++)
    {
        trans_t *trans = Transporters[i];
        player_t *victim = Players[GetInd[trans->target]],
                 *pl = (trans->id == NO_ID ? NULL : Players[GetInd[trans->id]]);
        int cx = (pl ? pl->pos.cx : trans->clk_pos.cx);
        int cy = (pl ? pl->pos.cy : trans->clk_pos.cy);
        Send_trans(conn, victim->pos.x, victim->pos.y, CLICK_TO_PIXEL(cx), CLICK_TO_PIXEL(cy));
    }
    for (i = 0; i < world->NumCannons; i++)
    {
        cannon_t *cannon = world->cannon + i;
        if (cannon->tractor_count > 0)
        {
            player_t *t = Players[GetInd[cannon->tractor_target]];
            if (click_inview(cv, t->pos.cx, t->pos.cy))
            {
                int j;
                for (j = 0; j < 3; j++)
                {
                    Send_connector(conn,
                                   (int)(t->pos.x + t->ship->pts[j][t->dir].x),
                                   (int)(t->pos.y + t->ship->pts[j][t->dir].y),
                                   CLICK_TO_PIXEL(cannon->clk_pos.cx),
                                   CLICK_TO_PIXEL(cannon->clk_pos.cy), 1);
                }
            }
        }
    }

    for (k = 0; k < num_player_shuffle; k++)
    {
        i = player_shuffle_ptr[k];
        pl_i = Players[i];
        if (!BIT(pl_i->status, PLAYING | PAUSE))
            continue;
        if (BIT(pl_i->status, GAME_OVER))
            continue;
        if (!click_inview(cv, pl_i->pos.cx, pl_i->pos.cy))
            continue;
        if (BIT(pl_i->status, PAUSE))
        {
            Send_paused(conn,
                        pl_i->pos.x,
                        pl_i->pos.y,
                        pl_i->count);
            continue;
        }

        /* Don't transmit information if fighter is invisible */
        if (pl->visibility[i].canSee || i == ind || Players_are_teammates(pl_i, pl) || Players_are_allies(pl_i, pl))
        {
            /*
             * Transmit ship information
             */
            Send_ship(conn,
                      pl_i->pos.x,
                      pl_i->pos.y,
                      pl_i->id,
                      pl_i->dir,
                      BIT(pl_i->used, HAS_SHIELD) != 0,
                      BIT(pl_i->used, HAS_CLOAKING_DEVICE) != 0,
                      BIT(pl_i->used, HAS_EMERGENCY_SHIELD) != 0,
                      BIT(pl_i->used, HAS_PHASING_DEVICE) != 0,
                      BIT(pl_i->used, HAS_DEFLECTOR) != 0);
        }
        if (BIT(pl_i->used, HAS_REFUEL))
        {
            if (click_inview(cv, world->fuel[pl_i->fs].clk_pos.cx,
                             world->fuel[pl_i->fs].clk_pos.cy))
                Send_refuel(conn,
                            (int)world->fuel[pl_i->fs].pix_pos.x,
                            (int)world->fuel[pl_i->fs].pix_pos.y,
                            pl_i->pos.x,
                            pl_i->pos.y);
        }
        if (BIT(pl_i->used, HAS_REPAIR))
        {
            double x = (double)(world->targets[pl_i->repair_target].blk_pos.x + 0.5) * BLOCK_SZ;
            double y = (double)(world->targets[pl_i->repair_target].blk_pos.y + 0.5) * BLOCK_SZ;
            cx = FLOAT_TO_CLICK(x);
            cy = FLOAT_TO_CLICK(y);
            if (click_inview(cv, cx, cy))
                /* same packet as refuel */
                Send_refuel(conn, pl_i->pos.x, pl_i->pos.y, (int)x, (int)y);
        }
        if (BIT(pl_i->used, HAS_TRACTOR_BEAM))
        {
            player_t *t = Players[GetInd[pl_i->lock.pl_id]];
            if (click_inview(cv, t->pos.cx, t->pos.cy))
            {
                int j;

                for (j = 0; j < 3; j++)
                    Send_connector(conn,
                                   (int)(t->pos.x + t->ship->pts[j][t->dir].x),
                                   (int)(t->pos.y + t->ship->pts[j][t->dir].y),
                                   pl_i->pos.x,
                                   pl_i->pos.y, 1);
            }
        }

        if (pl_i->ball != NULL && click_inview(cv, pl_i->ball->pos.cx, pl_i->ball->pos.cy))
            Send_connector(conn,
                           pl_i->ball->pos.x,
                           pl_i->ball->pos.y,
                           pl_i->pos.x,
                           pl_i->pos.y, 0);
    }
}

static void Frame_radar(connection_t *conn, int ind)
{
    int i, k, mask, shownuke, size;
    player_t *pl = Players[ind];
    object_t *shot;
    int cx, cy;

    Frame_radar_buffer_reset();

#ifndef NO_SMART_MIS_RADAR
    if (options.nukesOnRadar)
        mask = OBJ_SMART_SHOT | OBJ_TORPEDO | OBJ_HEAT_SHOT | OBJ_MINE;
    else
    {
        mask = (options.missilesOnRadar ? (OBJ_SMART_SHOT | OBJ_TORPEDO | OBJ_HEAT_SHOT) : 0);
        mask |= (options.minesOnRadar) ? OBJ_MINE : 0;
    }
    if (options.treasuresOnRadar)
        mask |= OBJ_BALL;
    if (options.asteroidsOnRadar)
        mask |= OBJ_ASTEROID;

    if (mask)
    {
        for (i = 0; i < NumObjs; i++)
        {
            shot = Obj[i];
            if (!BIT(shot->type, mask))
                continue;

            shownuke = (options.nukesOnRadar && (shot)->mods.nuclear);
            if (shownuke && (frame_loops & 2))
                size = 3;
            else
                size = 0;

            if (BIT(shot->type, OBJ_MINE))
            {
                if (!options.minesOnRadar && !shownuke)
                    continue;
                if (frame_loops % 8 >= 6)
                    continue;
            }
            else if (BIT(shot->type, OBJ_BALL))
                size = 2;
            else if (BIT(shot->type, OBJ_ASTEROID))
            {
                size = WIRE_PTR(shot)->size + 1;
                size |= 0x80;
            }
            else
            {
                if (!options.missilesOnRadar && !shownuke)
                    continue;
                if (frame_loops & 1)
                    continue;
            }

            if (Wrap_length(pl->pos.cx - shot->pos.cx,
                            pl->pos.cy - shot->pos.cy) /
                    CLICK <=
                pl->sensor_range)
                Frame_radar_buffer_add(shot->pos.cx, shot->pos.cy, size);
        }
    }
#endif

    if (options.playersOnRadar ||
        BIT(world->rules->mode, TEAM_PLAY) ||
        NumPseudoPlayers > 0 ||
        NumAlliances > 0)
    {
        for (k = 0; k < num_player_shuffle; k++)
        {
            i = player_shuffle_ptr[k];
            /*
             * Don't show on the radar:
             *                Ourselves (not necessarily same as who we watch).
             *                People who are not playing.
             *                People in other teams or alliances if;
             *                        no playersOnRadar or if not visible
             */
            if (Players[i]->conn == conn ||
                BIT(Players[i]->status, PLAYING | PAUSE | GAME_OVER) != PLAYING ||
                (!Players_are_teammates(pl, Players[i]) && !Players_are_allies(pl, Players[i]) && !Player_owns_tank(pl, Players[i]) && (!options.playersOnRadar || !pl->visibility[i].canSee)))
                continue;
            if (BIT(world->rules->mode, LIMITED_VISIBILITY) && Wrap_length(pl->pos.cx - Players[i]->pos.cx,
                                                                           pl->pos.cy - Players[i]->pos.cy) /
                                                                       CLICK >
                                                                   pl->sensor_range)
                continue;
            if (BIT(pl->used, HAS_COMPASS) && BIT(pl->lock.tagged, LOCK_PLAYER) && GetInd[pl->lock.pl_id] == i && frame_loops % 5 >= 3)
                continue;
            size = 3;
            if (Players_are_teammates(pl, Players[i]) || Players_are_allies(pl, Players[i]) || Player_owns_tank(pl, Players[i]))
                size |= 0x80;
            Frame_radar_buffer_add(Players[i]->pos.cx, Players[i]->pos.cy, size);
        }
    }

    Frame_radar_buffer_send(conn);
}

static void Frame_lose_item_state(int ind)
{
    player_t *pl = Players[ind];

    if (pl->lose_item_state != 0)
    {
        Send_loseitem(pl->lose_item, pl->conn);
        if (pl->lose_item_state == 1)
            pl->lose_item_state = -5;
        if (pl->lose_item_state < 0)
            pl->lose_item_state++;
    }
}

static void Frame_parameters(connection_t *conn, player_t *pl)
{
    Get_display_parameters(conn, &view_width, &view_height,
                           &debris_colors, &spark_rand);
    debris_x_areas = (view_width + 255) >> 8;
    debris_y_areas = (view_height + 255) >> 8;
    debris_areas = debris_x_areas * debris_y_areas;
    horizontal_blocks = (view_width + (BLOCK_SZ - 1)) / BLOCK_SZ;
    vertical_blocks = (view_height + (BLOCK_SZ - 1)) / BLOCK_SZ;

    pv.world.x = pl->pos.x - view_width / 2; /* Scroll */
    pv.world.y = pl->pos.y - view_height / 2;
    pv.realWorld = pv.world;
    if (BIT(world->rules->mode, WRAP_PLAY))
    {
        if (pv.world.x < 0 && pv.world.x + view_width < world->width)
            pv.world.x += world->width;
        else if (pv.world.x > 0 && pv.world.x + view_width >= world->width)
            pv.realWorld.x -= world->width;
        if (pv.world.y < 0 && pv.world.y + view_height < world->height)
            pv.world.y += world->height;
        else if (pv.world.y > 0 && pv.world.y + view_height >= world->height)
            pv.realWorld.y -= world->height;
    }

    view_click_width = PIXEL_TO_CLICK(view_width);
    view_click_height = PIXEL_TO_CLICK(view_height);

    cv.world.cx = pl->pos.cx - view_click_width / 2; /* Scroll */
    cv.world.cy = pl->pos.cy - view_click_height / 2;
    cv.realWorld = cv.world;
    if (BIT(world->rules->mode, WRAP_PLAY))
    {
        if (cv.world.cx < 0 && cv.world.cx + view_click_width < world->click_width)
            cv.world.cx += world->click_width;
        else if (cv.world.cx > 0 && cv.world.cx + view_click_width >= world->click_width)
            cv.realWorld.cx -= world->click_width;
        if (cv.world.cy < 0 && cv.world.cy + view_click_height < world->click_height)
            cv.world.cy += world->click_height;
        else if (cv.world.cy > 0 && cv.world.cy + view_click_height >= world->click_height)
            cv.realWorld.cy -= world->click_height;
    }
}

void Frame_update(void)
{
    int i,
        ind;
    connection_t *conn;
    player_t *pl, *pl2;
    time_t newTimeLeft = 0;
    static time_t oldTimeLeft;
    static bool game_over_called = false;

    if (++frame_loops >= LONG_MAX) /* Used for misc. timing purposes */
        frame_loops = 1;

    Frame_shuffle();

    if (options.gameDuration > 0.0 && game_over_called == false && oldTimeLeft != (newTimeLeft = gameOverTime - time(NULL)))
    {
        /*
         * Do this once a second.
         */
        if (newTimeLeft <= 0)
        {
            Game_Over();
            ShutdownServer = 30 * FPS; /* Shutdown in 30 seconds */
            game_over_called = true;
        }
    }

    for (i = 0; i < num_player_shuffle; i++)
    {
        pl = Players[i];
        conn = pl->conn;
        if (conn == NULL)
            continue;
        if (BIT(pl->status, PAUSE | GAME_OVER) && !options.allowViewing && !pl->isowner)
        {
            /*
             * Lower the frame rate for non-playing players
             * to reduce network load.
             * Owner always gets full framerate even if paused.
             * With allowViewing on, everyone gets full framerate.
             */
            if (BIT(pl->status, PAUSE))
            {
                if (frame_loops & 0x03)
                    continue;
            }
            else
            {
                if (frame_loops & 0x01)
                    continue;
            }
        }

        /*
         * Reduce frame rate to player's own rate.
         */
        // if (... TODO...)
        // {
        //     continue;
        // }

        if (Send_start_of_frame(conn) == -1)
            continue;
        if (newTimeLeft != oldTimeLeft)
            Send_time_left(conn, newTimeLeft);
        else if (options.maxRoundTime > 0 && roundtime >= 0)
            Send_time_left(conn, (roundtime + FPS - 1) / FPS);
        /*
         * If status is GAME_OVER or PAUSE'd, the user may look through the
         * other players 'eyes'.  If PAUSE'd this only works on team members.
         * We can't use Players_are_teammates() macro as PAUSE'd players are always on
         * equivalent teams.
         *
         * This is done by using two indexes, one
         * determining which data should be used (ind, set below) and
         * one determining which connection to send it to (conn).
         */
        if (BIT(pl->lock.tagged, LOCK_PLAYER))
        {
            if ((BIT(pl->status, (GAME_OVER | PLAYING)) == (GAME_OVER | PLAYING)) ||
                (BIT(pl->status, PAUSE) &&
                 ((BIT(world->rules->mode, TEAM_PLAY) && pl->team != TEAM_NOT_SET && pl->team == Players[GetInd[pl->lock.pl_id]]->team) ||
                  pl->isowner ||
                  options.allowViewing)))
                ind = GetInd[pl->lock.pl_id];
            else
                ind = i;
        }
        else
            ind = i;
        pl2 = Players[ind];
        if (pl2->damaged > 0)
            Send_damaged(conn, pl2->damaged);
        else
        {
            Frame_parameters(conn, pl2);
            if (Frame_status(conn, ind) <= 0)
                continue;
            Frame_map(conn, pl2);
            Frame_ships(conn, ind);
            Frame_shots(conn, ind);
            Frame_radar(conn, ind);
            Frame_lose_item_state(i);
            debris_end(conn);
            fastshot_end(conn);
        }
        sound_play_queued(Players[ind]);
        Send_end_of_frame(conn);
    }
    oldTimeLeft = newTimeLeft;

    Frame_radar_buffer_free();
}

void Set_message(const char *message)
{
    player_t *pl;
    int i;
    const char *msg;
    char tmp[MSG_LEN];

    if ((i = strlen(message)) >= MSG_LEN)
    {
#ifndef SILENT
        errno = 0;
        xperror("Max message len exceed (%d,%s)", i, message);
#endif
        strlcpy(tmp, message, MSG_LEN);
        msg = tmp;
    }
    else
        msg = message;
    for (i = 0; i < NumPlayers; i++)
    {
        pl = Players[i];
        if (pl->conn != NULL)
            Send_message(pl->conn, msg);
    }
}

void Set_player_message(player_t *pl, const char *message)
{
    int i;
    const char *msg;
    char tmp[MSG_LEN];

    if ((i = strlen(message)) >= MSG_LEN)
    {
#ifndef SILENT
        errno = 0;
        xperror("Max message len exceed (%d,%s)", i, message);
#endif
        memcpy(tmp, message, MSG_LEN - 1);
        tmp[MSG_LEN - 1] = '\0';
        msg = tmp;
    }
    else
        msg = message;
    if (pl->conn != NULL)
        Send_message(pl->conn, msg);
    else if (Player_is_robot(pl))
        Robot_message(GetInd[pl->id], msg);
}
