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
#include <climits>

#include "const.h"

#include "server.h"

#define SERVER
#include "xpconfig.h"
#include "serverconst.h"

#include "score.h"
#include "saudio.h"
#include "cannon.h"
#include "object.h"
#include "netserver.h"
#include "xperror.h"
#include "xpmath.h"
#include "asteroid.h"

#define MISSILE_POWER_SPEED_FACT 0.25
#define MISSILE_POWER_TURNSPEED_FACT 0.75
#define MINI_TORPEDO_SPREAD_TIME 6
#define MINI_TORPEDO_SPREAD_SPEED 20
#define MINI_TORPEDO_SPREAD_ANGLE 90
#define MINI_MINE_SPREAD_TIME 18
#define MINI_MINE_SPREAD_SPEED 8
#define MINI_MISSILE_SPREAD_ANGLE 45

#define CONFUSED_UPDATE_GRANULARITY 10
#define CONFUSED_TIME 3

/***********************
 * Functions for shots.
 */

void Place_mine(player_t *pl)
{
    vector_t zero_vel = {0.0, 0.0};

    if (pl->item[ITEM_MINE] <= 0 || (BIT(pl->used, HAS_SHIELD | HAS_PHASING_DEVICE) && !options.shieldedMining))
    {
        return;
    }

    if (options.minMineSpeed > 0)
    {
        Place_moving_mine(pl);
        return;
    }

    /*
     * Dropped mines are immune to gravity. The latest theory is that mines
     * contain some sort of anti-gravity device. An even later theory
     * claims that they are anchored to space the same way as the walls are.
     */
    Place_general_mine(pl->id, pl->team, 0, pl->pos, zero_vel, pl->mods);
}

void Place_moving_mine(player_t *pl)
{
    // player_t *pl = PlayersArray[ind];
    vector_t vel = pl->vel;

    if (pl->item[ITEM_MINE] <= 0 || (BIT(pl->used, HAS_SHIELD | HAS_PHASING_DEVICE) && !options.shieldedMining))
    {
        return;
    }

    if (options.minMineSpeed > 0)
    {
        if (pl->velocity < options.minMineSpeed)
        {
            if (pl->velocity >= 1)
            {
                vel.x *= (options.minMineSpeed / pl->velocity);
                vel.y *= (options.minMineSpeed / pl->velocity);
            }
            else
            {
                vel.x = options.minMineSpeed * tcos(pl->dir);
                vel.y = options.minMineSpeed * tsin(pl->dir);
            }
        }
    }

    Place_general_mine(pl->id, pl->team, GRAVITY, pl->pos, vel, pl->mods);
}

void Place_general_mine(int id, int team, int status,
                        clpos_t pos, vector_t vel, modifiers_t mods)
{
    char msg[MSG_LEN];
    int used, life;
    long drain;
    double mass;
    int i, minis;
    vector_t mv;
    player_t *pl = Player_by_id(id);
    // cannon_t *cannon = Cannon_by_id(id);

    if (NumObjs + mods.mini >= MAX_TOTAL_SHOTS)
        return;

    if (BIT(world->rules->mode, WRAP_PLAY))
        pos = World_wrap_clpos(pos);

    if (!World_contains_clpos(pos))
        return;

    if (pl && BIT(pl->obj_status, KILLED))
        life = (int)(rfrac() * FPS);
    else if (BIT(status, FROMCANNON))
        life = CANNON_SHOT_LIFE;
    else
        life = (options.mineLife ? options.mineLife : MINE_LIFETIME);

    if (!BIT(mods.warhead, CLUSTER))
        mods.velocity = 0;
    if (!mods.mini)
        mods.spread = 0;

    if (options.nukeMinSmarts <= 0)
    {
        CLR_BIT(mods.nuclear, MODS_NUCLEAR);
    }
    if (BIT(mods.nuclear, MODS_NUCLEAR))
    {
        if (pl)
        {
            used = (BIT(mods.nuclear, MODS_FULLNUCLEAR)
                        ? pl->item[ITEM_MINE]
                        : options.nukeMinMines);
            if (pl->item[ITEM_MINE] < options.nukeMinMines)
            {
                Set_player_message_f(pl,
                                     "You need at least %d mines to %s %s!",
                                     options.nukeMinMines,
                                     (BIT(status, GRAVITY) ? "throw" : "drop"),
                                     Describe_shot(OBJ_MINE, status, mods, 0));
                return;
            }
        }
        else
            used = options.nukeMinMines;
        mass = MINE_MASS * used * NUKE_MASS_MULT;
    }
    else
    {
        mass = (BIT(status, FROMCANNON) ? MINE_MASS * 0.6 : MINE_MASS);
        used = 1;
    }

    if (pl)
    {
        drain = ED_MINE;
        if (BIT(mods.warhead, CLUSTER))
        {
            drain += (long)(CLUSTER_MASS_DRAIN(mass));
        }
        if (pl->fuel.sum < -drain)
        {
            Set_player_message_f(pl,
                                 "You need at least %ld fuel units to %s %s!",
                                 (-drain) >> FUEL_SCALE_BITS,
                                 (BIT(status, GRAVITY) ? "throw" : "drop"),
                                 Describe_shot(OBJ_MINE, status, mods, 0));
            return;
        }
        if (options.baseMineRange)
        {
            for (i = 0; i < NumPlayers; i++)
            {
                player_t *pl_i = Player_by_index(i);

                if (pl_i->id != pl->id && !Team_immune(pl_i->id, pl->id) && !Player_is_tank(pl_i))
                {
                    int dx = CLICK_TO_PIXEL(pos.cx - world->bases[pl_i->home_base_ind].pos.cx);
                    int dy = CLICK_TO_PIXEL(pos.cy - world->bases[pl_i->home_base_ind].pos.cy);
                    if (sqr(dx) + sqr(dy) <= sqr(options.baseMineRange))
                    {
                        Set_player_message(pl, "No base mining!");
                        return;
                    }
                }
            }
        }
        Player_add_fuel(pl, drain);
        pl->item[ITEM_MINE] -= used;

        if (used > 1)
        {
            Set_message_f("%s has %s %s!", pl->name,
                          (BIT(status, GRAVITY) ? "thrown" : "dropped"),
                          Describe_shot(OBJ_MINE, status, mods, 0));
            sound_play_all(NUKE_LAUNCH_SOUND);
        }
        else
            sound_play_sensors(pl->pos,
                               BIT(status, GRAVITY)
                                   ? DROP_MOVING_MINE_SOUND
                                   : DROP_MINE_SOUND);
    }

    minis = (mods.mini + 1);
    SET_BIT(status, OWNERIMMUNE);

    for (i = 0; i < minis; i++)
    {
        mineobject_t *mine;

        if ((mine = MINE_PTR(Object_allocate())) == NULL)
            break;

        mine->type = OBJ_MINE;
        mine->color = BLUE;
        mine->info = options.mineFuseTime;
        mine->obj_status = status;
        mine->id = (pl ? pl->id : NO_ID);
        mine->team = team;
        mine->mine_owner = mine->id;
        Object_position_init_clpos(OBJ_PTR(mine), pos);
        if (minis > 1)
        {
            int space = RES / minis;
            int dir;
            double spread;

            spread = (double)((unsigned)mods.spread + 1);
            /*
             * Dir gives (S is ship upwards);
             *
             *                              o                    o   o
             *        X2: o S        o        X3:   S                X4:   S
             *                            o   o            o   o
             */
            dir = (i * space) + space / 2 + (minis - 2) * (RES / 2) + (pl ? pl->dir : 0);
            dir += (int)((rfrac() - 0.5) * space * 0.5);
            dir = MOD2(dir, RES);
            mv.x = MINI_MINE_SPREAD_SPEED * tcos(dir) / spread;
            mv.y = MINI_MINE_SPREAD_SPEED * tsin(dir) / spread;
            /*
             * This causes the added initial velocity to reduce to
             * zero over the MINI_MINE_SPREAD_TIME.
             */
            mine->mine_spread_left = MINI_MINE_SPREAD_TIME;
            mine->acc.x = -mv.x / MINI_MINE_SPREAD_TIME;
            mine->acc.y = -mv.y / MINI_MINE_SPREAD_TIME;
        }
        else
        {
            mv.x = mv.y = mine->acc.x = mine->acc.y = 0.0;
            mine->mine_spread_left = 0;
        }
        mine->vel = mv;
        mine->vel.x += vel.x * MINE_SPEED_FACT;
        mine->vel.y += vel.y * MINE_SPEED_FACT;
        mine->mass = mass / minis;
        mine->life = life / minis;
        mine->mods = mods;
        mine->pl_range = (int)(MINE_RANGE / minis);
        mine->pl_radius = MINE_RADIUS;
        Cell_add_object(OBJ_PTR(mine));
    }
}

/*
 * Up to and including 3.2.6 it was:
 *     Cause all of the given player's dropped/thrown mines to explode.
 * Since this caused a slowdown when many mines detonated it
 * is changed into:
 *     Cause the mine which is closest to a player and owned
 *     by that player to detonate.
 */
void Detonate_mines(player_t *pl)
{
    int i, closest = -1;
    double dist;
    double min_dist = world->hypotenuse + 1;

    if (Player_is_phasing(pl))
        return;

    for (i = 0; i < NumObjs; i++)
    {
        object_t *mine = Obj[i];

        if (!(mine->type == OBJ_MINE))
            continue;
        /*
         * Mines which have been ECM reprogrammed should only be detonatable
         * by the reprogrammer, not by the original mine placer:
         */
        if (mine->id == pl->id)
        {
            dist = Wrap_length(pl->pos.cx - mine->pos.cx,
                               pl->pos.cy - mine->pos.cy) /
                   CLICK;
            if (dist < min_dist)
            {
                min_dist = dist;
                closest = i;
            }
        }
    }
    if (closest != -1)
        Obj[closest]->life = 0;

    return;
}

/*
 * Describes shot of 'type' which has 'status' and 'mods'.  If 'hit' is
 * non-zero this description is part of a collision, otherwise its part
 * of a launch message.
 */
char *Describe_shot(int type, int status, modifiers_t mods, int hit)
{
    const char *name, *howmany = "a ", *plural = "";
    static char msg[MSG_LEN];

    switch (type)
    {
    case OBJ_MINE:
        if (BIT(status, GRAVITY))
            name = "bomb";
        else
            name = "mine";
        break;
    case OBJ_SMART_SHOT:
        name = "smart missile";
        break;
    case OBJ_TORPEDO:
        name = "torpedo";
        break;
    case OBJ_HEAT_SHOT:
        name = "heatseeker";
        break;
    case OBJ_CANNON_SHOT:
        if (BIT(mods.warhead, CLUSTER))
        {
            howmany = "";
            name = "flak";
        }
        else
            name = "shot";
        break;
    default:
        /*
         * Cluster shots are actual debris from a cluster explosion
         * so we describe it as "cluster debris".
         */
        if (BIT(mods.warhead, CLUSTER))
        {
            howmany = "";
            name = "debris";
        }
        else
            name = "shot";
        break;
    }

    if (mods.mini && !hit)
    {
        howmany = "some ";
        plural = (type == OBJ_TORPEDO) ? "es" : "s";
    }

    sprintf(msg, "%s%s%s%s%s%s%s%s%s",
            howmany,
            ((mods.velocity || mods.spread || mods.power) ? "modified " : ""),
            (mods.mini ? "mini " : ""),
            (BIT(mods.nuclear, MODS_FULLNUCLEAR) ? "full " : ""),
            (BIT(mods.nuclear, MODS_NUCLEAR) ? "nuclear " : ""),
            (BIT(mods.warhead, IMPLOSION) ? "imploding " : ""),
            (BIT(mods.warhead, CLUSTER) ? "cluster " : ""),
            name,
            plural);

    return msg;
}

static inline bool Player_can_fire_shot(player_t *pl)
{
    if (pl->shots >= pl->shot_max || BIT(pl->used, HAS_SHIELD | HAS_PHASING_DEVICE))
        return false;
    return true;
    // if (pl->shots >= options.maxPlayerShots || BIT(pl->used, HAS_SHIELD) || Player_is_phasing(pl))
    //     return false;
    // return true;
}

void Fire_main_shot(player_t *pl, int type, int dir)
{
    clpos_t m_gun, pos;

    if (!Player_can_fire_shot(pl))
        return;

    m_gun = Ship_get_m_gun_clpos(pl->ship, pl->dir);
    pos.cx = pl->pos.cx + m_gun.cx;
    pos.cy = pl->pos.cy + m_gun.cy;

    Fire_general_shot(pl->id, pl->team, false, pos, type,
                      dir, pl->mods, NO_ID);
}

void Fire_shot(player_t *pl, int type, int dir)
{
    if (!Player_can_fire_shot(pl))
        return;

    Fire_general_shot(pl->id, pl->team, false, pl->pos, type,
                      dir, pl->mods, NO_ID);
}

void Fire_left_shot(player_t *pl, int type, int dir, int gun)
{
    clpos_t l_gun, pos;

    if (!Player_can_fire_shot(pl))
        return;

    l_gun = Ship_get_l_gun_clpos(pl->ship, gun, pl->dir);
    pos.cx = pl->pos.cx + l_gun.cx;
    pos.cy = pl->pos.cy + l_gun.cy;

    Fire_general_shot(pl->id, pl->team, false, pos, type,
                      dir, pl->mods, NO_ID);
}

void Fire_right_shot(player_t *pl, int type, int dir, int gun)
{
    clpos_t r_gun, pos;

    if (!Player_can_fire_shot(pl))
        return;

    r_gun = Ship_get_r_gun_clpos(pl->ship, gun, pl->dir);
    pos.cx = pl->pos.cx + r_gun.cx;
    pos.cy = pl->pos.cy + r_gun.cy;

    Fire_general_shot(pl->id, pl->team, false, pos, type,
                      dir, pl->mods, NO_ID);
}

void Fire_left_rshot(player_t *pl, int type, int dir, int gun)
{
    clpos_t l_rgun, pos;

    if (!Player_can_fire_shot(pl))
        return;

    l_rgun = Ship_get_l_rgun_clpos(pl->ship, gun, pl->dir);
    pos.cx = pl->pos.cx + l_rgun.cx;
    pos.cy = pl->pos.cy + l_rgun.cy;

    Fire_general_shot(pl->id, pl->team, false, pos, type,
                      dir, pl->mods, NO_ID);
}

void Fire_right_rshot(player_t *pl, int type, int dir, int gun)
{
    clpos_t r_rgun, pos;

    if (!Player_can_fire_shot(pl))
        return;

    r_rgun = Ship_get_r_rgun_clpos(pl->ship, gun, pl->dir);
    pos.cx = pl->pos.cx + r_rgun.cx;
    pos.cy = pl->pos.cy + r_rgun.cy;

    Fire_general_shot(pl->id, pl->team, false, pos, type,
                      dir, pl->mods, NO_ID);
}

void Fire_general_shot(int id, int team, bool cannon,
                       clpos_t pos, int type, int dir,
                       modifiers_t mods, int target_id)
{
    char msg[MSG_LEN];
    int used, fuse = 0, lock = 0, status = GRAVITY, i, ldir, minis;
    int pl_range, pl_radius, rack_no = 0, racks_left = 0, r, on_this_rack = 0;
    int side = 0, fired = 0;
    int life = options.shotLife;
    long drain;
    double mass = options.shotMass,
           speed = options.shotSpeed,
           turnspeed = 0,
           max_speed = SPEED_LIMIT,
           angle,
           spread;
    vector_t mv;
    clpos_t shotpos;
    object_t *mini_objs[MODS_MINI_MAX + 1];
    torpobject_t *torp;
    player_t *pl = Player_by_id(id);
    // cannon_t *cannon = Cannon_by_id(id);

    if (NumObjs >= MAX_TOTAL_SHOTS)
        return;

    if (!BIT(mods.warhead, CLUSTER))
        mods.velocity = 0;
    if (!mods.mini)
        mods.spread = 0;

    if (cannon)
    {
        mass = CANNON_SHOT_MASS;
        life = CANNON_SHOT_LIFE;
        SET_BIT(status, FROMCANNON);
    }

    switch (type)
    {
    default:
        return;

    case OBJ_SHOT:
        CLEAR_MODS(mods); /* Shots can't be modified! */
        /* FALLTHROUGH */
    case OBJ_CANNON_SHOT:
        pl_range = pl_radius = 0;
        if (pl)
        {
            if (pl->fuel.sum < -ED_SHOT)
                return;
            Player_add_fuel(pl, ED_SHOT);
            sound_play_sensors(pl->pos, FIRE_SHOT_SOUND);
            pl->shots++;
        }
        if (!options.shotsGravity)
            CLR_BIT(status, GRAVITY);
        break;

    case OBJ_SMART_SHOT:
    case OBJ_HEAT_SHOT:
        if ((type == OBJ_HEAT_SHOT) ? !options.allowHeatSeekers : !options.allowSmartMissiles)
        {
            if (options.allowTorpedoes)
                type = OBJ_TORPEDO;
            else
                return;
        }
        /* FALLTHROUGH */
    case OBJ_TORPEDO:
        /*
         * Make sure there are enough object entries for the mini shots.
         */
        if (NumObjs + mods.mini >= MAX_TOTAL_SHOTS)
            return;

        if (pl && pl->item[ITEM_MISSILE] <= 0)
            return;

        if (options.nukeMinSmarts <= 0)
            CLR_BIT(mods.nuclear, MODS_NUCLEAR);

        if (BIT(mods.nuclear, MODS_NUCLEAR))
        {
            if (pl)
            {
                used = (BIT(mods.nuclear, MODS_FULLNUCLEAR)
                            ? pl->item[ITEM_MISSILE]
                            : options.nukeMinSmarts);
                if (pl->item[ITEM_MISSILE] < options.nukeMinSmarts)
                {
                    Set_player_message_f(pl,
                                         "You need at least %d missiles to fire %s!",
                                         options.nukeMinSmarts,
                                         Describe_shot(type, status, mods, 0));
                    return;
                }
            }
            else
                used = options.nukeMinSmarts;

            mass = MISSILE_MASS * used * NUKE_MASS_MULT;
            pl_range = (type == OBJ_TORPEDO)
                           ? (int)NUKE_RANGE
                           : MISSILE_RANGE;
        }
        else
        {
            mass = MISSILE_MASS;
            used = 1;
            pl_range = (type == OBJ_TORPEDO)
                           ? (int)TORPEDO_RANGE
                           : MISSILE_RANGE;
        }
        pl_range /= mods.mini + 1;
        pl_radius = MISSILE_LEN;

        drain = used * ED_SMART_SHOT;
        if (BIT(mods.warhead, CLUSTER))
        {
            if (pl)
                drain += (long)(CLUSTER_MASS_DRAIN(mass));
        }

        if (pl && BIT(pl->obj_status, KILLED))
            life = (int)(rfrac() * FPS);
        else if (!cannon)
            life = (options.missileLife ? options.missileLife : MISSILE_LIFETIME);

        switch (type)
        {
        case OBJ_HEAT_SHOT:
#ifndef HEAT_LOCK
            lock = NO_ID;
#else  /* HEAT_LOCK */
            if (pl == NULL)
                lock = target_id;
            else
            {
                if (!BIT(pl->lock.tagged, LOCK_PLAYER) || ((pl->lock.distance > pl->sensor_range) && BIT(world->rules->mode, LIMITED_VISIBILITY)))
                {
                    lock = NO_ID;
                }
                else
                    lock = pl->lock.pl_id;
            }
#endif /* HEAT_LOCK */
            if (pl)
                sound_play_sensors(pl->pos, FIRE_HEAT_SHOT_SOUND);
            max_speed = SMART_SHOT_MAX_SPEED * HEAT_SPEED_FACT;
            turnspeed = SMART_TURNSPEED * HEAT_SPEED_FACT;
            speed *= HEAT_SPEED_FACT;
            break;

        case OBJ_SMART_SHOT:
            if (pl == NULL)
                lock = target_id;
            else
            {
                if (!BIT(pl->lock.tagged, LOCK_PLAYER) || ((pl->lock.distance > pl->sensor_range) && BIT(world->rules->mode, LIMITED_VISIBILITY)) || !pl->visibility[GetInd(pl->lock.pl_id)].canSee)
                    return;
                lock = pl->lock.pl_id;
            }
            max_speed = SMART_SHOT_MAX_SPEED;
            turnspeed = SMART_TURNSPEED;
            break;

        case OBJ_TORPEDO:
            lock = NO_ID;
            fuse = 8;
            break;

        default:
            break;
        }

        if (pl)
        {
            if (pl->fuel.sum < -drain)
            {
                Set_player_message_f(pl,
                                     "You need at least %ld fuel units to fire %s!",
                                     (-drain) >> FUEL_SCALE_BITS,
                                     Describe_shot(type, status, mods, 0));
                return;
            }
            Player_add_fuel(pl, drain);
            pl->item[ITEM_MISSILE] -= used;

            if (used > 1)
            {
                Set_message_f("%s has launched %s!", pl->name,
                              Describe_shot(type, status, mods, 0));
                sound_play_all(NUKE_LAUNCH_SOUND);
            }
            else if (type == OBJ_SMART_SHOT)
                sound_play_sensors(pl->pos, FIRE_SMART_SHOT_SOUND);
            else if (type == OBJ_TORPEDO)
                sound_play_sensors(pl->pos, FIRE_TORPEDO_SOUND);
        }
        break;
    }

    minis = (mods.mini + 1);
    speed *= (1 + (mods.power * MISSILE_POWER_SPEED_FACT));
    max_speed *= (1 + (mods.power * MISSILE_POWER_SPEED_FACT));
    turnspeed *= (1 + (mods.power * MISSILE_POWER_TURNSPEED_FACT));
    spread = (double)((unsigned)mods.spread + 1);
    /*
     * Calculate the maximum time it would take to cross one ships width,
     * don't fuse the shot/missile/torpedo for the owner only until that
     * time passes.  This is a hack to stop various odd missile and shot
     * mounting points killing the player when they're firing.
     */
    fuse += (int)((2.0 * (double)SHIP_SZ) / speed + 1.0);

    /*
     *                         Missile Racks and Spread
     *                         ------------------------
     *
     *                     A short story by H. J. Thompson
     *
     * Once upon a time, back in the "good old days" of XPilot, it was
     * relatively easy thing to remember the few keys needed to fly and shoot.
     * It was the day of Sopwith Camels biplanes, albeit triangular ones,
     * doing close to-the-death machine gun combat with other triangular
     * Red Barons, the hard vacuum of space whistling silently by as only
     * something that doesn't exist could do (this was later augmented by
     * artificial aural feedback devices on certain advanced hardware).
     *
     * Eventually the weapon designers came up with "smart" missiles, and
     * another key was added to the control board, causing one missile to
     * launch straight forwards from the front of the triangular ship.
     * Soon other types of missiles were added, including "heat" seekers,
     * and fast straight travelling "torpedoes" (hark, is that the sonorous
     * ping-ping-ping of sonar equipment I hear?).
     *
     * Then one day along came a certain fellow who thought, among other
     * things, that it would be neat to fire up to four missiles with one
     * key press, just so the enemy pilot would be scared witless by the
     * sudden appearance of four missiles hot on their tail.  To make things
     * fair these "mini" missiles would have the same total damage of a
     * normal missile, but would travel at the speed of a normal missile.
     *
     * However this fellow mused that simply launching all the missiles in
     * the same direction and from the same point would cause the missiles
     * to appear on top of each other.  Thus he added code to "spread" the
     * missiles out at various angular offsets from the ship.  Indeed the
     * angular offsets could be controlled using a spread modifier, and yet
     * more keys appeared on a now crowded control desk.
     *
     * Interestingly the future would see the same fellow adding a two seater
     * variant of the standard single seater ship, allowing one person
     * to concentrate on flying the ship, while another could flick through
     * out-of-date manuals searching for the right key combinations on
     * the now huge console which would launch four full nuclear slow-cluster
     * imploding mini super speed close spread torpedoes at the currently
     * targetted enemy, and then engage emergency thrust and shields before
     * the ominous looking tri-winged dagger ship recoiled at high velocity
     * into a rocky wall half way across the other side of the universe.
     *
     * Back to our story, and this same fellow was musing at the design of
     * multiple "mini" missiles, and noted that the angle of launch would
     * also require a different launch point on the ship (actually it was
     * the same position as if the front of the ship was rotated to point in
     * the direction of missile launch, mainly because it was easier to
     * write the launch/guidance computer software that way).
     *
     * Later, some artistically (or sadistically) minded person decided that
     * triangular ships just didn't look good (even though they were very
     * spatially dynamic, cheap and easy to build), and wouldn't it be just
     * fantastic if one could have a ship shaped like a banana!  Sensibly,
     * however, he restricted missiles and guns to the normal single frontal
     * launching point.
     *
     * A few weeks later, somebody else decided how visually pleasing it
     * would be if one could design where missiles could be fired from by
     * adding "missile rack" points on the ship.  Up to four racks were
     * available, and missiles would fire from exactly these points on the
     * ship.  Since one to four missiles could be fired in one go, the
     * combinations with various ship designs were numerous (16).
     *
     * What would happen if somebody fired four missiles in one go, from a
     * ship that only had three missile racks?  How about two missiles from
     * one with four racks?  Sadly the missile launch software hadn't been
     * designed to take this sort of thing into account, and incredibly the
     * original programmer wasn't notified until after First Customer Ship
     * [sic], the launch software only slightly modified by the ship
     * designer, who didn't know the first thing about launch acceleration
     * curves or electronic owner immunity fuse timers.
     *
     * Pilots found their missiles were being fired from random points and
     * in sometimes very odd directions, occasionally even destroying the
     * ship without trace, severely annoying the ship's owners and several
     * insurance underwriters.  Not soon after several ship designers were
     * mysteriously killed in a freak "accident" involving a stray nuclear
     * cluster bomb, and the remaining ship designers became very careful
     * to place missile racks and extra gun turrets well away from the
     * ship's superstructure.
     *
     * The original programmer who invented multiple "mini" spreading
     * missiles quickly decided to revisit his code before any "accidents"
     * came his way, and spent a good few hours making sure one couldn't
     * shoot oneself in the "foot", and that missiles where launched in some
     * reasonable and sensible directions based on the position of the
     * missile racks.
     *
     *                         How It Actually Works
     *                        ---------------------
     *
     * The first obstacle is getting the right number of missiles fired
     * from each combination of missile rack configurations;
     *
     *
     *                Minis        1        2        3        4
     * Racks
     *        1                1        2        3        4
     *
     *        2                1/-        1/1        2/1        2/2
     *                        -/1                1/2
     *
     *        3                1/-/-        1/1/-        1/1/1        2/1/1
     *                        -/1/-        -/1/1                1/2/1
     *                        -/-/1        1/-/1                1/1/2
     *
     *        4                1/-/-/-        1/1/-/-        1/1/1/-        1/1/1/1
     *                        -/1/-/-        -/1/1/-        -/1/1/1
     *                        -/-/1/-        -/-/1/1        1/-/1/1
     *                        -/-/-/1 1/-/-/1        1/1/-/1
     *
     * To read; For example with 2 Minis and 3 Racks, the first round will
     * fire 1/1/-, which is one missile from left and middle racks.  The
     * next time fired will be -/1/1; middle and right, next fire is
     * 1/-/1; left and right.  Next time goes to the beggining state.
     *
     *                         Comment Point 1
     *                        ---------------
     *
     * The *starting* rack number for each salvo cycles through the number
     * of missiles racks.  This is stored in the player variable
     * 'pl->missile_rack', and is only incremented after each salvo (not
     * after each mini missile is fired).  This value is used to initialise
     * 'rack_no', which stores the current rack that missiles are fired from.
     *
     * 'on_this_rack' is computed to be the number of missiles that will be
     * fired from 'rack_no', and 'r' is used as a counter to this value.
     *
     * 'racks_left' count how many unused missiles racks are left on the ship
     * in this mini missile salvo.
     *
     *                         Comment Point 2
     *                        ---------------
     *
     * When 'r' reaches 'on_this_rack' all the missiles have been fired for
     * this rack, and the next rack should be used.  'rack_no' is incremented
     * modulo the number of available racks, and 'racks_left' is decremented.
     * At this point 'on_this_rack' is recomputed for the next rack, and 'r'
     * reset to zero.  Thus initially these two variables are both zero, and
     * 'rack_no' is one less, such that these variables can be computed inside
     * the loop to make the code simpler.
     *
     * The computation of 'on_this_rack' is as follows;  Given that there
     * are M missiles and R racks remaining;
     *
     *        on_this_rack = int(M / R);        (ie. round down to lowest int)
     *
     * Then;
     *
     *        (M - on_this_rack) / (R - 1) < (M / R).
     *
     * That is, the number of missiles fired on the next rack will be
     * more precise, and trivially can be seen that when R is 1, will
     * give an exact number of missiles to fire on the last rack.
     *
     * In the code 'M' is (minis - i), and 'R' is racks_left.
     *
     *                        Comment Point 3
     *                        ---------------
     *
     * In order that multiple missiles fired from one rack do not conincide,
     * each missile has to be "spread" based on the number of missiles
     * fired from this rack point.
     *
     * This is computed similar to the wide shot code;
     *
     *        angle = (N - 1 - 2 * i) / (N - 1)
     *
     * Where N is the number of shots/missiles to be fired, and i is a counter
     * from 0 .. N-1.
     *
     *                 i        0        1        2        3
     * N
     * 1                0
     * 2                1        -1
     * 3                1        0        -1
     * 4                1        0.333        -0.333        -1
     *
     * In this code 'N' is 'on_this_rack'.
     *
     * Also the position of the missile rack from the center line of the
     * ship (stored in 'side') has a linear effect on the angle, such that
     * a point farthest from the center line contributes the largest angle;
     *
     * angle += (side / SHIP_SZ)
     *
     * Since the eventual 'angle' value used in the code should be a
     * percentage of the unmodified launch angle, it should be ranged between
     * -1.00 and +1.00, and thus the first angle is reduced by 33% and the
     * second by 66%.
     *
     * Contact: harveyt@sco.com
     */

    if (pl && type != OBJ_SHOT)
    {
        /*
         * Initialise missile rack spread variables. (See Comment Point 1)
         */
        on_this_rack = 0;
        racks_left = pl->ship->num_m_rack;
        rack_no = pl->missile_rack - 1;
        if (++pl->missile_rack >= pl->ship->num_m_rack)
            pl->missile_rack = 0;
    }

    for (r = 0, i = 0; i < minis; i++, r++)
    {
        object_t *shot;

        if ((shot = Object_allocate()) == NULL)
            break;

        shot->life = life / minis;
        shot->fuselife = shot->life - fuse;
        shot->mass = mass / minis;
        shot->type = type;
        shot->id = (pl ? pl->id : NO_ID);
        shot->team = team;
        shot->color = (pl ? pl->color : WHITE);

        if (shot->type == OBJ_TORPEDO)
        {
            torpobject_t *torp = TORP_PTR(shot);
            torp->missile_turnspeed = turnspeed;
            torp->missile_max_speed = max_speed;
            torp->info = lock;
            torp->count = 0;
        }

        else if (shot->type == OBJ_HEAT_SHOT)
        {
            heatobject_t *heat = HEAT_PTR(shot);
            heat->missile_turnspeed = turnspeed;
            heat->missile_max_speed = max_speed;
            heat->info = lock;
            heat->count = 0;
        }

        else if (shot->type == OBJ_SMART_SHOT)
        {
            smartobject_t *smart = SMART_PTR(shot);
            smart->missile_turnspeed = turnspeed;
            smart->missile_max_speed = max_speed;
            smart->info = lock;
            smart->count = 0;
        }

        else
        {
            // TODO: do we need any of this?
            // warn("ERROR: minis: shot type %s", Object_typename(shot));
            // shot->info = lock;
            // shot->count = 0;
        }

        shotpos = pos;
        if (pl && type != OBJ_SHOT)
        {
            if (r == on_this_rack)
            {
                /*
                 * We've fired all the mini missiles for the current rack,
                 * we now move onto the next one. (See Comment Point 2)
                 */
                on_this_rack = (minis - i) / racks_left--;
                if (on_this_rack < 1)
                    on_this_rack = 1;
                if (++rack_no >= pl->ship->num_m_rack)
                    rack_no = 0;
                r = 0;
            }
            clpos_t m_rack;
            m_rack = Ship_get_m_rack_clpos(pl->ship, rack_no, pl->dir);
            shotpos.cx += m_rack.cx;
            shotpos.cy += m_rack.cy;
            side = CLICK_TO_PIXEL(
                Ship_get_m_rack_clpos(pl->ship, rack_no, 0).cy);
        }
        shotpos = World_wrap_clpos(shotpos);
        if (shotpos.cx < 0 || shotpos.cx >= world->cwidth || shotpos.cy < 0 || shotpos.cy >= world->cheight)
            continue;

        Object_position_init_clpos(shot, shotpos);

        if (type == OBJ_SHOT || !pl)
            angle = 0.0;
        else
        {
            /*
             * Calculate the percentage unmodified launch angle for missiles.
             * (See Comment Point 3).
             */
            if (on_this_rack <= 1)
                angle = 0.0;
            else
            {
                angle = (double)(on_this_rack - 1 - 2 * r);
                angle /= (3.0 * (double)(on_this_rack - 1));
            }
            angle += (double)(2 * side) / (double)(3 * SHIP_SZ);
        }

        /*
         * Torpedoes spread like mines, except the launch direction
         * is preset over the range +/- MINI_TORPEDO_SPREAD_ANGLE.
         * (This is not modified by the spread, the initial velocity is)
         *
         * Other missiles are just launched in a different direction
         * which varies over the range +/- MINI_MISSILE_SPREAD_ANGLE,
         * which the spread modifier varies.
         */
        switch (type)
        {
        case OBJ_TORPEDO:
            torp = TORP_PTR(shot);

            angle *= (MINI_TORPEDO_SPREAD_ANGLE / 360.0) * RES;
            ldir = MOD2(dir + (int)angle, RES);
            mv.x = MINI_TORPEDO_SPREAD_SPEED * tcos(ldir) / spread;
            mv.y = MINI_TORPEDO_SPREAD_SPEED * tsin(ldir) / spread;
            /*
             * This causes the added initial velocity to reduce to
             * zero over the MINI_TORPEDO_SPREAD_TIME.
             * FIX: torpedoes should have the same speed
             *      regardless of minification.
             */
            torp->torp_spread_left = MINI_TORPEDO_SPREAD_TIME;
            torp->acc.x = -mv.x / MINI_TORPEDO_SPREAD_TIME;
            torp->acc.y = -mv.y / MINI_TORPEDO_SPREAD_TIME;
            ldir = dir;
            break;

        default:
            angle *= (MINI_MISSILE_SPREAD_ANGLE / 360.0) * RES / spread;
            ldir = MOD2(dir + (int)angle, RES);
            mv.x = mv.y = shot->acc.x = shot->acc.y = 0;
            break;
        }

        shot->vel.x = mv.x + (pl ? pl->vel.x : 0.0) + tcos(ldir) * speed;
        shot->vel.y = mv.y + (pl ? pl->vel.y : 0.0) + tsin(ldir) * speed;
        shot->obj_status = status;
        shot->missile_dir = ldir;
        shot->mods = mods;
        shot->pl_range = pl_range;
        shot->pl_radius = pl_radius;
        Cell_add_object(shot);

        mini_objs[fired] = shot;
        fired++;
    }

    /*
     * Recoil must be done instantaneously otherwise ship moves back after
     * firing each mini missile.
     */
    if (pl)
    {
        for (i = 0; i < fired; i++)
            Recoil((object_t *)pl, mini_objs[i]);
    }
}

void Fire_normal_shots(player_t *pl)
{
    int i, shot_angle;

    if (frame_loops < pl->shot_time + options.fireRepeatRate)
        return;
    pl->shot_time = frame_loops;

    shot_angle = MODS_SPREAD_MAX - pl->mods.spread;

    Fire_main_shot(pl, OBJ_SHOT, pl->dir);
    for (i = 0; i < pl->item[ITEM_WIDEANGLE]; i++)
    {
        if (pl->ship->num_l_gun > 0)
        {
            Fire_left_shot(pl, OBJ_SHOT, MOD2(pl->dir + (1 + i) * shot_angle, RES), i % pl->ship->num_l_gun);
        }
        else
        {
            Fire_main_shot(pl, OBJ_SHOT, MOD2(pl->dir + (1 + i) * shot_angle, RES));
        }
        if (pl->ship->num_r_gun > 0)
        {
            Fire_right_shot(pl, OBJ_SHOT, MOD2(pl->dir - (1 + i) * shot_angle, RES), i % pl->ship->num_r_gun);
        }
        else
        {
            Fire_main_shot(pl, OBJ_SHOT, MOD2(pl->dir - (1 + i) * shot_angle, RES));
        }
    }
    for (i = 0; i < pl->item[ITEM_REARSHOT]; i++)
    {
        if ((pl->item[ITEM_REARSHOT] - 1 - 2 * i) < 0)
        {
            if (pl->ship->num_l_rgun > 0)
            {
                Fire_left_rshot(pl, OBJ_SHOT, MOD2(pl->dir + RES / 2 + ((pl->item[ITEM_REARSHOT] - 1 - 2 * i) * shot_angle) / 2, RES), (i - (pl->item[ITEM_REARSHOT] + 1) / 2) % pl->ship->num_l_rgun);
            }
            else
            {
                Fire_shot(pl, OBJ_SHOT, MOD2(pl->dir + RES / 2 + ((pl->item[ITEM_REARSHOT] - 1 - 2 * i) * shot_angle) / 2, RES));
            }
        }
        if ((pl->item[ITEM_REARSHOT] - 1 - 2 * i) > 0)
        {
            if (pl->ship->num_r_rgun > 0)
            {
                Fire_right_rshot(pl, OBJ_SHOT, MOD2(pl->dir + RES / 2 + ((pl->item[ITEM_REARSHOT] - 1 - 2 * i) * shot_angle) / 2, RES), (pl->item[ITEM_REARSHOT] / 2 - i - 1) % pl->ship->num_r_rgun);
            }
            else
            {
                Fire_shot(pl, OBJ_SHOT, MOD2(pl->dir + RES / 2 + ((pl->item[ITEM_REARSHOT] - 1 - 2 * i) * shot_angle) / 2, RES));
            }
        }
        if ((pl->item[ITEM_REARSHOT] - 1 - 2 * i) == 0)
            Fire_shot(pl, OBJ_SHOT, MOD2(pl->dir + RES / 2 + ((pl->item[ITEM_REARSHOT] - 1 - 2 * i) * shot_angle) / 2, RES));
    }
}

/* Removes shot from array */
void Delete_shot(int ind)
{
    // warn("Delete_shot: ind = %d", ind);

    object_t *shot = Obj[ind]; /* Used when swapping places */
    ballobject_t *ball;
    itemobject_t *item;
    player_t *pl;
    bool addMine = false, addHeat = false, addBall = false;
    modifiers_t mods;
    int i, intensity, type, color, num_debris, status;
    double modv, speed_modv, life_modv, num_modv, mass, min_life, max_life;

    warn("Delete_shot: shot->type = %d", shot->type);

    switch (shot->type)
    {

    case OBJ_SPARK:
    case OBJ_DEBRIS:
    case OBJ_WRECKAGE:
        break;

    case OBJ_ASTEROID:
        Break_asteroid(WIRE_PTR(shot));
        break;

    case OBJ_BALL:
        // warn("Delete_shot: BALL!");
        ball = BALL_PTR(shot);
        warn("Delete_shot: Ball with id %d", ball->id);
        if (ball->id != NO_ID)
        {
            player_t *ball_pl = Player_by_id(ball->id);
            warn("Delete_shot: Ball pl %p", ball_pl);
            Detach_ball(ball_pl, ball);
        }
        else
        {
            warn("Delete_shot: Ball->id is NO_ID");
            /*
             * Maybe some player is still busy trying to connect to this ball.
             */
            for (i = 0; i < NumPlayers; i++)
            {
                player_t *pl_i = Player_by_index(i);

                if (pl_i->ball == ball)
                    pl_i->ball = NULL;
            }
        }
        warn("Delete_shot: Ball->ball_owner id is %d", ball->ball_owner);
        if (ball->ball_owner == NO_ID)
        {
            /*
             * If the ball has never been owned, the only way it could
             * have been destroyed is by being knocked out of the goal.
             * Therefore we force the ball to be recreated.
             */
            warn("Delete_shot: Ball ball->ball_treasure is %p", ball->ball_treasure);
            ball->ball_treasure->have = false;
            warn("Delete_shot: Ball set bit RECREATE");
            SET_BIT(ball->obj_status, RECREATE);
            warn("Delete_shot: Ball set bit RECREATE done");
        }
        if (BIT(ball->obj_status, RECREATE))
        {
            warn("Delete_shot: Ball RECREATE");
            addBall = true;
            if (BIT(ball->obj_status, NOEXPLOSION))
                break;
            sound_play_sensors(ball->pos, EXPLODE_BALL_SOUND);
            Make_debris(ball->prevpos,
                        ball->vel,
                        ball->id,
                        ball->team,
                        OBJ_DEBRIS,
                        DEBRIS_MASS,
                        GRAVITY,
                        RED,
                        8,
                        10, 20,
                        (int)(10 + 10 * rfrac()),
                        0, RES - 1,
                        10.0, 50.0,
                        10.0, 54.0);
        }
        break;
        /* Shots related to a player. */

    case OBJ_MINE:
    case OBJ_HEAT_SHOT:
    case OBJ_TORPEDO:
    case OBJ_SMART_SHOT:
    case OBJ_CANNON_SHOT:
        if (shot->mass == 0)
            break;

        status = GRAVITY;
        if (shot->type == OBJ_MINE)
            status |= COLLISIONSHOVE;
        if (BIT(shot->obj_status, FROMCANNON))
            status |= FROMCANNON;

        if (BIT(shot->mods.nuclear, MODS_NUCLEAR))
            sound_play_all(NUKE_EXPLOSION_SOUND);

        else if (shot->type == OBJ_MINE)
            sound_play_sensors(shot->pos, MINE_EXPLOSION_SOUND);
        else
            sound_play_sensors(shot->pos, OBJECT_EXPLOSION_SOUND);

        if (BIT(shot->mods.warhead, CLUSTER))
        {
            type = OBJ_SHOT;
            if (shot->id != NO_ID)
            {
                player_t *pl = Player_by_id(shot->id);
                color = pl->color;
            }
            else
                color = WHITE;

            mass = options.shotMass * 3;
            modv = 1 << shot->mods.velocity;
            num_modv = 4;
            if (BIT(shot->mods.nuclear, MODS_NUCLEAR))
            {
                modv *= 4.0;
                num_modv = 1;
            }
            life_modv = modv * 0.20;
            speed_modv = 1.0 / modv;
            intensity = (int)CLUSTER_MASS_SHOTS(shot->mass);
        }
        else
        {
            type = OBJ_DEBRIS;
            color = RED;
            mass = DEBRIS_MASS;
            modv = 1;
            num_modv = 1;
            life_modv = modv;
            speed_modv = modv;
            if (shot->type == OBJ_MINE)
                intensity = 512;
            else
                intensity = 32;
            /*
             * Writing it like this:
             *   num_modv /= (shot->mods.mini + 1);
             * triggers a bug in HP C A.09.19.
             */
            num_modv = num_modv / ((double)(unsigned)shot->mods.mini + 1.0);
        }

        if (BIT(shot->mods.nuclear, MODS_NUCLEAR))
        {
            double nuke_factor;
            if (shot->type == OBJ_MINE)
                nuke_factor = NUKE_MINE_EXPL_MULT * shot->mass / MINE_MASS;
            else
                nuke_factor = NUKE_SMART_EXPL_MULT * shot->mass / MISSILE_MASS;
            nuke_factor = (nuke_factor * (shot->mods.mini + 1)) / SHOT_MULT(shot);
            intensity = (int)(intensity * nuke_factor);
        }

        if (BIT(shot->mods.warhead, IMPLOSION))
            /*intensity >>= 1;*/
            mass = -mass;

        if (BIT(shot->type, OBJ_TORPEDO_BIT | OBJ_HEAT_SHOT_BIT | OBJ_SMART_SHOT_BIT))
            intensity /= (1 + shot->mods.power);

        num_debris = (int)(intensity * num_modv * (0.20 + (0.10 * rfrac())));
        min_life = 8 * life_modv;
        max_life = (intensity >> 1) * life_modv;

#if 0
    warn("type = %16s (%c%c) inten = %-6d num = %-6d life: %.1f - %.1f",
         Object_typename(shot),
         (Mods_get(shot->mods, ModsNuclear) ? 'N' : '-'),
         (Mods_get(shot->mods, ModsCluster) ? 'C' : '-'),
         intensity, num_debris, min_life, max_life);
#endif

        Make_debris(shot->prevpos,
                    shot->vel,
                    shot->id,
                    shot->team,
                    type,
                    mass,
                    status,
                    color,
                    6,
                    (int)(0.20f * intensity * num_modv),
                    (int)(0.30f * intensity * num_modv),
                    num_debris,
                    0, RES - 1,
                    20 * speed_modv, (intensity >> 2) * speed_modv,
                    (int)min_life,
                    (int)max_life);
        break;

    case OBJ_SHOT:
        if (shot->id == NO_ID || BIT(shot->obj_status, FROMCANNON) || BIT(shot->mods.warhead, CLUSTER))
            break;
        pl = Player_by_id(shot->id);
        if (--pl->shots <= 0)
            pl->shots = 0;
        break;

        /* Special items. */
    case OBJ_ITEM:
        item = ITEM_PTR(shot);
        if (item->info != item->item_type)
        {
            warn("Delete_shot: shot->info != item->item_type, item->info = %ld, item->item_type = %d",
                 item->info, item->item_type);
        }

        switch (item->item_type)
        {
        case ITEM_MISSILE:
            if (shot->life == 0 && shot->color != WHITE)
            {
                shot->color = WHITE;
                shot->life = FPS * WARN_TIME;
                return;
            }
            if (shot->life == 0 && rfrac() < options.rogueHeatProb)
                addHeat = 1;
            break;

        case ITEM_MINE:
            if (!shot->life && shot->color != WHITE)
            {
                shot->color = WHITE;
                shot->life = FPS * WARN_TIME;
                return;
            }
            if (shot->life == 0 && rfrac() < options.rogueMineProb)
                addMine = 1;
            break;
        }

        if (item->info != item->item_type)
            warn("item->info != item->item_type");
        world->items[item->item_type].num--;

        break;

    default:
        xpprintf("%s Delete_shot(): Unkown shot type %d.\n",
                 showtime(), shot->type);
        printf("%s Delete_shot(): Unkown shot type %d.\n",
               showtime(), shot->type);
        break;
    }

    Cell_remove_object(shot);
    shot->life = 0;
    shot->type = 0;
    shot->mass = 0;

    Object_free_ind(ind);

    if (addMine || addHeat)
    {
        CLEAR_MODS(mods);
        if (BIT(world->rules->mode, ALLOW_CLUSTERS) && (rfrac() <= 0.333))
            SET_BIT(mods.warhead, CLUSTER);

        if (BIT(world->rules->mode, ALLOW_MODIFIERS) && (rfrac() <= 0.333))
            SET_BIT(mods.warhead, IMPLOSION);

        if (BIT(world->rules->mode, ALLOW_MODIFIERS))
            mods.velocity = (int)(rfrac() * (MODS_VELOCITY_MAX + 1));

        if (BIT(world->rules->mode, ALLOW_MODIFIERS))
            mods.power = (int)(rfrac() * (MODS_POWER_MAX + 1));

        if (addMine)
        {
            long gravity_status = ((rfrac() < 0.5) ? GRAVITY : 0);
            vector_t zero_vel = {0.0, 0.0};

            Place_general_mine(NO_ID, TEAM_NOT_SET, gravity_status,
                               shot->pos, zero_vel, mods);
        }
        else if (addHeat)
            Fire_general_shot(NO_ID, TEAM_NOT_SET, true, shot->pos,
                              OBJ_HEAT_SHOT, (int)(rfrac() * RES),
                              mods, NO_ID);
    }
    else if (addBall)
    {
        ball = BALL_PTR(shot);
        Make_treasure_ball(ball->ball_treasure);
    }
}

void Fire_laser(player_t *pl)
{
    if (pl->item[ITEM_LASER] > pl->num_pulses && pl->velocity < PULSE_SPEED - PULSE_SAMPLE_DISTANCE)
    {
        if (pl->fuel.sum <= -ED_LASER)
            CLR_BIT(pl->used, HAS_LASER);
        else
        {
            clpos_t pos;
            clpos_t m_gun = Ship_get_m_gun_clpos(pl->ship, pl->dir);
            pos.cx = pl->pos.cx + m_gun.cx + FLOAT_TO_CLICK(pl->vel.x);
            pos.cy = pl->pos.cy + m_gun.cy + FLOAT_TO_CLICK(pl->vel.y);
            pos.cx = WRAP_XCLICK(pos.cx);
            pos.cy = WRAP_YCLICK(pos.cy);
            if (World_contains_clpos(pos))
                Fire_general_laser(pl->id, pl->team, pos, pl->dir, pl->mods);
        }
    }
}

void Fire_general_laser(int id, int team, clpos_t pos, int dir,
                        modifiers_t mods)
{
    pulse_t *pulse;
    int life;
    player_t *pl = Player_by_id(id);
    /*cannon_t *cannon = Cannon_by_id(id);*/

    if (pl)
    {
        Player_add_fuel(pl, ED_LASER);
        sound_play_sensors(pos, FIRE_LASER_SOUND);
        life = (int)PULSE_LIFE(pl->item[ITEM_LASER]);
    }
    else
        life = (int)PULSE_LIFE(CANNON_PULSES);

    if (NumPulses >= MAX_TOTAL_PULSES)
        return;
    Pulses[NumPulses] = (pulse_t *)malloc(sizeof(pulse_t));
    if (Pulses[NumPulses] == NULL)
        return;

    pulse = Pulses[NumPulses];
    pulse->id = (pl ? pl->id : NO_ID);
    pulse->team = team;
    pulse->dir = dir;
    pulse->len = PULSE_LENGTH;
    pulse->life = life;
    pulse->mods = mods;
    pulse->refl = false;
    pulse->pix_pos.x = CLICK_TO_FLOAT(pos.cx) - PULSE_SPEED * tcos(dir);
    pulse->pix_pos.y = CLICK_TO_FLOAT(pos.cy) - PULSE_SPEED * tsin(dir);
    NumPulses++;
    if (pl)
        pl->num_pulses++;
}

void Update_connector_force(ballobject_t *ball)
{
    /*
     * The new ball movement code since XPilot version 3.4.0 as made
     * by Bretton Wade.  The code was submitted in context diff format
     * by Mark Boyns.  Here is a an excerpt from a post in
     * rec.games.computer.xpilot by Bretton Wade dated 27 Jun 1995:
     *
     *     If I'm not mistaken (not having looked very closely at the code
     *     because I wasn't sure what it was trying to do), the original move_ball
     *     routine was trying to model a Hook's law spring, but squared the
     *     deformation term, which would lead to exagerated behavior as the spring
     *     stretched too far. Not really a divide by zero, but effectively producing
     *     large numbers.
     *
     *     When I coded up the spring myself, I found that I could recreate the
     *     effect by using a VERY strong spring. This can be defeated, however, by
     *     damping. Specifically, If you compute the critical damping factor, then
     *     you could have the cable always be the correct length. This makes me
     *     wonder how to decide when the cable snaps.
     *
     *     I chose a relatively strong spring, and a small damping factor, to make
     *     for a nice realistic bounce when you grab at the treasure. It also gives a
     *     fairley close approximation to the "normal" feel of the treasure.
     *
     *     I modeled the cable as having zero mass, or at least insignificant mass as
     *     compared to the ship and ball. This greatly simplifies the math, and leads
     *     to the conclusion that there will be no change in velocity when the cable
     *     breaks. You can check this by integrating the momentum along the cable,
     *     and the ship or ball.
     *
     *     If you assume that the cable snaps in the middle, then half of the
     *     potential energy goes to each object attached. However, as you said, the
     *     total momentum of the system cannot change. Because the weight of the
     *     cable is small, the vast majority of the potential energy will become
     *     heat. I've had two physicists verify this for me, and they both worked
     *     really hard on the problem because they found it interesting.
     *
     * End of post.
     *
     * Changes since then:
     *
     * Comment from people was that the string snaps too soon.
     * Changed the value (max_spring_ratio) at which the string snaps
     * from 0.25 to 0.30.  Not sure if that helps enough, or too much.
     */

    player_t *pl = Player_by_id(ball->id);
    vector_t D;
    double length, force, ratio, accell, cosine;
    double pl_damping, ball_damping;
    /* const double                k = 1500.0, b = 2.0; */
    /* const double                max_spring_ratio = 0.30; */

    /* no player connected ? */
    if (!pl)
        return;

    /* compute the normalized vector between the ball and the player */
    D.x = WRAP_DX(pl->pix_pos.x - ball->pix_pos.x);
    D.y = WRAP_DY(pl->pix_pos.y - ball->pix_pos.y);
    length = VECTOR_LENGTH(D);
    if (length > 0.0)
    {
        D.x /= length;
        D.y /= length;
    }
    else
        D.x = D.y = 0.0;

    /* compute the ratio for the spring action */
    ratio = (options.ballConnectorLength - length) / options.ballConnectorLength;

    /* compute force by spring for this length */
    force = options.ballConnectorSpringConstant * ratio;

    /* If we have string-style connectors then it is allowed to be
     * shorted than its natural length. */
    if (options.connectorIsString && ratio > 0.0)
        return;

    /* if the tether is too long or too short, release it */
    if (ABS(ratio) > options.maxBallConnectorRatio)
    {
        Detach_ball(pl, ball);
        return;
    }
    ball->length = length;

    /* compute damping for player */
    cosine = (pl->vel.x * D.x) + (pl->vel.y * D.y);
    pl_damping = -options.ballConnectorDamping * cosine;

    /* compute damping for ball */
    cosine = (ball->vel.x * -D.x) + (ball->vel.y * -D.y);
    ball_damping = -options.ballConnectorDamping * cosine;

    /* compute accelleration for player, assume t = 1 */
    accell = (force + pl_damping + ball_damping) / pl->mass;
    pl->vel.x += D.x * accell;
    pl->vel.y += D.y * accell;

    /* compute accelleration for ball, assume t = 1 */
    accell = (force + ball_damping + pl_damping) / ball->mass;
    ball->vel.x += -D.x * accell;
    ball->vel.y += -D.y * accell;
}

void Update_torpedo(torpobject_t *torp)
{
    double acc;
    if (BIT(torp->mods.nuclear, MODS_NUCLEAR))
        acc = (torp->info++ < NUKE_SPEED_TIME) ? NUKE_ACC : 0.0;
    else
        acc = (torp->info++ < TORPEDO_SPEED_TIME) ? TORPEDO_ACC : 0.0;
    acc *= (1 + (torp->mods.power * MISSILE_POWER_SPEED_FACT));
    if (torp->torp_spread_left-- <= 0)
    {
        torp->acc.x = 0;
        torp->acc.y = 0;
        torp->torp_spread_left = 0;
    }
    torp->vel.x += acc * tcos(torp->missile_dir);
    torp->vel.y += acc * tsin(torp->missile_dir);
}

void Update_missile(missileobject_t *missile)
{
    player_t *pl;
    int angle, theta;
    double range = 0.0;
    double acc;
    double x_dif = 0.0;
    double y_dif = 0.0;
    double shot_speed;

    acc = SMART_SHOT_ACC;

    if (missile->type == OBJ_HEAT_SHOT)
    {
        heatobject_t *heat = HEAT_PTR(missile);

        acc = SMART_SHOT_ACC * HEAT_SPEED_FACT;
        if (heat->info >= 0)
        {
            clpos_t engine;

            /* Get player and set min to distance */
            pl = Player_by_id(heat->info);
            if (!pl)
                return;
            engine = Ship_get_engine_clpos(pl->ship, pl->dir);
            range = Wrap_length(pl->pos.cx + engine.cx - heat->pos.cx,
                                pl->pos.cy + engine.cy - heat->pos.cy) /
                    CLICK;
        }
        else
        {
            /* No player. Number of moves so that new target is searched */
            pl = 0;
            heat->count = HEAT_WIDE_TIMEOUT + HEAT_WIDE_ERROR;
        }
        if (pl && Player_is_thrusting(pl))
        {
            /*
             * Target is thrusting,
             * set number to moves to correct error value
             */
            if (range < HEAT_CLOSE_RANGE)
                heat->count = HEAT_CLOSE_ERROR;
            else if (range < HEAT_MID_RANGE)
                heat->count = HEAT_MID_ERROR;
            else
                heat->count = HEAT_WIDE_ERROR;
        }
        else
        {
            heat->count++;
            /* Look for new target */
            if ((range < HEAT_CLOSE_RANGE && heat->count > HEAT_CLOSE_TIMEOUT + HEAT_CLOSE_ERROR) || (range < HEAT_MID_RANGE && heat->count > HEAT_MID_TIMEOUT + HEAT_MID_ERROR) || heat->count > HEAT_WIDE_TIMEOUT + HEAT_WIDE_ERROR)
            {
                double l;
                int i;

                range = HEAT_RANGE * (heat->count / HEAT_CLOSE_TIMEOUT);
                for (i = 0; i < NumPlayers; i++)
                {
                    player_t *pl_i = Player_by_index(i);
                    clpos_t engine;

                    if (!Player_is_thrusting(pl_i))
                        continue;

                    engine = Ship_get_engine_clpos(pl_i->ship, pl_i->dir);
                    l = Wrap_length(pl_i->pos.cx + engine.cx - heat->pos.cx,
                                    pl_i->pos.cy + engine.cy - heat->pos.cy) /
                        CLICK;
                    /*
                     * After burners can be detected easier;
                     * so scale the length:
                     */
                    l *= MAX_AFTERBURNER + 1 - pl_i->item[ITEM_AFTERBURNER];
                    l /= MAX_AFTERBURNER + 1;
                    if (BIT(pl_i->have, HAS_AFTERBURNER))
                        l *= 16 - pl_i->item[ITEM_AFTERBURNER];
                    if (l < range)
                    {
                        heat->info = Player_by_index(i)->id;
                        range = l;
                        heat->count =
                            l < HEAT_CLOSE_RANGE ? HEAT_CLOSE_ERROR : l < HEAT_MID_RANGE ? HEAT_MID_ERROR
                                                                                         : HEAT_WIDE_ERROR;
                        pl = pl_i;
                    }
                }
            }
        }
        if (heat->info < 0)
            return;
        /*
         * Heat seekers cannot fly exactly, if target is far away or thrust
         * isn't active.  So simulate the error:
         */
        x_dif = (int)(rfrac() * 4 * heat->count);
        y_dif = (int)(rfrac() * 4 * heat->count);
    }
    else if (missile->type == OBJ_SMART_SHOT)
    {
        smartobject_t *smart = SMART_PTR(missile);

        if (BIT(smart->obj_status, CONFUSED) && (!(frame_loops % CONFUSED_UPDATE_GRANULARITY) || smart->count == CONFUSED_TIME))
        {

            if (smart->count)
            {
                smart->info = PlayersArray[(int)(rfrac() * NumPlayers)]->id;
                smart->count--;
            }
            else
            {
                CLR_BIT(smart->obj_status, CONFUSED);

                /* range is percentage from center to periphery of ecm burst */
                range = (ECM_DISTANCE - smart->smart_ecm_range) / ECM_DISTANCE;
                range *= 100.0;

                /*
                 * range%        lock%
                 * 100                100
                 *  50                75
                 *   0                50
                 */
                if ((int)(rfrac() * 100) <= ((int)(range / 2) + 50))
                {
                    smart->info = smart->new_info;
                }
            }
        }
        pl = Player_by_id(smart->info);
    }
    else
    {
        /*NOTREACHED*/
        return;
    }

    /*
     * Use a little look ahead to fly more exact
     */
    acc *= (1 + (missile->mods.power * MISSILE_POWER_SPEED_FACT));
    if ((shot_speed = VECTOR_LENGTH(missile->vel)) < 1)
        shot_speed = 1;
    range = Wrap_length(pl->pos.cx - missile->pos.cx,
                        pl->pos.cy - missile->pos.cy) /
            CLICK;
    x_dif += pl->vel.x * (range / shot_speed);
    y_dif += pl->vel.y * (range / shot_speed);
    theta = (int)Wrap_findDir(pl->pix_pos.x + x_dif - missile->pix_pos.x,
                              pl->pix_pos.y + y_dif - missile->pix_pos.y);

    {
        double x, y, vx, vy;
        int i, xi, yi, j, freemax, k, foundw;
        static struct
        {
            int dx, dy;
        } sur[8] = {
            {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};

#define BLOCK_PARTS 2
        vx = missile->vel.x;
        vy = missile->vel.y;
        x = shot_speed / (BLOCK_SZ * BLOCK_PARTS);
        vx /= x;
        vy /= x;
        x = missile->pix_pos.x;
        y = missile->pix_pos.y;
        foundw = 0;

        for (i = SMART_SHOT_LOOK_AH; i > 0 && foundw == 0; i--)
        {
            xi = (int)((x += vx) / BLOCK_SZ);
            yi = (int)((y += vy) / BLOCK_SZ);
            if (BIT(world->rules->mode, WRAP_PLAY))
            {
                if (xi < 0)
                    xi += world->x;
                else if (xi >= world->x)
                    xi -= world->x;
                if (yi < 0)
                    yi += world->y;
                else if (yi >= world->y)
                    yi -= world->y;
            }
            if (xi < 0 || xi >= world->x || yi < 0 || yi >= world->y)
                break;

            switch (world->block[xi][yi])
            {
            case TARGET:
            case TREASURE:
            case FUEL:
            case FILLED:
            case REC_LU:
            case REC_RU:
            case REC_LD:
            case REC_RD:
            case CANNON:
                if (range > (SMART_SHOT_LOOK_AH - i) * (BLOCK_SZ / BLOCK_PARTS))
                {
                    if (shot_speed > SMART_SHOT_MIN_SPEED)
                        shot_speed -= acc * (SMART_SHOT_DECFACT + 1);
                }
                foundw = 1;
            }
        }

        i = ((int)(missile->missile_dir * 8 / RES) & 7) + 8;
        xi = OBJ_X_IN_BLOCKS(missile);
        yi = OBJ_Y_IN_BLOCKS(missile);

        for (j = 2, angle = -1, freemax = 0; j >= -2; --j)
        {
            int si, xt, yt;

            for (si = 1, k = 0; si >= -1; --si)
            {
                xt = xi + sur[(i + j + si) & 7].dx;
                yt = yi + sur[(i + j + si) & 7].dy;

                if (xt >= 0 && xt < world->x && yt >= 0 && yt < world->y)
                    switch (world->block[xt][yt])
                    {
                    case TARGET:
                    case TREASURE:
                    case FUEL:
                    case FILLED:
                    case REC_LU:
                    case REC_RU:
                    case REC_LD:
                    case REC_RD:
                    case CANNON:
                        if (!si)
                            k = -32;
                        break;
                    default:
                        ++k;
                        break;
                    }
            }
            if (k > freemax || (k == freemax && ((j == -1 && (rfrac() < 0.5)) || j == 0 || j == 1)))
            {
                freemax = k > 2 ? 2 : k;
                angle = i + j;
            }

            if (k == 3 && !j)
            {
                angle = -1;
                break;
            }
        }

        if (angle >= 0)
        {
            i = angle & 7;
            theta = (int)Wrap_findDir((yi + sur[i].dy) * BLOCK_SZ - (missile->pix_pos.y + 2 * missile->vel.y),
                                      (xi + sur[i].dx) * BLOCK_SZ - (missile->pix_pos.x - 2 * missile->vel.x));
#ifdef SHOT_EXTRA_SLOWDOWN
            if (!foundw && range > (SHOT_LOOK_AH - i) * BLOCK_SZ)
            {
                if (shot_speed > (SMART_SHOT_MIN_SPEED + SMART_SHOT_MAX_SPEED) / 2)
                    shot_speed -= SMART_SHOT_DECC + SMART_SHOT_ACC;
            }
#endif
        }
    }
    angle = theta;

    if (angle < 0)
        angle += RES;
    angle %= RES;

    if (angle < missile->missile_dir)
        angle += RES;
    angle = angle - missile->missile_dir - RES / 2;

    if (angle < 0)
        missile->missile_dir += (uint8_t)(((-angle < missile->missile_turnspeed)
                                               ? -angle
                                               : missile->missile_turnspeed));
    else
        missile->missile_dir -= (uint8_t)(((angle < missile->missile_turnspeed)
                                               ? angle
                                               : missile->missile_turnspeed));

    missile->missile_dir = MOD2(missile->missile_dir, RES); /* NOTE!!!! */

    if (shot_speed < missile->missile_max_speed)
        shot_speed += acc;

    /*  missile->velocity = MIN(missile->velocity, missile->max_speed);  */

    missile->vel.x = tcos(missile->missile_dir) * shot_speed;
    missile->vel.y = tsin(missile->missile_dir) * shot_speed;
}

void Update_mine(mineobject_t *mine)
{
    if (BIT(mine->obj_status, CONFUSED) && --mine->count <= 0)
        CLR_BIT(mine->obj_status, CONFUSED);

    if (BIT(mine->obj_status, OWNERIMMUNE) && mine->info)
    {
        if (--mine->info <= 0)
            CLR_BIT(mine->obj_status, OWNERIMMUNE);
    }

    if (mine->mods.mini && mine->mine_spread_left-- <= 0)
    {
        mine->acc.x = 0;
        mine->acc.y = 0;
        mine->mine_spread_left = 0;
    }
}
