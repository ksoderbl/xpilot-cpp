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

#ifndef XINIT_H
#define XINIT_H

#include "xpaint.h"

#define MAX_VISUAL_NAME 12

#define MIN_TOP_WIDTH (640 + 2)
#define MAX_TOP_WIDTH (2560 + 2) // was (1280 + 2)
#define DEF_TOP_WIDTH (1280 + 2) // was (1024 + 2)
#define MIN_TOP_HEIGHT 480
#define MAX_TOP_HEIGHT 1440 // was 1024
#define DEF_TOP_HEIGHT 1024 // was 768

#define MAX_POINTER_BUTTONS 5
#define MAX_BUTTON_DEFS 3
#define NUM_BUTTON_DEFS(i) numButtonDefs[i]

extern Atom ProtocolAtom, KillAtom;
extern int buttonColor, windowColor, borderColor;
extern int ButtonHeight;
extern char visualName[MAX_VISUAL_NAME];
extern Visual *visual;
extern int dispDepth;
extern bool mono;
extern bool blockBitmaps;
extern bool colorSwitch;
extern bool multibuffer;
extern char color_names[MAX_COLORS][MAX_COLOR_LEN];
extern int top_width, top_height;
// extern int                draw_width, draw_height;
extern int players_width, players_height;
extern char *geometry;
extern bool autoServerMotdPopup;
extern bool refreshMotd;
extern char sparkColors[MSG_LEN];
extern int spark_color[MAX_COLORS];
// extern int                num_spark_colors;
extern bool ignoreWindowManager;
extern bool quitting;

/*
 * Prototypes for xinit.c
 */
extern const char *Item_get_text(int i);
extern int Init_top(void);
// extern int Init_playing_windows(void);
// extern int Alloc_msgs(void);
extern void Free_msgs(void);
extern void Expose_button_window(int color, Window w);
extern void Talk_resize(void);
extern void Talk_cursor(bool visible);
extern void Talk_map_window(bool map);
extern int Talk_do_event(XEvent *event);
extern int Talk_paste(char *data, int len, bool overwrite);
extern int Talk_place_cursor(XButtonEvent *xbutton, bool pending);
extern void Talk_window_cut(XButtonEvent *xbutton);
extern void Talk_cut_from_messages(XButtonEvent *xbutton);
extern void Clear_selection(void);
extern void Print_messages_to_stdout(void);
extern void Talk_reverse_cut(void);
extern int FatalError(Display *dpy);
extern void Resize(Window w, int width, int height);

extern int DrawShadowText(Display *, Window w, GC gc,
                          int x_border, int start_y, const char *str,
                          unsigned long fg, unsigned long bg);
extern void ShadowDrawString(Display *, Window w, GC gc,
                             int x, int start_y, const char *str,
                             unsigned long fg, unsigned long bg);
/*
 * about.c
 */
extern int About_callback(int, void *, const char **);
extern int Keys_callback(int, void *, const char **);
extern void Keys_destroy(void);
extern void About(Window w);
extern int Motd_callback(int, void *, const char **);
extern void Motd_destroy(void);
extern void Expose_about_window(void);
extern void Scale_dashes(void);
// extern int Startup_server_motd(void);

#endif
