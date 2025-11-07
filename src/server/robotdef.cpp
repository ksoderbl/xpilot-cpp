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
/* Robot code originally submitted by Maurice Abraham. */

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <cerrno>
#include <climits>

#include <unistd.h>

#include "commonmacros.h"

#include "server.h"

#define SERVER
#include "version.h"
#include "xpconfig.h"
#include "serverconst.h"
#include "global.h"
#include "map.h"
#include "score.h"
#include "bit.h"
#include "saudio.h"
#include "netserver.h"
#include "pack.h"
#include "robot.h"
#include "xperror.h"
#include "portability.h"
#include "xpmath.h"
#include "object.h"
#include "walls.h"

#define ROB_LOOK_AH 2

#define WITHIN(NOW, THEN, DIFF) (NOW <= THEN && (THEN - NOW) < DIFF)

/*
 * Flags for the default robots being in different modes (or moods).
 */
#define RM_ROBOT_IDLE (1 << 2)
#define RM_EVADE_LEFT (1 << 3)
#define RM_EVADE_RIGHT (1 << 4)
#define RM_ROBOT_CLIMB (1 << 5)
#define RM_HARVEST (1 << 6)
#define RM_ATTACK (1 << 7)
#define RM_TAKE_OFF (1 << 8)
#define RM_CANNON_KILL (1 << 9)
#define RM_REFUEL (1 << 10)
#define RM_NAVIGATE (1 << 11)

/* long term modes */
#define FETCH_TREASURE (1 << 0)
#define TARGET_KILL (1 << 1)
#define NEED_FUEL (1 << 2)

/*
 * Map objects a robot can fly through without damage.
 */
#define EMPTY_SPACE(s) BIT(1 << (s), SPACE_BLOCKS)

/*
 * Prototypes for methods of the default robot type.
 */
static void Robot_default_round_tick(void);
static void Robot_default_create(player_t *pl, char *str);
static void Robot_default_go_home(player_t *pl);
static void Robot_default_play(player_t *pl);
static void Robot_default_set_war(player_t *pl, int victim_id);
static int Robot_default_war_on_player(player_t *pl);
static void Robot_default_message(player_t *pl, const char *str);
static void Robot_default_destroy(player_t *pl);
static void Robot_default_invite(player_t *pl, player_t *inviter);
int Robot_default_setup(robot_type_t *type_ptr);

/*
 * Local static variables
 */
static double Visibility_distance;
static double Max_enemy_distance;

/*
 * The robot type structure for the default robot.
 */
static robot_type_t robot_default_type = {
    "default",
    Robot_default_round_tick,
    Robot_default_create,
    Robot_default_go_home,
    Robot_default_play,
    Robot_default_set_war,
    Robot_default_war_on_player,
    Robot_default_message,
    Robot_default_destroy,
    Robot_default_invite};

/*
 * The only thing we export from this file.
 * A function to initialize the robot type structure
 * with our name and the pointers to our action routines.
 *
 * Return 0 if all is OK, anything else will ignore this
 * robot type forever.
 */
int Robot_default_setup(robot_type_t *type_ptr)
{
    /* Not much to do for the default robot except init the type structure. */

    *type_ptr = robot_default_type;

    return 0;
}

/*
 * Private functions.
 */
static bool Check_robot_evade(player_t *pl, int mine_i, int ship_i);
static bool Check_robot_target(player_t *pl, clpos_t item_pos, int new_mode);
static bool Detect_ship(player_t *pl, player_t *ship);
static int Rank_item_value(player_t *pl, enum Item itemtype);
static bool Ball_handler(player_t *pl);

/*
 * Function to cast from player structure to robot data structure.
 * This isolates casts (aka. type violations) to a few places.
 */
static robot_default_data_t *Robot_default_get_data(player_t *pl)
{
    return (robot_default_data_t *)pl->robot_data_ptr->private_data;
}

/*
 * A default robot is created.
 */
static void Robot_default_create(player_t *pl, char *str)
{
    robot_default_data_t *my_data;

    if (!(my_data = XMALLOC(robot_default_data_t, 1)))
    {
        error("no mem for default robot");
        End_game();
    }

    my_data->robot_mode = RM_TAKE_OFF;
    my_data->robot_count = 0;
    my_data->robot_lock = LOCK_NONE;
    my_data->robot_lock_id = 0;
    my_data->longterm_mode = 0;

    if (str != NULL && *str != '\0' && sscanf(str, " %d %d", &my_data->attack, &my_data->defense) != 2)
    {
        if (str && *str)
        {
            xpprintf("%s invalid parameters for default robot: \"%s\"\n", showtime(), str);
            my_data->attack = (int)(rfrac() * 99.5f);
            my_data->defense = 100 - my_data->attack;
        }
        LIMIT(my_data->attack, 1, 99);
        LIMIT(my_data->defense, 1, 99);
    }
    /*
     * some parameters which may be changed to be dependent upon
     * the 'attack' and 'defense' settings of this robot.
     */
    if (BIT(world->rules->mode, TIMING))
    {
        my_data->robot_normal_speed = 10.0;
        my_data->robot_attack_speed = 25.0 + (my_data->attack / 10);
        my_data->robot_max_speed = 50.0 + (my_data->attack / 20) - (my_data->defense / 50);
    }
    else
    {
        my_data->robot_normal_speed = 6.0;
        my_data->robot_attack_speed = 15.0 + (my_data->attack / 25);
        my_data->robot_max_speed = 30.0 + (my_data->attack / 50) - (my_data->defense / 50);
    }

    pl->fuel.l3 += my_data->defense - my_data->attack + (int)((rfrac() - 0.5f) * 20);
    pl->fuel.l2 += 2 * (my_data->defense - my_data->attack) / 5 + (int)((rfrac() - 0.5f) * 8);
    pl->fuel.l1 += (my_data->defense - my_data->attack) / 5 + (int)((rfrac() - 0.5f) * 4);

    my_data->last_used_ecm = 0;
    my_data->last_dropped_mine = 0;
    my_data->last_fired_missile = 0;
    my_data->last_thrown_ball = 0;

    my_data->longterm_mode = 0;

    pl->robot_data_ptr->private_data = (void *)my_data;
}

/*
 * A default robot is placed on its homebase.
 */
static void Robot_default_go_home(player_t *pl)
{
    robot_default_data_t *my_data = Robot_default_get_data(pl);

    my_data->robot_mode = RM_TAKE_OFF;
    my_data->longterm_mode = 0;
}

/*
 * A default robot is declaring war (or resetting war).
 */
static void Robot_default_set_war(player_t *pl, int victim_id)
{
    robot_default_data_t *my_data = Robot_default_get_data(pl);

    if (victim_id == NO_ID)
        CLR_BIT(my_data->robot_lock, LOCK_PLAYER);
    else
    {
        my_data->robot_lock_id = victim_id;
        SET_BIT(my_data->robot_lock, LOCK_PLAYER);
    }
}

/*
 * Return the id of the player a default robot has war against (or NO_ID).
 */
static int Robot_default_war_on_player(player_t *pl)
{
    robot_default_data_t *my_data = Robot_default_get_data(pl);

    if (BIT(my_data->robot_lock, LOCK_PLAYER))
        return my_data->robot_lock_id;
    else
        return NO_ID;
}

/*
 * A default robot receives a message.
 */
static void Robot_default_message(player_t *pl, const char *message)
{
}

/*
 * A default robot is destroyed.
 */
static void Robot_default_destroy(player_t *pl)
{
    XFREE(pl->robot_data_ptr->private_data);
}

/*
 * A default robot is asked to join an alliance
 */
static void Robot_default_invite(player_t *pl, player_t *inviter)
{
    int war_id = Robot_default_war_on_player(pl), i;
    robot_default_data_t *my_data = Robot_default_get_data(pl);
    double limit;
    bool we_accept = true;

    if (pl->alliance != ALLIANCE_NOT_SET)
    {
        /* if there is a human in our alliance, they should decide
           let robots refuse in this case */
        for (i = 0; i < NumPlayers; i++)
        {
            player_t *pl_i = Player_by_index(i);

            if (Player_is_human(pl_i) && Players_are_allies(pl, pl_i))
            {
                we_accept = false;
                break;
            }
        }
        if (!we_accept)
        {
            Refuse_alliance(pl, inviter);
            return;
        }
    }
    limit = MAX(ABS(pl->score / MAX((my_data->attack / 10), 10)),
                my_data->defense);
    if (inviter->alliance == ALLIANCE_NOT_SET)
    {
        /* don't accept players we are at war with */
        if (inviter->id == war_id)
            we_accept = false;
        /* don't accept players who are not active */
        if (BIT(inviter->obj_status, PLAYING | GAME_OVER | PAUSE) != PLAYING)
            we_accept = false;
        /* don't accept players with scores substantially lower than ours */
        else if (inviter->score < (pl->score - limit))
            we_accept = false;
    }
    else
    {
        double avg_score = 0;
        int member_count = Get_alliance_member_count(inviter->alliance);

        for (i = 0; i < NumPlayers; i++)
        {
            if (Player_by_index(i)->alliance == inviter->alliance)
            {
                if (Player_by_index(i)->id == war_id)
                {
                    we_accept = false;
                    break;
                }
                avg_score += Player_by_index(i)->score;
            }
        }
        if (we_accept)
        {
            avg_score = avg_score / member_count;
            if (avg_score < (pl->score - limit))
                we_accept = false;
        }
    }
    if (we_accept)
        Accept_alliance(pl, inviter);
    else
        Refuse_alliance(pl, inviter);
}

static bool Really_empty_space(player_t *pl, int x, int y)
{
    // player_t *pl = PlayersArray[ind];
    int type = world->block[x][y];

    if (EMPTY_SPACE(type))
        return true;
    switch (type)
    {
    case FILLED:
    case REC_LU:
    case REC_LD:
    case REC_RU:
    case REC_RD:
    case FUEL:
    case TREASURE:
        return false;

    case WORMHOLE:
        if (!options.wormholeVisible || world->wormholes[world->itemID[x][y]].type == WORM_OUT)
        {
            return true;
        }
        else
        {
            return false;
        }

    case TARGET:
        if (!options.targetTeamCollision && BIT(world->rules->mode, TEAM_PLAY) && world->targets[world->itemID[x][y]].team == pl->team)
        {
            return true;
        }
        else
        {
            return false;
        }

    case CANNON:
        if (options.teamImmunity && BIT(world->rules->mode, TEAM_PLAY) && world->cannons[world->itemID[x][y]].team == pl->team)
        {
            return true;
        }
        else
        {
            return false;
        }

    default:
        break;
    }
    return false;
}

static bool Check_robot_evade(player_t *pl, int mine_i, int ship_i)
{
    int i;
    object_t *shot;
    player_t *ship;
    long stop_dist;
    bool evade;
    bool left_ok, right_ok;
    int safe_width;
    int travel_dir;
    int delta_dir;
    int aux_dir;
    int px[3], py[3];
    long dist;
    vector_t *gravity;
    int gravity_dir;
    long dx, dy;
    double velocity;
    robot_default_data_t *my_data = Robot_default_get_data(pl);

    safe_width = (my_data->defense / 200) * SHIP_SZ;
    /* Prevent overflow. */
    velocity = (pl->velocity <= SPEED_LIMIT) ? pl->velocity : SPEED_LIMIT;
    stop_dist = (long)((RES * velocity) / (MAX_PLAYER_TURNSPEED * pl->turnresistance) + (velocity * velocity * pl->mass) / (2 * MAX_PLAYER_POWER) + safe_width);
    /*
     * Limit the look ahead.  For very high speeds the current code
     * is ineffective and much too inefficient.
     */
    if (stop_dist > 10 * BLOCK_SZ)
        stop_dist = 10 * BLOCK_SZ;

    evade = false;

    if (pl->velocity <= 0.2)
    {
        vector_t *grav = &world->gravity
                              [OBJ_X_IN_BLOCKS(pl)][OBJ_Y_IN_BLOCKS(pl)];
        travel_dir = (int)findDir(grav->x, grav->y);
    }
    else
    {
        travel_dir = (int)findDir(pl->vel.x, pl->vel.y);
    }

    aux_dir = MOD2(travel_dir + RES / 4, RES);
    px[0] = CLICK_TO_PIXEL(pl->pos.cx);                /* ship center x */
    py[0] = CLICK_TO_PIXEL(pl->pos.cy);                /* ship center y */
    px[1] = (int)(px[0] + safe_width * tcos(aux_dir)); /* ship left side x */
    py[1] = (int)(py[0] + safe_width * tsin(aux_dir)); /* ship left side y */
    px[2] = 2 * px[0] - px[1];                         /* ship right side x */
    py[2] = 2 * py[0] - py[1];                         /* ship right side y */

    left_ok = true;
    right_ok = true;

    for (dist = 0; dist < stop_dist + BLOCK_SZ / 2; dist += BLOCK_SZ / 2)
    {
        for (i = 0; i < 3; i++)
        {
            dx = (long)((px[i] + dist * tcos(travel_dir)) / BLOCK_SZ);
            dy = (long)((py[i] + dist * tsin(travel_dir)) / BLOCK_SZ);

            if (BIT(world->rules->mode, WRAP_PLAY))
            {
                if (dx < 0)
                    dx += world->x;
                else if (dx >= world->x)
                    dx -= world->x;
                if (dy < 0)
                    dy += world->y;
                else if (dy >= world->y)
                    dy -= world->y;
            }
            if (dx < 0 || dx >= world->x || dy < 0 || dy >= world->y)
            {
                evade = true;
                if (i == 1)
                    left_ok = false;
                if (i == 2)
                    right_ok = false;
                continue;
            }
            if (!Really_empty_space(pl, dx, dy))
            {
                evade = true;
                if (i == 1)
                    left_ok = false;
                if (i == 2)
                    right_ok = false;
                continue;
            }
            /* Watch out for strong gravity */
            gravity = &world->gravity[dx][dy];
            if (sqr(gravity->x) + sqr(gravity->y) >= 0.5)
            {
                gravity_dir = (int)findDir(gravity->x - pl->pix_pos.x,
                                           gravity->y - pl->pix_pos.y);
                if (MOD2(gravity_dir - travel_dir, RES) <= RES / 4 ||
                    MOD2(gravity_dir - travel_dir, RES) >= 3 * RES / 4)
                {
                    evade = true;
                    if (i == 1)
                        left_ok = false;
                    if (i == 2)
                        right_ok = false;
                    continue;
                }
            }
        }
    }

    if (mine_i >= 0)
    {
        shot = Obj[mine_i];
        aux_dir = (int)Wrap_findDir(shot->pix_pos.x + shot->vel.x - pl->pix_pos.x,
                                    shot->pix_pos.y + shot->vel.y - pl->pix_pos.y);
        delta_dir = MOD2(aux_dir - travel_dir, RES);
        if (delta_dir < RES / 4)
        {
            left_ok = false;
            evade = true;
        }
        if (delta_dir > RES * 3 / 4)
        {
            right_ok = false;
            evade = true;
        }
    }
    if (ship_i >= 0)
    {
        ship = PlayersArray[ship_i];
        aux_dir = (int)Wrap_findDir(ship->pix_pos.x - pl->pix_pos.x + ship->vel.x * 2,
                                    ship->pix_pos.y - pl->pix_pos.y + ship->vel.y * 2);
        delta_dir = MOD2(aux_dir - travel_dir, RES);
        if (delta_dir < RES / 4)
        {
            left_ok = false;
            evade = true;
        }
        if (delta_dir > RES * 3 / 4)
        {
            right_ok = false;
            evade = true;
        }
    }
    if (pl->velocity > my_data->robot_max_speed)
        evade = true;

    if (!evade)
        return false;

    delta_dir = 0;
    while (!left_ok && !right_ok && delta_dir < 7 * RES / 8)
    {
        delta_dir += RES / 16;

        left_ok = true;
        aux_dir = MOD2(travel_dir + delta_dir, RES);
        for (dist = 0; dist < stop_dist + BLOCK_SZ / 2; dist += BLOCK_SZ / 2)
        {
            dx = (long)((px[0] + dist * tcos(aux_dir)) / BLOCK_SZ);
            dy = (long)((py[0] + dist * tsin(aux_dir)) / BLOCK_SZ);

            if (BIT(world->rules->mode, WRAP_PLAY))
            {
                if (dx < 0)
                    dx += world->x;
                else if (dx >= world->x)
                    dx -= world->x;
                if (dy < 0)
                    dy += world->y;
                else if (dy >= world->y)
                    dy -= world->y;
            }
            if (dx < 0 || dx >= world->x || dy < 0 || dy >= world->y)
            {
                left_ok = false;
                continue;
            }
            if (!Really_empty_space(pl, dx, dy))
            {
                left_ok = false;
                continue;
            }
            /* watch out for strong gravity */
            gravity = &world->gravity[dx][dy];
            if (sqr(gravity->x) + sqr(gravity->y) >= 0.5)
            {
                gravity_dir = (int)findDir(gravity->x - pl->pix_pos.x,
                                           gravity->y - pl->pix_pos.y);
                if (MOD2(gravity_dir - travel_dir, RES) <= RES / 4 ||
                    MOD2(gravity_dir - travel_dir, RES) >= 3 * RES / 4)
                {

                    left_ok = false;
                    continue;
                }
            }
        }

        right_ok = true;
        aux_dir = MOD2(travel_dir - delta_dir, RES);
        for (dist = 0; dist < stop_dist + BLOCK_SZ / 2; dist += BLOCK_SZ / 2)
        {
            dx = (long)((px[0] + dist * tcos(aux_dir)) / BLOCK_SZ);
            dy = (long)((py[0] + dist * tsin(aux_dir)) / BLOCK_SZ);

            if (BIT(world->rules->mode, WRAP_PLAY))
            {
                if (dx < 0)
                    dx += world->x;
                else if (dx >= world->x)
                    dx -= world->x;
                if (dy < 0)
                    dy += world->y;
                else if (dy >= world->y)
                    dy -= world->y;
            }
            if (dx < 0 || dx >= world->x || dy < 0 || dy >= world->y)
            {
                right_ok = false;
                continue;
            }
            if (!Really_empty_space(pl, dx, dy))
            {
                right_ok = false;
                continue;
            }
            /* watch out for strong gravity */
            gravity = &world->gravity[dx][dy];
            if (sqr(gravity->x) + sqr(gravity->y) >= 0.5)
            {
                gravity_dir = (int)findDir(gravity->x - pl->pix_pos.x,
                                           gravity->y - pl->pix_pos.y);
                if (MOD2(gravity_dir - travel_dir, RES) <= RES / 4 ||
                    MOD2(gravity_dir - travel_dir, RES) >= 3 * RES / 4)
                {

                    right_ok = false;
                    continue;
                }
            }
        }
    }

    pl->turnspeed = MAX_PLAYER_TURNSPEED;
    pl->power = MAX_PLAYER_POWER;

    delta_dir = MOD2(pl->dir - travel_dir, RES);

    if (my_data->robot_mode != RM_EVADE_LEFT && my_data->robot_mode != RM_EVADE_RIGHT)
    {
        if (left_ok && !right_ok)
            my_data->robot_mode = RM_EVADE_LEFT;
        else if (right_ok && !left_ok)
            my_data->robot_mode = RM_EVADE_RIGHT;
        else
            my_data->robot_mode = (delta_dir < RES / 2 ? RM_EVADE_LEFT : RM_EVADE_RIGHT);
    }
    /*-BA If facing the way we want to go, thrust
     *-BA If too far off, stop thrusting
     *-BA If in between, keep doing whatever we are already doing
     *-BA In all cases continue to straighten up
     */
    if (delta_dir < RES / 4 || delta_dir > 3 * RES / 4)
    {
        pl->turnacc = (my_data->robot_mode == RM_EVADE_LEFT ? pl->turnspeed : (-pl->turnspeed));
        Thrust(pl, false);
    }
    else if (delta_dir < 3 * RES / 8 || delta_dir > 5 * RES / 8)
        pl->turnacc = (my_data->robot_mode == RM_EVADE_LEFT ? pl->turnspeed : (-pl->turnspeed));
    else
    {
        pl->turnacc = 0;
        Thrust(pl, true);
        my_data->robot_mode = (delta_dir < RES / 2
                                   ? RM_EVADE_LEFT
                                   : RM_EVADE_RIGHT);
    }

    return true;
}

static void Robot_check_new_modifiers(player_t *pl, modifiers_t mods)
{
    if (!BIT(world->rules->mode, ALLOW_NUKES))
        mods.nuclear = 0;
    if (!BIT(world->rules->mode, ALLOW_CLUSTERS))
        CLR_BIT(mods.warhead, CLUSTER);
    if (!BIT(world->rules->mode, ALLOW_MODIFIERS))
    {
        mods.velocity =
            mods.mini =
                mods.spread =
                    mods.power = 0;
        CLR_BIT(mods.warhead, IMPLOSION);
    }
    if (!BIT(world->rules->mode, ALLOW_LASER_MODIFIERS))
        mods.laser = 0;
    pl->mods = mods;
}

static void Choose_weapon_modifier(player_t *pl, int weapon_type)
{
    int stock, min;
    modifiers_t mods;
    robot_default_data_t *my_data = Robot_default_get_data(pl);

    CLEAR_MODS(mods);

    switch (weapon_type)
    {
    case HAS_TRACTOR_BEAM:
        Robot_check_new_modifiers(pl, mods);
        return;

    case HAS_LASER:
        /*
         * Robots choose non-damage laser settings occasionally.
         */
        if ((my_data->robot_count % 4) == 0)
            mods.laser = (int)(rfrac() * (MODS_LASER_MAX + 1));
        Robot_check_new_modifiers(pl, mods);
        return;

    case OBJ_SHOT:
        /*
         * Robots usually use wide beam shots, however they may narrow
         * the beam occasionally.
         */
        mods.spread = 0;
        if ((my_data->robot_count % 4) == 0)
            mods.spread = (int)(rfrac() * (MODS_SPREAD_MAX + 1));
        Robot_check_new_modifiers(pl, mods);
        return;

    case OBJ_MINE:
        stock = pl->item[ITEM_MINE];
        min = options.nukeMinMines;
        break;

    case OBJ_SMART_SHOT:
    case OBJ_HEAT_SHOT:
    case OBJ_TORPEDO:
        stock = pl->item[ITEM_MISSILE];
        min = options.nukeMinSmarts;
        if ((my_data->robot_count % 4) == 0)
            mods.power = (int)(rfrac() * (MODS_POWER_MAX + 1));
        break;

    default:
        return;
    }

    if (stock >= min)
    {
        /*
         * More aggressive robots will choose to use nuclear weapons, this
         * means you can safely approach wimpy robots... perhaps.
         */
        if ((my_data->robot_count % 100) <= my_data->attack)
        {
            SET_BIT(mods.nuclear, NUCLEAR);
            if (stock > min && (stock < (2 * min) || (my_data->robot_count % 2) == 0))
                SET_BIT(mods.nuclear, FULLNUCLEAR);
        }
    }

    if (pl->fuel.sum > pl->fuel.l3)
    {
        if ((my_data->robot_count % 2) == 0)
        {
            if ((my_data->robot_count % 8) == 0)
                mods.velocity = (int)(rfrac() * MODS_VELOCITY_MAX) + 1;
            SET_BIT(mods.warhead, CLUSTER);
        }
    }
    else if ((my_data->robot_count % 3) == 0)
    {
        SET_BIT(mods.warhead, IMPLOSION);
    }

    /*
     * Robot may change to use mini device setting occasionally.
     */
    if ((my_data->robot_count % 10) == 0)
    {
        mods.mini = (int)(rfrac() * (MODS_MINI_MAX + 1));
        mods.spread = (int)(rfrac() * (MODS_SPREAD_MAX + 1));
    }

    Robot_check_new_modifiers(pl, mods);
}

static void Robotdef_fire_laser(player_t *pl)
{
    long item_dist;
    int item_dir;
    int travel_dir;
    int delta_dir;
    long dx, dy;
    long dist;
    bool clear_path, slowing;
    robot_default_data_t *my_data = Robot_default_get_data(pl);
    player_t *ship;

    if (BIT(my_data->robot_lock, LOCK_PLAYER) && BIT(Player_by_id(my_data->robot_lock_id)->obj_status,
                                                     PLAYING | PAUSE | GAME_OVER) == PLAYING)
        ship = Player_by_id(my_data->robot_lock_id);
    else if (BIT(pl->lock.tagged, LOCK_PLAYER))
        ship = Player_by_id(pl->lock.pl_id);
    else
        ship = NULL;
    if (ship && BIT(ship->obj_status, PLAYING | PAUSE | GAME_OVER) == PLAYING)
    {
        double x1, y1, x3, y3, x4, y4, x5, y5;
        double ship_dist, dir3, dir4, dir5;

        x1 = CLICK_TO_FLOAT(pl->pos.cx) + pl->vel.x + pl->ship->m_gun[pl->dir].x;
        y1 = CLICK_TO_FLOAT(pl->pos.cy) + pl->vel.y + pl->ship->m_gun[pl->dir].y;
        x3 = CLICK_TO_FLOAT(ship->pos.cx) + ship->vel.x;
        y3 = CLICK_TO_FLOAT(ship->pos.cy) + ship->vel.y;

        ship_dist = Wrap_length(x3 - x1, y3 - y1);

        if (ship_dist < PULSE_SPEED * PULSE_LIFE(pl->item[ITEM_LASER]) + SHIP_SZ)
        {
            dir3 = Wrap_findDir(x3 - x1, y3 - y1);
            x4 = x3 + tcos(MOD2((int)(dir3 - RES / 4), RES)) * SHIP_SZ;
            y4 = y3 + tsin(MOD2((int)(dir3 - RES / 4), RES)) * SHIP_SZ;
            x5 = x3 + tcos(MOD2((int)(dir3 + RES / 4), RES)) * SHIP_SZ;
            y5 = y3 + tsin(MOD2((int)(dir3 + RES / 4), RES)) * SHIP_SZ;
            dir4 = Wrap_findDir(x4 - x1, y4 - y1);
            dir5 = Wrap_findDir(x5 - x1, y5 - y1);
            if ((dir4 > dir5)
                    ? (pl->dir >= dir4 || pl->dir <= dir5)
                    : (pl->dir >= dir4 && pl->dir <= dir5))
                SET_BIT(pl->used, HAS_LASER);
        }
    }
}

static void Robotdef_do_tractor_beam(player_t *pl)
{
    long item_dist;
    int item_dir;
    int travel_dir;
    int delta_dir;
    long dx, dy;
    long dist;
    bool clear_path, slowing;
    robot_default_data_t *my_data = Robot_default_get_data(pl);
    player_t *ship;

    CLR_BIT(pl->used, USES_TRACTOR_BEAM);
    pl->tractor_is_pressor = false;

    if (BIT(pl->lock.tagged, LOCK_PLAYER) && pl->fuel.sum > pl->fuel.l3 && pl->lock.distance < TRACTOR_MAX_RANGE(pl->item[ITEM_TRACTOR_BEAM]))
    {
        double xvd, yvd, vel;
        long dir;
        int away;

        ship = Player_by_id(pl->lock.pl_id);
        xvd = ship->vel.x - pl->vel.x;
        yvd = ship->vel.y - pl->vel.y;
        vel = LENGTH(xvd, yvd);
        dir = (long)(findDir(pl->pix_pos.x - ship->pix_pos.x,
                             pl->pix_pos.y - ship->pix_pos.y) -
                     findDir(xvd, yvd));
        dir = MOD2(dir, RES);
        away = (dir >= RES / 4 && dir <= 3 * RES / 4);

        /*
         * vel  - The relative velocity of ship to us.
         * away - Heading away from us?
         */
        if (pl->velocity <= my_data->robot_normal_speed)
        {
            if (pl->lock.distance < (SHIP_SZ * 4) || (!away && vel > my_data->robot_attack_speed))
            {
                SET_BIT(pl->used, USES_TRACTOR_BEAM);
                pl->tractor_is_pressor = true;
            }
            else if (away && vel < my_data->robot_max_speed && vel > my_data->robot_normal_speed)
                SET_BIT(pl->used, USES_TRACTOR_BEAM);
        }
        if (Player_uses_tractor_beam(pl))
            SET_BIT(pl->lock.tagged, LOCK_VISIBLE);
    }
}

static bool Check_robot_target(player_t *pl, clpos_t item_pos, int new_mode)
{
    int item_dir, travel_dir, delta_dir;
    int dx, dy;
    double dist, item_dist, idir;
    bool clear_path, slowing;
    robot_default_data_t *my_data = Robot_default_get_data(pl);

    dx = CLICK_TO_PIXEL(item_pos.cx - pl->pos.cx), dx = WRAP_DX(dx);
    dy = CLICK_TO_PIXEL(item_pos.cy - pl->pos.cy), dy = WRAP_DY(dy);

    item_dist = LENGTH(dy, dx);

    if (dx == 0 && dy == 0)
    {
        vector_t *grav = &world->gravity
                              [OBJ_X_IN_BLOCKS(pl)][OBJ_Y_IN_BLOCKS(pl)];
        idir = findDir(grav->x, grav->y);
        item_dir = (int)(idir + 0.5);
        item_dir = MOD2(item_dir + RES / 2, RES);
    }
    else
    {
        idir = findDir((double)dx, (double)dy);
        item_dir = (int)(idir + 0.5);
        item_dir = MOD2(item_dir, RES);
    }

    if (new_mode == RM_REFUEL)
        item_dist = item_dist - 90;

    clear_path = true;

    for (dist = 0; clear_path && dist < item_dist; dist += BLOCK_SZ / 2)
    {
        dx = (int)((CLICK_TO_PIXEL(pl->pos.cx) + dist * tcos(item_dir)) / BLOCK_SZ);
        dy = (int)((CLICK_TO_PIXEL(pl->pos.cy) + dist * tsin(item_dir)) / BLOCK_SZ);

        dx = WRAP_XBLOCK(dx);
        dy = WRAP_YBLOCK(dy);
        if (dx < 0 || dx >= world->x || dy < 0 || dy >= world->y)
        {
            clear_path = false;
            continue;
        }
        if (!Really_empty_space(pl, dx, dy))
        {
            clear_path = false;
            continue;
        }
    }

    if (new_mode == RM_CANNON_KILL)
        item_dist -= 4 * BLOCK_SZ;

    if (!clear_path && new_mode != RM_NAVIGATE)
        return false;

    if (pl->velocity <= 0.2)
    {
        vector_t *grav = &world->gravity
                              [OBJ_X_IN_BLOCKS(pl)][OBJ_Y_IN_BLOCKS(pl)];
        travel_dir = (int)findDir(grav->x, grav->y);
    }
    else
    {
        travel_dir = (int)findDir(pl->vel.x, pl->vel.y);
    }

    pl->turnspeed = MAX_PLAYER_TURNSPEED / 2;
    pl->power = (BIT(world->rules->mode, TIMING) ? MAX_PLAYER_POWER : MAX_PLAYER_POWER / 2);

    delta_dir = MOD2(item_dir - travel_dir, RES);
    if (delta_dir >= RES / 4 && delta_dir <= 3 * RES / 4)
    {

        if (new_mode == RM_HARVEST ||
            (new_mode == RM_NAVIGATE &&
             (clear_path || dist > 8 * BLOCK_SZ)))
            /* reverse direction of travel */
            item_dir = MOD2(travel_dir + (delta_dir > RES / 2
                                              ? -5 * RES / 8
                                              : 5 * RES / 8),
                            RES);
        pl->turnspeed = MAX_PLAYER_TURNSPEED;
        slowing = true;

        if (pl->item[ITEM_MINE] && item_dist < 8 * BLOCK_SZ)
        {
            Choose_weapon_modifier(pl, OBJ_MINE);
            if (BIT(world->rules->mode, TIMING))
                Place_mine(pl);
            else
                Place_moving_mine(pl);
            new_mode = (rfrac() < 0.5) ? RM_EVADE_RIGHT : RM_EVADE_LEFT;
        }
    }
    else if (new_mode == RM_CANNON_KILL && item_dist <= 0)
    {

        /* too close, so move away */
        pl->turnspeed = MAX_PLAYER_TURNSPEED;
        item_dir = MOD2(item_dir + RES / 2, RES);
        slowing = true;
    }
    else
        slowing = false;

    if (new_mode == RM_NAVIGATE && !clear_path)
    {
        if (dist <= 8 * BLOCK_SZ && dist > 4 * BLOCK_SZ)
            item_dir = MOD2(item_dir + (delta_dir > RES / 2
                                            ? -3 * RES / 4
                                            : 3 * RES / 4),
                            RES);
        else if (dist <= 4 * BLOCK_SZ)
            item_dir = MOD2(item_dir + RES / 2, RES);
        pl->turnspeed = MAX_PLAYER_TURNSPEED;
        slowing = true;
    }

    delta_dir = MOD2(item_dir - pl->dir, RES);

    if (delta_dir > RES / 8 && delta_dir < 7 * RES / 8)
        pl->turnspeed = MAX_PLAYER_TURNSPEED;
    else if (delta_dir > RES / 16 && delta_dir < 15 * RES / 16)
        pl->turnspeed = MAX_PLAYER_TURNSPEED;
    else if (delta_dir > RES / 64 && delta_dir < 63 * RES / 64)
        pl->turnspeed = MAX_PLAYER_TURNSPEED;
    else
        pl->turnspeed = 0.0;

    pl->turnacc = (delta_dir < RES / 2 ? pl->turnspeed : (-pl->turnspeed));

    if (slowing || BIT(pl->used, HAS_SHIELD))
        Thrust(pl, true);
    else if (item_dist < 0)
        Thrust(pl, false);
    else if (item_dist < 3 * BLOCK_SZ && new_mode != RM_HARVEST)
    {
        if (pl->velocity < my_data->robot_normal_speed / 2)
            Thrust(pl, true);
        if (pl->velocity > my_data->robot_normal_speed)
            Thrust(pl, false);
    }
    else if ((new_mode != RM_ATTACK && new_mode != RM_NAVIGATE) || item_dist < 8 * BLOCK_SZ || (new_mode == RM_NAVIGATE && delta_dir > 3 * RES / 8 && delta_dir < 5 * RES / 8))
    {
        if (pl->velocity < 2 * my_data->robot_normal_speed)
            Thrust(pl, true);
        if (pl->velocity > 3 * my_data->robot_normal_speed)
            Thrust(pl, false);
    }
    else if (new_mode == RM_ATTACK || (new_mode == RM_NAVIGATE && (dist < 12 * BLOCK_SZ || (delta_dir > RES / 8 && delta_dir < 7 * RES / 8))))
    {
        if (pl->velocity < my_data->robot_attack_speed / 2)
            Thrust(pl, true);
        if (pl->velocity > my_data->robot_attack_speed)
            Thrust(pl, false);
    }
    else if (clear_path && (delta_dir < RES / 8 || delta_dir > 7 * RES / 8) && item_dist > 18 * BLOCK_SZ)
    {
        if (pl->velocity < my_data->robot_max_speed - my_data->robot_normal_speed)
            Thrust(pl, true);
        if (pl->velocity > my_data->robot_max_speed)
            Thrust(pl, false);
    }
    else
    {
        if (pl->velocity < my_data->robot_attack_speed)
            Thrust(pl, true);
        if (pl->velocity > my_data->robot_max_speed - my_data->robot_normal_speed)
            Thrust(pl, false);
    }

    if (new_mode == RM_ATTACK || (BIT(world->rules->mode, TIMING) && new_mode == RM_NAVIGATE))
    {
        if (pl->item[ITEM_ECM] > 0 && item_dist < ECM_DISTANCE / 4)
            Fire_ecm(pl);
        else if (pl->item[ITEM_TRANSPORTER] > 0 && item_dist < TRANSPORTER_DISTANCE && pl->fuel.sum > -ED_TRANSPORTER)
            Do_transporter(pl);
        else if (pl->item[ITEM_LASER] > pl->num_pulses && pl->fuel.sum + ED_LASER > pl->fuel.l3 && new_mode == RM_ATTACK)
            Robotdef_fire_laser(pl);
        else if (Player_has_tractor_beam(pl))
            Robotdef_do_tractor_beam(pl);

        if (BIT(pl->used, HAS_LASER))
        {
            pl->turnacc = 0.0;
            Choose_weapon_modifier(pl, HAS_LASER);
        }
        /*-BA Be more agressive, esp if lots of ammo
         * else if ((my_data->robot_count % 10) == 0
         * && pl->item[ITEM_MISSILE] > 0)
         */
        else if ((my_data->robot_count % 10) < pl->item[ITEM_MISSILE] && !WITHIN(my_data->robot_count,
                                                                                 my_data->last_fired_missile, 10))
        {
            int type;

            switch (my_data->robot_count % 5)
            {
            case 0:
            case 1:
            case 2:
                type = OBJ_SMART_SHOT;
                break;
            case 3:
                type = OBJ_HEAT_SHOT;
                break;
            default:
                type = OBJ_TORPEDO;
                break;
            }
            if (Detect_ship(pl, Player_by_id(pl->lock.pl_id)) && !pl->visibility[GetInd(pl->lock.pl_id)].canSee)
                type = OBJ_HEAT_SHOT;
            if (type == OBJ_SMART_SHOT && !options.allowSmartMissiles)
                type = OBJ_HEAT_SHOT;
            Choose_weapon_modifier(pl, type);
            Fire_shot(pl, type, pl->dir);
            if (type == OBJ_HEAT_SHOT)
                Thrust(pl, false);
            my_data->last_fired_missile = my_data->robot_count;
        }
        else if ((my_data->robot_count % 2) == 0 && item_dist < Visibility_distance
                 /*&& BIT(my_data->robot_lock, LOCK_PLAYER)*/)
        {
            if ((int)(rfrac() * 64) < pl->item[ITEM_MISSILE])
            {
                Choose_weapon_modifier(pl, OBJ_SMART_SHOT);
                Fire_shot(pl, OBJ_SMART_SHOT, pl->dir);
                my_data->last_fired_missile = my_data->robot_count;
            }
            else
            {
                if ((new_mode == RM_ATTACK && clear_path) || (my_data->robot_count % 50) == 0)
                {
                    Choose_weapon_modifier(pl, OBJ_SHOT);
                    Fire_normal_shots(pl);
                }
            }
        }
        /*-BA Be more agressive, esp if lots of ammo
         * if ((my_data->robot_count % 32) == 0)
         */
        else if ((my_data->robot_count % 32) < pl->item[ITEM_MINE] && !WITHIN(my_data->robot_count,
                                                                              my_data->last_dropped_mine, 10))
        {
            if (pl->fuel.sum > pl->fuel.l3)
            {
                Choose_weapon_modifier(pl, OBJ_MINE);
                Place_mine(pl);
            }
            else /*if (pl->fuel.sum < pl->fuel.l2)*/
            {
                Place_mine(pl);
                CLR_BIT(pl->used, USES_CLOAKING_DEVICE);
            }
            my_data->last_dropped_mine = my_data->robot_count;
        }
    }
    if (new_mode == RM_CANNON_KILL && !slowing)
    {
        if ((my_data->robot_count % 2) == 0 && item_dist < Visibility_distance && clear_path)
        {
            Choose_weapon_modifier(pl, OBJ_SHOT);
            Fire_normal_shots(pl);
        }
    }
    my_data->robot_mode = new_mode;
    return true;
}

static bool Check_robot_hunt(player_t *pl)
{
    player_t *ship;
    int ship_dir;
    int travel_dir;
    int delta_dir;
    int adj_dir;
    int toofast, tooslow;
    robot_default_data_t *my_data = Robot_default_get_data(pl);

    if (!BIT(my_data->robot_lock, LOCK_PLAYER) || my_data->robot_lock_id == pl->id)
        return false;
    if (pl->fuel.sum < pl->fuel.l3 /*MAX_PLAYER_FUEL/2*/)
        return false;
    ship = Player_by_id(my_data->robot_lock_id);
    if (!Detect_ship(pl, ship))
        return false;

    ship_dir = (int)Wrap_findDir(ship->pix_pos.x - pl->pix_pos.x, ship->pix_pos.y - pl->pix_pos.y);

    if (pl->velocity <= 0.2)
    {
        vector_t *grav = &world->gravity
                              [OBJ_X_IN_BLOCKS(pl)][OBJ_Y_IN_BLOCKS(pl)];
        travel_dir = (int)findDir(grav->x, grav->y);
    }
    else
    {
        travel_dir = (int)findDir(pl->vel.x, pl->vel.y);
    }

    delta_dir = MOD2(ship_dir - travel_dir, RES);
    tooslow = (pl->velocity < my_data->robot_attack_speed / 2);
    toofast = (pl->velocity > my_data->robot_attack_speed);

    if (!tooslow && !toofast && (delta_dir <= RES / 16 || delta_dir >= 15 * RES / 16))
    {

        pl->turnacc = 0;
        Thrust(pl, false);
        my_data->robot_mode = RM_ROBOT_IDLE;
        return true;
    }

    adj_dir = (delta_dir < RES / 2 ? RES / 4 : (-RES / 4));

    if (tooslow)
        adj_dir = adj_dir / 2; /* point forwards more */
    if (toofast)
        adj_dir = 3 * adj_dir / 2; /* point backwards more */

    adj_dir = MOD2(travel_dir + adj_dir, RES);
    delta_dir = MOD2(adj_dir - pl->dir, RES);

    if (delta_dir >= RES / 16 && delta_dir <= 15 * RES / 16)
    {
        pl->turnspeed = MAX_PLAYER_TURNSPEED / 4;
        pl->turnacc = (delta_dir < RES / 2 ? pl->turnspeed : (-pl->turnspeed));
    }

    if (delta_dir < RES / 8 || delta_dir > 7 * RES / 8)
        Thrust(pl, true);
    else
        Thrust(pl, false);

    my_data->robot_mode = RM_ROBOT_IDLE;
    return true;
}

static bool Detect_ship(player_t *pl, player_t *ship)
{
    int dx, dy;
    double distance;

    /* can't go after non-existing ships */
    if (!ship)
        return false;

    /* can't go after non-playing ships */
    if (!Player_is_alive(ship))
        return false;

    /* can't do anything with phased ships */
    if (Player_is_phasing(ship))
        return false;

    /* trivial */
    if (pl->visibility[GetInd(ship->id)].canSee)
        return true;

    /*
     * can't see it, so it must be cloaked
     * maybe we can detect it's presence from other clues?
     */
    dx = ship->pix_pos.x - pl->pix_pos.x, dx = WRAP_DX(dx);
    dy = ship->pix_pos.y - pl->pix_pos.y, dy = WRAP_DY(dy);
    if (sqr(dx) + sqr(dy) > sqr(Visibility_distance))
        return false; /* can't detect ships beyond visual range */

    if (BIT(ship->obj_status, THRUSTING) && options.cloakedExhaust)
        return true;

    if (BIT(ship->used, HAS_SHOT |
                            HAS_LASER |
                            HAS_REFUEL |
                            HAS_REPAIR |
                            HAS_CONNECTOR |
                            HAS_TRACTOR_BEAM))
        return true;

    if (BIT(ship->have, HAS_BALL))
        return true;

    /* the sky seems clear.. */
    return false;
}

/*
 * Determine how important an item is to a given player.
 * Return one of the following 3 values:
 */
#define ROBOT_MUST_HAVE_ITEM 2 /* must have */
#define ROBOT_HANDY_ITEM 1     /* handy */
#define ROBOT_IGNORE_ITEM 0    /* ignore */
/*
 */
static int Rank_item_value(player_t *pl, Item_t itemtype)
{
    // player_t *pl = PlayersArray[ind];

    if (itemtype == ITEM_AUTOPILOT)
        return ROBOT_IGNORE_ITEM; /* never useful for robots */
    if (pl->item[itemtype] >= world->items[itemtype].limit)
        return ROBOT_IGNORE_ITEM; /* already full */
    if ((IsDefensiveItem(itemtype) && CountDefensiveItems(pl) >= options.maxDefensiveItems) || (IsOffensiveItem(itemtype) && CountOffensiveItems(pl) >= options.maxOffensiveItems))
        return ROBOT_IGNORE_ITEM;
    if (itemtype == ITEM_FUEL)
    {
        if (pl->fuel.sum >= pl->fuel.max * 0.90)
        {
            return ROBOT_IGNORE_ITEM; /* already (almost) full */
        }
        else if (pl->fuel.sum <
                 (BIT(world->rules->mode, TIMING) ? pl->fuel.l1 : pl->fuel.l2))
        {
            return ROBOT_MUST_HAVE_ITEM; /* ahh fuel at last */
        }
        else
            return ROBOT_HANDY_ITEM;
    }
    if (BIT(world->rules->mode, TIMING))
    {
        switch (itemtype)
        {
        case ITEM_FUEL:             /* less refuel stops */
        case ITEM_REARSHOT:         /* shoot competitors behind you */
        case ITEM_AFTERBURNER:      /* the more speed the better */
        case ITEM_TRANSPORTER:      /* steal fuel when you overtake someone */
        case ITEM_MINE:             /* blows others off the track */
        case ITEM_ECM:              /* blinded players smash into walls */
        case ITEM_EMERGENCY_THRUST: /* makes you go really fast */
        case ITEM_EMERGENCY_SHIELD: /* could be useful when ramming */
            return ROBOT_MUST_HAVE_ITEM;
        case ITEM_WIDEANGLE:    /* not important in racemode */
        case ITEM_CLOAK:        /* not important in racemode */
        case ITEM_SENSOR:       /* who cares about seeing others? */
        case ITEM_TANK:         /* makes you heavier */
        case ITEM_MISSILE:      /* likely to hit self */
        case ITEM_LASER:        /* cost too much fuel */
        case ITEM_TRACTOR_BEAM: /* pushes/pulls owner off the track too */
        case ITEM_AUTOPILOT:    /* probably not useful */
        case ITEM_DEFLECTOR:    /* cost too much fuel */
        case ITEM_HYPERJUMP:    /* likely to end up in wrong place */
        case ITEM_PHASING:      /* robots don't know how to use them yet */
        case ITEM_MIRROR:       /* not important in racemode */
        case ITEM_ARMOR:        /* makes you heavier */
            return ROBOT_IGNORE_ITEM;
        default:
            break;
        }
    }
    else
    {
        switch (itemtype)
        {
        case ITEM_EMERGENCY_SHIELD:
        case ITEM_DEFLECTOR:
        case ITEM_ARMOR:
            if (BIT(pl->have, HAS_SHIELD))
                return ROBOT_HANDY_ITEM;
            else
                return ROBOT_MUST_HAVE_ITEM;

        case ITEM_REARSHOT:
        case ITEM_WIDEANGLE:
            if (options.maxPlayerShots <= 0 || options.shotLife <= 0 || !options.allowPlayerKilling)
                return ROBOT_HANDY_ITEM;
            else
                return ROBOT_MUST_HAVE_ITEM;

        case ITEM_MISSILE:
            if (options.maxPlayerShots <= 0 || options.shotLife <= 0 || !options.allowPlayerKilling)
                return ROBOT_IGNORE_ITEM;
            else
                return ROBOT_MUST_HAVE_ITEM;

        case ITEM_MINE:
        case ITEM_CLOAK:
            return ROBOT_MUST_HAVE_ITEM;

        case ITEM_LASER:
            if (options.allowPlayerKilling)
                return ROBOT_MUST_HAVE_ITEM;
            else
                return ROBOT_HANDY_ITEM;

        case ITEM_PHASING: /* robots don't know how to use them yet */
            return ROBOT_IGNORE_ITEM;

        default:
            break;
        }
    }
    return ROBOT_HANDY_ITEM;
}

static bool Ball_handler(player_t *pl)
{
    int i, closest_tr = NO_IND, closest_ntr = NO_IND;
    double closest_tr_dist = 1e19, closest_ntr_dist = 1e19, dist, dbdir, dtdir;
    int bdir, tdir;
    bool clear_path = true;
    robot_default_data_t *my_data = Robot_default_get_data(pl);

    for (i = 0; i < Num_treasures(); i++)
    {
        treasure_t *treasure = Treasure_by_index(i);

        if ((BIT(pl->have, HAS_BALL) || pl->ball) && treasure->team == pl->team)
        {
            dist = Wrap_length(treasure->pos.cx - pl->pos.cx,
                               treasure->pos.cy - pl->pos.cy) /
                   CLICK;
            if (dist < closest_tr_dist)
            {
                closest_tr = i;
                closest_tr_dist = dist;
            }
        }
        else if (treasure->team != pl->team && Team_by_index(treasure->team)->NumMembers > 0 && !BIT(pl->have, HAS_BALL) && !pl->ball && treasure->have)
        {
            dist = Wrap_length(treasure->pos.cx - pl->pos.cx,
                               treasure->pos.cy - pl->pos.cy) /
                   CLICK;
            if (dist < closest_ntr_dist)
            {
                closest_ntr = i;
                closest_ntr_dist = dist;
            }
        }
    }
    if (BIT(pl->have, HAS_BALL) || pl->ball)
    {
        ballobject_t *ball = NULL;
        int dist_np = INT_MAX;
        int xdist, ydist;
        int dx, dy;

        if (pl->ball)
            ball = pl->ball;
        else
        {
            for (i = 0; i < NumObjs; i++)
            {
                if (Obj[i]->type == OBJ_BALL && Obj[i]->id == pl->id)
                {
                    ball = BALL_PTR(Obj[i]);
                    break;
                }
            }
        }
        for (i = 0; i < NumPlayers; i++)
        {
            dist = (int)(LENGTH(ball->pix_pos.x - Player_by_index(i)->pix_pos.x,
                                ball->pix_pos.y - Player_by_index(i)->pix_pos.y));
            if (Player_by_index(i)->id != pl->id && (BIT(Player_by_index(i)->obj_status, PLAYING | PAUSE | GAME_OVER) == PLAYING) && dist < dist_np)
                dist_np = dist;
        }
        bdir = (int)findDir(ball->vel.x, ball->vel.y);
        tdir = (int)Wrap_findDir((world->treasures[closest_tr].blk_pos.bx + 0.5) * BLOCK_SZ - ball->pix_pos.x,
                                 (world->treasures[closest_tr].blk_pos.by + 0.5) * BLOCK_SZ - ball->pix_pos.y);
        xdist = (world->treasures[closest_tr].blk_pos.bx) - OBJ_X_IN_BLOCKS(ball);
        ydist = (world->treasures[closest_tr].blk_pos.by) - OBJ_Y_IN_BLOCKS(ball);
        for (dist = 0;
             clear_path && dist < (closest_tr_dist - BLOCK_SZ);
             dist += BLOCK_SZ / 2)
        {
            double fraction = (double)dist / closest_tr_dist;
            dx = (int)((fraction * xdist) + OBJ_X_IN_BLOCKS(ball));
            dy = (int)((fraction * ydist) + OBJ_Y_IN_BLOCKS(ball));
            if (BIT(world->rules->mode, WRAP_PLAY))
            {
                if (dx < 0)
                    dx += world->x;
                else if (dx >= world->x)
                    dx -= world->x;
                if (dy < 0)
                    dy += world->y;
                else if (dy >= world->y)
                    dy -= world->y;
            }
            if (dx < 0 || dx >= world->x || dy < 0 || dy >= world->y)
            {
                clear_path = false;
                continue;
            }
            if (!BIT(1U << world->block[dx][dy], SPACE_BLOCKS))
            {
                clear_path = false;
                continue;
            }
        }
        if (tdir == bdir && dist_np > closest_tr_dist && clear_path && sqr(ball->vel.x) + sqr(ball->vel.y) > 60)
        {
            Detach_ball(pl, NULL);
            CLR_BIT(pl->used, USES_CONNECTOR);
            my_data->last_thrown_ball = my_data->robot_count;
            CLR_BIT(my_data->longterm_mode, FETCH_TREASURE);
        }
        else
        {
            SET_BIT(my_data->longterm_mode, FETCH_TREASURE);
            return Check_robot_target(pl,
                                      Treasure_by_index(closest_tr)->pos,
                                      RM_NAVIGATE);
        }
    }
    else
    {
        double ball_dist, closest_ball_dist = closest_ntr_dist;
        int closest_ball = NO_IND;

        for (i = 0; i < NumObjs; i++)
        {
            if (Obj[i]->type == OBJ_BALL)
            {
                ballobject_t *ball = BALL_IND(i);

                if ((ball->id == NO_ID)
                        ? (ball->ball_owner != NO_ID)
                        : (Player_by_id(ball->id)->team != pl->team))
                {
                    ball_dist = LENGTH(pl->pos.cx - ball->pos.cx,
                                       pl->pos.cy - ball->pos.cy) /
                                CLICK;
                    if (ball_dist < closest_ball_dist)
                    {
                        closest_ball_dist = ball_dist;
                        closest_ball = i;
                    }
                }
            }
        }
        if (closest_ball == NO_IND && closest_ntr_dist < (my_data->robot_count / 10) * BLOCK_SZ)
        {
            SET_BIT(my_data->longterm_mode, FETCH_TREASURE);
            return Check_robot_target(pl,
                                      Treasure_by_index(closest_ntr)->pos,
                                      RM_NAVIGATE);
        }
        else if (closest_ball_dist < (my_data->robot_count / 10) * BLOCK_SZ && closest_ball_dist > options.ballConnectorLength)
        {
            SET_BIT(my_data->longterm_mode, FETCH_TREASURE);
            return Check_robot_target(pl,
                                      Obj[closest_ball]->pos,
                                      RM_NAVIGATE);
        }
    }
    return false;
}

static int Robot_default_play_check_map(player_t *pl)
{
    // player_t *pl = PlayersArray[ind];
    int j;
    int cannon_i, fuel_i, target_i;
    int dx, dy;
    int distance, cannon_dist, fuel_dist, target_dist;
    bool fuel_checked;
    robot_default_data_t *my_data = Robot_default_get_data(pl);

    fuel_checked = false;

    cannon_i = -1;
    cannon_dist = Visibility_distance;
    fuel_i = -1;
    fuel_dist = Visibility_distance;
    target_i = -1;
    target_dist = Visibility_distance;

    for (j = 0; j < Num_fuels(); j++)
    {
        fuel_t *fs = Fuel_by_index(j);

        if (fs->fuel < 100.0)
            continue;

        if (BIT(world->rules->mode, TEAM_PLAY) && options.teamFuel && fs->team != pl->team)
            continue;

        if ((dx = (fs->pix_pos.x - pl->pix_pos.x),
             dx = WRAP_DX(dx), ABS(dx)) < fuel_dist &&
            (dy = (fs->pix_pos.y - pl->pix_pos.y),
             dy = WRAP_DY(dy), ABS(dy)) < fuel_dist &&
            (distance = (int)LENGTH(dx, dy)) < fuel_dist)
        {
            if (world->block[fs->blk_pos.bx]
                            [fs->blk_pos.by] == FUEL)
            {
                fuel_i = j;
                fuel_dist = distance;
            }
        }
    }

    for (j = 0; j < Num_targets(); j++)
    {
        target_t *targ = Target_by_index(j);

        /* Ignore dead or owned targets */
        if (targ->dead_time > 0 || pl->team == targ->team || Team_by_index(targ->team)->NumMembers == 0)
            continue;

        if ((dx = targ->blk_pos.bx * BLOCK_SZ + BLOCK_SZ / 2 - pl->pix_pos.x,
             dx = WRAP_DX(dx), ABS(dx)) < target_dist &&
            (dy = targ->blk_pos.by * BLOCK_SZ + BLOCK_SZ / 2 - pl->pix_pos.y,
             dy = WRAP_DY(dy), ABS(dy)) < target_dist &&
            (distance = (int)LENGTH(dx, dy)) < target_dist)
        {
            target_i = j;
            target_dist = distance;
        }
    }

#if 0
    if (fuel_i != NO_IND)
    warn("Closest fuel   = %d, distance = %.2f", fuel_i, fuel_dist);
    if (target_i != NO_IND)
    warn("Closest target = %d, distance = %.2f", target_i, target_dist);
#endif

    if (fuel_i != NO_IND && (target_dist > fuel_dist || !BIT(world->rules->mode, TEAM_PLAY)) && BIT(my_data->longterm_mode, NEED_FUEL))
    {
        fuel_checked = true;
        SET_BIT(pl->used, USES_REFUEL);
        pl->fs = fuel_i;

        if (Check_robot_target(pl, Fuel_by_index(fuel_i)->pos,
                               RM_REFUEL))
            return 1;
    }
    if (target_i != NO_IND)
    {
        SET_BIT(my_data->longterm_mode, TARGET_KILL);
        if (Check_robot_target(pl, Target_by_index(target_i)->pos,
                               RM_CANNON_KILL))
            return 1;

        CLR_BIT(my_data->longterm_mode, TARGET_KILL);
    }

    for (j = 0; j < Num_cannons(); j++)
    {
        cannon_t *cannon = Cannon_by_index(j);

        if (cannon->dead_time > 0)
            continue;

        if (BIT(world->rules->mode, TEAM_PLAY) && cannon->team == pl->team)
            continue;

        if ((dx = cannon->pix_pos.x - pl->pix_pos.x,
             dx = WRAP_DX(dx), ABS(dx)) < cannon_dist &&
            (dy = cannon->pix_pos.y - pl->pix_pos.y,
             dy = WRAP_DY(dy), ABS(dy)) < cannon_dist &&
            (distance = (int)LENGTH(dx, dy)) < cannon_dist)
        {
            cannon_i = j;
            cannon_dist = distance;
        }
    }

#if 0
    if (cannon_i != NO_IND)
    warn("Closest cannon = %d, distance = %.2f", cannon_i, cannon_dist);
#endif

    if (cannon_i != NO_IND)
    {
        cannon_t *cannon = Cannon_by_index(cannon_i);
        clpos_t d = cannon->pos;

        d.cx += (click_t)(BLOCK_CLICKS * 0.1 * tcos(cannon->dir));
        d.cy += (click_t)(BLOCK_CLICKS * 0.1 * tsin(cannon->dir));

        if (Check_robot_target(pl, d, RM_CANNON_KILL))
            return 1;
    }

    if (fuel_i != NO_IND && !fuel_checked && BIT(my_data->longterm_mode, NEED_FUEL))
    {
        SET_BIT(pl->used, USES_REFUEL);
        pl->fs = fuel_i;

        if (Check_robot_target(pl, Fuel_by_index(fuel_i)->pos,
                               RM_REFUEL))
            return 1;
    }

    return 0;
}

static void Robot_default_play_check_objects(player_t *pl,
                                             int *item_i, double *item_dist,
                                             int *item_imp,
                                             int *mine_i, double *mine_dist)
{
    int j, obj_count;
    object_t *shot, **obj_list;
    int distance;
    int dx, dy;
    int shield_range;
    long killing_shots;
    robot_default_data_t *my_data = Robot_default_get_data(pl);

    /*-BA Neural overload - if NumObjs too high, only consider
     *-BA max_objs many objects - improves performance under nukes
     *-BA 1000 is a fairly arbitrary choice.  If you wish to tune it,
     *-BA take into account the following.  A 4 mine cluster nuke produces
     *-BA about 4000 short lived objects.  An 8 mine cluster nuke produces
     *-BA about 14000 short lived objects.  By default, there is a limit
     *-BA of about 16000 objects.  Each player/robot produces between
     *-BA 20 and 40 objects just thrusting, and up to perhaps another 100
     *-BA by firing.  If the number is set too low the robots just fly
     *-BA around with thier shields on looking stupid and not doing
     *-BA much.  If too high, your system will slow down too much when
     *-BA the cluster nukes start going off.
     */
    const int max_objs = 1000;

    killing_shots = KILLING_SHOTS;
    if (options.treasureCollisionMayKill)
        killing_shots |= OBJ_BALL_BIT;
    if (options.wreckageCollisionMayKill)
        killing_shots |= OBJ_WRECKAGE_BIT;
    if (options.asteroidCollisionMayKill)
        killing_shots |= OBJ_ASTEROID_BIT;

    Cell_get_objects(pl->pos,
                     (int)(Visibility_distance / BLOCK_SZ), max_objs,
                     &obj_list, &obj_count);

    for (j = 0; j < obj_count; j++)
    {

        shot = obj_list[j];

        /* Get rid of the most common object types first for speed. */
        if (shot->type == OBJ_SPARK || shot->type == OBJ_DEBRIS)
            continue;

        dx = WRAP_DX(shot->pix_pos.x - pl->pix_pos.x);
        dy = WRAP_DY(shot->pix_pos.y - pl->pix_pos.y);

        if (shot->type == OBJ_BALL && !WITHIN(my_data->last_thrown_ball,
                                              my_data->robot_count,
                                              3 * FPS))
            SET_BIT(pl->used, USES_CONNECTOR);

        /* Ignore shots if shields already up - nothing else to do anyway */
        if ((shot->type == OBJ_SHOT || shot->type == OBJ_CANNON_SHOT) && BIT(pl->used, HAS_SHIELD))
            continue;

        /*
         * The only thing left to do regarding objects is to check if
         * this robot needs to put up shields to protect against objects.
         */
        if (!BIT(OBJ_TYPEBIT(shot->type), killing_shots))
        {
            /* Find closest item */
            if (shot->type == OBJ_ITEM)
            {
                object_t *item = shot;

                if (ABS(dx) < *item_dist && ABS(dy) < *item_dist)
                {
                    int imp;

                    if (BIT(item->obj_status, RANDOM_ITEM))
                        /* It doesn't know what it is, so get it if it can */
                        imp = ROBOT_HANDY_ITEM;
                    else
                        imp = Rank_item_value(pl, (Item_t)obj_list[j]->info);
                    if (imp > ROBOT_IGNORE_ITEM && imp >= *item_imp)
                    {
                        *item_imp = imp;
                        *item_dist = (int)LENGTH(dx, dy);
                        *item_i = j;
                    }
                }
            }

            continue;
        }

        /* Any shot of team members excluding self are passive. */
        if (Team_immune(shot->id, pl->id))
            continue;

        /* Self shots may be passive too... */
        if (shot->id == pl->id && options.selfImmunity)
            continue;

        /* Find nearest missile/mine */
        if (BIT(OBJ_TYPEBIT(shot->type), OBJ_TORPEDO_BIT | OBJ_SMART_SHOT_BIT | OBJ_ASTEROID_BIT | OBJ_HEAT_SHOT_BIT | OBJ_BALL_BIT | OBJ_CANNON_SHOT_BIT) || (shot->type == OBJ_SHOT && !BIT(world->rules->mode, TIMING) && shot->id != pl->id && shot->id != NO_ID) || (shot->type == OBJ_MINE && shot->id != pl->id) || (shot->type == OBJ_WRECKAGE && !BIT(world->rules->mode, TIMING)))
        {
            if (ABS(dx) < *mine_dist && ABS(dy) < *mine_dist && (distance = LENGTH(dx, dy)) < *mine_dist)
            {
                *mine_i = j;
                *mine_dist = distance;
            }
            if ((dx = ((shot->pix_pos.x - pl->pix_pos.x) + (shot->vel.x - pl->vel.x)),
                 dx = WRAP_DX(dx), ABS(dx)) < *mine_dist &&
                (dy = ((shot->pix_pos.y - pl->pix_pos.y) + (shot->vel.y - pl->vel.y)),
                 dy = WRAP_DY(dy), ABS(dy)) < *mine_dist &&
                (distance = LENGTH(dx, dy)) < *mine_dist)
            {
                *mine_i = j;
                *mine_dist = distance;
            }
        }

        shield_range = 21 + SHIP_SZ + shot->pl_range;

        if ((dx = (shot->pix_pos.x + shot->vel.x - (pl->pix_pos.x + pl->vel.x)),
             dx = WRAP_DX(dx),
             ABS(dx)) < shield_range &&
            (dy = (shot->pix_pos.y + shot->vel.y - (pl->pix_pos.y + pl->vel.y)),
             dy = WRAP_DY(dy),
             ABS(dy)) < shield_range &&
            sqr(dx) + sqr(dy) <= sqr(shield_range) && (int)(rfrac() * 100) < (85 + (my_data->defense / 7) - (my_data->attack / 50)))
        {
            SET_BIT(pl->used, HAS_SHIELD);
            if (!options.cloakedShield)
                CLR_BIT(pl->used, USES_CLOAKING_DEVICE);
            Thrust(pl, true);

            if (BIT(OBJ_TYPEBIT(shot->type), OBJ_TORPEDO_BIT | OBJ_SMART_SHOT_BIT | OBJ_ASTEROID_BIT | OBJ_HEAT_SHOT_BIT | OBJ_MINE_BIT) && (pl->fuel.sum < pl->fuel.l3 || !BIT(pl->have, HAS_SHIELD)))
            {
                if (pl->item[ITEM_HYPERJUMP] > 0 && pl->fuel.sum > -ED_HYPERJUMP)
                {
                    pl->item[ITEM_HYPERJUMP]--;
                    Player_add_fuel(pl, ED_HYPERJUMP);
                    do_hyperjump(pl);
                    break;
                }
            }
        }
        if (shot->type == OBJ_SMART_SHOT)
        {
            if (*mine_dist < ECM_DISTANCE / 4)
                Fire_ecm(pl);
        }
        if (shot->type == OBJ_MINE)
        {
            if (*mine_dist < ECM_DISTANCE / 2)
                Fire_ecm(pl);
        }
        if (shot->type, OBJ_HEAT_SHOT)
        {
            Thrust(pl, false);
            if (pl->fuel.sum < pl->fuel.l3 && pl->fuel.sum > pl->fuel.l1 && pl->fuel.num_tanks > 0)
            {
                Tank_handle_detach(pl);
            }
        }
        if (shot->type == OBJ_ASTEROID)
        {
            int delta_dir = 0;
            if (*mine_dist > (WIRE_PTR(shot)->wire_size == 1 ? 2 : 4) * BLOCK_SZ && *mine_dist < 8 * BLOCK_SZ && (delta_dir = (pl->dir - Wrap_findDir(shot->pix_pos.x - pl->pix_pos.x, shot->pix_pos.y - pl->pix_pos.y)) < WIRE_PTR(shot)->wire_size * (RES / 10) || delta_dir > RES - WIRE_PTR(shot)->wire_size * (RES / 10)))
            {
                SET_BIT(pl->used, HAS_SHOT);
            }
        }
    }

    /* Convert *item_i from index in local obj_list[] to index in Obj[] */
    if (*item_i >= 0)
    {
        for (j = 0;
             (j < NumObjs) && (Obj[j]->id != obj_list[*item_i]->id);
             j++)
            ;
        if (j >= NumObjs)
            /* Perhaps an error should be printed, too? */
            *item_i = NO_IND;
        else
            *item_i = j;
    }
}

static void Robot_default_play_check_lasers(player_t *pl)
{
    // player_t *pl = PlayersArray[ind];
    int j;
    int dx, dy;
    int distance2;
    /* int                                shield_range; */
    robot_default_data_t *my_data = Robot_default_get_data(pl);

    /*
     * Test if others are firing lasers at us.
     * Maybe move this into the player loop.
     */
    if (BIT(pl->used, HAS_SHIELD) == 0 && BIT(pl->have, HAS_SHIELD) != 0)
    {
        /* shield_range = 21 + SHIP_SZ; */
        for (j = 0; j < NumPulses; j++)
        {
            pulse_t *pulse = Pulses[j];
            if (pulse->id == pl->id && !pulse->refl)
                continue;
            if (Team_immune(pulse->id, pl->id))
                continue;
            if (pl->id == pulse->id && options.selfImmunity)
                continue;
            dx = (long)WRAP_DX(pl->pix_pos.x - pulse->pix_pos.x);
            dy = (long)WRAP_DY(pl->pix_pos.y - pulse->pix_pos.y);
            distance2 = sqr(dx) + sqr(dy);
            if ((distance2 < sqr(PULSE_LENGTH) || (distance2 < sqr(2 * PULSE_LENGTH) && ABS(findDir(dx, dy) - pulse->dir) < RES / 8)) && (int)(rfrac() * 100) <
                                                                                                                                             (85 + (my_data->defense / 7) - (my_data->attack / 50)))
            {
                SET_BIT(pl->used, HAS_SHIELD);
                if (!options.cloakedShield)
                    CLR_BIT(pl->used, USES_CLOAKING_DEVICE);
                break;
            }
        }
    }
}

static void Robot_default_play(player_t *pl)
{
    player_t *ship;
    double distance, ship_dist, enemy_dist, speed, x_speed, y_speed;
    double item_dist, mine_dist, shoot_time;
    int j, ship_i, item_imp, enemy_i, item_i, mine_i;
    bool harvest_checked, evade_checked, navigate_checked;
    robot_default_data_t *my_data = Robot_default_get_data(pl);
    itemobject_t *item = NULL;

    if (my_data->robot_count <= 0)
        my_data->robot_count = 1000 + (int)(rfrac() * 32);

    my_data->robot_count--;

    CLR_BIT(pl->used, USES_SHOT | USES_SHIELD | USES_CLOAKING_DEVICE | USES_LASER);
    if (BIT(pl->have, HAS_EMERGENCY_SHIELD) && !BIT(pl->used, HAS_EMERGENCY_SHIELD))
        Emergency_shield(pl, true);
    harvest_checked = false;
    evade_checked = false;
    navigate_checked = false;

    mine_i = NO_IND;
    mine_dist = SHIP_SZ + 200;
    item_i = NO_IND;
    item_dist = Visibility_distance;
    item_imp = ROBOT_IGNORE_ITEM;

    if (Player_has_cloaking_device(pl) && pl->fuel.sum > pl->fuel.l2)
        SET_BIT(pl->used, USES_CLOAKING_DEVICE);

    if (Player_has_emergency_thrust(pl) && !Player_uses_emergency_thrust(pl))
        Emergency_thrust(pl, true);

    if (Player_has_deflector(pl) && !BIT(world->rules->mode, TIMING))
        Deflector(pl, true);

    if (pl->fuel.sum <= (BIT(world->rules->mode, TIMING) ? 0 : pl->fuel.l1))
    {
        if (!BIT(pl->obj_status, SELF_DESTRUCT))
        {
            SET_BIT(pl->obj_status, SELF_DESTRUCT);
            pl->count = 150;
        }
    }
    else
    {
        CLR_BIT(pl->obj_status, SELF_DESTRUCT);
    }

    /* blinded by ECM. since we're not supposed to see anything,
       put up shields and return */
    if (pl->damaged > 0)
    {
        SET_BIT(pl->used, HAS_SHIELD);
        if (!options.cloakedShield)
            CLR_BIT(pl->used, USES_CLOAKING_DEVICE);
        return;
    }

    if (pl->fuel.sum < pl->fuel.max * 0.80)
        for (j = 0; j < Num_fuels(); j++)
        {
            int dx, dy;
            if (BIT(world->rules->mode, TEAM_PLAY) && options.teamFuel && world->fuels[j].team != pl->team)
            {
                continue;
            }
            dx = (int)(world->fuels[j].pix_pos.x - pl->pix_pos.x);
            dy = (int)(world->fuels[j].pix_pos.y - pl->pix_pos.y);
            /* dx = WRAP_DX(dx);
               dy = WRAP_DY(dy); */
            if (sqr(dx) + sqr(dy) <= sqr(90) && world->fuels[j].fuel > REFUEL_RATE)
            {
                pl->fs = j;
                SET_BIT(pl->used, USES_REFUEL);
                break;
            }
            else
                CLR_BIT(pl->used, USES_REFUEL);
        }

    /* don't turn NEED_FUEL off until refueling stops */
    if (pl->fuel.sum < (BIT(world->rules->mode, TIMING) ? pl->fuel.l1 : pl->fuel.l3))
        SET_BIT(my_data->longterm_mode, NEED_FUEL);
    else if (!Player_is_refueling(pl))
        CLR_BIT(my_data->longterm_mode, NEED_FUEL);

    if (BIT(world->rules->mode, TEAM_PLAY))
    {
        for (j = 0; j < Num_targets(); j++)
        {
            if (world->targets[j].team == pl->team && world->targets[j].damage < TARGET_DAMAGE && world->targets[j].dead_time >= 0)
            {
                int dx = (world->targets[j].blk_pos.bx * BLOCK_SZ + BLOCK_SZ / 2) - pl->pix_pos.x;
                int dy = (world->targets[j].blk_pos.by * BLOCK_SZ + BLOCK_SZ / 2) - pl->pix_pos.y;
                /* dx = WRAP_DX(dx);
                   dy = WRAP_DY(dy); */
                if (sqr(dx) + sqr(dy) <= sqr(90))
                {
                    pl->repair_target = j;
                    SET_BIT(pl->used, USES_REPAIR);
                    break;
                }
            }
        }
    }

    Robot_default_play_check_objects(pl,
                                     &item_i, &item_dist, &item_imp,
                                     &mine_i, &mine_dist);

    Robot_default_play_check_lasers(pl);

    if (item_i >= 0)
        item = ITEM_PTR(Obj[item_i]);

    /* make sure robots take off from their bases */
    if (QUICK_LENGTH(pl->pos.cx - pl->home_base->pos.cx,
                     pl->pos.cy - pl->home_base->pos.cy) < BLOCK_CLICKS)
        Thrust(pl, true);

    ship_i = -1;
    ship_dist = SHIP_SZ * 6;
    enemy_i = -1;
    if (pl->fuel.sum > pl->fuel.l3)
    {
        enemy_dist = (BIT(world->rules->mode, LIMITED_VISIBILITY) ? MAX(pl->fuel.sum * ENERGY_RANGE_FACTOR,
                                                                        Visibility_distance)
                                                                  : Max_enemy_distance);
    }
    else
        enemy_dist = Visibility_distance;

    if (BIT(pl->used, HAS_SHIELD))
        ship_dist = 0;

    if (BIT(my_data->robot_lock, LOCK_PLAYER))
    {
        ship = Player_by_id(my_data->robot_lock_id);

        if (BIT(Player_by_id(my_data->robot_lock_id)->obj_status,
                PLAYING | GAME_OVER | PAUSE) == PLAYING)
        {
            if (Detect_ship(pl, ship))
            {
                if (BIT(my_data->robot_lock, LOCK_PLAYER) && my_data->robot_lock_id == ship->id)
                {
                    my_data->lock_last_seen = my_data->robot_count;
                    my_data->lock_last_pos.x = ship->pix_pos.x;
                    my_data->lock_last_pos.y = ship->pix_pos.y;
                }

                distance = Wrap_length(ship->pos.cx - pl->pos.cx,
                                       ship->pos.cy - pl->pos.cy) /
                           CLICK;

                if (distance < ship_dist)
                {
                    ship_i = GetInd(my_data->robot_lock_id);
                    ship_dist = distance;
                }

                if (distance < enemy_dist)
                {
                    enemy_i = j;
                    enemy_dist = distance;
                }
            }
        }
    }

    if (ship_i == NO_IND || enemy_i == NO_IND)
    {

        for (j = 0; j < NumPlayers; j++)
        {

            ship = Player_by_index(j);
            if (ship->id == pl->id || BIT(ship->obj_status, PLAYING | GAME_OVER | PAUSE) != PLAYING || Team_immune(pl->id, ship->id))
                continue;

            if (!Detect_ship(pl, ship))
                continue;

            distance = Wrap_length(ship->pos.cx - pl->pos.cx,
                                   ship->pos.cy - pl->pos.cy) /
                       CLICK;

            if (distance < ship_dist)
            {
                ship_i = j;
                ship_dist = distance;
            }

            if (!BIT(my_data->robot_lock, LOCK_PLAYER))
            {
                if ((my_data->robot_count % 3) == 0 && ((my_data->robot_count % 100) < my_data->attack) && distance < enemy_dist)
                {
                    enemy_i = j;
                    enemy_dist = distance;
                }
            }
        }
    }

    if (ship_dist < 3 * SHIP_SZ && BIT(pl->have, HAS_SHIELD))
    {
        SET_BIT(pl->used, HAS_SHIELD);
        if (!options.cloakedShield)
        {
            CLR_BIT(pl->used, USES_CLOAKING_DEVICE);
        }
    }

    if (ship_dist <= 10 * BLOCK_SZ && pl->fuel.sum <= pl->fuel.l3 && !BIT(world->rules->mode, TIMING))
    {
        if (pl->item[ITEM_HYPERJUMP] > 0 && pl->fuel.sum > -ED_HYPERJUMP)
        {
            pl->item[ITEM_HYPERJUMP]--;
            Player_add_fuel(pl, ED_HYPERJUMP);
            do_hyperjump(pl);
            return;
        }
    }

    if (ship_i != NO_IND && BIT(my_data->robot_lock, LOCK_PLAYER) && my_data->robot_lock_id == Player_by_index(ship_i)->id)
        ship_i = NO_IND; /* don't avoid target */

    if (enemy_i >= 0)
    {
        ship = Player_by_index(enemy_i);
        // Guard against ship = (nil)
        if (!pl)
            warn("enemy: pl = %p", pl);
        if (!ship)
            warn("enemy: ship = %p", ship);

        player_t *locked_pl = Player_by_id(pl->lock.pl_id);
        if (!locked_pl)
            warn("enemy: locked_pl = %p", locked_pl);

        if (!BIT(pl->lock.tagged, LOCK_PLAYER) ||
            (enemy_dist < pl->lock.distance / 2 && (BIT(world->rules->mode, TIMING) ? (ship->check >= pl->check && ship->round >= pl->round) : 1)) ||
            (enemy_dist < pl->lock.distance * 2 && BIT(world->rules->mode, TEAM_PLAY) && BIT(ship->have, HAS_BALL)) ||
            (locked_pl != NULL && ship->score > Player_by_id(pl->lock.pl_id)->score))
        {
            pl->lock.pl_id = ship->id;
            SET_BIT(pl->lock.tagged, LOCK_PLAYER);
            pl->lock.distance = enemy_dist;
            Compute_sensor_range(pl);
        }
    }

    if (BIT(pl->lock.tagged, LOCK_PLAYER))
    {
        int delta_dir;

        ship = Player_by_id(pl->lock.pl_id);
        // Guard against ship = (nil)
        if (!pl || !ship)
            warn("pl = %p, ship = %p", pl, ship);
        if (ship)
        {
            delta_dir = (int)(pl->dir - Wrap_findDir(ship->pix_pos.x - pl->pix_pos.x,
                                                     ship->pix_pos.y - pl->pix_pos.y));
            delta_dir = MOD2(delta_dir, RES);
            if (BIT(ship->obj_status, PLAYING | PAUSE | GAME_OVER) != PLAYING ||
                (BIT(my_data->robot_lock, LOCK_PLAYER) && my_data->robot_lock_id != pl->lock.pl_id && BIT(Player_by_id(my_data->robot_lock_id)->obj_status, PLAYING | PAUSE | GAME_OVER) == PLAYING) || !Detect_ship(pl, ship) || (pl->fuel.sum <= pl->fuel.l3 && !BIT(world->rules->mode, TIMING)) || (BIT(world->rules->mode, TIMING) && (delta_dir < 3 * RES / 4 || delta_dir > RES / 4)) || Team_immune(pl->id, ship->id))
            {
                /* unset the player lock */
                CLR_BIT(pl->lock.tagged, LOCK_PLAYER);
                pl->lock.pl_id = 1;
                pl->lock.distance = 0;
            }
        }
    }
    if (!evade_checked)
    {
        if (Check_robot_evade(pl, mine_i, ship_i))
        {
            if (options.playerShielding == 0 && options.playerStartsShielded != 0 && BIT(pl->have, HAS_SHIELD))
            {
                SET_BIT(pl->used, HAS_SHIELD);
                if (!options.cloakedShield)
                    CLR_BIT(pl->used, USES_CLOAKING_DEVICE);
            }
            else if (options.maxShieldedWallBounceSpeed >
                         options.maxUnshieldedWallBounceSpeed &&
                     options.maxShieldedWallBounceAngle >=
                         options.maxUnshieldedWallBounceAngle &&
                     BIT(pl->have, HAS_SHIELD))
            {
                SET_BIT(pl->used, HAS_SHIELD);
                if (!options.cloakedShield)
                    CLR_BIT(pl->used, USES_CLOAKING_DEVICE);
            }
            return;
        }
    }
    if (BIT(world->rules->mode, TIMING) && !navigate_checked)
    {
        int delta_dir;
        if (item_i >= 0)
        {
            delta_dir = (int)(pl->dir - Wrap_findDir(Obj[item_i]->pix_pos.x - pl->pix_pos.x,
                                                     Obj[item_i]->pix_pos.y - pl->pix_pos.y));
            delta_dir = MOD2(delta_dir, RES);
        }
        else
        {
            delta_dir = RES;
            item_imp = ROBOT_IGNORE_ITEM;
        }
        if ((item_imp == ROBOT_MUST_HAVE_ITEM && item_dist > 4 * BLOCK_SZ) || (item_imp == ROBOT_HANDY_ITEM && item_dist > 2 * BLOCK_SZ) || (item_imp == ROBOT_IGNORE_ITEM) || (delta_dir < 3 * RES / 4 && delta_dir > RES / 4))
        {
            navigate_checked = true;
            if (Check_robot_target(pl, Check_by_index(pl->check)->pos,
                                   RM_NAVIGATE))
                return;
        }
    }
    if (item != NULL && 3 * enemy_dist > 2 * item_dist && item_dist < 12 * BLOCK_SZ && !BIT(my_data->longterm_mode, FETCH_TREASURE) && (!BIT(my_data->longterm_mode, NEED_FUEL) || Obj[item_i]->info == ITEM_FUEL || Obj[item_i]->info == ITEM_TANK))
    {

        if (item_imp != ROBOT_IGNORE_ITEM)
        {
            clpos_t d = item->pos;

            harvest_checked = true;
            d.cx += (int)(item->vel.x * (ABS(d.cx - pl->pos.cx) /
                                         my_data->robot_normal_speed));
            d.cy += (int)(item->vel.y * (ABS(d.cy - pl->pos.cy) /
                                         my_data->robot_normal_speed));
            if (Check_robot_target(pl, d, RM_HARVEST))
                return;
        }
    }

    player_t *locked_pl = Player_by_id(pl->lock.pl_id);
    if (!locked_pl)
        warn("robot: locked_pl = %p", locked_pl);

    if (BIT(pl->lock.tagged, LOCK_PLAYER) &&
        locked_pl != NULL &&
        Detect_ship(pl, locked_pl))
    {
        clpos_t d;

        ship = locked_pl;
        shoot_time = (int)(pl->lock.distance / (options.shotSpeed + 1));
        d.cx = (int)(ship->pos.cx + ship->vel.x * shoot_time * CLICK);
        d.cy = (int)(ship->pos.cy + ship->vel.y * shoot_time * CLICK);
        /*-BA Also allow for our own momentum. */
        d.cx -= (int)(pl->vel.x * shoot_time * CLICK);
        d.cy -= (int)(pl->vel.y * shoot_time * CLICK);

        if (Check_robot_target(pl, d, RM_ATTACK) && !BIT(my_data->longterm_mode, FETCH_TREASURE | TARGET_KILL | NEED_FUEL))
            return;
    }
    if (BIT(world->rules->mode, TEAM_PLAY) && Num_treasures() > 0 && world->teams[pl->team].NumTreasures > 0 && !navigate_checked && !BIT(my_data->longterm_mode, TARGET_KILL | NEED_FUEL))
    {
        navigate_checked = true;
        if (Ball_handler(pl))
            return;
    }
    if (item_i >= 0 && !harvest_checked && item_dist < 12 * BLOCK_SZ)
    {

        if (item_imp != ROBOT_IGNORE_ITEM)
        {
            clpos_t d = Obj[item_i]->pos;

            d.cx += (int)(Obj[item_i]->vel.x * (ABS(d.cx - pl->pos.cx) /
                                                my_data->robot_normal_speed));
            d.cy += (int)(Obj[item_i]->vel.y * (ABS(d.cy - pl->pos.cy) /
                                                my_data->robot_normal_speed));

            if (Check_robot_target(pl, d, RM_HARVEST))
                return;
        }
    }

    if (Check_robot_hunt(pl))
    {
        if (options.playerShielding == 0 && options.playerStartsShielded != 0 && BIT(pl->have, HAS_SHIELD))
        {
            SET_BIT(pl->used, HAS_SHIELD);
            if (!options.cloakedShield)
                CLR_BIT(pl->used, USES_CLOAKING_DEVICE);
        }
        return;
    }

    if (Robot_default_play_check_map(pl) == 1)
        return;

    if (options.playerShielding == 0 && options.playerStartsShielded != 0 && BIT(pl->have, HAS_SHIELD))
    {
        SET_BIT(pl->used, HAS_SHIELD);
        if (!options.cloakedShield)
            CLR_BIT(pl->used, USES_CLOAKING_DEVICE);
    }

    int x = OBJ_X_IN_BLOCKS(pl);
    int y = OBJ_Y_IN_BLOCKS(pl);
    x_speed = pl->vel.x - 2 * world->gravity[x][y].x;
    y_speed = pl->vel.y - 2 * world->gravity[x][y].y;

    if (y_speed < (-my_data->robot_normal_speed) || (my_data->robot_count % 64) < 32)
    {
        my_data->robot_mode = RM_ROBOT_CLIMB;
        pl->turnspeed = MAX_PLAYER_TURNSPEED / 2;
        pl->power = MAX_PLAYER_POWER / 2;
        if (ABS(pl->dir - RES / 4) > RES / 16)
            pl->turnacc = (pl->dir < RES / 4 || pl->dir >= 3 * RES / 4
                               ? pl->turnspeed
                               : (-pl->turnspeed));
        else
            pl->turnacc = 0.0;

        if (y_speed < my_data->robot_normal_speed / 2 && pl->velocity < my_data->robot_attack_speed)
            Thrust(pl, true);
        else if (y_speed > my_data->robot_normal_speed)
            Thrust(pl, false);
        return;
    }
    my_data->robot_mode = RM_ROBOT_IDLE;
    pl->turnspeed = MAX_PLAYER_TURNSPEED / 2;
    pl->turnacc = 0;
    pl->power = MAX_PLAYER_POWER / 2;
    Thrust(pl, false);
    speed = LENGTH(x_speed, y_speed);
    if (speed < my_data->robot_normal_speed / 2)
        Thrust(pl, true);
    else if (speed > my_data->robot_normal_speed)
        Thrust(pl, false);
}

/*
 * This is called each round.
 * It allows us to adjust our file local parameters.
 */
static void Robot_default_round_tick(void)
{
    double min_visibility = 256.0;
    double min_enemy_distance = 512.0;

    /* reduce visibility when there are a lot of robots. */
    Visibility_distance = min_visibility + (((VISIBILITY_DISTANCE - min_visibility) * (NUM_IDS - NumRobots)) / NUM_IDS);

    /* limit distance to allowable enemies. */
    Max_enemy_distance = world->hypotenuse;
    if (world->hypotenuse > Visibility_distance)
        Max_enemy_distance = min_enemy_distance + (((world->hypotenuse - min_enemy_distance) * (NUM_IDS - NumRobots)) / NUM_IDS);
}
