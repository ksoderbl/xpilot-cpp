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

#include <cstdlib>
#include <cstring>
#include <cctype>

#include "xpconfig.h"
#include "xperror.h"

char *Conf_datadir(void)
{
    static char conf[] = CONF_DATADIR;

    return conf;
}

char *Conf_defaults_file_name(void)
{
    static char conf[] = CONF_DEFAULTS_FILE_NAME;

    return conf;
}

char *Conf_password_file_name(void)
{
    static char conf[] = CONF_PASSWORD_FILE_NAME;

    return conf;
}

char *Conf_mapdir(void)
{
    static char conf[] = CONF_MAPDIR;

    return conf;
}

static char conf_default_map_string[] = CONF_DEFAULT_MAP;

char *Conf_default_map(void)
{
    return conf_default_map_string;
}

char *Conf_servermotdfile(void)
{
    static char conf[] = CONF_SERVERMOTDFILE;
    static char env[] = "XPILOTSERVERMOTD";
    char *filename;

    filename = getenv(env);
    if (filename == NULL)
    {
        filename = conf;
    }

    return filename;
}

char *Conf_localmotdfile(void)
{
    static char conf[] = CONF_LOCALMOTDFILE;

    return conf;
}

char conf_logfile_string[] = CONF_LOGFILE;

char *Conf_logfile(void)
{
    return conf_logfile_string;
}

/* needed by client/default.c */
char conf_ship_file_string[] = CONF_SHIP_FILE;

char *Conf_ship_file(void)
{
    return conf_ship_file_string;
}

/* needed by client/default.c */
char conf_texturedir_string[] = CONF_TEXTUREDIR;

char *Conf_texturedir(void)
{
    return conf_texturedir_string;
}

/* needed by client/default.c */
char conf_soundfile_string[] = CONF_SOUNDFILE;

char *Conf_soundfile(void)
{
    return conf_soundfile_string;
}

char *Conf_localguru(void)
{
    static char conf[] = CONF_LOCALGURU;

    return conf;
}

char *Conf_contactaddress(void)
{
    static char conf[] = CONF_CONTACTADDRESS;

    return conf;
}

static char conf_robotfile_string[] = CONF_ROBOTFILE;

char *Conf_robotfile(void)
{
    return conf_robotfile_string;
}

char *Conf_zcat_ext(void)
{
    static char conf[] = CONF_ZCAT_EXT;

    return conf;
}

char *Conf_zcat_format(void)
{
    static char conf[] = CONF_ZCAT_FORMAT;

    return conf;
}

char *Conf_sounddir(void)
{
    static char conf[] = CONF_SOUNDDIR;

    return conf;
}

void Conf_print(void)
{
    warn("============================================================");
    warn("VERSION                   = %s", VERSION);
    warn("PACKAGE                   = %s", PACKAGE);

#ifdef PLOCKSERVER
    warn("PLOCKSERVER");
#endif
#ifdef DEVELOPMENT
    warn("DEVELOPMENT");
#endif

    warn("Conf_localguru()          = %s", Conf_localguru());
    warn("Conf_datadir()            = %s", Conf_datadir());
    warn("Conf_defaults_file_name() = %s", Conf_defaults_file_name());
    warn("Conf_password_file_name() = %s", Conf_password_file_name());
    warn("Conf_mapdir()             = %s", Conf_mapdir());
    warn("Conf_default_map()        = %s", Conf_default_map());
    warn("Conf_servermotdfile()     = %s", Conf_servermotdfile());
    warn("Conf_robotfile()          = %s", Conf_robotfile());
    warn("Conf_logfile()            = %s", Conf_logfile());
    warn("Conf_localmotdfile()      = %s", Conf_localmotdfile());
    warn("Conf_ship_file()          = %s", Conf_ship_file());
    warn("Conf_texturedir()         = %s", Conf_texturedir());
    // warn("Conf_fontdir()            = %s", Conf_fontdir());
    warn("Conf_sounddir()           = %s", Conf_sounddir());
    warn("Conf_soundfile()          = %s", Conf_soundfile());
    warn("Conf_zcat_ext()           = %s", Conf_zcat_ext());
    warn("Conf_zcat_format()        = %s", Conf_zcat_format());
    warn("============================================================");
}
