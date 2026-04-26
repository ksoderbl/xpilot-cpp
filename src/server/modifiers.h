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

#include <cstring>

/*
 * Weapons modifiers.
 */
typedef struct
{
    unsigned int nuclear : 2;  /* N  modifier */
    unsigned int warhead : 2;  /* CI modifier */
    unsigned int velocity : 2; /* V# modifier */
    unsigned int mini : 2;     /* X# modifier */
    unsigned int spread : 2;   /* Z# modifier */
    unsigned int power : 2;    /* B# modifier */
    unsigned int laser : 2;    /* LS LB modifier */
    unsigned int spare : 2;    /* padding for alignment */
} modifiers_t;

#define CLEAR_MODS(mods) memset(&(mods), 0, sizeof(modifiers_t))

#define MODS_NUCLEAR_MAX 2 /* - N FN */
// #define NUCLEAR (1U << 0)
// #define FULLNUCLEAR (1U << 1)

#define MODS_WARHEAD_MAX 3 /* - C I CI */
#define CLUSTER (1U << 0)
#define IMPLOSION (1U << 1)

#define MODS_VELOCITY_MAX 3 /* - V1 V2 V3 */
#define MODS_MINI_MAX 3     /* - X2 X3 X4 */
#define MODS_SPREAD_MAX 3   /* - Z1 Z2 Z3 */
#define MODS_POWER_MAX 3    /* - B1 B2 B3 */

#define MODS_LASER_MAX 2 /* - LS LB */
// #define STUN (1U << 0)
// #define BLIND (1U << 1)

#define MODS_NUCLEAR (1 << 0)
#define MODS_FULLNUCLEAR (1 << 1)
#define MODS_LASER_STUN (1 << 0)
#define MODS_LASER_BLIND (1 << 1)

typedef enum
{
    ModsNuclear,   /* 0,  MODS_NUCLEAR, MODS_NUCLEAR|MODS_FULLNUCLEAR */
    ModsCluster,   /* 0 - MODS_CLUSTER_MAX */
    ModsImplosion, /* 0 - MODS_IMPLOSION_MAX */
    ModsVelocity,  /* 0 - MODS_VELOCITY_MAX */
    ModsMini,      /* 0 - MODS_MINI_MAX */
    ModsSpread,    /* 0 - MODS_SPREAD_MAX */
    ModsPower,     /* 0 - MODS_POWER_MAX */
    ModsLaser      /* 0,  MODS_LASER_STUN, MODS_LASER_BLIND */
} modifier_t;

static inline void Mods_clear(modifiers_t *mods)
{
    // *mods = 0;
    memset(&(mods), 0, sizeof(modifiers_t));
}

int num2str(int num, char *str, int i);
int str2num(char **strp, int min, int max);

void Mods_to_string(modifiers_t mods, char *modstr, size_t size);
