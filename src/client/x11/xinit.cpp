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

#include <iostream>

#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <cerrno>

#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>

#include "draw.h"

#include "messages.h"
#include "paint.h"

#include "version.h"
#include "xpconfig.h"
#include "const.h"
#include "keys.h"
#include "icon.h"
#include "xpaint.h"
#include "xinit.h"
#include "widget.h"
#include "configure.h"
#include "xperror.h"
#include "netclient.h"
#include "dbuff.h"
#include "protoclient.h"
#include "portability.h"
#include "colors.h"

/*
 * Item structures.
 *
 * If you add an item here then please make sure you also add
 * the item in the proper place in ../replay/xpilot-replay.c.
 */
#include "items/itemRocketPack.xbm"
#include "items/itemCloakingDevice.xbm"
#include "items/itemEnergyPack.xbm"
#include "items/itemWideangleShot.xbm"
#include "items/itemRearShot.xbm"
#include "items/itemMinePack.xbm"
#include "items/itemSensorPack.xbm"
#include "items/itemTank.xbm"
#include "items/itemEcm.xbm"
#include "items/itemArmor.xbm"
#include "items/itemAfterburner.xbm"
#include "items/itemTransporter.xbm"
#include "items/itemDeflector.xbm"
#include "items/itemHyperJump.xbm"
#include "items/itemPhasingDevice.xbm"
#include "items/itemMirror.xbm"
#include "items/itemLaser.xbm"
#include "items/itemEmergencyThrust.xbm"
#include "items/itemTractorBeam.xbm"
#include "items/itemAutopilot.xbm"
#include "items/itemEmergencyShield.xbm"

/* How far away objects should be placed from each other etc... */
#define BORDER 10
#define BTN_BORDER 4

/* Information window dimensions */
#define ABOUT_WINDOW_WIDTH 600
#define ABOUT_WINDOW_HEIGHT 700

extern int RadarHeight;

/*
 * Globals.
 */
int ButtonHeight;
Atom ProtocolAtom, KillAtom;
int buttonColor, windowColor, borderColor;
bool quitting = false;
int top_width, top_height, top_x, top_y, top_posmask;
// int                        draw_width, draw_height;
int players_width, players_height;
char *geometry;
bool autoServerMotdPopup;
bool refreshMotd;
Cursor pointerControlCursor;
char sparkColors[MSG_LEN];
int spark_color[MAX_COLORS];
// int                        num_spark_colors;
bool ignoreWindowManager;

/*
 * NB!  Is dependent on the order of the items in item.h!
 */
static struct
{
    uint8_t *data;
    const char *keysText;
} itemBitmapData[NUM_ITEMS] = {
    {itemEnergyPack_bits,
     "Extra energy/fuel"},
    {itemWideangleShot_bits,
     "Extra front cannons"},
    {itemRearShot_bits,
     "Extra rear cannon"},
    {itemAfterburner_bits,
     "Afterburner; makes your engines more powerful"},
    {itemCloakingDevice_bits,
     "Cloaking device; "
     "makes you almost invisible, both on radar and on screen"},
    {itemSensorPack_bits,
     "Sensor; "
     "enables you to see cloaked opponents more easily"},
    {itemTransporter_bits,
     "Transporter; enables you to steal equipment from "
     "other players"},
    {itemTank_bits,
     "Tank; "
     "makes refueling quicker, increases maximum fuel "
     "capacity and can be jettisoned to confuse enemies"},
    {itemMinePack_bits,
     "Mine; "
     "can be dropped as a bomb or as a stationary mine"},
    {itemRocketPack_bits,
     "Rocket; can be utilized as smart missile, "
     "heatseeking missile, nuclear missile or just a "
     "plain unguided missile (torpedo)"},
    {itemEcm_bits,
     "ECM (Electronic Counter Measures); "
     "can be used to disturb electronic equipment, for instance "
     "can it be used to confuse smart missiles and reprogram "
     "robots to seek certain players"},
    {itemLaser_bits,
     "Laser; "
     "limited range laser beam, costs a lot of fuel, "
     "having more laser items increases the range of the laser, "
     "they can be irrepairably damaged by ECMs"},
    {itemEmergencyThrust_bits,
     "Emergency Thrust; "
     "gives emergency thrust capabilities for a limited period"},
    {itemTractorBeam_bits,
     "Tractor Beam; "
     "gives mutual attractive force to currently locked on ship, "
     "this means the heavier your ship, the less likely you will move "
     "when being tractored or using a tractor"},
    {itemAutopilot_bits,
     "Autopilot; "
     "when on, the ship will turn and thrust against the "
     "direction of travel"},
    {itemEmergencyShield_bits,
     "EmergencyShield; "
     "gives emergency shield capabilities for a limited period"},
    {itemDeflector_bits,
     "Deflector; "
     "pushes hostile objects away from your ship"},
    {itemHyperJump_bits,
     "Hyperjump; "
     "enables you to teleport to a random map location"},
    {itemPhasingDevice_bits,
     "Phasing Device; "
     "lets you fly through anything for a limited period"},
    {itemMirror_bits,
     "Mirror; "
     "reflects laser beams"},
    {itemArmor_bits,
     "Armor; "
     "absorbs shots in the absence of shields"},
};
Pixmap itemBitmaps[NUM_ITEMS]; /* Bitmaps for the items */

char dashes[NUM_DASHES];
char cdashes[NUM_CDASHES];

static int Quit_callback(int, void *, const char **);
static int Config_callback(int, void *, const char **);
static int Score_callback(int, void *, const char **);
static int Player_callback(int, void *, const char **);

static int button_form;
static int menu_button;

const char *Item_get_text(int i)
{
    return itemBitmapData[i].keysText;
}

/*
 * Set specified font for that GC.
 * Return font that is used for this GC, even if setting a new
 * font failed (return default font in that case).
 */
static XFontStruct *Set_font(Display *dpy, GC gc,
                             const char *fontName,
                             const char *resName)
{
    XFontStruct *font;

    if ((font = XLoadQueryFont(dpy, fontName)) == NULL)
    {
        error("Couldn't find font '%s' for %s, using default font",
              fontName, resName);
        font = XQueryFont(dpy, XGContextFromGC(gc));
    }
    else
        XSetFont(dpy, gc, font->fid);

    return font;
}

/*
 * Convert a string of color numbers into an array
 * of "colors[]" indices stored by "spark_color[]".
 * Initialize "num_spark_colors".
 */
static void Init_spark_colors(void)
{
    char buf[MSG_LEN];
    char *src, *dst;
    unsigned col;
    int i;

    num_spark_colors = 0;
    /*
     * The sparkColors specification may contain
     * any possible separator.  Only look at numbers.
     */

    /* hack but protocol will allow max 9 (MM) */
    for (src = sparkColors; *src && (num_spark_colors < 9); src++)
    {
        if (isascii(*src) && isdigit(*src))
        {
            dst = &buf[0];
            do
            {
                *dst++ = *src++;
            } while (*src &&
                     isascii(*src) &&
                     isdigit(*src) &&
                     ((dst - buf) < (sizeof(buf) - 1)));
            *dst = '\0';
            src--;
            if (sscanf(buf, "%u", &col) == 1)
            {
                if (col < (unsigned)maxColors)
                    spark_color[num_spark_colors++] = col;
            }
        }
    }
    if (num_spark_colors == 0)
    {
        if (maxColors <= 8)
        {
            /* 3 colors ranging from 5 up to 7 */
            for (i = 5; i < maxColors; i++)
                spark_color[num_spark_colors++] = i;
        }
        else
        {
            /* 7 colors ranging from 5 till 11 */
            for (i = 5; i < 12; i++)
                spark_color[num_spark_colors++] = i;
        }
        /* default spark colors always include RED. */
        spark_color[num_spark_colors++] = RED;
    }
    for (i = num_spark_colors; i < MAX_COLORS; i++)
        spark_color[i] = spark_color[num_spark_colors - 1];
}

/*
 * Initialize miscellaneous window hints and properties.
 */
extern char **Argv;
extern int Argc;
extern char myClass[];

static void Init_disp_prop(Display *d, Window win,
                           int w, int h, int x, int y,
                           int flags)
{
    XClassHint xclh;
    XWMHints xwmh;
    XSizeHints xsh;
    char msg[256];

    xwmh.flags = InputHint | StateHint | IconPixmapHint;
    xwmh.input = True;
    xwmh.initial_state = NormalState;
    xwmh.icon_pixmap = XCreateBitmapFromData(d, win,
                                             (char *)icon_bits,
                                             icon_width, icon_height);

    xsh.flags = (flags | PMinSize | PMaxSize | PBaseSize | PResizeInc);
    xsh.width = w;
    xsh.base_width =
        xsh.min_width = MIN_TOP_WIDTH;
    xsh.max_width = MAX_TOP_WIDTH;
    xsh.width_inc = 1;
    xsh.height = h;
    xsh.base_height =
        xsh.min_height = MIN_TOP_HEIGHT;
    xsh.max_height = MAX_TOP_HEIGHT;
    xsh.height_inc = 1;
    xsh.x = x;
    xsh.y = y;

    xclh.res_name = NULL;     /* NULL: Automatically uses Argv[0], */
    xclh.res_class = myClass; /* stripped of directory prefixes. */

    /*
     * Set the above properties.
     */
    XSetWMProperties(d, win, NULL, NULL, Argv, Argc,
                     &xsh, &xwmh, &xclh);

    /*
     * Now initialize icon and window title name.
     */
    if (titleFlip)
        sprintf(msg, "Successful connection to server at \"%s\".",
                servername);
    else
        sprintf(msg, "%s -- Server at \"%s\".", TITLE, servername);
    XStoreName(d, win, msg);

    sprintf(msg, "%s:%s", name, servername);
    XSetIconName(d, win, msg);

    if (d != dpy)
        return;

    /*
     * Specify IO error handler and the WM_DELETE_WINDOW atom in
     * an attempt to catch 'nasty' quits.
     */
    ProtocolAtom = XInternAtom(d, "WM_PROTOCOLS", False);
    KillAtom = XInternAtom(d, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(d, win, &KillAtom, 1);
    XSetIOErrorHandler(FatalError);
}

/*
 * The following function initializes a toplevel window.
 * It returns 0 if the initialization was successful,
 * or -1 if it couldn't initialize the double buffering routine.
 */
int Init_top(void)
{
    int i;
    int top_x, top_y;
    int x, y;
    unsigned w, h;
    int values;
    int top_flags;
    XGCValues xgc;
    XSetWindowAttributes sattr;
    unsigned long mask;

    if (topWindow)
    {
        error("Init_top called twice");
        exit(1);
    }

    if (Colors_init() == -1)
        return -1;

    if (shieldDrawMode == -1)
        shieldDrawMode = 0;
    if (hudColor >= maxColors || hudColor < 0)
        hudColor = BLUE;
    if (hudLockColor >= maxColors || hudLockColor < 0)
        hudLockColor = hudColor;
    if (wallColor >= maxColors || wallColor < 0)
        wallColor = BLUE;

    if (wallRadarColor >= maxColors)
        wallRadarColor = BLUE;
    if (targetRadarColor >= maxColors)
        targetRadarColor = BLUE;
    if (oldMessagesColor >= maxColors || oldMessagesColor < 0)
        oldMessagesColor = BLUE;
    if (decorColor >= maxColors || decorColor < 0)
        decorColor = RED;
    if (decorRadarColor >= maxColors)
        decorRadarColor = BLUE;

    shieldDrawMode = shieldDrawMode ? LineSolid : LineOnOffDash;

    /*
     * Get toplevel geometry.
     */
    top_flags = 0;
    if (geometry != NULL && geometry[0] != '\0')
    {
        mask = XParseGeometry(geometry, &x, &y, &w, &h);
    }
    else
    {
        mask = 0;
    }
    if ((mask & WidthValue) != 0)
    {
        top_width = w;
        top_flags |= USSize;
    }
    else
    {
        top_width = DEF_TOP_WIDTH;
        top_flags |= PSize;
    }
    LIMIT(top_width, MIN_TOP_WIDTH, MAX_TOP_WIDTH);
    if ((mask & HeightValue) != 0)
    {
        top_height = h;
        top_flags |= USSize;
    }
    else
    {
        top_height = DEF_TOP_HEIGHT;
        top_flags |= PSize;
    }
    LIMIT(top_height, MIN_TOP_HEIGHT, MAX_TOP_HEIGHT);
    if ((mask & XValue) != 0)
    {
        if ((mask & XNegative) != 0)
            top_x = DisplayWidth(dpy, DefaultScreen(dpy)) - top_width + x;
        else
            top_x = x;
        top_flags |= USPosition;
    }
    else
    {
        top_x = (DisplayWidth(dpy, DefaultScreen(dpy)) - top_width) / 2;
        top_flags |= PPosition;
    }
    if ((mask & YValue) != 0)
    {
        if ((mask & YNegative) != 0)
            top_y = DisplayHeight(dpy, DefaultScreen(dpy)) - top_height + y;
        else
            top_y = y;
        top_flags |= USPosition;
    }
    else
    {
        top_y = (DisplayHeight(dpy, DefaultScreen(dpy)) - top_height) / 2;
        top_flags |= PPosition;
    }
    if (geometry != NULL)
    {
        free(geometry);
        geometry = NULL;
    }

    /*
     * Create toplevel window (we need this first so that we can create GCs)
     */
    mask = 0;
    /*old debug: sattr.background_pixel = colors[WHITE].pixel;*/
    sattr.background_pixel = colors[BLACK].pixel;
    mask |= CWBackPixel;
    sattr.border_pixel = colors[WHITE].pixel;
    mask |= CWBorderPixel;
    if (colormap != 0)
    {
        sattr.colormap = colormap;
        mask |= CWColormap;
    }
    if (ignoreWindowManager)
    {
        sattr.override_redirect = True;
        mask |= CWOverrideRedirect;
    }
    topWindow = XCreateWindow(dpy,
                              DefaultRootWindow(dpy),
                              top_x, top_y,
                              top_width, top_height,
                              0, dispDepth,
                              InputOutput, visual,
                              mask, &sattr);
    XSelectInput(dpy, topWindow,
                 KeyPressMask | KeyReleaseMask | FocusChangeMask | StructureNotifyMask);
    Init_disp_prop(dpy, topWindow, top_width, top_height, top_x, top_y, top_flags);
    if (kdpy)
    {
        int scr = DefaultScreen(kdpy);
        keyboardWindow = XCreateSimpleWindow(kdpy,
                                             DefaultRootWindow(kdpy),
                                             top_x, top_y,
                                             top_width, top_height,
                                             0, 0, BlackPixel(dpy, scr));
        XSelectInput(kdpy, keyboardWindow,
                     KeyPressMask | KeyReleaseMask | FocusChangeMask);
        Init_disp_prop(kdpy, keyboardWindow, top_width, top_height,
                       top_x, top_y, top_flags);
    }

    /*
     * Create item bitmaps
     */
    for (i = 0; i < NUM_ITEMS; i++)
    {
        itemBitmaps[i] = XCreateBitmapFromData(dpy, topWindow,
                                               (char *)itemBitmapData[i].data,
                                               ITEM_SIZE, ITEM_SIZE);
    }

    /*
     * Creates and initializes the graphic contexts.
     */
    xgc.line_width = 0;
    xgc.line_style = LineSolid;
    xgc.cap_style = CapButt;
    xgc.join_style = JoinMiter; /* I think this is fastest, is it? */
    xgc.graphics_exposures = False;
    values = GCLineWidth | GCLineStyle | GCCapStyle | GCJoinStyle | GCGraphicsExposures;

    messageGC = XCreateGC(dpy, topWindow, values, &xgc);
    radarGC = XCreateGC(dpy, topWindow, values, &xgc);
    buttonGC = XCreateGC(dpy, topWindow, values, &xgc);
    scoreListGC = XCreateGC(dpy, topWindow, values, &xgc);
    textGC = XCreateGC(dpy, topWindow, values, &xgc);
    talkGC = XCreateGC(dpy, topWindow, values, &xgc);
    motdGC = XCreateGC(dpy, topWindow, values, &xgc);
    gameGC = XCreateGC(dpy, topWindow, values, &xgc);
    XSetBackground(dpy, gameGC, colors[BLACK].pixel);

    /*
     * Set fonts
     */
    gameFont = Set_font(dpy, gameGC, gameFontName, "gameFont");
    messageFont = Set_font(dpy, messageGC, messageFontName, "messageFont");
    scoreListFont = Set_font(dpy, scoreListGC, scoreListFontName, "scoreListFont");
    buttonFont = Set_font(dpy, buttonGC, buttonFontName, "buttonFont");
    textFont = Set_font(dpy, textGC, textFontName, "textFont");
    talkFont = Set_font(dpy, talkGC, talkFontName, "talkFont");
    motdFont = Set_font(dpy, motdGC, motdFontName, "motdFont");

    XSetState(dpy, gameGC,
              WhitePixel(dpy, DefaultScreen(dpy)),
              BlackPixel(dpy, DefaultScreen(dpy)),
              GXcopy, AllPlanes);
    XSetState(dpy, radarGC,
              WhitePixel(dpy, DefaultScreen(dpy)),
              BlackPixel(dpy, DefaultScreen(dpy)),
              GXcopy, AllPlanes);
    XSetState(dpy, messageGC,
              WhitePixel(dpy, DefaultScreen(dpy)),
              BlackPixel(dpy, DefaultScreen(dpy)),
              GXcopy, AllPlanes);
    XSetState(dpy, buttonGC,
              WhitePixel(dpy, DefaultScreen(dpy)),
              BlackPixel(dpy, DefaultScreen(dpy)),
              GXcopy, AllPlanes);
    XSetState(dpy, scoreListGC,
              WhitePixel(dpy, DefaultScreen(dpy)),
              BlackPixel(dpy, DefaultScreen(dpy)),
              GXcopy, AllPlanes);

    windowColor = BLUE;
    buttonColor = RED;
    borderColor = WHITE;

    return 0;
}

/*
 * Creates the playing windows.
 * Returns 0 on success, -1 on error.
 */
int Init_playing_windows(void)
{
    unsigned w, h;
    Pixmap pix;
    GC cursorGC;

    if (!topWindow)
    {
        if (Init_top())
            return -1;
    }

    Scale_dashes();

    draw_width = top_width - (256 + 2);
    draw_height = top_height;
    drawWindow = XCreateSimpleWindow(dpy, topWindow, 258, 0,
                                     draw_width, draw_height,
                                     0, 0, colors[BLACK].pixel);
    radarWindow = XCreateSimpleWindow(dpy, topWindow, 0, 0,
                                      256, RadarHeight, 0, 0,
                                      colors[BLACK].pixel);

    /* Create buttons */
#define BUTTON_WIDTH 84
    ButtonHeight = buttonFont->ascent + buttonFont->descent + 2 * BTN_BORDER;

    button_form = Widget_create_form(0, topWindow,
                                     0, RadarHeight,
                                     256, ButtonHeight + 2,
                                     0);
    Widget_create_activate(button_form,
                           0 + 0 * BUTTON_WIDTH, 0,
                           BUTTON_WIDTH, ButtonHeight,
                           1, "QUIT",
                           Quit_callback, NULL);
    Widget_create_activate(button_form,
                           1 + 1 * BUTTON_WIDTH, 0,
                           BUTTON_WIDTH, ButtonHeight,
                           1, "ABOUT",
                           About_callback, NULL);
    menu_button = Widget_create_menu(button_form,
                                     2 + 2 * BUTTON_WIDTH, 0,
                                     BUTTON_WIDTH, ButtonHeight,
                                     1, "MENU");
    Widget_add_pulldown_entry(menu_button,
                              "KEYS", Keys_callback, NULL);
    Widget_add_pulldown_entry(menu_button,
                              "CONFIG", Config_callback, NULL);
    Widget_add_pulldown_entry(menu_button,
                              "SCORE", Score_callback, NULL);
    Widget_add_pulldown_entry(menu_button,
                              "PLAYER", Player_callback, NULL);
    Widget_add_pulldown_entry(menu_button,
                              "MOTD", Motd_callback, NULL);
    Widget_map_sub(button_form);

    /* Create score list window */
    players_width = RadarWidth;
    players_height = top_height - (RadarHeight + ButtonHeight + 2);
    playersWindow = XCreateSimpleWindow(dpy, topWindow,
                                        0, RadarHeight + ButtonHeight + 2,
                                        players_width, players_height,
                                        0, 0,
                                        colors[windowColor].pixel);
    /*
     * Selecting the events we can handle.
     */
    XSelectInput(dpy, radarWindow, ExposureMask);
    XSelectInput(dpy, playersWindow, ExposureMask);

    if (!selectionAndHistory)
        XSelectInput(dpy, drawWindow, 0);
    else
        XSelectInput(dpy, drawWindow, ButtonPressMask | ButtonReleaseMask);

    /*
     * Initialize misc. pixmaps if we're not color switching.
     * (This could be in dbuff_init_buffer completely IMHO, -- Metalite)
     */
    switch (dbuf_state->type)
    {
    case PIXMAP_COPY:
        radarPixmap = XCreatePixmap(dpy, radarWindow, 256, RadarHeight, dispDepth);
        radarPixmap2 = XCreatePixmap(dpy, radarWindow, 256, RadarHeight, dispDepth);
        drawPixmap = XCreatePixmap(dpy, drawWindow, draw_width, draw_height, dispDepth);
        break;
    }

    XAutoRepeatOff(dpy); /* We don't want any autofire, yet! */
    if (kdpy)
        XAutoRepeatOff(kdpy);

    /*
     * Define a blank cursor for use with pointer control
     */
    XQueryBestCursor(dpy, drawWindow, 1, 1, &w, &h);
    pix = XCreatePixmap(dpy, drawWindow, w, h, 1);
    cursorGC = XCreateGC(dpy, pix, 0, NULL);
    XSetForeground(dpy, cursorGC, 0);
    XFillRectangle(dpy, pix, cursorGC, 0, 0, w, h);
    XFreeGC(dpy, cursorGC);
    pointerControlCursor = XCreatePixmapCursor(dpy, pix, pix, &colors[BLACK],
                                               &colors[BLACK], 0, 0);
    XFreePixmap(dpy, pix);

    /*
     * Maps the windows, makes the visible. Voila!
     */
    XMapSubwindows(dpy, topWindow);
    XMapWindow(dpy, topWindow);
    XSync(dpy, False);

    if (kdpy)
    {
        XMapWindow(kdpy, keyboardWindow);
        XSync(kdpy, False);
    }

    Init_spark_colors();

    return 0;
}

static int Config_callback(int widget_desc, void *data, const char **str)
{
    Config(true);
    return 0;
}

static int Score_callback(int widget_desc, void *data, const char **str)
{
    Config(false);
    if (showRealName != false)
    {
        showRealName = false;
        scoresChanged = 1;
    }
    return 0;
}

static int Player_callback(int widget_desc, void *data, const char **str)
{
    Config(false);
    if (showRealName != true)
    {
        showRealName = true;
        scoresChanged = 1;
    }
    return 0;
}

static int Quit_callback(int widget_desc, void *data, const char **str)
{
    quitting = true;
    return 0;
}

void Resize(Window w, int width, int height)
{
    if (w != topWindow)
    {
        return;
    }

    std::cout << "Resize: size: " << width << "x" << height << std::endl;

    // Limits for resizing
    LIMIT(width, MIN_TOP_WIDTH, MAX_TOP_WIDTH);
    LIMIT(height, MIN_TOP_HEIGHT, MAX_TOP_HEIGHT);
    if (width == top_width && height == top_height)
        // Same size as before, don't need to do anything.
        return;

    top_width = width;
    top_height = height;

    std::cout << "Resize: topWindow size: " << top_width << "x" << top_height << std::endl;

    if (!drawWindow)
        return;

    // Draw window does not include the top left radar or the scorelist/config area.
    draw_width = top_width - 258;
    draw_height = top_height;

    std::cout << "Resize: drawWindow size: " << width << "x" << height << std::endl;

    Send_display();
    Net_flush();
    XResizeWindow(dpy, drawWindow, draw_width, draw_height);
    if (dbuf_state->type == PIXMAP_COPY)
    {
        XFreePixmap(dpy, drawPixmap);
        drawPixmap = XCreatePixmap(dpy, drawWindow, draw_width, draw_height, dispDepth);
    }
    // Players window is the scorelist/config area below the radar.
    players_height = top_height - (RadarHeight + ButtonHeight + 2);
    XResizeWindow(dpy, playersWindow,
                  players_width, players_height);
    Talk_resize();
    Config_resize();
}

/*
 * Cleanup player structure, close the display etc.
 */
void Platform_specific_cleanup(void)
{
    if (dpy != NULL)
    {
        XAutoRepeatOn(dpy);
        Colors_cleanup();
        XCloseDisplay(dpy);
        dpy = NULL;
        if (kdpy)
        {
            XAutoRepeatOn(kdpy);
            XCloseDisplay(kdpy);
            kdpy = NULL;
        }
    }
    Free_msgs();
    Widget_cleanup();
}

int FatalError(Display *dpy)
{
    Net_cleanup();
    /*
     * Quit(&client);
     * It's already a fatal I/O error, nothing to cleanup.
     */
    exit(0);
    return (0);
}

void Scale_dashes()
{
    dashes[0] = WINSCALE(8);
    dashes[1] = WINSCALE(4);

    cdashes[0] = WINSCALE(3);
    cdashes[1] = WINSCALE(9);

    XSetDashes(dpy, gameGC, 0, dashes, NUM_DASHES);
}
