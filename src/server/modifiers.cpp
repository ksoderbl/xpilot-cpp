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

#include "bit.h"

#include "modifiers.h"
#include "object.h"

/*
 * Fast conversion of `num' into `str' starting at position `i', returns
 * index of character after converted number.
 */
static int num2str(int num, char *str, int i)
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

void Mods_to_string(modifiers_t mods, char *modstr, size_t size)
{
    int i = 0;
    if (BIT(mods.nuclear, FULLNUCLEAR))
        modstr[i++] = 'F';
    if (BIT(mods.nuclear, NUCLEAR))
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
        modstr[i++] = (BIT(mods.laser, STUN) ? 'S' : 'B');
    }
    modstr[i] = '\0';
}