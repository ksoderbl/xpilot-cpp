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
#include <cstdint>

// #include "player.h"

/*
 * Weapons modifiers.
 */
// typedef struct
// {
//     uint8_t nuclear;  /* N  modifier */
//     uint8_t warhead;  /* CI modifier */
//     uint8_t velocity; /* V# modifier */
//     uint8_t mini;     /* X# modifier */
//     uint8_t spread;   /* Z# modifier */
//     uint8_t power;    /* B# modifier */
//     uint8_t laser;    /* LS LB modifier */
//     uint8_t dummy;    /* alignment */
// } modifiers_t;

typedef uint16_t modifiers_t;

// #define Mods_clear(mods) memset(&(mods), 0, sizeof(modifiers_t))

#define MODS_NUCLEAR_MAX 3   /* - N FN */
#define MODS_CLUSTER_MAX 1   /* - C */
#define MODS_IMPLOSION_MAX 1 /* - I */

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
    memset((mods), 0, sizeof(modifiers_t));
}

// void Player_set_modbank(player_t *pl, int bank, const char *str);

void Mods_to_string(modifiers_t mods, char *dst, size_t size);
int Mods_set(modifiers_t *mods, modifier_t modifier, int val);
int Mods_get(modifiers_t mods, modifier_t modifier);
void Mods_filter(modifiers_t *mods);
