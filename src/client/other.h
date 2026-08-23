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

#include <string>
#include <vector>

#include <cstdint>

#include "shipshape.h"

class Other
{
public:
    Other();
    ~Other();

    double ratio;
    double score;
    int id;
    uint16_t team;
    int check;
    int round;
    long timing_loops;
    int timing;
    int life;
    int mychar;
    int alliance;
    int name_width;         /* In pixels */
    int name_len;           /* In bytes */
    int max_chars_in_names; /* name_width was calculated
                     for this value of maxCharsInNames */
    int ignorelevel;
    ShipShape *ship;
    std::string nick_name;
    std::string user_name;
    std::string host_name;
    std::string id_string;
};

extern Other *self; /* Player info */
extern std::vector<Other *> others;

Other *Other_by_id(int id);
Other *Other_by_name(const char *name, bool show_error_msg);
ShipShape *Ship_by_id(int id);
