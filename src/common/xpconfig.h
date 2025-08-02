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

#ifndef XPCONFIG_H
#define XPCONFIG_H

#ifndef CONF_DATADIR
#error "CONF_DATADIR NOT DEFINED. GIVING UP."
#endif

#ifndef CONF_LOCALGURU
#define CONF_LOCALGURU PACKAGE_BUGREPORT
#endif

#ifndef CONF_DEFAULT_MAP
#define CONF_DEFAULT_MAP "globe.xp"
#endif

// #ifndef CONF_DATADIR
// #define CONF_DATADIR "/usr/local/games/lib/xpilot/"
// #endif

#ifndef CONF_MAPDIR
#define CONF_MAPDIR CONF_DATADIR "maps/"
#endif

#ifndef CONF_TEXTUREDIR
#define CONF_TEXTUREDIR CONF_DATADIR "textures/"
#endif

#ifndef CONF_SOUNDDIR
#define CONF_SOUNDDIR CONF_DATADIR "sound/"
#endif

#ifndef CONF_DEFAULTS_FILE_NAME
#define CONF_DEFAULTS_FILE_NAME CONF_DATADIR "defaults.txt"
#endif

#ifndef CONF_PASSWORD_FILE_NAME
#define CONF_PASSWORD_FILE_NAME CONF_DATADIR "password.txt"
#endif

#ifndef CONF_ROBOTFILE
#define CONF_ROBOTFILE CONF_DATADIR "robots.txt"
#endif

#ifndef CONF_SERVERMOTDFILE
#define CONF_SERVERMOTDFILE CONF_DATADIR "servermotd.txt"
#endif

#ifndef CONF_LOCALMOTDFILE
#define CONF_LOCALMOTDFILE CONF_DATADIR "localmotd.txt"
#endif

#ifndef CONF_LOGFILE
#define CONF_LOGFILE CONF_DATADIR "log.txt"
#endif

#ifndef CONF_SHIP_FILE
#define CONF_SHIP_FILE CONF_DATADIR "shipshapes.txt"
#endif

#ifndef CONF_SOUNDFILE
#define CONF_SOUNDFILE CONF_DATADIR "sounds.sounds"
#endif

#ifndef CONF_ZCAT_EXT
#define CONF_ZCAT_EXT ".gz"
#endif

#ifndef CONF_ZCAT_FORMAT
#define CONF_ZCAT_FORMAT "gzip -d -c < %s"
#endif

#ifndef CONF_CONTACTADDRESS
#define CONF_CONTACTADDRESS "xpilot@xpilot.org"
#endif

/*
 * If COMPRESSED_MAPS is defined, the server will attempt to uncompress
 * maps on the fly (but only if neccessary). ZCAT_FORMAT should produce
 * a command that will unpack the given .Z file to stdout (for use in popen).
 * ZCAT_EXT should define the proper compressed file extension.
 */

#define COMPRESSED_MAPS

#ifdef DEBUG
#define D(x)                    \
        {                       \
                {x};            \
                fflush(stdout); \
        }
#else
#define D(x)
#endif

#define xpprintf printf

char *Conf_datadir(void);
char *Conf_defaults_file_name(void);
char *Conf_password_file_name(void);
char *Conf_mapdir(void);
char *Conf_default_map(void);
char *Conf_servermotdfile(void);
char *Conf_localmotdfile(void);
char *Conf_logfile(void);
char *Conf_ship_file(void);
char *Conf_mapdir(void);
char *Conf_texturedir(void);
char *Conf_sounddir(void);
char *Conf_soundfile(void);
char *Conf_localguru(void);
char *Conf_contactaddress(void);
char *Conf_robotfile(void);
char *Conf_zcat_ext(void);
char *Conf_zcat_format(void);

#endif /* XPCONFIG_H */
