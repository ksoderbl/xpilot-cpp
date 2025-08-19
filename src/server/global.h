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

#ifndef GLOBAL_H
#define GLOBAL_H

#include "object.h"
#include "map.h"
#include "list.h"

#include "option.h"

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define STR80 (80)

typedef struct
{
    char owner[STR80];
    char host[STR80];
} server_t;

/*
 * Global data.
 */
#define FPS options.framesPerSecond
// #define NumObjs (ObjCount + 0)

// extern player_t **Players;
// extern object_t *Obj[];
// extern pulse_t *Pulses[];
// extern ecm_t *Ecms[];
// extern trans_t *Transporters[];
extern long frame_loops;
// extern int NumPlayers;
// extern int NumPseudoPlayers;
// extern int ObjCount;
// extern int NumPulses;
// extern int NumEcms;
// extern int NumTransporters;
// extern int NumAlliances;
// extern int NumRobots;
extern int login_in_progress;
// extern world_t World;
extern server_t Server;
extern long DEF_BITS, KILL_BITS, DEF_HAVE, DEF_USED, USED_KILL;
extern int GetInd[];
extern int ShutdownServer;
extern int ShutdownDelay;
extern long main_loops;
extern int mainLoopTime;
extern char *serverAddr;
extern bool updateScores;
extern int game_lock;
extern int roundtime;
extern int roundsPlayed;
extern long KILLING_SHOTS;
// extern unsigned SPACE_BLOCKS;

#endif /* GLOBAL_H */
