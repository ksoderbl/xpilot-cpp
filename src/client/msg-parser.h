/*
 * XPilot, a multiplayer gravity war game.
 *
 * Copyright (C) 2003-2007 by
 *
 *      Kristian Söderblom
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

#include "other.h"

/* typedefs begin */
typedef enum
{
    BmsNone = 0,
    BmsBall,
    BmsSafe,
    BmsCover,
    BmsPop
} msg_bms_t;

bool Msg_is_in_angle_brackets(const char *message);
bool Msg_is_game_msg(const char *message);
void Msg_scan_game_msg(const char *message);
void Msg_scan_angle_bracketed_msg(const char *message);
msg_bms_t Msg_do_bms(const char *message);
void Add_roundend_messages(Other **order);

bool Bms_test_state(msg_bms_t bms);
void Bms_set_state(msg_bms_t bms);
