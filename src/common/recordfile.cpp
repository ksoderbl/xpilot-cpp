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

#include "recordfile.h"

void RWriteByte(uint8_t i, FILE *fp)
{
    putc(i, fp);
}

void RWriteShort(int16_t i, FILE *fp)
{
    putc(i, fp);
    i >>= 8;
    putc(i, fp);
}

void RWriteUShort(uint16_t i, FILE *fp)
{
    putc(i, fp);
    i >>= 8;
    putc(i, fp);
}

void RWriteLong(int32_t i, FILE *fp)
{
    putc(i, fp);
    i >>= 8;
    putc(i, fp);
    i >>= 8;
    putc(i, fp);
    i >>= 8;
    putc(i, fp);
}

void RWriteULong(uint32_t i, FILE *fp)
{
    putc(i, fp);
    i >>= 8;
    putc(i, fp);
    i >>= 8;
    putc(i, fp);
    i >>= 8;
    putc(i, fp);
}

void RWriteString(char *str, FILE *fp)
{
    int len = strlen(str);
    int i;

    RWriteUShort(len, fp);
    for (i = 0; i < len; i++)
    {
        putc(str[i], fp);
    }
}
