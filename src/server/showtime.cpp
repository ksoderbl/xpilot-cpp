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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <cstdlib>
#include <cstdio>
#include <ctime>

#include <sys/time.h>

#define SERVER
#include "proto.h"


char *showtime(void)
{
    time_t                now;
    static time_t        past;
    struct tm                *tmp;
    static char                month_names[13][4] = {
                            "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
                            "Bug"
                        };
    static char                buf[80];

    time(&now);
    if (now != past) {
        tmp = localtime(&now);
        sprintf(buf, "%02d %s %02d:%02d:%02d",
                tmp->tm_mday, month_names[tmp->tm_mon],
                tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
        past = now;
    }

    return buf;
}

