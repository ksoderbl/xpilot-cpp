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

#ifndef XPAINT_H
#define XPAINT_H

#include <X11/Xlib.h>

#include "types.h"

/* constants begin */
#define MAX_COLORS 16    /* Max. switched colors ever */
#define MAX_COLOR_LEN 32 /* Max. length of a color name */

#define MAX_MSGS 15 /* Max. messages displayed ever */

#define NUM_DASHES 2
#define NUM_CDASHES 2
#define DASHES_LENGTH 12

#define HUD_SIZE 90   /* Size/2 of HUD lines */
#define HUD_OFFSET 20 /* Hud line offset */
#define FUEL_GAUGE_OFFSET 6
#define HUD_FUEL_GAUGE_SIZE (2 * (HUD_SIZE - HUD_OFFSET - FUEL_GAUGE_OFFSET))
#define FUEL_NOTIFY (3 * FPS)

#define WARNING_DISTANCE (VISIBILITY_DISTANCE * 0.8)
/* constants end */

/* typedefs begin */
typedef struct
{
    char txt[MSG_LEN];
    short len;
    short pixelLen;
    int life;
} message_t;
/* typedefs end */

/* which index a message actually has (consider SHOW_REVERSE_SCROLL) */
#define TALK_MSG_SCREENPOS(_total, _pos) \
    (BIT(instruments, SHOW_REVERSE_SCROLL) ? (_total) - (_pos) : (_pos))

/* how to draw a selection */
#define DRAW_EMPHASIZED BLUE

/*
 * Global objects.
 */

extern char dashes[NUM_DASHES];
extern char cdashes[NUM_CDASHES];

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
// extern unsigned short        team;                /* What team is the player on? */
extern bool players_exposed; /* Is score window exposed? */
extern int radar_exposures;  /* Is radar window exposed? */

/* windows has 2 sets of item bitmaps */
#define ITEM_HUD 0       /* one color for the HUD */
#define ITEM_PLAYFIELD 1 /* and one color for the playfield */
extern Pixmap itemBitmaps[];

extern GC gc, messageGC, radarGC, buttonGC, scoreListGC, textGC, talkGC;
extern GC motdGC;
extern XGCValues gcv;
extern Window top, draw, keyboard, radar, players;
extern Pixmap p_draw;             /* Drawing area pixmap */
extern Pixmap p_radar;            /* Radar drawing pixmap */
extern Pixmap s_radar;            /* Second radar drawing pixmap */
extern long dpl_1[2];             /* Used by radar hack */
extern long dpl_2[2];             /* Used by radar hack */
extern Window about_w;            /* The About window */
extern Window about_close_b;      /* About close button */
extern Window about_next_b;       /* About next page button */
extern Window about_prev_b;       /* About prev page button */
extern Window talk_w;             /* Talk window */
extern XColor colors[MAX_COLORS]; /* Colors */
extern Colormap colormap;         /* Private colormap */
extern int maxColors;             /* Max. number of colors to use */
extern int hudColor;              /* Color index for HUD drawing */
extern int hudLockColor;          /* Color index for lock on HUD drawing */
extern int wallColor;             /* Color index for wall drawing */
extern int wallRadarColor;        /* Color index for walls on radar */
extern int targetRadarColor;      /* Color index for targets on radar */
extern int decorColor;            /* Color index for decoration drawing */
extern int decorRadarColor;       /* Color index for decorations on radar */
extern int oldMessagesColor;      /* Color index for old message strings */
extern bool gotFocus;             /* Do we have the mouse pointer */
extern bool talk_mapped;          /* Is talk window visible */
// extern short        ext_view_width;                /* Width of extended visible area */
// extern short        ext_view_height;        /* Height of extended visible area */
// extern int        active_view_width;        /* Width of active map area displayed. */
// extern int        active_view_height;        /* Height of active map area displayed. */
// extern int        ext_view_x_offset;        /* Offset of ext_view_width */
// extern int        ext_view_y_offset;        /* Offset of ext_view_height */
// extern u_byte        debris_colors;                /* Number of debris intensities */
extern DFLOAT charsPerTick;          /* Output speed of messages */
extern bool markingLights;           /* Marking lights on ships */
extern bool titleFlip;               /* Do special titlebar flipping? */
extern int shieldDrawMode;           /* How to draw players shield */
extern char modBankStr[][MAX_CHARS]; /* modifier banks strings */
extern char *texturePath;            /* Path list of texture directories */
extern char *wallTextureFile;        /* Filename of wall texture */
extern char *decorTextureFile;       /* Filename of decor texture */
extern char *ballTextureFile;        /* Filename of ball texture */

extern int(*radarDrawRectanglePtr) /* Function to draw player on radar */
    (Display *disp, Drawable d, GC gc,
     int x, int y, unsigned width, unsigned height);

extern int maxKeyDefs;
// extern long        loops;
extern int maxMessages;
extern int messagesToStdout;
extern bool selectionAndHistory;

extern DFLOAT scaleFactor; /* scale the draw (main playfield) window */
extern DFLOAT scaleFactor_s;
extern short scaleArray[];
extern void Init_scale_array(void);
#define WINSCALE(__n) ((__n) >= 0 ? scaleArray[(__n)] : -scaleArray[-(__n)])

void Paint_item_symbol(u_byte type, Drawable d, GC mygc, int x, int y, int color);
void Paint_item(u_byte type, Drawable d, GC mygc, int x, int y);

#endif
