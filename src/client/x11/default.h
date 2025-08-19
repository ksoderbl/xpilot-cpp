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

#ifndef DEFAULT_H
#define DEFAULT_H

#include <X11/X.h>

typedef struct
{
    const char *name;     /* option name */
    const char *noArg;    /* value for non-argument options */
    const char *fallback; /* default value */
    keys_t key;           /* key if not KEY_DUMMY */
    const char *help;     /* user help (multiline) */
    unsigned hash;        /* option name hashed. */
} option;

extern option options[];
extern int optionsCount; /* NELEM(options) */

// TODO: REMOVE
typedef struct
{
    KeySym keysym; /* Keysym-to-action array */
    keys_t key;
} keydefs_t;

extern keydefs_t *keyDefs;

#endif
