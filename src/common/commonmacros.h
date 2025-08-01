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

#ifndef COMMONMACROS_H
#define COMMONMACROS_H

#include <cstdlib>
#include "xperror.h"

/*
 * Macro to add one new element of a given type to a dynamic array.
 * T is the type of the element.
 * P is the pointer to the array memory.
 * N is the current number of elements in the array.
 * M is the current size of the array.
 * V is the new element to add.
 * The goal is to keep the number of malloc/realloc calls low
 * while not wasting too much memory because of over-allocation.
 */
#define STORE(T, P, N, M, V)                                                    \
    if (N >= M && ((M <= 0)                                                     \
                       ? (P = (T *)malloc((M = 1) * sizeof(*P)))                \
                       : (P = (T *)realloc(P, (M += M) * sizeof(*P)))) == NULL) \
    {                                                                           \
        xperror("No memory");                                                   \
        N = M = 0;                                                              \
        return -1;                                                              \
    }                                                                           \
    else                                                                        \
        (P[N++] = V)

/*
 * Macro to make room in a given dynamic array for new elements.
 * P is the pointer to the array memory.
 * N is the current number of elements in the array.
 * M is the current size of the array.
 * T is the type of the elements.
 * E is the number of new elements to store in the array.
 * The goal is to keep the number of malloc/realloc calls low
 * while not wasting too much memory because of over-allocation.
 */
#define EXPAND(P, N, M, T, E)                     \
    if ((N) + (E) > (M))                          \
    {                                             \
        if ((M) <= 0)                             \
        {                                         \
            M = (E) + 2;                          \
            P = (T *)malloc((M) * sizeof(T));     \
            N = 0;                                \
        }                                         \
        else                                      \
        {                                         \
            M = ((M) << 1) + (E);                 \
            P = (T *)realloc(P, (M) * sizeof(T)); \
        }                                         \
        if (P == NULL)                            \
        {                                         \
            xperror("No memory");                 \
            N = M = 0;                            \
            return; /* ! */                       \
        }                                         \
    }

#define UNEXPAND(P, N, M) \
    if ((N) < ((M) >> 2)) \
    {                     \
        free(P);          \
        M = 0;            \
    }                     \
    N = 0;

#ifndef PAINT_FREE
#define PAINT_FREE 1
#endif
#if PAINT_FREE
#define RELEASE(P, N, M) \
    if (!(N))            \
        ;                \
    else                 \
        (free(P), (M) = 0, (N) = 0)
#else
#define RELEASE(P, N, M) ((N) = 0)
#endif

/* borrowed from autobook */
#define XCALLOC(type, num) \
    ((type *)calloc((num), sizeof(type)))
#define XMALLOC(type, num) \
    ((type *)malloc((num) * sizeof(type)))
#define XREALLOC(type, p, num) \
    ((type *)realloc((p), (num) * sizeof(type)))
#define XFREE(ptr)      \
    do                  \
    {                   \
        if (ptr)        \
        {               \
            free(ptr);  \
            ptr = NULL; \
        }               \
    } while (0)

/* Use this to remove unused parameter warning. */
#define UNUSED_PARAM(x) x = x

#endif
