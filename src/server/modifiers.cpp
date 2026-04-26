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

int str2num(char **strp, int min, int max)
{
    char *str = *strp;
    int num = 0;

    while (isdigit(*str))
    {
        num *= 10;
        num += *str++ - '0';
    }
    *strp = str;
    if (num < min || num > max)
        return min;
    return num;
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
}

static inline int Get_cluster_modifier(modifiers_t mods)
{
    return 0;
}

static inline void Set_cluster_modifier(modifiers_t *mods, int value)
{
}

static inline int Get_implosion_modifier(modifiers_t mods)
{
    return 0;
}

static inline void Set_implosion_modifier(modifiers_t *mods, int value)
{
}

static inline int Get_velocity_modifier(modifiers_t mods)
{
    return 0;
}

static inline void Set_velocity_modifier(modifiers_t *mods, int value)
{
}

static inline int Get_mini_modifier(modifiers_t mods)
{
    return 0;
}

static inline void Set_mini_modifier(modifiers_t *mods, int value)
{
}
static inline int Get_spread_modifier(modifiers_t mods)
{
    return 0;
}

static inline void Set_spread_modifier(modifiers_t *mods, int value)
{
}

static inline int Get_power_modifier(modifiers_t mods)
{
    return 0;
}

static inline void Set_power_modifier(modifiers_t *mods, int value)
{
}

static inline int Get_laser_modifier(modifiers_t mods)
{
    return 0;
}

static inline void Set_laser_modifier(modifiers_t *mods, int value)
{
}

int Mods_set(modifiers_t *mods, modifier_t modifier, int val)
{
    return 0;
}

int Mods_get(modifiers_t mods, modifier_t modifier)
{
    return 0;
}

/*
 * modstr must be able to hold at least MAX_CHARS chars.
 */
void Mods_to_string(modifiers_t mods, char *modstr, size_t size)
{
    int i = 0;
    if (BIT(mods.nuclear, MODS_FULLNUCLEAR))
        modstr[i++] = 'F';
    if (BIT(mods.nuclear, MODS_NUCLEAR))
        modstr[i++] = 'N';
    if (BIT(mods.warhead, CLUSTER))
        modstr[i++] = 'C';
    if (BIT(mods.warhead, IMPLOSION))
        modstr[i++] = 'I';
    if (mods.velocity)
    {
        if (i)
            modstr[i++] = ' ';
        modstr[i++] = 'V';
        i = num2str(mods.velocity, modstr, i);
    }
    if (mods.mini)
    {
        if (i)
            modstr[i++] = ' ';
        modstr[i++] = 'X';
        i = num2str(mods.mini + 1, modstr, i);
    }
    if (mods.spread)
    {
        if (i)
            modstr[i++] = ' ';
        modstr[i++] = 'Z';
        i = num2str(mods.spread, modstr, i);
    }
    if (mods.power)
    {
        if (i)
            modstr[i++] = ' ';
        modstr[i++] = 'B';
        i = num2str(mods.power, modstr, i);
    }
    if (mods.laser)
    {
        if (i)
            modstr[i++] = ' ';
        modstr[i++] = 'L';
        modstr[i++] = (BIT(mods.laser, MODS_LASER_STUN) ? 'S' : 'B');
    }
    modstr[i] = '\0';
}
