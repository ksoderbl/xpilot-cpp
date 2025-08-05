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

#ifndef MESSAGES_H
#define MESSAGES_H

#include "const.h"

#define MAX_MSGS 15       /* Max. messages displayed ever */
#define MAX_HIST_MSGS 128 /* maximum */

#define MSG_DURATION 1024
#define MSG_FLASH 892

/*
 * is a selection pending (in progress), done, drawn emphasized?
 */
#define SEL_NONE (1 << 0)
#define SEL_PENDING (1 << 1)
#define SEL_SELECTED (1 << 2)
#define SEL_EMPHASIZED (1 << 3)
typedef struct
{
    char txt[MSG_LEN];
    short len;
    short pixelLen;
    int life;
} message_t;

/*
 * a selection (text, string indices, state,...)
 */
typedef struct
{
    /* a selection in the talk window */
    struct
    {
        int state; /* current state of the selection */
        int x1;    /* string indices */
        int x2;
        bool incl_nl; /* include a `\n'? */
    } talk;
    /* a selection in the draw window */
    struct
    {
        int state;
        int x1; /* string indices (for TalkMsg[].txt) */
        int x2; /* they are modified when the emphasized area */
        int y1; /* is scrolled down by new messages coming in */
        int y2;
    } draw;
    char *txt;    /* allocated when needed */
    int txt_size; /* size of txt buffer */
    int len;
    /* when a message `jumps' from talk window to the player messages: */
    bool keep_emphasizing;
} selection_t;

extern message_t *TalkMsg[MAX_MSGS], *GameMsg[MAX_MSGS];
extern message_t *TalkMsg_pending[], *GameMsg_pending[];
extern char *HistoryMsg[MAX_HIST_MSGS];
extern char *HistoryBlock;
extern message_t *MsgBlock;
extern message_t *MsgBlock_pending;

extern selection_t selection;

extern int maxLinesInHistory; /* number of lines to save in history */
extern int maxMessages;
extern int messagesToStdout;
extern bool selectionAndHistory;

void Add_message(const char *message);
void Delete_pending_messages(void);
void Add_pending_messages(void);
int Alloc_msgs(void);
void Free_msgs(void);
int Alloc_history(void);
void Free_selectionAndHistory(void);
void Print_messages_to_stdout(void);

#endif
