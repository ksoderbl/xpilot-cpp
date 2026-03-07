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
#include "portability.h"
#include "checknames.h"

#include "client.h"
#include "configure.h"
#include "guimap.h"
#include "guiobjects.h"
#include "record.h"
#include "talk.h"

#include "xinit.h"
#include "xpaint.h"

// In xdefault.cpp
extern void Initialize_global_variables(void);

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

const char *Program_name(void)
{
    return "xpilot-cpp-client-x11";
}

/*
 * Oh glorious main(), without thee we cannot exist.
 */
int main(int argc, char *argv[])
{
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

    seedMT((unsigned)time(NULL) ^ Get_process_id());

    memset(conpar, 0, sizeof(Connect_param_t));

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

    // /* CLIENTRANK */
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

    Simulate(false);

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
}
