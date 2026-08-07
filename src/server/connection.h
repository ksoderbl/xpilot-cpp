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

#pragma once

/*
 * This include file holds defines and types related to the
 * connection of a player.  It is mainly used by netserver.cpp.
 */

#include <string>

#include "net.h"
#include "shipshape.h"

/*
 * Different states a connection can be in.
 */
constexpr int CONN_FREE = 0x00;      /* free for use */
constexpr int CONN_LISTENING = 0x01; /* before connect() */
constexpr int CONN_SETUP = 0x02;     /* after verification */
constexpr int CONN_LOGIN = 0x04;     /* after setup info transferred */
constexpr int CONN_PLAYING = 0x08;   /* when actively playing */
constexpr int CONN_DRAIN = 0x20;     /* wait for all reliable data to be acked */
constexpr int CONN_READY = 0x40;     /* draining after LOGIN and before PLAYING */

/*
 * In order to not let the server be locked by a collection
 * of idle connections we timeout a client if it doesn't
 * continue with logging in in a reasonable tempo.
 * Sorry, our resources are limited.
 * But the timeout should be easily configurable.
 * The timeout specifies the number of seconds each connection
 * state may last.
 */
#define LISTEN_TIMEOUT 4
#define SETUP_TIMEOUT 15
#define LOGIN_TIMEOUT 40
#define READY_TIMEOUT 40
#define IDLE_TIMEOUT 30

/*
 * Maximum roundtrip time taken as serious for rountrip time calculations.
 */
#define MAX_RTT (FPS + 1)

/*
 * The retransmission timeout bounds in number of frames.
 */
#define MIN_RETRANSMIT (FPS / 8 + 1)
#define MAX_RETRANSMIT (FPS + 1)
#define DEFAULT_RETRANSMIT (FPS / 2)

/*
 * All the player connection state info.
 */
class connection_t
{
public:
    int ind = 0;                         /* index of connection, was conn_index */
    int state = 0;                       /* state of connection */
    int drain_state = 0;                 /* state after draining done */
    unsigned magic = 0;                  /* magic cookie */
    sockbuf_t r{};                       /* input buffer */
    sockbuf_t w{};                       /* output buffer */
    sockbuf_t c{};                       /* reliable data buffer */
    long start = 0;                      /* time of last state change */
    long timeout = 0;                    /* time when state timeouts */
    long last_send_loops = 0;            /* last update of reliable */
    long reliable_offset = 0;            /* amount of data acked */
    long reliable_unsent = 0;            /* next unsend reliable byte */
    long retransmit_at_loop = 0;         /* next retransmission time */
    int rtt_smoothed = 0;                /* smoothed roundtrip time */
    int rtt_dev = 0;                     /* roundtrip time deviation */
    int rtt_retransmit = 0;              /* retransmission time */
    int rtt_timeouts = 0;                /* how many timeouts */
    int acks = 0;                        /* good acknowledgements */
    int setup = 0;                       /* amount of setup done */
    int my_port = 0;                     /* server port for this player */
    int his_port = 0;                    /* client port for this player */
    int id = 0;                          /* index into GetIndArray[] or NO_ID */
    int team = 0;                        /* team of player */
    unsigned version = 0;                /* XPilot version of client */
    long last_key_change = 0;            /* last keyboard change */
    long talk_sequence_num = 0;          /* talk acknowledgement */
    long motd_offset = 0;                /* offset into motd or -1 */
    long motd_stop = 0;                  /* max offset into motd */
    int num_keyboard_updates = 0;        /* Keyboards in one packet */
    int view_width = 0, view_height = 0; /* Viewable area dimensions */
    int debris_colors = 0;               /* Max. debris intensities */
    int spark_rand = 0;                  /* Sparkling effect */
    int last_mouse_pos = 0;              /* value of last pointer pkt */
    std::string user;                    /* username of player */
    std::string nick;                    /* nickname of player */
    std::string dpy;                     /* display of player */
    shipshape_t *ship = nullptr;         /* ship shape of player */
    std::string addr;                    /* address of players host */
    std::string host;                    /* hostname of players host */
    int features = 0;                    /* supported features */
};
