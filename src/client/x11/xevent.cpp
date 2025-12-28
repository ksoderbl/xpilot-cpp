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

#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <cmath>

#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>

#include "messages.h"
#include "paint.h"

#include "keydefs.h"

#include "xpconfig.h"
#include "const.h"
#include "xinit.h"
#include "keys.h"
#include "packet.h"
#include "bit.h"
#include "setup.h"
#include "netclient.h"
#include "widget.h"
#include "xperror.h"
#include "record.h"
#include "portability.h"
#include "paintdata.h"
#include "talk.h"
#include "configure.h"
#include "xeventhandlers.h"
#include "xevent.h"

extern char *talk_fast_msgs[]; /* talk macros */

// static BITV_DECL(keyv, NUM_KEYS);

bool initialPointerControl = false;
bool pointerControl = false;
extern Cursor pointerControlCursor;

//
keys_t Lookup_key(XEvent *event, KeySym ks, bool reset)
{
    warn("Lookup_key: event type %d, keysym 0x%03lx, reset %d", event->type, ks, reset);

    keys_t ret = KEY_DUMMY;
    static int i = 0;

    if (reset)
    {
        /* binary search since keyDefs is sorted on keysym. */
        int lo = 0, hi = maxKeyDefs - 1;
        while (lo < hi)
        {
            i = (lo + hi) >> 1;
            if (ks > keyDefs[i].keysym)
            {
                lo = i + 1;
            }
            else
            {
                hi = i;
            }
        }
        if (lo == hi && ks == keyDefs[lo].keysym)
        {
            while (lo > 0 && ks == keyDefs[lo - 1].keysym)
            {
                lo--;
            }
            i = lo;
            ret = keyDefs[i].key;
            i++;
        }
    }
    else
    {
        if (i < maxKeyDefs && ks == keyDefs[i].keysym)
        {
            ret = keyDefs[i].key;
            i++;
        }
    }

    // #ifdef DEVELOPMENT
    if (reset && ret == KEY_DUMMY)
    {
        static XComposeStatus compose;
        char str[4];
        int count;

        memset(str, 0, sizeof str);
        count = XLookupString(&event->xkey, str, 1, &ks, &compose);
        if (count == NoSymbol)
            warn("Unknown keysym: 0x%03lx.", ks);
        else
        {
            if (*str)
                warn("No action bound to keysym 0x%03lx, which is key \"%s\"", ks, str);
            else
                warn("No action bound to keysym 0x%03lx", ks);
        }
    }
    // #endif

    return ret;
}

void Pointer_control_set_state(bool on)
{
    if (on)
    {
        pointerControl = true;
        XGrabPointer(dpy, drawWindow, true, 0, GrabModeAsync,
                     GrabModeAsync, drawWindow, pointerControlCursor, CurrentTime);
        XWarpPointer(dpy, None, drawWindow,
                     0, 0, 0, 0,
                     draw_width / 2, draw_height / 2);
        XDefineCursor(dpy, drawWindow, pointerControlCursor);
        XSelectInput(dpy, drawWindow,
                     PointerMotionMask | ButtonPressMask | ButtonReleaseMask);
    }
    else
    {
        pointerControl = false;
        XUngrabPointer(dpy, CurrentTime);
        XDefineCursor(dpy, drawWindow, None);
        XSelectInput(dpy, drawWindow, ButtonPressMask | ButtonReleaseMask);
    }
    XFlush(dpy);
}

void Talk_set_state(bool on)
{

    if (on)
    {
        /* Enable talking, disable pointer control if it is enabled. */
        if (pointerControl)
        {
            initialPointerControl = true;
            Pointer_control_set_state(false);
        }
        XSelectInput(dpy, drawWindow, PointerMotionMask | ButtonPressMask | ButtonReleaseMask);
        Talk_map_window(true);
    }
    else
    {
        /* Disable talking, enable pointer control if it was enabled. */
        Talk_map_window(false);
        if (initialPointerControl)
        {
            initialPointerControl = false;
            Pointer_control_set_state(true);
        }
    }
}

void Key_event(XEvent *event)
{
    KeySym ks;
    keys_t key;
    int change = false;
    bool (*key_do)(keys_t key);

    switch (event->type)
    {
    case KeyPress:
        key_do = Key_press;
        // Keyboard_button_pressed((xp_keysym_t)ks);
        break;
    case KeyRelease:
        key_do = Key_release;
        // Keyboard_button_released((xp_keysym_t)ks);
        break;
    default:
        return;
    }

    if ((ks = XLookupKeysym(&event->xkey, 0)) == NoSymbol)
    {
        return;
    }

    for (key = Lookup_key(event, ks, true);
         key != KEY_DUMMY;
         key = Lookup_key(event, ks, false))
    {

        change |= (*key_do)(key);
    }
    if (change)
    {
        Net_key_change();
    }
}

void Talk_event(XEvent *event)
{
    if (!Talk_do_event(event))
    {
        Talk_set_state(false);
    }
}

int talk_key_repeating;
XEvent talk_key_repeat_event;

void xevent_keyboard(int queued)
{
    int i, n;
    XEvent event;

    if (talk_key_repeating > 0)
    {
        if (++talk_key_repeating >= FPS && (talk_key_repeating - FPS) % ((FPS + 2) / 3) == 0)
        {
            Talk_event(&talk_key_repeat_event);
            if (!clData.talking)
                talk_key_repeating = 0;
        }
    }

    if (kdpy)
    {
        n = XEventsQueued(kdpy, queued);
        for (i = 0; i < n; i++)
        {
            XNextEvent(kdpy, &event);
            switch (event.type)
            {
            case KeyPress:
            case KeyRelease:
                Key_event(&event);
                break;

                /* Back in play */
            case FocusIn:
                gotFocus = true;
                XAutoRepeatOff(kdpy);
                break;

                /* Probably not playing now */
            case FocusOut:
            case UnmapNotify:
                gotFocus = false;
                XAutoRepeatOn(kdpy);
                break;

            case MappingNotify:
                XRefreshKeyboardMapping(&event.xmapping);
                break;
            }
        }
    }
}

ipos_t delta;
ipos_t mouse; /* position of mouse pointer. */
int movement; /* horizontal mouse movement. */

void xevent_pointer(void)
{
    XEvent event;

    if (pointerControl)
    {
        if (!clData.talking)
        {
            if (movement != 0)
            {
                Send_pointer_move(movement);
                delta.x = draw_width / 2 - mouse.x;
                delta.y = draw_height / 2 - mouse.y;
                if (ABS(delta.x) > 3 * draw_width / 8 || ABS(delta.y) > 1 * draw_height / 8)
                {

                    memset(&event, 0, sizeof(event));
                    event.type = MotionNotify;
                    event.xmotion.display = dpy;
                    event.xmotion.window = drawWindow;
                    event.xmotion.x = draw_width / 2;
                    event.xmotion.y = draw_height / 2;
                    XSendEvent(dpy, drawWindow, False, PointerMotionMask, &event);
                    XWarpPointer(dpy, None, drawWindow,
                                 0, 0, 0, 0,
                                 draw_width / 2, draw_height / 2);
                    XFlush(dpy);
                }
            }
        }
    }
}

int x_event(int new_input)
{
    int queued = 0, i, n;
    XEvent event;

#ifdef SOUND
    audioEvents();
#endif /* SOUND */

#ifdef JOYSTICK
    Joystick_event();
#endif /* JOYSTICK */

    movement = 0;

    switch (new_input)
    {
    case 0:
        queued = QueuedAlready;
        break;
    case 1:
        queued = QueuedAfterReading;
        break;
    case 2:
        queued = QueuedAfterFlush;
        break;
    default:
        warn("Bad input queue type (%d)", new_input);
        return -1;
    }
    n = XEventsQueued(dpy, queued);
    for (i = 0; i < n; i++)
    {
        XNextEvent(dpy, &event);

        switch (event.type)
        {
            /*
             * after requesting a selection we are notified that we
             * can access it.
             */
        case SelectionNotify:
            SelectionNotify_event(&event);
            break;
            /*
             * we are requested to provide a selection.
             */
        case SelectionRequest:
            SelectionRequest_event(&event);
            break;

        case SelectionClear:
            Clear_selection();
            break;

        case MapNotify:
            MapNotify_event(&event);
            break;

        case ClientMessage:
            if (ClientMessage_event(&event) == -1)
                return -1;
            break;

            /* Back in play */
        case FocusIn:
            FocusIn_event(&event);
            break;

            /* Probably not playing now */
        case FocusOut:
        case UnmapNotify:
            UnmapNotify_event(&event);
            break;

        case MappingNotify:
            XRefreshKeyboardMapping(&event.xmapping);
            break;

        case ConfigureNotify:
            ConfigureNotify_event(&event);
            break;

        case KeyPress:
            talk_key_repeating = 0;
            /* FALLTHROUGH */
        case KeyRelease:
            KeyChanged_event(&event);
            break;

        case ButtonPress:
            ButtonPress_event(&event);
            break;

        case MotionNotify:
            MotionNotify_event(&event);
            break;

        case ButtonRelease:
            if (ButtonRelease_event(&event) == -1)
                return -1;
            break;

        case Expose:
            Expose_event(&event);
            break;

        case EnterNotify:
        case LeaveNotify:
            Widget_event(&event);
            break;

        default:
            break;
        }
    }

    xevent_keyboard(queued);
    xevent_pointer();
    return 0;
}
