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

#include "config.h"

/* randommt.cpp */
#include "randommt.h"

/* xpmath.cpp */
extern double rfrac(void);
extern int mod(int x, int y);
extern int ON(const char *optval);
extern int OFF(const char *optval);
extern double findDir(double x, double y);
extern double findDir(double x, double y);
extern double tcos(double x);
extern double tsin(double x);

/* xpstrdup.cpp */
extern char *xp_safe_strdup(const char *old_string);

/* xpstrlcpy.ccp */
#ifndef HAVE_STRLCPY
size_t strlcpy(char *dest, const char *src, size_t size);
#endif
#ifndef HAVE_STRLCAT
size_t strlcat(char *dest, const char *src, size_t size);
#endif

/* xpstrcasecmp.cpp */
#ifndef HAVE_STRCASECMP
int strcasecmp(const char *str1, const char *str2);
#endif
#ifndef HAVE_STRNCASECMP
int strncasecmp(const char *str1, const char *str2, size_t n);
#endif

/* showtime.cpp */
char *showtime(void);
