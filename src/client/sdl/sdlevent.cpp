/*
 * XPilotNG/SDL, an SDL/OpenGL XPilot client. Copyright (C) 2003-2004 by
 *
 *     Juha Lindström
 *     Erik Andersson
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

#include <cassert>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#ifdef HAVE_XF86MISC
#include <SDL2/SDL_syswm.h>
#endif

#include "client.h"
#include "netclient.h"
#include "paint.h"

#include "sdlinit.h"
#include "sdlkeys.h"
#include "console.h"
#include "sdlpaint.h"
#include "glwidgets.h"
// #include "../xhacks.h"

/* TODO: remove these from client.h and put them in *event.h */
bool initialPointerControl = false;

static int mouseMovement; /* horizontal mouse movement. */

GLWidget *clicktarget[NUM_MOUSE_BUTTONS];
GLWidget *hovertarget = nullptr;

/*
 * SDL2 note:
 * - SDL_WM_GrabInput() is gone. Use SDL_SetWindowGrab() and typically
 *   SDL_SetRelativeMouseMode() for FPS-style pointer control.
 * - SDL_VIDEORESIZE is gone. Use SDL_WINDOWEVENT + SDL_WINDOWEVENT_RESIZED/SIZE_CHANGED.
 *
 * This file assumes you have an SDL_Window* somewhere globally accessible.
 * Hook this up to whatever your codebase uses (e.g. the one created in sdlinit.cpp).
 */
extern SDL_Window *gWindow;

int Process_event(SDL_Event *evt);

void Platform_specific_pointer_control_set_state(bool on)
{
    assert(clData.pointerControl != on);

    if (on)
    {
        MainWidget_ShowMenu(MainWidget, false);

        if (gWindow)
            SDL_SetWindowGrab(gWindow, SDL_TRUE);

        /* Relative mode gives xrel/yrel even if the cursor would hit screen edges. */
        SDL_SetRelativeMouseMode(SDL_TRUE);
        SDL_ShowCursor(SDL_DISABLE);
    }
    else
    {
        MainWidget_ShowMenu(MainWidget, true);

        SDL_SetRelativeMouseMode(SDL_FALSE);

        if (gWindow)
            SDL_SetWindowGrab(gWindow, SDL_FALSE);

        SDL_ShowCursor(SDL_ENABLE);
    }

#ifdef HAVE_XF86MISC
    if (gWindow)
    {
        SDL_SysWMinfo info;
        SDL_VERSION(&info.version);
        if (SDL_GetWindowWMInfo(gWindow, &info) == SDL_TRUE)
            Disable_emulate3buttons(on, info.info.x11.display);
    }
#endif
}

void Platform_specific_talk_set_state(bool on)
{
    assert(clData.talking != on);
    if (on)
        Console_show();
    else
        Console_hide();
}

void Record_toggle(void)
{
    /* TODO: implement if you think it is worth it */
    Add_message("Can't record with this client. [*Client reply*]");
}

void Toggle_radar_and_scorelist(void)
{
    /* TODO */
    return;
}

#ifndef _WINDOWS
void Toggle_fullscreen(void)
{
    static int initial_w = -1, initial_h = -1;

    if (initial_w == -1)
    {
        initial_w = draw_width;
        initial_h = draw_height;
    }

    if (!gWindow)
    {
        Add_message("No SDL window available. [*Client reply*]");
        return;
    }

    /* SDL2: detect fullscreen from the window flags (no legacy SDL_FULLSCREEN needed). */
    const Uint32 wf = SDL_GetWindowFlags(gWindow);
    const bool isFullscreen = (wf & SDL_WINDOW_FULLSCREEN) || (wf & SDL_WINDOW_FULLSCREEN_DESKTOP);

    if (isFullscreen)
    {
        SDL_SetWindowFullscreen(gWindow, 0);
        Resize_Window(initial_w, initial_h);
        return;
    }

    /* Remember the size we are toggling from (windowed size). */
    initial_w = draw_width;
    initial_h = draw_height;

    /* Use DESKTOP to avoid mode switches (closest to SDL1.2 "toggle" UX). */
    if (SDL_SetWindowFullscreen(gWindow, SDL_WINDOW_FULLSCREEN_DESKTOP) == 0)
    {
        /* Let the window manager decide the fullscreen size; SDL_WINDOWEVENT will trigger Resize_Window. */
        return;
    }

    /* Revert on failure */
    SDL_SetWindowFullscreen(gWindow, 0);
    Resize_Window(initial_w, initial_h);
    Add_message("Failed to change video mode. [*Client reply*]");
}
#else
void Toggle_fullscreen(void)
{
    Add_message("Changing mode does not work in Windows. [*Client reply*]");
}
#endif

int Process_event(SDL_Event *evt)
{
    int button;

    mouseMovement = 0;

    if (Console_process(evt))
        return 1;

    switch (evt->type)
    {

    case SDL_QUIT:
        Client_exit(0);
        break;

    case SDL_KEYDOWN:
        if (Console_isVisible())
            break;
        Keyboard_button_pressed((xp_keysym_t)evt->key.keysym.sym);
        break;

    case SDL_KEYUP:
        /* letting release events through to prevent some keys from locking */
        /*if (Console_isVisible()) break;*/
        Keyboard_button_released((xp_keysym_t)evt->key.keysym.sym);
        break;

    case SDL_MOUSEBUTTONDOWN:
        button = evt->button.button;
        if (!clData.pointerControl)
        {
            if (button >= 1 && button <= NUM_MOUSE_BUTTONS)
            {
                if ((clicktarget[button - 1] = FindGLWidget(MainWidget, evt->button.x, evt->button.y)))
                {
                    if (clicktarget[button - 1]->button)
                    {
                        clicktarget[button - 1]->button(button, evt->button.state,
                                                        evt->button.x, evt->button.y,
                                                        clicktarget[button - 1]->buttondata);
                    }
                }
            }
        }
        else
        {
            Pointer_button_pressed(button);
        }
        break;

    case SDL_MOUSEMOTION:
        if (clData.pointerControl)
        {
            mouseMovement += evt->motion.xrel;
        }
        else
        {
            /*printf("mouse motion xrel=%i yrel=%i\n",evt->motion.xrel,evt->motion.yrel);*/
            /*for (i = 0;i<NUM_MOUSE_BUTTONS;++i)*/ /* dragdrop for all mouse buttons*/
            if (clicktarget[0])
            { /*is button one pressed?*/
                /*printf("SDL_MOUSEBUTTONDOWN drag: area found!\n");*/
                if (clicktarget[0]->motion)
                {
                    clicktarget[0]->motion(evt->motion.xrel, evt->motion.yrel,
                                           evt->motion.x, evt->motion.y,
                                           clicktarget[0]->motiondata);
                }
            }
            else
            {
                GLWidget *tmp = FindGLWidget(MainWidget, evt->motion.x, evt->motion.y);
                if (tmp != hovertarget)
                {
                    if (hovertarget && hovertarget->hover)
                    {
                        hovertarget->hover(false, evt->motion.x, evt->motion.y, hovertarget->hoverdata);
                    }
                    tmp = FindGLWidget(MainWidget, evt->motion.x, evt->motion.y);
                    if (tmp && tmp->hover)
                        tmp->hover(true, evt->motion.x, evt->motion.y, tmp->hoverdata);
                    hovertarget = tmp;
                }
            }
        }
        break;

    case SDL_MOUSEBUTTONUP:
        button = evt->button.button;
        if (clData.pointerControl)
        {
            Pointer_button_released(button);
        }
        else
        {
            if (button >= 1 && button <= NUM_MOUSE_BUTTONS)
            {
                if (clicktarget[button - 1])
                {
                    if (clicktarget[button - 1]->button)
                    {
                        clicktarget[button - 1]->button(button, evt->button.state,
                                                        evt->button.x, evt->button.y,
                                                        clicktarget[button - 1]->buttondata);
                    }
                    clicktarget[button - 1] = nullptr;
                }
            }
        }
        break;

    /* SDL2 replacement for SDL_VIDEORESIZE */
    case SDL_WINDOWEVENT:
        if (evt->window.event == SDL_WINDOWEVENT_RESIZED ||
            evt->window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {
            Resize_Window(evt->window.data1, evt->window.data2);
        }
        break;

    default:
        break;
    }

    if (mouseMovement)
    {
        Client_pointer_move(mouseMovement);
        Net_flush();
    }
    return 1;
}

/* kps - just here so that this can link to generic client files */
void Config_redraw(void)
{
}
