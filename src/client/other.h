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

#pragma once

#include <cstdint>

#include "shipshape.h"

class Other
{
public:
    double ratio;
    double score;
    short id;
    uint16_t team;
    short check;
    short round;
    long timing_loops;
    short timing;
    short life;
    short mychar;
    short alliance;
    short name_width;         /* In pixels */
    short name_len;           /* In bytes */
    short max_chars_in_names; /* name_width was calculated
                     for this value of maxCharsInNames */
    short ignorelevel;
    ShipShape *ship;
    char nick_name[MAX_CHARS];
    char user_name[MAX_CHARS];
    char host_name[MAX_CHARS];
    char id_string[MAX_CHARS];
};

extern Other *self; /* Player info */
extern Other *Others;
extern int num_others, max_others;

Other *Other_by_id(int id);
Other *Other_by_name(const char *name, bool show_error_msg);
ShipShape *Ship_by_id(int id);
