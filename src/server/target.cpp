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

#include "const.h"
#include "map.h"
#include "option.h"
#include "server.h"

/*
 * Update targets
 */
void Target_update(void)
{
    for (int i = 0; i < Num_targets(); i++)
    {
        if (world->targets[i].dead_ticks > 0)
        {
            if (!--world->targets[i].dead_ticks)
            {
                world->block[world->targets[i].blk_pos.bx][world->targets[i].blk_pos.by] = TARGET;
                world->targets[i].conn_mask = 0;
                world->targets[i].update_mask = (unsigned)-1;
                world->targets[i].last_change = frame_loops;

                if (options.targetSync)
                {
                    uint16_t team = world->targets[i].team;

                    for (int j = 0; j < Num_targets(); j++)
                    {
                        if (world->targets[j].team == team)
                        {
                            world->block[world->targets[j].blk_pos.bx]
                                        [world->targets[j].blk_pos.by] = TARGET;
                            world->targets[j].conn_mask = 0;
                            world->targets[j].update_mask = (unsigned)-1;
                            world->targets[j].last_change = frame_loops;
                            world->targets[j].dead_ticks = 0;
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
}

void Object_hits_target2(object_t *obj, target_t *targ, double player_cost)
{
}

void World_restore_target(target_t *targ)
{
}

void World_remove_target(target_t *targ)
{
}