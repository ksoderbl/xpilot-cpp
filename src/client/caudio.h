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
/* This piece of code was provided by Greg Renda (greg@ncd.com). */
/* 961112 - Bucko - Header file */
/*
 * client audio
 */

#pragma once

#define SOUND

#ifdef SOUND

#include <climits>

extern char soundFile[PATH_MAX]; /* audio mappings */
extern int maxVolume;            /* maximum volume (in percent) */
extern bool sound;               /* option 'sound' */

int Handle_audio(int type, int volume);
void audioInit(char *display);
void audioCleanup(void);
void audioEvents(void);
void audioUpdate(void);
int audioDeviceInit(char *display);
void audioDevicePlay(char *filename, int type, int volume, void **priv);
void audioDeviceEvents(void);
void audioDeviceUpdate(void);
void audioDeviceFree(void *priv);
void audioDeviceClose(void);

#endif
