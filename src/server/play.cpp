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
#include <climits>
#include <cmath>

#include "server.h"

#define SERVER
#include "xpconfig.h"
#include "serverconst.h"
#include "global.h"
#include "saudio.h"
#include "score.h"
#include "object.h"
#include "xpmath.h"

int Punish_team(int ind, int t_destroyed, int t_target)
{
    static char msg[MSG_LEN];
    treasure_t *td = &world->treasures[t_destroyed];
    treasure_t *tt = &world->treasures[t_target];
    player_t *pl = PlayersArray[ind];
    int i;
    int win_score = 0, lose_score = 0;
    int win_team_members = 0, lose_team_members = 0;
    int somebody_flag = 0;
    int sc, por;

    Check_team_members(td->team);
    if (td->team == pl->team)
        return 0;

    if (BIT(world->rules->mode, TEAM_PLAY))
    {
        for (i = 0; i < NumPlayers; i++)
        {
            if (Player_is_tank(PlayersArray[i]) || (BIT(PlayersArray[i]->status, PAUSE) && PlayersArray[i]->count <= 0) || (BIT(PlayersArray[i]->status, GAME_OVER) && PlayersArray[i]->mychar == 'W' && PlayersArray[i]->score == 0))
                continue;
            if (PlayersArray[i]->team == td->team)
            {
                lose_score += PlayersArray[i]->score;
                lose_team_members++;
                if (BIT(PlayersArray[i]->status, GAME_OVER) == 0)
                    somebody_flag = 1;
            }
            else if (PlayersArray[i]->team == tt->team)
            {
                win_score += PlayersArray[i]->score;
                win_team_members++;
            }
        }
    }

    sound_play_all(DESTROY_BALL_SOUND);
    sprintf(msg, " < %s's (%d) team has destroyed team %d treasure >",
            pl->name, pl->team, td->team);
    Set_message(msg);

    if (!somebody_flag)
    {
        SCORE(pl, Rate(pl->score, CANNON_SCORE) / 2,
              tt->clk_pos.cx, tt->clk_pos.cy, "Treasure:");
        return 0;
    }

    td->destroyed++;
    world->teams[td->team].TreasuresLeft--;
    world->teams[tt->team].TreasuresDestroyed++;

    sc = 3 * Rate(win_score, lose_score);
    por = (sc * lose_team_members) / (2 * win_team_members + 1);

    for (i = 0; i < NumPlayers; i++)
    {
        if (Player_is_tank(PlayersArray[i]) ||
            (BIT(PlayersArray[i]->status, PAUSE) && PlayersArray[i]->count <= 0) ||
            (BIT(PlayersArray[i]->status, GAME_OVER) && PlayersArray[i]->mychar == 'W' && PlayersArray[i]->score == 0))
            continue;
        if (PlayersArray[i]->team == td->team)
        {
            SCORE(PlayersArray[i], -sc, tt->clk_pos.cx, tt->clk_pos.cy, "Treasure: ");
            if (options.treasureKillTeam)
                SET_BIT(PlayersArray[i]->status, KILLED);
        }
        else if (PlayersArray[i]->team == tt->team &&
                 (PlayersArray[i]->team != TEAM_NOT_SET || i == ind))
            SCORE(PlayersArray[i], (i == ind ? 3 * por : 2 * por), tt->clk_pos.cx, tt->clk_pos.cy, "Treasure: ");
    }

    if (options.treasureKillTeam)
    {
        PlayersArray[ind]->kills++;
    }

    updateScores = true;

    return 1;
}

/****************************
 * Functions for explosions.
 */

/* Create debris particles */
void Make_debris(
    /* pos.cx, pos.cy */ int cx, int cy,
    /* vel.x, vel.y   */ double velx, double vely,
    /* owner id       */ int id,
    /* owner team     */ unsigned short team,
    /* type           */ int type,
    /* mass           */ double mass,
    /* status         */ long status,
    /* color          */ int color,
    /* radius         */ int radius,
    /* min,max debris */ int min_debris, int max_debris,
    /* min,max dir    */ int min_dir, int max_dir,
    /* min,max speed  */ double min_speed, double max_speed,
    /* min,max life   */ int min_life, int max_life)
{
    object_t *debris;
    int i, num_debris, life;
    modifiers_t mods;

    if (BIT(world->rules->mode, WRAP_PLAY))
    {
        if (cx < 0)
            cx += world->click_width;
        else if (cx >= world->click_width)
            cx -= world->click_width;
        if (cy < 0)
            cy += world->click_height;
        else if (cy >= world->click_height)
            cy -= world->click_height;
    }
    if (cx < 0 || cx >= world->click_width || cy < 0 || cy >= world->click_height)
    {
        return;
    }
    if (max_life < min_life)
        max_life = min_life;
    if (options.ShotsLife >= FPS)
    {
        if (min_life > options.ShotsLife)
        {
            min_life = options.ShotsLife;
            max_life = options.ShotsLife;
        }
        else if (max_life > options.ShotsLife)
        {
            max_life = options.ShotsLife;
        }
    }
    if (min_speed * max_life > world->hypotenuse)
        min_speed = world->hypotenuse / max_life;
    if (max_speed * min_life > world->hypotenuse)
        max_speed = world->hypotenuse / min_life;
    if (max_speed < min_speed)
        max_speed = min_speed;

    CLEAR_MODS(mods);

    if (type == OBJ_SHOT)
    {
        SET_BIT(mods.warhead, CLUSTER);
        if (!options.ShotsGravity)
        {
            CLR_BIT(status, GRAVITY);
        }
    }

    num_debris = min_debris + (int)(rfrac() * (max_debris - min_debris));
    if (num_debris > MAX_TOTAL_SHOTS - NumObjs)
    {
        num_debris = MAX_TOTAL_SHOTS - NumObjs;
    }
    for (i = 0; i < num_debris; i++)
    {
        double speed, dx, dy, diroff;
        int dir, dirplus;

        if ((debris = Object_allocate()) == NULL)
        {
            break;
        }

        debris->color = color;
        debris->id = id;
        debris->team = team;
        Object_position_init_clicks(debris, cx, cy);
        dir = MOD2(min_dir + (int)(rfrac() * (max_dir - min_dir)), RES);
        dirplus = MOD2(dir + 1, RES);
        diroff = rfrac();
        dx = tcos(dir) + (tcos(dirplus) - tcos(dir)) * diroff;
        dy = tsin(dir) + (tsin(dirplus) - tsin(dir)) * diroff;
        speed = min_speed + rfrac() * (max_speed - min_speed);
        debris->vel.x = velx + dx * speed;
        debris->vel.y = vely + dy * speed;
        debris->acc.x = 0;
        debris->acc.y = 0;
        debris->mass = mass;
        debris->type = type;
        life = (int)(min_life + rfrac() * (max_life - min_life) + 1);
        if (life * speed > world->hypotenuse)
        {
            life = (long)(world->hypotenuse / speed);
        }
        debris->life = life;
        debris->fuselife = life;
        debris->pl_range = radius;
        debris->pl_radius = radius;
        debris->status = status;
        debris->mods = mods;
        Cell_add_object(debris);
    }
}
