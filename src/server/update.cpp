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

#include "xperror.h"

#include "server.h"
#include "robot.h"

#define SERVER
#include "xpconfig.h"
#include "serverconst.h"
#include "global.h"
#include "map.h"
#include "score.h"
#include "bit.h"
#include "saudio.h"
#include "object.h"
#include "cannon.h"
#include "asteroid.h"
#include "netserver.h"
#include "xpmath.h"
#include "walls.h"

#define update_object_speed(o_)                                                                  \
    if (BIT((o_)->obj_status, GRAVITY))                                                          \
    {                                                                                            \
        (o_)->vel.x += (o_)->acc.x + world->gravity[OBJ_X_IN_BLOCKS(o_)][OBJ_Y_IN_BLOCKS(o_)].x; \
        (o_)->vel.y += (o_)->acc.y + world->gravity[OBJ_X_IN_BLOCKS(o_)][OBJ_Y_IN_BLOCKS(o_)].y; \
    }                                                                                            \
    else                                                                                         \
    {                                                                                            \
        (o_)->vel.x += (o_)->acc.x;                                                              \
        (o_)->vel.y += (o_)->acc.y;                                                              \
    }

int roundtime = -1; /* time left this round */

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
    double bx, by, dx, dy, t, m;
    const int T = RECOVERY_DELAY;

    if (BIT(world->rules->mode, TIMING) && pl->round)
    {
        int check;

        if (pl->check)
            check = pl->check - 1;
        else
            check = world->NumChecks - 1;
        bx = (world->checks[check].x + 0.5) * BLOCK_SZ;
        by = (world->checks[check].y + 0.5) * BLOCK_SZ;
    }
    else
    {
        bx = (pl->home_base->blk_pos.bx + 0.5) * BLOCK_SZ;
        by = (pl->home_base->blk_pos.by + 0.5) * BLOCK_SZ;
    }
    dx = WRAP_DX(bx - pl->pix_pos.x);
    dy = WRAP_DY(by - pl->pix_pos.y);
    t = pl->count + 0.5f;
    if (2 * t <= T)
    {
        m = 2 / t;
    }
    else
    {
        t = T - t;
        m = (4 * t) / (T * T - 2 * t * t);
    }
    pl->vel.x = dx * m;
    pl->vel.y = dy * m;
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
                CLR_BIT(pl->used, HAS_SHIELD);
                CLR_BIT(pl->have, HAS_SHIELD);
            }
            sound_play_player(pl, CLOAK_SOUND);
            pl->updateVisibility = true;
            SET_BIT(pl->used, USES_CLOAKING_DEVICE);
        }
    }
    else
    {
        if (BIT(pl->used, USES_CLOAKING_DEVICE))
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
            CLR_BIT(pl->used, HAS_SHIELD);
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
    int dir;
    int afterburners;
    int ix, iy;
    double gx, gy;
    double acc, vel;
    double delta;
    double turnspeed, power;
    const double emergency_thrust_settings_delta = 150.0 / FPS;
    const double auto_pilot_settings_delta = 15.0 / FPS;
    const double auto_pilot_turn_factor = 2.5;
    const double auto_pilot_dead_velocity = 0.5;

    /*
     * If the last movement touched a wall then we shouldn't
     * mess with the position (speed too?) settings.
     */
    if (pl->last_wall_touch + 1 >= frame_loops)
    {
        return;
    }

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
    {
        afterburners = pl->item[ITEM_AFTERBURNER];
    }

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
    {
        dir = 1;
    }

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
    {
        pl->turnacc = dir * pl->turnspeed;
    }

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
    {
        Thrust(pl, false);
    }
    else
    {
        Thrust(pl, true);
    }
}

/********** **********
 * Updating objects and the like.
 */
void Update_objects(void)
{
    object_t *obj;
    bool tick = true;

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
    for (int i = 0; i < NUM_ITEMS; i++)
        if (world->items[i].num < world->items[i].max && world->items[i].chance > 0 && (rfrac() * world->items[i].chance) < 1.0f)
            Place_item(i, nullptr);

    /*
     * Let the fuel stations regenerate some fuel.
     */
    if (NumPlayers > 0)
    {
        int fuel = (int)(NumPlayers * STATION_REGENERATION);
        int frames_per_update = MAX_STATION_FUEL / (fuel * BLOCK_SZ);
        for (int i = 0; i < world->NumFuels; i++)
        {
            if (world->fuels[i].fuel == MAX_STATION_FUEL)
                continue;
            if ((world->fuels[i].fuel += fuel) >= MAX_STATION_FUEL)
                world->fuels[i].fuel = MAX_STATION_FUEL;
            else if (world->fuels[i].last_change + frames_per_update > frame_loops)
                /*
                 * We don't send fuelstation info to the clients every frame
                 * if it wouldn't change their display.
                 */
                continue;

            world->fuels[i].conn_mask = 0;
            world->fuels[i].last_change = frame_loops;
        }
    }

    /*
     * Update shots.
     */
    for (int i = 0; i < NumObjs; i++)
    {
        obj = Obj[i];

        if (obj->type == OBJ_MINE)
            Update_mine(MINE_PTR(obj));

        else if (obj->type == OBJ_TORPEDO)
            Update_torpedo(TORP_PTR(obj));

        else if (obj->type == OBJ_HEAT_SHOT)
            Update_missile(MISSILE_PTR(obj));

        else if (obj->type == OBJ_SMART_SHOT)
            Update_missile(MISSILE_PTR(obj));

        else if (obj->type == OBJ_BALL)
        {
            if (obj->id != NO_ID)
                Move_ball(i);
        }

        else if (obj->type == OBJ_WRECKAGE)
        {
            wireobject_t *wireobj = WIRE_PTR(obj);
            wireobj->wire_rotation =
                (wireobj->wire_rotation + (int)(wireobj->wire_turnspeed * RES)) % RES;
        }

        update_object_speed(obj);

        if (obj->type == OBJ_ASTEROID)
            Move_object(obj);
    }

    /*
     * Asteroids.
     */
    Asteroid_update();

    double ecmSizeFactor = 0.5;
    /*
     * Update ECM blasts
     */
    for (int i = 0; i < world->NumEcms; i++)
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
            // free(Ecms[i]);
            --world->NumEcms;
            world->ecms[i] = world->ecms[world->NumEcms];
            i--;
        }
    }

    /*
     * Update transporters
     */
    for (int i = 0; i < Num_transporters(); i++)
    {
        transporter_t *trans = Transporter_by_index(i);

        if (--trans->count <= 0)
        {
            // free(Transporters[i]);
            --world->NumTransporters;
            world->transporters[i] = world->transporters[world->NumTransporters];
            i--;
        }
    }

    if (Num_cannons() > 0)
        Cannon_update(tick);

    /*
     * Update targets
     */
    for (int i = 0; i < world->NumTargets; i++)
    {
        if (world->targets[i].dead_time > 0)
        {
            if (!--world->targets[i].dead_time)
            {
                world->block[world->targets[i].blk_pos.bx][world->targets[i].blk_pos.by] = TARGET;
                world->targets[i].conn_mask = 0;
                world->targets[i].update_mask = (unsigned)-1;
                world->targets[i].last_change = frame_loops;

                if (options.targetSync)
                {
                    uint16_t team = world->targets[i].team;

                    for (int j = 0; j < world->NumTargets; j++)
                    {
                        if (world->targets[j].team == team)
                        {
                            world->block[world->targets[j].blk_pos.bx]
                                        [world->targets[j].blk_pos.by] = TARGET;
                            world->targets[j].conn_mask = 0;
                            world->targets[j].update_mask = (unsigned)-1;
                            world->targets[j].last_change = frame_loops;
                            world->targets[j].dead_time = 0;
                            world->targets[j].damage = TARGET_DAMAGE;
                        }
                    }
                }
            }
            continue;
        }
        else if (world->targets[i].damage == TARGET_DAMAGE)
        {
            continue;
        }
        world->targets[i].damage += TARGET_REPAIR_PER_FRAME;
        if (world->targets[i].damage >= TARGET_DAMAGE)
        {
            world->targets[i].damage = TARGET_DAMAGE;
        }
        else if (world->targets[i].last_change + TARGET_UPDATE_DELAY < frame_loops)
        {
            /*
             * We don't send target info to the clients every frame
             * if the latest repair wouldn't change their display.
             */
            continue;
        }
        world->targets[i].conn_mask = 0;
        world->targets[i].last_change = frame_loops;
    }

    /* * * * * *
     *
     * Player loop. Computes miscellaneous updates.
     *
     */
    for (int ind = 0; ind < NumPlayers; ind++)
    {
        player_t *pl = PlayersArray[ind];

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
                sprintf(msg, "%s has committed suicide.", pl->name);
                Set_message(msg);
                Throw_items(pl);
                Kill_player(pl, true);
                updateScores = true;
            }
        }

        if (BIT(pl->obj_status, PLAYING | GAME_OVER | PAUSE) != PLAYING)
            continue;

        if (pl->stunned > 0)
        {
            pl->stunned--;
            CLR_BIT(pl->used, HAS_SHIELD | HAS_LASER | HAS_SHOT);
            Thrust(pl, false);
        }

        if (pl->shield_time > 0)
        {
            if (--pl->shield_time == 0)
            {
                if (!BIT(pl->used, USES_EMERGENCY_SHIELD))
                    CLR_BIT(pl->used, HAS_SHIELD);
            }
            if (BIT(pl->used, HAS_SHIELD) == 0)
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
                if (pl->item[ITEM_PHASING])
                    Phasing(pl, true);
                else
                    Phasing(pl, false);
            }
        }

        if (Player_uses_emergency_thrust(pl))
        {
            if (pl->fuel.sum > 0 && Player_is_thrusting(pl) && --pl->emergency_thrust_left <= 0)
            {
                if (pl->item[ITEM_EMERGENCY_THRUST])
                    Emergency_thrust(pl, true);
                else
                    Emergency_thrust(pl, false);
            }
        }

        if (BIT(pl->used, USES_EMERGENCY_SHIELD))
        {
            if (pl->fuel.sum > 0 && BIT(pl->used, HAS_SHIELD) && --pl->emergency_shield_left <= 0)
            {
                if (pl->item[ITEM_EMERGENCY_SHIELD])
                    Emergency_shield(pl, true);
                else
                    Emergency_shield(pl, false);
            }
        }

        if (BIT(pl->used, HAS_LASER))
        {
            if (pl->item[ITEM_LASER] <= 0 || Player_is_phasing(pl))
                CLR_BIT(pl->used, HAS_LASER);
            else
                Fire_laser(pl);
        }

        if (BIT(pl->used, USES_DEFLECTOR))
            Do_deflector(pl);

        /*
         * Only do autopilot code if switched on and player is not
         * damaged (ie. can see).
         */
        if ((Player_uses_autopilot(pl)) || (BIT(pl->obj_status, HOVERPAUSE) && !pl->damaged))
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

        /*
         * Compute energy drainage
         */
        if (BIT(pl->used, HAS_SHIELD))
            Player_add_fuel(pl, ED_SHIELD);

        if (Player_is_phasing(pl))
            Player_add_fuel(pl, ED_PHASING_DEVICE);

        if (BIT(pl->used, USES_CLOAKING_DEVICE))
            Player_add_fuel(pl, ED_CLOAKING_DEVICE);

#define UPDATE_RATE 100

        for (int j = 0; j < NumPlayers; j++)
        {
            if (pl->forceVisible)
                PlayersArray[j]->visibility[ind].canSee = 1;

            if (ind == j || !BIT(PlayersArray[j]->used, USES_CLOAKING_DEVICE))
                pl->visibility[j].canSee = 1;
            else if (pl->updateVisibility || PlayersArray[j]->updateVisibility || (int)(rfrac() * UPDATE_RATE) < ABS(frame_loops - pl->visibility[j].lastChange))
            {

                pl->visibility[j].lastChange = frame_loops;
                pl->visibility[j].canSee = (rfrac() * (pl->item[ITEM_SENSOR] + 1)) > (rfrac() * (PlayersArray[j]->item[ITEM_CLOAK] + 1));
            }
        }

        if (Player_is_refueling(pl))
        {
            if ((Wrap_length(pl->pos.cx - world->fuels[pl->fs].pos.cx,
                             pl->pos.cy - world->fuels[pl->fs].pos.cy) /
                     CLICK >
                 90.0) ||
                (pl->fuel.sum >= pl->fuel.max) ||
                (world->block[world->fuels[pl->fs].blk_pos.bx][world->fuels[pl->fs].blk_pos.by] != FUEL) ||
                Player_is_phasing(pl) ||
                (BIT(world->rules->mode, TEAM_PLAY) && options.teamFuel && world->fuels[pl->fs].team != pl->team))
            {
                CLR_BIT(pl->used, USES_REFUEL);
            }
            else
            {
                int i = pl->fuel.num_tanks;
                int ct = pl->fuel.current;

                do
                {
                    if (world->fuels[pl->fs].fuel > REFUEL_RATE)
                    {
                        world->fuels[pl->fs].fuel -= REFUEL_RATE;
                        world->fuels[pl->fs].conn_mask = 0;
                        world->fuels[pl->fs].last_change = frame_loops;
                        Player_add_fuel(pl, REFUEL_RATE);
                    }
                    else
                    {
                        Player_add_fuel(pl, world->fuels[pl->fs].fuel);
                        world->fuels[pl->fs].fuel = 0;
                        world->fuels[pl->fs].conn_mask = 0;
                        world->fuels[pl->fs].last_change = frame_loops;
                        CLR_BIT(pl->used, USES_REFUEL);
                        break;
                    }
                    if (pl->fuel.current == pl->fuel.num_tanks)
                        pl->fuel.current = 0;
                    else
                        pl->fuel.current += 1;
                } while (i--);
                pl->fuel.current = ct;
            }
        }

        /* target repair */
        if (BIT(pl->used, USES_REPAIR))
        {
            target_t *targ = &world->targets[pl->repair_target];
            if (Wrap_length(pl->pos.cx - targ->pos.cx, pl->pos.cy - targ->pos.cy) / CLICK > 90.0 ||
                targ->damage >= TARGET_DAMAGE ||
                targ->dead_time > 0 ||
                Player_is_phasing(pl))
                CLR_BIT(pl->used, USES_REPAIR);
            else
            {
                int i = pl->fuel.num_tanks;
                int ct = pl->fuel.current;

                do
                {
                    if (pl->fuel.tank[pl->fuel.current] > REFUEL_RATE)
                    {
                        targ->damage += TARGET_FUEL_REPAIR_PER_FRAME;
                        targ->conn_mask = 0;
                        targ->last_change = frame_loops;
                        Player_add_fuel(pl, -REFUEL_RATE);
                        if (targ->damage > TARGET_DAMAGE)
                        {
                            targ->damage = TARGET_DAMAGE;
                            break;
                        }
                    }
                    else
                    {
                        CLR_BIT(pl->used, USES_REPAIR);
                    }
                    if (pl->fuel.current == pl->fuel.num_tanks)
                        pl->fuel.current = 0;
                    else
                        pl->fuel.current += 1;
                } while (i--);
                pl->fuel.current = ct;
            }
        }

        if (pl->fuel.sum <= 0)
        {
            CLR_BIT(pl->used, HAS_SHIELD | HAS_CLOAKING_DEVICE | HAS_DEFLECTOR);
            Thrust(pl, false);
        }
        if (pl->fuel.sum > (pl->fuel.max - REFUEL_RATE))
            CLR_BIT(pl->used, USES_REFUEL);

        /*
         * Update acceleration vector etc.
         */
        if (Player_is_thrusting(pl))
        {
            double power = pl->power;
            double f = pl->power * 0.0008; /* 1/(FUEL_SCALE*MIN_POWER) */
            int a = (BIT(pl->used, USES_EMERGENCY_THRUST)
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
        {
            pl->acc.x = pl->acc.y = 0.0;
        }

        Player_set_mass(pl);

        if (BIT(pl->obj_status, WARPING))
        {
            position_t w;
            int wx, wy, proximity,
                nearestFront, nearestRear,
                proxFront, proxRear, j;

            if (pl->wormHoleHit >= world->NumWormholes)
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
                else if (rfrac() < 0.10f)
                {
                    do
                        j = (int)(rfrac() * world->NumWormholes);
                    while (world->wormholes[j].type == WORM_IN || pl->wormHoleHit == j || world->wormholes[j].temporary);
                }
                else
                {
                    nearestFront = nearestRear = -1;
                    proxFront = proxRear = 10000000;

                    for (j = 0; j < world->NumWormholes; j++)
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
                            j = (int)(rfrac() * world->NumWormholes);
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
                    w.x = OBJ_X_IN_PIXELS(pl);
                    w.y = OBJ_Y_IN_PIXELS(pl);
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
                    if (b->type == OBJ_BALL && b->id == pl->id)
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

        if (!BIT(pl->obj_status, PAUSE))
        {
            update_object_speed(pl); /* New position */
            Move_player(pl);
        }

        if ((!BIT(pl->used, USES_CLOAKING_DEVICE) || options.cloakedExhaust) && !Player_is_phasing(pl))
        {
            if (Player_is_thrusting(pl))
                Thrust(pl);
        }

        Compute_sensor_range(pl);

        pl->used &= pl->have;
    }

    for (int i = world->NumWormholes - 1; i >= 0; i--)
    {
        if (world->wormholes[i].countdown > 0)
            world->wormholes[i].countdown--;
        if (world->wormholes[i].temporary && world->wormholes[i].countdown <= 0)
            remove_temp_wormhole(i);
    }

    for (int ind = 0; ind < NumPlayers; ind++)
    {
        player_t *pl = PlayersArray[ind];

        pl->updateVisibility = 0;

        if (pl->forceVisible)
        {
            pl->forceVisible--;

            if (!pl->forceVisible)
                pl->updateVisibility = true;
        }

        if (BIT(pl->used, USES_TRACTOR_BEAM))
            Tractor_beam(pl);

        if (BIT(pl->lock.tagged, LOCK_PLAYER))
        {
            player_t *ship = Player_by_id(pl->lock.pl_id);
            // Guard against ship = (nil)
            if (!pl || !ship)
                warn("update: pl = %p, ship = %p", pl, ship);
            else
                pl->lock.distance =
                    Wrap_length(pl->pos.cx - ship->pos.cx,
                                pl->pos.cy - ship->pos.cy) /
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
    for (int ind = NumPlayers - 1; ind >= 0; ind--)
    {
        player_t *pl = PlayersArray[ind];

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
            Player_is_human(pl) && BIT(pl->obj_status, PAUSE) && frame_loops - pl->frame_last_busy > options.maxPauseTime)
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
    {
        Compute_game_status();
    }

    /*
     * Now update labels if need be.
     */
#define UPDATE_SCORE_DELAY (FPS)
    if (updateScores && frame_loops % UPDATE_SCORE_DELAY == 0)
        Update_score_table();
}
