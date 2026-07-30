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

#include "keys.h"

std::string keyToStr(keys_t key)
{
    switch (key)
    {
    case KEY_DUMMY:
        return "KEY_DUMMY";
    case KEY_LOCK_NEXT:
        return "KEY: TODO";
    case KEY_LOCK_PREV:
        return "KEY: TODO";
    case KEY_LOCK_CLOSE:
        return "KEY: TODO";
    case KEY_CHANGE_HOME:
        return "KEY: TODO";
    case KEY_SHIELD:
        return "KEY_SHIELD";
    case KEY_FIRE_SHOT:
        return "KEY_FIRE_SHOT";
    case KEY_FIRE_MISSILE:
        return "KEY: TODO";
    case KEY_FIRE_TORPEDO:
        return "KEY: TODO";
    case KEY_TOGGLE_NUCLEAR:
        return "KEY: TODO";
    case KEY_FIRE_HEAT:
        return "KEY: TODO";
    case KEY_DROP_MINE:
        return "KEY: TODO";
    case KEY_DETACH_MINE:
        return "KEY: TODO";
    case KEY_TURN_LEFT:
        return "KEY_TURN_LEFT";
    case KEY_TURN_RIGHT:
        return "KEY: TODO";
    case KEY_SELF_DESTRUCT:
        return "KEY: TODO";
    case KEY_LOSE_ITEM:
        return "KEY: TODO";
    case KEY_PAUSE:
        return "KEY_PAUSE";
    case KEY_TANK_DETACH:
        return "KEY_TANK_DETACH";
    case KEY_TANK_NEXT:
        return "KEY: TODO";
    case KEY_TANK_PREV:
        return "KEY: TODO";
    case KEY_TOGGLE_VELOCITY:
        return "KEY: TODO";
    case KEY_TOGGLE_CLUSTER:
        return "KEY: TODO";
    case KEY_SWAP_SETTINGS:
        return "KEY: TODO";
    case KEY_REFUEL:
        return "KEY: TODO";
    case KEY_CONNECTOR:
        return "KEY: TODO";
    case KEY_UNUSED_26:
        return "KEY: TODO";
    case KEY_UNUSED_27:
        return "KEY: TODO";
    case KEY_UNUSED_28:
        return "KEY: TODO";
    case KEY_UNUSED_29:
        return "KEY: TODO";
    case KEY_THRUST:
        return "KEY_THRUST";
    case KEY_CLOAK:
        return "KEY: TODO";
    case KEY_ECM:
        return "KEY: TODO";
    case KEY_DROP_BALL:
        return "KEY: TODO";
    case KEY_TRANSPORTER:
        return "KEY: TODO";
    case KEY_TALK:
        return "KEY: TODO";
    case KEY_FIRE_LASER:
        return "KEY: TODO";
    case KEY_LOCK_NEXT_CLOSE:
        return "KEY: TODO";
    case KEY_TOGGLE_COMPASS:
        return "KEY: TODO";
    case KEY_TOGGLE_MINI:
        return "KEY: TODO";
    case KEY_TOGGLE_SPREAD:
        return "KEY: TODO";
    case KEY_TOGGLE_POWER:
        return "KEY: TODO";
    case KEY_TOGGLE_AUTOPILOT:
        return "KEY: TODO";
    case KEY_TOGGLE_LASER:
        return "KEY: TODO";
    case KEY_EMERGENCY_THRUST:
        return "KEY: TODO";
    case KEY_TRACTOR_BEAM:
        return "KEY: TODO";
    case KEY_PRESSOR_BEAM:
        return "KEY: TODO";
    case KEY_CLEAR_MODIFIERS:
        return "KEY: TODO";
    case KEY_LOAD_MODIFIERS_1:
        return "KEY: TODO";
    case KEY_LOAD_MODIFIERS_2:
        return "KEY: TODO";
    case KEY_LOAD_MODIFIERS_3:
        return "KEY: TODO";
    case KEY_LOAD_MODIFIERS_4:
        return "KEY: TODO";
    case KEY_SELECT_ITEM:
        return "KEY: TODO";
    case KEY_PHASING:
        return "KEY: TODO";
    case KEY_REPAIR:
        return "KEY_REPAIR";
    case KEY_TOGGLE_IMPLOSION:
        return "KEY: TODO";
    case KEY_REPROGRAM:
        return "KEY: TODO";
    case KEY_LOAD_LOCK_1:
        return "KEY: TODO";
    case KEY_LOAD_LOCK_2:
        return "KEY: TODO";
    case KEY_LOAD_LOCK_3:
        return "KEY: TODO";
    case KEY_LOAD_LOCK_4:
        return "KEY: TODO";
    case KEY_EMERGENCY_SHIELD:
        return "KEY: TODO";
    case KEY_HYPERJUMP:
        return "KEY: TODO";
    case KEY_DETONATE_MINES:
        return "KEY: TODO";
    case KEY_DEFLECTOR:
        return "KEY: TODO";
    case KEY_UNUSED_65:
        return "KEY: TODO";
    case KEY_UNUSED_66:
        return "KEY: TODO";
    case KEY_UNUSED_67:
        return "KEY: TODO";
    case KEY_UNUSED_68:
        return "KEY: TODO";
    case KEY_UNUSED_69:
        return "KEY: TODO";
    case KEY_UNUSED_70:
        return "KEY: TODO";
    case KEY_UNUSED_71:
        return "KEY: TODO";
    case NUM_SERVER_KEYS:
        return "KEY: TODO";

    // /*
    //  * Hack (patent pending BG):
    //  * Here all keys only used by the client can be defined.
    //  * Be careful that the key vector is not set with these keys or
    //  * array boundaries will be exceeded.
    //  * The reason for this hack is to create new empty key slots while
    //  * retaining compatibility.  Change this at the next major cleanup.
    //  */
    // ,
    case KEY_MSG_1:
        return "KEY_MSG_1";
    case KEY_MSG_2:
        return "KEY_MSG_2";
    case KEY_MSG_3:
        return "KEY_MSG_3";
    case KEY_MSG_4:
        return "KEY_MSG_4";
    case KEY_MSG_5:
        return "KEY_MSG_5";
    case KEY_MSG_6:
        return "KEY_MSG_6";
    case KEY_MSG_7:
        return "KEY_MSG_7";
    case KEY_MSG_8:
        return "KEY_MSG_8";
    case KEY_MSG_9:
        return "KEY_MSG_9";
    case KEY_MSG_10:
        return "KEY_MSG_10";
    case KEY_MSG_11:
        return "KEY: TODO";
    case KEY_MSG_12:
        return "KEY: TODO";
    case KEY_MSG_13:
        return "KEY: TODO";
    case KEY_MSG_14:
        return "KEY: TODO";
    case KEY_MSG_15:
        return "KEY: TODO";
    case KEY_MSG_16:
        return "KEY: TODO";
    case KEY_MSG_17:
        return "KEY: TODO";
    case KEY_MSG_18:
        return "KEY: TODO";
    case KEY_MSG_19:
        return "KEY: TODO";
    case KEY_MSG_20:
        return "KEY: TODO";

    case KEY_ID_MODE:
        return "KEY: TODO";
    case KEY_TOGGLE_OWNED_ITEMS:
        return "KEY: TODO";
    case KEY_TOGGLE_MESSAGES:
        return "KEY: TODO";
    case KEY_POINTER_CONTROL:
        return "KEY: TODO";
    case KEY_TOGGLE_RECORD:
        return "KEY: TODO";
    case KEY_TOGGLE_SOUND:
        return "KEY: TODO";
    case KEY_PRINT_MSGS_STDOUT:
        return "KEY: TODO";
    case KEY_TALK_CURSOR_LEFT:
        return "KEY: TODO";
    case KEY_TALK_CURSOR_RIGHT:
        return "KEY: TODO";
    case KEY_TALK_CURSOR_UP:
        return "KEY: TODO";
    case KEY_TALK_CURSOR_DOWN:
        return "KEY: TODO";
    case KEY_SWAP_SCALEFACTOR:
        return "KEY: TODO";
    case KEY_TOGGLE_RADAR_SCORE:
        return "KEY: TODO";
    case KEY_INCREASE_POWER:
        return "KEY: TODO";
    case KEY_DECREASE_POWER:
        return "KEY: TODO";
    case KEY_INCREASE_TURNSPEED:
        return "KEY: TODO";
    case KEY_DECREASE_TURNSPEED:
        return "KEY: TODO";
    case KEY_TOGGLE_FULLSCREEN:
        return "KEY: TODO";
    case KEY_EXIT:
        return "KEY: TODO";
    case KEY_YES:
        return "KEY: TODO";
    case KEY_NO:
        return "KEY: TODO";
    case NUM_CLIENT_KEYS:
        return "KEY: TODO";
    }
    return "unknown";
}
