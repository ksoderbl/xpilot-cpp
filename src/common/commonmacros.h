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

#include <cstdlib>

#include <sys/time.h>

#include "xperror.h"

/* borrowed from autobook */
#define XCALLOC(type, num) \
    ((type *)calloc((num), sizeof(type)))
#define XMALLOC(type, num) \
    ((type *)malloc((num) * sizeof(type)))
#define XREALLOC(type, p, num) \
    ((type *)realloc((p), (num) * sizeof(type)))
#define XFREE(ptr)         \
    do                     \
    {                      \
        if (ptr)           \
        {                  \
            free(ptr);     \
            ptr = nullptr; \
        }                  \
    } while (0)

static inline double timeval_to_seconds(struct timeval *tvp)
{
    return (double)tvp->tv_sec + tvp->tv_usec * 1e-6;
}

static inline struct timeval seconds_to_timeval(double t)
{
    struct timeval tv;

    tv.tv_sec = (unsigned)t;
    tv.tv_usec = (unsigned)(((t - (double)tv.tv_sec) * 1e6) + 0.5);

    return tv;
}

/* returns 'tv2 - tv1' */
static inline int timeval_sub(struct timeval *tv2,
                              struct timeval *tv1)
{
    int s, us;

    s = tv2->tv_sec - tv1->tv_sec;
    us = tv2->tv_usec - tv1->tv_usec;

    return 1000000 * s + us;
}

#define SWAP_DOUBLES(a, b) \
    {                      \
        double tmp = a;    \
        a = b;             \
        b = tmp;           \
    }
