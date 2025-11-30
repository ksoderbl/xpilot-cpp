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

#include <cctype>

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