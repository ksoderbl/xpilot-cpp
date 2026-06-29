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
#include "walls1.h"
#include "robot.h"

int roundtime = -1;               /* time left this round */
static double time_to_tick = 1.0; /* game time till next tick */
static bool tick = false;         /* new tick of game time this frame */

static inline void update_object_speed(object_t *obj)
{
    // if (BIT(obj->obj_status, GRAVITY))
    // {
    //     obj->vel.x += obj->acc.x + world->gravity[OBJ_X_IN_BLOCKS(obj)][OBJ_Y_IN_BLOCKS(obj)].x;
    //     obj->vel.y += obj->acc.y + world->gravity[OBJ_X_IN_BLOCKS(obj)][OBJ_Y_IN_BLOCKS(obj)].y;
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
    /*
     * Transport a corpse from the place where it died back to its homebase,
     * or if in race mode, back to the last passed check point.
     *
     * During the first part of the distance we give it a positive constant
     * acceleration G, during the second part we make this a negative one -G.
     * This results in a visually pleasing take off and landing.
     */
    clpos_t startpos;
    double bx, by;
    double dx, dy, t, m;
    const int T = RECOVERY_DELAY;

    /*
    if (pl->home_base == NULL)
    {
        pl->vel.x = 0;
        pl->vel.y = 0;
        return;
    }
        */

    if (BIT(world->rules->mode, TIMING) && pl->round)
    {
        int check;

        if (pl->check)
            check = pl->check - 1;
        else
            check = Num_checks() - 1;
        // bx = (world->checks[check].x + 0.5) * BLOCK_SZ;
        // by = (world->checks[check].y + 0.5) * BLOCK_SZ;
        startpos = Check_by_index(check)->pos;
    }
    else
    {
        // bx = (world->bases[pl->home_base_ind].blk_pos.bx + 0.5) * BLOCK_SZ;
        // by = (world->bases[pl->home_base_ind].blk_pos.by + 0.5) * BLOCK_SZ;
        startpos = world->bases[pl->home_base_ind].pos;
    }
    // dx = WRAP_DX(bx - pl->pix_pos.x);
    // dy = WRAP_DY(by - pl->pix_pos.y);
    dx = WRAP_DCX(startpos.cx - pl->pos.cx);
    dy = WRAP_DCY(startpos.cy - pl->pos.cy);
    t = pl->count + 0.5f;
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
    const int phasing_time = 4 * FPS;

    if (on)
    {
        if (pl->phasing_left <= 0)
        {
            pl->phasing_left = phasing_time;
            pl->phasing_max = phasing_time;
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
    const int emergency_thrust_time = 4 * FPS;

    if (on)
    {
        if (pl->emergency_thrust_left <= 0)
        {
            pl->emergency_thrust_left = emergency_thrust_time;
            pl->emergency_thrust_max = emergency_thrust_time;
            pl->item[ITEM_EMERGENCY_THRUST]--;
        }
        if (!BIT(pl->used, USES_EMERGENCY_THRUST))
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
    const int emergency_shield_time = 4 * FPS; /* 8 -> 4 */

    if (on)
    {
        if (BIT(pl->have, HAS_EMERGENCY_SHIELD))
        {
            if (pl->emergency_shield_left <= 0)
            {
                pl->emergency_shield_left = emergency_shield_time;
                pl->emergency_shield_max = emergency_shield_time;
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
    gx = world->gravity[ix][iy].x;
    gy = world->gravity[ix][iy].y;

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
    vad = MOD2(vad - pl->dir, RES);
    if (vad > RES / 2)
    {
        vad = RES - vad;
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
    if (pl->turnspeed != turnspeed && vad > RES / 32)
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
    double fuel_times_256;
    int frames_per_update;

    if (NumPlayers == 0)
        return;

    // Let the fuel stations regenerate some fuel.
    fuel_times_256 = NumPlayers * STATION_REGENERATION * 256 * timeStep;
    frames_per_update = MAX_STATION_FUEL / (fuel_times_256 / 256 * BLOCK_SZ);

    for (i = 0; i < Num_fuels(); i++)
    {
        fuel_t *fs = Fuel_by_index(i);

        if (fs->fuel_times_256 == MAX_STATION_FUEL * 256)
            continue;
        if ((fs->fuel_times_256 += fuel_times_256) >= MAX_STATION_FUEL * 256)
            fs->fuel_times_256 = MAX_STATION_FUEL * 256;
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
                (wireobj->wire_rotation + (int)(wireobj->wire_turnspeed * timeStep * RES)) % RES;
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
            // --world->NumEcms;
            // world->ecms[i] = world->ecms[world->NumEcms];
            world->ecms.erase(world->ecms.begin() + i);
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
            // --world->NumTransporters;
            // world->transporters[i] = world->transporters[world->NumTransporters];
            world->transporters.erase(world->transporters.begin() + i);
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
        if (--pl->shield_time == 0)
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
        if (--pl->phasing_left <= 0)
        {
            if (pl->item[ITEM_PHASING] > 0)
                Phasing(pl, true);
            else
                Phasing(pl, false);
        }
    }

    if (Player_uses_emergency_thrust(pl))
    {
        if (pl->fuel.sum_times_256 > 0 && BIT(pl->obj_status, THRUSTING) && --pl->emergency_thrust_left <= 0)
        {
            if (pl->item[ITEM_EMERGENCY_THRUST])
                Emergency_thrust(pl, true);
            else
                Emergency_thrust(pl, false);
        }
    }

    if (BIT(pl->used, USES_EMERGENCY_SHIELD))
    {
        if (pl->fuel.sum_times_256 > 0 && BIT(pl->used, HAS_SHIELD) && --pl->emergency_shield_left <= 0)
        {
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
    if (BIT(pl->used, HAS_SHIELD))
        Player_add_fuel(pl, ED_SHIELD);

    if (Player_is_phasing(pl))
        Player_add_fuel(pl, ED_PHASING_DEVICE);

    if (Player_is_cloaked(pl))
        Player_add_fuel(pl, ED_CLOAKING_DEVICE);

    if (BIT(pl->used, USES_DEFLECTOR))
        Do_deflector(pl);
}

/*
 * Player is refueling.
 */
static void Do_refuel(player_t *pl)
{
    fuel_t *fs = Fuel_by_index(pl->fs);

    if ((Wrap_length(pl->pos.cx - fs->pos.cx,
                     pl->pos.cy - fs->pos.cy) /
             CLICK >
         90.0) ||
        (pl->fuel.sum_times_256 >= pl->fuel.max_times_256) ||
        (world->block[fs->blk_pos.bx][fs->blk_pos.by] != FUEL) ||
        BIT(pl->used, USES_PHASING_DEVICE) ||
        (BIT(world->rules->mode, TEAM_PLAY) && options.teamFuel && fs->team != pl->team))
    {
        CLR_BIT(pl->used, USES_REFUEL);
    }
    else
    {
        int n = pl->fuel.num_tanks;
        int ct = pl->fuel.current;

        do
        {
            if (fs->fuel_times_256 > REFUEL_RATE * 256 * timeStep)
            {
                fs->fuel_times_256 -= REFUEL_RATE * 256 * timeStep;
                fs->conn_mask = 0;
                fs->last_change = frame_loops;
                Player_add_fuel(pl, REFUEL_RATE * timeStep);
            }
            else
            {
                Player_add_fuel(pl, fs->fuel_times_256 / 256);
                fs->fuel_times_256 = 0;
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
    target_t *targ = &world->targets[pl->repair_target];
    if (Wrap_length(pl->pos.cx - targ->pos.cx, pl->pos.cy - targ->pos.cy) / CLICK > 90.0 ||
        targ->damage_times_256 >= TARGET_DAMAGE_TIMES_256 ||
        targ->dead_ticks > 0 ||
        BIT(pl->used, USES_PHASING_DEVICE))
        CLR_BIT(pl->used, USES_REPAIR);
    else
    {
        int n = pl->fuel.num_tanks;
        int ct = pl->fuel.current;

        do
        {
            if (pl->fuel.tank_times_256[pl->fuel.current] > REFUEL_RATE * 256 * timeStep)
            {
                targ->damage_times_256 += TARGET_FUEL_REPAIR_PER_FRAME_TIMES_256 * timeStep;
                targ->conn_mask = 0;
                targ->last_change = frame_loops;
                Player_add_fuel(pl, -REFUEL_RATE * timeStep);
                if (targ->damage_times_256 > TARGET_DAMAGE_TIMES_256)
                {
                    targ->damage_times_256 = TARGET_DAMAGE_TIMES_256;
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

        if (pl->damaged > 0)
            pl->damaged--;

        if (pl->count > 0)
        {
            pl->count--;
            if (!BIT(pl->obj_status, PLAYING))
            {
                Transport_to_home(pl);
                Move_player(pl);
                continue;
            }
        }

        if (pl->count == 0)
        {
            pl->count = -1;

            if (!BIT(pl->obj_status, PLAYING))
            {
                SET_BIT(pl->obj_status, PLAYING);
                Go_home(pl);
            }
            if (BIT(pl->obj_status, SELF_DESTRUCT))
            {
                SET_BIT(pl->obj_status, KILLED);
                Set_message_f("%s has committed suicide.", pl->name);
                Throw_items(pl);
                Kill_player(pl, true);
                updateScores = true;
            }
        }

        // if (!Player_is_active(pl))
        if (BIT(pl->obj_status, PLAYING | GAME_OVER | PAUSE) != PLAYING)
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
            pl->float_dir += RES;
        while (pl->float_dir >= RES)
            pl->float_dir -= RES;

        /*
         * turnresistance is zero: client requests linear turning behaviour
         * when playing with pointer control.
         */
        if (!pl->turnresistance)
            pl->turnvel = 0;

        Turn_player(pl);

        Update_visibility(pl, i);

        if (Player_is_refueling(pl))
            Do_refuel(pl);

        if (Player_is_repairing(pl))
            Do_repair(pl);

        if (pl->fuel.sum_times_256 <= 0)
        {
            Thrust(pl, false);
            CLR_BIT(pl->used, USES_SHIELD);
            CLR_BIT(pl->used, USES_CLOAKING_DEVICE);
            CLR_BIT(pl->used, USES_DEFLECTOR);
        }
        if (pl->fuel.sum_times_256 > (pl->fuel.max_times_256 - REFUEL_RATE * 256 * timeStep))
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
            Player_add_fuel_times_256(pl, -f * FUEL_SCALE_FACT); /* Decrement fuel */
        }
        else
        {
            pl->acc.x = pl->acc.y = 0.0;
        }

        Player_set_mass(pl);

        // TODO: wormhole update

        if (BIT(pl->obj_status, WARPING))
        {
            position_t w;
            int wx, wy, proximity,
                nearestFront, nearestRear,
                proxFront, proxRear, j;

            if (pl->wormHoleHit >= Num_wormholes())
            {
                /* could happen if the player hit a temporary wormhole
                   that was removed while the player was warping */
                CLR_BIT(pl->obj_status, WARPING);
                break;
            }

            if (pl->wormHoleHit != -1)
            {
                if (world->wormholes[pl->wormHoleHit].countdown > 0)
                {
                    j = world->wormholes[pl->wormHoleHit].lastdest;
                }
                else if (rfrac() < 0.10)
                {
                    do
                        j = (int)(rfrac() * Num_wormholes());
                    while (world->wormholes[j].type == WORM_IN || pl->wormHoleHit == j || world->wormholes[j].temporary);
                }
                else
                {
                    nearestFront = nearestRear = -1;
                    proxFront = proxRear = 10000000;

                    for (j = 0; j < Num_wormholes(); j++)
                    {
                        if (j == pl->wormHoleHit || world->wormholes[j].type == WORM_IN || world->wormholes[j].temporary)
                            continue;

                        wx = (world->wormholes[j].blk_pos.bx -
                              world->wormholes[pl->wormHoleHit].blk_pos.bx) *
                             BLOCK_SZ;
                        wy = (world->wormholes[j].blk_pos.by -
                              world->wormholes[pl->wormHoleHit].blk_pos.by) *
                             BLOCK_SZ;
                        wx = WRAP_DX(wx);
                        wy = WRAP_DX(wy);

                        proximity = (int)(pl->vel.y * wx + pl->vel.x * wy);
                        proximity = ABS(proximity);

                        if (pl->vel.x * wx + pl->vel.y * wy < 0)
                        {
                            if (proximity < proxRear)
                            {
                                nearestRear = j;
                                proxRear = proximity;
                            }
                        }
                        else if (proximity < proxFront)
                        {
                            nearestFront = j;
                            proxFront = proximity;
                        }
                    }

#define RANDOM_REAR_WORM
#ifndef RANDOM_REAR_WORM
                    j = nearestFront < 0 ? nearestRear : nearestFront;
#else  /* RANDOM_REAR_WORM */
                    if (nearestFront >= 0)
                    {
                        j = nearestFront;
                    }
                    else
                    {
                        do
                            j = (int)(rfrac() * Num_wormholes());
                        while (world->wormholes[j].type == WORM_IN || j == pl->wormHoleHit);
                    }
#endif /* RANDOM_REAR_WORM */
                }

                sound_play_sensors(pl->pos, WORM_HOLE_SOUND);

                w.x = (world->wormholes[j].blk_pos.bx + 0.5) * BLOCK_SZ;
                w.y = (world->wormholes[j].blk_pos.by + 0.5) * BLOCK_SZ;
            }
            else
            { /* wormHoleHit == -1 */
                int counter;
                for (counter = 20; counter > 0; counter--)
                {
                    w.x = (int)(rfrac() * world->width);
                    w.y = (int)(rfrac() * world->height);
                    if (BIT(1U << world->block[(int)(w.x / BLOCK_SZ)]
                                              [(int)(w.y / BLOCK_SZ)],
                            SPACE_BLOCKS))
                    {
                        break;
                    }
                }
                if (!counter)
                {
                    w.x = CLICK_TO_PIXEL(pl->pos.cx);
                    w.y = CLICK_TO_PIXEL(pl->pos.cy);
                }
                if (counter && options.wormTime && BIT(1U << world->block[OBJ_X_IN_BLOCKS(pl)][OBJ_Y_IN_BLOCKS(pl)], SPACE_BIT) && BIT(1U << world->block[(int)(w.x / BLOCK_SZ)][(int)(w.y / BLOCK_SZ)], SPACE_BIT))
                {
                    add_temp_wormholes(OBJ_X_IN_BLOCKS(pl),
                                       OBJ_Y_IN_BLOCKS(pl),
                                       (int)(w.x / BLOCK_SZ),
                                       (int)(w.y / BLOCK_SZ));
                }
                j = -2;
                sound_play_sensors(pl->pos, HYPERJUMP_SOUND);
            }

            /*
             * Don't connect to balls while warping.
             */
            if (BIT(pl->used, USES_CONNECTOR))
                pl->ball = NULL;

            if (BIT(pl->have, HAS_BALL))
            {
                /*
                 * Take every ball associated with player through worm hole.
                 * NB. the connector can cross a wall boundary this is
                 * allowed, so long as the ball itself doesn't collide.
                 */
                int k;
                for (k = 0; k < NumObjs; k++)
                {
                    object_t *b = Obj[k];
                    if (BIT(b->type, OBJ_BALL_BIT) && b->id == pl->id)
                    {
                        position_t ballpos;
                        ballpos.x = b->pix_pos.x + (w.x - pl->pix_pos.x);
                        ballpos.y = b->pix_pos.y + (w.y - pl->pix_pos.y);
                        ballpos.x = WRAP_XPIXEL(ballpos.x);
                        ballpos.y = WRAP_YPIXEL(ballpos.y);
                        if (ballpos.x < 0 || ballpos.x >= world->width || ballpos.y < 0 || ballpos.y >= world->height)
                        {
                            b->life = 0;
                        }
                        else
                        {
                            clpos_t ball_clpos;
                            ball_clpos.cx = FLOAT_TO_CLICK(ballpos.x);
                            ball_clpos.cy = FLOAT_TO_CLICK(ballpos.y);
                            Object_position_set_clpos(b, ball_clpos);
                            Object_position_remember(b);
                            b->vel.x *= WORM_BRAKE_FACTOR;
                            b->vel.y *= WORM_BRAKE_FACTOR;
                            Cell_add_object(b);
                        }
                    }
                }
            }

            pl->wormHoleDest = j;
            clpos_t pos;
            pos.cx = FLOAT_TO_CLICK(w.x);
            pos.cy = FLOAT_TO_CLICK(w.y);
            Player_position_init_clpos(pl, pos);
            pl->vel.x *= WORM_BRAKE_FACTOR;
            pl->vel.y *= WORM_BRAKE_FACTOR;
            pl->forceVisible += 15;

            if ((j != pl->wormHoleHit) && (pl->wormHoleHit != -1))
            {
                world->wormholes[pl->wormHoleHit].lastdest = j;
                if (!world->wormholes[j].temporary)
                {
                    world->wormholes[pl->wormHoleHit].countdown = (options.wormTime ? options.wormTime : WORMCOUNT);
                }
            }

            CLR_BIT(pl->obj_status, WARPING);
            SET_BIT(pl->obj_status, WARPED);

            sound_play_sensors(pl->pos, WORM_HOLE_SOUND);
        }

        if (!Player_is_paused(pl))
        {
            update_object_speed(OBJ_PTR(pl)); /* New position */
            Move_player(pl);
        }

        if ((!BIT(pl->used, USES_CLOAKING_DEVICE) || options.cloakedExhaust) && !BIT(pl->used, USES_PHASING_DEVICE))
        {
            if (BIT(pl->obj_status, THRUSTING))
                Thrust(pl);
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
    // xpinfo("in update_objects");

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
        if (world->items[i].num < world->items[i].max && world->items[i].chance > 0 && (rfrac() * world->items[i].chance) < 1.0f)
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
        if (world->wormholes[i].countdown > 0)
            world->wormholes[i].countdown--;
        if (world->wormholes[i].temporary && world->wormholes[i].countdown <= 0)
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
                Wrap_length(pl->pos.cx - lock_pl->pos.cx,
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
        if (--(Obj[i]->life) <= 0)
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
