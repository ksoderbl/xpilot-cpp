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
#include <climits>

#include "const.h"

#include "cell.h"
#include "frame.h"
#include "ship.h"
#include "server.h"
#include "update.h"
#include "walls.h"

// #include "xpconfig.h"
#include "serverconst.h"

#include "saudio.h"
#include "score.h"
#include "netserver.h"
#include "xperror.h"
#include "commonproto.h"
#include "robot.h"

static void Item_update_flags(player_t *pl)
{
    if (pl->item[ITEM_CLOAK] <= 0 && BIT(pl->have, HAS_CLOAKING_DEVICE))
    {
        CLR_BIT(pl->have, HAS_CLOAKING_DEVICE);
        pl->updateVisibility = true;
    }
    if (pl->item[ITEM_MIRROR] <= 0)
        CLR_BIT(pl->have, HAS_MIRROR);
    if (pl->item[ITEM_DEFLECTOR] <= 0)
        CLR_BIT(pl->have, HAS_DEFLECTOR);
    if (pl->item[ITEM_AFTERBURNER] <= 0)
        CLR_BIT(pl->have, HAS_AFTERBURNER);
    if (pl->item[ITEM_PHASING] <= 0 && !Player_is_phasing(pl) && pl->phasing_left <= 0)
        CLR_BIT(pl->have, HAS_PHASING_DEVICE);
    if (pl->item[ITEM_EMERGENCY_THRUST] <= 0 && !BIT(pl->used, HAS_EMERGENCY_THRUST) && pl->emergency_thrust_left <= 0)
        CLR_BIT(pl->have, HAS_EMERGENCY_THRUST);
    if (pl->item[ITEM_EMERGENCY_SHIELD] <= 0 && !BIT(pl->used, HAS_EMERGENCY_SHIELD) && pl->emergency_shield_left <= 0)
    {
        if (BIT(pl->have, HAS_EMERGENCY_SHIELD))
        {
            CLR_BIT(pl->have, HAS_EMERGENCY_SHIELD);
            if (!BIT(DEF_HAVE, HAS_SHIELD) && pl->shield_time <= 0)
            {
                CLR_BIT(pl->have, HAS_SHIELD);
                CLR_BIT(pl->used, HAS_SHIELD);
            }
        }
    }
    if (pl->item[ITEM_TRACTOR_BEAM] <= 0)
        CLR_BIT(pl->have, HAS_TRACTOR_BEAM);
    if (pl->item[ITEM_AUTOPILOT] <= 0)
    {
        if (Player_uses_autopilot(pl))
            Autopilot(pl, false);
        CLR_BIT(pl->have, HAS_AUTOPILOT);
    }
    if (pl->item[ITEM_ARMOR] <= 0)
        CLR_BIT(pl->have, HAS_ARMOR);
}

/*
 * Player loses some items after some event (collision, bounce).
 * The 'prob' parameter gives the chance that items are lost
 * and, if they are lost, what percentage.
 */
void Item_damage(player_t *pl, double prob)
{
    if (prob < 1.0)
    {
        int i;
        double loss;

        loss = prob;
        LIMIT(loss, 0.0, 1.0);

        for (i = 0; i < NUM_ITEMS; i++)
        {
            if (!(i == ITEM_FUEL || i == ITEM_TANK))
            {
                if (pl->item[i])
                {
                    double f = rfrac();

                    if (f < loss)
                        pl->item[i] = (int)(pl->item[i] * loss + 0.5);
                }
            }
        }

        Item_update_flags(pl);
    }
}

int Choose_random_item(void)
{
    int i;
    double item_prob_sum = 0;

    for (i = 0; i < NUM_ITEMS; i++)
        item_prob_sum += World.items[i].prob;

    if (item_prob_sum > 0.0)
    {
        double sum = item_prob_sum * rfrac();

        for (i = 0; i < NUM_ITEMS; i++)
        {
            sum -= World.items[i].prob;
            if (sum <= 0)
                break;
        }
        if (i >= NUM_ITEMS)
            i = ITEM_FUEL;
    }

    return i;
}

void Place_item(player_t *pl, int item)
{
    world_t *world = &World;
    int num_lose, num_per_pack, place_count, dist;
    int bx, by;
    long grav, rand_item;
    clpos_t pos;
    vector_t vel;
    item_concentrator_t *con;

    if (NumObjs >= MAX_TOTAL_SHOTS)
    {
        if (pl && !Player_is_killed(pl))
            pl->item[item]--;
        return;
    }

    if (pl)
    {
        if (Player_is_killed(pl))
        {
            num_lose = pl->item[item] - World.items[item].initial;
            if (num_lose <= 0)
                return;
            pl->item[item] -= num_lose;
            num_per_pack = (int)(num_lose * options.dropItemOnKillProb);
            if (num_per_pack < World.items[item].min_per_pack)
                return;
        }
        else
        {
            num_lose = pl->item[item];
            if (num_lose <= 0)
                return;
            if (World.items[item].min_per_pack == World.items[item].max_per_pack)
                num_per_pack = World.items[item].max_per_pack;
            else
                num_per_pack = World.items[item].min_per_pack + (int)(rfrac() * (1 + World.items[item].max_per_pack - World.items[item].min_per_pack));
            if (num_per_pack > num_lose)
                num_per_pack = num_lose;
            else
                num_lose = num_per_pack;
            pl->item[item] -= num_lose;
        }
    }
    else
    {
        if (World.items[item].min_per_pack == World.items[item].max_per_pack)
            num_per_pack = World.items[item].max_per_pack;
        else
            num_per_pack = World.items[item].min_per_pack + (int)(rfrac() * (1 + World.items[item].max_per_pack - World.items[item].min_per_pack));
    }

    if (pl)
    {
        grav = GRAVITY;
        rand_item = 0;
        pos = pl->prevpos;
        if (!Player_is_killed(pl))
        {
            /*
             * Player is dropping an item on purpose.
             * Give the item some offset so that the
             * player won't immediately pick it up again.
             */
            if (pl->vel.x >= 0)
                pos.cx -= (BLOCK_CLICKS + (int)(rfrac() * 8 * CLICK));
            else
                pos.cx += (BLOCK_CLICKS + (int)(rfrac() * 8 * CLICK));
            if (pl->vel.y >= 0)
                pos.cy -= (BLOCK_CLICKS + (int)(rfrac() * 8 * CLICK));
            else
                pos.cy += (BLOCK_CLICKS + (int)(rfrac() * 8 * CLICK));
        }
        pos = World_wrap_clpos(world, pos);
        if (!World_contains_clpos(world, pos))
            return;
    }
    else
    {
        if (rfrac() < options.movingItemProb)
            grav = GRAVITY;
        else
            grav = 0;

        if (rfrac() < options.randomItemProb)
            rand_item = RANDOM_ITEM;
        else
            rand_item = 0;

        if (Num_itemConcs() > 0 && rfrac() < options.itemConcentratorProb)
            con = ItemConc_by_index((int)(rfrac() * Num_itemConcs()));
        else
            con = nullptr;
        /*
         * kps - write a generic function that can be used here and
         * with asteroids.
         */
        /*
         * This will take very long (or forever) with maps
         * that hardly have any (or none) spaces.
         * So bail out after a few retries.
         */
        for (place_count = 0;; place_count++)
        {
            if (place_count >= 8)
                return;

            if (con)
            {
                int dir = (int)(rfrac() * ANGLE_RESOLUTION);
                dist = (int)(rfrac() * ((options.itemConcentratorRadius * BLOCK_SZ) + 1));
                dist = PIXEL_TO_CLICK(dist);
                // dist = (int)(rfrac() * ((options.itemConcentratorRadius * BLOCK_CLICKS) + 1));
                pos.cx = (click_t)(con->pos.cx + dist * tcos(dir));
                pos.cy = (click_t)(con->pos.cy + dist * tsin(dir));
                pos = World_wrap_clpos(world, pos);
                if (!World_contains_clpos(world, pos))
                    continue;
            }
            else
                pos = World_get_random_clpos(world);

            blkpos_t blk_pos = Clpos_to_blkpos(pos);
            bx = blk_pos.bx;
            by = blk_pos.by;
            if (BIT(1U << World.block[bx][by], SPACE_BLOCKS | CANNON_BIT))
                break;
        }
    }
    vel.x = vel.y = 0;
    if (grav)
    {
        if (pl)
        {
            vel.x += pl->vel.x;
            vel.y += pl->vel.y;
            if (!Player_is_killed(pl))
            {
                double vl = LENGTH(vel.x, vel.y);
                int dvx = (int)(rfrac() * 8);
                int dvy = (int)(rfrac() * 8);
                const float drop_speed_factor = 0.75f;

                vel.x *= drop_speed_factor;
                vel.y *= drop_speed_factor;
                if (vl < 1.0)
                {
                    vel.x -= (pl->vel.x >= 0) ? dvx : -dvx;
                    vel.y -= (pl->vel.y >= 0) ? dvy : -dvy;
                }
                else
                {
                    vel.x -= dvx * (vel.x / vl);
                    vel.y -= dvy * (vel.y / vl);
                }
            }
            else
            {
                double v = rfrac() * 6;
                int dir = (int)(rfrac() * ANGLE_RESOLUTION);

                vel.x += tcos(dir) * v;
                vel.y += tsin(dir) * v;
            }
        }
        else
        {
            vector_t gravity = World_gravity(world, pos);

            vel.x -= options.gravity * gravity.x;
            vel.y -= options.gravity * gravity.y;
            vel.x += (int)(rfrac() * 8) - 3;
            vel.y += (int)(rfrac() * 8) - 3;
        }
    }

    Make_item(pos, vel, item, num_per_pack, grav | rand_item);
}

void Make_item(clpos_t pos, vector_t vel,
               int type, int num_per_pack, int status)
{
    world_t *world = &World;
    itemobject_t *item;

    if (!World_contains_clpos(world, pos))
        return;

    if (World.items[type].num >= World.items[type].max)
        return;

    if ((item = ITEM_PTR(Object_allocate())) == nullptr)
        return;

    item->type = OBJ_ITEM;
    item->item_type = type;
    item->color = RED;
    item->obj_status = status;
    item->id = NO_ID;
    item->team = TEAM_NOT_SET;
    Object_position_init_clpos(OBJ_PTR(item), pos);
    item->vel = vel;
    item->acc.x =
        item->acc.y = 0.0;
    item->mass = 10.0;
    item->obj_life = 1500 + rfrac() * 512;
    item->item_count = num_per_pack;
    item->pl_range = ITEM_SIZE / 2;
    item->pl_radius = ITEM_SIZE / 2;

    World.items[type].num++;
    Cell_add_object(OBJ_PTR(item));
}

void Throw_items(player_t *pl)
{
    int num_items_to_throw, remain, item;

    if (!options.dropItemOnKillProb || !pl)
        return;

    for (item = 0; item < NUM_ITEMS; item++)
    {
        if (item == ITEM_FUEL || item == ITEM_TANK)
            continue;
        do
        {
            num_items_to_throw = pl->item[item] - World.items[item].initial;
            if (num_items_to_throw <= 0)
                break;
            Place_item(pl, item);
            remain = pl->item[item] - World.items[item].initial;
        } while (remain > 0 && remain < num_items_to_throw);
    }

    Item_update_flags(pl);
}

/*
 * Cause some remaining mines or missiles to be launched in
 * a random direction with a small life time (ie. magazine has
 * gone off).
 */
void Detonate_items(player_t *pl)
{
    player_t *owner_pl;
    int i;
    modifiers_t mods;

    if (!Player_is_killed(pl))
        return;

    /* ZE: Detonated items on tanks should belong to the tank's owner. */
    if (Player_is_tank(pl))
        owner_pl = Player_by_id(pl->lock.pl_id);
    else
        owner_pl = pl;

    /*
     * Initial items are immune to detonation.
     */
    if ((pl->item[ITEM_MINE] -= World.items[ITEM_MINE].initial) < 0)
        pl->item[ITEM_MINE] = 0;
    if ((pl->item[ITEM_MISSILE] -= World.items[ITEM_MISSILE].initial) < 0)
        pl->item[ITEM_MISSILE] = 0;

    /*
     * Drop shields in order to launch mines and missiles.
     */
    CLR_BIT(pl->used, HAS_SHIELD);

    /*
     * Mines are always affected by gravity and are sent in random directions
     * slowly out from the ship (velocity relative).
     */
    for (i = 0; i < pl->item[ITEM_MINE]; i++)
    {
        if (rfrac() < options.detonateItemOnKillProb)
        {
            int dir = (int)(rfrac() * ANGLE_RESOLUTION);
            double speed = rfrac() * 4.0;
            vector_t vel;

            mods = pl->mods;
            if (Mods_get(mods, ModsNuclear) && pl->item[ITEM_MINE] < options.nukeMinMines)
                Mods_set(&mods, ModsNuclear, 0);

            vel.x = pl->vel.x + speed * tcos(dir);
            vel.y = pl->vel.y + speed * tsin(dir);
            Place_general_mine(owner_pl->id, pl->team, GRAVITY,
                               pl->pos, vel, mods);
        }
    }
    for (i = 0; i < pl->item[ITEM_MISSILE]; i++)
    {
        if (rfrac() < options.detonateItemOnKillProb)
        {
            int type;

            if (pl->shots >= options.maxPlayerShots)
                break;

            /*
             * Missiles are random type at random players, which could
             * mean a misfire.
             */
            SET_BIT(pl->lock.tagged, LOCK_PLAYER);
            pl->lock.pl_id = Player_by_index((int)(rfrac() * NumPlayers))->id;

            switch ((int)(rfrac() * 3))
            {
            case 0:
                type = OBJ_TORPEDO;
                break;
            case 1:
                type = OBJ_HEAT_SHOT;
                break;
            default:
                type = OBJ_SMART_SHOT;
                break;
            }

            mods = pl->mods;
            if (Mods_get(mods, ModsNuclear) && pl->item[ITEM_MISSILE] < options.nukeMinSmarts)
                Mods_set(&mods, ModsNuclear, 0);

            Fire_general_shot(owner_pl->id, pl->team, false, pl->pos,
                              type, (int)(rfrac() * ANGLE_RESOLUTION), mods, NO_ID);
        }
    }
}

void Tractor_beam(player_t *pl)
{
    double maxdist, percent, cost;
    player_t *locked_pl = Player_by_id(pl->lock.pl_id);

    maxdist = TRACTOR_MAX_RANGE(pl->item[ITEM_TRACTOR_BEAM]);
    if (BIT(pl->lock.tagged, LOCK_PLAYER | LOCK_VISIBLE) != (LOCK_PLAYER | LOCK_VISIBLE) || !Player_is_alive(locked_pl) || pl->lock.distance >= maxdist || Player_is_phasing(pl) || Player_is_phasing(locked_pl))
    {
        CLR_BIT(pl->used, USES_TRACTOR_BEAM);
        return;
    }
    percent = TRACTOR_PERCENT(pl->lock.distance, maxdist);
    cost = TRACTOR_COST(percent);
    if (pl->fuel.sum < -cost)
    {
        CLR_BIT(pl->used, USES_TRACTOR_BEAM);
        return;
    }
    General_tractor_beam(pl->id, pl->pos, pl->item[ITEM_TRACTOR_BEAM],
                         locked_pl, pl->tractor_is_pressor);
}

void General_tractor_beam(int id, clpos_t pos,
                          int items, player_t *victim, bool pressor)
{
    world_t *world = &World;
    double maxdist = TRACTOR_MAX_RANGE(items);
    double maxforce = TRACTOR_MAX_FORCE(items), percent, force, dist, cost, a;
    int theta;
    player_t *pl = Player_by_id(id);
    /*cannon_t *cannon = Cannon_by_id(id);*/

    dist = World_wrap_length(
               world,
               pos.cx - victim->pos.cx,
               pos.cy - victim->pos.cy) /
           CLICK;
    if (dist > maxdist)
        return;

    percent = TRACTOR_PERCENT(dist, maxdist);
    cost = TRACTOR_COST(percent);
    force = TRACTOR_FORCE(pressor, percent, maxforce);

    sound_play_sensors(pos, pressor ? PRESSOR_BEAM_SOUND : TRACTOR_BEAM_SOUND);

    if (pl)
        Player_add_fuel(pl, cost);

    a = World_wrap_cfindDir(
        world,
        pos.cx - victim->pos.cx,
        pos.cy - victim->pos.cy);
    theta = (int)a;

    if (pl)
    {
        pl->vel.x += tcos(theta) * (force / pl->mass);
        pl->vel.y += tsin(theta) * (force / pl->mass);
        Record_shove(pl, victim, frame_loops);
        Record_shove(victim, pl, frame_loops);
    }
    victim->vel.x -= tcos(theta) * (force / victim->mass);
    victim->vel.y -= tsin(theta) * (force / victim->mass);
}

void Do_deflector(player_t *pl)
{
    world_t *world = &World;
    double range = (pl->item[ITEM_DEFLECTOR] * 0.5 + 1) * BLOCK_SZ;
    double maxforce = pl->item[ITEM_DEFLECTOR] * 0.2;
    object_t *obj, **obj_list;
    int i, obj_count;
    double dist, dx, dy;

    if (pl->fuel.sum < -ED_DEFLECTOR)
    {
        if (BIT(pl->used, USES_DEFLECTOR))
            Deflector(pl, false);
        return;
    }
    Player_add_fuel(pl, ED_DEFLECTOR);

    Cell_get_objects(pl->pos,
                     (int)(range / BLOCK_SZ + 1), 200,
                     &obj_list, &obj_count);

    for (i = 0; i < obj_count; i++)
    {
        obj = obj_list[i];

        if (obj->obj_life <= 0.0)
            continue;

        if (obj->id == pl->id)
        {
            if (BIT(obj->obj_status, OWNERIMMUNE) || obj->fuselife < obj->obj_life || options.selfImmunity)
                continue;
        }
        else
        {
            if (Team_immune(obj->id, pl->id))
                continue;
        }

        /* don't push balls out of treasure boxes */
        if (obj->type == OBJ_BALL && !BIT(obj->obj_status, GRAVITY))
            continue;

        int dcx = WORLD_WRAP_DCX(world, obj->pos.cx - pl->pos.cx);
        int dcy = WORLD_WRAP_DCY(world, obj->pos.cy - pl->pos.cy);

        dx = CLICK_TO_FLOAT(dcx);
        dy = CLICK_TO_FLOAT(dcy);

        dist = LENGTH(dx, dy) - SHIP_SZ;
        if (dist < range && dist > 0)
        {
            int dir, idir;
            double a;

            a = findDir(dx, dy);
            dir = (int)a;
            idir = MOD2((int)(dir - findDir(obj->vel.x, obj->vel.y)), ANGLE_RESOLUTION);

            if (idir > ANGLE_RESOLUTION * 0.25 && idir < ANGLE_RESOLUTION * 0.75)
            {
                double force = ((double)(range - dist) / range) * ((double)(range - dist) / range) * maxforce * ((ANGLE_RESOLUTION * 0.25) - ABS(idir - ANGLE_RESOLUTION * 0.5)) / (ANGLE_RESOLUTION * 0.25);
                double dv = force / ABS(obj->mass);

                obj->vel.x += tcos(dir) * dv;
                obj->vel.y += tsin(dir) * dv;
            }
        }
    }
}

void Do_transporter(player_t *pl)
{
    world_t *world = &World;
    player_t *victim = nullptr;
    int i;
    double dist, closest = TRANSPORTER_DISTANCE;

    /* if not available, fail silently */
    if (!pl->item[ITEM_TRANSPORTER] ||
        pl->fuel.sum < -ED_TRANSPORTER ||
        Player_is_phasing(pl))
        return;

    /* find victim */
    for (i = 0; i < NumPlayers; i++)
    {
        player_t *pl_i = Player_by_index(i);

        if (pl_i == pl ||
            !Player_is_active(pl_i) ||
            Team_immune(pl->id, pl_i->id) ||
            Player_is_tank(pl_i) ||
            Player_is_phasing(pl_i))
            continue;
        dist = World_wrap_length(world,
                                 pl->pos.cx - pl_i->pos.cx,
                                 pl->pos.cy - pl_i->pos.cy);
        if (dist < closest)
        {
            closest = dist;
            victim = pl_i;
        }
    }

    /* no victims in range */
    if (!victim)
    {
        sound_play_sensors(pl->pos, TRANSPORTER_FAIL_SOUND);
        Player_add_fuel(pl, ED_TRANSPORTER);
        pl->item[ITEM_TRANSPORTER]--;
        return;
    }

    /* victim found */
    Do_general_transporter(pl->id, pl->pos, victim, nullptr, nullptr);
}

void Do_general_transporter(int id, clpos_t pos,
                            player_t *victim, int *itemp, double *amountp)
{
    char msg[MSG_LEN];
    const char *what = nullptr;
    int i, item = ITEM_FUEL;
    double amount;
    player_t *pl = Player_by_id(id);
    /*cannon_t *cannon = Cannon_by_id(id);*/

    /* choose item type to steal */
    for (i = 0; i < 50; i++)
    {
        item = (int)(rfrac() * NUM_ITEMS);
        if (victim->item[item] || (item == ITEM_TANK && victim->fuel.num_tanks) || (item == ITEM_FUEL && victim->fuel.sum))
            break;
    }

    if (i == 50)
    {
        /* you can't pluck from a bald chicken.. */
        sound_play_sensors(pos, TRANSPORTER_FAIL_SOUND);
        if (!pl)
        {
            *amountp = 0;
            *itemp = -1;
        }
        return;
    }
    else
    {
        sound_play_sensors(pos, TRANSPORTER_SUCCESS_SOUND);
        if (Num_transporters() < MAX_TOTAL_TRANSPORTERS)
        {
            transporter_t t;
            t.pos = pos;
            t.victim_id = victim->id;
            t.id = (pl ? pl->id : NO_ID);
            t.count = 5;
            World.transporters.push_back(t);
            // World.transporters[World.NumTransporters].pos = pos;
            // World.transporters[World.NumTransporters].victim_id = victim->id;
            // World.transporters[World.NumTransporters].id = (pl ? pl->id : NO_ID);
            // World.transporters[World.NumTransporters].count = 5;
            // World.NumTransporters++;
        }
    }

    /* remove loot from victim */
    amount = 1;
    if (!(item == ITEM_MISSILE || item == ITEM_FUEL || item == ITEM_TANK))
        victim->item[item]--;

    /* describe loot and update victim */
    msg[0] = '\0';
    switch (item)
    {
    case ITEM_AFTERBURNER:
        what = "an afterburner";
        if (victim->item[item] <= 0)
            CLR_BIT(victim->have, HAS_AFTERBURNER);
        break;
    case ITEM_MISSILE:
        amount = MIN(victim->item[item], 3);
        if (amount == 1)
            sprintf(msg, "%s stole a missile from %s.",
                    (pl ? pl->name.c_str() : "A cannon"), victim->name.c_str());
        else
            sprintf(msg, "%s stole %d missiles from %s",
                    (pl ? pl->name.c_str() : "A cannon"), (int)amount, victim->name.c_str());
        break;
    case ITEM_CLOAK:
        what = "a cloaking device";
        victim->updateVisibility = true;
        if (victim->item[item] <= 0)
            Cloak(victim, false);
        break;
    case ITEM_WIDEANGLE:
        what = "a wideangle";
        break;
    case ITEM_REARSHOT:
        what = "a rearshot";
        break;
    case ITEM_MINE:
        what = "a mine";
        break;
    case ITEM_SENSOR:
        what = "a sensor";
        victim->updateVisibility = true;
        break;
    case ITEM_ECM:
        what = "an ECM";
        break;
    case ITEM_ARMOR:
        what = "an armor";
        if (victim->item[item] <= 0)
            CLR_BIT(victim->have, HAS_ARMOR);
        break;
    case ITEM_TRANSPORTER:
        what = "a transporter";
        break;
    case ITEM_MIRROR:
        what = "a mirror";
        if (victim->item[item] <= 0)
            CLR_BIT(victim->have, HAS_MIRROR);
        break;
    case ITEM_DEFLECTOR:
        what = "a deflector";
        if (victim->item[item] <= 0)
            Deflector(victim, false);
        break;
    case ITEM_HYPERJUMP:
        what = "a hyperjump";
        break;
    case ITEM_PHASING:
        what = "a phasing device";
        if (victim->item[item] <= 0)
        {
            if (Player_is_phasing(victim))
                Phasing(victim, false);
            CLR_BIT(victim->have, HAS_PHASING_DEVICE);
        }
        break;
    case ITEM_LASER:
        what = "a laser";
        break;
    case ITEM_EMERGENCY_THRUST:
        what = "an emergency thrust";
        if (victim->item[item] <= 0)
        {
            if (BIT(victim->used, HAS_EMERGENCY_THRUST))
                Emergency_thrust(victim, false);
            CLR_BIT(victim->have, HAS_EMERGENCY_THRUST);
        }
        break;
    case ITEM_EMERGENCY_SHIELD:
        what = "an emergency shield";
        if (victim->item[item] <= 0)
        {
            if (BIT(victim->used, HAS_EMERGENCY_SHIELD))
                Emergency_shield(victim, false);
            CLR_BIT(victim->have, HAS_EMERGENCY_SHIELD);
            if (!BIT(DEF_HAVE, HAS_SHIELD))
            {
                CLR_BIT(victim->have, HAS_SHIELD);
                CLR_BIT(victim->used, HAS_SHIELD);
            }
        }
        break;
    case ITEM_TRACTOR_BEAM:
        what = "a tractor beam";
        if (victim->item[item] <= 0)
            CLR_BIT(victim->have, HAS_TRACTOR_BEAM);
        break;
    case ITEM_AUTOPILOT:
        what = "an autopilot";
        if (victim->item[item] <= 0)
        {
            if (Player_uses_autopilot(victim))
                Autopilot(victim, false);
            CLR_BIT(victim->have, HAS_AUTOPILOT);
        }
        break;
    case ITEM_TANK:
        /* for tanks, amount is the amount of fuel in the stolen tank */
        what = "a tank";
        i = (int)(rfrac() * victim->fuel.num_tanks) + 1;
        amount = victim->fuel.tank[i];
        Player_remove_tank(victim, i);
        break;
    case ITEM_FUEL:
    {
        /* choose percentage between 10 and 50. */
        double percent = 10.0 + 40.0 * rfrac();
        amount = victim->fuel.sum * percent / 100.0;
        sprintf(msg, "%s stole %.1f units (%.1f%%) of fuel from %s.",
                (pl ? pl->name.c_str() : "A cannon"),
                amount, percent, victim->name.c_str());
    }
        Player_add_fuel(victim, -amount);
        break;
    default:
        warn("Do_general_transporter: unknown item type.");
        break;
    }

    /* inform the world about the robbery */
    if (!msg[0])
        sprintf(msg, "%s stole %s from %s.", (pl ? pl->name.c_str() : "A cannon"),
                what, victim->name.c_str());
    Set_message(msg);

    /* cannons take care of themselves */
    if (!pl)
    {
        *itemp = item;
        *amountp = amount;
        return;
    }

    /* don't forget the penalty for robbery */
    pl->item[ITEM_TRANSPORTER]--;
    Player_add_fuel(pl, ED_TRANSPORTER);

    /* update thief */
    if (!(item == ITEM_FUEL || item == ITEM_TANK))
        pl->item[item] += (int)amount;
    switch (item)
    {
    case ITEM_AFTERBURNER:
        SET_BIT(pl->have, HAS_AFTERBURNER);
        LIMIT(pl->item[item], 0, MAX_AFTERBURNER);
        break;
    case ITEM_CLOAK:
        SET_BIT(pl->have, HAS_CLOAKING_DEVICE);
        pl->updateVisibility = true;
        break;
    case ITEM_SENSOR:
        pl->updateVisibility = true;
        break;
    case ITEM_MIRROR:
        SET_BIT(pl->have, HAS_MIRROR);
        break;
    case ITEM_ARMOR:
        SET_BIT(pl->have, HAS_ARMOR);
        break;
    case ITEM_DEFLECTOR:
        SET_BIT(pl->have, HAS_DEFLECTOR);
        break;
    case ITEM_PHASING:
        SET_BIT(pl->have, HAS_PHASING_DEVICE);
        break;
    case ITEM_EMERGENCY_THRUST:
        SET_BIT(pl->have, HAS_EMERGENCY_THRUST);
        break;
    case ITEM_EMERGENCY_SHIELD:
        SET_BIT(pl->have, HAS_EMERGENCY_SHIELD);
        break;
    case ITEM_TRACTOR_BEAM:
        SET_BIT(pl->have, HAS_TRACTOR_BEAM);
        break;
    case ITEM_AUTOPILOT:
        SET_BIT(pl->have, HAS_AUTOPILOT);
        break;
    case ITEM_TANK:
        /* for tanks, amount is the amount of fuel in the stolen tank */
        if (pl->fuel.num_tanks < MAX_TANKS)
            Player_add_tank(pl, amount);
        break;
    case ITEM_FUEL:
        Player_add_fuel(pl, amount);
        break;
    default:
        break;
    }

    LIMIT(pl->item[item], 0, World.items[item].limit);
}

void do_hyperjump(player_t *pl)
{
    SET_BIT(pl->obj_status, WARPING);
    pl->wormHoleHit = -1;
}

void do_lose_item(player_t *pl)
{
    int item;

    if (!pl)
        return;

    item = pl->lose_item;
    if (item < 0 || item >= NUM_ITEMS)
    {
        error("BUG: do_lose_item %d", item);
        return;
    }
    if (item == ITEM_FUEL || item == ITEM_TANK)
        return;

    if (pl->item[item] <= 0)
        return;

    if (!options.loseItemDestroys && !Player_is_phasing(pl))
        Place_item(pl, item);
    else
        pl->item[item]--;

    Item_update_flags(pl);
}

void Fire_general_ecm(int id, int team, clpos_t pos)
{
    world_t *world = &World;
    object_t *shot;
    mineobject_t *closest_mine = nullptr;
    smartobject_t *smart;
    mineobject_t *mine;
    double closest_mine_range = World.hypotenuse;
    int i, j, ecm_ind;
    double range, perim, damage;
    player_t *pl = Player_by_id(id), *p;
    ecm_t t;

    if (Num_ecms() >= MAX_TOTAL_ECMS)
        return;
    // Ecms[World.NumEcms] = (ecm_t *)malloc(sizeof(ecm_t));
    // if (Ecms[World.NumEcms] == nullptr)
    //     return;
    // ecm = &World.ecms[World.NumEcms];
    // ecm->pos = pos;
    // ecm->id = (pl ? pl->id : NO_ID);
    // ecm->size = (int)ECM_DISTANCE;
    // World.NumEcms++;
    t.pos = pos;
    t.id = (pl ? pl->id : NO_ID);
    t.size = ECM_DISTANCE;
    World.ecms.push_back(t);
    ecm_ind = Num_ecms();
    if (ecm_ind < 0)
        return;

    if (pl)
    {
        ecm_t *ecm = Ecm_by_index(ecm_ind);

        pl->ecmcount++;
        pl->item[ITEM_ECM]--;
        Player_add_fuel(pl, ED_ECM);
        sound_play_sensors(ecm->pos, ECM_SOUND);
    }

    for (i = 0; i < NumObjs; i++)
    {
        shot = Obj[i];

        if (!(shot->type == OBJ_SMART_SHOT || shot->type == OBJ_MINE))
            continue;
        if ((range = (World_wrap_length(
                          world,
                          pos.cx - shot->pos.cx,
                          pos.cy - shot->pos.cy) /
                      CLICK)) > ECM_DISTANCE)
            continue;

        /*
         * Ignore mines owned by yourself which you are immune to,
         * or missiles owned by you which are after somebody else.
         *
         * Ignore any object not owned by you which are owned by
         * team members if team immunity is on.
         */
        if (shot->id != NO_ID)
        {
            player_t *owner_pl = Player_by_id(shot->id);

            if (pl == owner_pl)
            {
                if (shot->type == OBJ_MINE)
                {
                    if (BIT(shot->obj_status, OWNERIMMUNE))
                        continue;
                }
                if (shot->type == OBJ_SMART_SHOT)
                {
                    smart = SMART_PTR(shot);
                    if (smart->smart_lock_id != owner_pl->id)
                        continue;
                }
            }
            else if ((pl && Team_immune(pl->id, owner_pl->id)) || (Team_play(world) && team == shot->team))
                continue;
        }

        switch (shot->type)
        {
        case OBJ_SMART_SHOT:
            /*
             * See Move_smart_shot() for re-lock probabilities after confusion
             * ends.
             */
            smart = SMART_PTR(shot);
            SET_BIT(smart->obj_status, CONFUSED);
            smart->smart_ecm_range = range;
            smart->smart_count = CONFUSED_TIME;
            if (pl &&
                BIT(pl->lock.tagged, LOCK_PLAYER) &&
                (pl->lock.distance <= pl->sensor_range || !BIT(world->rules.mode, LIMITED_VISIBILITY)) &&
                pl->visibility[GetInd(pl->lock.pl_id)].canSee)
                smart->new_info = pl->lock.pl_id;
            else
                smart->new_info = Player_by_index((int)(rfrac() * NumPlayers))->id;
            /* Can't redirect missiles to team mates. */
            /* So let the missile keep on following this unlucky player. */
            /*-BA Why not redirect missiles to team mates?
             *-BA It's not ideal, but better them than me...
             *if (TEAM_IMMUNE(ind, GetInd(smart->new_info))) {
             *        smart->new_info = ind;
             * }
             */
            break;

        case OBJ_MINE:
            mine = MINE_PTR(shot);
            mine->ecm_range = range;

            /*
             * perim is distance from the mine to its detonation perimeter
             *
             * range is the proportion from the mine detotation perimeter
             * to the maximum ecm range.
             * low values of range mean the mine is close
             *
             * remember the closest unconfused mine -- it gets reprogrammed
             */
            perim = MINE_RANGE / (Mods_get(mine->mods, ModsMini) + 1);
            range = (range - perim) / (ECM_DISTANCE - perim);

            /*
             * range%                explode%        confuse time (seconds)
             * 100                5                2
             *  50                10                6
             *         0 (closest)        15                10
             */
            if (range <= 0 || (int)(rfrac() * 100.0f) < ((int)(10 * (1 - range)) + 5))
            {
                mine->obj_life = 0.0;
                break;
            }
            mine->mine_count = ((8 * (1 - range)) + 2) * 12;
            if (!BIT(mine->obj_status, CONFUSED) && (closest_mine == nullptr || range < closest_mine_range))
            {
                closest_mine = mine;
                closest_mine_range = range;
            }
            SET_BIT(mine->obj_status, CONFUSED);
            if (mine->mine_count <= 0)
                CLR_BIT(mine->obj_status, CONFUSED);
            break;
        default:
            break;
        }
    }

    /*
     * range%                reprogram%
     * 100                50
     *  50                75
     *         0 (closest)        100
     */
    if (options.ecmsReprogramMines && closest_mine != nullptr)
    {
        range = closest_mine_range;
        if (range <= 0 || (int)(rfrac() * 100.0) < (100 - (int)(50 * range)))
        {
            closest_mine->id = (pl ? pl->id : NO_ID);
            closest_mine->team = team;
        }
    }

    /* in non-team mode cannons are immune to cannon ECMs */
    if (Team_play(world) || pl)
    {
        for (i = 0; i < Num_cannons(); i++)
        {
            cannon_t *c = Cannon_by_index(i);

            if (Team_play(world) && c->team == team)
                continue;
            range = World_wrap_length(
                        world,
                        pos.cx - c->pos.cx,
                        pos.cy - c->pos.cy) /
                    CLICK;
            if (range > ECM_DISTANCE)
                continue;
            damage = (ECM_DISTANCE - range) / ECM_DISTANCE;
            if (c->item[ITEM_LASER])
                c->item[ITEM_LASER] -= (int)(damage * c->item[ITEM_LASER] + 0.5);
            c->damaged += (int)(24 * range * pow(0.75, c->item[ITEM_SENSOR]));
        }
    }

    for (i = 0; i < NumPlayers; i++)
    {
        p = Player_by_index(i);

        if (p == pl)
            continue;

        /*
         * Team members are always immune from ECM effects from other
         * team members.  Its too nasty otherwise.
         */
        if (Team_play(world) && p->team == team)
            continue;

        if (pl && Players_are_allies(pl, p))
            continue;

        if (Player_is_phasing(p))
            continue;

        if (Player_is_active(p))
        {
            range = World_wrap_length(
                        world,
                        pos.cx - p->pos.cx,
                        pos.cy - p->pos.cy) /
                    CLICK;
            if (range > ECM_DISTANCE)
                continue;

            /* range is how close the player is to the center of ecm */
            range = ((ECM_DISTANCE - range) / ECM_DISTANCE);

            /*
             * range%        damage (sec)        laser destroy%        reprogram%        drop%
             * 100        4                75                100                25
             * 50        2                50                75                15
             * 0        0                25                50                5
             */

            /*
             * should this be FPS dependant: damage = 4.0f * FPS * range; ?
             * No, i think.
             */
            damage = 24.0 * range;

            if (p->item[ITEM_CLOAK] <= 1)
                p->forceVisible += (int)damage;
            else
                p->forceVisible += (int)(damage * pow(0.75, (p->item[ITEM_CLOAK] - 1)));

            /* ECM may cause balls to detach. */
            if (BIT(p->have, HAS_BALL))
            {
                for (j = 0; j < NumObjs; j++)
                {
                    shot = Obj[j];
                    if (shot->type == OBJ_BALL)
                    {
                        ballobject_t *ball = BALL_PTR(shot);

                        if (ball->ball_owner == p->id)
                        {
                            if ((int)(rfrac() * 100.0) < ((int)(20 * range) + 5))
                                Detach_ball(p, ball);
                        }
                    }
                }
            }

            /* ECM damages sensitive equipment like lasers */
            if (p->item[ITEM_LASER] > 0)
                p->item[ITEM_LASER] -= (int)(range * p->item[ITEM_LASER] + 0.5);

            if (!Player_is_robot(p) || !options.ecmsReprogramRobots || !pl)
            {
                /* player is blinded by light flashes. */
                long duration = (int)(damage * pow(0.75, p->item[ITEM_SENSOR]));
                p->damaged += duration;
                if (pl)
                    Record_shove(p, pl, frame_loops + (long)duration);
            }
            else
            {
                if (BIT(pl->lock.tagged, LOCK_PLAYER) && (pl->lock.distance < pl->sensor_range || !BIT(world->rules.mode, LIMITED_VISIBILITY)) && pl->visibility[GetInd(pl->lock.pl_id)].canSee && pl->lock.pl_id != p->id
                    /*&& !TEAM_IMMUNE(ind, GetInd(pl->lock.pl_id))*/)
                {

                    /*
                     * Player programs robot to seek target.
                     */
                    Robot_program(p, pl->lock.pl_id);
                }
            }
        }
    }
}

void Fire_ecm(player_t *pl)
{
    if (pl->item[ITEM_ECM] == 0 || pl->fuel.sum <= -ED_ECM || pl->ecmcount >= MAX_PLAYER_ECMS || Player_is_phasing(pl))
        return;

    Fire_general_ecm(pl->id, pl->team, pl->pos);
}

Item_t Item_by_option_name(const char *name)
{
    if (!strcasecmp(name, "initialfuel"))
        return ITEM_FUEL;
    if (!strcasecmp(name, "initialwideangles"))
        return ITEM_WIDEANGLE;
    if (!strcasecmp(name, "initialrearshots"))
        return ITEM_REARSHOT;
    if (!strcasecmp(name, "initialafterburners"))
        return ITEM_AFTERBURNER;
    if (!strcasecmp(name, "initialcloaks"))
        return ITEM_CLOAK;
    if (!strcasecmp(name, "initialsensors"))
        return ITEM_SENSOR;
    if (!strcasecmp(name, "initialtransporters"))
        return ITEM_TRANSPORTER;
    if (!strcasecmp(name, "initialtanks"))
        return ITEM_TANK;
    if (!strcasecmp(name, "initialmines"))
        return ITEM_MINE;
    if (!strcasecmp(name, "initialmissiles"))
        return ITEM_MISSILE;
    if (!strcasecmp(name, "initialecms"))
        return ITEM_ECM;
    if (!strcasecmp(name, "initiallasers"))
        return ITEM_LASER;
    if (!strcasecmp(name, "initialemergencythrusts"))
        return ITEM_EMERGENCY_THRUST;
    if (!strcasecmp(name, "initialtractorbeams"))
        return ITEM_TRACTOR_BEAM;
    if (!strcasecmp(name, "initialautopilots"))
        return ITEM_AUTOPILOT;
    if (!strcasecmp(name, "initialemergencyshields"))
        return ITEM_EMERGENCY_SHIELD;
    if (!strcasecmp(name, "initialdeflectors"))
        return ITEM_DEFLECTOR;
    if (!strcasecmp(name, "initialhyperjumps"))
        return ITEM_HYPERJUMP;
    if (!strcasecmp(name, "initialphasings"))
        return ITEM_PHASING;
    if (!strcasecmp(name, "initialmirrors"))
        return ITEM_MIRROR;
    if (!strcasecmp(name, "initialarmor") || !strcasecmp(name, "initialarmors"))
        return ITEM_ARMOR;

    return NO_ITEM;
}

int IsOffensiveItem(enum Item i)
{
    if (BIT(1 << i,
            ITEM_BIT_WIDEANGLE |
                ITEM_BIT_REARSHOT |
                ITEM_BIT_MINE |
                ITEM_BIT_MISSILE |
                ITEM_BIT_LASER))
        return true;
    return false;
}

int IsDefensiveItem(enum Item i)
{
    if (BIT(1 << i,
            ITEM_BIT_CLOAK |
                ITEM_BIT_ECM |
                ITEM_BIT_TRANSPORTER |
                ITEM_BIT_TRACTOR_BEAM |
                ITEM_BIT_EMERGENCY_SHIELD |
                ITEM_BIT_MIRROR |
                ITEM_BIT_DEFLECTOR |
                ITEM_BIT_HYPERJUMP |
                ITEM_BIT_PHASING |
                ITEM_BIT_TANK |
                ITEM_BIT_ARMOR))
        return true;
    return false;
}

int CountOffensiveItems(player_t *pl)
{
    return (pl->item[ITEM_WIDEANGLE] + pl->item[ITEM_REARSHOT] +
            pl->item[ITEM_MINE] + pl->item[ITEM_MISSILE] +
            pl->item[ITEM_LASER]);
}

int CountDefensiveItems(player_t *pl)
{
    int count;

    count = pl->item[ITEM_CLOAK] + pl->item[ITEM_ECM] + pl->item[ITEM_ARMOR] +
            pl->item[ITEM_TRANSPORTER] + pl->item[ITEM_TRACTOR_BEAM] +
            pl->item[ITEM_EMERGENCY_SHIELD] + pl->fuel.num_tanks +
            pl->item[ITEM_DEFLECTOR] + pl->item[ITEM_HYPERJUMP] +
            pl->item[ITEM_PHASING] + pl->item[ITEM_MIRROR];
    if (pl->emergency_shield_left > 0)
        count++;
    if (pl->phasing_left > 0)
        count++;
    return count;
}
