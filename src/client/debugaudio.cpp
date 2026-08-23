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
 * Debug audio driver.
 * Sometimes new xpilot versions break the XPilot sound system
 * which is hard to debug on systems without sound.
 * Hence this file to monitor what's going on.
 */

#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <unistd.h>

#include "audio.h"

#include "xperror.h"

int audioDeviceInit(char *display)
{
    debugprint("debug audio: init\n");

    return 0;
}

void audioDevicePlay(char *filename, int type, int volume, void **private_data)
{
    debugprint("debug audio: play file %s, type %d, vol %d, priv %p\n",
               filename, type, volume, private_data);
}

void audioDeviceEvents(void)
{
    debugprint("debug audio: events\n");
}

void audioDeviceUpdate(void)
{
    debugprint("debug audio: update\n");
}

void audioDeviceFree(void *private_data)
{
    debugprint("debug audio: audioDeviceFree\n");
}

void audioDeviceClose()
{
    debugprint("debug audio: audioDeviceClose\n");
}
