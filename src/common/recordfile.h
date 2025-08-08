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

#ifndef RECORDFILE_H
#define RECORDFILE_H

#include <cstdio>
#include <cstdint>
#include <cstring>

#include <iostream>
#include <string>
#include <vector>

// Same info as in XColor
struct XPRColor
{
    uint32_t pixel;
    uint16_t red, green, blue;
    uint8_t flags; /* do_red, do_green, do_blue */
    uint8_t pad;
};

struct XPRHeader
{
    std::string nickname;   /* XPilot nick name of player */
    std::string realname;   /* login name of player */
    std::string hostname;   /* hostname of player */
    std::string servername; /* hostname of server */
    uint8_t fps;            /* frames per second of game */
    std::string recorddate; /* date of game played */
    std::vector<XPRColor> colors;
    std::string gameFontName;
    std::string msgFontName;
    uint16_t view_width;
    uint16_t view_height;
};

class RecordFile
{
};

void RWriteByte(uint8_t i, FILE *fp);
void RWriteShort(int16_t i, FILE *fp);
void RWriteUShort(uint16_t i, FILE *fp);
void RWriteLong(int32_t i, FILE *fp);
void RWriteULong(uint32_t i, FILE *fp);
// Remove support for char *, rename RWriteStdString to RWriteString.
void RWriteString(char *str, FILE *fp);
void RWriteStdString(std::string &str, FILE *fp);

void RWriteHeader(struct XPRHeader &hdr, FILE *fp);

#endif
