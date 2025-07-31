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

/*
 * The following macros decide the speed of the game and
 * how often the server should draw a frame.  (Hmm...)
 */

#ifndef        UPDATES_PR_FRAME
#    define UPDATES_PR_FRAME        1
#endif

/*
 * If COMPRESSED_MAPS is defined, the server will attempt to uncompress
 * maps on the fly (but only if neccessary). ZCAT_FORMAT should produce
 * a command that will unpack the given .Z file to stdout (for use in popen).
 * ZCAT_EXT should define the proper compressed file extension.
 */

#define COMPRESSED_MAPS

#ifdef        DEBUG
#        define D(x)        { {x}; fflush(stdout); }
#else
#        define D(x)
#endif

#define        xpprintf        printf

char *Conf_libdir(void);
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
