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

/*
 * This is the server side of the network connnection stuff.
 *
 * We try very hard to not let the game be disturbed by
 * players logging in.  Therefore a new connection
 * passes through several states before it is actively
 * playing.
 * First we make a new connection structure available
 * with a new socket to listen on.  This socket port
 * number is told to the client via the pack mechanism.
 * In this state the client has to send a packet to this
 * newly created socket with its name and playing parameters.
 * If this succeeds the connection advances to its second state.
 * In this second state the essential server configuration
 * like the map and so on is transmitted to the client.
 * If the client has acknowledged all this data then it
 * advances to the third state, which is the
 * ready-but-not-playing-yet state.  In this state the client
 * has some time to do its final initializations, like mapping
 * its user interface windows and so on.
 * When the client is ready to accept frame updates and process
 * keyboard events then it sends the start-play packet.
 * This play packet advances the connection state into the
 * actively-playing state.  A player structure is allocated and
 * initialized and the other human players are told about this new player.
 * The newly started client is told about the already playing players and
 * play has begun.
 * Apart from these four states there are also two intermediate states.
 * These intermediate states are entered when the previous state
 * has filled the reliable data buffer and the client has not
 * acknowledged all the data yet that is in this reliable data buffer.
 * They are so called output drain states.  Not doing anything else
 * then waiting until the buffer is empty.
 * The difference between these two intermediate states is tricky.
 * The second intermediate state is entered after the
 * ready-but-not-playing-yet state and before the actively-playing state.
 * The difference being that in this second intermediate state the client
 * is already considered an active player by the rest of the server
 * but should not get frame updates yet until it has acknowledged its last
 * reliable data.
 *
 * Communication between the server and the clients is only done
 * using UDP datagrams.  The first client/serverized version of XPilot
 * was using TCP only, but this was too unplayable across the Internet,
 * because TCP is a data stream always sending the next byte.
 * If a packet gets lost then the server has to wait for a
 * timeout before a retransmission can occur.  This is too slow
 * for a real-time program like this game, which is more interested
 * in recent events than in sequenced/reliable events.
 * Therefore UDP is now used which gives more network control to the
 * program.
 * Because some data is considered crucial, like the names of
 * new players and so on, there also had to be a mechanism which
 * enabled reliable data transmission.  Here this is done by creating
 * a data stream which is piggybacked on top of the unreliable data
 * packets.  The client acknowledges this reliable data by sending
 * its byte position in the reliable data stream.  So if the client gets
 * a new reliable data packet and it has not had this data before and
 * there is also no data packet missing inbetween, then it advances
 * its byte position and acknowledges this new position to the server.
 * Otherwise it discards the packet and sends its old byte position
 * to the server meaning that it detected a packet loss.
 * The server maintains an acknowledgement timeout timer for each
 * connection so that it can retransmit a reliable data packet
 * if the acknowledgement timer expires.
 */

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cctype>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "draw.h"
#include "strdup.h"
#include "strlcpy.h"

#define SERVER
#include "version.h"
#include "xpconfig.h"
#include "serverconst.h"
#include "global.h"
#include "proto.h"
#include "map.h"
#include "pack.h"
#include "bit.h"
#include "types.h"
#include "socklib.h"
#include "sched.h"
#include "net.h"
#include "xperror.h"
#include "netserver.h"
#include "packet.h"
#include "setup.h"
#include "connection.h"
#include "saudio.h"
#include "checknames.h"
#include "server.h"
#include "asteroid.h"
#include "score.h"

#define MAX_SELECT_FD (sizeof(int) * 8 - 1)
#define MAX_RELIABLE_DATA_PACKET_SIZE 1024

#define MAX_MOTD_CHUNK 512
#define MAX_MOTD_SIZE (30 * 1024)
#define MAX_MOTD_LOOPS (10 * FPS)

static connection_t *Conn = NULL;
static setup_t *Setup = NULL;
static setup_t *Oldsetup = NULL;
static int max_connections = 0;
static int (*playing_receive[256])(connection_t *conn),
    (*login_receive[256])(connection_t *conn),
    (*drain_receive[256])(connection_t *conn);
int compress_maps = 1;
int login_in_progress;
static int num_logins, num_logouts;

static int Compress_map(uint8_t *map, int size);
static int Init_setup(void);
static int Handle_listening(connection_t *conn);
static int Handle_setup(connection_t *conn);
static int Handle_login(connection_t *conn, char *errmsg, int errsize);
static void Handle_input(int fd, void *arg);

static int Receive_keyboard(connection_t *conn);
static int Receive_quit(connection_t *conn);
static int Receive_play(connection_t *conn);
static int Receive_power(connection_t *conn);
static int Receive_ack(connection_t *conn);
static int Receive_ack_cannon(connection_t *conn);
static int Receive_ack_fuel(connection_t *conn);
static int Receive_ack_target(connection_t *conn);
static int Receive_discard(connection_t *conn);
static int Receive_undefined(connection_t *conn);
static int Receive_talk(connection_t *conn);
static int Receive_display(connection_t *conn);
static int Receive_modifier_bank(connection_t *conn);
static int Receive_motd(connection_t *conn);
static int Receive_shape(connection_t *conn);
static int Receive_pointer_move(connection_t *conn);
static int Receive_audio_request(connection_t *conn);
static int Receive_fps_request(connection_t *conn);

static int Send_motd(connection_t *conn);

/*
 * Compress the map data using a simple Run Length Encoding algorithm.
 * If there is more than one consecutive byte with the same type
 * then we set the high bit of the byte and then the next byte
 * gives the number of repetitions.
 * This works well for most maps which have lots of series of the
 * same map object and is simple enough to got implemented quickly.
 */
static int Compress_map(uint8_t *map, int size)
{
    int i, j, k;

    for (i = j = 0; i < size; i++, j++)
    {
        if (i + 1 < size && map[i] == map[i + 1])
        {
            for (k = 2; i + k < size; k++)
            {
                if (map[i] != map[i + k])
                {
                    break;
                }
                if (k == 255)
                {
                    break;
                }
            }
            map[j] = (map[i] | SETUP_COMPRESSED);
            map[++j] = k;
            i += k - 1;
        }
        else
        {
            map[j] = map[i];
        }
    }
    return j;
}

/*
 * Initialize the structure that gives the client information
 * about our setup.  Like the map and playing rules.
 * We only setup this structure once to save time when new
 * players log in during play.
 */
static int Init_setup(void)
{
    int i, x, y, team, type, size,
        wormhole = 0,
        treasure = 0,
        target = 0,
        base = 0,
        cannon = 0;
    uint8_t *mapdata, *mapptr;

    if ((mapdata = (uint8_t *)malloc(world->x * world->y)) == NULL)
    {
        error("No memory for mapdata");
        return -1;
    }
    memset(mapdata, SETUP_SPACE, world->x * world->y);
    mapptr = mapdata;
    errno = 0;
    for (x = 0; x < world->x; x++)
    {
        for (y = 0; y < world->y; y++, mapptr++)
        {
            type = world->block[x][y];
            switch (type)
            {
            case ACWISE_GRAV:
            case CWISE_GRAV:
            case POS_GRAV:
            case NEG_GRAV:
            case UP_GRAV:
            case DOWN_GRAV:
            case RIGHT_GRAV:
            case LEFT_GRAV:
                if (!options.gravityVisible)
                    type = SPACE;
                break;
            case WORMHOLE:
                if (!options.wormholeVisible)
                    type = SPACE;
                break;
            case ITEM_CONCENTRATOR:
                if (!options.itemConcentratorVisible)
                    type = SPACE;
                break;
            case ASTEROID_CONCENTRATOR:
                if (!options.asteroidConcentratorVisible)
                    type = SPACE;
                break;
            case FRICTION:
                if (!options.blockFrictionVisible)
                    type = SPACE;
                else
                    type = DECOR_FILLED;
                break;
            default:
                break;
            }
            switch (type)
            {
            case SPACE:
                *mapptr = SETUP_SPACE;
                break;
            case FILLED:
                *mapptr = SETUP_FILLED;
                break;
            case REC_RU:
                *mapptr = SETUP_REC_RU;
                break;
            case REC_RD:
                *mapptr = SETUP_REC_RD;
                break;
            case REC_LU:
                *mapptr = SETUP_REC_LU;
                break;
            case REC_LD:
                *mapptr = SETUP_REC_LD;
                break;
            case FUEL:
                *mapptr = SETUP_FUEL;
                break;
            case ACWISE_GRAV:
                *mapptr = SETUP_ACWISE_GRAV;
                break;
            case CWISE_GRAV:
                *mapptr = SETUP_CWISE_GRAV;
                break;
            case POS_GRAV:
                *mapptr = SETUP_POS_GRAV;
                break;
            case NEG_GRAV:
                *mapptr = SETUP_NEG_GRAV;
                break;
            case UP_GRAV:
                *mapptr = SETUP_UP_GRAV;
                break;
            case DOWN_GRAV:
                *mapptr = SETUP_DOWN_GRAV;
                break;
            case RIGHT_GRAV:
                *mapptr = SETUP_RIGHT_GRAV;
                break;
            case LEFT_GRAV:
                *mapptr = SETUP_LEFT_GRAV;
                break;
            case ITEM_CONCENTRATOR:
                *mapptr = SETUP_ITEM_CONCENTRATOR;
                break;
            case ASTEROID_CONCENTRATOR:
                *mapptr = SETUP_ASTEROID_CONCENTRATOR;
                break;
            case DECOR_FILLED:
                *mapptr = SETUP_DECOR_FILLED;
                break;
            case DECOR_RU:
                *mapptr = SETUP_DECOR_RU;
                break;
            case DECOR_RD:
                *mapptr = SETUP_DECOR_RD;
                break;
            case DECOR_LU:
                *mapptr = SETUP_DECOR_LU;
                break;
            case DECOR_LD:
                *mapptr = SETUP_DECOR_LD;
                break;
            case WORMHOLE:
                switch (world->wormHoles[wormhole++].type)
                {
                case WORM_NORMAL:
                    *mapptr = SETUP_WORM_NORMAL;
                    break;
                case WORM_IN:
                    *mapptr = SETUP_WORM_IN;
                    break;
                case WORM_OUT:
                    *mapptr = SETUP_WORM_OUT;
                    break;
                default:
                    error("Bad wormhole (%d,%d).", x, y);
                    free(mapdata);
                    return -1;
                }
                break;
            case TREASURE:
                *mapptr = SETUP_TREASURE + world->treasures[treasure++].team;
                break;
            case TARGET:
                *mapptr = SETUP_TARGET + world->targets[target++].team;
                break;
            case BASE:
                if (world->base[base].team == TEAM_NOT_SET)
                {
                    team = 0;
                }
                else
                {
                    team = world->base[base].team;
                }
                switch (world->base[base++].dir)
                {
                case DIR_UP:
                    *mapptr = SETUP_BASE_UP + team;
                    break;
                case DIR_RIGHT:
                    *mapptr = SETUP_BASE_RIGHT + team;
                    break;
                case DIR_DOWN:
                    *mapptr = SETUP_BASE_DOWN + team;
                    break;
                case DIR_LEFT:
                    *mapptr = SETUP_BASE_LEFT + team;
                    break;
                default:
                    error("Bad base at (%d,%d).", x, y);
                    free(mapdata);
                    return -1;
                }
                break;
            case CANNON:
                switch (world->cannon[cannon++].dir)
                {
                case DIR_UP:
                    *mapptr = SETUP_CANNON_UP;
                    break;
                case DIR_RIGHT:
                    *mapptr = SETUP_CANNON_RIGHT;
                    break;
                case DIR_DOWN:
                    *mapptr = SETUP_CANNON_DOWN;
                    break;
                case DIR_LEFT:
                    *mapptr = SETUP_CANNON_LEFT;
                    break;
                default:
                    error("Bad cannon at (%d,%d).", x, y);
                    free(mapdata);
                    return -1;
                }
                break;
            case CHECK:
                for (i = 0; i < world->NumChecks; i++)
                {
                    if (x != world->check[i].x || y != world->check[i].y)
                    {
                        continue;
                    }
                    *mapptr = SETUP_CHECK + i;
                    break;
                }
                if (i >= world->NumChecks)
                {
                    error("Bad checkpoint at (%d,%d).", x, y);
                    free(mapdata);
                    return -1;
                }
                break;
            default:
                error("Unknown map type (%d) at (%d,%d).", type, x, y);
                *mapptr = SETUP_SPACE;
                break;
            }
        }
    }
    if (compress_maps == 0)
    {
        type = SETUP_MAP_UNCOMPRESSED;
        size = world->x * world->y;
    }
    else
    {
        type = SETUP_MAP_ORDER_XY;
        size = Compress_map(mapdata, world->x * world->y);
        if (size <= 0 || size > world->x * world->y)
        {
            errno = 0;
            error("Map compression error (%d)", size);
            free(mapdata);
            return -1;
        }
        if ((mapdata = (uint8_t *)realloc(mapdata, size)) == NULL)
        {
            error("Cannot reallocate mapdata");
            return -1;
        }
    }

#ifndef SILENT
    if (type != SETUP_MAP_UNCOMPRESSED)
    {
        xpprintf("%s Map compression ratio is %-4.2f%%\n", showtime(),
                 100.0 * size / (world->x * world->y));
    }
#endif
    if ((Setup = (setup_t *)malloc(sizeof(setup_t) + size)) == NULL)
    {
        error("No memory to hold setup");
        free(mapdata);
        return -1;
    }
    memset(Setup, 0, sizeof(setup_t) + size);
    memcpy(Setup->map_data, mapdata, size);
    free(mapdata);
    Setup->setup_size = ((char *)&Setup->map_data[0] - (char *)Setup) + size;
    Setup->map_data_len = size;
    Setup->map_order = type;
    Setup->frames_per_second = FPS;
    Setup->lives = world->rules->lives;
    Setup->mode = world->rules->mode;
    Setup->x = world->x;
    Setup->y = world->y;
    strlcpy(Setup->name, world->name, sizeof(Setup->name));
    strlcpy(Setup->author, world->author, sizeof(Setup->author));

    return 0;
}

/*
 * Initialize the function dispatch tables for the various client
 * connection states.  Some states use the same table.
 */
static void Init_receive(void)
{
    int i;

    for (i = 0; i < 256; i++)
    {
        login_receive[i] = Receive_undefined;
        playing_receive[i] = Receive_undefined;
        drain_receive[i] = Receive_undefined;
    }

    drain_receive[PKT_QUIT] = Receive_quit;
    drain_receive[PKT_ACK] = Receive_ack;
    drain_receive[PKT_VERIFY] = Receive_discard;
    drain_receive[PKT_PLAY] = Receive_discard;
    drain_receive[PKT_SHAPE] = Receive_discard;

    login_receive[PKT_PLAY] = Receive_play;
    login_receive[PKT_QUIT] = Receive_quit;
    login_receive[PKT_ACK] = Receive_ack;
    login_receive[PKT_VERIFY] = Receive_discard;
    login_receive[PKT_POWER] = Receive_power;
    login_receive[PKT_POWER_S] = Receive_power;
    login_receive[PKT_TURNSPEED] = Receive_power;
    login_receive[PKT_TURNSPEED_S] = Receive_power;
    login_receive[PKT_TURNRESISTANCE] = Receive_power;
    login_receive[PKT_TURNRESISTANCE_S] = Receive_power;
    login_receive[PKT_DISPLAY] = Receive_display;
    login_receive[PKT_MODIFIERBANK] = Receive_modifier_bank;
    login_receive[PKT_MOTD] = Receive_motd;
    login_receive[PKT_SHAPE] = Receive_shape;
    login_receive[PKT_REQUEST_AUDIO] = Receive_audio_request;
    login_receive[PKT_ASYNC_FPS] = Receive_fps_request;

    playing_receive[PKT_ACK] = Receive_ack;
    playing_receive[PKT_VERIFY] = Receive_discard;
    playing_receive[PKT_PLAY] = Receive_play;
    playing_receive[PKT_QUIT] = Receive_quit;
    playing_receive[PKT_KEYBOARD] = Receive_keyboard;
    playing_receive[PKT_POWER] = Receive_power;
    playing_receive[PKT_POWER_S] = Receive_power;
    playing_receive[PKT_TURNSPEED] = Receive_power;
    playing_receive[PKT_TURNSPEED_S] = Receive_power;
    playing_receive[PKT_TURNRESISTANCE] = Receive_power;
    playing_receive[PKT_TURNRESISTANCE_S] = Receive_power;
    playing_receive[PKT_ACK_CANNON] = Receive_ack_cannon;
    playing_receive[PKT_ACK_FUEL] = Receive_ack_fuel;
    playing_receive[PKT_ACK_TARGET] = Receive_ack_target;
    playing_receive[PKT_TALK] = Receive_talk;
    playing_receive[PKT_DISPLAY] = Receive_display;
    playing_receive[PKT_MODIFIERBANK] = Receive_modifier_bank;
    playing_receive[PKT_MOTD] = Receive_motd;
    playing_receive[PKT_SHAPE] = Receive_shape;
    playing_receive[PKT_POINTER_MOVE] = Receive_pointer_move;
    playing_receive[PKT_REQUEST_AUDIO] = Receive_audio_request;
    playing_receive[PKT_ASYNC_FPS] = Receive_fps_request;
}

/*
 * Initialize the connection structures.
 */
int Setup_net_server(void)
{
    size_t size;

    Init_receive();

    if (Init_setup() == -1)
    {
        return -1;
    }
    /*
     * The number of connections is limited by the number of bases
     * and the max number of possible file descriptors to use in
     * the select(2) call minus those for stdin, stdout, stderr,
     * the contact socket, and the socket for the resolver library routines.
     */
    max_connections = MIN(MAX_SELECT_FD - 5, world->NumBases);
    size = max_connections * sizeof(*Conn);
    if ((Conn = (connection_t *)malloc(size)) == NULL)
    {
        error("Cannot allocate memory for connections");
        return -1;
    }
    memset(Conn, 0, size);

    return 0;
}

static void Conn_set_state(connection_t *conn, int state, int drain_state)
{
    static int num_conn_busy;
    static int num_conn_playing;

    if ((conn->state & (CONN_PLAYING | CONN_READY)) != 0)
    {
        num_conn_playing--;
    }
    else if (conn->state == CONN_FREE)
    {
        num_conn_busy++;
    }

    conn->state = state;
    conn->drain_state = drain_state;
    conn->start = main_loops;

    if (conn->state == CONN_PLAYING)
    {
        num_conn_playing++;
        conn->timeout = IDLE_TIMEOUT;
    }
    else if (conn->state == CONN_READY)
    {
        num_conn_playing++;
        conn->timeout = READY_TIMEOUT;
    }
    else if (conn->state == CONN_LOGIN)
    {
        conn->timeout = LOGIN_TIMEOUT;
    }
    else if (conn->state == CONN_SETUP)
    {
        conn->timeout = SETUP_TIMEOUT;
    }
    else if (conn->state == CONN_LISTENING)
    {
        conn->timeout = LISTEN_TIMEOUT;
    }
    else if (conn->state == CONN_FREE)
    {
        num_conn_busy--;
        conn->timeout = IDLE_TIMEOUT;
    }

    login_in_progress = num_conn_busy - num_conn_playing;
}

/*
 * Cleanup a connection.  The client may not know yet that
 * it is thrown out of the game so we send it a quit packet.
 * We send it twice because of UDP it could get lost.
 * Since 3.0.6 the client receives a short message
 * explaining why the connection was terminated.
 */
void Destroy_connection(connection_t *conn, const char *reason)
{
    int id,
        len;
    sock_t *sock;
    char pkt[MAX_CHARS];

    if (conn->state == CONN_FREE)
    {
        errno = 0;
        error("Cannot destroy empty connection (\"%s\")", reason);
        return;
    }

    sock = &conn->w.sock;
    remove_input(sock->fd);

    pkt[0] = PKT_QUIT;
    strlcpy(&pkt[1], reason, sizeof(pkt) - 1);
    len = strlen(pkt) + 1;
    if (sock_write(sock, pkt, len) != len)
    {
        sock_get_error(sock);
        sock_write(sock, pkt, len);
    }
#ifndef SILENT
    xpprintf("%s Goodbye %s=%s@%s|%s (\"%s\")\n",
             showtime(),
             conn->nick ? conn->nick : "",
             conn->real ? conn->real : "",
             conn->host ? conn->host : "",
             conn->dpy ? conn->dpy : "",
             reason);
#endif

    Conn_set_state(conn, CONN_FREE, CONN_FREE);

    if (conn->id != NO_ID)
    {
        id = conn->id;
        conn->id = NO_ID;
        Players[GetInd[id]]->conn = NULL;
        Delete_player(GetInd[id]);
    }
    if (conn->real != NULL)
    {
        free(conn->real);
    }
    if (conn->nick != NULL)
    {
        free(conn->nick);
    }
    if (conn->dpy != NULL)
    {
        free(conn->dpy);
    }
    if (conn->addr != NULL)
    {
        free(conn->addr);
    }
    if (conn->host != NULL)
    {
        free(conn->host);
    }
    Sockbuf_cleanup(&conn->w);
    Sockbuf_cleanup(&conn->r);
    Sockbuf_cleanup(&conn->c);

    num_logouts++;

    if (sock_write(sock, pkt, len) != len)
    {
        sock_get_error(sock);
        sock_write(sock, pkt, len);
    }
    sock_close(sock);

    memset(conn, 0, sizeof(*conn));
}

int Check_connection(char *real, char *nick, char *dpy, char *addr)
{
    int i;
    connection_t *conn;

    for (i = 0; i < max_connections; i++)
    {
        conn = &Conn[i];
        if (conn->state == CONN_LISTENING)
        {
            if (strcasecmp(conn->nick, nick) == 0)
            {
                if (!strcmp(real, conn->real) && !strcmp(dpy, conn->dpy) && !strcmp(addr, conn->addr))
                {
                    return conn->my_port;
                }
                return -1;
            }
        }
    }
    return -1;
}

/*
 * A client has requested a playing connection with this server.
 * See if we have room for one more player and if his name is not
 * already in use by some other player.  Because the confirmation
 * may get lost we are willing to send it another time if the
 * client connection is still in the CONN_LISTENING state.
 */
int Setup_connection(char *real, char *nick, char *dpy, int team,
                     char *addr, char *host, unsigned version)
{
    int i,
        free_conn_index = max_connections,
        my_port;
    sock_t sock;
    connection_t *conn;

    for (i = 0; i < max_connections; i++)
    {
        conn = &Conn[i];
        if (conn->state == CONN_FREE)
        {
            if (free_conn_index == max_connections)
            {
                free_conn_index = i;
            }
            continue;
        }
        if (strcasecmp(conn->nick, nick) == 0)
        {
            if (conn->state == CONN_LISTENING && strcmp(real, conn->real) == 0 && strcmp(dpy, conn->dpy) == 0 && version == conn->version)
            {
                /*
                 * May happen for multi-homed hosts
                 * and if previous packet got lost.
                 */
                return conn->my_port;
            }
            else
            {
                /*
                 * Nick already in use.
                 */
                return -1;
            }
        }
    }

    if (free_conn_index >= max_connections)
    {
#ifndef SILENT
        xpprintf("%s Full house for %s(%s)@%s(%s)\n",
                 showtime(), real, nick, host, dpy);
#endif
        return -1;
    }
    conn = &Conn[free_conn_index];
    conn->conn_index = free_conn_index;
    if (options.clientPortStart && (!options.clientPortEnd || options.clientPortEnd > 65535))
    {
        options.clientPortEnd = 65535;
    }
    if (options.clientPortEnd && (!options.clientPortStart || options.clientPortStart < 1024))
    {
        options.clientPortStart = 1024;
    }

    if (!options.clientPortStart || !options.clientPortEnd ||
        (options.clientPortStart > options.clientPortEnd))
    {

        if (sock_open_udp(&sock, serverAddr, 0) == SOCK_IS_ERROR)
        {
            error("Cannot create datagram socket (%d)", sock.error.error);
            return -1;
        }
    }
    else
    {
        int found_socket = 0;
        for (i = options.clientPortStart; i <= options.clientPortEnd; i++)
        {
            if (sock_open_udp(&sock, serverAddr, i) != SOCK_IS_ERROR)
            {
                found_socket = 1;
                break;
            }
        }
        if (found_socket == 0)
        {
            error("Could not find a usable port in given port range");
            return -1;
        }
    }

    if ((my_port = sock_get_port(&sock)) == -1)
    {
        error("Cannot get port from socket");
        sock_close(&sock);
        return -1;
    }
    if (sock_set_non_blocking(&sock, 1) == -1)
    {
        error("Cannot make client socket non-blocking");
        sock_close(&sock);
        return -1;
    }
    if (sock_set_receive_buffer_size(&sock, SERVER_RECV_SIZE + 256) == -1)
    {
        error("Cannot set receive buffer size to %d", SERVER_RECV_SIZE + 256);
    }
    if (sock_set_send_buffer_size(&sock, SERVER_SEND_SIZE + 256) == -1)
    {
        error("Cannot set send buffer size to %d", SERVER_SEND_SIZE + 256);
    }

    Sockbuf_init(&conn->w, &sock, SERVER_SEND_SIZE,
                 SOCKBUF_WRITE | SOCKBUF_DGRAM);

    Sockbuf_init(&conn->r, &sock, SERVER_RECV_SIZE,
                 SOCKBUF_READ | SOCKBUF_DGRAM);

    Sockbuf_init(&conn->c, (sock_t *)NULL, MAX_SOCKBUF_SIZE,
                 SOCKBUF_WRITE | SOCKBUF_READ | SOCKBUF_LOCK);

    conn->my_port = my_port;
    conn->real = xp_strdup(real);
    conn->nick = xp_strdup(nick);
    conn->dpy = xp_strdup(dpy);
    conn->addr = xp_strdup(addr);
    conn->host = xp_strdup(host);
    conn->ship = NULL;
    conn->team = team;
    conn->version = version;
    conn->start = main_loops;
    conn->magic = randomMT() + my_port + sock.fd + team + main_loops;
    conn->id = NO_ID;
    conn->timeout = LISTEN_TIMEOUT;
    conn->last_key_change = 0;
    conn->reliable_offset = 0;
    conn->reliable_unsent = 0;
    conn->last_send_loops = 0;
    conn->retransmit_at_loop = 0;
    conn->rtt_retransmit = DEFAULT_RETRANSMIT;
    conn->rtt_smoothed = 0;
    conn->rtt_dev = 0;
    conn->rtt_timeouts = 0;
    conn->acks = 0;
    conn->setup = 0;
    conn->motd_offset = -1;
    conn->motd_stop = 0;
    conn->view_width = DEF_VIEW_SIZE;
    conn->view_height = DEF_VIEW_SIZE;
    conn->debris_colors = 0;
    conn->spark_rand = DEF_SPARK_RAND;
    Conn_set_state(conn, CONN_LISTENING, CONN_FREE);
    if (conn->w.buf == NULL || conn->r.buf == NULL || conn->c.buf == NULL || conn->real == NULL || conn->nick == NULL || conn->dpy == NULL || conn->addr == NULL || conn->host == NULL)
    {
        error("Not enough memory for connection");
        /* socket is not yet connected, but it doesn't matter much. */
        Destroy_connection(conn, "no memory");
        return -1;
    }

    install_input(Handle_input, sock.fd, conn);

    return my_port;
}

/*
 * Handle a connection that is in the listening state.
 */
static int Handle_listening(connection_t *conn)
{
    uint8_t type;
    int n;
    char nick[MAX_CHARS],
        real[MAX_CHARS];

    if (conn->state != CONN_LISTENING)
    {
        Destroy_connection(conn, "not listening");
        return -1;
    }
    Sockbuf_clear(&conn->r);
    errno = 0;
    n = sock_receive_any(&conn->r.sock, conn->r.buf, conn->r.size);
    if (n <= 0)
    {
        if (n == 0 || errno == EWOULDBLOCK || errno == EAGAIN)
        {
            n = 0;
        }
        else if (n != 0)
        {
            Destroy_connection(conn, "read first packet error");
        }
        return n;
    }
    conn->r.len = n;
    conn->his_port = sock_get_last_port(&conn->r.sock);
    if (sock_connect(&conn->w.sock, conn->addr, conn->his_port) == -1)
    {
        error("Cannot connect datagram socket (%s,%d,%d,%d,%d)",
              conn->addr, conn->his_port,
              conn->w.sock.error.error,
              conn->w.sock.error.call,
              conn->w.sock.error.line);
        if (sock_get_error(&conn->w.sock))
        {
            error("sock_get_error fails too, giving up");
            Destroy_connection(conn, "connect error");
            return -1;
        }
        errno = 0;
        if (sock_connect(&conn->w.sock, conn->addr, conn->his_port) == -1)
        {
            error("Still cannot connect datagram socket (%s,%d,%d,%d,%d)",
                  conn->addr, conn->his_port,
                  conn->w.sock.error.error,
                  conn->w.sock.error.call,
                  conn->w.sock.error.line);
            Destroy_connection(conn, "connect error");
            return -1;
        }
    }
#ifndef SILENT
    xpprintf("%s Welcome %s=%s@%s|%s (%s/%d)", showtime(), conn->nick,
             conn->real, conn->host, conn->dpy, conn->addr, conn->his_port);
    if (conn->version != MY_VERSION)
    {
        xpprintf(" (version %04x)\n", conn->version);
    }
    else
    {
        xpprintf("\n");
    }
#endif
    if (conn->r.ptr[0] != PKT_VERIFY)
    {
        Send_reply(conn, PKT_VERIFY, PKT_FAILURE);
        Send_reliable(conn);
        Destroy_connection(conn, "not connecting");
        return -1;
    }
    if ((n = Packet_scanf(&conn->r, "%c%s%s",
                          &type, real, nick)) <= 0)
    {
        Send_reply(conn, PKT_VERIFY, PKT_FAILURE);
        Send_reliable(conn);
        Destroy_connection(conn, "verify incomplete");
        return -1;
    }
    Fix_user_name(real);
    Fix_nick_name(nick);
    if (strcmp(real, conn->real))
    {
#ifndef SILENT
        xpprintf("%s Client verified incorrectly (%s,%s)(%s,%s)\n",
                 showtime(), real, nick, conn->real, conn->nick);
#endif
        Send_reply(conn, PKT_VERIFY, PKT_FAILURE);
        Send_reliable(conn);
        Destroy_connection(conn, "verify incorrect");
        return -1;
    }
    Sockbuf_clear(&conn->w);
    if (Send_reply(conn, PKT_VERIFY, PKT_SUCCESS) == -1 || Packet_printf(&conn->c, "%c%u", PKT_MAGIC, conn->magic) <= 0 || Send_reliable(conn) <= 0)
    {
        Destroy_connection(conn, "confirm failed");
        return -1;
    }

    Conn_set_state(conn, CONN_DRAIN, CONN_SETUP);

    return 1; /* success! */
}

/*
 * Handle a connection that is in the transmit-server-configuration-data state.
 */
static int Handle_setup(connection_t *conn)
{
    char *buf;
    int n,
        len;

    if (conn->state != CONN_SETUP)
    {
        Destroy_connection(conn, "not setup");
        return -1;
    }

    if (conn->setup == 0)
    {
        n = Packet_printf(&conn->c,
                          "%ld"
                          "%ld%hd"
                          "%hd%hd"
                          "%hd%hd"
                          "%s%s",
                          Setup->map_data_len,
                          Setup->mode, Setup->lives,
                          Setup->x, Setup->y,
                          Setup->frames_per_second, Setup->map_order,
                          Setup->name, Setup->author);
        if (n <= 0)
        {
            Destroy_connection(conn, "setup 0 write error");
            return -1;
        }
        conn->setup = (char *)&Setup->map_data[0] - (char *)Setup;
    }
    else if (conn->setup < Setup->setup_size)
    {
        if (conn->c.len > 0)
        {
            /* If there is still unacked reliable data test for acks. */
            Handle_input(-1, (void *)conn);
            if (conn->state == CONN_FREE)
            {
                return -1;
            }
        }
    }
    if (conn->setup < Setup->setup_size)
    {
        len = MIN(conn->c.size, 4096) - conn->c.len;
        if (len <= 0)
        {
            /* Wait for acknowledgement of previously transmitted data. */
            return 0;
        }
        if (len > Setup->setup_size - conn->setup)
        {
            len = Setup->setup_size - conn->setup;
        }
        buf = (char *)Setup;
        if (Sockbuf_write(&conn->c, &buf[conn->setup], len) != len)
        {
            Destroy_connection(conn, "sockbuf write setup error");
            return -1;
        }
        conn->setup += len;
        if (len >= 512)
        {
            conn->start += (len * FPS) / (8 * 512) + 1;
        }
    }
    if (conn->setup >= Setup->setup_size)
    {
        Conn_set_state(conn, CONN_DRAIN, CONN_LOGIN);
    }

    return 0;
}

/*
 * A client has requested to start active play.
 * See if we can allocate a player structure for it
 * and if this succeeds update the player information
 * to all connected players.
 */
static int Handle_login(connection_t *conn, char *errmsg, int errsize)
{
    player_t *pl;
    int i,
        war_on_id,
        conn_bit;
    char msg[MSG_LEN];

    if (NumPlayers - NumPseudoPlayers >= world->NumBases)
    {
        errno = 0;
        strlcpy(errmsg, "Not enough bases for players", errsize);
        error("%s", errmsg);
        return -1;
    }
    if (BIT(world->rules->mode, TEAM_PLAY))
    {
        if (conn->team < 0 || conn->team >= MAX_TEAMS || (options.reserveRobotTeam && (conn->team == options.robotTeam)))
        {
            conn->team = TEAM_NOT_SET;
        }
        else if (world->teams[conn->team].NumBases <= 0)
        {
            conn->team = TEAM_NOT_SET;
        }
        else
        {
            Check_team_members(conn->team);
            if (world->teams[conn->team].NumMembers - world->teams[conn->team].NumRobots >= world->teams[conn->team].NumBases)
            {
                conn->team = TEAM_NOT_SET;
            }
        }
        if (conn->team == TEAM_NOT_SET)
        {
            conn->team = Pick_team(PickForHuman);
            if (conn->team == TEAM_NOT_SET ||
                (conn->team == options.robotTeam && options.reserveRobotTeam))
            {
                errno = 0;
                strlcpy(errmsg, "Can't pick team", errsize);
                error("%s", errmsg);
                return -1;
            }
        }
    }
    else
    {
        conn->team = TEAM_NOT_SET;
    }
    for (i = 0; i < NumPlayers; i++)
    {
        if (strcasecmp(Players[i]->name, conn->nick) == 0)
        {
            warn("Name already in use %s", conn->nick);
            strlcpy(errmsg, "Name already in use", errsize);
            return -1;
        }
    }
    if (Init_player(NumPlayers, conn->ship) <= 0)
    {
        strlcpy(errmsg, "Init_player failed: no free ID", errsize);
        return -1;
    }
    pl = Players[NumPlayers];
    strlcpy(pl->name, conn->nick, MAX_CHARS);
    strlcpy(pl->realname, conn->real, MAX_CHARS);
    strlcpy(pl->hostname, conn->host, MAX_CHARS);
    pl->isowner = (!strcmp(pl->realname, Server.owner) &&
                   !strcmp(conn->addr, "127.0.0.1"));
    if (conn->team != TEAM_NOT_SET)
        pl->team = conn->team;
    pl->version = conn->version;

    Pick_startpos(NumPlayers);
    Go_home(NumPlayers);
    if (pl->team != TEAM_NOT_SET)
    {
        world->teams[pl->team].NumMembers++;
    }
    NumPlayers++;
    request_ID();
    conn->id = pl->id;
    pl->conn = conn;
    memset(pl->last_keyv, 0, sizeof(pl->last_keyv));
    memset(pl->prev_keyv, 0, sizeof(pl->prev_keyv));

    Conn_set_state(conn, CONN_READY, CONN_PLAYING);

    if (Send_reply(conn, PKT_PLAY, PKT_SUCCESS) <= 0)
    {
        strlcpy(errmsg, "Cannot send play reply", errsize);
        error("%s", errmsg);
        return -1;
    }

#ifndef SILENT
    xpprintf("%s %s (%d) starts at startpos %d.\n", showtime(),
             pl->name, NumPlayers, pl->home_base);
#endif

    /*
     * Tell him about himself first.
     */
    Send_player(pl->conn, pl->id);
    Send_score(pl->conn, pl->id, pl->score,
               pl->life, pl->mychar, pl->alliance);
    Send_base(pl->conn, pl->id, pl->home_base);
    /*
     * And tell him about all the others.
     */
    for (i = 0; i < NumPlayers - 1; i++)
    {
        Send_player(pl->conn, Players[i]->id);
        Send_score(pl->conn, Players[i]->id, Players[i]->score,
                   Players[i]->life, Players[i]->mychar, Players[i]->alliance);
        if (!IS_TANK_IND(i))
        {
            Send_base(pl->conn, Players[i]->id, Players[i]->home_base);
        }
    }
    /*
     * And tell all the others about him.
     */
    for (i = 0; i < NumPlayers - 1; i++)
    {
        if (Players[i]->conn != NULL)
        {
            Send_player(Players[i]->conn, pl->id);
            Send_score(Players[i]->conn, pl->id, pl->score,
                       pl->life, pl->mychar, pl->alliance);
            Send_base(Players[i]->conn, pl->id, pl->home_base);
        }
        /*
         * And tell him about the relationships others have with eachother.
         */
        else if (IS_ROBOT_IND(i))
        {
            if ((war_on_id = Robot_war_on_player(i)) != NO_ID)
            {
                Send_war(pl->conn, Players[i]->id, war_on_id);
            }
        }
    }

    if (NumPlayers == 1)
    {
        sprintf(msg, "Welcome to \"%s\", made by %s.",
                world->name, world->author);
    }
    else if (BIT(world->rules->mode, TEAM_PLAY))
    {
        sprintf(msg, "%s (%s, team %d) has entered \"%s\", made by %s.",
                pl->name, pl->realname, pl->team, world->name, world->author);
    }
    else
    {
        sprintf(msg, "%s (%s) has entered \"%s\", made by %s.",
                pl->name, pl->realname, world->name, world->author);
    }
    Set_message(msg);

    if (conn->version < MY_VERSION)
    {
        const char sender[] = "[*Server notice*]";
        sprintf(msg, "Server runs version %s. %s", VERSION, sender);
        Set_player_message(pl, msg);
        if (conn->version < 0x4401)
        {
            printf("THIS NEVER HAPPENS: fdjskafjdfadkasj\n");
            sprintf(msg,
                    "Your client does not support the fast radar packet. %s",
                    sender);
            Set_player_message(pl, msg);
        }
        if (conn->version < 0x4400 && options.maxAsteroidDensity > 0)
        {
            printf("THIS NEVER HAPPENS: 2jclajvkjafkdjsakfj894\n");
            sprintf(msg,
                    "Your client will see the %d asteroids as balls. %s",
                    (int)world->asteroids.max,
                    sender);
            Set_player_message(pl, msg);
        }
    }

    conn_bit = (1 << conn->conn_index);
    for (i = 0; i < world->NumCannons; i++)
    {
        /*
         * The client assumes at startup that all cannons are active.
         */
        if (world->cannon[i].dead_time == 0)
        {
            SET_BIT(world->cannon[i].conn_mask, conn_bit);
        }
        else
        {
            CLR_BIT(world->cannon[i].conn_mask, conn_bit);
        }
    }
    for (i = 0; i < world->NumFuels; i++)
    {
        /*
         * The client assumes at startup that all fuelstations are filled.
         */
        if (world->fuel[i].fuel == MAX_STATION_FUEL)
        {
            SET_BIT(world->fuel[i].conn_mask, conn_bit);
        }
        else
        {
            CLR_BIT(world->fuel[i].conn_mask, conn_bit);
        }
    }
    for (i = 0; i < world->NumTargets; i++)
    {
        /*
         * The client assumes at startup that all targets are not damaged.
         */
        if (world->targets[i].dead_time == 0 && world->targets[i].damage == TARGET_DAMAGE)
        {
            SET_BIT(world->targets[i].conn_mask, conn_bit);
            CLR_BIT(world->targets[i].update_mask, conn_bit);
        }
        else
        {
            CLR_BIT(world->targets[i].conn_mask, conn_bit);
            SET_BIT(world->targets[i].update_mask, conn_bit);
        }
    }

    sound_player_init(pl);

    sound_play_all(START_SOUND);

    num_logins++;

    if (options.resetOnHuman > 0 && (NumPlayers - NumPseudoPlayers - NumRobots) <= options.resetOnHuman)
    {
        if (BIT(world->rules->mode, TIMING))
            Race_game_over();
        else if (BIT(world->rules->mode, TEAM_PLAY))
            Team_game_over(-1, "");
        else if (BIT(world->rules->mode, LIMITED_LIVES))
            Individual_game_over(-1);
    }

    if (NumPlayers == 1)
    {
        if (options.maxRoundTime > 0)
            roundtime = options.maxRoundTime * FPS;
        else
            roundtime = -1;
        sprintf(msg, "Player entered. Delaying 0 seconds until next %s.",
                (BIT(world->rules->mode, TIMING) ? "race" : "round"));
        Set_message(msg);
    }

    return 0;
}

/*
 * Process a client packet.
 * The client may be in one of several states,
 * therefore we use function dispatch tables for easy processing.
 * Some functions may process requests from clients being
 * in different states.
 */
static void Handle_input(int fd, void *arg)
{
    connection_t *conn = (connection_t *)arg;
    int type,
        result,
        (**receive_tbl)(connection_t *conn);

    if (conn->state & (CONN_PLAYING | CONN_READY))
    {
        receive_tbl = &playing_receive[0];
    }
    else if (conn->state == CONN_LOGIN)
    {
        receive_tbl = &login_receive[0];
    }
    else if (conn->state & (CONN_DRAIN | CONN_SETUP))
    {
        receive_tbl = &drain_receive[0];
    }
    else if (conn->state == CONN_LISTENING)
    {
        Handle_listening(conn);
        return;
    }
    else
    {
        if (conn->state != CONN_FREE)
        {
            Destroy_connection(conn, "not input");
        }
        return;
    }
    conn->num_keyboard_updates = 0;

    Sockbuf_clear(&conn->r);
    if (Sockbuf_read(&conn->r) == -1)
    {
        Destroy_connection(conn, "input error");
        return;
    }
    if (conn->r.len <= 0)
    {
        /*
         * No input.
         */
        return;
    }
    while (conn->r.ptr < conn->r.buf + conn->r.len)
    {
        type = (conn->r.ptr[0] & 0xFF);
        result = (*receive_tbl[type])(conn);
        if (result == -1)
        {
            /*
             * Unrecoverable error.
             * Connection has been destroyed.
             */
            return;
        }
        if (result == 0)
        {
            /*
             * Incomplete client packet.
             * Drop rest of packet.
             */
            Sockbuf_clear(&conn->r);
            break;
        }
        if (conn->state == CONN_PLAYING)
        {
            conn->start = main_loops;
        }
    }
}

int Input(void)
{
    int i,
        ind,
        num_reliable = 0,
        input_reliable[MAX_SELECT_FD];
    connection_t *conn;
    char msg[MSG_LEN];

    for (i = 0; i < max_connections; i++)
    {
        conn = &Conn[i];
        if (conn->state == CONN_FREE)
        {
            continue;
        }
        if (conn->start + conn->timeout * FPS < main_loops)
        {
            /*
             * Timeout this fellow if we have not heard a single thing
             * from him for a long time.
             */
            if (conn->state & (CONN_PLAYING | CONN_READY))
            {
                sprintf(msg, "%s mysteriously disappeared!?", conn->nick);
                Set_message(msg);
            }
            sprintf(msg, "timeout %02x", conn->state);
            Destroy_connection(conn, msg);
            continue;
        }
        if (conn->state != CONN_PLAYING)
        {
            input_reliable[num_reliable++] = i;
            if (conn->state == CONN_SETUP)
            {
                Handle_setup(conn);
                continue;
            }
        }
    }

    for (i = 0; i < num_reliable; i++)
    {
        ind = input_reliable[i];
        conn = &Conn[ind];
        if (conn->state & (CONN_DRAIN | CONN_READY | CONN_SETUP | CONN_LOGIN))
        {
            if (conn->c.len > 0)
            {
                if (Send_reliable(conn) == -1)
                {
                    continue;
                }
            }
        }
    }

    if (num_logins | num_logouts)
    {
        /* Tell the meta server */
        Meta_update(1);
        num_logins = 0;
        num_logouts = 0;
    }

    return login_in_progress;
}

/*
 * Send a reply to a special client request.
 * Not used consistently everywhere.
 * It could be used to setup some form of reliable
 * communication from the client to the server.
 */
int Send_reply(connection_t *conn, int replyto, int result)
{
    int n;

    n = Packet_printf(&conn->c, "%c%c%c", PKT_REPLY, replyto, result);
    if (n == -1)
    {
        Destroy_connection(conn, "write error");
        return -1;
    }
    return n;
}

static int Send_modifiers(connection_t *conn, char *mods)
{
    return Packet_printf(&conn->w, "%c%s", PKT_MODIFIERS, mods);
}

/*
 * Send items.
 * The advantage of this scheme is that it only uses bytes for items
 * which the player actually owns.  This reduces the packet size.
 * Another advantage is that here it doesn't matter if an old client
 * receives counts for items it doesn't know about.
 * This is new since pack version 4203.
 */
static int Send_self_items(connection_t *conn, player_t *pl)
{
    unsigned item_mask = 0;
    int i, n;
    int item_count = 0;

    /* older clients should have the items sent as part of the self packet. */
    if (conn->version < 0x4203)
    {
        printf("THIS NEVER HAPPENS: fdjgoi3jjgkaij\n");
        return 1;
    }
    /* build mask with one bit for each item type which the player owns. */
    for (i = 0; i < NUM_ITEMS; i++)
    {
        if (pl->item[i] > 0)
        {
            item_mask |= (1 << i);
            item_count++;
        }
    }
    /* don't send anything if there are no items. */
    if (item_count == 0)
    {
        return 1;
    }
    /* check if enough buffer space is available for the complete packet. */
    if (conn->w.size - conn->w.len <= 5 + item_count)
    {
        return 0;
    }
    /* build the header. */
    n = Packet_printf(&conn->w, "%c%u", PKT_SELF_ITEMS, item_mask);
    if (n <= 0)
    {
        return n;
    }
    /* build rest of packet containing the per item counts. */
    for (i = 0; i < NUM_ITEMS; i++)
    {
        if (item_mask & (1 << i))
        {
            conn->w.buf[conn->w.len++] = pl->item[i];
        }
    }
    /* return the number of bytes added to the packet. */
    return 5 + item_count;
}

/*
 * Send all frame data related to the player self and his HUD.
 */
int Send_self(connection_t *conn,
              player_t *pl,
              int lock_id,
              int lock_dist,
              int lock_dir,
              int autopilotlight,
              long status,
              char *mods)
{
    int n;
    uint8_t stat = (uint8_t)status;
    int sbuf_len = conn->w.len;

    if (conn->version >= 0x4203)
    {
        n = Packet_printf(&conn->w,
                          "%c"
                          "%hd%hd%hd%hd%c"
                          "%c%c%c"
                          "%hd%hd%c%c"
                          "%c%hd%hd"
                          "%hd%hd%c"
                          "%c%c",
                          PKT_SELF,
                          (int)(pl->pos.x + 0.5), (int)(pl->pos.y + 0.5),
                          (int)pl->vel.x, (int)pl->vel.y,
                          pl->dir,
                          (int)(pl->power + 0.5),
                          (int)(pl->turnspeed + 0.5),
                          (int)(pl->turnresistance * 255.0 + 0.5),
                          lock_id, lock_dist, lock_dir,
                          pl->check,

                          pl->fuel.current,
                          pl->fuel.sum >> FUEL_SCALE_BITS,
                          pl->fuel.max >> FUEL_SCALE_BITS,

                          conn->view_width, conn->view_height,
                          conn->debris_colors,

                          stat,
                          autopilotlight

        );
        if (n <= 0)
        {
            return n;
        }
        n = Send_self_items(conn, pl);
        if (n <= 0)
        {
            return n;
        }
        return Send_modifiers(conn, mods);
    }

    n = Packet_printf(&conn->w,
                      "%c"
                      "%hd%hd%hd%hd%c"
                      "%c%c%c"
                      "%hd%hd%c%c"
                      "%c%c%c%c%c"
                      "%c%c%c%c%c"
                      "%c%c%c%c"
                      "%c%hd%hd"
                      "%hd%hd%c"
                      "%c%c",
                      PKT_SELF,
                      (int)(pl->pos.x + 0.5), (int)(pl->pos.y + 0.5),
                      (int)pl->vel.x, (int)pl->vel.y,
                      pl->dir,
                      (int)(pl->power + 0.5),
                      (int)(pl->turnspeed + 0.5),
                      (int)(pl->turnresistance * 255.0 + 0.5),
                      lock_id, lock_dist, lock_dir,
                      pl->check,

                      pl->item[ITEM_CLOAK],
                      pl->item[ITEM_SENSOR],
                      pl->item[ITEM_MINE],
                      pl->item[ITEM_MISSILE],
                      pl->item[ITEM_ECM],

                      pl->item[ITEM_TRANSPORTER],
                      pl->item[ITEM_WIDEANGLE],
                      pl->item[ITEM_REARSHOT],
                      pl->item[ITEM_AFTERBURNER],
                      pl->fuel.num_tanks,

                      pl->item[ITEM_LASER],
                      pl->item[ITEM_EMERGENCY_THRUST],
                      pl->item[ITEM_TRACTOR_BEAM],
                      pl->item[ITEM_AUTOPILOT],

                      pl->fuel.current,
                      pl->fuel.sum >> FUEL_SCALE_BITS,
                      pl->fuel.max >> FUEL_SCALE_BITS,

                      conn->view_width, conn->view_height,
                      conn->debris_colors,

                      stat,
                      autopilotlight

    );
    if (n <= 0)
    {
        return n;
    }
    if (conn->version >= 0x3800)
    {
        n = Packet_printf(&conn->w,
                          "%c%c%c%c", /* %c", */
                          pl->item[ITEM_EMERGENCY_SHIELD],
                          pl->item[ITEM_DEFLECTOR],
                          pl->item[ITEM_HYPERJUMP],
                          pl->item[ITEM_PHASING] /* ,
                          pl->item[ITEM_MIRROR] */
        );
        if (n <= 0)
        {
            conn->w.len = sbuf_len;
            return n;
        }
        if (conn->version >= 0x4100)
        {
            n = Packet_printf(&conn->w,
                              "%c",
                              pl->item[ITEM_MIRROR]);
            if (n <= 0)
            {
                conn->w.len = sbuf_len;
                return n;
            }
            if (conn->version >= 0x4201)
            {
                n = Packet_printf(&conn->w,
                                  "%c",
                                  pl->item[ITEM_ARMOR]);
                if (n <= 0)
                {
                    conn->w.len = sbuf_len;
                    return n;
                }
            }
        }
    }
    else if (conn->version >= 0x3200)
    {
        n = Packet_printf(&conn->w,
                          "%c",
                          pl->item[ITEM_EMERGENCY_SHIELD]);
        if (n <= 0)
        {
            conn->w.len = sbuf_len;
            return n;
        }
    }

    return Send_modifiers(conn, mods);
}

/*
 * Somebody is leaving the game.
 */
int Send_leave(connection_t *conn, int id)
{
    if (!BIT(conn->state, CONN_PLAYING | CONN_READY))
    {
        errno = 0;
        error("Connection not ready for leave info (%d,%d)",
              conn->state, conn->id);
        return 0;
    }
    return Packet_printf(&conn->c, "%c%hd", PKT_LEAVE, id);
}

/*
 * Somebody is declaring war.
 */
int Send_war(connection_t *conn, int robot_id, int killer_id)
{
    if (!BIT(conn->state, CONN_PLAYING | CONN_READY))
    {
        errno = 0;
        error("Connection not ready for war declaration (%d,%d)",
              conn->state, conn->id);
        return 0;
    }
    return Packet_printf(&conn->c, "%c%hd%hd", PKT_WAR,
                         robot_id, killer_id);
}

/*
 * Somebody is programming a robot to seek some player.
 */
int Send_seek(connection_t *conn, int programmer_id, int robot_id, int sought_id)
{
    if (!BIT(conn->state, CONN_PLAYING | CONN_READY))
    {
        errno = 0;
        error("Connection not ready for seek declaration (%d,%d)",
              conn->state, conn->id);
        return 0;
    }
    return Packet_printf(&conn->c, "%c%hd%hd%hd", PKT_SEEK,
                         programmer_id, robot_id, sought_id);
}

/*
 * Somebody is joining the game.
 */
int Send_player(connection_t *conn, int id)
{
    player_t *pl = Players[GetInd[id]];
    int n;
    char buf[MSG_LEN], ext[MSG_LEN];
    int sbuf_len = conn->c.len;

    if (!BIT(conn->state, CONN_PLAYING | CONN_READY))
    {
        errno = 0;
        error("Connection not ready for player info (%d,%d)",
              conn->state, conn->id);
        return 0;
    }
    Convert_ship_2_string(pl->ship, buf, ext, 0x3200);
    n = Packet_printf(&conn->c,
                      "%c%hd"
                      "%c%c"
                      "%s%s%s"
                      "%S",
                      PKT_PLAYER, pl->id,
                      pl->team, pl->mychar,
                      pl->name, pl->realname, pl->hostname,
                      buf);
    if (conn->version > 0x3200)
    {
        if (n > 0)
        {
            n = Packet_printf(&conn->c, "%S", ext);
            if (n <= 0)
            {
                conn->c.len = sbuf_len;
            }
        }
    }
    return n;
}

/*
 * Send the new score for some player to a client.
 */
int Send_score(connection_t *conn, int id, int score,
               int life, int mychar, int alliance)
{
    if (!BIT(conn->state, CONN_PLAYING | CONN_READY))
    {
        errno = 0;
        error("Connection not ready for score(%d,%d)",
              conn->state, conn->id);
        return 0;
    }
    if (conn->version < 0x4500)
    {
        printf("THIS NEVER HAPPENS: 2tkjgfkljadfjsjafj\n");
        /* older clients don't get alliance info or decimals of the score */
        return Packet_printf(&conn->c, "%c%hd%hd%hd%c", PKT_SCORE,
                             id, (int)(score + (score > 0 ? 0.5 : -0.5)),
                             life, mychar);
    }
    else
    {
        int allchar = ' ';
        if (alliance != ALLIANCE_NOT_SET)
        {
            if (options.announceAlliances)
            {
                allchar = alliance + '0';
            }
            else
            {
                if (Players[GetInd[conn->id]]->alliance == alliance)
                    allchar = '+';
            }
        }
        return Packet_printf(&conn->c, "%c%hd%d%hd%c%c", PKT_SCORE, id,
                             (int)(score * 100 + (score > 0 ? 0.5 : -0.5)),
                             life, mychar, allchar);
    }
}

/*
 * Send the new score for some team to a client.
 */
int Send_team_score(connection_t *conn, int team, int score)
{
    if (!BIT(conn->state, CONN_PLAYING | CONN_READY))
    {
        errno = 0;
        error("Connection not ready for team score(%d,%d)",
              conn->state, conn->id);
        return 0;
    }
    if (conn->version < 0x4500)
    {
        printf("THIS NEVER HAPPENS: rtjgjkjfjui3j\n");
        /* older clients don't know about team scores */
        return 0;
    }
    return Packet_printf(&conn->c, "%c%hd%d", PKT_TEAM_SCORE,
                         team, (int)(score * 100 + (score > 0 ? 0.5 : -0.5)));
}

/*
 * Send the new race info for some player to a client.
 */
int Send_timing(connection_t *conn, int id, int check, int round)
{
    if (conn->version < 0x3261)
    {
        printf("THIS NEVER HAPPENS: jkljklasjfdkjasf\n");
        return 1;
    }
    if (!BIT(conn->state, CONN_PLAYING | CONN_READY))
    {
        errno = 0;
        error("Connection not ready for timing(%d,%d)",
              conn->state, conn->id);
        return 0;
    }
    return Packet_printf(&conn->c, "%c%hd%hu", PKT_TIMING,
                         id, round * MAX_CHECKS + check);
}

/*
 * Send info about a player having which base.
 */
int Send_base(connection_t *conn, int id, int num)
{
    if (!BIT(conn->state, CONN_PLAYING | CONN_READY))
    {
        errno = 0;
        error("Connection not ready for base info (%d,%d)",
              conn->state, conn->id);
        return 0;
    }
    return Packet_printf(&conn->c, "%c%hd%hu", PKT_BASE, id, num);
}

/*
 * Send the amount of fuel in a fuelstation.
 */
int Send_fuel(connection_t *conn, int num, int fuel)
{
    return Packet_printf(&conn->w, "%c%hu%hu", PKT_FUEL,
                         num, fuel >> FUEL_SCALE_BITS);
}

int Send_score_object(connection_t *conn, int score, int x, int y, const char *string)
{
    if (!BIT(conn->state, CONN_PLAYING | CONN_READY))
    {
        errno = 0;
        error("Connection not ready for base info (%d,%d)",
              conn->state, conn->id);
        return 0;
    }
    if (conn->version < 0x4500)
    {
        printf("THIS NEVER HAPPENS: vgklaj4u20\n");
        /* older clients don't get decimals of the score */
        return Packet_printf(&conn->c, "%c%hd%hu%hu%s", PKT_SCORE_OBJECT,
                             (int)(score + (score > 0 ? 0.5 : -0.5)),
                             x, y, string);
    }
    else
    {
        return Packet_printf(&conn->c, "%c%d%hu%hu%s", PKT_SCORE_OBJECT,
                             (int)(score * 100 + (score > 0 ? 0.5 : -0.5)),
                             x, y, string);
    }
}

int Send_cannon(connection_t *conn, int num, int dead_time)
{
    return Packet_printf(&conn->w, "%c%hu%hu", PKT_CANNON,
                         num, dead_time);
}

int Send_destruct(connection_t *conn, int count)
{
    return Packet_printf(&conn->w, "%c%hd", PKT_DESTRUCT, count);
}

int Send_shutdown(connection_t *conn, int count, int delay)
{
    return Packet_printf(&conn->w, "%c%hd%hd", PKT_SHUTDOWN,
                         count, delay);
}

int Send_thrusttime(connection_t *conn, int count, int max)
{
    return Packet_printf(&conn->w, "%c%hd%hd", PKT_THRUSTTIME, count, max);
}

int Send_shieldtime(connection_t *conn, int count, int max)
{
    return Packet_printf(&conn->w, "%c%hd%hd", PKT_SHIELDTIME, count, max);
}

int Send_phasingtime(connection_t *conn, int count, int max)
{
    return Packet_printf(&conn->w, "%c%hd%hd", PKT_PHASINGTIME, count, max);
}

int Send_debris(connection_t *conn, int type, uint8_t *p, int n)
{
    int avail;
    sockbuf_t *w = &conn->w;

    if ((n & 0xFF) != n)
    {
        errno = 0;
        error("Bad number of debris %d", n);
        return 0;
    }
    avail = w->size - w->len - SOCKBUF_WRITE_SPARE - 2;
    if (n * 2 >= avail)
    {
        if (avail > 2)
        {
            n = (avail - 1) / 2;
        }
        else
        {
            return 0;
        }
    }
    w->buf[w->len++] = PKT_DEBRIS + type;
    w->buf[w->len++] = n;
    memcpy(&w->buf[w->len], p, n * 2);
    w->len += n * 2;

    return n;
}

int Send_wreckage(connection_t *conn, int x, int y, uint8_t wrtype, uint8_t size, uint8_t rot)
{
    if (options.wreckageCollisionMayKill && conn->version > 0x4201)
    {
        /* Set the highest bit when wreckage is deadly. */
        wrtype |= 0x80;
    }
    else
    {
        wrtype &= ~0x80;
    }

    return Packet_printf(&conn->w, "%c%hd%hd%c%c%c", PKT_WRECKAGE,
                         x, y, wrtype, size, rot);
}

int Send_asteroid(connection_t *conn, int x, int y, uint8_t type, uint8_t size, uint8_t rot)
{
    uint8_t type_size;

    if (conn->version < 0x4400)
    {
        printf("THIS NEVER HAPPENS: 2kjrk32jkr43j\n");
        return Send_ecm(conn, x, y, 2 * (int)ASTEROID_RADIUS(size));
    }

    type_size = ((type & 0x0F) << 4) | (size & 0x0F);

    return Packet_printf(&conn->w, "%c%hd%hd%c%c", PKT_ASTEROID,
                         x, y, type_size, rot);
}

int Send_fastshot(connection_t *conn, int type, uint8_t *p, int n)
{
    int avail;
    sockbuf_t *w = &conn->w;

    if ((n & 0xFF) != n)
    {
        errno = 0;
        error("Bad number of fastshot %d", n);
        return 0;
    }
    avail = w->size - w->len - SOCKBUF_WRITE_SPARE - 3;
    if (n * 2 >= avail)
    {
        if (avail > 2)
        {
            n = (avail - 1) / 2;
        }
        else
        {
            return 0;
        }
    }
    w->buf[w->len++] = PKT_FASTSHOT;
    w->buf[w->len++] = type;
    w->buf[w->len++] = n;
    memcpy(&w->buf[w->len], p, n * 2);
    w->len += n * 2;

    return n;
}

int Send_missile(connection_t *conn, int x, int y, int len, int dir)
{
    return Packet_printf(&conn->w, "%c%hd%hd%c%c",
                         PKT_MISSILE, x, y, len, dir);
}

int Send_ball(connection_t *conn, int x, int y, int id)
{
    return Packet_printf(&conn->w, "%c%hd%hd%hd", PKT_BALL, x, y, id);
}

int Send_mine(connection_t *conn, int x, int y, int teammine, int id)
{
    return Packet_printf(&conn->w, "%c%hd%hd%c%hd", PKT_MINE, x, y,
                         teammine, id);
}

int Send_target(connection_t *conn, int num, int dead_time, int damage)
{
    return Packet_printf(&conn->w, "%c%hu%hu%hu", PKT_TARGET,
                         num, dead_time, damage);
}

int Send_wormhole(connection_t *conn, int x, int y)
{
    if (conn->version < 0x4501)
    {
        printf("THIS NEVER HAPPENS: my3jklqj34j3\n");
        const int wormStep = 5;
        int wormAngle = (frame_loops & 7) * (RES / 8);

        return Send_ecm(conn,
                        x,
                        y,
                        BLOCK_SZ - 2) +
               Send_ecm(conn,
                        (int)(x + wormStep * tcos(wormAngle)),
                        (int)(y + wormStep * tsin(wormAngle)),
                        BLOCK_SZ - 2 - 2 * wormStep) +
               Send_ecm(conn,
                        (int)(x + 2 * wormStep * tcos(wormAngle)),
                        (int)(y + 2 * wormStep * tsin(wormAngle)),
                        BLOCK_SZ - 2 - 4 * wormStep);
    }
    return Packet_printf(&conn->w, "%c%hd%hd", PKT_WORMHOLE, x, y);
}

int Send_item(connection_t *conn, int x, int y, int type)
{
    if (type >= ITEM_EMERGENCY_SHIELD)
    {
        if (conn->version < 0x3200)
        {
            printf("THIS NEVER HAPPENS: bvkjoij23oij32\n");
            return 1;
        }
    }
    return Packet_printf(&conn->w, "%c%hd%hd%c", PKT_ITEM, x, y, type);
}

int Send_paused(connection_t *conn, int x, int y, int count)
{
    return Packet_printf(&conn->w, "%c%hd%hd%hd", PKT_PAUSED, x, y, count);
}

int Send_ecm(connection_t *conn, int x, int y, int size)
{
    return Packet_printf(&conn->w, "%c%hd%hd%hd", PKT_ECM, x, y, size);
}

int Send_trans(connection_t *conn, int x1, int y1, int x2, int y2)
{
    return Packet_printf(&conn->w, "%c%hd%hd%hd%hd",
                         PKT_TRANS, x1, y1, x2, y2);
}

int Send_ship(connection_t *conn, int x, int y, int id, int dir,
              bool shield, bool cloak, bool emergency_shield, bool phased, bool deflector)
{
    uint8_t flags =
        (static_cast<uint8_t>(shield) << 0) |
        (static_cast<uint8_t>(cloak) << 1) |
        (static_cast<uint8_t>(emergency_shield) << 2) |
        (static_cast<uint8_t>(phased) << 3) |
        (static_cast<uint8_t>(deflector) << 4);

    return Packet_printf(&conn->w,
                         "%c%hd%hd%hd"
                         "%c"
                         "%c",
                         PKT_SHIP, x, y, id,
                         dir,
                         flags);
}

int Send_refuel(connection_t *conn, int x0, int y0, int x1, int y1)
{
    return Packet_printf(&conn->w,
                         "%c%hd%hd%hd%hd",
                         PKT_REFUEL, x0, y0, x1, y1);
}

int Send_connector(connection_t *conn, int x0, int y0, int x1, int y1, int tractor)
{
    return Packet_printf(&conn->w,
                         "%c%hd%hd%hd%hd%c",
                         PKT_CONNECTOR, x0, y0, x1, y1, tractor);
}

int Send_laser(connection_t *conn, int color, int x, int y, int len, int dir)
{
    return Packet_printf(&conn->w, "%c%c%hd%hd%hd%c", PKT_LASER,
                         color, x, y, len, dir);
}

int Send_radar(connection_t *conn, int x, int y, int size)
{
    /* Only since 4.2.1 can clients correctly handle teammates in green. */
    /* Except the original patch from kth.se was 4.1.0 "experimental 1" */
    if (conn->version < 0x4210 && conn->version != 0x4101)
    {
        printf("THIS NEVER HAPPENS: jtj4kj3kl4j3k4j\n");
        size &= ~0x80;
    }
    return Packet_printf(&conn->w, "%c%hd%hd%c", PKT_RADAR, x, y, size);
}

int Send_fastradar(connection_t *conn, uint8_t *buf, int n)
{
    int avail;
    sockbuf_t *w = &conn->w;

    if ((n & 0xFF) != n)
    {
        errno = 0;
        error("Bad number of fastradar %d", n);
        return 0;
    }
    avail = w->size - w->len - SOCKBUF_WRITE_SPARE - 3;
    if (n * 3 >= avail)
    {
        if (avail > 3)
        {
            n = (avail - 2) / 3;
        }
        else
        {
            return 0;
        }
    }
    w->buf[w->len++] = PKT_FASTRADAR;
    w->buf[w->len++] = (uint8_t)(n & 0xFF);
    memcpy(&w->buf[w->len], buf, n * 3);
    w->len += n * 3;

    return (2 + (n * 3));
}

int Send_damaged(connection_t *conn, int damaged)
{
    return Packet_printf(&conn->w, "%c%c", PKT_DAMAGED, damaged);
}

int Send_audio(connection_t *conn, int type, int vol)
{
    if (conn->w.size - conn->w.len <= 32)
    {
        return 0;
    }
    return Packet_printf(&conn->w, "%c%c%c", PKT_AUDIO, type, vol);
}

int Send_time_left(connection_t *conn, long sec)
{
    return Packet_printf(&conn->w, "%c%ld", PKT_TIME_LEFT, sec);
}

int Send_eyes(connection_t *conn, int id)
{
    return Packet_printf(&conn->w, "%c%hd", PKT_EYES, id);
}

int Send_message(connection_t *conn, const char *msg)
{
    if (!BIT(conn->state, CONN_PLAYING | CONN_READY))
    {
        errno = 0;
        error("Connection not ready for message (%d,%d)",
              conn->state, conn->id);
        return 0;
    }
    return Packet_printf(&conn->c, "%c%S", PKT_MESSAGE, msg);
}

int Send_loseitem(int lose_item_index, connection_t *conn)
{
    if (conn->version < 0x3400)
    { /* this should never hit since */
        /* only a 3.4+ client would send */
        /* the loseitem key */
        printf("THIS NEVER HAPPENS: 3mklj3kl54j\n");
        return 1;
    }
    return Packet_printf(&conn->w, "%c%c", PKT_LOSEITEM, lose_item_index);
}

int Send_start_of_frame(connection_t *conn)
{
    if (conn->state != CONN_PLAYING)
    {
        if (conn->state != CONN_READY)
        {
            errno = 0;
            error("Connection not ready for frame (%d,%d)",
                  conn->state, conn->id);
        }
        return -1;
    }
    /*
     * We tell the client which frame number this is and
     * which keyboard update we have last received.
     */
    Sockbuf_clear(&conn->w);
    if (Packet_printf(&conn->w,
                      "%c%ld%ld",
                      PKT_START, frame_loops, conn->last_key_change) <= 0)
    {
        Destroy_connection(conn, "write error");
        return -1;
    }

    /* Return ok */
    return 0;
}

int Send_end_of_frame(connection_t *conn)
{
    int n;

    last_packet_of_frame = 1;
    n = Packet_printf(&conn->w, "%c%ld", PKT_END, frame_loops);
    last_packet_of_frame = 0;
    if (n == -1)
    {
        Destroy_connection(conn, "write error");
        return -1;
    }
    if (n == 0)
    {
        /*
         * Frame update size exceeded buffer size.
         * Drop this packet.
         */
        Sockbuf_clear(&conn->w);
        return 0;
    }
    while (conn->motd_offset >= 0 && conn->c.len + conn->w.len < MAX_RELIABLE_DATA_PACKET_SIZE)
    {
        Send_motd(conn);
    }
    if (conn->c.len > 0 && conn->w.len < MAX_RELIABLE_DATA_PACKET_SIZE)
    {
        if (Send_reliable(conn) == -1)
        {
            return -1;
        }
        if (conn->w.len == 0)
        {
            return 1;
        }
    }
    if (Sockbuf_flush(&conn->w) == -1)
    {
        Destroy_connection(conn, "flush error");
        return -1;
    }
    Sockbuf_clear(&conn->w);
    return 0;
}

static int Receive_keyboard(connection_t *conn)
{
    player_t *pl;
    long change;
    uint8_t ch;
    int size = KEYBOARD_SIZE;

    if (conn->version < 0x3800)
    {
        printf("THIS NEVER HAPPENS: 2nkj23kj4j2k342k\n");
        /* older servers have a keyboard_size of 8 bytes instead of 9. */
        size--;
    }
    if (conn->r.ptr - conn->r.buf + size + 1 + 4 > conn->r.len)
    {
        /*
         * Incomplete client packet.
         */
        return 0;
    }
    Packet_scanf(&conn->r, "%c%ld", &ch, &change);
    if (change <= conn->last_key_change)
    {
        /*
         * We already have this key.
         * Nothing to do.
         */
        conn->r.ptr += size;
    }
    else
    {
        conn->last_key_change = change;
        pl = Players[GetInd[conn->id]];
        memcpy(pl->last_keyv, conn->r.ptr, size);
        conn->r.ptr += size;
        Handle_keyboard(GetInd[conn->id]);
    }
    if (conn->num_keyboard_updates++ && (conn->state & CONN_PLAYING))
    {
        Destroy_connection(conn, "no macros");
        return -1;
    }

    return 1;
}

static int Receive_quit(connection_t *conn)
{
    Destroy_connection(conn, "client quit");

    return -1;
}

static int Receive_play(connection_t *conn)
{
    uint8_t ch;
    int n;
    char errmsg[MAX_CHARS];

    if ((n = Packet_scanf(&conn->r, "%c", &ch)) != 1)
    {
        errno = 0;
        error("Cannot receive play packet");
        Destroy_connection(conn, "receive error");
        return -1;
    }
    if (ch != PKT_PLAY)
    {
        errno = 0;
        error("Packet is not of play type");
        Destroy_connection(conn, "not play");
        return -1;
    }
    if (conn->state != CONN_LOGIN)
    {
        if (conn->state != CONN_PLAYING)
        {
            if (conn->state == CONN_READY)
            {
                conn->r.ptr = conn->r.buf + conn->r.len;
                return 0;
            }
            errno = 0;
            error("Connection not in login state (%02x)", conn->state);
            Destroy_connection(conn, "not login");
            return -1;
        }
        if (Send_reliable(conn) == -1)
        {
            return -1;
        }
        return 0;
    }
    Sockbuf_clear(&conn->w);
    strlcpy(errmsg, "login failed", sizeof(errmsg));
    if (Handle_login(conn, errmsg, sizeof(errmsg)) == -1)
    {
        Destroy_connection(conn, errmsg);
        return -1;
    }

    return 2;
}

static int Receive_power(connection_t *conn)
{
    player_t *pl;
    uint8_t ch;
    short tmp;
    int n;
    DFLOAT power;
    int autopilot;

    if ((n = Packet_scanf(&conn->r, "%c%hd", &ch, &tmp)) <= 0)
    {
        if (n == -1)
        {
            Destroy_connection(conn, "read error");
        }
        return n;
    }
    power = (DFLOAT)tmp / 256.0F;
    pl = Players[GetInd[conn->id]];
    autopilot = BIT(pl->used, HAS_AUTOPILOT);
    /* old client are going to send autopilot-mangled data, ignore it */
    if (autopilot && pl->version < 0x4200)
        return 1;

    switch (ch)
    {
    case PKT_POWER:
        if (autopilot)
            pl->auto_power_s = power;
        else
            pl->power = power;
        break;
    case PKT_POWER_S:
        pl->power_s = power;
        break;
    case PKT_TURNSPEED:
        if (autopilot)
            pl->auto_turnspeed_s = power;
        else
            pl->turnspeed = power;
        break;
    case PKT_TURNSPEED_S:
        pl->turnspeed_s = power;
        break;
    case PKT_TURNRESISTANCE:
        if (autopilot)
            pl->auto_turnresistance_s = power;
        else
            pl->turnresistance = power;
        break;
    case PKT_TURNRESISTANCE_S:
        pl->turnresistance_s = power;
        break;
    default:
        errno = 0;
        error("Not a power packet (%d,%02x)", ch, conn->state);
        Destroy_connection(conn, "not power");
        return -1;
    }
    return 1;
}

/*
 * Send the reliable data.
 * If the client is in the receive-frame-updates state then
 * all reliable data is piggybacked at the end of the
 * frame update packets.  (Except maybe for the MOTD data, which
 * could be transmitted in its own packets since MOTDs can be big.)
 * Otherwise if the client is not actively playing yet then
 * the reliable data is sent in its own packets since there
 * is no other data to combine it with.
 *
 * This thing still is not finished, but it works better than in 3.0.0 I hope.
 */
int Send_reliable(connection_t *conn)
{
    char *read_buf;
    int i,
        n,
        len,
        todo,
        max_todo;
    long rel_off;
    const int max_packet_size = MAX_RELIABLE_DATA_PACKET_SIZE,
              min_send_size = 1; /* was 4 in 3.0.7, 1 in 3.1.0 */

    if (conn->c.len <= 0 || conn->last_send_loops == main_loops)
    {
        conn->last_send_loops = main_loops;
        return 0;
    }
    read_buf = conn->c.buf;
    max_todo = conn->c.len;
    rel_off = conn->reliable_offset;
    if (conn->w.len > 0)
    {
        /* We are piggybacking on a frame update. */
        if (conn->w.len >= max_packet_size - min_send_size)
        {
            /* Frame already too big */
            return 0;
        }
        if (max_todo > max_packet_size - conn->w.len)
        {
            /* Do not exceed minimum fragment size. */
            max_todo = max_packet_size - conn->w.len;
        }
    }
    if (conn->retransmit_at_loop > main_loops)
    {
        /*
         * It is no time to retransmit yet.
         */
        if (max_todo <= conn->reliable_unsent - conn->reliable_offset + min_send_size || conn->w.len == 0)
        {
            /*
             * And we cannot send anything new either
             * and we do not want to introduce a new packet.
             */
            return 0;
        }
    }
    else if (conn->retransmit_at_loop != 0)
    {
        /*
         * Timeout.
         * Either our packet or the acknowledgement got lost,
         * so retransmit.
         */
        conn->acks >>= 1;
    }

    todo = max_todo;
    for (i = 0; i <= conn->acks && todo > 0; i++)
    {
        len = (todo > max_packet_size) ? max_packet_size : todo;
        if (Packet_printf(&conn->w, "%c%hd%ld%ld", PKT_RELIABLE,
                          len, rel_off, main_loops) <= 0 ||
            Sockbuf_write(&conn->w, read_buf, len) != len)
        {
            error("Cannot write reliable data");
            Destroy_connection(conn, "write error");
            return -1;
        }
        if ((n = Sockbuf_flush(&conn->w)) < len)
        {
            if (n == 0 && (errno == EWOULDBLOCK || errno == EAGAIN))
            {
                conn->acks = 0;
                break;
            }
            else
            {
                error("Cannot flush reliable data (%d)", n);
                Destroy_connection(conn, "flush error");
                return -1;
            }
        }
        todo -= len;
        rel_off += len;
        read_buf += len;
    }

    /*
     * Drop rest of outgoing data packet if something remains at all.
     */
    Sockbuf_clear(&conn->w);

    conn->last_send_loops = main_loops;

    if (max_todo - todo <= 0)
    {
        /*
         * We have not transmitted anything at all.
         */
        return 0;
    }

    /*
     * Retransmission timer with exponential backoff.
     */
    if (conn->rtt_retransmit > MAX_RETRANSMIT)
    {
        conn->rtt_retransmit = MAX_RETRANSMIT;
    }
    if (conn->retransmit_at_loop <= main_loops)
    {
        conn->retransmit_at_loop = main_loops + conn->rtt_retransmit;
        conn->rtt_retransmit <<= 1;
        conn->rtt_timeouts++;
    }
    else
    {
        conn->retransmit_at_loop = main_loops + conn->rtt_retransmit;
    }

    if (rel_off > conn->reliable_unsent)
    {
        conn->reliable_unsent = rel_off;
    }

    return (max_todo - todo);
}

static int Receive_ack(connection_t *conn)
{
    int n;
    uint8_t ch;
    long rel,
        rtt, /* RoundTrip Time */
        diff,
        delta,
        rel_loops;

    if ((n = Packet_scanf(&conn->r, "%c%ld%ld",
                          &ch, &rel, &rel_loops)) <= 0)
    {
        errno = 0;
        error("Cannot read ack packet (%d)", n);
        Destroy_connection(conn, "read error");
        return -1;
    }
    if (ch != PKT_ACK)
    {
        errno = 0;
        error("Not an ack packet (%d)", ch);
        Destroy_connection(conn, "not ack");
        return -1;
    }
    rtt = main_loops - rel_loops;
    if (rtt > 0 && rtt <= MAX_RTT)
    {
        /*
         * These roundtrip estimation calculations are derived from Comer's
         * books "Internetworking with TCP/IP" parts I & II.
         */
        if (conn->rtt_smoothed == 0)
        {
            /*
             * Initialize the rtt estimator by this first measurement.
             * The estimator is scaled by 3 bits.
             */
            conn->rtt_smoothed = rtt << 3;
        }
        /*
         * Scale the estimator back by 3 bits before calculating the error.
         */
        delta = rtt - (conn->rtt_smoothed >> 3);
        /*
         * Add one eigth of the error to the estimator.
         */
        conn->rtt_smoothed += delta;
        /*
         * Now we need the absolute value of the error.
         */
        if (delta < 0)
        {
            delta = -delta;
        }
        /*
         * The rtt deviation is scaled by 2 bits.
         * Now we add one fourth of the difference between the
         * error and the previous deviation to the deviation.
         */
        conn->rtt_dev += delta - (conn->rtt_dev >> 2);
        /*
         * The calculation of the retransmission timeout is what this is
         * all about.  We take the smoothed rtt plus twice the deviation
         * as the next retransmission timeout to use.  Because of the
         * scaling used we get the following statement:
         */
        conn->rtt_retransmit = ((conn->rtt_smoothed >> 2) + conn->rtt_dev) >> 1;
        /*
         * Now keep it within reasonable bounds.
         */
        if (conn->rtt_retransmit < MIN_RETRANSMIT)
        {
            conn->rtt_retransmit = MIN_RETRANSMIT;
        }
    }
    diff = rel - conn->reliable_offset;
    if (diff > conn->c.len)
    {
        /* Impossible to ack data that has not been send */
        errno = 0;
        error("Bad ack (diff=%ld,cru=%ld,c=%ld,len=%d)",
              diff, rel, conn->reliable_offset, conn->c.len);
        Destroy_connection(conn, "bad ack");
        return -1;
    }
    else if (diff <= 0)
    {
        /* Late or duplicate ack of old data.  Discard. */
        return 1;
    }
    Sockbuf_advance(&conn->c, (int)diff);
    conn->reliable_offset += diff;
    if ((n = ((diff + 512 - 1) / 512)) > conn->acks)
    {
        conn->acks = n;
    }
    else
    {
        conn->acks++;
    }
    if (conn->reliable_offset >= conn->reliable_unsent)
    {
        /*
         * All reliable data has been sent and acked.
         */
        conn->retransmit_at_loop = 0;
        if (conn->state == CONN_DRAIN)
        {
            Conn_set_state(conn, conn->drain_state, conn->drain_state);
        }
    }
    if (conn->state == CONN_READY && (conn->c.len <= 0 || (conn->c.buf[0] != PKT_REPLY && conn->c.buf[0] != PKT_PLAY && conn->c.buf[0] != PKT_SUCCESS && conn->c.buf[0] != PKT_FAILURE)))
    {
        Conn_set_state(conn, conn->drain_state, conn->drain_state);
    }
    conn->rtt_timeouts = 0;

    return 1;
}

static int Receive_discard(connection_t *conn)
{
    errno = 0;
    error("Discarding packet %d while in state %02x",
          conn->r.ptr[0], conn->state);
    conn->r.ptr = conn->r.buf + conn->r.len;

    return 0;
}

static int Receive_undefined(connection_t *conn)
{
    errno = 0;
    error("Unknown packet type (%d,%02x)", conn->r.ptr[0], conn->state);
    Destroy_connection(conn, "undefined packet");
    return -1;
}

static int Receive_ack_cannon(connection_t *conn)
{
    long loops_ack;
    uint8_t ch;
    int n;
    unsigned short num;

    if ((n = Packet_scanf(&conn->r, "%c%ld%hu",
                          &ch, &loops_ack, &num)) <= 0)
    {
        if (n == -1)
        {
            Destroy_connection(conn, "read error");
        }
        return n;
    }
    if (num >= world->NumCannons)
    {
        Destroy_connection(conn, "bad cannon ack");
        return -1;
    }
    if (loops_ack > world->cannon[num].last_change)
    {
        SET_BIT(world->cannon[num].conn_mask, 1 << conn->conn_index);
    }
    return 1;
}

static int Receive_ack_fuel(connection_t *conn)
{
    long loops_ack;
    uint8_t ch;
    int n;
    unsigned short num;

    if ((n = Packet_scanf(&conn->r, "%c%ld%hu",
                          &ch, &loops_ack, &num)) <= 0)
    {
        if (n == -1)
        {
            Destroy_connection(conn, "read error");
        }
        return n;
    }
    if (num >= world->NumFuels)
    {
        Destroy_connection(conn, "bad fuel ack");
        return -1;
    }
    if (loops_ack > world->fuel[num].last_change)
    {
        SET_BIT(world->fuel[num].conn_mask, 1 << conn->conn_index);
    }
    return 1;
}

static int Receive_ack_target(connection_t *conn)
{
    long loops_ack;
    uint8_t ch;
    int n;
    unsigned short num;

    if ((n = Packet_scanf(&conn->r, "%c%ld%hu",
                          &ch, &loops_ack, &num)) <= 0)
    {
        if (n == -1)
        {
            Destroy_connection(conn, "read error");
        }
        return n;
    }
    if (num >= world->NumTargets)
    {
        Destroy_connection(conn, "bad target ack");
        return -1;
    }
    /*
     * Because the "loops" value as received by the client as part
     * of a frame update is 1 higher than the actual change to the
     * target in collision.c a valid map object change
     * acknowledgement must be at least 1 higher.
     * That's why we should use the '>' symbol to compare
     * and not the '>=' symbol.
     * The same applies to cannon and fuelstation updates.
     * This fix was discovered for 3.2.7, previously some
     * destroyed targets could have been displayed with
     * a diagonal cross through them.
     */
    if (loops_ack > world->targets[num].last_change)
    {
        SET_BIT(world->targets[num].conn_mask, 1 << conn->conn_index);
        CLR_BIT(world->targets[num].update_mask, 1 << conn->conn_index);
    }
    return 1;
}

/*
 * If a message contains a colon then everything before that colon is
 * either a unique player name prefix, or a team number with players.
 * If the string does not match one team or one player the message is not sent.
 * If no colon, the message is general.
 */
static void Handle_talk(connection_t *conn, char *str)
{
    player_t *pl = Players[GetInd[conn->id]];
    int i, sent, team;
    unsigned int len;
    char *cp,
        msg[MSG_LEN * 2];

    if ((cp = strchr(str, ':')) == NULL || cp == str || strchr("-_~)(/\\}{[]", cp[1]) /* smileys are smileys */
    )
    {
        sprintf(msg, "%s [%s]", str, pl->name);
        Set_message(msg);
        return;
    }
    *cp++ = '\0';
    len = strlen(str);
    sprintf(msg, "%s [%s]", cp, pl->name);

    if (strspn(str, "0123456789") == len)
    { /* Team message */
        team = atoi(str);
        sprintf(msg + strlen(msg), ":[%d]", team);
        for (sent = i = 0; i < NumPlayers; i++)
        {
            if (Players[i]->team != TEAM_NOT_SET && Players[i]->team == team)
            {
                sent++;
                Set_player_message(Players[i], msg);
            }
        }
        if (sent)
        {
            if (pl->team != team)
                Set_player_message(pl, msg);
        }
        else
        {
            sprintf(msg, "Message not sent, nobody in team %d!",
                    team);
            Set_player_message(pl, msg);
        }
    }
    else if (strcasecmp(str, "god") == 0)
    {
        Server_log_admin_message(GetInd[conn->id], cp);
    }
    else
    { /* Player message */
        sent = -1;
        /* first look for an exact match on player nickname. */
        for (i = 0; i < NumPlayers; i++)
        {
            if (strcasecmp(Players[i]->name, str) == 0)
            {
                sent = i;
                break;
            }
        }
        if (sent == -1)
        {
            /* now look for a partial match on both nick and realname. */
            for (sent = -1, i = 0; i < NumPlayers; i++)
            {
                if (strncasecmp(Players[i]->name, str, len) == 0 || strncasecmp(Players[i]->realname, str, len) == 0)
                    sent = (sent == -1) ? i : -2;
            }
        }
        switch (sent)
        {
        case -2:
            sprintf(msg, "Message not sent, %s matches more than one player!",
                    str);
            Set_player_message(pl, msg);
            break;
        case -1:
            sprintf(msg, "Message not sent, %s does not match any player!",
                    str);
            Set_player_message(pl, msg);
            break;
        default:
            if (Players[sent] != pl)
            {
                sprintf(msg + strlen(msg), ":[%s]", Players[sent]->name);
                Set_player_message(Players[sent], msg);
                Set_player_message(pl, msg);
            }
            break;
        }
    }
}

static int Receive_talk(connection_t *conn)
{
    uint8_t ch;
    int n;
    long seq;
    char str[MAX_CHARS];

    if ((n = Packet_scanf(&conn->r, "%c%ld%s", &ch, &seq, str)) <= 0)
    {
        if (n == -1)
        {
            Destroy_connection(conn, "read error");
        }
        return n;
    }
    if (seq > conn->talk_sequence_num)
    {
        if ((n = Packet_printf(&conn->c, "%c%ld", PKT_TALK_ACK, seq)) <= 0)
        {
            if (n == -1)
            {
                Destroy_connection(conn, "write error");
            }
            return n;
        }
        conn->talk_sequence_num = seq;
        if (*str == '/')
        {
            Handle_player_command(Players[GetInd[conn->id]], str + 1);
        }
        else
        {
            Handle_talk(conn, str);
        }
    }
    return 1;
}

static int Receive_display(connection_t *conn)
{
    uint8_t ch, debris_colors, spark_rand;
    short width, height;
    int n;

    if ((n = Packet_scanf(&conn->r, "%c%hd%hd%c%c", &ch, &width, &height,
                          &debris_colors, &spark_rand)) <= 0)
    {
        if (n == -1)
        {
            Destroy_connection(conn, "read error");
        }
        return n;
    }
    LIMIT(width, MIN_VIEW_SIZE, MAX_VIEW_SIZE);
    LIMIT(height, MIN_VIEW_SIZE, MAX_VIEW_SIZE);
    conn->view_width = width;
    conn->view_height = height;
    conn->debris_colors = debris_colors;
    conn->spark_rand = spark_rand;
    return 1;
}

static int str2num(char **strp, int min, int max)
{
    char *str = *strp;
    int num = 0;

    while (isdigit(*str))
    {
        num *= 10;
        num += *str++ - '0';
    }
    *strp = str;
    if (num < min || num > max)
        return min;
    return num;
}

static int Receive_modifier_bank(connection_t *conn)
{
    player_t *pl;
    uint8_t bank;
    char str[MAX_CHARS];
    uint8_t ch;
    char *cp;
    modifiers_t mods;
    int n;

    if ((n = Packet_scanf(&conn->r, "%c%c%s", &ch, &bank, str)) <= 0)
    {
        if (n == -1)
        {
            Destroy_connection(conn, "read modbank");
        }
        return n;
    }
    pl = Players[GetInd[conn->id]];
    if (bank < NUM_MODBANKS)
    {
        CLEAR_MODS(mods);
        if (BIT(world->rules->mode, ALLOW_MODIFIERS))
        {
            for (cp = str; *cp; cp++)
            {
                switch (*cp)
                {
                case 'F':
                case 'f':
                    if (!BIT(world->rules->mode, ALLOW_NUKES))
                        break;
                    if (*(cp + 1) == 'N' || *(cp + 1) == 'n')
                        SET_BIT(mods.nuclear, FULLNUCLEAR);
                    break;
                case 'N':
                case 'n':
                    if (!BIT(world->rules->mode, ALLOW_NUKES))
                        break;
                    SET_BIT(mods.nuclear, NUCLEAR);
                    break;
                case 'C':
                case 'c':
                    if (!BIT(world->rules->mode, ALLOW_CLUSTERS))
                        break;
                    SET_BIT(mods.warhead, CLUSTER);
                    break;
                case 'I':
                case 'i':
                    SET_BIT(mods.warhead, IMPLOSION);
                    break;
                case 'V':
                case 'v':
                    cp++;
                    mods.velocity = str2num(&cp, 0, MODS_VELOCITY_MAX);
                    cp--;
                    break;
                case 'X':
                case 'x':
                    cp++;
                    mods.mini = str2num(&cp, 1, MODS_MINI_MAX + 1) - 1;
                    cp--;
                    break;
                case 'Z':
                case 'z':
                    cp++;
                    mods.spread = str2num(&cp, 0, MODS_SPREAD_MAX);
                    cp--;
                    break;
                case 'B':
                case 'b':
                    cp++;
                    mods.power = str2num(&cp, 0, MODS_POWER_MAX);
                    cp--;
                    break;
                case 'L':
                case 'l':
                    cp++;
                    if (!BIT(world->rules->mode, ALLOW_LASER_MODIFIERS))
                        break;
                    if (*cp == 'S' || *cp == 's')
                        SET_BIT(mods.laser, STUN);
                    if (*cp == 'B' || *cp == 'b')
                        SET_BIT(mods.laser, BLIND);
                    break;
                }
            }
        }
        pl->modbank[bank] = mods;
    }
    return 1;
}

void Get_display_parameters(connection_t *conn, int *width, int *height,
                            int *debris_colors, int *spark_rand)
{
    *width = conn->view_width;
    *height = conn->view_height;
    *debris_colors = conn->debris_colors;
    *spark_rand = conn->spark_rand;
}

int Get_player_id(connection_t *conn)
{
    return conn->id;
}

int Get_conn_version(connection_t *conn)
{
    return conn->version;
}

const char *Get_player_addr(connection_t *conn)
{
    return conn->addr;
}

const char *Get_player_dpy(connection_t *conn)
{
    return conn->dpy;
}

static int Receive_shape(connection_t *conn)
{
    int n;
    char ch;
    char str[2 * MSG_LEN];

    if ((n = Packet_scanf(&conn->r, "%c%S", &ch, str)) <= 0)
    {
        if (n == -1)
        {
            Destroy_connection(conn, "read shape");
        }
        return n;
    }
    if (conn->version > 0x3200)
    {
        if ((n = Packet_scanf(&conn->r, "%S", &str[strlen(str)])) <= 0)
        {
            if (n == -1)
            {
                Destroy_connection(conn, "read shape ext");
            }
            return n;
        }
    }
    if (conn->state == CONN_LOGIN && conn->ship == NULL)
    {
        conn->ship = Parse_shape_str(str);
    }
    return 1;
}

static int Receive_motd(connection_t *conn)
{
    uint8_t ch;
    long offset;
    int n;
    long bytes;

    if ((n = Packet_scanf(&conn->r,
                          "%c%ld%ld",
                          &ch, &offset, &bytes)) <= 0)
    {
        if (n == -1)
        {
            Destroy_connection(conn, "read error");
        }
        return n;
    }
    conn->motd_offset = offset;
    conn->motd_stop = offset + bytes;

    return 1;
}

/*
 * Return part of the MOTD into buf starting from offset
 * and continueing at most for maxlen bytes.
 * Return the total MOTD size in size_ptr.
 * The return value is the actual amount of MOTD bytes copied
 * or -1 on error.  A value of 0 means EndOfMOTD.
 *
 * The MOTD is completely read into a dynamic buffer.
 * If this MOTD buffer hasn't been accessed for a while
 * then on the next access the MOTD file is checked for changes.
 */
int Get_motd(char *buf, int offset, int maxlen, int *size_ptr)
{
    static int motd_size;
    static char *motd_buf;
    static long motd_loops;
    static time_t motd_mtime;

    if (size_ptr)
    {
        *size_ptr = 0;
    }
    if (offset < 0 || maxlen < 0)
    {
        return -1;
    }

    if (!motd_loops || (motd_loops + MAX_MOTD_LOOPS < main_loops && offset == 0))
    {

        int fd, size;
        struct stat st;

        motd_loops = main_loops;

        if ((fd = open(options.motdFileName, O_RDONLY)) == -1)
        {
            motd_size = 0;
            return -1;
        }
        if (fstat(fd, &st) == -1 || st.st_size == 0)
        {
            motd_size = 0;
            close(fd);
            return -1;
        }
        size = st.st_size;
        if (size > MAX_MOTD_SIZE)
        {
            size = MAX_MOTD_SIZE;
        }
        if (size != motd_size)
        {
            motd_mtime = 0;
            motd_size = size;
            if (motd_size == 0)
            {
                close(fd);
                return 0;
            }
            if (motd_buf)
            {
                free(motd_buf);
            }
            if ((motd_buf = (char *)malloc(size)) == NULL)
            {
                close(fd);
                return -1;
            }
        }
        if (motd_mtime != st.st_mtime)
        {
            motd_mtime = st.st_mtime;
            if ((size = read(fd, motd_buf, motd_size)) <= 0)
            {
                free(motd_buf);
                motd_buf = 0;
                close(fd);
                motd_size = 0;
                return -1;
            }
            motd_size = size;
        }
        close(fd);
    }

    motd_loops = main_loops;

    if (size_ptr)
    {
        *size_ptr = motd_size;
    }
    if (offset + maxlen > motd_size)
    {
        maxlen = motd_size - offset;
    }
    if (maxlen <= 0)
    {
        return 0;
    }
    memcpy(buf, motd_buf + offset, maxlen);
    return maxlen;
}

/*
 * Send the server MOTD to the client.
 * The last time we send a motd packet it should
 * have datalength zero to mean EOMOTD.
 */
static int Send_motd(connection_t *conn)
{
    int len;
    int off = conn->motd_offset,
        size = 0;
    char buf[MAX_MOTD_CHUNK];

    len = MIN(MAX_MOTD_CHUNK, MAX_RELIABLE_DATA_PACKET_SIZE - conn->c.len - 10);
    if (len >= 10)
    {
        len = Get_motd(buf, off, len, &size);
        if (len <= 0)
        {
            len = 0;
            conn->motd_offset = -1;
        }
        if (Packet_printf(&conn->c,
                          "%c%ld%hd%ld",
                          PKT_MOTD, off, len, size) <= 0)
        {
            Destroy_connection(conn, "motd header");
            return -1;
        }
        if (len > 0)
        {
            conn->motd_offset += len;
            if (Sockbuf_write(&conn->c, buf, len) != len)
            {
                Destroy_connection(conn, "motd data");
                return -1;
            }
        }
    }

    /* Return ok */
    return 1;
}

static int Receive_pointer_move(connection_t *conn)
{
    player_t *pl;
    uint8_t ch;
    short movement;
    int n;
    DFLOAT turnspeed, turndir;

    if ((n = Packet_scanf(&conn->r, "%c%hd", &ch, &movement)) <= 0)
    {
        if (n == -1)
            Destroy_connection(conn, "read error");
        return n;
    }
    pl = Players[GetInd[conn->id]];
    if (BIT(pl->status, HOVERPAUSE))
        return 1;

    if (BIT(pl->used, HAS_AUTOPILOT))
        Autopilot(pl, false);
    turnspeed = movement * pl->turnspeed / MAX_PLAYER_TURNSPEED;
    if (turnspeed < 0)
    {
        turndir = -1.0;
        turnspeed = -turnspeed;
    }
    else
        turndir = 1.0;

    if (pl->turnresistance)
        LIMIT(turnspeed, MIN_PLAYER_TURNSPEED, MAX_PLAYER_TURNSPEED);
    /* Minimum amount of turning if you want to turn at all?
      And the only effect of that maximum is making
      finding the correct settings harder for new mouse players,
      because the limit is checked BEFORE multiplying by turnres!
      Kept here to avoid changing the feeling for old players who
      are already used to this odd behavior. New players should set
      turnresistance to 0.
    */
    else
        LIMIT(turnspeed, 0, 5 * RES);

    pl->turnvel -= turndir * turnspeed;

    return 1;
}

static int Receive_fps_request(connection_t *conn)
{
    player_t *pl;
    int n;
    uint8_t ch;
    uint8_t fps;

    if ((n = Packet_scanf(&conn->r, "%c%c", &ch, &fps)) <= 0)
    {
        if (n == -1)
            Destroy_connection(conn, "read error");
        return n;
    }
    if (conn->id != NO_ID)
    {
        pl = Players[GetInd[conn->id]];
        pl->player_fps = fps;
        // TODO: Fix this stuff
        if (fps > FPS)
            pl->player_fps = FPS;
        if (fps < (FPS / 2))
            pl->player_fps = (FPS + 1) / 2;
        if (fps == 0)
            pl->player_fps = FPS;
        if ((fps == 20) && options.ignore20MaxFPS)
            pl->player_fps = FPS;
    }

    return 1;
}

static int Receive_audio_request(connection_t *conn)
{
    player_t *pl;
    int n;
    uint8_t ch;
    uint8_t onoff;

    if ((n = Packet_scanf(&conn->r, "%c%c", &ch, &onoff)) <= 0)
    {
        if (n == -1)
        {
            Destroy_connection(conn, "read error");
        }
        return n;
    }
    if (conn->id != NO_ID)
    {
        pl = Players[GetInd[conn->id]];
        sound_player_onoff(pl, onoff);
    }

    return 1;
}

int Check_max_clients_per_IP(char *host_addr)
{
    int i, clients_per_ip = 0;
    connection_t *conn;

    if (options.maxClientsPerIP <= 0)
        return 0;

    for (i = 0; i < max_connections; i++)
    {
        conn = &Conn[i];
        if (conn->state != CONN_FREE && !strcasecmp(conn->addr, host_addr))
            clients_per_ip++;
    }

    if (clients_per_ip >= options.maxClientsPerIP)
        return 1;

    return 0;
}
