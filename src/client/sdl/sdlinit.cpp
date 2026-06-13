/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client. Copyright (C) 2003-2004 by
 *
 *      Juha Lindström
 *      Erik Andersson
 *      Darel Cullen
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

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_ttf.h>
#include <GL/glu.h>

#include <climits>
#include <cstdio>
#include <cstdlib>

#include "version.h"
#include "xpconfig.h"

#include "commonmacros.h"
#include "commonproto.h"
#include "xperror.h"
#include "xpmemory.h"

#include "paint.h"

#include "text.h"
#include "console.h"
#include "sdlkeys.h"
#include "glwidgets.h"
#include "sdlpaint.h"
#include "sdlinit.h"

/* These are only needed for the polygon tessellation */
/* I'd like to move them to Paint_init/cleanup but because it */
/* is called before the map is ready I need separate functions */
/* for now.. */
extern int Gui_init(void);
extern void Gui_cleanup(void);

int draw_depth;

static SDL_DisplayMode gDesktopMode{};

static bool Init_desktop_display_mode()
{
    if (SDL_GetDesktopDisplayMode(0, &gDesktopMode) != 0)
    {
        error("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

/* Flags: in SDL2 we no longer pass these to SDL_SetVideoMode.
 * Keep videoFlags as a legacy "our own" bitmask if other code expects it.
 */
int videoFlags;

/* SDL2: SDL_Surface* from SDL_SetVideoMode no longer exists for OpenGL windows. */
SDL_Surface *MainSDLSurface = nullptr;

/* SDL2 window + GL context */
SDL_Window *gWindow = nullptr;
SDL_GLContext gGLContext = nullptr;

font_data gamefont;
font_data mapfont;
int gameFontSize;
int mapFontSize;
char *gamefontname;

/* SDL1.2 compatibility: SDL_FULLSCREEN is not defined in SDL2. We keep it as our own bit. */
#ifndef SDL_FULLSCREEN
#define SDL_FULLSCREEN 0x00000001
#endif

/* ugly kps hack */
static bool file_exists(const char *path)
{
    FILE *fp;

    if (!path)
    {
        return false;
    }
    else
    {
        fp = fopen(path ? path : "", "r");
        if (fp)
        {
            fclose(fp);
            return true;
        }
        return false;
    }
}

int Init_playing_windows(void)
{
    /*
    sdl_init_colors();
    Init_spark_colors();
    */
    if (!AppendGLWidgetList(&MainWidget, Init_MainWidget(&gamefont)))
    {
        error("widget initialization failed");
        return -1;
    }
    if (Console_init())
    {
        error("console initialization failed");
        return -1;
    }
    if (Gui_init())
    {
        error("gui initialization failed");
        return -1;
    }

    return 0;
}

/* SDL2 replacement for SDL_ListModes + closest match logic */
static bool find_size(int *w, int *h)
{
    if (!w || !h || *w <= 0 || *h <= 0)
        return false;

    int displayIndex = 0;
    if (gWindow)
        displayIndex = SDL_GetWindowDisplayIndex(gWindow);

    if (displayIndex < 0)
        displayIndex = 0;

    SDL_DisplayMode target{};
    target.w = *w;
    target.h = *h;
    target.format = 0;
    target.refresh_rate = 0;
    target.driverdata = nullptr;

    SDL_DisplayMode closest{};
    if (SDL_GetClosestDisplayMode(displayIndex, &target, &closest) == nullptr)
        return false;

    *w = closest.w;
    *h = closest.h;
    return true;
}

static void apply_gl_state_after_resize()
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glViewport(0, 0, draw_width, draw_height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, draw_width, 0, draw_height);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

int Init_window(void)
{
    int value;
    char defaultfontname[] = CONF_FONTDIR "FreeSansBoldOblique.ttf";
    bool gf_exists = true, df_exists = true, gf_init = false, mf_init = false;

    if (TTF_Init())
    {
        error("SDL_ttf initialization failed: %s", TTF_GetError());
        return -1;
    }
    warn("SDL_ttf initialized.\n");

    Conf_print();

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        error("failed to initialize SDL: %s", SDL_GetError());
        return -1;
    }

    (void)Init_desktop_display_mode();

    atexit(SDL_Quit);

    num_spark_colors = 8;

    /* legacy flags: keep around, but don't pass to SDL2 to create the GL window */
    videoFlags = 0;
#ifndef _WINDOWS
    /* Resizable window in SDL2 is a window flag. Keep legacy marker off here. */
#else
    videoFlags |= SDL_FULLSCREEN;
#endif

    /* SDL2: choose a sane default "depth" for old code paths */
    draw_depth = 32;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    /* SDL2 window flags */
    Uint32 windowFlags = SDL_WINDOW_OPENGL;
#ifndef _WINDOWS
    windowFlags |= SDL_WINDOW_RESIZABLE;
#endif

    if (videoFlags & SDL_FULLSCREEN)
    {
        /* match old Windows behavior */
        windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

        /* If you still want "closest mode" behavior, keep the find_size call. */
        if (!find_size((int *)&draw_width, (int *)&draw_height))
        {
            videoFlags ^= SDL_FULLSCREEN;
            windowFlags &= ~SDL_WINDOW_FULLSCREEN_DESKTOP;
        }
    }

    gWindow = SDL_CreateWindow(
        TITLE,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        draw_width,
        draw_height,
        windowFlags);

    if (!gWindow)
    {
        error("Could not create SDL2 window: %s", SDL_GetError());
        return -1;
    }

    gGLContext = SDL_GL_CreateContext(gWindow);
    if (!gGLContext)
    {
        error("Could not create OpenGL context: %s", SDL_GetError());
        return -1;
    }

    SDL_GL_MakeCurrent(gWindow, gGLContext);

    /* Optional: enable vsync if available */
    (void)SDL_GL_SetSwapInterval(1);

    SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &value);
    printf("RGB bpp %d/", value);
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &value);
    printf("%d/", value);
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &value);
    printf("%d ", value);
    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &value);
    printf("Bit Depth is %d\n", value);

    apply_gl_state_after_resize();

    /* SDL2 replacement for SDL_WM_SetCaption */
    SDL_SetWindowTitle(gWindow, TITLE);

    /* this prevents a freetype crash if you pass non existant fonts */
    if (!file_exists(gamefontname))
    {
        error("cannot find your game font '%s'.\n"
              "Please check that it exists!",
              gamefontname);
        printf("Reverting to defaultfont '%s'\n", defaultfontname);
        gf_exists = false;
    }
    if (!file_exists(defaultfontname))
    {
        error("cannot find the default font! '%s'", defaultfontname);
        df_exists = false;
    }

    if (!gf_exists && !df_exists)
    {
        error("Failed to find any font files!\n"
              "Probably you forgot to run 'make install',use '-TTFont <font.ttf>' argument"
              " until you do");
        return -1;
    }

    if (gf_exists)
    {
        if (fontinit(&gamefont, gamefontname, gameFontSize))
        {
            error("Font initialization failed with %s", gamefontname);
        }
        else
            gf_init = true;
    }
    if (!gf_init && df_exists)
    {
        if (fontinit(&gamefont, defaultfontname, gameFontSize))
        {
            error("Default font initialization failed with %s", defaultfontname);
        }
        else
            gf_init = true;
    }

    if (!gf_init)
    {
        error("Failed to initialize any game font! (quitting)");
        return -1;
    }

    if (gf_exists)
    {
        if (fontinit(&mapfont, gamefontname, mapFontSize))
        {
            error("Font initialization failed with %s", gamefontname);
        }
        else
            mf_init = true;
    }
    if (!mf_init && df_exists)
    {
        if (fontinit(&mapfont, defaultfontname, mapFontSize))
        {
            error("Default font initialization failed with %s", defaultfontname);
        }
        else
            mf_init = true;
    }

    if (!mf_init)
    {
        error("Failed to initialize any map font! (quitting)");
        return -1;
    }

    return 0;
}

/* function to reset our viewport after a window resize */
int Resize_Window(int width, int height)
{
    SDL_Rect b = {0, 0, 0, 0};

    if (videoFlags & SDL_FULLSCREEN)
        if (!find_size(&width, &height))
            return -1;

    b.w = draw_width = width;
    b.h = draw_height = height;

    SetBounds_GLWidget(MainWidget, &b);

    /* SDL2: replace SDL_SetVideoMode with SDL_SetWindowSize */
    if (!gWindow)
        return -1;

    SDL_SetWindowSize(gWindow, width, height);

    /* Re-apply projection + viewport */
    apply_gl_state_after_resize();
    return 0;
}

void Platform_specific_cleanup(void)
{
    Close_Widget(&MainWidget);
    Gui_cleanup();
    Console_cleanup();
    fontclean(&gamefont);
    fontclean(&mapfont);
    TTF_Quit();

    if (gGLContext)
    {
        SDL_GL_DeleteContext(gGLContext);
        gGLContext = nullptr;
    }
    if (gWindow)
    {
        SDL_DestroyWindow(gWindow);
        gWindow = nullptr;
    }

    SDL_Quit();
}

static bool Set_geometry(xp_option_t *opt, const char *s)
{
    int w = 0, h = 0;

    if (s[0] == '=')
    {
        sscanf(s, "%*c%d%*c%d", &w, &h);
    }
    else
    {
        sscanf(s, "%d%*c%d", &w, &h);
    }
    if (w == 0 || h == 0)
        return false;
    if (gWindow != nullptr)
    {
        Resize_Window(w, h);
    }
    else
    {
        draw_width = w;
        draw_height = h;
    }
    return true;
}

static const char *Get_geometry(xp_option_t *opt)
{
    static char buf[20]; /* should be enough */
    snprintf(buf, 20, "%dx%d", draw_width, draw_height);
    return buf;
}

static bool Set_fontName(xp_option_t *opt, const char *value)
{
    XFREE(gamefontname);
    gamefontname = xp_safe_strdup(value);

    return true;
}

static const char *Get_fontName(xp_option_t *opt)
{
    return gamefontname;
}

static xp_option_t sdlinit_options[] = {
    XP_STRING_OPTION(
        "geometry",
        "1280x1024",
        NULL,
        0,
        Set_geometry, NULL, Get_geometry,
        XP_OPTFLAG_DEFAULT,
        "Set the initial window geometry.\n"),

    XP_INT_OPTION(
        "gameFontSize",
        16, 12, 32,
        &gameFontSize,
        NULL,
        XP_OPTFLAG_DEFAULT,
        "Height of font used for game strings.\n"),

    XP_INT_OPTION(
        "mapFontSize",
        16, 12, 64,
        &mapFontSize,
        NULL,
        XP_OPTFLAG_DEFAULT,
        "Height of font used for strings painted on the map.\n"),

    XP_STRING_OPTION(
        "TTFont",
        CONF_FONTDIR "FreeSansBoldOblique.ttf",
        NULL, 0,
        Set_fontName, NULL, Get_fontName,
        XP_OPTFLAG_DEFAULT,
        "Set the font to use.\n")};

void Store_sdlinit_options(void)
{
    STORE_OPTIONS(sdlinit_options);
}
