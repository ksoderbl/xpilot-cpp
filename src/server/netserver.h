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

#include "player.h"

int Setup_net_server(void);
void Destroy_connection(Connection *connp, const char *reason);
int Check_connection(char *real, char *nick, char *dpy, char *addr);
int Setup_connection(char *real, char *nick, char *dpy, int team,
                     char *addr, char *host, unsigned version);
int Input(void);
int Send_reply(Connection *connp, int replyto, int result);
int Send_self(Connection *connp, Player *pl,
              int lock_id,
              int lock_dist,
              int lock_dir,
              int autopilotlight,
              int status,
              char *mods);
int Send_leave(Connection *connp, int id);
int Send_player(Connection *connp, int id);
int Send_team(Connection *connp, int id, int team);
int Send_score(Connection *connp, int id, double score,
               int life, int mychar, int alliance);
int Send_score_object(Connection *connp, double score, clpos_t pos, const char *string);
int Send_timing(Connection *connp, int id, int check, int round);
int Send_base(Connection *connp, int id, int num);
int Send_fuel(Connection *connp, int num, double fuel);
int Send_cannon(Connection *connp, int num, int dead_ticks);
int Send_destruct(Connection *connp, int count);
int Send_shutdown(Connection *connp, int count, int delay);
int Send_thrusttime(Connection *connp, int count, int max);
int Send_shieldtime(Connection *connp, int count, int max);
int Send_phasingtime(Connection *connp, int count, int max);
int Send_debris(Connection *connp, int type, uint8_t *p, unsigned n);
int Send_wreckage(Connection *connp, clpos_t pos, int wrtype, int size, int rot);
int Send_asteroid(Connection *connp, clpos_t pos, int type, int size, int rot);
int Send_fastshot(Connection *connp, int type, uint8_t *p, unsigned n);
int Send_missile(Connection *connp, clpos_t pos, int len, int dir);
int Send_ball(Connection *connp, clpos_t pos, int id, int style);
int Send_mine(Connection *connp, clpos_t pos, int teammine, int id);
int Send_target(Connection *connp, int num, int dead_ticks, double damage);
int Send_wormhole(Connection *connp, clpos_t pos);
int Send_polystyle(Connection *connp, int polyind, int newstyle);
int Send_audio(Connection *connp, int type, int vol);
int Send_item(Connection *connp, clpos_t pos, int type);
int Send_paused(Connection *connp, clpos_t pos, int count);
int Send_appearing(Connection *connp, clpos_t pos, int id, int count);
int Send_ecm(Connection *connp, clpos_t pos, int size);
int Send_ship(Connection *connp, clpos_t pos, int id, int dir, int shield, int cloak, int eshield, int phased, int deflector);
int Send_refuel(Connection *connp, clpos_t pos1, clpos_t pos2);
int Send_connector(Connection *connp, clpos_t pos1, clpos_t pos2, int tractor);
int Send_laser(Connection *connp, int color, clpos_t pos, int len, int dir);
int Send_radar(Connection *connp, int x, int y, int size);
int Send_fastradar(Connection *connp, uint8_t *buf, unsigned n);
int Send_damaged(Connection *connp, int damaged);
int Send_message(Connection *connp, const char *msg);
int Send_loseitem(Connection *connp, int lose_item_index);
int Send_start_of_frame(Connection *connp);
int Send_end_of_frame(Connection *connp);
int Send_reliable(Connection *connp);
int Send_time_left(Connection *connp, long sec);
int Send_eyes(Connection *connp, int id);
int Send_trans(Connection *connp, clpos_t pos1, clpos_t pos2);
void Get_display_parameters(Connection *connp, int *width, int *height,
                            int *debris_colors, int *spark_rand);
int Get_player_id(Connection *connp);
std::string Player_get_addr(Player *pl);
std::string Player_get_dpy(Player *pl);
int Send_shape(Connection *connp, int shape);
int Check_max_clients_per_IP(std::string host_addr);

#define FEATURE(connp, feature) ((connp)->features & (feature))
#define F_POLY (1 << 0)
#define F_FLOATSCORE (1 << 1)
#define F_EXPLICITSELF (1 << 2)
#define F_ASTEROID (1 << 3)
#define F_TEMPWORM (1 << 4)
#define F_FASTRADAR (1 << 5)
#define F_SEPARATEPHASING (1 << 6)
#define F_TEAMRADAR (1 << 7)
#define F_SHOW_APPEARING (1 << 8)
#define F_SENDTEAM F_SHOW_APPEARING
#define F_CUMULATIVETURN (1 << 9)
#define F_BALLSTYLE (1 << 10)
#define F_POLYSTYLE (1 << 11)
