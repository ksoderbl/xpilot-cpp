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

#include <cstring>

#include "other.h"
#include "messages.h"

Other::Other()
{
    // warn("Other::Other: Hello world!");
    ratio = 0.0;
    score = 0.0;
    check = 0;
    round = 0;
    timing_loops = 0;
    timing = 0;
    life = 0;
    alliance = 0;
    name_width = 0;
    name_len = 0;
    max_chars_in_names = -1;
    ignorelevel = 0;
}

Other::~Other()
{
    debugprint("Other::Other: Goodbye cruel world! (%s)", nick_name.c_str());
}

Other *self = nullptr; /* player info */
std::vector<Other *> others;

Other *Other_by_id(int id)
{
    int i;

    if (id != -1)
    {
        for (i = 0; i < others.size(); i++)
        {
            if (others[i]->id == id)
                return others[i];
        }
    }
    return nullptr;
}

Other *Other_by_name(const char *name, bool show_error_msg)
{
    int i;
    Other *found_other = nullptr, *other;
    size_t len;

    if (name == nullptr || (len = strlen(name)) == 0)
        goto match_none;

    /* Look for an exact match on player nickname. */
    for (i = 0; i < others.size(); i++)
    {
        other = others[i];
        if (!strcasecmp(other->nick_name.c_str(), name))
            return other;
    }

    /* Look if 'name' matches beginning of only one nick. */
    for (i = 0; i < others.size(); i++)
    {
        other = others[i];

        if (!strncasecmp(other->nick_name.c_str(), name, len))
        {
            if (found_other)
                goto match_several;
            found_other = other;
            continue;
        }
    }
    if (found_other)
        return found_other;

    /*
     * Check what players' name 'name' is a substring of (case insensitively).
     */
    for (i = 0; i < others.size(); i++)
    {
        int j;
        other = others[i];

        for (j = 0; j < 1 + (int)other->nick_name.length() - (int)len; j++)
        {
            if (!strncasecmp(other->nick_name.c_str() + j, name, len))
            {
                if (found_other)
                    goto match_several;
                found_other = other;
                break;
            }
        }
    }
    if (found_other)
        return found_other;

match_none:
{
    if (show_error_msg)
        Add_message("Name does not match any player. [*Client reply*]");
    return nullptr;
}
match_several:
{
    if (show_error_msg)
        Add_message("Name matches several players. [*Client reply*]");
    return nullptr;
}
}

ShipShape *Ship_by_id(int id)
{
    Other *other;

    if ((other = Other_by_id(id)) == nullptr)
        return Parse_shape_str("");
    return other->ship;
}
