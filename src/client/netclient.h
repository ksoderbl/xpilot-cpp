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

#ifndef NETCLIENT_H
#define NETCLIENT_H

#include "setup.h"
#include "types.h"

#define MIN_RECEIVE_WINDOW_SIZE 1
#define MAX_RECEIVE_WINDOW_SIZE 4

#define FPS (Setup->frames_per_second)
#define MAX_SUPPORTED_FPS 255

typedef struct
{
    int view_width;
    int view_height;
    int spark_rand;
    int num_spark_colors;
} display_t;

extern int simulating;
extern setup_t *Setup;
extern int receive_window_size;
extern long last_loops;
extern display_t server_display; /* the servers idea about our display */

typedef struct
{
    int movement;
    double turnspeed;
    int id;
} pointer_move_t;

#define MAX_POINTER_MOVES 128

extern pointer_move_t pointer_moves[MAX_POINTER_MOVES];
extern int pointer_move_next;
extern long last_keyboard_ack;
extern bool dirPrediction;

int Net_setup(void);
int Net_verify(char *user_name, char *nick_name, char *dpy, int my_team);
int Net_init(char *server, int port);
void Net_cleanup(void);
void Net_key_change(void);
int Net_flush(void);
int Net_fd(void);
int Net_start(void);
void Net_init_measurement(void);
void Net_init_lag_measurement(void);
int Net_input(void);
/* void Net_measurement(long loop, int status);*/
int Receive_start(void);
int Receive_end(void);
int Receive_message(void);
int Receive_self(void);
int Receive_self_items(void);
int Receive_modifiers(void);
int Receive_refuel(void);
int Receive_connector(void);
int Receive_laser(void);
int Receive_missile(void);
int Receive_ball(void);
int Receive_ship(void);
int Receive_mine(void);
int Receive_item(void);
int Receive_destruct(void);
int Receive_shutdown(void);
int Receive_thrusttime(void);
int Receive_shieldtime(void);
int Receive_phasingtime(void);
int Receive_rounddelay(void);
int Receive_debris(void);
int Receive_wreckage(void);
int Receive_asteroid(void);
int Receive_wormhole(void);
int Receive_polystyle(void);
int Receive_fastshot(void);
int Receive_ecm(void);
int Receive_trans(void);
int Receive_paused(void);
int Receive_appearing(void);
int Receive_radar(void);
int Receive_fastradar(void);
int Receive_damaged(void);
int Receive_leave(void);
int Receive_war(void);
int Receive_seek(void);
int Receive_player(void);
int Receive_team(void);
int Receive_score(void);
int Receive_score_object(void);
int Receive_team_score(void);
int Receive_timing(void);
int Receive_fuel(void);
int Receive_cannon(void);
int Receive_target(void);
int Receive_base(void);
int Receive_reliable(void);
int Receive_quit(void);
int Receive_string(void);
int Receive_reply(int *replyto, int *result);
int Send_ack(long rel_loops);
int Send_keyboard(uint8_t *);
int Send_shape(char *);
int Send_power(double power);
int Send_power_s(double power_s);
int Send_turnspeed(double turnspeed);
int Send_turnspeed_s(double turnspeed_s);
int Send_turnresistance(double turnresistance);
int Send_turnresistance_s(double turnresistance_s);
int Send_pointer_move(int movement);
int Receive_audio(void);
int Receive_talk_ack(void);
int Send_talk(void);
int Send_display(void);
int Send_modifier_bank(int);
int Net_talk(char *str);
int Net_ask_for_motd(long offset, long maxlen);
int Receive_time_left(void);
int Receive_eyes(void);
int Receive_motd(void);
int Receive_magic(void);
int Send_audio_request(bool on);
int Send_fps_request(int fps);
int Receive_loseitem(void);

#endif
