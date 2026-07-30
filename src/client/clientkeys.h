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

#include "keys.h"

typedef enum
{
    /*
     * Hack (patent pending BG):
     * Here all keys only used by the client can be defined.
     * Be careful that the key vector is not set with these keys or
     * array boundaries will be exceeded.
     * The reason for this hack is to create new empty key slots while
     * retaining compatibility.  Change this at the next major cleanup.
     */
    KEY_MSG_1 = NUM_SERVER_KEYS, /* talk macros */
    KEY_MSG_2,
    KEY_MSG_3,
    KEY_MSG_4,
    KEY_MSG_5,
    KEY_MSG_6,
    KEY_MSG_7,
    KEY_MSG_8,
    KEY_MSG_9,
    KEY_MSG_10,
    KEY_MSG_11,
    KEY_MSG_12,
    KEY_MSG_13,
    KEY_MSG_14,
    KEY_MSG_15,
    KEY_MSG_16,
    KEY_MSG_17,
    KEY_MSG_18,
    KEY_MSG_19,
    KEY_MSG_20,

    KEY_ID_MODE,
    KEY_TOGGLE_OWNED_ITEMS,
    KEY_TOGGLE_MESSAGES,
    KEY_POINTER_CONTROL,
    KEY_TOGGLE_RECORD,
    KEY_TOGGLE_SOUND, /* no ifdef SOUND here */
    KEY_PRINT_MSGS_STDOUT,
    KEY_TALK_CURSOR_LEFT,
    KEY_TALK_CURSOR_RIGHT,
    KEY_TALK_CURSOR_UP,
    KEY_TALK_CURSOR_DOWN,
    KEY_SWAP_SCALEFACTOR,
    KEY_TOGGLE_RADAR_SCORE,
    KEY_INCREASE_POWER,
    KEY_DECREASE_POWER,
    KEY_INCREASE_TURNSPEED,
    KEY_DECREASE_TURNSPEED,
    KEY_TOGGLE_FULLSCREEN,
    KEY_EXIT,
    KEY_YES,
    KEY_NO,
    // NUM_CLIENT_KEYS /* The number of keys really used by the client. */
} client_keys_t;

constexpr int NUM_CLIENT_KEYS = KEY_NO + 1;
