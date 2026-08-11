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
/*
 * client audio
 */

#define SOUND

#ifdef SOUND

#define MAX_RANDOM_SOUNDS 6

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <sys/types.h>

#include "caudio.h"
#include "xpconfig.h"
#include "const.h"
#include "types.h"

#include "commonproto.h"
#include "commonmacros.h"
#include "xperror.h"

#include "client.h"

#include "audio.h"

/* options */
static bool audioEnabled = false;
bool sound;
char soundFile[PATH_MAX]; /* audio mappings */
int maxVolume;            /* maximum volume (in percent) */
/* options end */

static struct
{
    char **filenames;
    void **priv;
    int nsounds;
} table[MAX_SOUNDS];

static bool audioIsEnabled(void)
{
    if (!audioEnabled)
        return false;
    if (!sound)
        return false;
    if (maxVolume <= 0)
        return false;
    return true;
}

void audioInit(char *display)
{
    FILE *fp;
    char buf[512], *file, *soundstr, *ifile;
    int i, j;

#if 0
    /* kps - let's not do this, otherwise sounds can't be enabled ingame */
    if (!maxVolume)
    {
        xpinfo("maxVolume is 0: no sound.\n");
        return;
    }
#endif
    if (!(fp = fopen(soundFile, "r")))
    {
        error("Could not open soundfile %s", soundFile);
        return;
    }

    while (fgets(buf, sizeof(buf), fp))
    {
        /* ignore comments */
        if (*buf == '\n' || *buf == '#')
            continue;

        soundstr = strtok(buf, " \t");
        file = strtok(nullptr, " \t\n");

        for (i = 0; i < MAX_SOUNDS; i++)
            if (!strcmp(soundstr, soundNames[i]))
            {
                size_t filename_ptrs_size = sizeof(char *) * MAX_RANDOM_SOUNDS;
                size_t private_ptrs_size = sizeof(void *) * MAX_RANDOM_SOUNDS;
                table[i].filenames = (char **)malloc(filename_ptrs_size);
                table[i].priv = (void **)malloc(private_ptrs_size);
                memset(table[i].priv, 0, private_ptrs_size);
                ifile = strtok(file, " \t\n|");
                j = 0;
                while (ifile && j < MAX_RANDOM_SOUNDS)
                {
                    if (*ifile == '/')
                        table[i].filenames[j] = strdup(ifile);
                    else
                    {
                        size_t filename_size = strlen(Conf_sounddir()) + strlen(ifile) + 1;
                        table[i].filenames[j] = (char *)malloc(filename_size);
                        if (table[i].filenames[j] != nullptr)
                        {
                            strcpy(table[i].filenames[j], Conf_sounddir());
                            strcat(table[i].filenames[j], ifile);
                        }
                    }
                    j++;
                    ifile = strtok(nullptr, " \t\n|");
                }
                table[i].nsounds = j;
                break;
            }

        if (i == MAX_SOUNDS)
            warn("audioInit: Unknown sound '%s' (ignored)", soundstr);
    }

    fclose(fp);

    audioEnabled = !audioDeviceInit(display);
}

void audioCleanup(void)
{
    /* release malloc'ed memory here */
    int i, j;

    for (i = 0; i < MAX_SOUNDS; i++)
    {
        for (j = 0; j < table[i].nsounds; j++)
            audioDeviceFree(table[i].priv[j]);
        XFREE(table[i].filenames);
        XFREE(table[i].priv);
    }
    audioDeviceClose();
}

void audioEvents(void)
{
    if (audioIsEnabled())
        audioDeviceEvents();
}

void audioUpdate(void)
{
    if (audioIsEnabled())
        audioDeviceUpdate();
}

int Handle_audio(int type, int volume)
{
    int pick = 0;

    if (!audioIsEnabled() || !table[type].filenames)
        return 0;

    if (table[type].nsounds > 1)
    {
        /*
         * Multiple sounds were specified.  Pick one at random.
         */
        pick = randomMT() % table[type].nsounds;
    }

    if (!table[type].priv[pick])
    {
        int i;

        /* eliminate duplicate sounds */
        for (i = 0; i < MAX_SOUNDS; i++)
            if (i != type && table[i].filenames && table[i].priv[pick] && !strcmp(table[type].filenames[0], table[i].filenames[0]))
            {
                table[type].priv[0] = table[i].priv[0];
                break;
            }
    }

    audioDevicePlay(table[type].filenames[pick], type, MIN(volume, maxVolume),
                    &table[type].priv[pick]);

    return 0;
}

#endif /* SOUND */
