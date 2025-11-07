/*
 * XPilot, a multiplayer gravity war game.  Copyright (C) 1991-2001 by
 *
 *      Bjørn Stabell
 *      Ken Ronny Schouten
 *      Bert Gijsbers
 *      Dick Balaska
 *      Kimiko Koopman
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

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <climits>

#include "const.h"
#include "randommt.h"
#include "xperror.h"

#include "server.h"

#define SERVER
#include "xpconfig.h"
#include "serverconst.h"
#include "global.h"
#include "bit.h"
#include "object.h"
#include "cannon.h"
#include "saudio.h"
#include "xpmath.h"
#include "polygon.h"

static int Cannon_select_weapon(cannon_t *cannon);
static void Cannon_aim(cannon_t *cannon, int weapon, player_t **pl_p, int *dir);
static void Cannon_fire(cannon_t *cannon, int weapon, player_t *pl, int dir);
static int Cannon_in_danger(cannon_t *cannon);
static int Cannon_select_defense(cannon_t *cannon);
static void Cannon_defend(cannon_t *cannon, int defense);

/* the items that are useful to cannons.
   these are the items that cannon get 'for free' once in a while.
   cannons can get other items, but only by picking them up or
   stealing them from players. */
long CANNON_USE_ITEM = (ITEM_BIT_FUEL | ITEM_BIT_WIDEANGLE | ITEM_BIT_REARSHOT | ITEM_BIT_AFTERBURNER | ITEM_BIT_SENSOR | ITEM_BIT_TRANSPORTER | ITEM_BIT_TANK | ITEM_BIT_MINE | ITEM_BIT_ECM | ITEM_BIT_LASER | ITEM_BIT_EMERGENCY_THRUST | ITEM_BIT_ARMOR | ITEM_BIT_TRACTOR_BEAM | ITEM_BIT_MISSILE | ITEM_BIT_PHASING);

void Cannon_update(bool tick)
{
    int i;
    /*
     * Updating cannons, maybe a little bit of fireworks too?
     */
    for (i = 0; i < Num_cannons(); i++)
    {
        cannon_t *c = Cannon_by_index(i);
        if (c->dead_time > 0)
        {
            if (!--c->dead_time)
            {
                world->block[c->blk_pos.bx][c->blk_pos.by] = CANNON;
                c->conn_mask = 0;
                c->last_change = frame_loops;
            }
            continue;
        }
        else
        {
            /* don't check too often, because this gets quite expensive
               on maps with many cannons with defensive items */
            if (options.cannonsUseItems && options.cannonsDefend && rfrac() < 0.65)
                Cannon_check_defense(c);

            if (!BIT(c->used, USES_EMERGENCY_SHIELD) && !BIT(c->used, USES_PHASING_DEVICE) && !c->damaged && !c->tractor_count && rfrac() * 16 < 1)
                Cannon_check_fire(c);

            else if (options.cannonsUseItems && options.itemProbMult > 0 && options.cannonItemProbMult > 0)
            {
                int item = (int)(rfrac() * NUM_ITEMS);
                /* this gives the cannon an item about once every minute */
                if (world->items[item].cannonprob > 0 && options.cannonItemProbMult > 0 && (int)(rfrac() * (60 * FPS)) < (options.cannonItemProbMult * world->items[item].cannonprob))
                    Cannon_add_item(c, item, (item == ITEM_FUEL ? ENERGY_PACK_FUEL : 1));
            }
        }
        if (c->damaged > 0)
        {
            c->damaged--;
        }
        if (c->tractor_count > 0)
        {
            player_t *tpl = Player_by_id(c->tractor_target_id);

            if (tpl == NULL)
            {
                c->tractor_target_id = NO_ID;
                c->tractor_count = 0;
            }
            else if (Wrap_length(tpl->pos.cx - c->pos.cx,
                                 tpl->pos.cy - c->pos.cy) /
                             CLICK <
                         TRACTOR_MAX_RANGE(c->item[ITEM_TRACTOR_BEAM]) &&
                     BIT(tpl->obj_status, PLAYING | GAME_OVER | KILLED | PAUSE) == PLAYING)
            {
                General_tractor_beam(c->id, c->pos,
                                     c->item[ITEM_TRACTOR_BEAM],
                                     tpl, c->tractor_is_pressor);
                c->tractor_count--;
            }
            else
            {
                c->tractor_count = 0;
            }
        }
        if (c->emergency_shield_left > 0)
        {
            if (--c->emergency_shield_left <= 0)
            {
                CLR_BIT(c->used, USES_EMERGENCY_SHIELD);
                sound_play_sensors(c->pos, EMERGENCY_SHIELD_OFF_SOUND);
            }
        }
        if (c->phasing_left > 0)
        {
            if (--c->phasing_left <= 0)
            {
                CLR_BIT(c->used, USES_PHASING_DEVICE);
                sound_play_sensors(c->pos, PHASING_OFF_SOUND);
            }
        }
    }
}

/* adds the given amount of an item to the cannon's inventory. the number of
   tanks is taken to be 1. amount is then the amount of fuel in that tank.
   fuel is given in 'units', but is stored in fuelpacks. */
void Cannon_add_item(cannon_t *c, int item_type, int amount)
{
    switch (item_type)
    {
    case ITEM_TANK:
        c->item[ITEM_TANK] += amount;
        LIMIT(c->item[ITEM_TANK], 0, world->items[ITEM_TANK].limit);
        /* FALLTHROUGH */
    case ITEM_FUEL:
        c->item[ITEM_FUEL] += (int)(amount / ENERGY_PACK_FUEL + 0.5);
        LIMIT(c->item[ITEM_FUEL],
              0,
              (int)(world->items[ITEM_FUEL].limit / ENERGY_PACK_FUEL + 0.5));
        break;
    default:
        c->item[item_type] += amount;
        LIMIT(c->item[item_type], 0, world->items[item_type].limit);
        break;
    }
}

// static inline int Cannon_get_initial_item(cannon_t *c, Item_t i)
// {
//     int init_amount;

//     init_amount = c->initial_items[i];
//     if (init_amount < 0)
//         init_amount = world->items[i].cannon_initial;

//     return init_amount;
// }

void Cannon_throw_items(cannon_t *c)
{
    int i, dir;
    object_t *obj;
    double velocity;

    for (i = 0; i < NUM_ITEMS; i++)
    {
        if (i == ITEM_FUEL)
            continue;
        c->item[i] -= world->items[i].initial;
        while (c->item[i] > 0)
        {
            int amount = world->items[i].max_per_pack - (int)(rfrac() * (1 + world->items[i].max_per_pack - world->items[i].min_per_pack));
            LIMIT(amount, 0, c->item[i]);
            if (rfrac() < (options.dropItemOnKillProb * CANNON_DROP_ITEM_PROB) && (obj = Object_allocate()) != NULL)
            {
                obj->objtype = OBJTYPE_ITEM;
                obj->info = i;
                obj->color = RED;
                obj->obj_status = GRAVITY;
                dir = (int)(c->dir - (CANNON_SPREAD * 0.5) + (rfrac() * CANNON_SPREAD));
                dir = MOD2(dir, RES);
                obj->id = NO_ID;
                obj->team = TEAM_NOT_SET;
                Object_position_init_clpos(obj, c->pos);
                velocity = rfrac() * 6;
                obj->vel.x = tcos(dir) * velocity;
                obj->vel.y = tsin(dir) * velocity;
                obj->acc.x = 0;
                obj->acc.y = 0;
                obj->mass = 10;
                obj->life = 1500 + (int)(rfrac() * 512);
                obj->count = amount;
                obj->pl_range = ITEM_SIZE / 2;
                obj->pl_radius = ITEM_SIZE / 2;
                world->items[i].num++;
                Cell_add_object(obj);
            }
            c->item[i] -= amount;
        }
    }
}

/* initializes the given cannon at startup or after death and gives it some
   items. */
void Cannon_init(cannon_t *c)
{
    int i;

    c->last_change = frame_loops;
    for (i = 0; i < NUM_ITEMS; i++)
    {
        c->item[i] = 0;
        if (options.cannonsUseItems)
            Cannon_add_item(c, i, (int)(rfrac() * (world->items[i].initial + 1)));
    }
    c->damaged = 0;
    c->tractor_target_id = NO_ID;
    c->tractor_count = 0;
    c->tractor_is_pressor = false;
    c->used = 0;
    c->emergency_shield_left = 0;
    c->phasing_left = 0;
}

void Cannon_check_defense(cannon_t *c)
{
    int defense = Cannon_select_defense(c);

    if (defense >= 0 && Cannon_in_danger(c))
        Cannon_defend(c, defense);
}

void Cannon_check_fire(cannon_t *c)
{
    player_t *pl = NULL;
    int dir = 0,
        weapon = Cannon_select_weapon(c);

    Cannon_aim(c, weapon, &pl, &dir);
    if (pl)
        Cannon_fire(c, weapon, pl, dir);
}

/* selects one of the available defenses. see cannon.h for descriptions. */
static int Cannon_select_defense(cannon_t *c)
{
    int smartness = Cannon_get_smartness(c);

    /* mode 0 does not defend */
    if (smartness == 0)
        return -1;

    /* still protected */
    if (BIT(c->used, USES_EMERGENCY_SHIELD) || BIT(c->used, USES_PHASING_DEVICE))
        return -1;

    if (c->item[ITEM_EMERGENCY_SHIELD])
        return CD_EM_SHIELD;

    if (c->item[ITEM_PHASING])
        return CD_PHASING;

    /* no defense available */
    return -1;
}

/* checks if a cannon is about to be hit by a hazardous object.
   mode 0 does not detect danger.
   modes 1 - 3 use progressively more accurate detection. */
static int Cannon_in_danger(cannon_t *c)
{
    const int range = 4 * BLOCK_SZ;
    const uint32_t kill_shots = (KILLING_SHOTS) | OBJ_MINE_BIT | OBJ_SHOT_BIT | OBJ_PULSE_BIT | OBJ_SMART_SHOT_BIT | OBJ_HEAT_SHOT_BIT | OBJ_TORPEDO_BIT | OBJ_ASTEROID_BIT;
    object_t *shot, **obj_list;
    const int max_objs = 100;
    int obj_count, i, danger = false;
    int npx, npy, tdx, tdy;
    int cpx = CLICK_TO_PIXEL(c->pos.cx), cpy = CLICK_TO_PIXEL(c->pos.cy);
    int smartness = Cannon_get_smartness(c);

    if (smartness == 0)
        return false;

    Cell_get_objects(c->pos, range, max_objs,
                     &obj_list, &obj_count);

    for (i = 0; (i < obj_count) && !danger; i++)
    {
        shot = obj_list[i];

        if (shot->life <= 0)
            continue;
        uint32_t typebit = OBJ_TYPEBIT(shot->objtype);
        if (!BIT(typebit, kill_shots))
            continue;
        if (BIT(shot->obj_status, FROMCANNON))
            continue;
        if (BIT(world->rules->mode, TEAM_PLAY) && options.teamImmunity && shot->team == c->team)
            continue;

        npx = shot->pix_pos.x;
        npy = shot->pix_pos.y;
        if (smartness > 1)
        {
            npx += shot->vel.x;
            npy += shot->vel.y;
            if (smartness > 2)
            {
                npx += shot->acc.x;
                npy += shot->acc.y;
            }
        }
        tdx = WRAP_DX(npx - cpx);
        tdy = WRAP_DY(npy - cpy);
        if (LENGTH(tdx, tdy) <= ((4.5 - smartness) * BLOCK_SZ))
        {
            danger = true;
            break;
        }
    }

    return danger;
}

/* activates the selected defense. */
static void Cannon_defend(cannon_t *c, int defense)
{
    switch (defense)
    {
    case CD_EM_SHIELD:
        c->emergency_shield_left += 4 * FPS;
        SET_BIT(c->used, USES_EMERGENCY_SHIELD);
        c->item[ITEM_EMERGENCY_SHIELD]--;
        sound_play_sensors(c->pos, EMERGENCY_SHIELD_ON_SOUND);
        break;
    case CD_PHASING:
        c->phasing_left += 4 * FPS;
        SET_BIT(c->used, USES_PHASING_DEVICE);
        c->tractor_count = 0;
        c->item[ITEM_PHASING]--;
        sound_play_sensors(c->pos, PHASING_ON_SOUND);
        break;
    default:
        warn("Cannon_defend: Unknown defense.");
        break;
    }
}

/* selects one of the available weapons. see cannon.h for descriptions. */
static int Cannon_select_weapon(cannon_t *c)
{
    if (c->item[ITEM_MINE] && rfrac() < 0.5)
        return CW_MINE;
    if (c->item[ITEM_MISSILE] && rfrac() < 0.5)
        return CW_MISSILE;
    if (c->item[ITEM_LASER] && (int)(rfrac() * (c->item[ITEM_LASER] + 1)))
        return CW_LASER;
    if (c->item[ITEM_ECM] && rfrac() < 0.333)
        return CW_ECM;
    if (c->item[ITEM_TRACTOR_BEAM] && rfrac() < 0.5)
        return CW_TRACTORBEAM;
    if (c->item[ITEM_TRANSPORTER] && rfrac() < 0.333)
        return CW_TRANSPORTER;
    if ((c->item[ITEM_AFTERBURNER] || c->item[ITEM_EMERGENCY_THRUST]) && c->item[ITEM_FUEL] && (int)(rfrac() * ((c->item[ITEM_EMERGENCY_THRUST] ? MAX_AFTERBURNER : c->item[ITEM_AFTERBURNER]) + 3)) > 2)
        return CW_GASJET;
    return CW_SHOT;
}

/* determines in which direction to fire.
   mode 0 fires straight ahead.
   mode 1 in a random direction.
   mode 2 aims at the current position of the closest player,
          then limits that to the sector in front of the cannon,
          then adds a small error.
   mode 3 calculates where the player will be when the shot reaches her,
          checks if that position is within limits and selects the player
          who will be closest in this way.
   the targeted player is also returned (for all modes).
   mode 0 always fires if it sees a player.
   modes 1 and 2 only fire if a player is within range of the selected weapon.
   mode 3 only fires if a player will be in range when the shot is expected to hit.
 */
static void Cannon_aim(cannon_t *c, int weapon, player_t **pl_p, int *dir)
{
    int speed = options.shotSpeed;
    int range = CANNON_SHOT_LIFE_MAX * speed;
    int cpx = (int)c->pix_pos.x;
    int cpy = (int)c->pix_pos.y;
    int visualrange = (int)(CANNON_DISTANCE + 2 * c->item[ITEM_SENSOR] * BLOCK_SZ);
    bool found = false, ready = false;
    int closest = range;
    int ddir, i, smartness = Cannon_get_smartness(c);

    switch (weapon)
    {
    case CW_MINE:
        speed = (int)(speed * 0.5 + 0.1 * smartness);
        range = (int)(range * 0.5 + 0.1 * smartness);
        break;
    case CW_LASER:
        speed = PULSE_SPEED;
        range = (int)(PULSE_LIFE(CANNON_PULSES) * speed);
        break;
    case CW_ECM:
        /* smarter cannons wait a little longer before firing an ECM */
        if (smartness > 1)
        {
            range = (int)((ECM_DISTANCE / smartness + (int)(rfrac() * (int)(ECM_DISTANCE - ECM_DISTANCE / smartness))));
        }
        else
        {
            range = (int)ECM_DISTANCE;
        }
        break;
    case CW_TRACTORBEAM:
        range = TRACTOR_MAX_RANGE(c->item[ITEM_TRACTOR_BEAM]);
        break;
    case CW_TRANSPORTER:
        /* smarter cannons have a smaller chance of using a transporter when
           target is out of range */
        if (smartness > 2 || (int)(rfrac() * sqr(smartness + 1)))
            range = (int)TRANSPORTER_DISTANCE;
        break;
    case CW_GASJET:
        if (c->item[ITEM_EMERGENCY_THRUST])
        {
            speed *= 2;
            range *= 2;
        }
        break;
    }

    for (i = 0; i < NumPlayers && !ready; i++)
    {
        player_t *pl = Player_by_index(i);
        int tdist, tdx, tdy;

        tdx = WRAP_DX(pl->pix_pos.x - cpx);
        if (ABS(tdx) >= visualrange)
            continue;
        tdy = WRAP_DY(pl->pix_pos.y - cpy);
        if (ABS(tdy) >= visualrange)
            continue;
        tdist = (int)LENGTH(tdx, tdy);
        if (tdist > visualrange)
            continue;

        /* mode 3 also checks if a player is using a phasing device */
        if (BIT(pl->obj_status, PLAYING | GAME_OVER | PAUSE | KILLED) != PLAYING ||
            (BIT(world->rules->mode, TEAM_PLAY) && pl->team == c->team) ||
            (!pl->forceVisible && BIT(pl->used, USES_CLOAKING_DEVICE) && (int)(rfrac() * (pl->item[ITEM_CLOAK] + 1)) > (int)(rfrac() * (c->item[ITEM_SENSOR] + 1))) ||
            (smartness > 2 && Player_is_phasing(pl)))
            continue;

        switch (smartness)
        {
        case 0:
            ready = true;
            break;
        default:
        case 1:
            if (tdist < range)
                ready = true;
            break;
        case 2:
            if (tdist < closest)
            {
                *dir = (int)findDir(tdx, tdy);
                found = true;
            }
            break;
        case 3:
            if (tdist < range)
            {
                double time = tdist / speed;
                int npx = (int)(pl->pix_pos.x + pl->vel.x * time + pl->acc.x * time * time);
                int npy = (int)(pl->pix_pos.y + pl->vel.y * time + pl->acc.y * time * time);
                int tdir;

                tdx = WRAP_DX(npx - cpx);
                tdy = WRAP_DY(npy - cpy);
                tdir = (int)findDir(tdx, tdy);
                ddir = MOD2(tdir - c->dir, RES);
                if ((ddir < (CANNON_SPREAD * 0.5) || ddir > RES - (CANNON_SPREAD * 0.5)) && (int)LENGTH(tdx, tdy) < closest)
                {
                    *dir = tdir;
                    found = true;
                }
            }
            break;
        }
        if (found || ready)
        {
            closest = tdist;
            *pl_p = pl;
        }
    }
    if (!(found || ready))
    {
        *pl_p = NULL;
        return;
    }

    switch (smartness)
    {
    case 0:
        *dir = c->dir;
        break;
    default:
    case 1:
        *dir = c->dir;
        *dir += (int)((rfrac() - 0.5) * CANNON_SPREAD);
        break;
    case 2:
        ddir = MOD2(*dir - c->dir, RES);
        if (ddir > (CANNON_SPREAD * 0.5) && ddir < RES / 2)
        {
            *dir = (int)(c->dir + (CANNON_SPREAD * 0.5) + 3);
        }
        else if (ddir < RES - (CANNON_SPREAD * 0.5) && ddir > RES / 2)
        {
            *dir = (int)(c->dir - (CANNON_SPREAD * 0.5) - 3);
        }
        *dir += (int)(rfrac() * 7) - 3;
        break;
    case 3:
        /* nothing to be done for mode 3 */
        break;
    }
    *dir = MOD2(*dir, RES);
}

/* does the actual firing. also determines in which way to use weapons that
   have more than one possible use. */
static void Cannon_fire(cannon_t *c, int weapon, player_t *pl, int dir)
{
    int cpx = (int)c->pix_pos.x;
    int cpy = (int)c->pix_pos.y;
    modifiers_t mods;
    bool played = false;
    int i, smartness = Cannon_get_smartness(c);
    int speed = options.shotSpeed;
    vector_t zero_vel = {0.0, 0.0};

    CLEAR_MODS(mods);
    switch (weapon)
    {
    case CW_MINE:
        if (BIT(world->rules->mode, ALLOW_CLUSTERS) && (rfrac() < 0.25))
            SET_BIT(mods.warhead, CLUSTER);
        if (BIT(world->rules->mode, ALLOW_MODIFIERS))
        {
            if (rfrac() >= 0.2)
                SET_BIT(mods.warhead, IMPLOSION);
            mods.power = (int)(rfrac() * (MODS_POWER_MAX + 1));
            mods.velocity = (int)(rfrac() * (MODS_VELOCITY_MAX + 1));
        }
        if (rfrac() < 0.5)
        { /* place mine in front of cannon */
            Place_general_mine(c->id, c->team, FROMCANNON,
                               c->pos, zero_vel, mods);
            sound_play_sensors(c->pos, DROP_MINE_SOUND);
            played = true;
        }
        else
        { /* throw mine at player */
            vector_t vel;
            if (BIT(world->rules->mode, ALLOW_MODIFIERS))
            {
                mods.mini = (int)(rfrac() * MODS_MINI_MAX) + 1;
                mods.spread = (int)(rfrac() * (MODS_SPREAD_MAX + 1));
            }
            speed = (int)(speed * 0.5 + 0.1 * smartness);
            vel.x = tcos(dir) * speed;
            vel.y = tsin(dir) * speed;
            Place_general_mine(c->id, c->team, GRAVITY | FROMCANNON,
                               c->pos, vel, mods);
            sound_play_sensors(c->pos, DROP_MOVING_MINE_SOUND);
            played = true;
        }
        c->item[ITEM_MINE]--;
        break;
    case CW_MISSILE:
        if (BIT(world->rules->mode, ALLOW_CLUSTERS) && (rfrac() < 0.333))
            SET_BIT(mods.warhead, CLUSTER);
        if (BIT(world->rules->mode, ALLOW_MODIFIERS))
        {
            if (rfrac() >= 0.25)
                SET_BIT(mods.warhead, IMPLOSION);
            mods.power = (int)(rfrac() * (MODS_POWER_MAX + 1));
            mods.velocity = (int)(rfrac() * (MODS_VELOCITY_MAX + 1));
            /* Because cannons don't have missile racks, all mini missiles
               would be fired from the same point and appear to the players
               as 1 missile (except heatseekers, which would appear to split
               in midair because of navigation errors (see Move_smart_shot)).
               Therefore, we don't minify cannon missiles.
            mods.mini = (int)(rfrac() * MODS_MINI_MAX) + 1;
            mods.spread = (int)(rfrac() * (MODS_SPREAD_MAX + 1));
            */
        }
        /* smarter cannons use more advanced missile types */
        switch ((int)(rfrac() * (1 + smartness)))
        {
        default:
            if (options.allowSmartMissiles)
            {
                Fire_general_shot(c->id, c->team, c->pos,
                                  OBJTYPE_SMART_SHOT, dir, mods, pl->id);
                sound_play_sensors(c->pos, FIRE_SMART_SHOT_SOUND);
                played = true;
                break;
            }
            /* FALLTHROUGH */
        case 1:
            if (options.allowHeatSeekers && Player_is_thrusting(pl))
            {
                Fire_general_shot(c->id, c->team, c->pos,
                                  OBJTYPE_HEAT_SHOT, dir, mods, pl->id);
                sound_play_sensors(c->pos, FIRE_HEAT_SHOT_SOUND);
                played = true;
                break;
            }
            /* FALLTHROUGH */
        case 0:
            Fire_general_shot(c->id, c->team, c->pos,
                              OBJTYPE_TORPEDO, dir, mods, NO_ID);
            sound_play_sensors(c->pos, FIRE_TORPEDO_SOUND);
            played = true;
            break;
        }
        c->item[ITEM_MISSILE]--;
        break;
    case CW_LASER:
        /* stun and blinding lasers are very dangerous,
           so we don't use them often */
        if (BIT(world->rules->mode, ALLOW_LASER_MODIFIERS) && (rfrac() * (8 - smartness)) >= 1)
            mods.laser = (int)(rfrac() * (MODS_LASER_MAX + 1));
        Fire_general_laser(c->id, c->team, c->pos, dir, mods);
        sound_play_sensors(c->pos, FIRE_LASER_SOUND);
        played = true;
        break;
    case CW_ECM:
        Fire_general_ecm(c->id, c->team, c->pos);
        c->item[ITEM_ECM]--;
        sound_play_sensors(c->pos, ECM_SOUND);
        played = true;
        break;
    case CW_TRACTORBEAM:
        /* smarter cannons use tractors more often and also push/pull longer */
        c->tractor_is_pressor = (rfrac() * (smartness + 1) >= 1);
        c->tractor_target_id = pl->id;
        c->tractor_count = 11 + (int)(rfrac() * ((3 * smartness) + 1));
        break;
    case CW_TRANSPORTER:
        c->item[ITEM_TRANSPORTER]--;
        if (Wrap_length(pl->pos.cx - c->pos.cx, pl->pos.cy - c->pos.cy) < TRANSPORTER_DISTANCE * CLICK)
        {
            int item = -1;
            double amount = 0.0;
            Do_general_transporter(c->id, c->pos, pl, &item, &amount);
            if (item != -1)
                Cannon_add_item(c, item, amount);
            // No sound here, mark as played, so doesn't play below.
            // TODO: get rid of played variable.
            played = true;
        }
        else
        {
            sound_play_sensors(c->pos, TRANSPORTER_FAIL_SOUND);
            played = true;
        }
        break;
    case CW_GASJET:
        /* use emergency thrusts to make extra big jets */
        if ((rfrac() * (c->item[ITEM_EMERGENCY_THRUST] + 1)) >= 1)
        {
            Make_debris(c->pos,
                        zero_vel,
                        NO_ID,
                        c->team,
                        OBJTYPE_SPARK,
                        THRUST_MASS,
                        GRAVITY | FROMCANNON,
                        RED,
                        8,
                        300, 700,
                        dir - 4 * (4 - smartness),
                        dir + 4 * (4 - smartness),
                        0.1, speed * 4,
                        3.0, 20.0);
            c->item[ITEM_EMERGENCY_THRUST]--;
        }
        else
        {
            Make_debris(c->pos,
                        zero_vel,
                        NO_ID,
                        c->team,
                        OBJTYPE_SPARK,
                        THRUST_MASS,
                        GRAVITY | FROMCANNON,
                        RED,
                        8,
                        150, 350,
                        dir - 3 * (4 - smartness),
                        dir + 3 * (4 - smartness),
                        0.1, speed * 2,
                        3.0, 20.0);
        }
        c->item[ITEM_FUEL]--;
        sound_play_sensors(c->pos, THRUST_SOUND);
        played = true;
        break;
    case CW_SHOT:
    default:
        if (options.cannonFlak)
            mods.warhead = CLUSTER;
        /* smarter cannons fire more accurately and
           can therefore narrow their bullet streams */
        for (i = 0; i < (1 + 2 * c->item[ITEM_WIDEANGLE]); i++)
        {
            int a_dir = dir + (4 - smartness) * (-c->item[ITEM_WIDEANGLE] + i);
            a_dir = MOD2(a_dir, RES);
            Fire_general_shot(c->id, c->team, c->pos,
                              OBJTYPE_CANNON_SHOT, a_dir, mods, NO_ID);
        }
        /* I'm not sure cannons should use rearshots.
           After all, they are restricted to 60 degrees when picking their
           target. */
        for (i = 0; i < c->item[ITEM_REARSHOT]; i++)
        {
            int a_dir = (int)(dir + (RES / 2) + (4 - smartness) * (-((c->item[ITEM_REARSHOT] - 1) * 0.5) + i));
            a_dir = MOD2(a_dir, RES);
            Fire_general_shot(c->id, c->team, c->pos,
                              OBJTYPE_CANNON_SHOT, a_dir, mods, NO_ID);
        }
    }

    /* finally, play sound effect */
    if (!played)
    {
        sound_play_sensors(c->pos, CANNON_FIRE_SOUND);
    }
}
