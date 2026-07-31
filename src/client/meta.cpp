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

#include <sys/time.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

#include "meta.h"

#include "xpconfig.h"

#include "const.h"
#include "net.h"
#include "commonproto.h"
#include "xperror.h"

static struct Meta metas[NUM_METAS] = {
    {META_HOST, META_IP, {0}, MetaConnecting},
    //    {META_HOST_TWO, META_IP_TWO, {0}, MetaConnecting}
};

/*
 * Here we hold the servers which are listed by the meta servers.
 * We record the time we contacted Meta so as to not overload Meta.
 * server_it is an iterator pointing at the first server for the next page.
 */
server_list_t server_list;
time_t server_list_creation_time;
server_list_iter_t server_it = server_list.end();

/*
 * Convert a string to lowercase.
 */
void string_to_lower(char *s)
{
    for (; *s; s++)
        *s = static_cast<char>(tolower(static_cast<unsigned char>(*s)));
}

/*
 * From a hostname return the part after the last dot.
 * E.g.: Vincent.CS.Berkeley.EDU will return EDU.
 */
char *Get_domain_from_hostname(char *host_name)
{
    static char last_domain[] = "\x7E\x7E";
    char *dom;

    if ((dom = strrchr(host_name, '.')) != nullptr)
    {
        if (dom[1] == '\0')
        {
            dom[0] = '\0';
            dom = strrchr(host_name, '.');
        }
    }
    if (dom)
    {
        dom++; /* skip dot */
        /* test toplevel domain for validity */
        if (!isdigit(static_cast<unsigned char>(*dom)))
        {
            if (strlen(dom) >= 2 && strlen(dom) <= 3)
            {
                return dom;
            }
        }
    }

    return last_domain;
}

/*
 * Sort servers based on:
 *        1) number of players.
 *        2) pingtime.
 *        3) country.
 *        4) hostname.
 */
int Welcome_sort_server_list(void)
{
    for (server_info_t *sip : server_list)
    {
        if (!sip)
            continue;

        if (!sip->hostname.empty())
        {
            string_to_lower(sip->hostname.data());
            if (!strncmp(sip->hostname.c_str(), "xpilot", 6))
            {
                sip->hostname[0] = 'X';
                sip->hostname[1] = 'P';
            }
            sip->domain = Get_domain_from_hostname(sip->hostname.data());
        }
        else
        {
            sip->domain = "~~";
        }
    }

    std::sort(server_list.begin(), server_list.end(),
              [](const server_info_t *a, const server_info_t *b)
              {
                  if (a->users != b->users)
                      return a->users > b->users;

                  if (a->pingtime != b->pingtime)
                      return a->pingtime < b->pingtime;

                  if (a->domain != b->domain)
                      return a->domain < b->domain;

                  if (a->hostname != b->hostname)
                      return a->hostname < b->hostname;

                  return a->port < b->port;
              });

    server_it = server_list.begin();
    return 0;
}

/*
 * Put server info on a sorted list.
 */
int Add_server_info(server_info_t *sip)
{
    if (!sip)
        return -1;

    auto it = server_list.begin();
    for (; it != server_list.end(); ++it)
    {
        server_info_t *it_sip = *it;

        /* sort on IP. */
        if (it_sip->ip < sip->ip)
            continue;

        if (it_sip->ip == sip->ip)
        {
            /* same server when same IP + port. */
            if (it_sip->port < sip->port)
                continue;

            if (it_sip->port == sip->port)
            {
                /* work around bug in meta: keep server with highest uptime. */
                if (it_sip->uptime > sip->uptime)
                {
                    /* printf("duplicate: not adding\n"); */
                    return -1;
                }
                else
                {
                    Delete_server_info(it_sip);
                    it = server_list.erase(it);
                    /* printf("duplicate: replacing\n"); */
                }
            }
        }
        break;
    }

    server_list.insert(it, sip);

    /* print for debugging */
    D(printf("list size = %d after %08x, %d\n",
             (int)server_list.size(), sip->ip, sip->port));

    return 0;
}

/*
 * Variant on strtok which does not skip empty fields.
 * Two delimiters after another returns the empty string ("").
 */
char *my_strtok(char *buf, const char *sep)
{
    static char *oldbuf;
    char *ptr;
    char *start;

    if (buf)
        oldbuf = buf;

    start = oldbuf;
    if (!start || !*start)
        return nullptr;

    for (ptr = start; *ptr; ptr++)
    {
        if (strchr(sep, *ptr))
            break;
    }
    oldbuf = (*ptr) ? (ptr + 1) : (ptr);
    *ptr = '\0';
    return start;
}

/*
 * Parse one line of meta output and
 * put the fields in a structure.
 * The structure is put on a sorted list.
 */
void Add_meta_line(char *meta_line)
{
    char *fields[NUM_META_DATA_FIELDS];
    int i;
    int num = 0;
    char *p;
    unsigned ip0, ip1, ip2, ip3 = 0;
    std::string text = meta_line ? meta_line : "";
    server_info_t *sip;

    if (text.empty())
        return;

    /* split line into fields. */
    for (p = my_strtok(text.data(), ":"); p; p = my_strtok(nullptr, ":"))
    {
        if (num < NUM_META_DATA_FIELDS)
        {
            fields[num++] = p;
        }
    }
    if (num < NUM_META_DATA_FIELDS)
    {
        /* should not happen, except maybe for last line. */
        return;
    }
    if (fields[0] != text.data())
    {
        /* sanity check, should not happen. */
        return;
    }

    sip = new server_info_t();
    sip->pingtime = PING_UNKNOWN;
    sip->version = fields[0];
    sip->hostname = fields[1];
    sip->users_str = fields[3];
    sip->mapname = fields[4];
    sip->mapsize = fields[5];
    sip->author = fields[6];
    sip->status = fields[7];
    sip->bases_str = fields[8];
    sip->fps_str = fields[9];
    sip->playlist = fields[10];
    sip->sound = fields[11];
    sip->teambases_str = fields[13];
    sip->timing = fields[14];
    sip->ip_str = fields[15];
    sip->freebases = fields[16];
    sip->queue_str = fields[17];

    if (sscanf(fields[i = 2], "%u", &sip->port) != 1 ||
        sscanf(fields[i = 3], "%u", &sip->users) != 1 ||
        sscanf(fields[i = 8], "%u", &sip->bases) != 1 ||
        sscanf(fields[i = 9], "%u", &sip->fps) != 1 ||
        sscanf(fields[i = 12], "%u", &sip->uptime) != 1 ||
        sscanf(fields[i = 13], "%u", &sip->teambases) != 1 ||
        sscanf(fields[i = 15], "%u.%u.%u.%u", &ip0, &ip1, &ip2, &ip3) != 4 ||
        (ip0 | ip1 | ip2 | ip3) > 255 ||
        sscanf(fields[i = 17], "%u", &sip->queue) != 1)
    {
        printf("error %d in: %s\n", i, meta_line);
        delete sip;
        return;
    }
    else
    {
        sip->ip = (ip0 << 24) | (ip1 << 16) | (ip2 << 8) | ip3;

        // 2026-03: There's a new metaserver at 45.55.104.252, which has about 800
        // servers running the Lifeless map on IPs 136.244.224.x. I don't want to see these.
        if (sip->mapname == "Lifeless")
        {
            delete sip;
            return;
        }

        if (Add_server_info(sip) == -1)
        {
            delete sip;
            return;
        }
    }
}

/*
 * Connect to the meta servers asynchronously.
 * Return the number of connections made,
 * and the highest fd.
 */
void Meta_connect(int *connections_ptr, int *maxfd_ptr)
{
    int i;
    int status;
    int connections = 0;
    int max = -1;
    char buf[MSG_LEN];

    for (i = 0; i < NUM_METAS; i++)
    {
        if (metas[i].sock.fd != SOCK_FD_INVALID)
            sock_close(&metas[i].sock);

        status = sock_open_tcp_connected_non_blocking(&metas[i].sock,
                                                      metas[i].addr,
                                                      META_PROG_PORT);
        if (status == SOCK_IS_ERROR)
        {
            sprintf(buf, "Could not establish connection with %s",
                    metas[i].name);
            error(buf);
            // Welcome_create_label(1, buf);
        }
        else
        {
            connections++;
            if (metas[i].sock.fd > max)
                max = metas[i].sock.fd;
        }
    }
    if (connections_ptr)
        *connections_ptr = connections;

    if (maxfd_ptr)
        *maxfd_ptr = max;
}

/*
 * Lookup the IPs of the metas.
 */
void Meta_dns_lookup(void)
{
    int i;
    char *addr;
    char buf[MSG_LEN];

    for (i = 0; i < NUM_METAS; i++)
    {
        if (metas[i].sock.fd == -2)
        {
            metas[i].sock.fd = SOCK_FD_INVALID;
            sprintf(buf, "Doing a DNS lookup on %s ... ", metas[i].name);
            // Welcome_create_label(1, buf);
            addr = sock_get_addr_by_name(metas[i].name);
            if (addr)
            {
                strlcpy(metas[i].addr, addr, sizeof(metas[i].addr));
            }
        }
    }
}

void Ping_servers(void)
{
    static int serial;              /* mark pings to identify stale reply */
    const int interval = 1000 / 14; /* assumes we can do 14fps of pings */
    const int tries = 1;            /* at least 1 ping for ever server.
                                     * in practice we get several */
    int maxwait = tries * interval * (int)server_list.size();
    sock_t sock;
    fd_set input_mask, readmask;
    struct timeval start, end, timeout;
    server_list_iter_t it, that;
    server_info_t *it_sip;
    sockbuf_t sbuf, rbuf;
    int ms;
    char *reply_ip;
    int reply_port;
    unsigned reply_magic;
    uint8_t reply_serial, reply_status;
    int outstanding;
    char buf[MSG_LEN];

    sprintf(buf, "Pinging servers (%d seconds)...", (maxwait + 500) / 1000);
    // Welcome_create_label(1, buf);

    if (sock_open_udp(&sock, nullptr, 0) == -1)
    {
        return;
    }
    if (sock_set_non_blocking(&sock, 1) == -1)
    {
        sock_close(&sock);
        return;
    }
    if (Sockbuf_init(&sbuf, &sock, CLIENT_RECV_SIZE,
                     SOCKBUF_WRITE | SOCKBUF_DGRAM) == -1)
    {
        sock_close(&sock);
        return;
    }
    if (Sockbuf_init(&rbuf, &sock, CLIENT_RECV_SIZE,
                     SOCKBUF_READ | SOCKBUF_DGRAM) == -1)
    {
        Sockbuf_cleanup(&sbuf);
        sock_close(&sock);
        return;
    }

    FD_ZERO(&input_mask);
    FD_SET(sock.fd, &input_mask);

    it = server_list.end();
    outstanding = 0;
    ms = 0;
    gettimeofday(&start, nullptr);
    do
    {
        while (outstanding < (ms / interval + 1) && !server_list.empty())
        {
            if (it == server_list.end())
            {
                ++serial;
                serial &= 0xFF;
                if (serial == 0)
                    serial = 1;

                /*
                 * Send a packet to the contact port with
                 * a valid magic number but client version
                 * zero.  The server will reply to this
                 * so that the client can tell the user
                 * what version they need.
                 *
                 * Normally this would be a CONTACT_pack but
                 * we cheat and use the packet type field as
                 * a serial number, since the server is
                 * nice enough to send back whatever we send.
                 */
                Sockbuf_clear(&sbuf);
                Packet_printf(&sbuf, "%u%s%hu%c",
                              MAGIC & 0xffff, "p",
                              sock_get_port(&sock), serial);

                /*
                 * Assuming sort order is the most to least
                 * desirable servers, give the interesting
                 * servers first crack at more pings, making
                 * their results more accurate.
                 */
                // Welcome_sort_server_list();
                it = server_list.begin();
            }
            it_sip = *it;
            sock_send_dest(&sock, it_sip->ip_str.c_str(), it_sip->port,
                           sbuf.buf, sbuf.len);
            gettimeofday(&it_sip->start, nullptr);
            /* if it has never been pinged (pung?) mark it now
             * as "not responding" instead of just blank.
             */
            if (it_sip->pingtime == PING_UNKNOWN)
                it_sip->pingtime = PING_NORESP;

            it_sip->serial = serial;
            outstanding++;
            ++it;
        }
        timeout.tv_sec = 0;
        timeout.tv_usec = (interval - (ms % interval)) * 1000;
        readmask = input_mask;
        if (select(sock.fd + 1, &readmask, 0, 0, &timeout) == -1 && errno != EINTR)
            break;

        gettimeofday(&end, nullptr);
        ms = (end.tv_sec - start.tv_sec) * 1000 +
             (end.tv_usec - start.tv_usec) / 1000;

        Sockbuf_clear(&rbuf);
        if ((rbuf.len = sock_receive_any(&sock, rbuf.buf, rbuf.size)) < 4)

            continue;

        if (outstanding > 0)

            --outstanding;

        if (Packet_scanf(&rbuf, "%u%c%c",
                         &reply_magic, &reply_serial,
                         &reply_status) <= 0)
            continue;

        reply_ip = sock_get_last_addr(&sock);
        reply_port = sock_get_last_port(&sock);
        for (that = server_list.begin(); that != server_list.end(); ++that)
        {
            it_sip = *that;
            if (!strcmp(it_sip->ip_str.c_str(), reply_ip) && reply_port == (int)it_sip->port)
            {
                int n;

                if (reply_serial != it_sip->serial)
                    /* replied to an old ping, alive but
                     * slower than 'interval' at least
                     */
                    it_sip->pingtime = MIN(it_sip->pingtime, PING_SLOW);
                else
                {
                    n = (end.tv_sec - it_sip->start.tv_sec) * 1000 +
                        (end.tv_usec - it_sip->start.tv_usec) / 1000;
/* kps - current value is more useful than minimum value */
#if 0
                    it_sip->pingtime = MIN(it_sip->pingtime, n);
#else
                    it_sip->pingtime = n;
#endif
                }
                break;
            }
        }
    } while (ms < maxwait);

    Sockbuf_cleanup(&sbuf);
    Sockbuf_cleanup(&rbuf);
    sock_close(&sock);
}

/*
 * Deallocate the server list.
 */
void Delete_server_list(void)
{
    for (server_info_t *sip : server_list)
        Delete_server_info(sip);

    server_list.clear();
    server_list_creation_time = 0;
    server_it = server_list.end();
}

/*
 * Deallocate a ServerInfo structure.
 */
void Delete_server_info(server_info_t *sip)
{
    delete sip;
}

/*
 * User pressed the Internet button.
 */
int Get_meta_data(void)
{
    int i;
    int max = -1;
    int connections = 0;
    int descriptor_count;
    int readers = 0;
    int senders = 0;
    int bytes_read;
    int buffer_space;
    int total_bytes_read = 0;
    int server_count;
    time_t start, now;
    fd_set rset_in, wset_in;
    fd_set rset_out, wset_out;
    struct timeval tv;
    char *newline;
    char buf[MSG_LEN];

    /*
     * Buffer to hold data from a socket connection to a Meta.
     * The ptr points to the first byte of the unprocessed data.
     * The end points to where the next new data should be loaded.
     */
    struct MetaData
    {
        char *ptr;
        char *end;
        char buf[4096];
    };
    struct MetaData md[NUM_METAS];

    /* lookup addresses. */
    Meta_dns_lookup();

    /* connect asynchronously. */
    Meta_connect(&connections, &max);
    if (!connections)
    {
        // Welcome_create_label(1,
        //                      "Could not establish connections with any metaserver");
        return -1;
    }

    sprintf(buf, "Establishing %s with %d metaserver%s ... ",
            ((connections > 1) ? "connections" : "a connection"),
            connections,
            ((connections > 1) ? "s" : ""));
    // Welcome_create_label(1, buf);

    /* setup select(2) structures. */
    FD_ZERO(&rset_in);
    FD_ZERO(&wset_in);
    for (i = 0; i < NUM_METAS; i++)
    {
        metas[i].state = MetaConnecting;
        if (metas[i].sock.fd != SOCK_FD_INVALID)
            FD_SET(metas[i].sock.fd, &wset_in);
        md[i].ptr = nullptr;
        md[i].end = nullptr;
    }
    /*
     * First wait for the asynchronously connected sockets to become writable.
     * When a socket becomes writable it means that the connection attempt
     * has completed.  After that has happened we can test the socket for
     * readability.  When the connection attempt failed the read will
     * return -1 and probably set errno to ENOTCONN.
     *
     * We try to connect and read for a limited number of seconds.
     * Whenever a connection has succeeded we add another 5 seconds.
     * Whenever a read has succeeded we also add another 5 seconds.
     *
     * Keep administration of the number of sockets in the connected state,
     * the readability state, or the meta-is-sending-data state.
     */
    for (start = time(&now) + 5;
         connections > 0 && now < start + 5; time(&now))
    {
        tv.tv_sec = start + 5 - now;
        tv.tv_usec = 0;

        D(printf("select for %ld (con %d, read %d, send %d) at %ld\n",
                 tv.tv_sec, connections, readers, senders, time(0)));

        rset_out = rset_in;
        wset_out = wset_in;
        descriptor_count =
            select(max + 1, &rset_out, &wset_out, nullptr, &tv);

        D(printf("select = %d at %ld\n", descriptor_count, time(0)));

        if (descriptor_count <= 0)
            break;

        for (i = 0; i < NUM_METAS; i++)
        {
            if (metas[i].sock.fd == SOCK_FD_INVALID)
                continue;
            else if (FD_ISSET(metas[i].sock.fd, &wset_out))
            {
                /* promote socket from writable to readable. */
                FD_CLR(metas[i].sock.fd, &wset_in);
                FD_SET(metas[i].sock.fd, &rset_in);
                metas[i].state = MetaReadable;
                readers++;
                if (!senders)
                {
                    sprintf(buf, "%d metaserver%s accepted a connection.",
                            readers, (readers > 1) ? "s have" : " has");
                    // Welcome_create_label(1, buf);
                    D(printf("%s\n", buf));
                }
                time(&start);
            }
            else if (FD_ISSET(metas[i].sock.fd, &rset_out))
            {
                if (md[i].ptr == nullptr && md[i].end == nullptr)
                {
                    md[i].ptr = md[i].buf;
                    md[i].end = md[i].buf;
                }
                buffer_space = &md[i].buf[sizeof(md[i].buf)] - md[i].end;
                bytes_read =
                    read(metas[i].sock.fd, md[i].end, buffer_space);
                if (bytes_read <= 0)
                {
                    if (bytes_read == -1)
                    {
                        error("Error while reading data from meta %d\n",
                              i + 1);
                    }
                    FD_CLR(metas[i].sock.fd, &rset_in);
                    close(metas[i].sock.fd);
                    metas[i].sock.fd = SOCK_FD_INVALID;
                    --connections;
                    --readers;
                    if (metas[i].state == MetaReceiving)
                    {
                        --senders;
                        if (senders == 0 &&
                            !server_list.empty() && (int)server_list.size() >= 30)
                        {
                            /*
                             * Assume that this meta has sent us all there is
                             */
                            connections = 0;
                        }
                    }
                    if (connections == 0)
                        break;
                }
                else
                {
                    /* Received some bytes from this connection. */
                    total_bytes_read += bytes_read;

                    /* If this connection wasn't marked
                     * as receiving do so now.
                     */
                    if (metas[i].state != MetaReceiving)
                    {
                        metas[i].state = MetaReceiving;
                        ++senders;
                    }

                    sprintf(buf, "Received %d bytes from %d metaserver%s.",
                            total_bytes_read,
                            senders,
                            ((senders == 1) ? "" : "s"));
                    // Welcome_create_label(1, buf);
                    D(printf("%s\n", buf));

                    /* adjust buffer for newly read bytes. */
                    md[i].end += bytes_read;

                    /* process data up to the last line ending in a '\n'.
                     */
                    while ((newline = (char *)memchr(md[i].ptr, '\n',
                                                     md[i].end - md[i].ptr)) != nullptr)
                    {

                        *newline = '\0';
                        if (newline > md[i].ptr && newline[-1] == '\r')
                            newline[-1] = '\0';

                        Add_meta_line(md[i].ptr);
                        md[i].ptr = newline + 1;
                    }
                    /* move partial data to the start of the buffer. */
                    if (md[i].ptr > md[i].buf)
                    {
                        int incomplete_data = (md[i].end - md[i].ptr);
                        memmove(md[i].buf, md[i].ptr, incomplete_data);
                        md[i].ptr = md[i].buf;
                        md[i].end = md[i].ptr + incomplete_data;
                    }
                    /* allow more time to receive more data */
                    time(&start);
                }
            }
        }
    }

    for (i = 0; i < NUM_METAS; i++)
    {
        if (metas[i].sock.fd != SOCK_FD_INVALID)
        {
            close(metas[i].sock.fd);
            metas[i].sock.fd = SOCK_FD_INVALID;
        }
    }

    server_count = (int)server_list.size();

    if (server_count > 0)
    {
        sprintf(buf, "Received information about %d Internet servers",
                server_count);
        server_list_creation_time = time(nullptr);
    }
    else
        sprintf(buf, "Could not contact any Internet Meta server");

    // Welcome_create_label(1, buf);

    server_it = server_list.begin();
    return server_count;
}