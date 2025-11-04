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
#include <cmath>
#include <cstdio>
#include <cerrno>

#include "commonmacros.h"
#include "const.h"

#include "server.h"

#define SERVER
#include "xpconfig.h"
#include "serverconst.h"
#include "global.h"
#include "map.h"
#include "score.h"
#include "bit.h"
#include "netserver.h"
#include "saudio.h"
#include "xperror.h"
#include "object.h"
#include "xpmath.h"
#include "robot.h"

bool updateScores = true;

int playerArrayNumber;
player_t **PlayersArray;
#define MAX_SPECTATORS 0
int GetIndArray[NUM_IDS + MAX_SPECTATORS + 1];

/*
 * Get index in Players array for player with id 'id'.
 */
int GetInd(int id)
{
    if (id == NO_ID)
        return NO_IND;

    /*
     * kps - in some places where we look at the id we don't
     * bother about spectators.
     * This should be cleaned up in general.
     */
    if (id < 0 || id >= NELEM(GetIndArray))
    {
        /*warn("GetInd: id = %d, array size = %d\n",
          id, NUM_IDS + MAX_SPECTATORS + 1);*/
        return NO_IND;
    }
    return GetIndArray[id];
}

/********* **********
 * Functions on player array.
 */

void Pick_startpos(player_t *pl)
{
    // player_t *pl = PlayersArray[ind];
    int ind = GetInd(pl->id);
    int i, num_free;
    int pick = 0, seen = 0;
    static int prev_num_bases = 0;
    static char *free_bases = NULL;

    if (Player_is_tank(pl))
    {
        pl->home_base = 0;
        return;
    }

    if (prev_num_bases != Num_bases())
    {
        prev_num_bases = Num_bases();
        XFREE(free_bases);
        free_bases = (char *)malloc(Num_bases() * sizeof(*free_bases));
        if (free_bases == NULL)
        {
            error("Can't allocate memory for free_bases");
            End_game();
        }
    }

    num_free = 0;
    for (i = 0; i < Num_bases(); i++)
    {
        if (world->bases[i].team == pl->team)
        {
            num_free++;
            free_bases[i] = 1;
        }
        else
        {
            free_bases[i] = 0; /* other team */
        }
    }

    for (i = 0; i < NumPlayers; i++)
    {
        player_t *pl_i = Player_by_index(i);

        if (pl_i->id != pl->id && !Player_is_tank(pl_i) && free_bases[pl_i->home_base])
        {
            free_bases[pl_i->home_base] = 0; /* occupado */
            num_free--;
        }
    }

    if (BIT(world->rules->mode, TIMING))
    { /* pick first free base */
        for (i = 0; i < Num_bases(); i++)
        {
            if (free_bases[world->baseorder[i].base_idx])
                break;
        }
    }
    else
    {
        pick = (int)(rfrac() * num_free);
        seen = 0;
        for (i = 0; i < Num_bases(); i++)
        {
            if (free_bases[i] != 0)
            {
                if (seen < pick)
                    seen++;
                else
                    break;
            }
        }
    }

    if (i == Num_bases())
    {
        error("Can't pick startpos (ind=%d,num=%d,free=%d,pick=%d,seen=%d)",
              ind, Num_bases(), num_free, pick, seen);
        End_game();
    }
    else
    {
        pl->home_base = BIT(world->rules->mode, TIMING) ? world->baseorder[i].base_idx : i;
        if (ind < NumPlayers)
        {
            for (i = 0; i < NumPlayers; i++)
            {
                player_t *pl_i = PlayersArray[i];

                if (pl_i->conn != NULL)
                {
                    Send_base(pl_i->conn,
                              pl->id,
                              pl->home_base);
                }
            }
            if (BIT(pl->obj_status, PLAYING) == 0)
                pl->count = RECOVERY_DELAY;
            else if (BIT(pl->obj_status, PAUSE | GAME_OVER))
                Go_home(pl);
        }
    }
}

void Go_home(player_t *pl)
{
    // player_t *pl = PlayersArray[ind];
    int ind = GetInd(pl->id);

    printf("Go_home: ind = %d, pl->ind = %d\n", ind, pl->ind);
    if (ind != pl->ind)
    {
        player_t *pl1 = PlayersArray[ind];
        player_t *pl2 = PlayersArray[pl->ind];
        warn("******** ERROR!!!!");
        warn("pl1: '%s', pl2: '%s'", pl1->name, pl2->name);
    }

    int i, x, y, dir, check;
    double vx, vy, velo;

    if (Player_is_tank(pl))
    {
        /*NOTREACHED*/
        /* Tanks have no homebase. */
        error("BUG: gohome tank");
        return;
    }

    if (BIT(world->rules->mode, TIMING) && pl->round && !BIT(pl->obj_status, GAME_OVER))
    {
        if (pl->check)
            check = pl->check - 1;
        else
            check = world->NumChecks - 1;
        x = world->checks[check].x;
        y = world->checks[check].y;
        vx = (rfrac() - 0.5) * 0.1;
        vy = (rfrac() - 0.5) * 0.1;
        velo = LENGTH(vx, vy);
        dir = pl->last_check_dir;
        dir = MOD2(dir + (int)((rfrac() - 0.5) * (RES / 8)), RES);
    }
    else
    {
        x = world->bases[pl->home_base].blk_pos.x;
        y = world->bases[pl->home_base].blk_pos.y;
        dir = world->bases[pl->home_base].dir;
        vx = vy = velo = 0;
    }

    pl->dir = dir;
    pl->float_dir = dir;
    clpos_t pos;
    pos.cx = FLOAT_TO_CLICK((x + 0.5) * BLOCK_SZ + vx);
    pos.cy = FLOAT_TO_CLICK((y + 0.5) * BLOCK_SZ + vy);
    Player_position_init_clpos(pl, pos);
    pl->vel.x = vx;
    pl->vel.y = vy;
    pl->velocity = velo;
    pl->acc.x = pl->acc.y = 0.0;
    pl->turnacc = pl->turnvel = 0.0;
    memset(pl->last_keyv, 0, sizeof(pl->last_keyv));
    memset(pl->prev_keyv, 0, sizeof(pl->prev_keyv));
    Player_used_kill(pl);

    if (options.playerStartsShielded != 0)
    {
        SET_BIT(pl->used, HAS_SHIELD);
        if (options.playerShielding == 0)
        {
            pl->shield_time = 2 * FPS;
            SET_BIT(pl->have, HAS_SHIELD);
        }
        if (BIT(pl->have, HAS_DEFLECTOR))
            Deflector(pl, true);
    }
    Thrust(pl, false);
    pl->updateVisibility = 1;
    for (i = 0; i < NumPlayers; i++)
    {
        player_t *pl_i = PlayersArray[i];

        pl->visibility[i].lastChange = 0;
        pl_i->visibility[ind].lastChange = 0;
    }

    if (Player_is_robot(pl))
        Robot_go_home(pl);

    warn("******** RETURN FROM GO HOME!!!!");
}

/*
 * Compute the current sensor range for player 'pl'.  This is based on the
 * amount of fuel, the number of sensor items (each one adds 25%), and the
 * minimum and maximum visibility limits in effect.
 */
void Compute_sensor_range(player_t *pl)
{
    static int init = 0;
    static double EnergyRangeFactor;

    if (!init)
    {
        if (options.minVisibilityDistance <= 0.0)
            options.minVisibilityDistance = VISIBILITY_DISTANCE;
        else
            options.minVisibilityDistance *= BLOCK_SZ;
        if (options.maxVisibilityDistance <= 0.0)
            options.maxVisibilityDistance = world->hypotenuse;
        else
            options.maxVisibilityDistance *= BLOCK_SZ;

        if (world->items[ITEM_FUEL].initial > 0.0)
        {
            EnergyRangeFactor = options.minVisibilityDistance /
                                (world->items[ITEM_FUEL].initial * (1.0 + ((double)world->items[ITEM_SENSOR].initial * 0.25)));
            EnergyRangeFactor /= FUEL_SCALE_FACT;
        }
        else
        {
            EnergyRangeFactor = ENERGY_RANGE_FACTOR;
        }
        init = 1;
    }

    pl->sensor_range = pl->fuel.sum * EnergyRangeFactor;
    pl->sensor_range *= (1.0 + ((double)pl->item[ITEM_SENSOR] * 0.25));
    if (pl->sensor_range < options.minVisibilityDistance)
        pl->sensor_range = options.minVisibilityDistance;
    if (pl->sensor_range > options.maxVisibilityDistance)
        pl->sensor_range = options.maxVisibilityDistance;
}

/*
 * Give ship one more tank, if possible.
 */
void Player_add_tank(player_t *pl, long tank_fuel)
{
    long tank_cap, add_fuel;

    if (pl->fuel.num_tanks < MAX_TANKS)
    {
        pl->fuel.num_tanks++;
        tank_cap = TANK_CAP(pl->fuel.num_tanks);
        add_fuel = tank_fuel;
        LIMIT(add_fuel, 0, tank_cap);
        pl->fuel.sum += add_fuel;
        pl->fuel.max += tank_cap;
        pl->fuel.tank[pl->fuel.num_tanks] = add_fuel;
        pl->emptymass += TANK_MASS;
        pl->item[ITEM_TANK] = pl->fuel.num_tanks;
    }
}

/*
 * Remove a tank from a ship, if possible.
 */
void Player_remove_tank(player_t *pl, int which_tank)
{
    // player_t *pl = PlayersArray[ind];
    int i, tank_ind;
    long tank_fuel, tank_cap;

    if (pl->fuel.num_tanks > 0)
    {
        tank_ind = which_tank;
        LIMIT(tank_ind, 1, pl->fuel.num_tanks);
        pl->emptymass -= TANK_MASS;
        tank_fuel = pl->fuel.tank[tank_ind];
        tank_cap = TANK_CAP(tank_ind);
        pl->fuel.max -= tank_cap;
        pl->fuel.sum -= tank_fuel;
        pl->fuel.num_tanks--;
        if (pl->fuel.current > pl->fuel.num_tanks)
            pl->fuel.current = 0;
        else
        {
            for (i = tank_ind; i <= pl->fuel.num_tanks; i++)
                pl->fuel.tank[i] = pl->fuel.tank[i + 1];
        }
        pl->item[ITEM_TANK] = pl->fuel.num_tanks;
    }
}

void Player_hit_armor(player_t *pl)
{
    if (--pl->item[ITEM_ARMOR] <= 0)
        CLR_BIT(pl->have, HAS_ARMOR);
}

void Player_used_kill(player_t *pl)
{
    pl->used &= ~USED_KILL;
    if (!BIT(DEF_HAVE, HAS_SHIELD))
        CLR_BIT(pl->have, HAS_SHIELD);
}

/*
 * Calculate the mass of a player.
 */
void Player_set_mass(player_t *pl)
{
    //  BUGFIX: xpilot 4.5.5beta has option minItemMass,
    //  making the ship 3 units too heavy on blood's music.
    //  Fixed by removing minItemMass option.
    pl->mass = pl->emptymass + FUEL_MASS(pl->fuel.sum) + pl->item[ITEM_ARMOR] * ARMOR_MASS;
    // printf("Player %d mass is %f\n", ind, pl->mass);
}

/*
 * Give player the initial number of tanks and amount of fuel.
 * Upto the maximum allowed.
 */
static void Player_init_fuel(player_t *pl, long total_fuel)
{
    // player_t *pl = PlayersArray[ind];
    long fuel = total_fuel;
    int i;

    pl->fuel.num_tanks = 0;
    pl->fuel.current = 0;
    pl->fuel.max = TANK_CAP(0);
    pl->fuel.sum = MIN(fuel, pl->fuel.max);
    pl->fuel.tank[0] = pl->fuel.sum;
    pl->emptymass = options.shipMass;
    pl->item[ITEM_TANK] = pl->fuel.num_tanks;

    fuel -= pl->fuel.sum;

    for (i = 1; i <= world->items[ITEM_TANK].initial; i++)
    {
        Player_add_tank(pl, fuel);
        fuel -= pl->fuel.tank[i];
    }
}

int Init_player(int ind, shipshape_t *ship)
{
    player_t *pl = PlayersArray[ind];
    bool too_late = false;
    int i;

    pl->vel.x = pl->vel.y = 0.0;
    pl->acc.x = pl->acc.y = 0.0;
    pl->float_dir = pl->dir = DIR_UP;
    pl->turnvel = 0.0;
    pl->oldturnvel = 0.0;
    pl->turnacc = 0.0;
    pl->mass = options.shipMass;
    pl->emptymass = options.shipMass;

    for (i = 0; i < NUM_ITEMS; i++)
    {
        if (!BIT(1U << i, ITEM_BIT_FUEL | ITEM_BIT_TANK))
            pl->item[i] = world->items[i].initial;
    }

    pl->fuel.sum = world->items[ITEM_FUEL].initial << FUEL_SCALE_BITS;
    Player_init_fuel(pl, pl->fuel.sum);

    if (options.allowShipShapes == true && ship)
        pl->ship = ship;
    else
    {
        /*
                pl->ship = Default_ship();
        */
        shipshape_t *tryship = Parse_shape_str(options.defaultShipShape);

        if (tryship)
            pl->ship = tryship;
        else
            pl->ship = Default_ship();
    }

    pl->power = 45.0;
    pl->turnspeed = 30.0;
    pl->turnresistance = 0.12;
    pl->power_s = 35.0;
    pl->turnspeed_s = 25.0;
    pl->turnresistance_s = 0.12;

    pl->check = 0;
    pl->round = 0;
    pl->time = 0;
    pl->last_lap_time = 0;
    pl->last_lap = 0;
    pl->best_lap = 0;
    pl->count = -1;
    pl->shield_time = 0;
    pl->last_wall_touch = 0;

    pl->type = OBJ_PLAYER;
    pl->type_ext = 0; /* assume human player */
    pl->shots = 0;
    pl->missile_rack = 0;
    pl->forceVisible = 0;
    Compute_sensor_range(pl);
    pl->shot_max = options.maxPlayerShots;
    pl->shot_time = 0;
    pl->color = WHITE;
    pl->score = 0;
    pl->prev_score = 0;
    pl->prev_check = 0;
    pl->prev_round = 0;
    pl->fs = 0;
    pl->repair_target = 0;
    pl->name[0] = '\0';
    pl->num_pulses = 0;
    pl->emergency_thrust_left = 0;
    pl->emergency_thrust_max = 0;
    pl->emergency_shield_left = 0;
    pl->emergency_shield_max = 0;
    pl->phasing_left = 0;
    pl->phasing_max = 0;
    pl->ecmcount = 0;
    pl->damaged = 0;
    pl->stunned = 0;

    pl->obj_status = PLAYING | GRAVITY | DEF_BITS;
    pl->have = DEF_HAVE;
    pl->used = DEF_USED;

    if (pl->item[ITEM_CLOAK] > 0)
        SET_BIT(pl->have, HAS_CLOAKING_DEVICE);

    CLEAR_MODS(pl->mods);
    for (i = 0; i < NUM_MODBANKS; i++)
        CLEAR_MODS(pl->modbank[i]);
    for (i = 0; i < LOCKBANK_MAX; i++)
        pl->lockbank[i] = NOT_CONNECTED;

    {
        static uint16_t pseudo_team_no = 0;
        pl->pseudo_team = pseudo_team_no++;
    }
    pl->mychar = ' ';
    pl->prev_mychar = pl->mychar;
    pl->life = world->rules->lives;
    pl->prev_life = pl->life;
    pl->ball = NULL;

    pl->player_fps = FPS;

    pl->kills = 0;
    pl->deaths = 0;

    /*
     * If limited lives and if nobody has lost a life yet, you may enter
     * now, otherwise you will have to wait 'til everyone gets GAME OVER.
     */
    if (BIT(world->rules->mode, LIMITED_LIVES))
    {
        for (i = 0; i < NumPlayers; i++)
        {
            player_t *pl_i = PlayersArray[i];
            /* If a non-team member has lost a life,
             * then it's too late to join. */
            if (pl_i->life < world->rules->lives && !Players_are_teammates(pl, pl_i))
            {
                too_late = true;
                break;
            }
        }
        if (too_late)
        {
            pl->mychar = 'W';
            pl->prev_life = pl->life = 0;
            SET_BIT(pl->obj_status, GAME_OVER);
        }
    }

    pl->team = TEAM_NOT_SET;

    pl->alliance = ALLIANCE_NOT_SET;
    pl->prev_alliance = ALLIANCE_NOT_SET;
    pl->invite = NO_ID;

    pl->lock.tagged = LOCK_NONE;
    pl->lock.pl_id = 0;

    pl->robot_data_ptr = NULL;

    pl->wormDrawCount = 0;

    pl->id = peek_ID();
    GetIndArray[pl->id] = ind;
    pl->ind = ind;

    pl->conn = NULL;
    pl->audio = NULL;

    pl->lose_item = 0;
    pl->lose_item_state = 0;

    pl->shove_next = 0;
    for (i = 0; i < MAX_RECORDED_SHOVES; i++)
    {
        pl->shove_record[i].pusher_id = NO_ID;
    }

    pl->frame_last_busy = frame_loops;

    pl->isowner = 0;
    pl->isoperator = 0;

    return pl->id;
}

static player_t *playerArray;
static visibility_t *visibilityArray;

void Alloc_players(int number)
{
    player_t *p;
    visibility_t *t;
    size_t n = number;
    int i;

    /* Allocate space for pointers */
    PlayersArray = (player_t **)calloc(number + 1, sizeof(player_t *));

    /* Allocate space for all entries, all player structs */
    p = playerArray = XCALLOC(player_t, n);

    /* Allocate space for all visibility arrays, n arrays of n entries */
    t = visibilityArray = XCALLOC(visibility_t, n * n);

    if (!PlayersArray || !playerArray || !visibilityArray)
    {
        error("Not enough memory for Players.");
        exit(1);
    }

    /* PlayersArray[-1] should evaluate to NULL. */
    PlayersArray++;

    for (i = 0; i < number; i++)
    {
        PlayersArray[i] = p++;
        PlayersArray[i]->visibility = t;
        /* Advance to next block/array */
        t += number;
    }

    playerArrayNumber = number;

    /* Initialize player id to index lookup table */
    for (i = 0; i < NELEM(GetIndArray); i++)
        GetIndArray[i] = NO_IND;
}

void Free_players(void)
{
    if (PlayersArray)
    {
        --PlayersArray;
        free(PlayersArray);
        PlayersArray = NULL;

        free(playerArray);
        free(visibilityArray);
    }
}

void Update_score_table(void)
{
    int i, j, check;
    player_t *pl;

    for (j = 0; j < NumPlayers; j++)
    {
        pl = PlayersArray[j];
        if (pl->score != pl->prev_score || pl->life != pl->prev_life || pl->mychar != pl->prev_mychar || pl->alliance != pl->prev_alliance)
        {
            pl->prev_score = pl->score;
            pl->prev_life = pl->life;
            pl->prev_mychar = pl->mychar;
            pl->prev_alliance = pl->alliance;
            for (i = 0; i < NumPlayers; i++)
            {
                player_t *pl_i = PlayersArray[i];

                if (pl_i->conn != NULL)
                {
                    Send_score(pl_i->conn, pl->id,
                               pl->score, pl->life,
                               pl->mychar, pl->alliance);
                }
            }
        }
        if (BIT(world->rules->mode, TIMING))
        {
            if (pl->check != pl->prev_check || pl->round != pl->prev_round)
            {
                pl->prev_check = pl->check;
                pl->prev_round = pl->round;
                check = (pl->round == 0)
                            ? 0
                        : (pl->check == 0)
                            ? (world->NumChecks - 1)
                            : (pl->check - 1);
                for (i = 0; i < NumPlayers; i++)
                {
                    player_t *pl_i = PlayersArray[i];
                    if (pl_i->conn != NULL)
                    {
                        Send_timing(pl_i->conn, pl->id, check, pl->round);
                    }
                }
            }
        }
    }
    updateScores = false;
}

void Reset_all_players(void)
{
    player_t *pl;
    int i, j;
    char msg[MSG_LEN];

    updateScores = true;

    for (i = 0; i < NumPlayers; i++)
    {
        pl = Player_by_index(i);
        if (options.endOfRoundReset)
        {
            if (BIT(pl->obj_status, PAUSE))
            {
                Player_death_reset(pl);
            }
            else
            {
                Kill_player(pl);
                if (pl != Player_by_index(i))
                {
                    /* player was deleted. */
                    i--;
                    continue;
                }
            }
        }
        CLR_BIT(pl->obj_status, GAME_OVER);
        CLR_BIT(pl->have, HAS_BALL);
        pl->kills = 0;
        pl->deaths = 0;
        pl->round = 0;
        pl->check = 0;
        pl->time = 0;
        pl->best_lap = 0;
        pl->last_lap = 0;
        pl->last_lap_time = 0;
        if (!BIT(pl->obj_status, PAUSE))
        {
            pl->mychar = ' ';
            pl->frame_last_busy = frame_loops;
            pl->life = world->rules->lives;
            if (BIT(world->rules->mode, TIMING))
            {
                pl->count = RECOVERY_DELAY;
            }
        }
        if (Player_is_tank(pl))
            pl->mychar = 'T';
        else if (Player_is_robot(pl))
            pl->mychar = 'R';
    }
    if (BIT(world->rules->mode, TEAM_PLAY))
    {

        /* Detach any balls and kill ball */
        /* We are starting all over again */
        for (j = NumObjs - 1; j >= 0; j--)
        {
            if (BIT(Obj[j]->type, OBJ_BALL))
            {
                ballobject_t *ball = BALL_IND(j);
                ball->id = NO_ID;
                ball->life = 0;
                ball->ball_owner = 0; /* why not -1 ??? */
                CLR_BIT(ball->obj_status, RECREATE);
                Delete_shot(j);
            }
        }

        /* Reset the treasures */
        for (i = 0; i < world->NumTreasures; i++)
        {
            world->treasures[i].destroyed = 0;
            world->treasures[i].have = false;
            Make_treasure_ball(i);
        }

        /* Reset the teams */
        for (i = 0; i < MAX_TEAMS; i++)
        {
            world->teams[i].TreasuresDestroyed = 0;
            world->teams[i].TreasuresLeft = world->teams[i].NumTreasures - world->teams[i].NumEmptyTreasures;
        }

        if (options.endOfRoundReset)
        {
            /* Reset the targets */
            for (i = 0; i < world->NumTargets; i++)
            {
                if (world->targets[i].damage != TARGET_DAMAGE || world->targets[i].dead_time != 0)
                {
                    world->block[world->targets[i].blk_pos.x][world->targets[i].blk_pos.y] = TARGET;
                    world->targets[i].dead_time = 0;
                    world->targets[i].damage = TARGET_DAMAGE;
                    world->targets[i].conn_mask = 0;
                    world->targets[i].update_mask = (unsigned)-1;
                    world->targets[i].last_change = frame_loops;
                }
            }
        }
    }

    if (options.endOfRoundReset)
    {
        for (i = 0; i < NumObjs; i++)
        {
            object_t *obj = Obj[i];
            if (BIT(obj->type, OBJ_SHOT | OBJ_MINE | OBJ_DEBRIS | OBJ_SPARK | OBJ_CANNON_SHOT | OBJ_TORPEDO | OBJ_SMART_SHOT | OBJ_HEAT_SHOT | OBJ_ITEM))
            {
                obj->life = 0;
                if (BIT(obj->type, OBJ_TORPEDO | OBJ_SMART_SHOT | OBJ_HEAT_SHOT | OBJ_CANNON_SHOT | OBJ_MINE))
                {
                    /* Take care that no new explosions are made. */
                    obj->mass = 0;
                }
            }
        }
    }

    roundtime = options.maxRoundTime * FPS;

    Update_score_table();
}

void Check_team_members(int team)
{
    player_t *pl;
    int members, i;

    if (!BIT(world->rules->mode, TEAM_PLAY))
        return;

    for (members = i = 0; i < NumPlayers; i++)
    {
        pl = Player_by_index(i);
        if (pl->team != TEAM_NOT_SET && !Player_is_tank(pl) && pl->team == team)
            members++;
    }
    if (world->teams[team].NumMembers != members)
    {
        error("Server has reset team %d members from %d to %d",
              team, world->teams[team].NumMembers, members);
        for (i = 0; i < NumPlayers; i++)
        {
            pl = Player_by_index(i);
            if (pl->team != TEAM_NOT_SET && !Player_is_tank(pl) && pl->team == team)
                error("Team %d currently has player %d: \"%s\"",
                      team, i + 1, pl->name);
        }
        world->teams[team].NumMembers = members;
    }
}

static void Compute_end_of_round_values(double *average_score,
                                        int *num_best_players,
                                        double *best_ratio,
                                        int best_players[])
{
    int i;
    double ratio;

    /* Initialize everything */
    *average_score = 0;
    *num_best_players = 0;
    *best_ratio = -1.0;

    /* Figure out what the average score is and who has the best kill/death */
    /* ratio for this round */
    for (i = 0; i < NumPlayers; i++)
    {
        if (Player_is_tank(Player_by_index(i)) || (BIT(Player_by_index(i)->obj_status, PAUSE) && Player_by_index(i)->count <= 0))
            continue;
        *average_score += Player_by_index(i)->score;
        ratio = (double)Player_by_index(i)->kills / (Player_by_index(i)->deaths + 1);
        if (ratio > *best_ratio)
        {
            *best_ratio = ratio;
            best_players[0] = i;
            *num_best_players = 1;
        }
        else if (ratio == *best_ratio)
            best_players[(*num_best_players)++] = i;
    }
    *average_score /= NumPlayers;
}

static void Give_best_player_bonus(double average_score,
                                   int num_best_players,
                                   double best_ratio,
                                   int best_players[])
{
    int i;
    double points;
    char msg[MSG_LEN];

    if (best_ratio == 0)
    {
        sprintf(msg, "There is no Deadly Player");
    }
    else if (num_best_players == 1)
    {
        player_t *bp = PlayersArray[best_players[0]];

        sprintf(msg,
                "%s is the Deadliest Player with a kill ratio of %d/%d.",
                bp->name,
                bp->kills, bp->deaths);
        points = best_ratio * Rate(bp->score, average_score);
        SCORE(bp, points, bp->pos, "[Deadliest]");
    }
    else
    {
        msg[0] = '\0';
        for (i = 0; i < num_best_players; i++)
        {
            player_t *bp = PlayersArray[best_players[i]];
            double ratio = Rate(bp->score, average_score);
            double score = (ratio + num_best_players) / num_best_players;

            if (msg[0])
            {
                if (i == num_best_players - 1)
                    strcat(msg, " and ");
                else
                    strcat(msg, ", ");
            }
            if (strlen(msg) + 8 + strlen(bp->name) >= sizeof(msg))
            {
                Set_message(msg);
                msg[0] = '\0';
            }
            strcat(msg, bp->name);
            points = (int)(best_ratio * score);
            SCORE(bp, points, bp->pos, "[Deadly]");
        }
        if (strlen(msg) + 64 >= sizeof(msg))
        {
            Set_message(msg);
            msg[0] = '\0';
        }
        sprintf(msg + strlen(msg),
                " are the Deadly Players with kill ratios of %d/%d.",
                PlayersArray[best_players[0]]->kills,
                PlayersArray[best_players[0]]->deaths);
    }
    Set_message(msg);
}

static void Give_individual_bonus(int ind, double average_score)
{
    double ratio;
    double points;
    player_t *pl = PlayersArray[ind];

    ratio = (double)pl->kills / (pl->deaths + 1);
    points = ratio * Rate(pl->score, average_score);
    SCORE(pl, points, pl->pos, "[Winner]");
}

static void Count_rounds(void)
{
    char msg[MSG_LEN];

    if (!options.roundsToPlay)
    {
        return;
    }

    ++roundsPlayed;

    sprintf(msg, " < Round %d out of %d completed. >",
            roundsPlayed, options.roundsToPlay);
    Set_message(msg);
    if (roundsPlayed >= options.roundsToPlay)
    {
        Game_Over();
    }
}

void Team_game_over(int winning_team, const char *reason)
{
    int i, j;
    double average_score;
    int num_best_players;
    int *best_players;
    double best_ratio;
    char msg[MSG_LEN];

    if (!(best_players = (int *)malloc(NumPlayers * sizeof(int))))
    {
        error("no mem");
        End_game();
    }

    /* Figure out the average score and who has the best kill/death ratio */
    /* ratio for this round */
    Compute_end_of_round_values(&average_score,
                                &num_best_players,
                                &best_ratio,
                                best_players);

    /* Print out the results of the round */
    if (winning_team != -1)
    {
        sprintf(msg, " < Team %d has won the game%s! >", winning_team,
                reason);
        sound_play_all(TEAM_WIN_SOUND);
    }
    else
    {
        sprintf(msg, " < We have a draw%s! >", reason);
        sound_play_all(TEAM_DRAW_SOUND);
    }
    Set_message(msg);

    /* Give bonus to the best player */
    Give_best_player_bonus(average_score,
                           num_best_players,
                           best_ratio,
                           best_players);

    /* Give bonuses to the winning team */
    if (winning_team != -1)
    {
        for (i = 0; i < NumPlayers; i++)
        {
            if (Player_by_index(i)->team != winning_team)
                continue;
            if (Player_is_tank(Player_by_index(i)) ||
                (BIT(Player_by_index(i)->obj_status, PAUSE) && Player_by_index(i)->count <= 0) ||
                (BIT(Player_by_index(i)->obj_status, GAME_OVER) && Player_by_index(i)->mychar == 'W' && Player_by_index(i)->score == 0))
                continue;
            for (j = 0; j < num_best_players; j++)
            {
                if (i == best_players[j])
                    break;
            }
            if (j == num_best_players)
                Give_individual_bonus(i, average_score);
        }
    }

    Reset_all_players();

    Count_rounds();

    free(best_players);
}

void Individual_game_over(int winner)
{
    int i, j;
    double average_score;
    int num_best_players;
    int *best_players;
    double best_ratio;
    char msg[MSG_LEN];

    if (!(best_players = (int *)malloc(NumPlayers * sizeof(int))))
    {
        error("no mem");
        End_game();
    }

    /* Figure out what the average score is and who has the best kill/death */
    /* ratio for this round */
    Compute_end_of_round_values(&average_score, &num_best_players,
                                &best_ratio, best_players);

    /* Print out the results of the round */
    if (winner == -1)
    {
        Set_message(" < We have a draw! >");
        sound_play_all(PLAYER_DRAW_SOUND);
    }
    else if (winner == -2)
    {
        Set_message(" < The robots have won the game! >");
        /* Perhaps this should be a different sound? */
        sound_play_all(PLAYER_WIN_SOUND);
    }
    else
    {
        sprintf(msg, " < %s has won the game! >", PlayersArray[winner]->name);
        Set_message(msg);
        sound_play_all(PLAYER_WIN_SOUND);
    }

    /* Give bonus to the best player */
    Give_best_player_bonus(average_score,
                           num_best_players,
                           best_ratio,
                           best_players);

    /* Give bonus to the winning player */
    if (winner >= 0)
    {
        for (i = 0; i < num_best_players; i++)
        {
            if (winner == best_players[i])
            {
                break;
            }
        }
        if (i == num_best_players)
        {
            Give_individual_bonus(winner, average_score);
        }
    }
    else if (winner == -2)
    {
        for (j = 0; j < NumPlayers; j++)
        {
            player_t *pl_j = PlayersArray[j];

            if (Player_is_robot(pl_j))
            {
                for (i = 0; i < num_best_players; i++)
                {
                    if (j == best_players[i])
                    {
                        break;
                    }
                }
                if (i == num_best_players)
                {
                    Give_individual_bonus(j, average_score);
                }
            }
        }
    }

    Reset_all_players();

    free(best_players);
}

void Race_game_over(void)
{
    player_t *pl;
    int i,
        j,
        k,
        bestlap = 0,
        num_best_players = 0,
        num_active_players = 0,
        num_ordered_players = 0;
    int *order;
    char msg[MSG_LEN];

    /*
     * Reassign players's starting posisitions based upon
     * personal best lap times.
     */
    if ((order = (int *)malloc(NumPlayers * sizeof(int))) != NULL)
    {
        for (i = 0; i < NumPlayers; i++)
        {
            pl = Player_by_index(i);
            if (Player_is_tank(pl))
            {
                continue;
            }
            if (BIT(pl->obj_status, PAUSE) || (BIT(pl->obj_status, GAME_OVER) && pl->mychar == 'W') || pl->best_lap <= 0)
            {
                j = i;
            }
            else
            {
                for (j = 0; j < i; j++)
                {
                    if (pl->best_lap < PlayersArray[order[j]]->best_lap)
                    {
                        break;
                    }
                    if (BIT(PlayersArray[order[j]]->obj_status, PAUSE) || (BIT(PlayersArray[order[j]]->obj_status, GAME_OVER) && PlayersArray[order[j]]->mychar == 'W'))
                    {
                        break;
                    }
                }
            }
            for (k = i - 1; k >= j; k--)
            {
                order[k + 1] = order[k];
            }
            order[j] = i;
            num_ordered_players++;
        }
        for (i = 0; i < num_ordered_players; i++)
        {
            pl = PlayersArray[order[i]];
            if (pl->home_base != world->baseorder[i].base_idx)
            {
                pl->home_base = world->baseorder[i].base_idx;
                for (j = 0; j < NumPlayers; j++)
                {
                    if (PlayersArray[j]->conn != NULL)
                    {
                        Send_base(PlayersArray[j]->conn,
                                  pl->id,
                                  pl->home_base);
                    }
                }
                if (BIT(pl->obj_status, PAUSE))
                {
                    Go_home(pl);
                }
            }
        }
        free(order);
    }

    for (i = NumPlayers - 1; i >= 0; i--)
    {
        pl = Player_by_index(i);
        CLR_BIT(pl->pl_status, RACE_OVER | FINISH);
        if (BIT(pl->obj_status, PAUSE) || (BIT(pl->obj_status, GAME_OVER) && pl->mychar == 'W') || Player_is_tank(pl))
        {
            continue;
        }
        num_active_players++;

        /* Kill any remaining players */
        if (!BIT(pl->obj_status, GAME_OVER))
            Kill_player(pl);
        else
            Player_death_reset(pl);
        if (pl != Player_by_index(i))
        {
            continue;
        }
        if ((pl->best_lap < bestlap || bestlap == 0) &&
            pl->best_lap > 0)
        {
            bestlap = pl->best_lap;
            num_best_players = 0;
        }
        if (pl->best_lap == bestlap)
            num_best_players++;
    }

    /* If someone completed a lap */
    if (bestlap > 0)
    {
        for (i = 0; i < NumPlayers; i++)
        {
            pl = Player_by_index(i);
            if (BIT(pl->obj_status, PAUSE) || (BIT(pl->obj_status, GAME_OVER) && pl->mychar == 'W') || Player_is_tank(pl))
            {
                continue;
            }
            if (pl->best_lap == bestlap)
            {
                sprintf(msg,
                        "%s %s the best lap time of %.2fs",
                        pl->name,
                        (num_best_players == 1) ? "had" : "shares",
                        (double)bestlap / FPS);
                Set_message(msg);
                SCORE(pl, 5 + num_active_players, pl->pos,
                      (num_best_players == 1) ? "[Fastest lap]" : "[Joint fastest lap]");
            }
        }

        updateScores = true;
    }
    else if (num_active_players > NumRobots)
    {
        Set_message("No-one even managed to complete one lap, you should be "
                    "ashamed of yourselves.");
    }

    Reset_all_players();

    Count_rounds();
}

void Compute_game_status(void)
{
    int i;
    player_t *pl;
    char msg[MSG_LEN];

    if (roundtime > 0)
        roundtime--;

    if (BIT(world->rules->mode, TIMING))
    {
        /*
         * We need a completely separate scoring system for race mode.
         * I'm not sure how race mode should interact with team mode,
         * so for the moment race mode takes priority.
         *
         * Race mode and limited lives mode interact. With limited lives on,
         * race ends after all players have completed the course, or have died.
         * With limited lives mode off, the race ends when the first player
         * completes the course - all remaining players are then killed to
         * reset them.
         *
         * In limited lives mode, where the race can be run to completion,
         * points are awarded not just to the winner but to everyone who
         * completes the course (with more going to the winner). These
         * points are awarded as the player crosses the line. At the end
         * of the race, a bonus is awarded to the player with the fastest lap.
         *
         * In unlimited lives mode, just the winner and the holder of the
         * fastest lap get points.
         */

        player_t *alive = NULL;
        int num_alive_players = 0,
            num_active_players = 0,
            num_finished_players = 0,
            num_race_over_players = 0,
            num_waiting_players = 0,
            position = 1,
            total_pts;
        double pts;

        /* First count the players */
        for (i = 0; i < NumPlayers; i++)
        {
            pl = Player_by_index(i);
            if (BIT(pl->obj_status, PAUSE) || Player_is_tank(pl))
                continue;
            if (!BIT(pl->obj_status, GAME_OVER))
                num_alive_players++;
            else if (pl->mychar == 'W')
            {
                num_waiting_players++;
                continue;
            }

            if (BIT(pl->pl_status, RACE_OVER))
            {
                num_race_over_players++;
                position++;
            }
            else if (BIT(pl->obj_status, FINISH))
                num_finished_players++;
            else if (!BIT(pl->obj_status, GAME_OVER))
                alive = pl;

            /*
             * An active player is one who is:
             *   still in the race.
             *   reached the finish line just now.
             *   has finished the race in a previous frame.
             *   died too often.
             */
            num_active_players++;
        }
        if (num_active_players == 0 && num_waiting_players == 0)
            return;

        /* Now if any players are unaccounted for */
        if (num_finished_players > 0)
        {
            /*
             * Ok, update positions. Everyone who finished the race in the last
             * frame gets the current position.
             */

            /* Only play the sound for the first person to cross the finish */
            if (position == 1)
                sound_play_all(PLAYER_WIN_SOUND);

            total_pts = 0;
            for (i = 0; i < num_finished_players; i++)
                total_pts += (10 + 2 * num_active_players) >> (position - 1 + i);
            pts = total_pts / num_finished_players;

            for (i = 0; i < NumPlayers; i++)
            {
                pl = Player_by_index(i);
                if (BIT(pl->obj_status, PAUSE) || (BIT(pl->obj_status, GAME_OVER) && pl->mychar == 'W') || Player_is_tank(pl))
                    continue;
                if (BIT(pl->obj_status, FINISH))
                {
                    CLR_BIT(pl->obj_status, FINISH);
                    SET_BIT(pl->pl_status, RACE_OVER);
                    if (pts > 0)
                    {
                        sprintf(msg,
                                "%s finishes %sin position %d "
                                "scoring %.2f point%s.",
                                pl->name,
                                (num_finished_players == 1) ? "" : "jointly ",
                                position, pts,
                                (pts == 1) ? "" : "s");
                        Set_message(msg);
                        sprintf(msg, "[Position %d%s]", position,
                                (num_finished_players == 1) ? "" : " (jointly)");
                        SCORE(pl, pts, pl->pos, msg);
                    }
                    else
                    {
                        sprintf(msg,
                                "%s finishes %sin position %d.",
                                pl->name,
                                (num_finished_players == 1) ? "" : "jointly ",
                                position);
                        Set_message(msg);
                    }
                }
            }
        }

        /*
         * If the maximum allowed time for this race is over, end it.
         */
        if (options.maxRoundTime > 0 && roundtime == 0)
        {
            Set_message("Timer expired. Race ends now.");
            Race_game_over();
            return;
        }

        /*
         * In limited lives mode, wait for everyone to die, except
         * for the last player.
         */
        if (BIT(world->rules->mode, LIMITED_LIVES))
        {
            if (num_alive_players > 1)
                return;
            if (num_alive_players == 1)
            {
                if (num_finished_players + num_race_over_players == 0)
                    return;
                if (!alive || alive->round == 0)
                    return;
            }
        }
        else if (num_finished_players == 0)
            return;

        Race_game_over();
    }
    else if (BIT(world->rules->mode, TEAM_PLAY))
    {
        /* Do we have a winning team ? */

        enum TeamState
        {
            TeamEmpty,
            TeamDead,
            TeamAlive
        } team_state[MAX_TEAMS];
        int num_dead_teams = 0;
        int num_alive_teams = 0;
        int winning_team = -1;

        for (i = 0; i < MAX_TEAMS; i++)
            team_state[i] = TeamEmpty;

        for (i = 0; i < NumPlayers; i++)
        {
            if (Player_is_tank(Player_by_index(i)))
                /* Ignore tanks. */
                continue;
            else if (BIT(Player_by_index(i)->obj_status, PAUSE))
                /* Ignore paused players. */
                continue;
#if 0
            /* not all teammode maps have treasures. */
            else if (world->teams[Player_by_index(i)->team].NumTreasures == 0) {
                /* Ignore players with no treasure troves */
                continue;
            }
#endif
            else if (BIT(Player_by_index(i)->obj_status, GAME_OVER))
            {
                if (team_state[Player_by_index(i)->team] == TeamEmpty)
                {
                    /* Assume all teammembers are dead. */
                    num_dead_teams++;
                    team_state[Player_by_index(i)->team] = TeamDead;
                }
            }
            /*
             * If the player is not paused and he is not in the
             * game over mode and his team owns treasures then he is
             * considered alive.
             * But he may not be playing though if the rest of the team
             * was genocided very quickly after game reset, while this
             * player was still being transported back to his homebase.
             */
            else if (team_state[Player_by_index(i)->team] != TeamAlive)
            {
                if (team_state[Player_by_index(i)->team] == TeamDead)
                    /* Oops!  Not all teammembers are dead yet. */
                    num_dead_teams--;
                team_state[Player_by_index(i)->team] = TeamAlive;
                ++num_alive_teams;
                /* Remember a team which was alive. */
                winning_team = Player_by_index(i)->team;
            }
        }

        if (num_alive_teams > 1)
        {
            char *bp;
            int teams_with_treasure = 0;
            int team_win[MAX_TEAMS];
            double team_score[MAX_TEAMS];
            int winners;
            int max_destroyed = 0;
            int max_left = 0;
            double max_score = 0;
            team_t *team_ptr;

            /*
             * Game is not over if more than one team which have treasures
             * still have one remaining in play.  Note that it is possible
             * for max_destroyed to be zero, in the case where a team
             * destroys some treasures and then all quit, and the remaining
             * teams did not destroy any.
             */
            for (i = 0; i < MAX_TEAMS; i++)
            {
                team_score[i] = 0;
                if (team_state[i] != TeamAlive)
                {
                    team_win[i] = 0;
                    continue;
                }
                team_win[i] = 1;
                team_ptr = &(world->teams[i]);
                if (team_ptr->TreasuresDestroyed > max_destroyed)
                    max_destroyed = team_ptr->TreasuresDestroyed;
                if ((team_ptr->TreasuresLeft > 0) ||
                    (team_ptr->NumTreasures == team_ptr->NumEmptyTreasures))
                    teams_with_treasure++;
            }

            /*
             * Game is not over if more than one team has treasure.
             */
            if ((teams_with_treasure > 1 || !max_destroyed) && (roundtime != 0 || options.maxRoundTime <= 0))
                return;

            if (options.maxRoundTime > 0 && roundtime == 0)
                Set_message("Timer expired. Round ends now.");

            /*
             * Find the winning team;
             *        Team destroying most number of treasures;
             *        If drawn; the one with most saved treasures,
             *        If drawn; the team with the most points,
             *        If drawn; an overall draw.
             */
            for (winners = i = 0; i < MAX_TEAMS; i++)
            {
                if (!team_win[i])
                    continue;
                if (world->teams[i].TreasuresDestroyed == max_destroyed)
                {
                    if (world->teams[i].TreasuresLeft > max_left)
                        max_left = world->teams[i].TreasuresLeft;
                    winning_team = i;
                    winners++;
                }
                else
                    team_win[i] = 0;
            }
            if (winners == 1)
            {
                sprintf(msg, " by destroying %d treasures", max_destroyed);
                Team_game_over(winning_team, msg);
                return;
            }

            for (i = 0; i < NumPlayers; i++)
            {
                if (BIT(Player_by_index(i)->obj_status, PAUSE) || Player_is_tank(Player_by_index(i)))
                    continue;
                team_score[Player_by_index(i)->team] += Player_by_index(i)->score;
            }

            for (winners = i = 0; i < MAX_TEAMS; i++)
            {
                if (!team_win[i])
                    continue;
                if (world->teams[i].TreasuresLeft == max_left)
                {
                    if (team_score[i] > max_score)
                        max_score = team_score[i];
                    winning_team = i;
                    winners++;
                }
                else
                    team_win[i] = 0;
            }
            if (winners == 1)
            {
                sprintf(msg,
                        " by destroying %d treasures"
                        " and successfully defending %d",
                        max_destroyed, max_left);
                Team_game_over(winning_team, msg);
                return;
            }

            for (winners = i = 0; i < MAX_TEAMS; i++)
            {
                if (!team_win[i])
                    continue;
                if (team_score[i] == max_score)
                {
                    winning_team = i;
                    winners++;
                }
                else
                    team_win[i] = 0;
            }
            if (winners == 1)
            {
                sprintf(msg, " by destroying %d treasures, saving %d, and "
                             "scoring %.2f points",
                        max_destroyed, max_left, max_score);
                Team_game_over(winning_team, msg);
                return;
            }

            /* Highly unlikely */

            sprintf(msg, " between teams ");
            bp = msg + strlen(msg);
            for (i = 0; i < MAX_TEAMS; i++)
            {
                if (!team_win[i])
                    continue;
                *bp++ = "0123456789"[i];
                *bp++ = ',';
                *bp++ = ' ';
            }
            bp -= 2;
            *bp = '\0';
            Team_game_over(-1, msg);
        }
        else if (num_dead_teams > 0)
        {
            if (num_alive_teams == 1)
                Team_game_over(winning_team, " by staying alive");
            else
                Team_game_over(-1, " as everyone died");
        }
        else
        {
            /*
             * num_alive_teams <= 1 && num_dead_teams == 0
             *
             * There is a possibility that the game has ended because players
             * quit, the game over state is needed to reset treasures.  We
             * must count how many treasures are missing, if there are any
             * the playing team (if any) wins.
             */
            int i, treasures_destroyed;

            for (treasures_destroyed = i = 0; i < MAX_TEAMS; i++)
                treasures_destroyed += (world->teams[i].NumTreasures - world->teams[i].NumEmptyTreasures - world->teams[i].TreasuresLeft);
            if (treasures_destroyed)
                Team_game_over(winning_team, " by staying in the game");
        }
    }
    else
    {

        /* Do we have a winner ? (No team play) */
        int num_alive_players = 0;
        int num_active_players = 0;
        int num_alive_robots = 0;
        int num_active_humans = 0;
        int winner = -1;

        for (i = 0; i < NumPlayers; i++)
        {
            player_t *pl_i = Player_by_index(i);

            if (BIT(pl_i->obj_status, PAUSE) || Player_is_tank(pl_i))
                continue;
            if (!BIT(pl_i->obj_status, GAME_OVER))
            {
                num_alive_players++;
                if (Player_is_robot(pl_i))
                    num_alive_robots++;
                winner = i; /* Tag player that's alive */
            }
            else if (Player_is_human(pl_i))
                num_active_humans++;
            num_active_players++;
        }

        if (num_alive_players == 1 && num_active_players > 1)
            Individual_game_over(winner);
        else if (num_alive_players == 0 && num_active_players >= 1)
            Individual_game_over(-1);
        else if (num_alive_robots > 1 && num_alive_players == num_alive_robots && num_active_humans > 0)
            Individual_game_over(-2);
        else if (options.maxRoundTime > 0 && roundtime == 0)
        {
            Set_message("Timer expired. Round ends now.");
            Individual_game_over(-1);
        }
    }
}

void Delete_player(player_t *pl)
{
    // player_t *pl = PlayersArray[ind];
    // Hack to get ind
    int ind = -1;
    for (int k = 0; k < NumPlayers; k++)
    {
        if (pl == PlayersArray[k])
        {
            ind = k;
            break;
        }
    }

    warn("Delete_player: ind = %d", ind);
    warn("Delete_player: getindarray's pl id = %d", GetIndArray[pl->id]);

    object_t *obj;
    int i, j,
        id = pl->id;

    /* call before important player structures are destroyed */
    Leave_alliance(pl);

    if (Player_is_robot(pl))
    {
        Robot_destroy(pl);
    }

    /* Delete remaining shots */
    for (i = NumObjs - 1; i >= 0; i--)
    {
        obj = Obj[i];
        if (obj->id == id)
        {
            if (obj->type == OBJ_BALL)
            {
                Delete_shot(i);
                BALL_PTR(obj)->ball_owner = NO_ID;
            }
            else if (BIT(obj->type, OBJ_DEBRIS | OBJ_SPARK))
            {
                /* Okay, so you want robot explosions to exist,
                 * even if the robot left the game. */
                obj->id = NO_ID;
            }
            else
            {
                if (!options.keepShots)
                {
                    obj->life = 0;
                    if (BIT(obj->type,
                            OBJ_CANNON_SHOT | OBJ_MINE | OBJ_SMART_SHOT | OBJ_HEAT_SHOT | OBJ_TORPEDO))
                    {
                        obj->mass = 0;
                    }
                }
                obj->id = NO_ID;
                if (BIT(obj->type, OBJ_MINE))
                    MINE_PTR(obj)->mine_owner = NO_ID;
            }
        }
        else
        {
            if (BIT(obj->type, OBJ_MINE))
            {
                mineobject_t *mine = MINE_PTR(obj);

                if (mine->mine_owner == id)
                {
                    mine->mine_owner = NO_ID;
                    if (!options.keepShots)
                    {
                        obj->life = 0;
                        obj->mass = 0;
                    }
                }
            }
            else if (BIT(obj->type, OBJ_CANNON_SHOT))
            {
                if (!options.keepShots)
                {
                    obj->life = 0;
                    obj->mass = 0;
                }
            }
            else if (BIT(obj->type, OBJ_BALL))
            {
                ballobject_t *ball = BALL_PTR(obj);
                if (ball->ball_owner == id)
                {
                    ball->ball_owner = NO_ID;
                }
            }
        }
    }

    if (pl->num_pulses)
    {
        for (i = 0; i < NumPulses; i++)
        {
            if (Pulses[i]->id == pl->id)
            {
                free(Pulses[i]);
                if (--NumPulses > i)
                {
                    Pulses[i] = Pulses[NumPulses];
                    i--;
                }
            }
        }
        pl->num_pulses = 0;
    }
    Free_ship_shape(pl->ship);

    sound_close(pl);

    NumPlayers--;
    if (Player_is_tank(pl))
    {
        NumPseudoPlayers--;
    }

    if (pl->team != TEAM_NOT_SET && !Player_is_tank(pl))
    {
        world->teams[pl->team].NumMembers--;
        if (Player_is_robot(pl))
            world->teams[pl->team].NumRobots--;
    }

    if (Player_is_robot(pl))
        NumRobots--;

    /*
     * Swap entry no 'ind' with the last one.
     *
     * Change the PlayersArray[] pointer array to have PlayersArray[ind] point to
     * a valid player and move our leaving player to PlayersArray[NumPlayers].
     */
    pl = PlayersArray[NumPlayers]; /* Swap pointers... */
    PlayersArray[NumPlayers] = PlayersArray[ind];
    PlayersArray[ind] = pl;
    pl = PlayersArray[NumPlayers]; /* Restore pointer. */

    GetIndArray[PlayersArray[ind]->id] = ind;
    GetIndArray[PlayersArray[NumPlayers]->id] = NumPlayers;

    Check_team_members(pl->team);

    for (i = NumPlayers - 1; i >= 0; i--)
    {
        player_t *pl_i = PlayersArray[i];

        if (Player_is_tank(pl_i) && pl_i->lock.pl_id == id)
        {
            /* remove tanks which were released by this player. */
            if (options.keepShots)
                pl_i->lock.pl_id = NO_ID;
            else
                Delete_player(pl_i);
            continue;
        }
        if (BIT(pl_i->lock.tagged, LOCK_PLAYER | LOCK_VISIBLE) && (pl_i->lock.pl_id == id || NumPlayers <= 1))
        {
            CLR_BIT(pl_i->lock.tagged, LOCK_PLAYER | LOCK_VISIBLE);
            CLR_BIT(pl_i->used, USES_TRACTOR_BEAM);
        }
        if (Player_is_robot(pl_i) && Robot_war_on_player(pl_i) == id)
        {
            Robot_reset_war(pl_i);
        }
        for (j = 0; j < LOCKBANK_MAX; j++)
        {
            if (pl_i->lockbank[j] == id)
                pl_i->lockbank[j] = NOT_CONNECTED;
        }
        for (j = 0; j < MAX_RECORDED_SHOVES; j++)
        {
            if (pl_i->shove_record[j].pusher_id == id)
                pl_i->shove_record[j].pusher_id = NO_ID;
        }
    }

    for (i = NumPlayers - 1; i >= 0; i--)
    {
        player_t *pl_i = PlayersArray[i];

        if (pl_i->conn != NULL)
            Send_leave(pl_i->conn, id);
        else if (Player_is_tank(pl_i))
        {
            if (pl_i->lock.pl_id == id)
                Delete_player(pl_i);
        }
    }

    release_ID(id);
}

void Detach_ball(player_t *pl, int obj)
{
    int i, cnt;
    // player_t *pl = PlayersArray[ind];

    if (obj == -1 || BALL_PTR(Obj[obj]) == pl->ball)
    {
        pl->ball = NULL;
        CLR_BIT(pl->used, USES_CONNECTOR);
    }

    if (BIT(pl->have, HAS_BALL))
    {
        for (cnt = i = 0; i < NumObjs; i++)
        {
            if (Obj[i]->type == OBJ_BALL && Obj[i]->id == pl->id)
            {
                if (obj == -1 || obj == i)
                {
                    Obj[i]->id = NO_ID;
                    /* Don't reset owner so you can throw balls */
                }
                else
                {
                    cnt++;
                }
            }
        }
        if (cnt == 0)
            CLR_BIT(pl->have, HAS_BALL);
        else
        {
            sound_play_sensors(pl->pos, DROP_BALL_SOUND);
        }
    }
}

void Kill_player(player_t *pl)
{
    // player_t *pl = PlayersArray[ind];

    Explode_fighter(pl);
    Player_death_reset(pl);
}

void Player_death_reset(player_t *pl)
{
    // player_t *pl = PlayersArray[ind];
    long minfuel;
    int i;

    if (Player_is_tank(pl))
    {
        Delete_player(pl);
        return;
    }

    Detach_ball(pl, -1);
    if (Player_uses_autopilot(pl) || BIT(pl->obj_status, HOVERPAUSE))
    {
        CLR_BIT(pl->obj_status, HOVERPAUSE);
        Autopilot(pl, false);
    }

    pl->vel.x = pl->vel.y = 0.0;
    pl->acc.x = pl->acc.y = 0.0;
    pl->emptymass = pl->mass = options.shipMass;
    pl->obj_status |= DEF_BITS;
    pl->obj_status &= ~(KILL_BITS);

    for (i = 0; i < NUM_ITEMS; i++)
    {
        if (!BIT(1U << i, ITEM_BIT_FUEL | ITEM_BIT_TANK))
            pl->item[i] = world->items[i].initial;
    }

    pl->forceVisible = 0;
    pl->shot_max = options.maxPlayerShots;
    pl->count = MAX(RECOVERY_DELAY, pl->count);
    pl->ecmcount = 0;
    pl->emergency_thrust_left = 0;
    pl->emergency_thrust_max = 0;
    pl->emergency_shield_left = 0;
    pl->emergency_shield_max = 0;
    pl->phasing_left = 0;
    pl->phasing_max = 0;
    pl->damaged = 0;
    pl->stunned = 0;
    pl->lock.distance = 0;

    pl->fuel.sum = (long)(pl->fuel.sum * 0.90); /* Loose 10% of fuel */
    minfuel = (world->items[ITEM_FUEL].initial * FUEL_SCALE_FACT);
    minfuel += (int)(rfrac() * (1 + minfuel) * 0.2f);
    pl->fuel.sum = MAX(pl->fuel.sum, minfuel);
    Player_init_fuel(pl, pl->fuel.sum);

    /*-BA Handle the combination of limited life games and
     *-BA robotLeaveLife by making a robot leave iff it gets
     *-BA eliminated in any round.  Means that robotLeaveLife
     *-BA is ignored, but that robotsLeave is still respected.
     *-KK Added check on race mode. Since in race mode everyone
     *-KK gets killed at the end of the round, all robots would
     *-KK be replaced in the next round. I don't think that's
     *-KK the Right Thing to do.
     *-KK Also, only check a robot's score at the end of the round.
     *-KK 27-2-98 Check on team mode too. It's very confusing to
     *-KK have different robots in your team every round.
     */

    if (!BIT(pl->obj_status, PAUSE))
    {
        pl->deaths++;

        if (BIT(world->rules->mode, LIMITED_LIVES))
        {
            pl->life--;
            if (pl->life == -1)
            {
                if (Player_is_robot(pl))
                {
                    if (!BIT(world->rules->mode, TIMING | TEAM_PLAY) || (options.robotsLeave && pl->score < options.robotLeaveScore))
                    {
                        Robot_delete(pl, false);
                        return;
                    }
                }
                pl->life = 0;
                SET_BIT(pl->obj_status, GAME_OVER);
                pl->mychar = 'D';
                Player_lock_closest(pl, false);
            }
        }
        else
        {
            pl->life++;
        }
    }

    pl->have = DEF_HAVE;
    pl->used |= DEF_USED;
    pl->used &= ~(USED_KILL);
    pl->used &= pl->have;
}

/* determines if two players are immune to eachother */
bool Team_immune(int id1, int id2)
{
    player_t *pl1, *pl2;

    if (id1 == id2)
        /* owned stuff is never team immune */
        return false;

    if (!options.teamImmunity)
        return false;

    if (id1 == NO_ID || id2 == NO_ID)
        /* can't find owner for cannon stuff */
        return false;

    pl1 = Player_by_id(id1);
    pl2 = Player_by_id(id2);

    if (Players_are_teammates(pl1, pl2))
        return true;

    if (Players_are_allies(pl1, pl2))
        return true;

    return false;
}
