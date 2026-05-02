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
#include "const.h"
#include "strlcpy.h"

#include "modifiers.h"
#include "server.h"

#define SERVER
#include "map.h"
#include "xpconfig.h"
#include "serverconst.h"
#include "bit.h"
#include "netserver.h"
#include "saudio.h"
#include "xperror.h"
#include "xpmath.h"
#include "player.h"
#include "robot.h"

#define MAX_SHUFFLE_INDEX 65535

typedef uint16_t shuffle_t;

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
long frame_loops_slow = 1;
double frame_time = 0;
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
    view_cwidth,
    view_cheight,
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

static inline bool clpos_inview(click_visibility_t *cv, clpos_t pos)
{
    return (((pos.cx > cv->world.cx && pos.cx < cv->world.cx + view_cwidth) ||
             (pos.cx > cv->realWorld.cx && pos.cx < cv->realWorld.cx + view_cwidth)) &&
            ((pos.cy > cv->world.cy && pos.cy < cv->world.cy + view_cheight) ||
             (pos.cy > cv->realWorld.cy && pos.cy < cv->realWorld.cy + view_cheight)));
}

static inline bool click_inview(click_visibility_t &cv, int cx, int cy)
{
    clpos_t pos = {cx, cy};
    return clpos_inview(&cv, pos);
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
            error("No memory for debris");
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
            error("No memory for debris");
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

static void Frame_radar_buffer_add(clpos_t pos, int s)
{
    radar_t *p;

    EXPAND(radar_ptr, num_radar, max_radar, radar_t, 1);
    p = &radar_ptr[num_radar++];
    p->x = CLICK_TO_PIXEL(pos.cx);
    p->y = CLICK_TO_PIXEL(pos.cy);
    p->size = s;
}

static void Frame_radar_buffer_send(connection_t *conn, player_t *pl)
{
    int i, dest, tmp;
    radar_t *p;
    const int radar_width = 256;
    int radar_height, radar_x, radar_y, send_x, send_y;
    shuffle_t *radar_shuffle;
    size_t shuffle_bufsize;

    radar_height = (radar_width * world->y) / world->x;

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

    // if (Get_conn_version(conn) <= 0x4400)
    // if (!FEATURE(conn, F_FASTRADAR))
    if (false)
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
        unsigned fast_count = 0;

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

static int Frame_status(connection_t *conn, player_t *pl)
{
    static char modsstr[MAX_CHARS];
    int n, lock_ind, lock_id = NO_ID, lock_dist = 0, lock_dir = 0;
    int showautopilot;

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
        lock_ind = GetInd(lock_id);
        player_t *lock_pl = Player_by_id(pl->lock.pl_id);

        if ((!BIT(world->rules->mode, LIMITED_VISIBILITY) || pl->lock.distance <= pl->sensor_range)
#ifndef SHOW_CLOAKERS_RANGE
            && (pl->visibility[lock_ind].canSee ||
                Player_owns_tank(pl, lock_pl) ||
                Players_are_teammates(pl, lock_pl) ||
                Players_are_allies(pl, lock_pl))
#endif
            && BIT(lock_pl->obj_status, PLAYING | GAME_OVER) == PLAYING &&
            (options.playersOnRadar || click_inview(cv, lock_pl->pos.cx, lock_pl->pos.cy)) &&
            pl->lock.distance != 0)
        {
            SET_BIT(pl->lock.tagged, LOCK_VISIBLE);
            lock_dir = (int)Wrap_findDir((int)(lock_pl->pix_pos.x - pl->pix_pos.x),
                                         (int)(lock_pl->pix_pos.y - pl->pix_pos.y));
            lock_dist = (int)pl->lock.distance;
        }
    }

    if (Player_is_hoverpaused(pl))
        showautopilot = (pl->count <= 0 || (frame_loops_slow % 8) < 4);
    else if (Player_uses_autopilot(pl))
        showautopilot = (frame_loops_slow % 8) < 4;
    else
        showautopilot = 0;

    /*
     * Don't forget to modify Receive_modifier_bank() in netserver.c
     */
    Mods_to_string(pl->mods, modsstr, sizeof(modsstr));
    n = Send_self(conn,
                  pl,
                  lock_id,
                  lock_dist,
                  lock_dir,
                  showautopilot,
                  Player_by_id(Get_player_id(conn))->obj_status,
                  modsstr);
    if (n <= 0)
        return 0;

    if (Player_uses_emergency_thrust(pl))
        Send_thrusttime(conn,
                        pl->emergency_thrust_left,
                        pl->emergency_thrust_max);
    if (BIT(pl->used, USES_EMERGENCY_SHIELD))
        Send_shieldtime(conn,
                        pl->emergency_shield_left,
                        pl->emergency_shield_max);
    if (BIT(pl->obj_status, SELF_DESTRUCT) && pl->count > 0)
        Send_destruct(conn, pl->count);
    if (Player_is_phasing(pl))
        Send_phasingtime(conn,
                         pl->phasing_left,
                         pl->phasing_max);
    if (ShutdownServer != -1)
        Send_shutdown(conn, ShutdownServer, ShutdownDelay);

    return 1;
}

static void Frame_map(connection_t *conn, player_t *pl)
{
    int i, k;
    // int bx, by;
    int conn_bit = (1 << conn->ind);
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
    for (k = 0; k < Num_targets(); k++)
    {
        target_t *targ;

        if (++i >= Num_targets())
            i = 0;
        targ = Target_by_index(i);
        if (BIT(targ->update_mask, conn_bit) || (BIT(targ->conn_mask, conn_bit) == 0 && clpos_inview(&cv, targ->pos)))
        {
            Send_target(conn, i, (int)targ->dead_ticks, targ->damage);
            pl->last_target_update = i;
            bytes_left -= target_packet_size;
            if (++packet_count >= max_packet)
                break;
        }
    }

    packet_count = 0;
    max_packet = MAX(5, bytes_left / cannon_packet_size);
    i = MAX(0, pl->last_cannon_update);
    for (k = 0; k < Num_cannons(); k++)
    {
        cannon_t *cannon;

        if (++i >= Num_cannons())
            i = 0;
        cannon = Cannon_by_index(i);
        if (clpos_inview(&cv, cannon->pos))
        {
            if (BIT(cannon->conn_mask, conn_bit) == 0)
            {
                Send_cannon(conn, i, (int)cannon->dead_ticks);
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
    for (k = 0; k < Num_fuels(); k++)
    {
        fuel_t *fs;

        if (++i >= Num_fuels())
            i = 0;

        fs = Fuel_by_index(i);
        if (BIT(fs->conn_mask, conn_bit) == 0)
        {
            if (world->block[fs->blk_pos.bx]
                            [fs->blk_pos.by] == FUEL)
            {
                if (click_inview(cv,
                                 fs->pos.cx,
                                 fs->pos.cy))
                {
                    Send_fuel(conn, i, (int)fs->fuel * (1.0 / FUEL_SCALE_FACT));
                    pl->last_fuel_update = i;
                    bytes_left -= max_packet * fuel_packet_size;
                    if (++packet_count >= max_packet)
                        break;
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
        worm = &world->wormholes[i];
        if (options.wormholeVisible &&
            worm->temporary &&
            (worm->type == WORM_IN || worm->type == WORM_NORMAL) &&
            click_inview(cv, worm->pos.cx, worm->pos.cy))
        {
            Send_wormhole(conn, worm->pos);
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

    num_object_shuffle = MIN(NumObjs, options.maxVisibleObject);

    if (max_object_shuffle < num_object_shuffle)
    {
        XFREE(object_shuffle_ptr);
        max_object_shuffle = num_object_shuffle;
        object_shuffle_ptr = XMALLOC(shuffle_t, max_object_shuffle);
        if (object_shuffle_ptr == NULL)
            max_object_shuffle = 0;
    }

    if (max_object_shuffle < num_object_shuffle)
        num_object_shuffle = max_object_shuffle;

    for (i = 0; i < num_object_shuffle; i++)
        object_shuffle_ptr[i] = i;

    /* permute. Not perfect distribution but probably doesn't matter here */
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

    num_player_shuffle = MIN(NumPlayers, MAX_SHUFFLE_INDEX);

    if (max_player_shuffle < num_player_shuffle)
    {
        XFREE(player_shuffle_ptr);
        max_player_shuffle = num_player_shuffle;
        player_shuffle_ptr = XMALLOC(shuffle_t, max_player_shuffle);
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

static void Frame_shots(connection_t *conn, player_t *pl)
{
    int x, y, cx, cy;
    int i, k, color;
    int fuzz = 0, teamshot, len;
    int obj_count;
    object_t *shot;
    object_t **obj_list;
    clpos_t pos;
    int hori_blocks, vert_blocks;

    hori_blocks = (view_width + (BLOCK_SZ - 1)) / (2 * BLOCK_SZ);
    vert_blocks = (view_height + (BLOCK_SZ - 1)) / (2 * BLOCK_SZ);
    Cell_get_objects(pl->pos,
                     MAX(hori_blocks, vert_blocks), num_object_shuffle,
                     &obj_list,
                     &obj_count);
    for (k = 0; k < num_object_shuffle; k++)
    {
        i = object_shuffle_ptr[k];
        if (i >= obj_count)
            continue;
        shot = obj_list[i];
        x = shot->pix_pos.x;
        y = shot->pix_pos.y;
        cx = shot->pos.cx;
        cy = shot->pos.cy;
        pos = shot->pos;

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
            {
                /*
                 * produce a sparkling effect by not displaying
                 * particles every frame.
                 */
                break;
            }
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
                        color = (int)shot->life / 2;
                    else
                        color = (int)shot->life / 4;
                }
                else
                {
                    if (color == BLUE)
                        color = (int)shot->life / 4;
                    else
                        color = (int)shot->life / 8;
                }
                if (color >= debris_colors)
                    color = debris_colors - 1;
            }

            debris_store((int)(shot->pix_pos.x - pv.world.x),
                         (int)(shot->pix_pos.y - pv.world.y),
                         color);
            break;

        case OBJ_WRECKAGE:
            if (spark_rand != 0 || options.wreckageCollisionMayKill)
            {
                wireobject_t *wreck = WIRE_PTR(shot);
                Send_wreckage(conn, pos, wreck->wire_type,
                              wreck->wire_size, wreck->wire_rotation);
            }
            break;

        case OBJ_ASTEROID:
        {
            wireobject_t *ast = WIRE_PTR(shot);
            Send_asteroid(conn, pos, ast->wire_type,
                          ast->wire_size, ast->wire_rotation);
        }
        break;

        case OBJ_SHOT:
        case OBJ_CANNON_SHOT:
            if (Team_immune(shot->id, pl->id) || (shot->id != NO_ID && Player_is_paused(Player_by_id(shot->id))) || (shot->id == NO_ID && BIT(world->rules->mode, TEAM_PLAY) && shot->team == pl->team))
            {
                color = BLUE;
                teamshot = DEBRIS_TYPES;
            }
            else if (shot->id == pl->id && options.selfImmunity)
            {
                color = BLUE;
                teamshot = DEBRIS_TYPES;
            }
            else if (Mods_get(shot->mods, ModsNuclear) && (frame_loops_slow & 2))
            {
                color = RED;
                teamshot = DEBRIS_TYPES;
            }
            else
                teamshot = 0;

            fastshot_store((int)(shot->pix_pos.x - pv.world.x),
                           (int)(shot->pix_pos.y - pv.world.y),
                           color, teamshot);
            break;

        case OBJ_TORPEDO:
            len = options.distinguishMissiles ? TORPEDO_LEN : MISSILE_LEN;
            Send_missile(conn, pos, len, MISSILE_PTR(shot)->missile_dir);
            break;
        case OBJ_SMART_SHOT:
            len = options.distinguishMissiles ? SMART_SHOT_LEN : MISSILE_LEN;
            Send_missile(conn, pos, len, MISSILE_PTR(shot)->missile_dir);
            break;
        case OBJ_HEAT_SHOT:
            len = options.distinguishMissiles ? HEAT_SHOT_LEN : MISSILE_LEN;
            Send_missile(conn, pos, len, MISSILE_PTR(shot)->missile_dir);
            break;
        case OBJ_BALL:
        {
            ballobject_t *ball = BALL_PTR(shot);

            Send_ball(conn, pos, ball->id,
                      0xff);
            //   options.ballStyles ? ball->ball_style : 0xff);

            break;
        }
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
                if (BIT(mine->obj_status, CONFUSED))
                    confused = 1;
            }
            if (mine->id != NO_ID && Player_is_paused(Player_by_id(mine->id)))
            {
                laid_by_team = 1;
            }
            else
            {
                laid_by_team = (Team_immune(mine->id, pl->id) || (BIT(mine->obj_status, OWNERIMMUNE) && mine->mine_owner == pl->id));
                if (confused)
                {
                    id = 0;
                    laid_by_team = (rfrac() < 0.5);
                }
            }
            Send_mine(conn, pos, laid_by_team, id);
        }
        break;

        case OBJ_ITEM:
        {
            itemobject_t *item = ITEM_PTR(shot);

            if (item->info != item->item_type)
            {
                warn("Frame_shots: shot->info != item->item_type, shot->info = %ld, item->item_type = %d",
                     item->count, item->item_type);
            }

            int item_type = item->item_type;

            if (BIT(item->obj_status, RANDOM_ITEM))
                item_type = Choose_random_item();

            Send_item(conn, pos, item_type);
        }
        break;
        }
    }
}

static void Frame_ships(connection_t *conn, player_t *pl)
{
    pulse_t *pulse;
    int i, j, k, color, dir;
    int cx, cy;

    for (j = 0; j < NumPulses; j++)
    {
        pulse = Pulses[j];
        if (pulse->len <= 0)
            continue;
        cx = FLOAT_TO_CLICK(pulse->pix_pos.x);
        cy = FLOAT_TO_CLICK(pulse->pix_pos.y);
        if (BIT(world->rules->mode, WRAP_PLAY))
        {
            if (cx < 0)
                cx += world->cwidth;
            else if (cx >= world->cwidth)
                cx -= world->cwidth;
            if (cy < 0)
                cy += world->cheight;
            else if (cy >= world->cheight)
                cy -= world->cheight;
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

        clpos_t pos;
        pos.cx = cx;
        pos.cy = cy;
        Send_laser(conn, color, pos, pulse->len, dir);
    }
    for (i = 0; i < Num_ecms(); i++)
    {
        ecm_t *ecm = Ecm_by_index(i);

        if (clpos_inview(&cv, ecm->pos))
            Send_ecm(conn, ecm->pos, (int)ecm->size);
    }

    for (i = 0; i < Num_transporters(); i++)
    {
        transporter_t *trans = Transporter_by_index(i);
        player_t *victim = Player_by_id(trans->victim_id);
        player_t *tpl = Player_by_id(trans->id);

        clpos_t pos = (tpl ? tpl->pos : trans->pos);
        Send_trans(conn, victim->pos, pos);
    }
    for (i = 0; i < Num_cannons(); i++)
    {
        cannon_t *cannon = Cannon_by_index(i);

        if (cannon->tractor_count > 0)
        {
            player_t *t = Player_by_id(cannon->tractor_target_id);
            if (clpos_inview(&cv, t->pos))
            {
                int j;
                for (j = 0; j < 3; j++)
                {
                    clpos_t pts, pos;

                    pts = Ship_get_point_clpos(t->ship, j, t->dir);
                    pos.cx = t->pos.cx + pts.cx;
                    pos.cy = t->pos.cy + pts.cy;
                    Send_connector(conn, pos, cannon->pos, 1);
                }
            }
        }
    }

    for (k = 0; k < num_player_shuffle; k++)
    {
        player_t *pl_i;

        i = player_shuffle_ptr[k];
        pl_i = Player_by_index(i);
        if (!BIT(pl_i->obj_status, PLAYING | PAUSE))
            continue;
        if (BIT(pl_i->obj_status, GAME_OVER))
            continue;
        if (!click_inview(cv, pl_i->pos.cx, pl_i->pos.cy))
            continue;
        if (Player_is_paused(pl_i))
        {
            Send_paused(conn, pl_i->pos, pl_i->count);
            continue;
        }

        /* Don't transmit information if fighter is invisible */
        if (pl->visibility[i].canSee || pl_i->id == pl->id || Players_are_teammates(pl_i, pl) || Players_are_allies(pl_i, pl))
        {
            /*
             * Transmit ship information
             */
            Send_ship(conn,
                      pl_i->pos,
                      pl_i->id,
                      pl_i->dir,
                      BIT(pl_i->used, HAS_SHIELD) != 0,
                      Player_is_cloaked(pl_i) ? 1 : 0,
                      BIT(pl_i->used, HAS_EMERGENCY_SHIELD) != 0,
                      Player_is_phasing(pl_i) ? 1 : 0,
                      BIT(pl_i->used, USES_DEFLECTOR) != 0);
        }
        if (Player_is_refueling(pl_i))
        {
            fuel_t *fs = Fuel_by_index(pl_i->fs);

            if (clpos_inview(&cv, fs->pos))
                Send_refuel(conn, fs->pos, pl_i->pos);
        }
        if (Player_is_repairing(pl_i))
        {
            target_t *targ = Target_by_index(pl_i->repair_target);

            if (clpos_inview(&cv, targ->pos))
                /* same packet as refuel */
                Send_refuel(conn, pl_i->pos, targ->pos);
        }
        if (Player_uses_tractor_beam(pl_i))
        {
            player_t *t = Player_by_id(pl_i->lock.pl_id);

            if (clpos_inview(&cv, t->pos))
            {
                int j;

                for (j = 0; j < 3; j++)
                {
                    clpos_t pts, pos;

                    pts = Ship_get_point_clpos(t->ship, j, t->dir);
                    pos.cx = t->pos.cx + pts.cx;
                    pos.cy = t->pos.cy + pts.cy;
                    Send_connector(conn, pos, pl_i->pos, 1);
                }
            }
        }

        if (pl_i->ball != NULL && clpos_inview(&cv, pl_i->ball->pos))
            Send_connector(conn, pl_i->ball->pos, pl_i->pos, 0);
    }
}

static void Frame_radar(connection_t *conn, player_t *pl)
{
    int i, k, mask, shownuke, size;
    object_t *shot;
    clpos_t pos;

    Frame_radar_buffer_reset();

#ifndef NO_SMART_MIS_RADAR
    if (options.nukesOnRadar)
        mask = OBJ_SMART_SHOT_BIT | OBJ_TORPEDO_BIT | OBJ_HEAT_SHOT_BIT | OBJ_MINE_BIT;
    else
    {
        mask = (options.missilesOnRadar ? (OBJ_SMART_SHOT_BIT | OBJ_TORPEDO_BIT | OBJ_HEAT_SHOT_BIT) : 0);
        mask |= (options.minesOnRadar) ? OBJ_MINE_BIT : 0;
    }
    if (options.treasuresOnRadar)
        mask |= OBJ_BALL_BIT;
    if (options.asteroidsOnRadar)
        mask |= OBJ_ASTEROID_BIT;

    if (mask)
    {
        for (i = 0; i < NumObjs; i++)
        {
            shot = Obj[i];
            if (!BIT(shot->type, mask))
                continue;

            // shownuke = (options.nukesOnRadar && (shot)->mods.nuclear);
            shownuke = (options.nukesOnRadar && Mods_get(shot->mods, ModsNuclear));
            if (shownuke && (frame_loops_slow & 2))
                size = 3;
            else
                size = 0;

            if (shot->type == OBJ_MINE)
            {
                if (!options.minesOnRadar && !shownuke)
                    continue;
                if (frame_loops_slow % 8 >= 6)
                    continue;
            }
            else if (shot->type == OBJ_BALL)
            {
                size = 2;
            }
            else if (shot->type == OBJ_ASTEROID)
            {
                size = WIRE_PTR(shot)->wire_size + 1;
                size |= 0x80;
            }
            else
            {
                if (!options.missilesOnRadar && !shownuke)
                    continue;
                if (frame_loops_slow & 1)
                    continue;
            }

            pos = shot->pos;
            if (Wrap_length(pl->pos.cx - pos.cx,
                            pl->pos.cy - pos.cy) <= pl->sensor_range * CLICK)
                Frame_radar_buffer_add(pos, size);
        }
    }
#endif

    if (options.playersOnRadar || BIT(world->rules->mode, TEAM_PLAY) || NumPseudoPlayers > 0 || NumAlliances > 0)
    {
        for (k = 0; k < num_player_shuffle; k++)
        {
            player_t *pl_i;

            i = player_shuffle_ptr[k];
            pl_i = Player_by_index(i);
            /*
             * Don't show on the radar:
             *                Ourselves (not necessarily same as who we watch).
             *                People who are not playing.
             *                People in other teams or alliances if;
             *                         no options.playersOnRadar or if not visible
             */
            if (pl_i->conn == conn || !Player_is_active(pl_i) /* kps - active / playing ??? */
                || (!Players_are_teammates(pl_i, pl) && !Players_are_allies(pl, pl_i) && !Player_owns_tank(pl, pl_i) && (!options.playersOnRadar || !pl->visibility[i].canSee)))
                continue;
            pos = pl_i->pos;
            if (BIT(world->rules->mode, LIMITED_VISIBILITY) && Wrap_length(pl->pos.cx - pos.cx,
                                                                           pl->pos.cy - pos.cy) > pl->sensor_range * CLICK)
                continue;
            if (Player_uses_compass(pl) && BIT(pl->lock.tagged, LOCK_PLAYER) && GetInd(pl->lock.pl_id) == i && frame_loops_slow % 5 >= 3)
                continue;
            size = 3;
            if (Players_are_teammates(pl_i, pl) || Players_are_allies(pl, pl_i) || Player_owns_tank(pl, pl_i))
                size |= 0x80;
            Frame_radar_buffer_add(pos, size);
        }
    }

    Frame_radar_buffer_send(conn, pl);
}

static void Frame_lose_item_state(player_t *pl)
{
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

    pv.world.x = pl->pix_pos.x - view_width / 2; /* Scroll */
    pv.world.y = pl->pix_pos.y - view_height / 2;
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

    view_cwidth = PIXEL_TO_CLICK(view_width);
    view_cheight = PIXEL_TO_CLICK(view_height);

    cv.world.cx = pl->pos.cx - view_cwidth / 2; /* Scroll */
    cv.world.cy = pl->pos.cy - view_cheight / 2;
    cv.realWorld = cv.world;
    if (BIT(world->rules->mode, WRAP_PLAY))
    {
        if (cv.world.cx < 0 && cv.world.cx + view_cwidth < world->cwidth)
            cv.world.cx += world->cwidth;
        else if (cv.world.cx > 0 && cv.world.cx + view_cwidth >= world->cwidth)
            cv.realWorld.cx -= world->cwidth;
        if (cv.world.cy < 0 && cv.world.cy + view_cheight < world->cheight)
            cv.world.cy += world->cheight;
        else if (cv.world.cy > 0 && cv.world.cy + view_cheight >= world->cheight)
            cv.realWorld.cy -= world->cheight;
    }
}

void Frame_update(void)
{
    int i, ind, player_fps;
    connection_t *conn;
    player_t *pl, *pl2;
    time_t newTimeLeft = 0;
    static time_t oldTimeLeft;
    static bool game_over_called = false;
    static double frame_time2 = 0.0;

    if (++frame_loops >= LONG_MAX) /* Used for misc. timing purposes */
        frame_loops = 1;
    frame_time += timeStep;
    frame_time2 += timeStep;
    if (frame_time2 >= 1.0)
    {
        frame_time2 -= 1.0;
        frame_loops_slow++;
    }

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
        pl = Player_by_index(i);
        conn = pl->conn;
        if (conn == NULL)
            continue;
        if (BIT(pl->obj_status, PAUSE | GAME_OVER) && !options.allowViewing && !pl->isowner)
        {
            /*
             * Lower the frame rate for non-playing players
             * to reduce network load.
             * Owner always gets full framerate even if paused.
             * With allowViewing on, everyone gets full framerate.
             */
            if (Player_is_paused(pl))
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
            if ((BIT(pl->obj_status, (GAME_OVER | PLAYING)) == (GAME_OVER | PLAYING)) ||
                (Player_is_paused(pl) &&
                 ((BIT(world->rules->mode, TEAM_PLAY) && pl->team != TEAM_NOT_SET && pl->team == Player_by_id(pl->lock.pl_id)->team) ||
                  pl->isowner ||
                  options.allowViewing)))
                ind = GetInd(pl->lock.pl_id);
            else
                ind = i;
        }
        else
            ind = i;
        pl2 = PlayersArray[ind];
        if (pl2->damaged > 0)
            Send_damaged(conn, pl2->damaged);
        else
        {
            Frame_parameters(conn, pl2);
            if (Frame_status(conn, pl2) <= 0)
                continue;
            Frame_map(conn, pl2);
            Frame_ships(conn, pl2);
            Frame_shots(conn, pl2);
            Frame_radar(conn, pl2);
            Frame_lose_item_state(pl);
            debris_end(conn);
            fastshot_end(conn);
        }
        sound_play_queued(pl2);
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
        warn("Max message len exceed (%d,%s)", i, message);
        strlcpy(tmp, message, MSG_LEN);
        msg = tmp;
    }
    else
        msg = message;
    for (i = 0; i < NumPlayers; i++)
    {
        pl = Player_by_index(i);
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
        warn("Max message len exceed (%d,%s)", i, message);
        strlcpy(tmp, message, MSG_LEN);
        msg = tmp;
    }
    else
        msg = message;
    if (pl->conn != NULL)
        Send_message(pl->conn, msg);
    else if (Player_is_robot(pl))
        Robot_message(pl, msg);
}

void Set_message_f(const char *fmt, ...)
{
    player_t *pl;
    int i;
    size_t len;
    static char msg[2 * MSG_LEN];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    if ((len = strlen(msg)) >= MSG_LEN)
    {
        warn("Set_message_f: Max len exceeded (%d,\"%s\")", len, msg);
        msg[MSG_LEN - 1] = '\0';
        assert(strlen(msg) < MSG_LEN);
    }

    // teamcup_log("    %s\n", msg);

    // if (!rplayback || playback)
    for (i = 0; i < NumPlayers; i++)
    {
        pl = Player_by_index(i);
        if (pl->conn != NULL)
            Send_message(pl->conn, msg);
    }
    // for (i = 0; i < NumSpectators; i++)
    // {
    //     pl = Player_by_index(i + spectatorStart);
    //     Send_message(pl->conn, msg);
    // }
}

void Set_player_message_f(player_t *pl, const char *fmt, ...)
{
    size_t len;
    static char msg[2 * MSG_LEN];
    va_list ap;

    // if (rplayback && !playback && pl->rectype != 2)
    //     return;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    if ((len = strlen(msg)) >= MSG_LEN)
    {
        warn("Set_player_message_f: Max len exceeded (%d,\"%s\")",
             len, msg);
        msg[MSG_LEN - 1] = '\0';
        assert(strlen(msg) < MSG_LEN);
    }

    if (pl->conn != NULL)
        Send_message(pl->conn, msg);
    else if (Player_is_robot(pl))
        Robot_message(pl, msg);
}
