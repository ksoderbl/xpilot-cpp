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

#ifndef        PROTOCLIENT_H
#define        PROTOCLIENT_H

#ifndef TYPES_H
#include "types.h"
#endif

/*
 * default.c
 */
extern void Parse_options(int *argcp, char **argvp, char *realName, int *port,
                          int *my_team, bool *text, bool *list,
                          bool *join, bool *noLocalMotd,
                          char *nickName, char *dispName, char *hostName,
                          char *shut_msg);
extern void defaultCleanup(void);                                /* memory cleanup */

extern void Get_xpilotrc_file(char *, unsigned);

/*
 * join.c
 */
extern int Join(char *server_addr, char *server_name, int port,
                char *real, char *nick, int my_team,
                char *display, unsigned version);

/*
 * paintdata.c
 */
extern void paintdataCleanup(void);                /* memory cleanup */

/*
 * paintobjects.c
 */
extern int Init_wreckage(void);
extern int Init_asteroids(void);


/*
 * query.c
 */
#ifdef SOCKLIB_H
extern int Query_all(sock_t *sockfd, int port, char *msg, int msglen);
#endif

#ifdef        LIMIT_ACCESS
extern bool                Is_allowed(char *);
#endif

/*
 * sim.c
 */
extern void Simulate(void);

/*
 * textinterface.c
 */
#ifdef CONNECTPARAM_H
int Connect_to_server(int auto_connect, int list_servers,
                      int auto_shutdown, char *shutdown_reason,
                      Connect_param_t *conpar);
int Contact_servers(int count, char **servers,
                    int auto_connect, int list_servers,
                    int auto_shutdown, char *shutdown_message,
                    int find_max, int *num_found,
                    char **server_addresses, char **server_names,
                    unsigned *server_versions,
                    Connect_param_t *conpar);
#endif

/*
 * welcome.c
 */
#ifdef CONNECTPARAM_H
int Welcome_screen(Connect_param_t *conpar);
#endif

/*
 * widget.c
 */
void Widget_cleanup(void);

/*
 * xinit.c
 */


#endif        /* PROTOCLIENT_H */


