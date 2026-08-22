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

#include <cstring>

#include <X11/Xlib.h>

#include "commonproto.h"

#include "clientoptions.h"
#include "other.h"
#include "paint.h"

#include "types.h"
#include "xperror.h"

/* The fonts used in the game */
extern XFontStruct *gameFont;
extern XFontStruct *messageFont;
extern XFontStruct *scoreListFont;
extern XFontStruct *buttonFont;
extern XFontStruct *textFont;
extern XFontStruct *talkFont;
extern XFontStruct *motdFont;

/* The name of the fonts used in the game */
#define FONT_LEN 256
extern char gameFontName[FONT_LEN];
extern char messageFontName[FONT_LEN];
extern char scoreListFontName[FONT_LEN];
extern char buttonFontName[FONT_LEN];
extern char textFontName[FONT_LEN];
extern char talkFontName[FONT_LEN];
extern char motdFontName[FONT_LEN];

extern Display *dpy;     /* Display of player (pointer) */
extern Display *kdpy;    /* Keyboard display */
extern short about_page; /* Which page is the player on? */
// extern uint16_t        team;                /* What team is the player on? */
// extern bool players_exposed; /* Is score window exposed? */
extern int radar_exposures;     /* Is radar window exposed? */
extern bool radar_score_mapped; /* Is the radar and score window mapped */

#define ITEM_HUD 0       /* one color for the HUD */
#define ITEM_PLAYFIELD 1 /* and one color for the playfield */

extern Pixmap itemBitmaps[];
extern GC gameGC, messageGC, radarGC, buttonGC;
extern GC scoreListGC, textGC, talkGC, motdGC;
extern XGCValues gcv;

extern Window topWindow, drawWindow, keyboardWindow;
extern Window radarWindow, playersWindow;

extern Pixmap drawPixmap;         /* Drawing area pixmap */
extern Pixmap radarPixmap;        /* Radar drawing pixmap */
extern Pixmap radarPixmap2;       /* Second radar drawing pixmap */
extern long dpl_1[2];             /* Used by radar hack */
extern long dpl_2[2];             /* Used by radar hack */
extern Window aboutWindow;        /* The About window */
extern Window about_close_b;      /* About close button */
extern Window about_next_b;       /* About next page button */
extern Window about_prev_b;       /* About prev page button */
extern Window talkWindow;         /* Talk window */
extern XColor colors[MAX_COLORS]; /* Colors */
extern Colormap colormap;         /* Private colormap */
extern int maxColors;             /* Max. number of colors to use */
extern bool gotFocus;             /* Do we have the mouse pointer */

extern int spaceColor;

extern unsigned long current_foreground;

static inline void SET_FG(unsigned long fg)
{
    // warn("SET_FG: fg = 0x%08x", fg);
    if (fg != current_foreground)
        XSetForeground(dpy, gameGC, current_foreground = fg);
}

#define MAX_LINE_WIDTH 4

static inline void Check_name_string(Other *other)
{
    if (other && other->max_chars_in_names != clientOptions.maxCharsInNames)
    {
        int len;

        other->id_string = other->nick_name;
        len = other->id_string.length();
        if (clientOptions.maxCharsInNames >= 0 && clientOptions.maxCharsInNames < len)
        {
            std::string s = other->id_string.substr(0, clientOptions.maxCharsInNames);
            other->id_string = s;
        }
        other->name_len = other->id_string.length();
        other->name_width = 2 + XTextWidth(gameFont, other->id_string.c_str(), other->name_len);
        other->max_chars_in_names = clientOptions.maxCharsInNames;
    }
}

// static inline void FIND_NAME_WIDTH(Other *other)
// {
//     if ((other)->name_width == 0)
//     {
//         (other)->name_len = ((other)->nick_name.length());
//         (other)->name_width = 2 + XTextWidth(gameFont, (other)->nick_name.c_str(),
//                                              (other)->name_len);
//     }
// }

extern void Paint_item_symbol(int type, Drawable d, GC mygc,
                              int x, int y, int color);
extern void Paint_item(int type, Drawable d, GC mygc, int x, int y);
extern void Gui_paint_item_symbol(int type, Drawable d, GC mygc,
                                  int x, int y, int c);
extern void Gui_paint_item(int type, Drawable d, GC mygc, int x, int y);

extern void Store_xpaint_options(void);

/*
 * colors.c
 */
extern void List_visuals(void);
extern int Colors_init(void);
extern int Colors_init_bitmaps(void);
extern void Colors_free_bitmaps(void);
extern void Colors_cleanup(void);
extern void Colors_debug(void);
extern void Init_spark_colors(void);
extern void Store_color_options(void);
