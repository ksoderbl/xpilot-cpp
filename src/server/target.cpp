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

#include "target.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <cmath>
#include <climits>

#include "commonproto.h"
#include "const.h"
#include "xperror.h"

#include "cannon.h"
#include "frame.h"
#include "server.h"
#include "ship.h"

#include "xpconfig.h"
#include "serverconst.h"

#include "score.h"
#include "saudio.h"
#include "item.h"
#include "walls1.h"
#include "click.h"
#include "object.h"
#include "robot.h"
#include "map.h"
#include "serveroption.h"
#include "server.h"

/*
 * Update targets
 */
void Target_update(void)
{
    world_t *world = &World;
    int i, j;

    for (i = 0; i < Num_targets(); i++)
    {
        target_t *targ = Target_by_index(i);

        if (targ->dead_ticks > 0)
        {
            if ((targ->dead_ticks -= timeStep) <= 0)
            {
                // World.block[targ->blk_pos.bx][targ->blk_pos.by] = TARGET;
                // targ->conn_mask = 0;
                // targ->update_mask = (unsigned)-1;
                // targ->last_change = frame_loops;
                World_restore_target(world, targ);

                if (options.targetSync)
                {
                    for (j = 0; j < Num_targets(); j++)
                    {
                        target_t *t = Target_by_index(j);

                        if (t->team == targ->team)
                        {
                            // World.block[t->blk_pos.bx][t->blk_pos.by] = TARGET;
                            // t->conn_mask = 0;
                            // t->update_mask = (unsigned)-1;
                            // t->last_change = frame_loops;
                            // t->dead_ticks = 0;
                            t->damage = TARGET_DAMAGE;
                            World_restore_target(world, targ);
                        }
                    }
                }
            }
            continue;
        }
        else if (targ->damage == TARGET_DAMAGE)
            continue;

        targ->damage += TARGET_REPAIR_PER_TICK * timeStep;
        if (targ->damage >= TARGET_DAMAGE)
            targ->damage = TARGET_DAMAGE;

        else if (targ->last_change + TARGET_UPDATE_DELAY < frame_loops)
            /*
             * We don't send target info to the clients every frame
             * if the latest repair wouldn't change their display.
             */
            continue;

        targ->conn_mask = 0;
        targ->last_change = frame_loops;
    }
}

void Object_hits_target2(object_t *obj, target_t *targ, double player_cost)
{
    world_t *world = &World;
    int j;
    player_t *kp;
    double win_score = 0.0, lose_score = 0.0, drainfactor;
    int win_team_members = 0, lose_team_members = 0,
        targets_remaining = 0, targets_total = 0;
    vector_t zero_vel = {0.0, 0.0};
    bool somebody = false;

    /* a normal shot or a direct mine hit work, cannons don't */
    /* KK: should shots/mines by cannons of opposing teams work? */
    /* also players suiciding on target will cause damage */
    if (!BIT(OBJ_TYPEBIT(obj->type),
             KILLING_SHOTS | OBJ_MINE_BIT | OBJ_PULSE_BIT | OBJ_PLAYER_BIT))
        return;

    if (obj->id == NO_ID)
        return;

    kp = Player_by_id(obj->id);

    /* Targets are always team immune. */
    if (targ->team != TEAM_NOT_SET && targ->team == obj->team)
        return;

    switch (obj->type)
    {
    case OBJ_SHOT:
        // TODO
        // if (options.shotHitFuelDrainUsesKineticEnergy)
        // {
        //     drainfactor = VECTOR_LENGTH(obj->vel);
        //     drainfactor = (drainfactor * drainfactor * ABS(obj->mass)) / (options.shotSpeed * options.shotSpeed * options.shotMass);
        // }
        // else
        drainfactor = 1.0;
        targ->damage += ED_SHOT_HIT * drainfactor * SHOT_MULT(obj);
        break;
    case OBJ_PULSE:
        targ->damage += ED_LASER_HIT;
        break;
    case OBJ_SMART_SHOT:
    case OBJ_TORPEDO:
    case OBJ_HEAT_SHOT:
        if (!obj->mass)
            /* happens at end of round reset. */
            return;
        if (Mods_get(obj->mods, ModsNuclear))
            targ->damage = 0.0;
        else
            targ->damage += ED_SMART_SHOT_HIT / (Mods_get(obj->mods, ModsMini) + 1);
        break;
    case OBJ_MINE:
        if (!obj->mass)
            /* happens at end of round reset. */
            return;
        targ->damage -= TARGET_DAMAGE / (Mods_get(obj->mods, ModsMini) + 1);
        break;
    case OBJ_PLAYER:
        if (player_cost <= 0.0 || player_cost > TARGET_DAMAGE / 4.0)
            player_cost = TARGET_DAMAGE / 4.0;
        targ->damage -= player_cost;
        break;

    default:
        /*???*/
        break;
    }

    targ->conn_mask = 0;
    targ->last_change = frame_loops;
    if (targ->damage > 0.0)
        return;

    World_remove_target(world, targ);

    Make_debris(targ->pos,
                zero_vel,
                NO_ID,
                targ->team,
                OBJ_DEBRIS,
                4.5,
                GRAVITY,
                RED,
                6,
                (int)(75 + 75 * rfrac()),
                0, ANGLE_RESOLUTION - 1,
                20.0, 70.0,
                10.0, 100.0);

    if (Team_play(world))
    {
        for (j = 0; j < NumPlayers; j++)
        {
            player_t *pl = Player_by_index(j);

            if (Player_is_tank(pl) ||
                (Player_is_paused(pl) && pl->pause_count <= 0) ||
                Player_is_waiting(pl))
                continue;

            if (pl->team == targ->team)
            {
                lose_score += Get_Score(pl);
                lose_team_members++;
                if (!Player_is_dead(pl))
                    somebody = true;
            }
            else if (pl->team == kp->team)
            {
                win_score += Get_Score(pl);
                win_team_members++;
            }
        }
    }
    if (somebody)
    {
        for (j = 0; j < Num_targets(); j++)
        {
            target_t *t = Target_by_index(j);

            if (t->team == targ->team)
            {
                targets_total++;
                if (t->dead_ticks <= 0)
                    targets_remaining++;
            }
        }
    }
    if (!somebody)
        return;

    Handle_Scoring(SCORE_TARGET, kp, nullptr, targ, nullptr);

    sound_play_sensors(targ->pos, DESTROY_TARGET_SOUND);

    if (targets_remaining > 0)
    {
        /*
         * If players can't collide with their own targets, we
         * assume there are many used as shields.  Don't litter
         * the game with the message below.
         */
        if (options.targetTeamCollision && targets_total < 10)
            Set_message_f("%s blew up one of team %d's targets.",
                          kp->name, targ->team);
        return;
    }

    Set_message_f("%s blew up team %d's %starget.",
                  kp->name, targ->team, (targets_total > 1) ? "last " : "");
}

void Object_hits_target1(object_t *obj, target_t *targ, double player_cost)
{
    world_t *world = &World;
    int j, sc, por, bx, by;
    int win_score = 0,
        lose_score = 0;
    int win_team_members = 0,
        lose_team_members = 0,
        targets_remaining = 0,
        targets_total = 0;
    int drainfactor;
    vector_t zero_vel = {0.0, 0.0};
    player_t *kp;
    bool somebody = false;

    /* a normal shot or a direct mine hit work, cannons don't */
    /* KK: should shots/mines by cannons of opposing teams work? */
    /* also players suiciding on target will cause damage */
    if (!BIT(obj->type, KILLING_SHOTS | OBJ_MINE_BIT | OBJ_PULSE_BIT | OBJ_PLAYER_BIT))
        return;

    if (obj->id == NO_ID)
        return;

    kp = Player_by_id(obj->id);

    /* Targets are always team immune. */
    if (targ->team == obj->team)
        return;

    switch (obj->type)
    {
    case OBJ_SHOT:
        drainfactor = 1.0;
        targ->damage += ED_SHOT_HIT * drainfactor * SHOT_MULT(obj);
        break;
    case OBJ_PULSE:
        targ->damage += ED_LASER_HIT;
        break;
    case OBJ_SMART_SHOT:
    case OBJ_TORPEDO:
    case OBJ_HEAT_SHOT:
        if (!obj->mass)
            /* happens at end of round reset. */
            return;
        if (Mods_get(obj->mods, ModsNuclear) & MODS_NUCLEAR)
            targ->damage = 0.0;
        else
            targ->damage += ED_SMART_SHOT_HIT / (Mods_get(obj->mods, ModsMini) + 1);
        break;
    case OBJ_MINE:
        if (!obj->mass)
            /* happens at end of round reset. */
            return;
        targ->damage -= TARGET_DAMAGE / (Mods_get(obj->mods, ModsMini) + 1);
        break;
    case OBJ_PLAYER:
        if (player_cost <= 0.0 || player_cost > TARGET_DAMAGE / 4.0)
            player_cost = TARGET_DAMAGE / 4.0;
        targ->damage -= player_cost;
        break;

    default:
        /*???*/
        break;
    }

    targ->conn_mask = 0;
    targ->last_change = frame_loops;
    if (targ->damage > 0.0)
        return;

    targ->update_mask = (unsigned)-1;
    targ->damage = TARGET_DAMAGE;
    targ->dead_ticks = options.targetDeadTime;

    /*
     * Destroy target.
     * Turn it into a space to simplify other calculations.
     */
    blkpos_t blkpos = Clpos_to_blkpos(targ->pos);
    // World.block[blkpos.bx][blkpos.by] = SPACE;
    World_set_block(world, blkpos, SPACE);

    Make_debris(targ->pos,
                zero_vel,
                NO_ID,
                targ->team,
                OBJ_DEBRIS,
                4.5,
                GRAVITY,
                RED,
                6,
                (int)(75 + 75 * rfrac()),
                0, ANGLE_RESOLUTION - 1,
                20.0, 70.0,
                10.0, 100.0);

    if (Team_play(world))
    {
        for (j = 0; j < NumPlayers; j++)
        {
            player_t *pl_j = Player_by_index(j);

            if (Player_is_tank(pl_j) ||
                (Player_is_paused(pl_j) && pl_j->pause_count <= 0) ||
                Player_is_waiting(pl_j))
                continue;
            if (pl_j->team == targ->team)
            {
                lose_score += Get_Score(pl_j);
                lose_team_members++;
                // if (BIT(pl_j->obj_status, GAME_OVER) == 0)
                if (!Player_is_dead(pl_j))
                    somebody = true;
            }
            else if (pl_j->team == kp->team)
            {
                win_score += Get_Score(pl_j);
                win_team_members++;
            }
        }
    }
    if (somebody)
    {
        for (j = 0; j < Num_targets(); j++)
        {
            target_t *t = Target_by_index(j);

            if (t->team == targ->team)
            {
                targets_total++;
                if (t->dead_ticks == 0)
                    targets_remaining++;
            }
        }
    }
    if (!somebody)
        return;

    Handle_Scoring(SCORE_TARGET, kp, nullptr, targ, nullptr);

    sound_play_sensors(targ->pos, DESTROY_TARGET_SOUND);

    if (targets_remaining > 0)
    {
        sc = Rate(kp->score, CANNON_SCORE) / 4;
        sc = sc * (targets_total - targets_remaining) / (targets_total + 1);
        if (sc > 0)
        {
            Score(kp, sc, targ->pos, "Target: ");
        }
        /*
         * If players can't collide with their own targets, we
         * assume there are many used as shields.  Don't litter
         * the game with the message below.
         */
        if (options.targetTeamCollision && targets_total < 10)
            Set_message_f("%s blew up one of team %d's targets.",
                          kp->name, targ->team);
        return;
    }

    Set_message_f("%s blew up team %d's %starget.",
                  kp->name, targ->team, (targets_total > 1) ? "last " : "");

    if (options.targetKillTeam)
    {
        kp->kills++;
    }

    sc = Rate(win_score, lose_score);
    por = (sc * lose_team_members) / win_team_members;

    for (j = 0; j < NumPlayers; j++)
    {
        player_t *pl_j = Player_by_index(j);

        if (Player_is_tank(pl_j) ||
            (Player_is_paused(pl_j) && pl_j->pause_count <= 0) ||
            Player_is_waiting(pl_j))
            continue;

        if (pl_j->team == targ->team)
        {
            if (options.targetKillTeam &&
                targets_remaining == 0 &&
                !BIT(pl_j->obj_status, LEGACY_KILLED | LEGACY_PAUSE | LEGACY_GAME_OVER))
                Player_set_state(pl_j, PL_STATE_KILLED);
            Score(pl_j, -sc, targ->pos, "Target: ");
        }
        else if (pl_j->team == kp->team &&
                 (pl_j->team != TEAM_NOT_SET || pl_j->id == kp->id))
            Score(pl_j, por, targ->pos, "Target: ");
    }
}

hitmask_t Target_hitmask(target_t *targ)
{
    /* target is destroyed - nothing hits */
    if (targ->dead_ticks > 0)
        return ALL_BITS;

    /* everything hits targets that don't belong to a team */
    if (targ->team == TEAM_NOT_SET)
        return 0;

    /* if options.targetTeamCollision is true, everything hits a target */
    if (options.targetTeamCollision)
        return 0;

    /* note that targets are always team immune */
    return HITMASK(targ->team);
}

void Target_set_hitmask(int group, target_t *targ)
{
    assert(targ->group == group);
    P_set_hitmask(targ->group, Target_hitmask(targ));
}

void Target_init(void)
{
    int group;

    for (group = 0; group < num_groups; group++)
    {
        group_t *gp = groupptr_by_id(group);

        if (gp->type == TARGET)
            Target_set_hitmask(group, Target_by_index(gp->mapobj_ind));
    }
#if 0
    P_grouphack(TARGET, Target_set_hitmask);
#endif
}

void World_restore_target(world_t *world, target_t *targ)
{
    blkpos_t blk = Clpos_to_blkpos(targ->pos);

    World_set_block(world, blk, TARGET);

    targ->conn_mask = 0;
    targ->update_mask = ~0;
    targ->last_change = frame_loops;
    targ->dead_ticks = 0;
}

void World_remove_target(world_t *world, target_t *targ)
{
}

// Returns pointer to target at block with coordinates (x, y).
// This assumes the map is block based.
target_t *targetXY(int x, int y)
{
    for (int i = 0; i < Num_targets(); i++)
    {
        target_t *target = Target_by_index(i);
        blkpos_t blk = Clpos_to_blkpos(target->pos);
        if (blk.bx == x && blk.by == y)
            return target;
    }
    return nullptr;
}
