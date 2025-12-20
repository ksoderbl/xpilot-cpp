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

#include <cstdio>
#include <cstring>

#include "bit.h"
#include "const.h"
#include "rules.h"
#include "xperror.h"

#include "netclient.h"
#include "paint.h"

/*
 * Globals.
 */
ipos_t world;
ipos_t realWorld;

uint16_t team; /* What team is the player on? */
bool players_exposed;

short ext_view_width;   /* Width of extended visible area */
short ext_view_height;  /* Height of extended visible area */
int active_view_width;  /* Width of active map area displayed. */
int active_view_height; /* Height of active map area displayed. */
int ext_view_x_offset;  /* Offset ext_view_width */
int ext_view_y_offset;  /* Offset ext_view_height */
uint8_t debris_colors;  /* Number of debris intensities from server */

char modBankStr[NUM_MODBANKS][MAX_CHARS]; /* modifier banks */

int maxKeyDefs;

long loops = 0;
long loopsSlow = 0; /* Proceeds slower than loops */
double timePerFrame = 0.0;
static double time_counter = 0.0;

unsigned draw_width, draw_height;
int num_spark_colors;

double scaleFactor;
double scaleFactor_s;
short scaleArray[SCALE_ARRAY_SIZE];

int Check_view_dimensions(void)
{
    int width_wanted = draw_width;
    int height_wanted = draw_height;
    int srv_width, srv_height;

    width_wanted = (int)(width_wanted * scaleFactor + 0.5);
    height_wanted = (int)(height_wanted * scaleFactor + 0.5);

    srv_width = width_wanted;
    srv_height = height_wanted;
    LIMIT(srv_height, MIN_VIEW_SIZE, MAX_VIEW_SIZE);
    LIMIT(srv_width, MIN_VIEW_SIZE, MAX_VIEW_SIZE);
    if (ext_view_width != srv_width || ext_view_height != srv_height)
        Send_display();

    active_view_width = ext_view_width;
    active_view_height = ext_view_height;
    ext_view_x_offset = 0;
    ext_view_y_offset = 0;
    if (width_wanted > ext_view_width)
    {
        ext_view_width = width_wanted;
        ext_view_x_offset = (width_wanted - active_view_width) / 2;
    }
    if (height_wanted > ext_view_height)
    {
        ext_view_height = height_wanted;
        ext_view_y_offset = (height_wanted - active_view_height) / 2;
    }

    return 0;
}

void Paint_frame_start(void)
{
    if (start_loops != end_loops)
        warn("Start neq. End (%ld,%ld,%ld)", start_loops, end_loops, loops);
    loops = end_loops;

    /*
     * If time() changed from previous value, assume one second has passed.
     */
    if (newSecond)
    {
        /* kps - improve */
        recordFPS = (int)(clientFPS + 0.5);
        timePerFrame = 1.0 / recordFPS;

        /* TODO: move this somewhere else */
        /* check once per second if we are playing */
        if (newSecond && self && !strchr("PW", self->mychar))
            played_this_round = true;
    }

    /*
     * Instead of using loops to determining if things are drawn this frame,
     * loopsSlow should be used. We don't want things to be drawn too fast
     * at high fps.
     */
    time_counter += timePerFrame;
    if (time_counter >= (1.0 / 12))
    {
        loopsSlow++;
        time_counter -= (1.0 / 12);
    }
}

void Paint_score_table(void)
{
    struct team_score
    {
        int score;
        int life;
        int playing;
    };
    struct team_score team[MAX_TEAMS],
        *team_order[MAX_TEAMS];
    other_t *other,
        **order;
    int i, j, k, best = -1;
    double ratio, best_ratio = -1e7;

    if (scoresChanged == 0)
    {
        return;
    }

    if (players_exposed == false)
    {
        return;
    }

    if (num_others < 1)
    {
        Paint_score_start();
        scoresChanged = 0;
        return;
    }

    if ((order = (other_t **)malloc(num_others * sizeof(other_t *))) == NULL)
    {
        error("No memory for score");
        return;
    }
    if (BIT(Setup->mode, TEAM_PLAY | TIMING) == TEAM_PLAY)
    {
        memset(&team[0], 0, sizeof team);
    }
    for (i = 0; i < num_others; i++)
    {
        other = &Others[i];
        if (BIT(Setup->mode, TIMING))
        {
            /*
             * Sort the score table on position in race.
             * Put paused and waiting players last as well as tanks.
             */
            if (strchr("PTW", other->mychar))
            {
                j = i;
            }
            else
            {
                for (j = 0; j < i; j++)
                {
                    if (order[j]->timing < other->timing)
                    {
                        break;
                    }
                    if (strchr("PTW", order[j]->mychar))
                    {
                        break;
                    }
                    if (order[j]->timing == other->timing)
                    {
                        if (order[j]->timing_loops > other->timing_loops)
                        {
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            if (BIT(Setup->mode, LIMITED_LIVES))
            {
                ratio = (float)other->score;
            }
            else
            {
                ratio = (float)other->score / (other->life + 1);
            }
            if (best == -1 || ratio > best_ratio)
            {
                best_ratio = ratio;
                best = i;
            }
            for (j = 0; j < i; j++)
            {
                if (order[j]->score < other->score)
                {
                    break;
                }
            }
        }
        for (k = i; k > j; k--)
        {
            order[k] = order[k - 1];
        }
        order[j] = other;

        if (BIT(Setup->mode, TEAM_PLAY | TIMING) == TEAM_PLAY)
        {
            switch (other->mychar)
            {
            case 'P':
            case 'W':
            case 'T':
                break;
            case ' ':
            case 'R':
                if (BIT(Setup->mode, LIMITED_LIVES))
                {
                    team[other->team].life += other->life + 1;
                }
                else
                {
                    team[other->team].life += other->life;
                }
                /*FALLTHROUGH*/
            default:
                team[other->team].playing++;
                team[other->team].score += other->score;
                break;
            }
        }
    }
    Paint_score_start();
    if (BIT(Setup->mode, TIMING))
    {
        best = order[0] - Others;
    }
    for (i = 0; i < num_others; i++)
    {
        other = order[i];
        j = other - Others;
        Paint_score_entry(i, other, (j == best) ? true : false);
    }
    if (BIT(Setup->mode, TEAM_PLAY | TIMING) == TEAM_PLAY)
    {
        int pos = num_others + 1;
        int num_playing_teams = 0;
        for (i = 0; i < MAX_TEAMS; i++)
        {
            if (team[i].playing)
            {
                for (j = 0; j < num_playing_teams; j++)
                {
                    if (team[i].score > team_order[j]->score || (team[i].score == team_order[j]->score && ((BIT(Setup->mode, LIMITED_LIVES))
                                                                                                               ? (team[i].life > team_order[j]->life)
                                                                                                               : (team[i].life < team_order[j]->life))))
                    {
                        for (k = i; k > j; k--)
                        {
                            team_order[k] = team_order[k - 1];
                        }
                        break;
                    }
                }
                team_order[j] = &team[i];
                num_playing_teams++;
            }
        }
        for (i = 0; i < num_playing_teams; i++)
        {
            other_t tmp;
            tmp.id = -1;
            tmp.team = team_order[i] - &team[0];
            tmp.war_id = -1;
            tmp.name_width = 0;
            tmp.ship = NULL;
            sprintf(tmp.nick_name, "Team %d", tmp.team);
            strcpy(tmp.user_name, tmp.nick_name);
            strcpy(tmp.nick_name, "");
            if (BIT(Setup->mode, LIMITED_LIVES) && team_order[i]->life == 0)
            {
                tmp.mychar = 'D';
            }
            else
            {
                tmp.mychar = ' ';
            }
            tmp.score = team_order[i]->score;
            tmp.life = team_order[i]->life;
            Paint_score_entry(pos++, &tmp, false);
        }
    }

    free(order);

    scoresChanged = 0;
}