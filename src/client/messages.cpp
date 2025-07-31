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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>

// #ifndef _WINDOWS
// # include <unistd.h>
// # include <X11/Xlib.h>
// # include <X11/Xos.h>
// #endif

#include "xpconfig.h"
#include "const.h"
#include "xperror.h"
#include "bit.h"
// #include "types.h"
// #include "keys.h"
// #include "rules.h"
// #include "setup.h"
// #include "xpaint.h"
// #include "paintdata.h"
// #include "paintmacros.h"
// #include "record.h"
// #include "xinit.h"
// #include "protoclient.h"
// #include "blockbitmaps.h"
#include "commonproto.h"
#include "messages.h"


// extern setup_t                *Setup;
// extern int                RadarHeight;
// extern score_object_t        score_objects[MAX_SCORE_OBJECTS];
// extern int                score_object;
// extern XGCValues        gcv;

// int        hudColor;                /* Color index for HUD drawing */
// int        hudLockColor;                /* Color index for lock on HUD drawing */
// int        oldMessagesColor;        /* Color index for old messages */
// DFLOAT        charsPerTick = 0.0;        /* Output speed of messages */

message_t        *TalkMsg[MAX_MSGS], *GameMsg[MAX_MSGS];
/* store incoming messages while a cut is pending */
message_t        *TalkMsg_pending[MAX_MSGS], *GameMsg_pending[MAX_MSGS];
/* history of the talk window */
char                *HistoryMsg[MAX_HIST_MSGS];

/* provide cut&paste and message history */
selection_t        selection;
static        char                *HistoryBlock = NULL;
extern        char                *HistoryMsg[MAX_HIST_MSGS];
bool                        selectionAndHistory = false;
int                        maxLinesInHistory;
int        maxMessages;                /* Max. number of messages to display */
int        messagesToStdout;        /* Send messages to standard output */

// #ifndef _WINDOWS
// /* selection in talk- or draw-window */
// extern selection_t selection;
// extern void Delete_pending_messages(void);
// #endif

static message_t        *MsgBlock = NULL;
static message_t        *MsgBlock_pending = NULL;

int Alloc_msgs(void)
{
    message_t                *x, *x2 = 0;
    int                        i;

    if ((x = (message_t *)malloc(2 * MAX_MSGS * sizeof(message_t))) == NULL){
        xperror("No memory for messages");
        return -1;
    }

    if (selectionAndHistory &&
        ((x2 = (message_t *)malloc(2 * MAX_MSGS * sizeof(message_t))) == NULL)){
        xperror("No memory for history messages");
        free(x);
        return -1;
    }
    if (selectionAndHistory) {
        MsgBlock_pending        = x2;
    }

    MsgBlock                = x;

    for (i = 0; i < 2 * MAX_MSGS; i++) {
        if (i < MAX_MSGS) {
            TalkMsg[i] = x;
            if (selectionAndHistory) TalkMsg_pending[i] = x2;
        } else {
            GameMsg[i - MAX_MSGS] = x;
            if (selectionAndHistory) GameMsg_pending[i - MAX_MSGS] = x2;
        }
        x->txt[0] = '\0';
        x->len = 0;
        x->life = 0;
        x++;

        if (selectionAndHistory) {
            x2->txt[0] = '\0';
            x2->len = 0;
            x2->life = 0;
            x2++;
        }
    }
    return 0;
}

void Free_msgs(void)
{
    if (MsgBlock) {
        free(MsgBlock);
        MsgBlock = NULL;
    }
    if (MsgBlock_pending) {
        free(MsgBlock_pending);
        MsgBlock_pending = NULL;
    }
}

int Alloc_history(void)
{
    char        *hist_ptr;
    int                i;

    /* maxLinesInHistory is a runtime constant */
    if ((hist_ptr = (char *)malloc(maxLinesInHistory * MAX_CHARS)) == NULL) {
        xperror("No memory for history");
        return -1;
    }
    HistoryBlock        = hist_ptr;

    for (i = 0; i < maxLinesInHistory; i++) {
        HistoryMsg[i]        = hist_ptr;
        hist_ptr[0]        = '\0';
        hist_ptr        += MAX_CHARS;
    }
    return 0;
}

void Free_selectionAndHistory(void)
{
    if (HistoryBlock) {
        free(HistoryBlock);
        HistoryBlock = NULL;
    }
    if (selection.txt) {
        free(selection.txt);
        selection.txt = NULL;
    }
}

/*
 * add an incoming talk/game message.
 * however, buffer new messages if there is a pending selection.
 * Add_pending_messages() will be called later in Talk_cut_from_messages().
 */
void Add_message(const char *message)
{
    int                        i, len;
    message_t                *tmp, **msg_set;
    bool                is_drawn_talk_message        = false; /* not pending */
    int                        last_msg_index;
    bool                scrolling                = false; /* really moving */

    len = strlen(message);
    if (message[len - 1] == ']' || strncmp(message, " <", 2) == 0) {
        if (selectionAndHistory && selection.draw.state == SEL_PENDING) {
            /* the buffer for the pending messages */
            msg_set = TalkMsg_pending;
        } else {
            msg_set = TalkMsg;
            is_drawn_talk_message = true;
        }
    } else {
        if (selectionAndHistory && selection.draw.state == SEL_PENDING) {
            msg_set = GameMsg_pending;
        } else {
            msg_set = GameMsg;
        }
    }

    if (selectionAndHistory && is_drawn_talk_message) {
        /* how many talk messages */
        last_msg_index = 0;
        while (last_msg_index < maxMessages
                && TalkMsg[last_msg_index]->len != 0) {
            last_msg_index++;
        }
        last_msg_index--; /* make it an index */

        /*
         * keep the emphasizing (`jumping' from talk window to talk messages)
         */
        if (selection.keep_emphasizing) {
            selection.keep_emphasizing = false;
            selection.talk.state = SEL_NONE;
            selection.draw.state = SEL_EMPHASIZED;
            selection.draw.y1 = -1;
            selection.draw.y2 = -1;
        } /* talk window emphasized */
    } /* talk messages */

    tmp = msg_set[maxMessages - 1];
    for (i = maxMessages - 1; i > 0; i--) {
        msg_set[i] = msg_set[i - 1];
    }
    msg_set[0] = tmp;

    msg_set[0]->life = MSG_DURATION;
    strlcpy(msg_set[0]->txt, message, MSG_LEN);
    msg_set[0]->len = len;

    /*
     * scroll also the emphasizing
     */
    if (selectionAndHistory && is_drawn_talk_message
          && selection.draw.state == SEL_EMPHASIZED ) {

        if ((scrolling && selection.draw.y2 == 0)
              || (selection.draw.y1 == maxMessages - 1)) {
            /*
             * the emphasizing vanishes, as it's `last' line
             * is `scrolled away'
             */
            selection.draw.state = SEL_SELECTED;        
        } else {
            if (scrolling) {
                selection.draw.y2--;
                if ( selection.draw.y1 == 0) {
                    selection.draw.x1 = 0;
                } else {
                    selection.draw.y1--;
                }
            } else {
                selection.draw.y1++;
                if (selection.draw.y2 == maxMessages - 1) {
                    selection.draw.x2 = msg_set[selection.draw.y2]->len - 1;
                } else {
                    selection.draw.y2++;
                }
            }
        }
    }

#ifdef DEVELOPMENT
    /* Anti-censor hack restores original 4 letter words.
     * XPilot is not assumed to be a game for children
     * who are still under parental guidance.
     */
    for (i = 0; i < len - 3; i++) {
        static char censor_text[] = "@&$*";
        static char rough_text[][5] = { "fuck", "shit", "damn" };
        static int rough_index = 0;
        if (msg_set[0]->txt[i] == censor_text[0]
            && !strncmp(&msg_set[0]->txt[i], censor_text, 4)) {
            if (++rough_index >= 3) {
                rough_index = 0;
            }
            memcpy(&msg_set[0]->txt[i], rough_text[rough_index], 4);
        }
    }
#endif

    // TODO
    // msg_set[0]->pixelLen = XTextWidth(messageFont, msg_set[0]->txt, msg_set[0]->len);

    /* Print messages to standard output.
     */
    if (messagesToStdout == 2 ||
        (messagesToStdout == 1 &&
         message[0] &&
         message[strlen(message)-1] == ']')) {

        xpprintf("%s\n", message);
    }
}


/*
 * clear the buffer for the pending messages
 */
void Delete_pending_messages(void)
{
    message_t* msg;
    int i;
    if (!selectionAndHistory)
        return;

    for (i = 0; i < maxMessages; i++) {
        msg = TalkMsg_pending[i];
        if (msg->len > 0) {
            msg->txt[0] = '\0';
            msg->len = 0;
        }
        msg = GameMsg_pending[i];
        if (msg->len > 0) {
            msg->txt[0] = '\0';
            msg->len = 0;
        }
    }
}


// /*
//  * after a pending cut has been completed,
//  * add the (buffered) messages which were coming in meanwhile.
//  */
// void Add_pending_messages(void)
// {
//     int                        i;

//     if (!selectionAndHistory)
//         return;
//     /* just through all messages */
//     for (i = maxMessages-1; i >= 0; i--) {
//         if (TalkMsg_pending[i]->len > 0) {
//             Add_message(TalkMsg_pending[i]->txt);
//         }
//         if (GameMsg_pending[i]->len > 0) {
//             Add_message(GameMsg_pending[i]->txt);
//         }
//     }
//     Delete_pending_messages();
// }
// #endif


/*
 * Print all available messages to stdout.
 */
void Print_messages_to_stdout(void)
{
    int i, k;
    int direction, offset;

    if (!selectionAndHistory)
        return;

    direction        = 1;
    offset                = 0;

    xpprintf("[talk messages]\n");
    for (k = 0; k < maxMessages; k++) {
        i = direction * k + offset;
        if (TalkMsg[i] && TalkMsg[i]->len > 0) {
            xpprintf("  %s\n", TalkMsg[i]->txt);
        }
    }

    xpprintf("[server messages]\n");
    for (k = maxMessages - 1; k >= 0; k--) {
        i = direction * k + offset;
        if (GameMsg[i] && GameMsg[i]->len > 0) {
            xpprintf("  %s\n", GameMsg[i]->txt);
        }
    }
    xpprintf("\n");
}
