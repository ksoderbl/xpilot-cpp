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
 * Include portability related stuff in one file.
 */
#ifndef PORTABILITY_H_INCLUDED
#define PORTABILITY_H_INCLUDED

#define PATHNAME_SEP '/'

/*
 * Prototypes for OS function wrappers in portability.c.
 */
extern int Get_process_id(void); /* getpid */
extern void Get_login_name(char *buf, int size);

#endif /* PORTABILITY_H_INCLUDED */
