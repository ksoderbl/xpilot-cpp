/*
 * XPilot, a multiplayer gravity war game.  Copyright (C) 1991-2004 by
 *
 *      Bjørn Stabell
 *      Ken Ronny Schouten
 *      Bert Gijsbers
 *      Dick Balaska
 *      Erik Andersson
 *      Kristian Söderblom
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

#include "messages.h"

// #include <stdlib.h>
#include <cstring>
#include <cstdio>
// #include <errno.h>
// #include <math.h>
// #include <sys/types.h>

// #include "const.h"

#include "bit.h"
#include "commonmacros.h"
#include "commonproto.h"
#include "rules.h"
#include "xpconfig.h"
#include "xperror.h"

#include "client.h"
#include "netclient.h"
#include "clientrank.h"
#include "clientsetup.h"
#include "msg-parser.h"

message_t *TalkMsg[MAX_MSGS], *GameMsg[MAX_MSGS];
message_t *TalkMsg_pending[MAX_MSGS], *GameMsg_pending[MAX_MSGS];
char *HistoryMsg[MAX_HIST_MSGS];

/* provide cut&paste and message history */
static char *HistoryBlock = nullptr;
int maxLinesInHistory = 32;
int maxMessages = 8;      /* Max. number of messages to display */
int messagesToStdout = 0; /* Send messages to standard output */

static message_t *MsgBlock = nullptr;
static message_t *MsgBlock_pending = nullptr;

// TODO: move this to msg-parser.cpp
/*
 * Checks if the message is sent by someone in your team.
 * In 'bracket' we will store info about where the
 * player name starts so the bms does can ignore that.
 */
static bool Msg_is_from_our_team(const char *message, const char **msg2)
{
    Other *other;
    static char buf[MAX_CHARS + 8];
    size_t bufstrlen, len;
    int i;

    if (self == nullptr)
        return false;

    len = strlen(message);
    for (i = 0; i < others.size(); i++)
    {
        other = others[i];
        if (other->team != self->team)
            continue;

        /* first check if someone in your team sent the message for all */
        sprintf(buf, "[%s]", other->nick_name.c_str());
        bufstrlen = strlen(buf);
        if (len < bufstrlen)
            continue;
        if (!strcmp(&message[len - bufstrlen], buf))
        {
            *msg2 = buf;
            strlcpy(buf, message, len - bufstrlen);
            return true;
        }

        /* if not, check if it was sent to your team only */
        sprintf(buf, "[%s]:[%d]", other->nick_name.c_str(), other->team);
        bufstrlen = strlen(buf);
        if (len < bufstrlen)
            continue;
        if (!strcmp(&message[len - bufstrlen], buf))
        {
            *msg2 = buf;
            strlcpy(buf, message, len - bufstrlen);
            return true;
        }
    }
    return false;
}

int Alloc_msgs(void)
{
    message_t *x, *x2 = nullptr;
    int i;

    x = XMALLOC(message_t, 2 * MAX_MSGS);
    if (x == nullptr)
    {
        error("No memory for messages");
        return -1;
    }

    x2 = XMALLOC(message_t, 2 * MAX_MSGS);
    if (x2 == nullptr)
    {
        error("No memory for history messages");
        free(x);
        return -1;
    }

    MsgBlock_pending = x2;
    MsgBlock = x;

    for (i = 0; i < 2 * MAX_MSGS; i++)
    {
        if (i < MAX_MSGS)
        {
            TalkMsg[i] = x;
            TalkMsg_pending[i] = x2;
        }
        else
        {
            GameMsg[i - MAX_MSGS] = x;
            GameMsg_pending[i - MAX_MSGS] = x2;
        }
        x->txt[0] = '\0';
        x->len = 0;
        x->lifeTime = 0.0;
        x++;

        x2->txt[0] = '\0';
        x2->len = 0;
        x2->lifeTime = 0.0;
        x2++;
    }
    return 0;
}

void Free_msgs(void)
{
    XFREE(MsgBlock);
    XFREE(MsgBlock_pending);
}

int Alloc_history(void)
{
    char *hist_ptr;
    int i;

    /* maxLinesInHistory is a runtime constant */
    hist_ptr = XMALLOC(char, (size_t)maxLinesInHistory *MAX_CHARS);
    if (hist_ptr == nullptr)
    {
        error("No memory for history");
        return -1;
    }
    HistoryBlock = hist_ptr;

    for (i = 0; i < maxLinesInHistory; i++)
    {
        HistoryMsg[i] = hist_ptr;
        hist_ptr[0] = '\0';
        hist_ptr += MAX_CHARS;
    }
    return 0;
}

void Free_selectionAndHistory(void)
{
    XFREE(HistoryBlock);
    XFREE(selection.txt);
}

/*
 * add an incoming talk/game message.
 * however, buffer new messages if there is a pending selection.
 * Add_pending_messages() will be called later in Talk_cut_from_messages().
 */
void Add_message(const char *message)
{
    // warn("ADD MESSAGE: %s", message);

    int i, last_msg_index;
    message_t *msg, **msg_set;
    msg_bms_t bmsinfo = BmsNone;
    const char *msg2;
    bool is_game_msg = false;
    bool is_drawn_talk_message = false; /* not pending */
    bool scrolling = false;             /* really moving */

    is_game_msg = Msg_is_game_msg(message);
    if (!is_game_msg)
    {
        if (selection.draw.state == SEL_PENDING)
        {
            /* the buffer for the pending messages */
            msg_set = TalkMsg_pending;
        }
        else
        {
            msg_set = TalkMsg;
            is_drawn_talk_message = true;
        }
    }
    else
    {
        if (selection.draw.state == SEL_PENDING)
            msg_set = GameMsg_pending;
        else
            msg_set = GameMsg;
    }

    if (is_game_msg)
        Msg_scan_game_msg(message);

    else if (Msg_is_in_angle_brackets(message))
        Msg_scan_angle_bracketed_msg(message);

    else if (!is_game_msg && BIT(Setup->mode, TEAM_PLAY) && Msg_is_from_our_team(message, &msg2))
        bmsinfo = Msg_do_bms(msg2);

    if (is_drawn_talk_message)
    {
        /* how many talk messages */
        last_msg_index = 0;
        while (last_msg_index < maxMessages && TalkMsg[last_msg_index]->len != 0)
            last_msg_index++;
        last_msg_index--; /* make it an index */

        /*
         * keep the emphasizing ('jumping' from talk window to talk messages)
         */
        if (selection.keep_emphasizing)
        {
            selection.keep_emphasizing = false;
            selection.talk.state = SEL_NONE;
            selection.draw.state = SEL_EMPHASIZED;
            selection.draw.y1 = -1;
            selection.draw.y2 = -1;
        } /* talk window emphasized */
    } /* talk messages */

    msg = msg_set[maxMessages - 1];
    for (i = maxMessages - 1; i > 0; i--)
        msg_set[i] = msg_set[i - 1];

    msg_set[0] = msg;
    msg->lifeTime = MSG_LIFE_TIME;
    strlcpy(msg->txt, message, MSG_LEN);
    msg->len = strlen(message);
    msg->bmsinfo = bmsinfo;

    /* Clear bms flags for out of date messages. */
    if (bmsinfo != BmsNone)
        Bms_set_state(bmsinfo);

    /*
     * scroll also the emphasizing
     */
    if (is_drawn_talk_message && selection.draw.state == SEL_EMPHASIZED)
    {

        if ((scrolling && selection.draw.y2 == 0) || (selection.draw.y1 == maxMessages - 1))
        {
            /*
             * the emphasizing vanishes, as it's 'last' line
             * is 'scrolled away'
             */
            selection.draw.state = SEL_SELECTED;
        }
        else
        {
            if (scrolling)
            {
                selection.draw.y2--;
                if (selection.draw.y1 == 0)
                    selection.draw.x1 = 0;
                else
                    selection.draw.y1--;
            }
            else
            {
                selection.draw.y1++;
                if (selection.draw.y2 == maxMessages - 1)
                    selection.draw.x2 = msg_set[selection.draw.y2]->len - 1;
                else
                    selection.draw.y2++;
            }
        }
    }

    /* Print messages to standard output.
     */
    if (messagesToStdout == 2 ||
        (messagesToStdout == 1 &&
         message[0] &&
         message[strlen(message) - 1] == ']'))
        printf("%s\n", message);
}

/*
 * clear the buffer for the pending messages
 */
void Delete_pending_messages(void)
{
    message_t *msg;
    int i;

    for (i = 0; i < maxMessages; i++)
    {
        msg = TalkMsg_pending[i];
        if (msg->len > 0)
        {
            msg->txt[0] = '\0';
            msg->len = 0;
        }
        msg = GameMsg_pending[i];
        if (msg->len > 0)
        {
            msg->txt[0] = '\0';
            msg->len = 0;
        }
    }
}

/*
 * after a pending cut has been completed,
 * add the (buffered) messages which were coming in meanwhile.
 */
void Add_pending_messages(void)
{
    int i;

    /* just through all messages */
    for (i = maxMessages - 1; i >= 0; i--)
    {
        if (TalkMsg_pending[i]->len > 0)
            Add_message(TalkMsg_pending[i]->txt);
        if (GameMsg_pending[i]->len > 0)
            Add_message(GameMsg_pending[i]->txt);
    }
    Delete_pending_messages();
}

/*
 * Print all available messages to stdout.
 */
void Print_messages_to_stdout(void)
{
    int i;

    printf("[talk messages]\n");
    for (i = 0; i < maxMessages; i++)
    {
        if (TalkMsg[i] && TalkMsg[i]->len > 0)
            printf("  %s\n", TalkMsg[i]->txt);
    }

    printf("[server messages]\n");
    for (i = maxMessages - 1; i >= 0; i--)
    {
        if (GameMsg[i] && GameMsg[i]->len > 0)
            printf("  %s\n", GameMsg[i]->txt);
    }
    printf("\n");
}
