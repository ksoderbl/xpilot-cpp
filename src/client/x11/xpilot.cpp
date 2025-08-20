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
#include <cstdio>
#include <cstring>
#include <cctype>
#include <cerrno>
#include <ctime>
#include <sys/types.h>

#include <unistd.h>
#include <sys/time.h>
#include <sys/param.h>
#include <netdb.h>

#include "randommt.h"
#include "strlcpy.h"

#include "client.h"
#include "colors.h"
#include "configure.h"
#include "guimap.h"
#include "guiobjects.h"
#include "record.h"
#include "talk.h"

#include "xinit.h"

#include "version.h"
#include "xpconfig.h"
#include "const.h"
#include "types.h"
#include "pack.h"
#include "bit.h"
#include "xperror.h"
#include "socklib.h"
#include "net.h"
#include "connectparam.h"
#include "protoclient.h"
#include "portability.h"
#include "checknames.h"

char hostname[SOCK_HOSTNAME_LENGTH];

char **Argv;
int Argc;

static void printfile(const char *filename)
{
    FILE *fp;
    int c;

    if ((fp = fopen(filename, "r")) == NULL)
        return;

    while ((c = fgetc(fp)) != EOF)
        putchar(c);

    fclose(fp);
}

/*
 * Oh glorious main(), without thee we cannot exist.
 */
int main(int argc, char *argv[])
{
    // int result;
    // bool auto_connect = false,
    //      text = false,
    //      list_servers = false,
    //      auto_shutdown = false,
    //      noLocalMotd = false;
    // char *cp;
    // static char shutdown_reason[MAX_CHARS];
    // Connect_param_t *conpar = &connectParam;
    int result, retval = 1;
    bool auto_shutdown = false;
    Connect_param_t *conpar = &connectParam;

    /*
     * --- Output copyright notice ---
     */
    printf("  " COPYRIGHT ".\n"
           "  " TITLE " comes with ABSOLUTELY NO WARRANTY; "
           "for details see the\n"
           "  provided COPYING file.\n\n");
    if (strcmp(Conf_localguru(), PACKAGE_BUGREPORT))
        printf("  %s is responsible for the local installation.\n\n",
               Conf_localguru());

    Conf_print();

    Argc = argc;
    Argv = argv;

    /*
     * --- Miscellaneous initialization ---
     */
    init_error(argv[0]);

    seedMT((unsigned)time((time_t *)0) ^ Get_process_id());

    memset(conpar, 0, sizeof(Connect_param_t));
    // conpar->contact_port = SERVER_PORT;
    // conpar->team = TEAM_NOT_SET;

    /*
     * --- Create global option array ---
     */
    Store_default_options();
    Store_X_options();
    Store_hud_options();
    Store_paintradar_options();
    Store_xpaint_options();
    Store_guimap_options();
    Store_guiobject_options();
    Store_talk_macro_options();
    Store_key_options();
    Store_record_options();
    Store_color_options();

    /*
     * --- Check commandline arguments and resource files ---
     */
    memset(&xpArgs, 0, sizeof(xp_args_t));
    Parse_options(&argc, argv);
    /*strcpy(clientname,connectParam.nick_name); */

    Config_init();
    Handle_X_options();

    // cp = getenv("XPILOTHOST");
    // if (cp)
    // {
    //     strlcpy(hostname, cp, sizeof(hostname));
    // }
    // else
    // {
    //     sock_get_local_hostname(hostname, sizeof hostname, 0);
    // }
    // if (Check_host_name(hostname) == NAME_ERROR)
    // {
    //     xpprintf("fixing host from \"%s\" ", hostname);
    //     Fix_host_name(hostname);
    //     xpprintf("to \"%s\"\n", hostname);
    // }

    // cp = getenv("XPILOTUSER");
    // if (cp)
    // {
    //     strlcpy(conpar->user_name, cp, sizeof(conpar->user_name));
    // }
    // else
    // {
    //     Get_login_name(conpar->user_name, sizeof(conpar->user_name) - 1);
    // }
    // if (Check_user_name(conpar->user_name) == NAME_ERROR)
    // {
    //     xpprintf("fixing name from \"%s\" ", conpar->user_name);
    //     Fix_user_name(conpar->user_name);
    //     xpprintf("to \"%s\"\n", conpar->user_name);
    // }

    /* CLIENTRANK */
    // Init_saved_scores();

    if (xpArgs.list_servers)
        xpArgs.auto_connect = true;

    if (xpArgs.shutdown_reason[0] != '\0')
    {
        auto_shutdown = true;
        xpArgs.auto_connect = true;
    }

    /*
     * --- Message of the Day ---
     */
    printfile(Conf_localmotdfile());

    if (xpArgs.text || xpArgs.auto_connect || argv[1])
    {
        if (xpArgs.list_servers)
            printf("LISTING AVAILABLE SERVERS:\n");

        result = Contact_servers(argc - 1, &argv[1],
                                 xpArgs.auto_connect, xpArgs.list_servers,
                                 auto_shutdown, xpArgs.shutdown_reason,
                                 0, NULL, NULL, NULL, NULL,
                                 conpar);
    }
    else
        result = Welcome_screen(conpar);

    if (result == 1)
        retval = Join(conpar);

    // if (instruments.clientRanker)
    //     Print_saved_scores();

    return retval;

    // /*
    //  * --- Check commandline arguments and resource files ---
    //  */
    // Parse_options(&argc, argv, conpar->user_name,
    //               &conpar->contact_port, &conpar->team,
    //               &text, &list_servers,
    //               &auto_connect, &noLocalMotd,
    //               conpar->nick_name, conpar->disp_name,
    //               hostname, shutdown_reason);
    // if (Check_nick_name(conpar->nick_name) == NAME_ERROR)
    // {
    //     xpprintf("fixing nick from \"%s\" ", conpar->nick_name);
    //     Fix_nick_name(conpar->nick_name);
    //     xpprintf("to \"%s\"\n", conpar->nick_name);
    // }

    // if (list_servers)
    // {
    //     auto_connect = true;
    // }
    // if (shutdown_reason[0] != '\0')
    // {
    //     auto_shutdown = true;
    //     auto_connect = true;
    // }

    // /*
    //  * --- Message of the Day ---
    //  */
    // if (!noLocalMotd)
    //     printfile(Conf_localmotdfile());

    // Simulate();

    // if (text || auto_connect || argv[1])
    // {
    //     if (list_servers)
    //         printf("LISTING AVAILABLE SERVERS:\n");

    //     result = Contact_servers(argc - 1, &argv[1],
    //                              auto_connect, list_servers,
    //                              auto_shutdown, shutdown_reason,
    //                              0, 0, 0, 0, 0,
    //                              conpar);
    // }
    // else
    // {
    //     result = Welcome_screen(conpar);
    // }

    // if (result == 1)
    // {
    //     return Join(conpar->server_addr, conpar->server_name, conpar->login_port,
    //                 conpar->user_name, conpar->nick_name, conpar->team,
    //                 conpar->disp_name, conpar->server_version);
    // }
    // return 1;
}
