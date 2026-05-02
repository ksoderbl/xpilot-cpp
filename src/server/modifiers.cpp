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

#include "modifiers.h"
#include "player.h"

#include "xperror.h"

#include <cctype>

#include "bit.h"

/*
 * Fast conversion of 'num' into 'str' starting at position 'i', returns
 * index of character after converted number.
 */
int num2str(int num, char *str, int i)
{
    int digits, t;

    if (num < 0)
    {
        str[i++] = '-';
        num = -num;
    }
    if (num < 10)
    {
        str[i++] = '0' + num;
        return i;
    }
    for (t = num, digits = 0; t; t /= 10, digits++)
        ;
    for (t = i + digits - 1; t >= 0; t--)
    {
        str[t] = num % 10;
        num /= 10;
    }
    return i + digits;
}

#define MODS_N_BIT0 (1 << 0) /* nuclear */
#define MODS_N_BIT1 (1 << 1) /* fullnuclear */
#define MODS_C_BIT (1 << 2)  /* cluster */
#define MODS_I_BIT (1 << 3)  /* implosion */
#define MODS_V_BIT0 (1 << 4) /* velocity */
#define MODS_V_BIT1 (1 << 5)
#define MODS_X_BIT0 (1 << 6) /* mini */
#define MODS_X_BIT1 (1 << 7)
#define MODS_Z_BIT0 (1 << 8) /* spread */
#define MODS_Z_BIT1 (1 << 9)
#define MODS_B_BIT0 (1 << 10) /* power */
#define MODS_B_BIT1 (1 << 11)
#define MODS_LS_BIT (1 << 12) /* stun laser */
#define MODS_LB_BIT (1 << 13) /* blinding laser */

static inline int Get_nuclear_modifier(modifiers_t mods)
{
    return 0;
}

static inline void Set_nuclear_modifier(modifiers_t *mods, int value)
{
    LIMIT(value, 0, MODS_NUCLEAR_MAX);
}

static inline int Get_cluster_modifier(modifiers_t mods)
{
    // (BIT(obj->mods.warhead, CLUSTER))
    // BIT(mods.warhead, CLUSTER)
    return 0;
}

static inline void Set_cluster_modifier(modifiers_t *mods, int value)
{
    LIMIT(value, 0, MODS_CLUSTER_MAX);
}

static inline int Get_implosion_modifier(modifiers_t mods)
{
    return 0;
}

static inline void Set_implosion_modifier(modifiers_t *mods, int value)
{
    LIMIT(value, 0, MODS_IMPLOSION_MAX);
}

static inline int Get_velocity_modifier(modifiers_t mods)
{
    return 0;
}

static inline void Set_velocity_modifier(modifiers_t *mods, int value)
{
    LIMIT(value, 0, MODS_VELOCITY_MAX);
}

static inline int Get_mini_modifier(modifiers_t mods)
{
    // minis = (mods.mini + 1)
    return 0;
}

static inline void Set_mini_modifier(modifiers_t *mods, int value)
{
    LIMIT(value, 0, MODS_MINI_MAX);
}

static inline int Get_spread_modifier(modifiers_t mods)
{
    return 0;
}

static inline void Set_spread_modifier(modifiers_t *mods, int value)
{
    LIMIT(value, 0, MODS_SPREAD_MAX);
}

static inline int Get_power_modifier(modifiers_t mods)
{
    return 0;
}

static inline void Set_power_modifier(modifiers_t *mods, int value)
{
    LIMIT(value, 0, MODS_POWER_MAX);
}

static inline int Get_laser_modifier(modifiers_t mods)
{
    return 0;
}

static inline void Set_laser_modifier(modifiers_t *mods, int value)
{
    LIMIT(value, 0, MODS_LASER_MAX);
}

int Mods_set(modifiers_t *mods, modifier_t modifier, int val)
{
    // SET_BIT(mods.warhead, CLUSTER)
    // SET_BIT(mods.warhead, IMPLOSION)
    // SET_BIT(mods.nuclear, MODS_NUCLEAR)
    // CLR_BIT(obj->mods.warhead, CLUSTER)
    return 0;
}

int Mods_get(modifiers_t mods, modifier_t modifier)
{
    switch (modifier)
    {
    case ModsNuclear:
        return Get_nuclear_modifier(mods);
    case ModsCluster:
        return Get_cluster_modifier(mods);
    case ModsImplosion:
        return Get_implosion_modifier(mods);
    case ModsVelocity:
        return Get_velocity_modifier(mods);
    case ModsMini:
        return Get_mini_modifier(mods);
    case ModsSpread:
        return Get_spread_modifier(mods);
    case ModsPower:
        return Get_power_modifier(mods);
    case ModsLaser:
        return Get_laser_modifier(mods);
    default:
        break;
    }
    return 0;
}

/*
 * modstr must be able to hold at least MAX_CHARS chars.
 */
void Mods_to_string(modifiers_t mods, char *modstr, size_t size)
{
    int i = 0;

    if (size < MAX_CHARS)
        return;

    int nuclear = Get_nuclear_modifier(mods);
    // if (BIT(mods.nuclear, MODS_FULLNUCLEAR))
    if (nuclear & MODS_FULLNUCLEAR)
        modstr[i++] = 'F';
    // if (BIT(mods.nuclear, MODS_NUCLEAR))
    if (nuclear & MODS_NUCLEAR)
        modstr[i++] = 'N';
    // if (BIT(mods.warhead, CLUSTER))
    if (Get_cluster_modifier(mods))
        modstr[i++] = 'C';
    // if (BIT(mods.warhead, IMPLOSION))
    if (Get_implosion_modifier(mods))
        modstr[i++] = 'I';
    int velocity = Get_velocity_modifier(mods);
    // if (mods.velocity)
    if (velocity)
    {
        if (i)
            modstr[i++] = ' ';
        modstr[i++] = 'V';
        i = num2str(velocity, modstr, i);
    }
    int mini = Get_mini_modifier(mods);
    // if (mods.mini)
    if (mini)
    {
        if (i)
            modstr[i++] = ' ';
        modstr[i++] = 'X';
        i = num2str(mini + 1, modstr, i);
    }
    int spread = Get_spread_modifier(mods);
    // if (mods.spread)
    if (spread)
    {
        if (i)
            modstr[i++] = ' ';
        modstr[i++] = 'Z';
        i = num2str(spread, modstr, i);
    }
    int power = Get_power_modifier(mods);
    // if (mods.power)
    if (power)
    {
        if (i)
            modstr[i++] = ' ';
        modstr[i++] = 'B';
        i = num2str(power, modstr, i);
    }
    int laser = Get_laser_modifier(mods);
    // if (mods.laser)
    if (laser)
    {
        if (i)
            modstr[i++] = ' ';
        modstr[i++] = 'L';
        modstr[i++] = (BIT(laser, MODS_LASER_STUN) ? 'S' : 'B');
    }
    modstr[i] = '\0';
}

void Mods_filter(modifiers_t *mods)
{
    if (!BIT(world->rules->mode, ALLOW_NUKES))
        // mods->nuclear = 0;
        Mods_set(mods, ModsNuclear, 0);

    if (!BIT(world->rules->mode, ALLOW_CLUSTERS))
        // CLR_BIT(mods->warhead, CLUSTER);
        Mods_set(mods, ModsCluster, 0);

    if (!BIT(world->rules->mode, ALLOW_MODIFIERS))
    {
        // CLR_BIT(mods->warhead, IMPLOSION);
        // mods->velocity = 0;
        // mods->mini = 0;
        // mods->spread = 0;
        // mods->power = 0;
        Mods_set(mods, ModsImplosion, 0);
        Mods_set(mods, ModsVelocity, 0);
        Mods_set(mods, ModsMini, 0);
        Mods_set(mods, ModsSpread, 0);
        Mods_set(mods, ModsPower, 0);
    }

    if (!BIT(world->rules->mode, ALLOW_LASER_MODIFIERS))
        // mods->laser = 0;
        Mods_set(mods, ModsLaser, 0);
}

static int str2num(const char **strp, int min, int max)
{
    const char *str = *strp;
    int num = 0;

    while (isdigit(*str))
    {
        num *= 10;
        num += *str++ - '0';
    }
    *strp = str;
    LIMIT(num, min, max);
    return num;
}

void Player_set_modbank(player_t *pl, int bank, const char *str)
{
    const char *cp;
    modifiers_t mods;
    int mini, velocity, spread, power;

    warn("Player_set_modbank: player %s, bank %d, str %s", pl->name, bank, str);

    if (bank >= NUM_MODBANKS)
        return;

    Mods_clear(&mods);
    if (BIT(world->rules->mode, ALLOW_MODIFIERS))
    {
        for (cp = str; *cp; cp++)
        {
            switch (*cp)
            {
            case 'F':
            case 'f':
                if (!BIT(world->rules->mode, ALLOW_NUKES))
                    break;
                if (*(cp + 1) == 'N' || *(cp + 1) == 'n')
                    // SET_BIT(mods.nuclear, MODS_FULLNUCLEAR);
                    Mods_set(&mods, ModsNuclear, MODS_FULLNUCLEAR);
                break;
            case 'N':
            case 'n':
                if (!BIT(world->rules->mode, ALLOW_NUKES))
                    break;
                Mods_set(&mods, ModsNuclear, MODS_NUCLEAR);
                break;
            case 'C':
            case 'c':
                if (!BIT(world->rules->mode, ALLOW_CLUSTERS))
                    break;
                Mods_set(&mods, ModsCluster, 1);
                break;
            case 'I':
            case 'i':
                Mods_set(&mods, ModsImplosion, 1);
                break;
            case 'V':
            case 'v':
                cp++;
                velocity = str2num(&cp, 0, MODS_VELOCITY_MAX);
                Mods_set(&mods, ModsVelocity, velocity);
                cp--;
                break;
            case 'X':
            case 'x':
                cp++;
                mini = str2num(&cp, 1, MODS_MINI_MAX + 1) - 1;
                Mods_set(&mods, ModsMini, mini);
                cp--;
                break;
            case 'Z':
            case 'z':
                cp++;
                spread = str2num(&cp, 0, MODS_SPREAD_MAX);
                Mods_set(&mods, ModsSpread, spread);
                cp--;
                break;
            case 'B':
            case 'b':
                cp++;
                power = str2num(&cp, 0, MODS_POWER_MAX);
                Mods_set(&mods, ModsPower, power);
                cp--;
                break;
            case 'L':
            case 'l':
                cp++;
                if (!BIT(world->rules->mode, ALLOW_LASER_MODIFIERS))
                    break;
                if (*cp == 'S' || *cp == 's')
                    // SET_BIT(mods.laser, MODS_LASER_STUN);
                    Mods_set(&mods, ModsLaser, MODS_LASER_STUN);
                if (*cp == 'B' || *cp == 'b')
                    // SET_BIT(mods.laser, MODS_LASER_BLIND);
                    Mods_set(&mods, ModsLaser, MODS_LASER_BLIND);
                break;
            default:
                /* Ignore unknown modifiers. */
                break;
            }
        }
    }
    pl->modbank[bank] = mods;
}
