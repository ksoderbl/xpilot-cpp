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

#include "commonproto.h"

#include "version.h"
#include "xpconfig.h"
#include "const.h"
#include "types.h"
#include "clientpack.h"
#include "bit.h"
#include "xperror.h"
#include "socklib.h"
#include "net.h"

#include "portability.h"
#include "checknames.h"

#include "client.h"
#include "configure.h"
#include "guimap.h"
#include "guiobjects.h"
#include "record.h"
#include "talk.h"
#include "clientrank.h"

#include "xinit.h"
#include "xpaint.h"

char hostname[SOCK_HOSTNAME_LENGTH];

char **Argv;
int Argc;

static void printfile(const char *filename)
{
    FILE *fp;
    int c;

    if ((fp = fopen(filename, "r")) == nullptr)
        return;

    while ((c = fgetc(fp)) != EOF)
        putchar(c);

    fclose(fp);
}

std::string Program_name(void)
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

    debugprint("main => calling Conf_print");

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

    debugprint("main => calling init_error");

    /*
     * --- Miscellaneous initialization ---
     */
    init_error(argv[0]);

    debugprint("main => calling seed MT");

    seedMT((unsigned)time(nullptr) ^ Get_process_id());

    ConnectParam *conpar = &clientOptions.connectParam;

    debugprint("main => calling create global option array");

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

    debugprint("main => calling parse options");

    /*
     * --- Check commandline arguments and resource files ---
     */
    memset(&xpArgs, 0, sizeof(xp_args_t));
    Parse_options(&argc, argv);
    /*strcpy(clientname,clientOptions.connectParam.nick_name); */

    debugprint("main => calling config init");

    Config_init();

    debugprint("main => calling handle x options");

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

    debugprint("main => simulate");

    Simulate(false);

    if (xpArgs.text || xpArgs.auto_connect || argv[1])
    {
        debugprint("main => calling contact servers");

        if (xpArgs.list_servers)
            printf("LISTING AVAILABLE SERVERS:\n");

        result = Contact_servers(argc - 1, &argv[1],
                                 xpArgs.auto_connect, xpArgs.list_servers,
                                 auto_shutdown, xpArgs.shutdown_reason,
                                 0, nullptr, nullptr, nullptr, nullptr,
                                 conpar);
    }
    else
    {
        debugprint("main => calling welcome screen");
        result = Welcome_screen(conpar);
    }

    if (result == 1)
    {
        debugprint("main => calling join");
        retval = Join(conpar);
    }

    if (clientOptions.instruments.clientRanker)
        Print_saved_scores();

    debugprint("main => RETURNING %d", retval);

    return retval;
}
