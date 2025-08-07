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
#include "recordfmt.h"

#include <iomanip> // for std::hex, std::setw, std::setfill

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

void RWriteStdString(std::string &str, FILE *fp)
{
    int len = str.length();
    int i;

    RWriteUShort(len, fp);
    for (i = 0; i < len; i++)
    {
        putc(str[i], fp);
    }
}

static void debugPrintHeader(XPRHeader &hdr)
{
    std::cout << "XPRHeader:" << std::endl;
    std::cout << "nickname: " << hdr.nickname << std::endl;
    std::cout << "realname: " << hdr.realname << std::endl;
    std::cout << "hostname: " << hdr.hostname << std::endl;
    std::cout << "servername: " << hdr.servername << std::endl;
    std::cout << "recorddate: " << hdr.recorddate << std::endl;
    int i = 0;
    for (XPRColor color : hdr.colors)
    {
        std::cout << "Color " << i++ << std::endl
                  << std::hex << std::setw(8) << std::setfill('0') << static_cast<int>(color.pixel) << std::endl
                  << std::setw(8) << std::setfill('0') << static_cast<int>(color.red) << std::endl
                  << std::setw(8) << std::setfill('0') << static_cast<int>(color.green) << std::endl
                  << std::setw(8) << std::setfill('0') << static_cast<int>(color.blue) << std::endl
                  << std::dec << std::endl; // Reset to decimal
    }
    std::cout << "gameFontName: " << hdr.gameFontName << std::endl;
    std::cout << "msgFontName: " << hdr.msgFontName << std::endl;
    std::cout << "view_width: " << hdr.view_width << std::endl;
    std::cout << "view_height: " << hdr.view_height << std::endl;
}

void RWriteHeader(XPRHeader &hdr, FILE *fp)
{
    int i;

    rewind(fp);

    // First write out magic 4 letter word
    RWriteByte('X', fp);
    RWriteByte('P', fp);
    RWriteByte('R', fp);
    RWriteByte('C', fp);

    // Write which version of the XPilot Record Protocol this is.
    RWriteByte(RC_MAJORVERSION, fp);
    RWriteByte('.', fp);
    RWriteByte(RC_MINORVERSION, fp);
    RWriteByte('\n', fp);

    // Write player's nick, login, host, server, FPS and the date.
    RWriteStdString(hdr.nickname, fp);
    RWriteStdString(hdr.realname, fp);
    RWriteStdString(hdr.hostname, fp);
    RWriteStdString(hdr.servername, fp);
    RWriteByte(hdr.fps, fp);
    RWriteStdString(hdr.recorddate, fp);

    // Write info about graphics setup.
    int maxColors = hdr.colors.size();
    RWriteByte(maxColors, fp);
    for (XPRColor color : hdr.colors)
    {
        RWriteULong(color.pixel, fp);
        RWriteUShort(color.red, fp);
        RWriteUShort(color.green, fp);
        RWriteUShort(color.blue, fp);
    }
    RWriteStdString(hdr.gameFontName, fp);
    RWriteStdString(hdr.msgFontName, fp);

    RWriteUShort(hdr.view_width, fp);
    RWriteUShort(hdr.view_height, fp);

    debugPrintHeader(hdr);
}
