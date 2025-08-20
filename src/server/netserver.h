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

#ifndef NETSERVER_H
#define NETSERVER_H

#include "connection.h"

int Get_motd(char *buf, int offset, int maxlen, int *size_ptr);
int Setup_net_server(void);
void Destroy_connection(connection_t *conn, const char *reason);
int Check_connection(char *real, char *nick, char *dpy, char *addr);
int Setup_connection(char *real, char *nick, char *dpy, int team,
                     char *addr, char *host, unsigned version);
int Input(void);
int Send_reply(connection_t *conn, int replyto, int result);
int Send_self(connection_t *conn, player_t *pl,
              int lock_id,
              int lock_dist,
              int lock_dir,
              int autopilotlight,
              long status,
              char *mods);
int Send_leave(connection_t *conn, int id);
int Send_war(connection_t *conn, int robot_id, int killer_id);
int Send_seek(connection_t *conn, int programmer_id, int robot_id, int sought_id);
int Send_player(connection_t *conn, int id);
int Send_score(connection_t *conn, int id, int score,
               int life, int mychar, int alliance);
int Send_score_object(connection_t *conn, int score, int x, int y, const char *string);
int Send_team_score(connection_t *conn, int team, int score);
int Send_timing(connection_t *conn, int id, int check, int round);
int Send_base(connection_t *conn, int id, int num);
int Send_fuel(connection_t *conn, int num, int fuel);
int Send_cannon(connection_t *conn, int num, int dead_time);
int Send_destruct(connection_t *conn, int count);
int Send_shutdown(connection_t *conn, int count, int delay);
int Send_thrusttime(connection_t *conn, int count, int max);
int Send_shieldtime(connection_t *conn, int count, int max);
int Send_phasingtime(connection_t *conn, int count, int max);
int Send_debris(connection_t *conn, int type, uint8_t *p, int n);
int Send_wreckage(connection_t *conn, int x, int y, uint8_t wrtype, uint8_t size, uint8_t rot);
int Send_asteroid(connection_t *conn, int x, int y, uint8_t type, uint8_t size, uint8_t rot);
int Send_fastshot(connection_t *conn, int type, uint8_t *p, int n);
int Send_missile(connection_t *conn, int x, int y, int len, int dir);
int Send_ball(connection_t *conn, int x, int y, int id);
int Send_mine(connection_t *conn, int x, int y, int teammine, int id);
int Send_target(connection_t *conn, int num, int dead_time, int damage);
int Send_wormhole(connection_t *conn, int x, int y);
int Send_audio(connection_t *conn, int type, int vol);
int Send_item(connection_t *conn, int x, int y, int type);
int Send_paused(connection_t *conn, int x, int y, int count);
int Send_ecm(connection_t *conn, int x, int y, int size);
int Send_ship(connection_t *conn, int x, int y, int id, int dir, bool shield, bool cloak, bool eshield,
              bool phased, bool deflector);
int Send_refuel(connection_t *conn, int x0, int y0, int x1, int y1);
int Send_connector(connection_t *conn, int x0, int y0, int x1, int y1, int tractor);
int Send_laser(connection_t *conn, int color, int x, int y, int len, int dir);
int Send_radar(connection_t *conn, int x, int y, int size);
int Send_fastradar(connection_t *conn, uint8_t *buf, int n);
int Send_damaged(connection_t *conn, int damaged);
int Send_message(connection_t *conn, const char *msg);
int Send_loseitem(int lose_item_index, connection_t *conn);
int Send_start_of_frame(connection_t *conn);
int Send_end_of_frame(connection_t *conn);
int Send_reliable(connection_t *conn);
int Send_time_left(connection_t *conn, long sec);
int Send_eyes(connection_t *conn, int id);
int Send_trans(connection_t *conn, int x1, int y1, int x2, int y2);
void Get_display_parameters(connection_t *conn, int *width, int *height,
                            int *debris_colors, int *spark_rand);
int Get_player_id(connection_t *conn);
int Get_conn_version(connection_t *conn);
const char *Get_player_addr(connection_t *conn);
const char *Get_player_dpy(connection_t *conn);
int Send_shape(connection_t *conn, int shape);
int Check_max_clients_per_IP(char *host_addr);

#endif
