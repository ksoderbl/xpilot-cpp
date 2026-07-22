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
/*
 * Adapted from 'The UNIX Programming Environment' by Kernighan & Pike
 * and an example from the manualpage for vprintf by
 * Gaute Nessan, University of Tromsoe.
 *
 * Modified by Bjoern Stabell.
 * Dick Balaska added the memory/leak checking.
 * Bert Gijsbers added warn, fatal, coredump.
 */

#pragma once

#include <cerrno>
#include <cstdarg>

extern void xpinfo(const char *fmt, ...);
extern void warn(const char *fmt, ...);
extern void error(const char *fmt, ...);
extern void fatal(const char *fmt, ...);
extern void dumpcore(const char *fmt, ...);

extern void init_error(const char *prog);
