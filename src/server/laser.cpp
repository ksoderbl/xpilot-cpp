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

#include <vector>

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <cmath>
#include <climits>

#include "click.h"

#include "cell.h"
#include "frame.h"
#include "server.h"
#include "ship.h"

#include "xpconfig.h"
#include "serverconst.h"

#include "map.h"
#include "score.h"
#include "saudio.h"
#include "xperror.h"
#include "portability.h"
#include "object.h"
#include "asteroid.h"
#include "commonproto.h"
#include "robot.h"
#include "cannon.h"
#include "walls.h"

#include "update.h"

void Fire_laser(player_t *pl)
{
    world_t *world = &World;

    if (pl->item[ITEM_LASER] > pl->num_pulses && pl->velocity < PULSE_SPEED - PULSE_SAMPLE_DISTANCE)
    {
        // TODO: should this not be ED_LASER ?
        if (pl->fuel.sum <= -ED_LASER_HIT)
            CLR_BIT(pl->used, HAS_LASER);
        else
        {
            clpos_t pos;
            clpos_t m_gun = Ship_get_m_gun_clpos(pl->ship, pl->dir);
            pos.cx = pl->pos.cx + m_gun.cx + FLOAT_TO_CLICK(pl->vel.x);
            pos.cy = pl->pos.cy + m_gun.cy + FLOAT_TO_CLICK(pl->vel.y);
            pos.cx = WORLD_WRAP_XCLICK(world, pos.cx);
            pos.cy = WORLD_WRAP_YCLICK(world, pos.cy);
            if (World_contains_clpos(world, pos))
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
    if (Pulses[NumPulses] == nullptr)
        return;

    pulse = Pulses[NumPulses];
    pulse->id = (pl ? pl->id : NO_ID);
    pulse->team = team;
    pulse->dir = dir;
    pulse->len = PULSE_LENGTH;
    pulse->life = life;
    pulse->mods = mods;
    pulse->refl = false;
    pulse->pos.cx = pos.cx - PULSE_SPEED * tcos(dir) * CLICK;
    pulse->pos.cy = pos.cy - PULSE_SPEED * tsin(dir) * CLICK;
    NumPulses++;
    if (pl)
        pl->num_pulses++;
}

/*
 * Type to hold info about a player
 * which might be hit by a laser pulse.
 */
typedef struct victim
{
    int ind;          /* player index */
    clpos_t pos;      /* current player position */
    double prev_dist; /* distance at previous sample */
} victim_t;

/*
 * Type to hold info about all players
 * which may be hit by a laser pulse.
 */
typedef struct vicbuf
{
    int num_vic;       /* number of victims. */
    int max_vic;       /* max number */
    victim_t *vic_ptr; /* pointer to buffer for victims */
} vicbuf_t;

/*
 * Destroy one laser pulse.
 */
static void Laser_pulse_destroy_one(int pulse_index)
{
    // int ind;
    player_t *pl;
    pulse_t *pulse_ptr;

    pulse_ptr = Pulses[pulse_index];
    if (pulse_ptr->id != NO_ID)
    {
        pl = Player_by_id(pulse_ptr->id);
        pl->num_pulses--;
    }

    free(pulse_ptr);

    if (--NumPulses > pulse_index)
    {
        Pulses[pulse_index] = Pulses[NumPulses];
    }
}

/*
 * Destroy all laser pulses.
 */
static void Laser_pulse_destroy_all(void)
{
    int p;

    for (p = NumPulses - 1; p >= 0; --p)
    {
        Laser_pulse_destroy_one(p);
    }
}

/*
 * Loop over all players and put the
 * ones which are close the pulse midpoint
 * in a vicbuf structure.
 */
static void Laser_pulse_find_victims(
    vicbuf_t *vicbuf,
    pulse_t *pulse,
    double midx,
    double midy)
{
    world_t *world = &World;
    int i;
    player_t *vic;
    double dist;
    int midcx = FLOAT_TO_CLICK(midx);
    int midcy = FLOAT_TO_CLICK(midy);

    vicbuf->num_vic = 0;
    for (i = 0; i < NumPlayers; i++)
    {
        vic = Player_by_index(i);

        if (!Player_is_alive(vic))
            continue;

        if (Player_is_phasing(vic))
            continue;

        if (vic->id == pulse->id && options.selfImmunity)
            continue;
        if (options.selfImmunity &&
            Player_is_tank(vic) &&
            vic->lock.pl_id == pulse->id)
            continue;
        if (Team_immune(vic->id, pulse->id))
            continue;

        /* special case for cannon pulses */
        if (pulse->id == NO_ID &&
            options.teamImmunity &&
            Team_play(world) &&
            pulse->team == vic->team)
            continue;

        if (vic->id == pulse->id && !pulse->refl)
            continue;

        dist = World_wrap_length(
                   world,
                   vic->pos.cx - midcx,
                   vic->pos.cy - midcy) /
               CLICK;
        if (dist > pulse->len / 2 + SHIP_SZ)
            continue;

        if (vicbuf->max_vic == 0)
        {
            size_t victim_bufsize = NumPlayers * sizeof(victim_t);
            vicbuf->vic_ptr = (victim_t *)malloc(victim_bufsize);
            if (vicbuf->vic_ptr == nullptr)
                break;

            vicbuf->max_vic = NumPlayers;
        }
        vicbuf->vic_ptr[vicbuf->num_vic].ind = i;
        vicbuf->vic_ptr[vicbuf->num_vic].pos.cx = vic->pos.cx;
        vicbuf->vic_ptr[vicbuf->num_vic].pos.cy = vic->pos.cy;
        vicbuf->vic_ptr[vicbuf->num_vic].prev_dist = 1e10;
        vicbuf->num_vic++;
    }
}

/*
 * Do what needs to be done when a laser pulse
 * actually hits a player.
 * If the pulse was reflected by a mirror
 * then set "refl" to true.
 */
static void Laser_pulse_hits_player(
    pulse_t *pulse,
    object_t *obj,
    double x,
    double y,
    victim_t *victim,
    bool *refl)
{
    world_t *world = &World;
    player_t *pl;
    player_t *vicpl;
    // int ind;
    int sc;
    char msg[MSG_LEN];

    if (pulse->id != NO_ID)
        pl = Player_by_id(pulse->id);
    else
        pl = nullptr;

    vicpl = PlayersArray[victim->ind];
    vicpl->forceVisible++;

    if (BIT(vicpl->have, HAS_MIRROR) && (rfrac() * (2 * vicpl->item[ITEM_MIRROR])) >= 1)
    {
        double px = x - tcos(pulse->dir) * 0.5 * PULSE_SAMPLE_DISTANCE;
        double py = y - tsin(pulse->dir) * 0.5 * PULSE_SAMPLE_DISTANCE;

        pulse->pos.cx = FLOAT_TO_CLICK(px);
        pulse->pos.cy = FLOAT_TO_CLICK(py);
        pulse->dir = (int)World_wrap_cfindDir(
                         world,
                         vicpl->pos.cx - pulse->pos.cx,
                         vicpl->pos.cy - pulse->pos.cy) *
                         2 -
                     ANGLE_RESOLUTION / 2 - pulse->dir;
        pulse->dir = MOD2(pulse->dir, ANGLE_RESOLUTION);
        pulse->life += vicpl->item[ITEM_MIRROR];
        pulse->len = PULSE_LENGTH;
        pulse->refl = true;
        *refl = true;
        return;
    }

    sound_play_sensors(vicpl->pos, PLAYER_EAT_LASER_SOUND);
    if (Player_uses_emergency_shield(vicpl))
        return;
    if (!BIT(obj->type, KILLING_SHOTS))
        return;
    // if (BIT(pulse->mods.laser, MODS_LASER_STUN) || (options.laserIsStunGun == true && options.allowLaserModifiers == false))
    if ((Mods_get(pulse->mods, ModsLaser) & MODS_LASER_STUN) || (options.laserIsStunGun == true && options.allowLaserModifiers == false))
    {
        if (BIT(vicpl->used, HAS_SHIELD | HAS_LASER | HAS_SHOT) || BIT(vicpl->obj_status, THRUSTING))
        {
            if (pl)
            {
                sprintf(msg,
                        "%s got paralysed by %s's stun laser.",
                        vicpl->name, pl->name);
                if (vicpl->id == pl->id)
                    strcat(msg, " How strange!");
            }
            else
            {
                sprintf(msg,
                        "%s got paralysed by a stun laser.",
                        vicpl->name);
            }
            Set_message(msg);
            CLR_BIT(vicpl->used,
                    HAS_SHIELD | HAS_LASER | OBJ_SHOT_BIT);
            CLR_BIT(vicpl->obj_status, THRUSTING);
            vicpl->stunned += 5;
        }
    }
    // else if (BIT(pulse->mods.laser, MODS_LASER_BLIND))
    else if (Mods_get(pulse->mods, ModsLaser) & MODS_LASER_BLIND)
    {
        vicpl->damaged += (FPS + 6);
        vicpl->forceVisible += (FPS + 6);
        if (pl)
            Record_shove(vicpl, pl, frame_loops + FPS + 6);
    }
    else
    {
        Player_add_fuel(vicpl, ED_LASER_HIT);
        if (!BIT(vicpl->used, HAS_SHIELD) && !BIT(vicpl->have, HAS_ARMOR))
        {
            Player_set_state(vicpl, PL_STATE_KILLED);

            if (pl)
            {
                sprintf(msg,
                        "%s got roasted alive by %s's laser.",
                        vicpl->name, pl->name);
                if (vicpl->id == pl->id)
                {
                    sc = Rate(0, pl->score) * options.laserKillScoreMult * options.selfKillScoreMult;
                    Score(vicpl, -sc, vicpl->pos, vicpl->name);
                    strcat(msg, " How strange!");
                }
                else
                {
                    sc = Rate(pl->score,
                              vicpl->score) *
                         options.laserKillScoreMult;
                    Score_players(pl, sc, vicpl->name,
                                  vicpl, -sc, pl->name);
                }
            }
            else
            {
                sc = Rate(CANNON_SCORE, vicpl->score) / 4;
                Score(vicpl, -sc, vicpl->pos, "Cannon");
                sprintf(msg,
                        "%s got roasted alive by cannonfire.",
                        vicpl->name);
            }
            sound_play_sensors(vicpl->pos, PLAYER_ROASTED_SOUND);
            Set_message(msg);
            if (pl && pl->id != vicpl->id)
            {
                pl->kills++;
                Robot_war(vicpl, pl);
            }
        }
        if (!BIT(vicpl->used, HAS_SHIELD) && BIT(vicpl->have, HAS_ARMOR))
            Player_hit_armor(vicpl);
    }
}

/*
 * Check a given pulse position against a list of players.
 * Do what needs to be done when on any pulse hits player event.
 * Return the number of hits.
 * When the pulse was reflected then "refl" will have been set to true.
 */
static int Laser_pulse_check_player_hits(
    pulse_t *pulse,
    object_t *obj,
    double x,
    double y,
    vicbuf_t *vicbuf,
    bool *refl)
{
    world_t *world = &World;
    int j;
    int hits = 0;
    /* int                        ind; */
    double dist;
    /* player                *pl; */
    victim_t *victim;

    int cx = FLOAT_TO_CLICK(x);
    int cy = FLOAT_TO_CLICK(y);

    /*
    if (pulse->id != NO_ID) {
        ind = GetInd[pulse->id];
        pl = PlayersArray[ind];
    } else {
        ind = -1;
        pl = nullptr;
    }
    */

    for (j = vicbuf->num_vic - 1; j >= 0; --j)
    {
        victim = &(vicbuf->vic_ptr[j]);
        dist = World_wrap_length(
                   world,
                   cx - victim->pos.cx,
                   cy - victim->pos.cy) /
               CLICK;
        if (dist <= SHIP_SZ)
        {
            Laser_pulse_hits_player(
                pulse,
                obj,
                x, y,
                victim,
                refl);
            hits++;
            /* stop at the first hit. */
            break;
        }
        else if (dist >= victim->prev_dist)
            /* remove victim by copying the last victim over it */
            vicbuf->vic_ptr[j] = vicbuf->vic_ptr[--vicbuf->num_vic];
        else
            /* remember shortest distance from pulse to player_t */
            vicbuf->vic_ptr[j].prev_dist = dist;
    }

    return hits;
}

static void Laser_pulse_get_object_list(
    std::vector<object_t *> &obj_list,
    pulse_t *pulse,
    double midx,
    double midy)
{
    world_t *world = &World;
    double dx, dy;
    int range;
    object_t *ast;

    obj_list.clear();

    std::vector<wireobject_t *> &asteroids = Asteroid_get_list();
    if (asteroids.size() > 0)
    {
        /* fill list with interesting objects
         * which are close to our pulse. */
        for (wireobject_t *wireobject : asteroids)
        {
            ast = OBJ_PTR(wireobject);
            dx = midx - CLICK_TO_FLOAT(ast->pos.cx);
            dy = midy - CLICK_TO_FLOAT(ast->pos.cy);
            dx = WORLD_WRAP_DX(world, dx);
            dy = WORLD_WRAP_DY(world, dy);
            range = ast->pl_radius + pulse->len / 2;
            if (sqr(dx) + sqr(dy) < sqr(range))
                obj_list.push_back(ast);
        }
    }
}

/*
 * For all existing laser pulse check
 * if they collide with ships or asteroids.
 */
void Laser_pulse_collision(void)
{
    world_t *world = &World;
    int i;
    int p;
    int max, hits;
    bool refl;
    vicbuf_t vicbuf;
    double x, y, x1, x2, y1, y2;
    double dx, dy;
    double midx, midy;
    player_t *pl;
    pulse_t *pulse;
    object_t *obj = nullptr;
    std::vector<object_t *> obj_list;

    /*
     * Allocate one object with which we will
     * do pulse wall bounce checking.
     */
    if ((obj = Object_allocate()) == nullptr)
    {
        /* overload.  we can't do bounce checking. */
        Laser_pulse_destroy_all();
        return;
    }

    /* init vicbuf */
    vicbuf.num_vic = 0;
    vicbuf.max_vic = 0;
    vicbuf.vic_ptr = nullptr;

    for (p = NumPulses - 1; p >= 0; --p)
    {
        pulse = Pulses[p];

        /* check for end of pulse life */
        if (--pulse->life < 0 || pulse->len < PULSE_LENGTH)
        {
            Laser_pulse_destroy_one(p);
            continue;
        }

        if (pulse->id != NO_ID)
            pl = Player_by_id(pulse->id);
        else
            pl = nullptr;

        pulse->pos.cx += tcos(pulse->dir) * PULSE_SPEED * CLICK;
        pulse->pos.cy += tsin(pulse->dir) * PULSE_SPEED * CLICK;

        if (BIT(world->rules.mode, WRAP_PLAY))
        {
            pulse->pos = World_wrap_clpos(world, pulse->pos);

            x1 = CLICK_TO_FLOAT(pulse->pos.cx);
            y1 = CLICK_TO_FLOAT(pulse->pos.cy);
            x2 = x1 + tcos(pulse->dir) * pulse->len;
            y2 = y1 + tsin(pulse->dir) * pulse->len;
        }
        else
        {
            x1 = CLICK_TO_FLOAT(pulse->pos.cx);
            y1 = CLICK_TO_FLOAT(pulse->pos.cy);

            if (x1 < 0 || x1 >= World.width || y1 < 0 || y1 >= World.height)
            {
                pulse->len = 0;
                continue;
            }
            x2 = x1 + tcos(pulse->dir) * pulse->len;
            if (x2 < 0)
            {
                pulse->len = (int)(pulse->len * (0 - x1) / (x2 - x1));
                x2 = x1 + tcos(pulse->dir) * pulse->len;
            }
            if (x2 >= World.width)
            {
                pulse->len = (int)(pulse->len * (World.width - 1 - x1) / (x2 - x1));
                x2 = x1 + tcos(pulse->dir) * pulse->len;
            }
            y2 = y1 + tsin(pulse->dir) * pulse->len;
            if (y2 < 0)
            {
                pulse->len = (int)(pulse->len * (0 - y1) / (y2 - y1));
                x2 = x1 + tcos(pulse->dir) * pulse->len;
                y2 = y1 + tsin(pulse->dir) * pulse->len;
            }
            if (y2 > World.height)
            {
                pulse->len = (int)(pulse->len * (World.height - 1 - y1) / (y2 - y1));
                x2 = x1 + tcos(pulse->dir) * pulse->len;
                y2 = y1 + tsin(pulse->dir) * pulse->len;
            }
            if (pulse->len <= 0)
            {
                pulse->len = 0;
                continue;
            }
        }

        /* calculate delta x and y for pulse start and end position. */
        dx = x2 - x1;
        dy = y2 - y1;
        dx = WORLD_WRAP_DX(world, dx);
        dy = WORLD_WRAP_DY(world, dy);

        /* max is the highest absolute delta length of either x or y. */
        max = (int)MAX(ABS(dx), ABS(dy));
        if (max == 0)
            continue;

        /* calculate the midpoint of the new laser pulse position. */
        midx = x1 + (dx * 0.5);
        midy = y1 + (dy * 0.5);
        midx = WRAP_XPIXEL(world, midx);
        midy = WRAP_YPIXEL(world, midy);

        /* assemble a shortlist of players which might get hit. */
        Laser_pulse_find_victims(&vicbuf, pulse, midx, midy);

        Laser_pulse_get_object_list(
            obj_list,
            pulse,
            midx, midy);

        // if (obj_list.size() > 0)
        // {
        //     printf("Laser_pulse_collision: pulse %d, obj_list.size() = %d\n", p, obj_list.size());
        //     if (pl)
        //         printf("Laser_pulse_collision: Player %d: %s\n", GetInd(pl->id), pl->name);
        // }

        pulseobject_t *pulse_ptr = PULSE_PTR(obj);
        pulse_ptr->type = OBJ_PULSE_BIT;
        pulse_ptr->obj_life = 1;
        pulse_ptr->id = pulse->id;
        pulse_ptr->team = pulse->team;
        pulse_ptr->dirty_pulse_count = 0;
        pulse_ptr->obj_status = 0;
        if (pulse->id == NO_ID)
            pulse_ptr->obj_status = FROMCANNON;
        clpos_t pos;
        pos.cx = FLOAT_TO_CLICK(x1);
        pos.cy = FLOAT_TO_CLICK(y1);
        Object_position_init_clpos(obj, pos);

        refl = false;

        for (i = hits = 0; i <= max; i += PULSE_SAMPLE_DISTANCE)
        {
            x = x1 + (i * dx) / max;
            y = y1 + (i * dy) / max;
            obj->vel.x = (x - CLICK_TO_FLOAT(obj->pos.cx));
            obj->vel.y = (y - CLICK_TO_FLOAT(obj->pos.cy));
            /* changed from = x - obj->pos.x to make lasers disappear
               less frequently when wrapping. There's still a small
               chance of it happening though. */
            Move_object(obj);
            if (obj->obj_life == 0)
                break;
            if (BIT(world->rules.mode, WRAP_PLAY))
            {
                if (x < 0)
                {
                    x += World.width;
                    x1 += World.width;
                }
                else if (x >= World.width)
                {
                    x -= World.width;
                    x1 -= World.width;
                }
                if (y < 0)
                {
                    y += World.height;
                    y1 += World.height;
                }
                else if (y >= World.height)
                {
                    y -= World.height;
                    y1 -= World.height;
                }
            }

            /* check for collision with objects. */
            if (obj_list.size() > 0)
            {
                for (object_t *ast : obj_list)
                {
                    double adx, ady;
                    adx = x - CLICK_TO_FLOAT(ast->pos.cx);
                    ady = y - CLICK_TO_FLOAT(ast->pos.cy);
                    adx = WORLD_WRAP_DX(world, adx);
                    ady = WORLD_WRAP_DY(world, ady);

                    if (sqr(adx) + sqr(ady) <= sqr(ast->pl_radius))
                    {
                        obj->obj_life = 0.0;
                        ast->obj_life += ASTEROID_FUEL_HIT(ED_LASER_HIT,
                                                           WIRE_PTR(ast)->wire_size);
                        if (ast->obj_life < 0.0)
                            ast->obj_life = 0.0;
                        if (ast->obj_life == 0 && pl && options.asteroidPoints > 0 && pl->score <= options.asteroidMaxScore)
                            Score(pl, options.asteroidPoints, ast->pos, "");
                        break;
                    }
                }
            }

            if (obj->obj_life == 0)
                /* pulse hit asteroid */
                continue;

            hits = Laser_pulse_check_player_hits(
                pulse, obj,
                x, y,
                &vicbuf,
                &refl);

            if (hits > 0)
                break;
        }

        if (i < max && refl == false)
            pulse->len = (pulse->len * i) / max;
    }
    if (vicbuf.max_vic > 0 && vicbuf.vic_ptr != nullptr)
        free(vicbuf.vic_ptr);

    obj->type = OBJ_DEBRIS;
    obj->obj_life = 0.0;
    Cell_add_object(obj);
}

/*
 * Do what needs to be done when a laser pulse
 * actually hits a player.
 */
void Laser_pulse_hits_player2(player_t *pl, pulseobject_t *pulse)
{
    world_t *world = &World;
    player_t *kp = Player_by_id(pulse->id);
    cannon_t *cannon = nullptr;

    if (kp == nullptr)
        /* Perhaps it was a cannon pulse? */
        cannon = Cannon_by_id(pulse->id);

    pl->forceVisible += 1;
    if (Player_has_mirror(pl) && (rfrac() * (2 * pl->item[ITEM_MIRROR])) >= 1)
    {
        pulse->pulse_dir = (int)(World_wrap_cfindDir(
                                     world,
                                     pl->pos.cx - pulse->pos.cx,
                                     pl->pos.cy - pulse->pos.cy) *
                                     2 -
                                 ANGLE_RESOLUTION / 2 - pulse->pulse_dir);
        pulse->pulse_dir = MOD2(pulse->pulse_dir, ANGLE_RESOLUTION);

        pulse->vel.x = options.pulseSpeed * tcos(pulse->pulse_dir);
        pulse->vel.y = options.pulseSpeed * tsin(pulse->pulse_dir);

        pulse->obj_life += pl->item[ITEM_MIRROR];
        pulse->pulse_len = 0 /*PULSE_LENGTH*/;
        pulse->pulse_refl = true;
        return;
    }

    sound_play_sensors(pl->pos, PLAYER_EAT_LASER_SOUND);
    if (Player_uses_emergency_shield(pl))
        return;
    assert(pulse->type == OBJ_PULSE);

    /* kps - do we need some hack so that the laser pulse is
     * not removed in the same frame that its life ends ?? */
    pulse->obj_life = 0.0;
    if ((Mods_get(pulse->mods, ModsLaser) & MODS_LASER_STUN) || (options.laserIsStunGun && options.allowLaserModifiers == false))
    {
        if (BIT(pl->used, HAS_SHIELD | HAS_LASER | HAS_SHOT) || Player_is_thrusting(pl))
        {
            if (kp)
                Set_message_f("%s got paralysed by %s's stun laser.%s",
                              pl->name, kp->name,
                              pl->id == kp->id ? " How strange!" : "");
            else
                Set_message_f("%s got paralysed by a stun laser.", pl->name);

            CLR_BIT(pl->used,
                    HAS_SHIELD | HAS_LASER | OBJ_SHOT);
            Thrust(pl, false);
            pl->stunned += 5;
        }
    }
    else if (Mods_get(pulse->mods, ModsLaser) & MODS_LASER_BLIND)
    {
        pl->damaged += (12 + 6);
        pl->forceVisible += (12 + 6);
        if (kp)
            Record_shove(pl, kp, frame_loops + 12 + 6);
    }
    else
    {
        Player_add_fuel(pl, ED_LASER_HIT);
        if (!BIT(pl->used, HAS_SHIELD) && !Player_has_armor(pl))
        {
            Player_set_state(pl, PL_STATE_KILLED);
            Handle_Scoring(SCORE_LASER, kp, pl, cannon, nullptr);
            if (kp)
            {
                Set_message_f("%s got roasted alive by %s's laser.%s",
                              pl->name, kp->name,
                              pl->id == kp->id ? " How strange!" : "");
            }
            else if (cannon != nullptr)
            {
                Set_message_f("%s got roasted alive by cannonfire.", pl->name);
            }
            else
            {
                assert(pulse->id == NO_ID);
                Set_message_f("%s got roasted alive.", pl->name);
            }

            sound_play_sensors(pl->pos, PLAYER_ROASTED_SOUND);
            if (kp && kp->id != pl->id)
            {
                Robot_war(pl, kp);
            }
        }
        if (!BIT(pl->used, HAS_SHIELD) && Player_has_armor(pl))
            Player_hit_armor(pl);
    }
}
