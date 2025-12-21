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

#include <cmath>

#include "const.h"
#include "types.h"

// extern double tbl_sin[];
// extern double tbl_cos[];

// #if 0
//   /* The way it was: one table, and always range checking. */
// #define tsin(x) (tbl_sin[MOD2(x, TABLE_SIZE)])
// #define tcos(x) (tbl_sin[MOD2((x) + TABLE_SIZE / 4, TABLE_SIZE)])
// #else
// #if 0
//    /* Range checking: find out where the table size is exceeded. */
// #define CHK2(x, m) ((MOD2(x, m) != x) ? (printf("MOD %s:%d:%s\n", __FILE__, __LINE__, #x), MOD2(x, m)) : (x))
// #else
// /* No range checking. */
// #define CHK2(x, m) (x)
// #endif
// /* New table lookup with optional range checking and no extra calculations. */
// #define tsin(x) (tbl_sin[CHK2(x, TABLE_SIZE)])
// #define tcos(x) (tbl_cos[CHK2(x, TABLE_SIZE)])
// #endif

int ON(const char *optval);
int OFF(const char *optval);
int mod(int x, int y);
int f2i(double f);
double findDir(double x, double y);
double rfrac(void);
// void Make_table(void);
double tcos(double x);
double tsin(double x);
