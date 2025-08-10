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
/* This piece of code was provided by Greg Renda (greg@ncd.com). */

#ifndef SAUDIO_H
#define SAUDIO_H

#if defined(SERVER_SOUND) && defined(SERVER) && !defined(SOUND)
/* Enable only sound support in the server, not in the client. */
#define SOUND 1
#endif

#define SDBG(x) /*#x*/

#ifndef SOUND

/*
 * Define like this to avoid having to put #ifdef SOUND all over the place.
 */
#define sound_player_init(player) ((player)->audio = NULL)
#define sound_player_onoff(player, onoff)
#define sound_play_player(player, index)
#define sound_play_all(index)
#define sound_play_sensors(cx, cy, index)
#define sound_play_queued(player)
#define sound_close(player)

#else /* SOUND */

#include "audio.h"

int sound_player_init(player_t *pl);
void sound_player_onoff(player_t *pl, bool on);
void sound_play_player(player_t *pl, int index);
void sound_play_all(int index);
void sound_play_sensors(int cx, int cy, int index);
void sound_play_queued(player_t *pl);
void sound_close(player_t *pl);

#endif /* SOUND */

#endif /* SAUDIO_H */
