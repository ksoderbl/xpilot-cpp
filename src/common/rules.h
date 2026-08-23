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

/*
 * Bitfield definitions for playing mode.
 */
// constexpr int CRASH_WITH_PLAYER = (1 << 0);
// constexpr int BOUNCE_WITH_PLAYER = (1 << 1);
// constexpr int PLAYER_KILLINGS = (1 << 2);
constexpr int LIMITED_LIVES = (1 << 3);
constexpr int TIMING = (1 << 4);
// constexpr int PLAYER_SHIELDING = (1 << 6);
// constexpr int LIMITED_VISIBILITY = (1 << 7);
constexpr int TEAM_PLAY = (1 << 8);
constexpr int WRAP_PLAY = (1 << 9);
// constexpr int ALLOW_NUKES = (1 << 10);
// constexpr int ALLOW_CLUSTERS = (1 << 11);
// constexpr int ALLOW_MODIFIERS = (1 << 12);
// constexpr int ALLOW_LASER_MODIFIERS = (1 << 13);
constexpr int ALLIANCES = (1 << 14);

/*
 * Client uses only a subset of them:
 */
constexpr int CLIENT_RULES_MASK = (WRAP_PLAY | TEAM_PLAY | TIMING | LIMITED_LIVES | ALLIANCES);

/*
 * Player status bits, used in network protocol.
 * The bits that the client needs must fit into a byte,
 * so the first 8 bitvalues are reserved for that purpose.
 */
constexpr uint32_t PLAYER_STATUS_PLAYING = (1 << 0);   /* alive or killed */
constexpr uint32_t PLAYER_STATUS_PAUSE = (1 << 1);     /* paused */
constexpr uint32_t PLAYER_STATUS_GAME_OVER = (1 << 2); /* waiting or dead */
constexpr uint32_t PLAYER_STATUS_THRUSTING = (1 << 3); /* not used by client? */

#define OLD_PLAYING (1U << 0)   /* alive or killed */
#define OLD_PAUSE (1U << 1)     /* paused */
#define OLD_GAME_OVER (1U << 2) /* waiting or dead */
