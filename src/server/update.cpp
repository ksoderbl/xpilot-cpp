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
#include <cstdio>
#include <cmath>

#include "click.h"
#include "commonproto.h"
#include "xperror.h"

#include "server.h"
#include "target.h"

#define SERVER
#include "xpconfig.h"
#include "serverconst.h"

#include "map.h"
#include "score.h"
#include "bit.h"
#include "saudio.h"
#include "object.h"
#include "cannon.h"
#include "asteroid.h"
#include "netserver.h"
#include "robot.h"
#include "walls.h"
#include "wormhole.h"

int roundtime = -1;               /* time left this round */
static double time_to_tick = 1.0; /* game time till next tick */
static bool tick = false;         /* new tick of game time this frame */

static inline void update_object_speed(object_t *obj)
{
    // if (BIT(obj->obj_status, GRAVITY))
    // {
    //     obj->vel.x += obj->acc.x + World.gravity[OBJ_X_IN_BLOCKS(obj)][OBJ_Y_IN_BLOCKS(obj)].x;
    //     obj->vel.y += obj->acc.y + World.gravity[OBJ_X_IN_BLOCKS(obj)][OBJ_Y_IN_BLOCKS(obj)].y;
    // }
    // else
    // {
    obj->vel.x += obj->acc.x;
    obj->vel.y += obj->acc.y;
    // }
}

static char msg[MSG_LEN];

static void Transport_to_home(player_t *pl)
{
    world_t *world = &World;
    /*
     * Transport a corpse from the place where it died back to its homebase,
     * or if in race mode, back to the last passed check point.
     *
     * During the first part of the distance we give it a positive constant
     * acceleration G, during the second part we make this a negative one -G.
     * This results in a visually pleasing take off and landing.
     */
    clpos_t startpos;
    double dx, dy, t, m;
    const double T = RECOVERY_DELAY;

    /*
    if (pl->home_base == NULL)
    {
        pl->vel.x = 0;
        pl->vel.y = 0;
        return;
    }
        */

    if (BIT(World.rules->mode, TIMING) && pl->round)
    {
        int check;

        if (pl->check)
            check = pl->check - 1;
        else
            check = Num_checks() - 1;
        startpos = Check_by_index(check)->pos;
    }
    else
        startpos = World.bases[pl->home_base_ind].pos;

    dx = WORLD_WRAP_DCX(world, startpos.cx - pl->pos.cx);
    dy = WORLD_WRAP_DCY(world, startpos.cy - pl->pos.cy);
    t = pl->recovery_count;
    if (2 * t <= T)
        m = 2 / t;
    else
    {
        t = T - t;
        m = (4 * t) / (T * T - 2 * t * t);
    }
    pl->vel.x = dx * m / CLICK;
    pl->vel.y = dy * m / CLICK;
}

/*
 * Turn phasing on or off.
 */
void Phasing(player_t *pl, bool on)
{
    if (on)
    {
        if (pl->phasing_left <= 0)
        {
            pl->phasing_left = PHASING_TIME;
            pl->item[ITEM_PHASING]--;
        }
        SET_BIT(pl->used, USES_PHASING_DEVICE);
        CLR_BIT(pl->used, USES_REFUEL);
        CLR_BIT(pl->used, USES_REPAIR);
        if (BIT(pl->used, USES_CONNECTOR))
            pl->ball = NULL;
        CLR_BIT(pl->used, USES_TRACTOR_BEAM);
        CLR_BIT(pl->obj_status, GRAVITY);
        sound_play_sensors(pl->pos, PHASING_ON_SOUND);
    }
    else
    {
        CLR_BIT(pl->used, USES_PHASING_DEVICE);
        if (pl->phasing_left <= 0)
        {
            if (pl->item[ITEM_PHASING] <= 0)
                CLR_BIT(pl->have, HAS_PHASING_DEVICE);
        }
        SET_BIT(pl->obj_status, GRAVITY);
        sound_play_sensors(pl->pos, PHASING_OFF_SOUND);
    }
}

/*
 * Turn cloak on or off.
 */
void Cloak(player_t *pl, bool on)
{
    if (on)
    {
        if (!BIT(pl->used, USES_CLOAKING_DEVICE) && pl->item[ITEM_CLOAK] > 0)
        {
            if (!options.cloakedShield)
            {
                if (BIT(pl->used, USES_EMERGENCY_SHIELD))
                    Emergency_shield(pl, false);
                if (BIT(pl->used, USES_DEFLECTOR))
                    Deflector(pl, false);
                CLR_BIT(pl->used, USES_SHIELD);
                CLR_BIT(pl->have, HAS_SHIELD);
            }
            sound_play_player(pl, CLOAK_SOUND);
            pl->updateVisibility = true;
            SET_BIT(pl->used, USES_CLOAKING_DEVICE);
        }
    }
    else
    {
        if (Player_is_cloaked(pl))
        {
            sound_play_player(pl, CLOAK_SOUND);
            pl->updateVisibility = true;
            CLR_BIT(pl->used, USES_CLOAKING_DEVICE);
        }
        if (!pl->item[ITEM_CLOAK])
            CLR_BIT(pl->have, HAS_CLOAKING_DEVICE);
        if (!options.cloakedShield)
        {
            if (BIT(pl->have, HAS_EMERGENCY_SHIELD))
            {
                SET_BIT(pl->have, HAS_SHIELD);
                Emergency_shield(pl, true);
            }
            if (BIT(DEF_HAVE, HAS_SHIELD) && !BIT(pl->have, HAS_SHIELD))
                SET_BIT(pl->have, HAS_SHIELD);
            if (BITV_ISSET(pl->last_keyv, KEY_SHIELD))
                SET_BIT(pl->used, HAS_SHIELD);
        }
    }
}

/*
 * Turn deflector on or off.
 */
void Deflector(player_t *pl, bool on)
{
    if (on)
    {
        if (!BIT(pl->used, USES_DEFLECTOR) && pl->item[ITEM_DEFLECTOR] > 0)
        {
            /* only allow deflector when cloaked shielding or not cloaked */
            if (options.cloakedShield || !BIT(pl->used, USES_CLOAKING_DEVICE))
            {
                SET_BIT(pl->used, USES_DEFLECTOR);
                sound_play_player(pl, DEFLECTOR_SOUND);
            }
        }
    }
    else
    {
        if (BIT(pl->used, USES_DEFLECTOR))
        {
            CLR_BIT(pl->used, USES_DEFLECTOR);
            sound_play_player(pl, DEFLECTOR_SOUND);
        }
        if (!pl->item[ITEM_DEFLECTOR])
            CLR_BIT(pl->have, HAS_DEFLECTOR);
    }
}

/*
 * Turn emergency thrust on or off.
 */
void Emergency_thrust(player_t *pl, bool on)
{
    if (on)
    {
        if (pl->emergency_thrust_left <= 0)
        {
            pl->emergency_thrust_left = EMERGENCY_THRUST_TIME;
            pl->item[ITEM_EMERGENCY_THRUST]--;
        }
        if (!Player_uses_emergency_thrust(pl))
        {
            SET_BIT(pl->used, USES_EMERGENCY_THRUST);
            sound_play_sensors(pl->pos, EMERGENCY_THRUST_ON_SOUND);
        }
    }
    else
    {
        if (Player_uses_emergency_thrust(pl))
        {
            CLR_BIT(pl->used, USES_EMERGENCY_THRUST);
            sound_play_sensors(pl->pos, EMERGENCY_THRUST_OFF_SOUND);
        }
        if (pl->emergency_thrust_left <= 0)
        {
            if (pl->item[ITEM_EMERGENCY_THRUST] <= 0)
                CLR_BIT(pl->have, HAS_EMERGENCY_THRUST);
        }
    }
}

/*
 * Turn emergency shield on or off.
 */
void Emergency_shield(player_t *pl, bool on)
{
    if (on)
    {
        if (BIT(pl->have, HAS_EMERGENCY_SHIELD))
        {
            if (pl->emergency_shield_left <= 0)
            {
                pl->emergency_shield_left = EMERGENCY_SHIELD_TIME;
                pl->item[ITEM_EMERGENCY_SHIELD]--;
            }
            if (options.cloakedShield || !BIT(pl->used, USES_CLOAKING_DEVICE))
            {
                SET_BIT(pl->have, HAS_SHIELD);
                if (!BIT(pl->used, USES_EMERGENCY_SHIELD))
                {
                    SET_BIT(pl->used, USES_EMERGENCY_SHIELD);
                    sound_play_sensors(pl->pos, EMERGENCY_SHIELD_ON_SOUND);
                }
            }
        }
    }
    else
    {
        if (pl->emergency_shield_left <= 0)
        {
            if (pl->item[ITEM_EMERGENCY_SHIELD] <= 0)
                CLR_BIT(pl->have, HAS_EMERGENCY_SHIELD);
        }
        if (!BIT(DEF_HAVE, HAS_SHIELD))
        {
            CLR_BIT(pl->have, HAS_SHIELD);
            CLR_BIT(pl->used, USES_SHIELD);
        }
        if (BIT(pl->used, USES_EMERGENCY_SHIELD))
        {
            CLR_BIT(pl->used, USES_EMERGENCY_SHIELD);
            sound_play_sensors(pl->pos, EMERGENCY_SHIELD_OFF_SOUND);
        }
    }
}

/*
 * Turn thrust on or off.
 */
void Thrust(player_t *pl, bool on)
{
    if (on)
        SET_BIT(pl->obj_status, THRUSTING);
    else
        CLR_BIT(pl->obj_status, THRUSTING);
}

/*
 * Turn autopilot on or off.  This always clears the thrusting bit.  During
 * automatic pilot mode any changes to the current power, turnacc, turnspeed
 * and turnresistance settings will be temporary.
 */
void Autopilot(player_t *pl, bool on)
{
    if (on)
    {
        Thrust(pl, false);
        pl->auto_power_s = pl->power;
        pl->auto_turnspeed_s = pl->turnspeed;
        pl->auto_turnresistance_s = pl->turnresistance;
        SET_BIT(pl->used, USES_AUTOPILOT);
        pl->power = (MIN_PLAYER_POWER + MAX_PLAYER_POWER) / 2.0;
        pl->turnspeed = (MIN_PLAYER_TURNSPEED + MAX_PLAYER_TURNSPEED) / 2.0;
        pl->turnresistance = 0.2;
        sound_play_sensors(pl->pos, AUTOPILOT_ON_SOUND);
    }
    else
    {
        Thrust(pl, false);
        pl->power = pl->auto_power_s;
        pl->turnacc = 0.0;
        pl->turnspeed = pl->auto_turnspeed_s;
        pl->turnresistance = pl->auto_turnresistance_s;
        CLR_BIT(pl->used, USES_AUTOPILOT);
        sound_play_sensors(pl->pos, AUTOPILOT_OFF_SOUND);
    }
}

/*
 * Automatic pilot will try to hold the ship steady, turn to face away
 * from direction of travel, if so then turn on thrust which will
 * cause the ship to come to a rest within a short period of time.
 * This code is fairly self contained.
 */
static void do_Autopilot(player_t *pl)
{
    int vad; /* Velocity Away Delta */
    int dir, afterburners;
    int ix, iy;
    double gx, gy;
    double acc, vel, delta, turnspeed, power, a;
    const double emergency_thrust_settings_delta = 150.0 / FPS;
    const double auto_pilot_settings_delta = 15.0 / FPS;
    const double auto_pilot_turn_factor = 2.5;
    const double auto_pilot_dead_velocity = 0.5;

    /*
     * If the last movement touched a wall then we shouldn't
     * mess with the position (speed too?) settings.
     */
    if (pl->last_wall_touch + 1 >= frame_loops)
        return;

    /*
     * Having more autopilot items or using emergency thrust causes a much
     * quicker deceleration to occur than during normal flight.  Having
     * no autopilot items will cause minimum delta to occur, this is because
     * the autopilot code is used by the pause code.
     */
    delta = auto_pilot_settings_delta;
    if (pl->item[ITEM_AUTOPILOT])
        delta *= pl->item[ITEM_AUTOPILOT];

    if (Player_uses_emergency_thrust(pl))
    {
        afterburners = MAX_AFTERBURNER;
        if (delta < emergency_thrust_settings_delta)
            delta = emergency_thrust_settings_delta;
    }
    else
        afterburners = pl->item[ITEM_AFTERBURNER];

    ix = OBJ_X_IN_BLOCKS(pl);
    iy = OBJ_Y_IN_BLOCKS(pl);
    gx = World.gravity[ix][iy].x;
    gy = World.gravity[ix][iy].y;

    /*
     * Due to rounding errors if the velocity is very small we were probably
     * on target to stop last time round, so we actually absolutely stop.
     * This enables the ship to orient away from gravity and set up the
     * thrust to counteract it.
     */
    if ((vel = VECTOR_LENGTH(pl->vel)) < auto_pilot_dead_velocity)
    {
        pl->vel.x = pl->vel.y = vel = 0.0;
        Player_position_restore(pl);
    }

    /*
     * Calculate power needed to change instantaneously to stopped.  We
     * must include gravity here for next time round the update loop.
     */
    acc = LENGTH(gx, gy) + vel;
    power = acc * pl->mass;
    if (afterburners)
        power /= AFTER_BURN_POWER_FACTOR(afterburners);

    /*
     * Calculate direction change needed to reduce velocity to zero.
     */
    if (vel == 0.0)
    {
        if (gx == 0 && gy == 0)
            vad = pl->dir;
        else
            vad = (int)findDir(-gx, -gy);
    }
    else
    {
        vad = (int)findDir(-pl->vel.x, -pl->vel.y);
    }
    vad = MOD2(vad - pl->dir, ANGLE_RESOLUTION);
    if (vad > ANGLE_RESOLUTION / 2)
    {
        vad = ANGLE_RESOLUTION - vad;
        dir = -1;
    }
    else
        dir = 1;

    /*
     * Calculate turnspeed needed to change direction instantaneously by
     * above direction change.
     */
    turnspeed = ((double)vad) / pl->turnresistance - pl->turnvel;
    if (turnspeed < 0)
    {
        turnspeed = -turnspeed;
        dir = -dir;
    }

    /*
     * Change the turnspeed setting towards the perfect value, and limit
     * to the maximum only (limiting to the minimum causes oscillation).
     */
    if (turnspeed < pl->turnspeed)
    {
        pl->turnspeed -= delta;
        if (turnspeed > pl->turnspeed)
            pl->turnspeed = turnspeed;
    }
    else if (turnspeed > pl->turnspeed)
    {
        pl->turnspeed += delta;
        if (turnspeed < pl->turnspeed)
            pl->turnspeed = turnspeed;
    }
    if (pl->turnspeed > MAX_PLAYER_TURNSPEED)
        pl->turnspeed = MAX_PLAYER_TURNSPEED;

    /*
     * Decide if its wise to turn this time.
     */
    if (pl->turnspeed > (turnspeed * auto_pilot_turn_factor))
    {
        pl->turnacc = 0.0;
        pl->turnvel = 0.0;
    }
    else
        pl->turnacc = dir * pl->turnspeed;

    /*
     * Change the power setting towards the perfect value, and limit
     * to the maximum only (limiting to the minimum causes oscillation).
     */
    if (power < pl->power)
    {
        pl->power -= delta;
        if (power > pl->power)
            pl->power = power;
    }
    else if (power > pl->power)
    {
        pl->power += delta;
        if (power < pl->power)
            pl->power = power;
    }
    if (pl->power > MAX_PLAYER_POWER)
        pl->power = MAX_PLAYER_POWER;

    /*
     * Don't thrust if the direction will not be absolutely correct and hasn't
     * been very close last time.  The latter clause was added such that
     * when a fine direction adjustment is needed, but the turnspeed is too
     * high at the moment, it gets the ship slowing down even though it
     * will impart some sideways velocity.
     */
    if (pl->turnspeed != turnspeed && vad > ANGLE_RESOLUTION / 32)
    {
        Thrust(pl, false);
        return;
    }

    /*
     * Only thrust if the power setting is correct or less than correct,
     * we don't want to over thrust.
     */
    if (pl->power > power)
        Thrust(pl, false);
    else
        Thrust(pl, true);
}

static void Fuel_update(void)
{
    int i;
    double fuel;
    int frames_per_update;

    if (NumPlayers == 0)
        return;

    // Let the fuel stations regenerate some fuel.
    fuel = (NumPlayers * STATION_REGENERATION * timeStep);
    frames_per_update = (int)(MAX_STATION_FUEL / (fuel * BLOCK_SZ));

    for (i = 0; i < Num_fuels(); i++)
    {
        fuel_t *fs = Fuel_by_index(i);

        if (fs->fuel == MAX_STATION_FUEL)
            continue;
        if ((fs->fuel += fuel) >= MAX_STATION_FUEL)
            fs->fuel = MAX_STATION_FUEL;
        else if (fs->last_change + frames_per_update > frame_loops)
            /*
             * We don't send fuelstation info to the clients every frame
             * if it wouldn't change their display.
             */
            continue;

        fs->conn_mask = 0;
        fs->last_change = frame_loops;
    }
}

/*
static void legacy_mode_ball_hack(ballobject_t *ball)
*/

static void Misc_object_update(void)
{
    int i;
    object_t *obj;

    for (i = 0; i < NumObjs; i++)
    {
        obj = Obj[i];

        if (obj->type == OBJ_MINE)
            Update_mine(MINE_PTR(obj));

        else if (obj->type == OBJ_TORPEDO)
            Update_torpedo(TORP_PTR(obj));

        else if (obj->type == OBJ_SMART_SHOT || obj->type == OBJ_HEAT_SHOT)
            Update_missile(MISSILE_PTR(obj));

        else if (obj->type == OBJ_BALL)
        {
            if (obj->id != NO_ID)
            {
                ballobject_t *ball = BALL_PTR(obj);

                Update_connector_force(ball);
            }
        }

        else if (obj->type == OBJ_WRECKAGE)
        {
            wireobject_t *wireobj = WIRE_PTR(obj);

            wireobj->wire_rotation =
                (wireobj->wire_rotation + (int)(wireobj->wire_turnspeed * timeStep * ANGLE_RESOLUTION)) % ANGLE_RESOLUTION;
        }

        update_object_speed(obj);

        if (!(obj->type == OBJ_ASTEROID))
            Move_object(obj);
    }
}

static void Ecm_update(void)
{
    int i;
    double ecmSizeFactor = 0.5;

    // if (Num_ecms() > 0)
    //     warn("Ecm_Update: ecms: %d", Num_ecms());

    // Update ECM blasts
    for (i = 0; i < Num_ecms(); i++)
    {
        ecm_t *ecm = Ecm_by_index(i);

        if ((ecm->size *= ecmSizeFactor) < 1.0)
        {
            if (ecm->id != NO_ID)
            {
                player_t *pl = Player_by_id(ecm->id);

                if (pl)
                    pl->ecmcount--;
            }
            // --World.NumEcms;
            // World.ecms[i] = World.ecms[World.NumEcms];
            World.ecms.erase(World.ecms.begin() + i);
            i--;
        }
    }
}

static void Transporter_update(void)
{
    int i;

    // if (Num_transporters() > 0)
    //     warn("Num_transporters: transporters: %d", Num_transporters());

    for (i = 0; i < Num_transporters(); i++)
    {
        transporter_t *trans = Transporter_by_index(i);

        if (--trans->count <= 0)
        {
            // --World.NumTransporters;
            // World.transporters[i] = World.transporters[World.NumTransporters];
            World.transporters.erase(World.transporters.begin() + i);
            i--;
        }
    }
}

static void Players_turn(void)
{
    int i;
    player_t *pl;
    double new_float_dir;
}

static void Use_items(player_t *pl)
{
    if (pl->shield_time > 0)
    {
        pl->shield_time = 0;
        if ((pl->shield_time -= timeStep) <= 0)
        {
            if (!BIT(pl->used, USES_EMERGENCY_SHIELD))
                CLR_BIT(pl->used, USES_SHIELD);
        }
        if (BIT(pl->used, USES_SHIELD) == 0)
        {
            /* BG 95/06/03: change test on "have" to "used". */
            if (!BIT(pl->used, USES_EMERGENCY_SHIELD))
                CLR_BIT(pl->have, HAS_SHIELD);
            pl->shield_time = 0;
        }
    }

    if (Player_is_phasing(pl))
    {
        if ((pl->phasing_left -= timeStep) <= 0)
        {
            pl->phasing_left = 0;
            if (pl->item[ITEM_PHASING] > 0)
                Phasing(pl, true);
            else
                Phasing(pl, false);
        }
    }

    if (Player_uses_emergency_thrust(pl))
    {
        if (pl->fuel.sum > 0 && Player_is_thrusting(pl) && (pl->emergency_thrust_left -= timeStep) <= 0)
        {
            pl->emergency_thrust_left = 0;
            if (pl->item[ITEM_EMERGENCY_THRUST] > 0)
                Emergency_thrust(pl, true);
            else
                Emergency_thrust(pl, false);
        }
    }

    if (BIT(pl->used, USES_EMERGENCY_SHIELD))
    {
        if (pl->fuel.sum > 0 && BIT(pl->used, USES_SHIELD) && ((pl->emergency_shield_left -= timeStep) <= 0))
        {
            pl->emergency_shield_left = 0;
            if (pl->item[ITEM_EMERGENCY_SHIELD])
                Emergency_shield(pl, true);
            else
                Emergency_shield(pl, false);
        }
    }

    if (BIT(pl->used, HAS_LASER))
    {
        if (pl->item[ITEM_LASER] <= 0 || BIT(pl->used, USES_PHASING_DEVICE))
            CLR_BIT(pl->used, HAS_LASER);
        else
            Fire_laser(pl);
    }

    /*
     * Compute energy drainage
     */
    {
        if (BIT(pl->used, USES_SHIELD))
            Player_add_fuel(pl, ED_SHIELD);

        if (Player_is_phasing(pl))
            Player_add_fuel(pl, ED_PHASING_DEVICE);

        if (Player_is_cloaked(pl))
            Player_add_fuel(pl, ED_CLOAKING_DEVICE);

        if (BIT(pl->used, USES_DEFLECTOR))
            // Do_deflector(pl); <- TODO Do_deflector somewhere else
            Player_add_fuel(pl, ED_DEFLECTOR);
    }
}

/*
 * Player is refueling.
 */
static void Do_refuel(player_t *pl)
{
    world_t *world = &World;
    fuel_t *fs = Fuel_by_index(pl->fs);

    if ((World_wrap_length(
             world,
             pl->pos.cx - fs->pos.cx,
             pl->pos.cy - fs->pos.cy) > 90.0 * CLICK) ||
        (pl->fuel.sum >= pl->fuel.max) ||
        Player_is_phasing(pl) ||
        (BIT(World.rules->mode, TEAM_PLAY) && options.teamFuel && fs->team != pl->team))
        CLR_BIT(pl->used, USES_REFUEL);
    else
    {
        int n = pl->fuel.num_tanks;
        int ct = pl->fuel.current;

        do
        {
            if (fs->fuel > REFUEL_RATE * timeStep)
            {
                fs->fuel -= REFUEL_RATE * timeStep;
                fs->conn_mask = 0;
                fs->last_change = frame_loops;
                Player_add_fuel(pl, REFUEL_RATE * timeStep);
            }
            else
            {
                Player_add_fuel(pl, fs->fuel);
                fs->fuel = 0;
                fs->conn_mask = 0;
                fs->last_change = frame_loops;
                CLR_BIT(pl->used, USES_REFUEL);
                break;
            }
            if (pl->fuel.current == pl->fuel.num_tanks)
                pl->fuel.current = 0;
            else
                pl->fuel.current += 1;
        } while (n--);
        pl->fuel.current = ct;
    }
}

/*
 * Player is repairing a target.
 */
static void Do_repair(player_t *pl)
{
    world_t *world = &World;
    target_t *targ = Target_by_index(pl->repair_target);

    if ((World_wrap_length(world,
                           pl->pos.cx - targ->pos.cx,
                           pl->pos.cy - targ->pos.cy) > 90.0 * CLICK) ||
        targ->damage >= TARGET_DAMAGE ||
        targ->dead_ticks > 0 ||
        Player_is_phasing(pl))
        CLR_BIT(pl->used, USES_REPAIR);
    else
    {
        int n = pl->fuel.num_tanks;
        int ct = pl->fuel.current;

        do
        {
            if (pl->fuel.tank[pl->fuel.current] > REFUEL_RATE * timeStep)
            {
                targ->damage += TARGET_FUEL_REPAIR_PER_FRAME * timeStep;
                targ->conn_mask = 0;
                targ->last_change = frame_loops;
                Player_add_fuel(pl, -REFUEL_RATE * timeStep);
                if (targ->damage > TARGET_DAMAGE)
                {
                    targ->damage = TARGET_DAMAGE;
                    break;
                }
            }
            else
                CLR_BIT(pl->used, USES_REPAIR);

            if (pl->fuel.current == pl->fuel.num_tanks)
                pl->fuel.current = 0;
            else
                pl->fuel.current += 1;
        } while (n--);
        pl->fuel.current = ct;
    }
}

/* kps - UPDATE_RATE should depend on gamespeed */
#define UPDATE_RATE 100
static void Update_visibility(player_t *pl, int ind)
{
    int j;

    for (j = 0; j < NumPlayers; j++)
    {
        player_t *pl_j = Player_by_index(j);

        if (pl->forceVisible > 0)
            pl_j->visibility[ind].canSee = true;

        if (ind == j || !Player_is_cloaked(pl_j))
            pl->visibility[j].canSee = true;
        else if (pl->updateVisibility || pl_j->updateVisibility || (int)(rfrac() * UPDATE_RATE) < ABS(frame_loops - pl->visibility[j].lastChange))
        {

            pl->visibility[j].lastChange = frame_loops;

            if ((rfrac() * (pl->item[ITEM_SENSOR] + 1)) > (rfrac() * (pl_j->item[ITEM_CLOAK] + 1)))
                pl->visibility[j].canSee = true;
            else
                pl->visibility[j].canSee = false;
        }
    }
}
#undef UPDATE_RATE

/* * * * * *
 *
 * Player loop. Computes miscellaneous updates.
 *
 */
static void Update_players(void)
{
    int i;
    player_t *pl;

    for (i = 0; i < NumPlayers; i++)
    {
        pl = Player_by_index(i);

        /* Limits. */
        LIMIT(pl->power, MIN_PLAYER_POWER, MAX_PLAYER_POWER);
        LIMIT(pl->turnspeed, MIN_PLAYER_TURNSPEED, MAX_PLAYER_TURNSPEED);
        LIMIT(pl->turnresistance, MIN_PLAYER_TURNRESISTANCE,
              MAX_PLAYER_TURNRESISTANCE);

        if ((pl->damaged -= timeStep) <= 0)
            pl->damaged = 0;

        if (pl->flooding > FPS + 2)
        {
            Set_message_f("%s was kicked out because of flooding. "
                          "[*Server notice*]",
                          pl->name);
            Destroy_connection(pl->conn, "flooding");
            i--;
            continue;
        }
        else if (pl->flooding > 0)
            pl->flooding--;

        // if (pl->count > 0)
        // {
        //     pl->count--;
        //     if (!BIT(pl->obj_status, PLAYING))
        //     {
        //         Transport_to_home(pl);
        //         Move_player(pl);
        //         continue;
        //     }
        // }
        // if (pl->count == 0)
        // {
        //     pl->count = -1;
        //     if (!BIT(pl->obj_status, PLAYING))
        //     {
        //         SET_BIT(pl->obj_status, PLAYING);
        //         Go_home(pl);
        //     }
        // }
        if (pl->pause_count > 0)
        {
            pl->pause_count -= timeStep;
            warn("Player %s pause count is %f", pl->name, pl->pause_count);
            if (pl->pause_count <= 0)
                pl->pause_count = 0;
        }

        if (pl->recovery_count > 0)
        {
            pl->recovery_count -= timeStep;
            // warn("Player %s recovery count is %f", pl->name, pl->recovery_count);
            if (pl->recovery_count <= 0)
            {
                warn("Player %s recovered!", pl->name);
                /* Player has recovered (unless he is already dead). */
                pl->recovery_count = 0;
                if (BIT(World.rules->mode, LIMITED_LIVES))
                {
                    if (!Player_is_dead(pl))
                        Player_set_state(pl, PL_STATE_ALIVE);
                }
                else
                    Player_set_state(pl, PL_STATE_ALIVE);
                Go_home(pl);
            }
            else
            {
                /* Player didn't recover yet. */
                Transport_to_home(pl);
                Move_player(pl);
                continue;
            }
        }

        if (Player_is_self_destructing(pl))
        {
            pl->self_destruct_count -= timeStep;
            // warn("Player %s self destruct count is %f", pl->name, pl->self_destruct_count);
            if (pl->self_destruct_count <= 0)
            {
                Handle_Scoring(SCORE_SELF_DESTRUCT, pl, NULL, NULL, NULL);
                Player_set_state(pl, PL_STATE_KILLED);
                Set_message_f("%s has committed suicide.", pl->name);
                Throw_items(pl);
                Kill_player(pl, true);
                updateScores = true;
            }
        }

        if (!Player_is_active(pl))
            continue;

        if (pl->stunned > 0)
        {
            pl->stunned--;
            CLR_BIT(pl->used, HAS_SHIELD | HAS_LASER | HAS_SHOT);
            Thrust(pl, false);
        }

        Use_items(pl);

        /*
         * Only do autopilot code if switched on and player is not
         * damaged (ie. can see).
         */
        if ((Player_uses_autopilot(pl) || Player_is_hoverpaused(pl)) && !pl->damaged)
            do_Autopilot(pl);

        /*
         * Compute turn
         */
        pl->turnvel += pl->turnacc;

        /*
         * turnresistance is zero: client requests linear turning behaviour
         * when playing with pointer control.
         */
        if (pl->turnresistance)
        {
            pl->turnvel *= pl->turnresistance;
        }

        pl->float_dir += pl->turnvel;

        while (pl->float_dir < 0)
            pl->float_dir += ANGLE_RESOLUTION;
        while (pl->float_dir >= ANGLE_RESOLUTION)
            pl->float_dir -= ANGLE_RESOLUTION;

        /*
         * turnresistance is zero: client requests linear turning behaviour
         * when playing with pointer control.
         */
        if (!pl->turnresistance)
            pl->turnvel = 0;

        Turn_player(pl, false); // TODO: false = don't care

        Update_visibility(pl, i);

        if (Player_is_refueling(pl))
            Do_refuel(pl);

        if (Player_is_repairing(pl))
            Do_repair(pl);

        if (pl->fuel.sum <= 0)
        {
            Thrust(pl, false);
            CLR_BIT(pl->used, USES_SHIELD);
            CLR_BIT(pl->used, USES_CLOAKING_DEVICE);
            CLR_BIT(pl->used, USES_DEFLECTOR);
        }
        if (pl->fuel.sum > (pl->fuel.max - REFUEL_RATE * timeStep))
            CLR_BIT(pl->used, USES_REFUEL);

        /*
         * Update acceleration vector etc.
         */
        if (Player_is_thrusting(pl))
        {
            double power = pl->power;
            double f = pl->power * 0.0008; /* 1/(FUEL_SCALE*MIN_POWER) */
            int a = (Player_uses_emergency_thrust(pl)
                         ? MAX_AFTERBURNER
                         : pl->item[ITEM_AFTERBURNER]);
            double inert = pl->mass;

            if (a)
            {
                power = AFTER_BURN_POWER(power, a);
                f = AFTER_BURN_FUEL(f, a);
            }
            pl->acc.x = power * tcos(pl->dir) / inert;
            pl->acc.y = power * tsin(pl->dir) / inert;
            Player_add_fuel(pl, -f); /* Decrement fuel */
        }
        else
            pl->acc.x = pl->acc.y = 0.0;

        Player_set_mass(pl);

        // TODO: wormhole update

        /*
         * Handle hyperjumps and wormholes.
         */
        if (BIT(pl->obj_status, WARPING))
        {
            Do_warp(pl);
        }

        if (!Player_is_paused(pl))
        {
            update_object_speed(OBJ_PTR(pl)); /* New position */
            Move_player(pl);
        }

        if ((!Player_is_cloaked(pl) || options.cloakedExhaust) && !Player_is_phasing(pl))
        {
            if (Player_is_thrusting(pl))
                Make_thrust_sparks(pl);
        }

        Compute_sensor_range(pl);

        pl->used &= pl->have;
    }
}

/********** **********
 * Updating objects and the like.
 */
void Update_objects(void)
{
    world_t *world = &World;
    int i;
    player_t *pl;
    object_t *obj;

    tick = true;
    /*
     * Update robots.
     */
    Robot_update(tick);

    /*
     * Autorepeat fire, must unfortunately be done here, not in
     * the player loop below, because of collisions between the shots
     * and the auto-firing player that would otherwise occur.
     */
    if (options.fireRepeatRate > 0)
    {
        for (int i = 0; i < NumPlayers; i++)
        {
            player_t *pl = Player_by_index(i);
            if (BIT(pl->used, HAS_SHOT))
                Fire_normal_shots(pl);
        }
    }

    /*
     * Special items.
     */
    for (i = 0; i < NUM_ITEMS; i++)
        if (World.items[i].num < World.items[i].max && World.items[i].chance > 0 && (rfrac() * World.items[i].chance) < 1.0f)
            Place_item(NULL, i);

    Fuel_update();
    Misc_object_update();

    Asteroid_update();
    if (Num_ecms() > 0)
        Ecm_update();
    if (Num_transporters() > 0)
        Transporter_update();

    bool tick = true;
    if (Num_cannons() > 0)
        Cannon_update(tick);

    if (Num_targets() > 0)
        Target_update();

    // xpinfo("player loop");

    /* * * * * *
     *
     * Player loop. Computes miscellaneous updates.
     *
     */
    Update_players();

    for (int i = Num_wormholes() - 1; i >= 0; i--)
    {
        if (World.wormholes[i].countdown > 0)
            World.wormholes[i].countdown--;
        if (World.wormholes[i].temporary && World.wormholes[i].countdown <= 0)
            remove_temp_wormhole(i);
    }

    // xpinfo("visibility");

    for (int ind = 0; ind < NumPlayers; ind++)
    {
        player_t *pl = PlayersArray[ind];

        pl->updateVisibility = false;

        if (pl->forceVisible)
        {
            pl->forceVisible--;

            if (!pl->forceVisible)
                pl->updateVisibility = true;
        }

        if (Player_uses_tractor_beam(pl))
            Tractor_beam(pl);

        if (BIT(pl->lock.tagged, LOCK_PLAYER))
        {
            player_t *lock_pl = Player_by_id(pl->lock.pl_id);
            pl->lock.distance =
                World_wrap_length(
                    world,
                    pl->pos.cx - lock_pl->pos.cx,
                    pl->pos.cy - lock_pl->pos.cy) /
                CLICK;
        }
    }

    /*
     * Checking for collision, updating score etc. (see collision.c)
     */
    Check_collision();

    /*
     * Update tanks, Kill players that ought to be killed.
     */
    for (i = NumPlayers - 1; i >= 0; i--)
    {
        pl = Player_by_index(i);

        if (Player_is_alive(pl))
            Update_tanks(&(pl->fuel));

        if (Player_is_killed(pl))
        {
            Throw_items(pl);
            Detonate_items(pl);
            Kill_player(pl, true);

            if (Player_is_human(pl))
            {
                if (frame_loops - pl->frame_last_busy > 60 * FPS)
                {
                    if ((NumPlayers - NumRobots - NumPseudoPlayers) > 1)
                        Pause_player(pl, true);
                }
            }
        }

        if (options.maxPauseTime > 0 &&
            Player_is_human(pl) && Player_is_paused(pl) && frame_loops - pl->frame_last_busy > options.maxPauseTime)
        {
            sprintf(msg,
                    "%s was auto-kicked for pausing too long [*Server notice*]",
                    pl->name);
            Set_message(msg);
            Destroy_connection(pl->conn, "auto-kicked: paused too long");
        }
    }

    /*
     * Kill shots that ought to be dead.
     */
    for (int i = NumObjs - 1; i >= 0; i--)
        if (--(Obj[i]->obj_life) <= 0)
            Delete_shot(i);

    /*
     * Compute general game status, do we have a winner?
     * (not called after Game_Over() )
     */
    if (options.gameDuration >= 0.0 || options.maxRoundTime > 0)
        Compute_game_status();

    /*
     * Now update labels if need be.
     */
#define UPDATE_SCORE_DELAY (FPS)
    if (updateScores && ((frame_loops % UPDATE_SCORE_DELAY) == 0))
        Update_score_table();
}
