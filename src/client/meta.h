/*
 * XPilot, a multiplayer gravity war game.  Copyright (C) 1991-2004 by
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
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "clientpack.h"
#include "socklib.h"

/*
 * max number of servers we can find on the local network.
 */
#define MAX_LOCAL_SERVERS 10

/*
 * Some constants for describing access to the meta servers.
 * XXX These are also defined in some other file.
 */
#define NUM_METAS 1
#define META_HOST "meta.xpilot.org"
#define META_HOST_TWO "meta2.xpilot.org"
#define META_IP "45.55.104.252"
#define META_IP_TWO "194.28.50.74"
#define META_PROG_PORT 4401
#define NUM_META_DATA_FIELDS 18

#define PING_UNKNOWN 10000 /* never transmitted a ping to it */
#define PING_NORESP 9999   /* never responded to our ping */
#define PING_SLOW 9998     /* responded to first ping after \
                            * we had already retried (ie slow!) */

/********************** Data Structures *********************/

/*
 * All the fields for a server in one line of meta output.
 * the strings in this structure should really be an array
 * of char pointers to reduce the amount of code.
 */
struct ServerInfo
{
    std::string version;
    std::string hostname;
    std::string users_str;
    std::string mapname;
    std::string mapsize;
    std::string author;
    std::string status;
    std::string bases_str;
    std::string fps_str;
    std::string playlist;
    std::string sound;
    std::string teambases_str;
    std::string timing;
    std::string ip_str;
    std::string freebases;
    std::string queue_str;
    std::string domain;
    std::string pingtime_str;

    unsigned port = 0;
    unsigned ip = 0;
    unsigned users = 0;
    unsigned bases = 0;
    unsigned fps = 0;
    unsigned uptime = 0;
    unsigned teambases = 0;
    unsigned queue = 0;
    unsigned pingtime = PING_UNKNOWN;
    struct timeval start{};
    uint8_t serial = 0;
};

typedef struct ServerInfo server_info_t;
using server_list_t = std::vector<server_info_t *>;
using server_list_iter_t = server_list_t::iterator;

/*
 * Here we hold the servers which are listed by the meta servers.
 * We record the time we contacted Meta so as to not overload Meta.
 * server_it is an iterator pointing at the first server for the next page.
 */
extern server_list_t server_list;
extern time_t server_list_creation_time;
extern server_list_iter_t server_it;

/*
 * States a connection to a meta server can be in.
 */
enum MetaState
{
    MetaConnecting = 0,
    MetaReadable = 1,
    MetaReceiving = 2
};

/*
 * Structure describing a meta server.
 * Hostname, IP address, and socket filedescriptor.
 */
struct Meta
{
    char name[MAX_HOST_LEN];
    char addr[16];
    sock_t sock;
    enum MetaState state; /* connecting, readable, receiving */
};

void Delete_server_list(void);
void Delete_server_info(server_info_t *sip);
void string_to_lower(char *s);
char *Get_domain_from_hostname(char *host_name);
int Welcome_sort_server_list(void);
int Add_server_info(server_info_t *sip);
char *my_strtok(char *buf, const char *sep);
void Add_meta_line(char *meta_line);
void Meta_connect(int *connections_ptr, int *maxfd_ptr);
void Meta_dns_lookup(void);
void Ping_servers(void);
int Get_meta_data(void);