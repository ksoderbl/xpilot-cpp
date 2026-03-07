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
#include <cstring>
#include <cctype>
#include <cerrno>
#include <climits>

#include <unistd.h>
#include <pwd.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>

#include "commonmacros.h"
#include "const.h"
#include "strdup.h"
#include "strlcpy.h"

#include "client.h"
#include "messages.h"
#include "paint.h"

#include "keydefs.h"

#include "xpconfig.h"
#include "xpaint.h"
#include "xinit.h"
#include "bit.h"
#include "keys.h"
#include "netclient.h"
#include "widget.h"
#include "configure.h"
#include "setup.h"
#include "xperror.h"
#include "portability.h"

// NEW

static int Config_creator(xp_option_t *opt, int widget_desc, int *height);
static int Config_create_save(int widget_desc, int *height);
static int Config_close(int widget_desc, void *data, const char **strptr);
static int Config_next(int widget_desc, void *data, const char **strptr);
static int Config_prev(int widget_desc, void *data, const char **strptr);
static int Config_save(int widget_desc, void *data, const char **strptr);
static int Config_save_confirm_callback(int widget_desc, void *popup_desc,
                                        const char **strptr);

static int num_default_options = 0;
static int max_default_options = 0;
static int *default_option_indices = NULL;
static int num_color_options = 0;
static int max_color_options = 0;
static int *color_option_indices = NULL;

static bool config_created = false,
            config_mapped = false;
static int config_page,
    config_x,
    config_y,
    config_width,
    config_height,
    config_space,
    config_max,
    config_button_space,
    config_text_space,
    config_text_height,
    config_button_height,
    config_entry_height,
    config_bool_width,
    config_bool_height,
    config_int_width,
    config_double_width,
    config_arrow_width,
    config_arrow_height;
static int *config_widget_desc,
    config_save_confirm_desc = NO_WIDGET;

static int *config_widget_ids = NULL;
static int config_what = CONFIG_NONE;

/* this must be updated if new config menu items are added */
static int Nelem_config_creator(void)
{
    if (config_what == CONFIG_DEFAULT)
        return num_default_options + 1;
    if (config_what == CONFIG_COLORS)
        return num_color_options + 1;
    return 0;
}

static xp_option_t *Config_creator_option(int i)
{
    int ind = -1;

    if (config_what == CONFIG_DEFAULT && i >= 0 && i < num_default_options)
        ind = default_option_indices[i];
    if (config_what == CONFIG_COLORS && i >= 0 && i < num_color_options)
        ind = color_option_indices[i];
    return Option_by_index(ind);
}

static int Update_bool_option(int widget_desc, void *data, bool *val)
{
    xp_option_t *opt = (xp_option_t *)data;

    Set_bool_option(opt, *val, xp_option_origin_config);

    return 0;
}

static int Update_int_option(int widget_desc, void *data, int *val)
{
    xp_option_t *opt = (xp_option_t *)data;

    Set_int_option(opt, *val, xp_option_origin_config);

    return 0;
}

static int Update_double_option(int widget_desc, void *data, double *val)
{
    xp_option_t *opt = (xp_option_t *)data;

    Set_double_option(opt, *val, xp_option_origin_config);

    return 0;
}

static void Create_config(void)
{
    int i,
        num,
        height,
        offset,
        width,
        widget_desc;
    bool full;

    /*
     * Window dimensions relative to the top window.
     */
    config_x = 0;
    config_y = RadarHeight + ButtonHeight + 2;
    config_width = 256;
    config_height = top_height - config_y;

    /*
     * Space between label-text and label-border.
     */
    config_text_space = 3;
    /*
     * Height of a label window.
     */
    config_text_height = 2 * 1 + textFont->ascent + textFont->descent;

    /*
     * Space between button-text and button-border.
     */
    config_button_space = 3;
    /*
     * Height of a button window.
     */
    config_button_height = buttonFont->ascent + buttonFont->descent + 2 * 1;

    config_entry_height = MAX(config_text_height, config_button_height);

    /*
     * Space between entries and between an entry and the border.
     */
    config_space = 6;

    /*
     * Sizes of the different widget types.
     */
    config_bool_width = XTextWidth(buttonFont, "Yes", 3) + 2 * config_button_space;
    config_bool_height = config_button_height;
    config_arrow_height = config_text_height;
    config_arrow_width = config_text_height;
    config_int_width = 4 + XTextWidth(buttonFont, "10000", 5);
    config_double_width = 4 + XTextWidth(buttonFont, "0.222", 5);

    config_max = Nelem_config_creator();
    config_widget_desc = XMALLOC(int, config_max);
    if (config_widget_desc == NULL)
    {
        error("No memory for config");
        return;
    }

    num = -1;
    full = true;
    for (i = 0; i < Nelem_config_creator(); i++)
    {
        xp_option_t *opt = Config_creator_option(i);

        if (full)
        {
            full = false;
            num++;
            config_widget_desc[num] = Widget_create_form(NO_WIDGET, topWindow,
                                                         config_x, config_y,
                                                         config_width, config_height,
                                                         0);
            if (config_widget_desc[num] == 0)
                break;

            height = config_height - config_space - config_button_height;
            width = 2 * config_button_space + XTextWidth(buttonFont,
                                                         "PREV", 4);
            offset = (config_width - width) / 2;
            widget_desc =
                Widget_create_activate(config_widget_desc[num],
                                       offset, height,
                                       width, config_button_height,
                                       0, "PREV", Config_prev,
                                       (void *)(long)num);
            if (widget_desc == 0)
                break;

            width = 2 * config_button_space + XTextWidth(buttonFont,
                                                         "NEXT", 4);
            offset = config_width - width - config_space;
            widget_desc =
                Widget_create_activate(config_widget_desc[num],
                                       offset, height,
                                       width, config_button_height,
                                       0, "NEXT", Config_next,
                                       (void *)(long)num);
            if (widget_desc == 0)
                break;

            width = 2 * config_button_space + XTextWidth(buttonFont,
                                                         "CLOSE", 5);
            offset = config_space;
            widget_desc =
                Widget_create_activate(config_widget_desc[num],
                                       offset, height,
                                       width, config_button_height,
                                       0, "CLOSE", Config_close,
                                       (void *)(long)num);
            if (widget_desc == 0)
                break;

            height = config_space;
        }

        if (opt == NULL && i != Nelem_config_creator() - 1)
            assert(0);

        if ((config_widget_ids[i] =
                 Config_creator(opt, config_widget_desc[num], &height)) == 0)
        {
            i--;
            full = true;
            if (height == config_space)
                break;
            continue;
        }
    }
    if (i < Nelem_config_creator())
    {
        for (; num >= 0; num--)
        {
            if (config_widget_desc[num] != 0)
                Widget_destroy(config_widget_desc[num]);
        }
        config_created = false;
        config_mapped = false;
    }
    else
    {
        config_max = num + 1;
        config_widget_desc = XREALLOC(int, config_widget_desc, config_max);
        config_page = 0;
        for (i = 0; i < config_max; i++)
            Widget_map_sub(config_widget_desc[i]);
        config_created = true;
        config_mapped = false;
    }
}

static int Config_close(int widget_desc, void *data, const char **strptr)
{
    Widget_unmap(config_widget_desc[config_page]);
    config_mapped = false;
    return 0;
}

static int Config_next(int widget_desc, void *data, const char **strptr)
{
    int prev_page = config_page;

    if (config_max > 1)
    {
        config_page = (config_page + 1) % config_max;
        Widget_raise(config_widget_desc[config_page]);
        Widget_unmap(config_widget_desc[prev_page]);
        config_mapped = true;
    }
    return 0;
}

static int Config_prev(int widget_desc, void *data, const char **strptr)
{
    int prev_page = config_page;

    if (config_max > 1)
    {
        config_page = (config_page - 1 + config_max) % config_max;
        Widget_raise(config_widget_desc[config_page]);
        Widget_unmap(config_widget_desc[prev_page]);
        config_mapped = true;
    }
    return 0;
}

static int Config_create_bool(int widget_desc, int *height,
                              const char *str, bool val,
                              int (*callback)(int, void *, bool *),
                              void *data)
{
    int offset,
        label_width,
        boolw;

    if (*height + 2 * config_entry_height + 2 * config_space >= config_height)
        return 0;
    label_width = XTextWidth(textFont, str, (int)strlen(str)) + 2 * config_text_space;
    offset = config_width - (config_space + config_bool_width);
    if (config_space + label_width > offset)
    {
        if (*height + 3 * config_entry_height + 2 * config_space >= config_height)
            return 0;
    }

    Widget_create_label(widget_desc, config_space, *height + (config_entry_height - config_text_height) / 2,
                        label_width, config_text_height, true,
                        0, str);
    if (config_space + label_width > offset)
        *height += config_entry_height;
    boolw = Widget_create_bool(widget_desc,
                               offset, *height + (config_entry_height - config_bool_height) / 2,
                               config_bool_width,
                               config_bool_height,
                               0, val, callback, data);
    *height += config_entry_height + config_space;

    return boolw;
}

static int Config_create_int(int widget_desc, int *height,
                             const char *str, int *val, int min, int max,
                             int (*callback)(int, void *, int *), void *data)
{
    int offset,
        label_width,
        intw;

    if (*height + 2 * config_entry_height + 2 * config_space >= config_height)
        return 0;
    label_width = XTextWidth(textFont, str, (int)strlen(str)) + 2 * config_text_space;
    offset = config_width - (config_space + 2 * config_arrow_width + config_int_width);
    if (config_space + label_width > offset)
    {
        if (*height + 3 * config_entry_height + 2 * config_space >= config_height)
            return 0;
    }
    Widget_create_label(widget_desc, config_space, *height + (config_entry_height - config_text_height) / 2,
                        label_width, config_text_height, true,
                        0, str);
    if (config_space + label_width > offset)
        *height += config_entry_height;
    intw = Widget_create_int(widget_desc, offset, *height + (config_entry_height - config_text_height) / 2,
                             config_int_width, config_text_height,
                             0, val, min, max, callback, data);
    offset += config_int_width;
    Widget_create_arrow_left(widget_desc, offset, *height + (config_entry_height - config_arrow_height) / 2,
                             config_arrow_width, config_arrow_height,
                             0, intw);
    offset += config_arrow_width;
    Widget_create_arrow_right(widget_desc, offset, *height + (config_entry_height - config_arrow_height) / 2,
                              config_arrow_width, config_arrow_height,
                              0, intw);
    *height += config_entry_height + config_space;

    return intw;
}

static int Config_create_color(int widget_desc, int *height, int color,
                               const char *str, int *val, int min, int max,
                               int (*callback)(int, void *, int *), void *data)
{
    int offset, label_width, colw;

    if (*height + 2 * config_entry_height + 2 * config_space >= config_height)
        return 0;
    label_width = XTextWidth(textFont, str, (int)strlen(str)) + 2 * config_text_space;
    offset = config_width - (config_space + 2 * config_arrow_width + config_int_width);
    if (config_space + label_width > offset)
    {
        if (*height + 3 * config_entry_height + 2 * config_space >= config_height)
            return 0;
    }
    Widget_create_label(widget_desc, config_space, *height + (config_entry_height - config_text_height) / 2,
                        label_width, config_text_height, true,
                        0, str);
    if (config_space + label_width > offset)
        *height += config_entry_height;
    colw = Widget_create_color(widget_desc, color, offset, *height + (config_entry_height - config_text_height) / 2,
                               config_int_width, config_text_height,
                               0, val, min, max, callback, data);
    offset += config_int_width;
    Widget_create_arrow_left(widget_desc, offset, *height + (config_entry_height - config_arrow_height) / 2,
                             config_arrow_width, config_arrow_height,
                             0, colw);
    offset += config_arrow_width;
    Widget_create_arrow_right(widget_desc, offset, *height + (config_entry_height - config_arrow_height) / 2,
                              config_arrow_width, config_arrow_height,
                              0, colw);
    *height += config_entry_height + config_space;

    return colw;
}

static int Config_create_double(int widget_desc, int *height,
                                const char *str, double *val,
                                double min, double max,
                                int (*callback)(int, void *, double *),
                                void *data)
{
    int offset,
        label_width,
        doublew;

    if (*height + 2 * config_entry_height + 2 * config_space >= config_height)
        return 0;
    label_width = XTextWidth(textFont, str, (int)strlen(str)) + 2 * config_text_space;
    offset = config_width - (config_space + 2 * config_arrow_width + config_double_width);
    if (config_space + label_width > offset)
    {
        if (*height + 3 * config_entry_height + 2 * config_space >= config_height)
            return 0;
    }
    Widget_create_label(widget_desc, config_space, *height + (config_entry_height - config_text_height) / 2,
                        label_width, config_text_height, true,
                        0, str);
    if (config_space + label_width > offset)
        *height += config_entry_height;
    doublew = Widget_create_double(widget_desc, offset, *height + (config_entry_height - config_text_height) / 2,
                                   config_double_width, config_text_height,
                                   0, val, min, max, callback, data);
    offset += config_double_width;
    Widget_create_arrow_left(widget_desc, offset, *height + (config_entry_height - config_arrow_height) / 2,
                             config_arrow_width, config_arrow_height,
                             0, doublew);
    offset += config_arrow_width;
    Widget_create_arrow_right(widget_desc, offset, *height + (config_entry_height - config_arrow_height) / 2,
                              config_arrow_width, config_arrow_height,
                              0, doublew);
    *height += config_entry_height + config_space;

    return doublew;
}

static int Config_create_save(int widget_desc, int *height)
{
    static char save_str[] = "Save Configuration";
    int space,
        button_desc,
        width = 2 * config_button_space + XTextWidth(buttonFont, save_str,
                                                     (int)strlen(save_str));

    space = config_height - (*height + 2 * config_entry_height + 2 * config_space);
    if (space < 0)
        return 0;
    button_desc =
        Widget_create_activate(widget_desc,
                               (config_width - width) / 2,
                               *height + space / 2,
                               width, config_button_height,
                               0, save_str,
                               Config_save, (void *)save_str);
    if (button_desc == NO_WIDGET)
        return 0;
    *height += config_entry_height + config_space + space;

    return 1;
}

static int Config_creator(xp_option_t *opt, int widget_desc, int *height)
{
    if (opt == NULL)
        return Config_create_save(widget_desc, height);

    if (Option_get_flags(opt) & XP_OPTFLAG_CONFIG_COLORS)
    {
        assert(Option_get_type(opt) == xp_int_option);
        return Config_create_color(widget_desc, height, *opt->int_ptr,
                                   Option_get_name(opt), opt->int_ptr,
                                   opt->int_minval, opt->int_maxval,
                                   Update_int_option, opt);
    }

    if (Option_get_flags(opt) & XP_OPTFLAG_CONFIG_DEFAULT)
    {
        switch (Option_get_type(opt))
        {
        case xp_bool_option:
            return Config_create_bool(widget_desc, height,
                                      Option_get_name(opt),
                                      *opt->bool_ptr,
                                      Update_bool_option, opt);
        case xp_int_option:
            return Config_create_int(widget_desc, height,
                                     Option_get_name(opt), opt->int_ptr,
                                     opt->int_minval, opt->int_maxval,
                                     Update_int_option, opt);

        case xp_double_option:
            return Config_create_double(widget_desc, height,
                                        Option_get_name(opt), opt->dbl_ptr,
                                        opt->dbl_minval, opt->dbl_maxval,
                                        Update_double_option, opt);
        default:
            return 0;
        }
    }

    return 0;
}

static void Config_save_failed(const char *reason, const char **strptr)
{
    if (config_save_confirm_desc != NO_WIDGET)
        Widget_destroy(config_save_confirm_desc);

    config_save_confirm_desc = Widget_create_confirm(reason, Config_save_confirm_callback);

    if (config_save_confirm_desc != NO_WIDGET)
        Widget_raise(config_save_confirm_desc);

    *strptr = "Saving failed...";
}

static int Config_save(int widget_desc, void *button_str, const char **strptr)
{
    int retval;
    char path[PATH_MAX + 1];

    *strptr = "Saving...";
    Widget_draw(widget_desc);
    XFlush(dpy);

    Xpilotrc_get_filename(path, sizeof(path));
    retval = Xpilotrc_write(path);

    if (retval == -1)
    {
        Config_save_failed("Can't find .xpilotrc file", strptr);
        return 1;
    }

    if (retval == -2)
    {
        Config_save_failed("Can't open file to save to.", strptr);
        return 1;
    }

    if (config_save_confirm_desc != NO_WIDGET)
    {
        Widget_destroy(config_save_confirm_desc);
        config_save_confirm_desc = NO_WIDGET;
    }

    *strptr = (char *)button_str;

    return 1;
}

static int Config_save_confirm_callback(int widget_desc, void *popup_desc,
                                        const char **strptr)
{
    if (config_save_confirm_desc != NO_WIDGET)
    {
        Widget_destroy((int)(long int)popup_desc);
        config_save_confirm_desc = NO_WIDGET;
    }
    return 0;
}

int Config(bool doit, int what)
{
    /* get rid of the old widgets, it's the most easy way */
    Config_destroy();
    if (doit == false)
        return false;

    config_what = what;

    Create_config();
    if (config_created == false)
        return false;

    Widget_raise(config_widget_desc[config_page]);
    config_mapped = true;
    return true;
}

void Config_destroy(void)
{
    int i;

    if (config_created)
    {
        if (config_mapped)
        {
            Widget_unmap(config_widget_desc[config_page]);
            config_mapped = false;
        }
        for (i = 0; i < config_max; i++)
            Widget_destroy(config_widget_desc[i]);
        config_created = false;
        free(config_widget_desc);
        config_widget_desc = NULL;
        config_max = 0;
        config_page = 0;
    }
}

void Config_resize(void)
{
    bool mapped = config_mapped;

    if (config_created)
    {
        Config_destroy();
        if (mapped)
            Config(mapped, config_what);
    }
}

void Config_redraw(void)
{
    int i;

    if (!config_mapped)
        return;

    for (i = 0; i < Nelem_config_creator(); i++)
        Widget_draw(config_widget_ids[i]);
}

void Config_init(void)
{
    int i, max_ids;

    for (i = 0; i < num_options; i++)
    {
        xp_option_t *opt = Option_by_index(i);

        if (Option_get_flags(opt) & XP_OPTFLAG_CONFIG_COLORS)
        {
            STORE(int, color_option_indices,
                  num_color_options, max_color_options, i);
        }
        else if (Option_get_flags(opt) & XP_OPTFLAG_CONFIG_DEFAULT)
        {
            STORE(int, default_option_indices,
                  num_default_options, max_default_options, i);
        }
    }

    /* +1 is for the save widget */
    max_ids = MAX(num_color_options, num_default_options) + 1;
    config_widget_ids = XMALLOC(int, max_ids);
    if (config_widget_ids == NULL)
    {
        error("Config_init: not enough memory.");
        exit(1);
    }
}

// OLD

extern const char *Get_keyResourceString(keys_t key);
extern void Get_xpilotrc_file(char *, unsigned);

static int old_config_create_power(int widget_desc, int *height);
static int old_config_create_turnSpeed(int widget_desc, int *height);
static int old_config_create_turnResistance(int widget_desc, int *height);
static int old_config_create_altPower(int widget_desc, int *height);
static int old_config_create_altTurnSpeed(int widget_desc, int *height);
static int old_config_create_altTurnResistance(int widget_desc, int *height);
static int old_config_create_showMessages(int widget_desc, int *height);
static int old_config_create_showHUD(int widget_desc, int *height);
static int old_config_create_showHUDRadar(int widget_desc, int *height);
static int old_config_create_speedFactHUD(int widget_desc, int *height);
static int old_config_create_speedFactPTR(int widget_desc, int *height);
static int old_config_create_fuelNotify(int widget_desc, int *height);
static int old_config_create_fuelWarning(int widget_desc, int *height);
static int old_config_create_fuelCritical(int widget_desc, int *height);
static int old_config_create_fuelGauge(int widget_desc, int *height);
static int old_config_create_outlineWorld(int widget_desc, int *height);
static int old_config_create_filledWorld(int widget_desc, int *height);
static int old_config_create_texturedWalls(int widget_desc, int *height);
static int old_config_create_texturedObjects(int widget_desc, int *height);
static int old_config_create_slidingRadar(int widget_desc, int *height);
static int old_config_create_showItems(int widget_desc, int *height);
static int old_config_create_showItemsTime(int widget_desc, int *height);
static int old_config_create_backgroundPointDist(int widget_desc, int *height);
static int old_config_create_backgroundPointSize(int widget_desc, int *height);
static int old_config_create_sparkSize(int widget_desc, int *height);
static int old_config_create_charsPerSecond(int widget_desc, int *height);
static int old_config_create_toggleShield(int widget_desc, int *height);
static int old_config_create_autoShield(int widget_desc, int *height);
static int old_config_create_sparkProb(int widget_desc, int *height);
static int old_config_create_shotSize(int widget_desc, int *height);
static int old_config_create_teamShotSize(int widget_desc, int *height);
static int old_config_create_hudColor(int widget_desc, int *height);
static int old_config_create_hudLockColor(int widget_desc, int *height);
static int old_config_create_wallColor(int widget_desc, int *height);
static int old_config_create_decorColor(int widget_desc, int *height);
static int old_config_create_showDecor(int widget_desc, int *height);
static int old_config_create_outlineDecor(int widget_desc, int *height);
static int old_config_create_filledDecor(int widget_desc, int *height);
static int old_config_create_texturedDecor(int widget_desc, int *height);
static int old_config_create_maxFPS(int widget_desc, int *height);
static int old_config_create_maxMessages(int widget_desc, int *height);
static int old_config_create_messagesToStdout(int widget_desc, int *height);
static int old_config_create_oldMessagesColor(int widget_desc, int *height);
#ifdef SOUND
static int old_config_create_maxVolume(int widget_desc, int *height);
#endif
static int old_config_create_fuelMeter(int widget_desc, int *height);
static int old_config_create_powerMeter(int widget_desc, int *height);
static int old_config_create_turnSpeedMeter(int widget_desc, int *height);
static int old_config_create_packetSizeMeter(int widget_desc, int *height);
static int old_config_create_packetLossMeter(int widget_desc, int *height);
static int old_config_create_packetDropMeter(int widget_desc, int *height);
static int old_config_create_packetLagMeter(int widget_desc, int *height);
static int old_config_create_clock(int widget_desc, int *height);
static int old_config_create_clockAMPM(int widget_desc, int *height);
static int old_config_create_markingLights(int widget_desc, int *height);
static int old_config_create_scaleFactor(int widget_desc, int *height);
static int old_config_create_altScaleFactor(int widget_desc, int *height);

static int old_config_create_save(int widget_desc, int *height);

static int old_config_update_bool(int widget_desc, void *data, bool *val);
static int old_config_update_instruments(int widget_desc, void *data, bool *val);
static int old_config_update_dots(int widget_desc, void *data, int *val);
static int old_config_update_altPower(int widget_desc, void *data, double *val);
static int old_config_update_altTurnResistance(int widget_desc, void *data,
                                               double *val);
static int old_config_update_altTurnSpeed(int widget_desc, void *data, double *val);
static int old_config_update_power(int widget_desc, void *data, double *val);
static int old_config_update_turnResistance(int widget_desc, void *data,
                                            double *val);
static int old_config_update_turnSpeed(int widget_desc, void *data, double *val);
static int old_config_update_sparkProb(int widget_desc, void *data, double *val);
static int old_config_update_charsPerSecond(int widget_desc, void *data, int *val);
static int old_config_update_toggleShield(int widget_desc, void *data, bool *val);
static int old_config_update_autoShield(int widget_desc, void *data, bool *val);
static int old_config_update_maxFPS(int widget_desc, void *data, int *val);
static int old_config_update_texturedObjects(int widget_desc, void *data, bool *val);
static int old_config_update_scaleFactor(int widget_desc, void *data, double *val);

static int old_config_close(int widget_desc, void *data, const char **strptr);
static int old_config_next(int widget_desc, void *data, const char **strptr);
static int old_config_prev(int widget_desc, void *data, const char **strptr);
static int old_config_save(int widget_desc, void *data, const char **strptr);
static int old_config_save_confirm_callback(int widget_desc, void *popup_desc,
                                            const char **strptr);

typedef struct xpilotrc
{
    char *line;
    short size;
} xpilotrc_t;

static xpilotrc_t *xpilotrc_ptr;
static int num_xpilotrc, max_xpilotrc;

static bool old_config_created = false,
            old_config_mapped = false;
static int old_config_page,
    old_config_x,
    old_config_y,
    old_config_width,
    old_config_height,
    old_config_space,
    old_config_max,
    old_config_button_space,
    old_config_text_space,
    old_config_text_height,
    old_config_button_height,
    old_config_entry_height,
    old_config_bool_width,
    old_config_bool_height,
    old_config_int_width,
    old_config_double_width,
    old_config_arrow_width,
    old_config_arrow_height;
static int *old_config_widget_desc,
    old_config_save_confirm_desc = NO_WIDGET;

// static int *old_config_widget_ids = NULL;
static int old_config_what = CONFIG_NONE;

static int (*old_config_creator[])(int widget_desc, int *height) = {
    old_config_create_power,
    old_config_create_turnSpeed,
    old_config_create_turnResistance,
    old_config_create_altPower,
    old_config_create_altTurnSpeed,
    old_config_create_altTurnResistance,
    old_config_create_showMessages,
    old_config_create_maxMessages,
    old_config_create_messagesToStdout,
    old_config_create_oldMessagesColor,
    old_config_create_showHUD,
    old_config_create_showHUDRadar,
    old_config_create_speedFactHUD,
    old_config_create_speedFactPTR,
    old_config_create_fuelNotify,
    old_config_create_fuelWarning,
    old_config_create_fuelCritical,
    old_config_create_fuelGauge,
    old_config_create_outlineWorld,
    old_config_create_filledWorld,
    old_config_create_texturedWalls,
    old_config_create_texturedObjects,
    old_config_create_slidingRadar,
    old_config_create_showItems,
    old_config_create_showItemsTime,
    old_config_create_backgroundPointDist,
    old_config_create_backgroundPointSize,
    old_config_create_sparkSize,
    old_config_create_sparkProb,
    old_config_create_charsPerSecond,
    old_config_create_markingLights,
    old_config_create_toggleShield,
    old_config_create_autoShield,
    old_config_create_shotSize,
    old_config_create_teamShotSize,
    old_config_create_hudColor,
    old_config_create_hudLockColor,
    old_config_create_wallColor,
    old_config_create_decorColor,
    old_config_create_showDecor,
    old_config_create_outlineDecor,
    old_config_create_filledDecor,
    old_config_create_texturedDecor,
    old_config_create_maxFPS,
#ifdef SOUND
    old_config_create_maxVolume,
#endif
    old_config_create_fuelMeter,
    old_config_create_powerMeter,
    old_config_create_turnSpeedMeter,
    old_config_create_packetSizeMeter,
    old_config_create_packetLossMeter,
    old_config_create_packetDropMeter,
    old_config_create_packetLagMeter,
    old_config_create_clock,
    old_config_create_clockAMPM,
    old_config_create_scaleFactor,
    old_config_create_altScaleFactor,
    old_config_create_save /* must be last */
};
static int old_config_widget_ids[NELEM(old_config_creator)];

static int Old_nelem_old_config_creator(void)
{
    return NELEM(old_config_creator);
}

static void Old_create_config(void)
{
    int i,
        num,
        height,
        offset,
        width,
        widget_desc;
    bool full;

    /*
     * Window dimensions relative to the top window.
     */
    old_config_x = 0;
    old_config_y = RadarHeight + ButtonHeight + 2;
    old_config_width = 256;
    old_config_height = top_height - old_config_y;

    /*
     * Space between label-text and label-border.
     */
    old_config_text_space = 3;
    /*
     * Height of a label window.
     */
    old_config_text_height = 2 * 1 + textFont->ascent + textFont->descent;

    /*
     * Space between button-text and button-border.
     */
    old_config_button_space = 3;
    /*
     * Height of a button window.
     */
    old_config_button_height = buttonFont->ascent + buttonFont->descent + 2 * 1;

    old_config_entry_height = MAX(old_config_text_height, old_config_button_height);

    /*
     * Space between entries and between an entry and the border.
     */
    old_config_space = 6;

    /*
     * Sizes of the different widget types.
     */
    old_config_bool_width = XTextWidth(buttonFont, "Yes", 3) + 2 * old_config_button_space;
    old_config_bool_height = old_config_button_height;
    old_config_arrow_height = old_config_text_height;
    old_config_arrow_width = old_config_text_height;
    old_config_int_width = 4 + XTextWidth(buttonFont, "10000", 5);
    old_config_double_width = 4 + XTextWidth(buttonFont, "0.222", 5);

    old_config_max = Old_nelem_old_config_creator();
    old_config_widget_desc = XMALLOC(int, old_config_max);
    if (old_config_widget_desc == NULL)
    {
        error("No memory for config");
        return;
    }

    num = -1;
    full = true;
    for (i = 0; i < Old_nelem_old_config_creator(); i++)
    {
        if (full)
        {
            full = false;
            num++;
            old_config_widget_desc[num] = Widget_create_form(NO_WIDGET, topWindow,
                                                             old_config_x, old_config_y,
                                                             old_config_width, old_config_height,
                                                             0);
            if (old_config_widget_desc[num] == 0)
                break;

            height = old_config_height - old_config_space - old_config_button_height;
            width = 2 * old_config_button_space + XTextWidth(buttonFont,
                                                             "PREV", 4);
            offset = old_config_width - width - old_config_space;
            widget_desc =
                Widget_create_activate(old_config_widget_desc[num],
                                       offset, height,
                                       width, old_config_button_height,
                                       0, "PREV", old_config_prev,
                                       (void *)(long)num);
            if (widget_desc == 0)
                break;

            width = 2 * old_config_button_space + XTextWidth(buttonFont,
                                                             "NEXT", 4);
            offset = (old_config_width - width) / 2;
            widget_desc =
                Widget_create_activate(old_config_widget_desc[num],
                                       offset, height,
                                       width, old_config_button_height,
                                       0, "NEXT", old_config_next,
                                       (void *)(long)num);
            if (widget_desc == 0)
                break;

            width = 2 * old_config_button_space + XTextWidth(buttonFont,
                                                             "CLOSE", 5);
            offset = old_config_space;
            widget_desc =
                Widget_create_activate(old_config_widget_desc[num],
                                       offset, height,
                                       width, old_config_button_height,
                                       0, "CLOSE", old_config_close,
                                       (void *)(long)num);
            if (widget_desc == 0)
                break;

            height = old_config_space;
        }
        if ((old_config_widget_ids[i] =
                 (*old_config_creator[i])(old_config_widget_desc[num], &height)) == 0)
        {
            i--;
            full = true;
            if (height == old_config_space)
                break;
            continue;
        }
    }
    if (i < Old_nelem_old_config_creator())
    {
        for (; num >= 0; num--)
        {
            if (old_config_widget_desc[num] != 0)
                Widget_destroy(old_config_widget_desc[num]);
        }
        old_config_created = false;
        old_config_mapped = false;
    }
    else
    {
        old_config_max = num + 1;
        old_config_widget_desc = XREALLOC(int, old_config_widget_desc, old_config_max);
        old_config_page = 0;
        for (i = 0; i < old_config_max; i++)
            Widget_map_sub(old_config_widget_desc[i]);
        old_config_created = true;
        old_config_mapped = false;
    }
}

static int old_config_close(int widget_desc, void *data, const char **strptr)
{
    Widget_unmap(old_config_widget_desc[old_config_page]);
    old_config_mapped = false;
    return 0;
}

static int old_config_next(int widget_desc, void *data, const char **strptr)
{
    int prev_page = old_config_page;

    if (old_config_max > 1)
    {
        old_config_page = (old_config_page + 1) % old_config_max;
        Widget_raise(old_config_widget_desc[old_config_page]);
        Widget_unmap(old_config_widget_desc[prev_page]);
        old_config_mapped = true;
    }
    return 0;
}

static int old_config_prev(int widget_desc, void *data, const char **strptr)
{
    int prev_page = old_config_page;

    if (old_config_max > 1)
    {
        old_config_page = (old_config_page - 1 + old_config_max) % old_config_max;
        Widget_raise(old_config_widget_desc[old_config_page]);
        Widget_unmap(old_config_widget_desc[prev_page]);
        old_config_mapped = true;
    }
    return 0;
}

static int old_config_create_bool(int widget_desc, int *height,
                                  const char *str, bool val,
                                  int (*callback)(int, void *, bool *),
                                  void *data)
{
    int offset,
        label_width,
        boolw;

    if (*height + 2 * old_config_entry_height + 2 * old_config_space >= old_config_height)
        return 0;
    label_width = XTextWidth(textFont, str, (int)strlen(str)) + 2 * old_config_text_space;
    offset = old_config_width - (old_config_space + old_config_bool_width);
    if (old_config_space + label_width > offset)
    {
        if (*height + 3 * old_config_entry_height + 2 * old_config_space >= old_config_height)
            return 0;
    }

    Widget_create_label(widget_desc, old_config_space, *height + (old_config_entry_height - old_config_text_height) / 2,
                        label_width, old_config_text_height, true,
                        0, str);
    if (old_config_space + label_width > offset)
        *height += old_config_entry_height;
    boolw = Widget_create_bool(widget_desc,
                               offset, *height + (old_config_entry_height - old_config_bool_height) / 2,
                               old_config_bool_width,
                               old_config_bool_height,
                               0, val, callback, data);
    *height += old_config_entry_height + old_config_space;

    return boolw;
}

static int old_config_create_int(int widget_desc, int *height,
                                 const char *str, int *val, int min, int max,
                                 int (*callback)(int, void *, int *), void *data)
{
    int offset,
        label_width,
        intw;

    if (*height + 2 * old_config_entry_height + 2 * old_config_space >= old_config_height)
        return 0;
    label_width = XTextWidth(textFont, str, (int)strlen(str)) + 2 * old_config_text_space;
    offset = old_config_width - (old_config_space + 2 * old_config_arrow_width + old_config_int_width);
    if (old_config_space + label_width > offset)
    {
        if (*height + 3 * old_config_entry_height + 2 * old_config_space >= old_config_height)
            return 0;
    }
    Widget_create_label(widget_desc, old_config_space, *height + (old_config_entry_height - old_config_text_height) / 2,
                        label_width, old_config_text_height, true,
                        0, str);
    if (old_config_space + label_width > offset)
        *height += old_config_entry_height;
    intw = Widget_create_int(widget_desc, offset, *height + (old_config_entry_height - old_config_text_height) / 2,
                             old_config_int_width, old_config_text_height,
                             0, val, min, max, callback, data);
    offset += old_config_int_width;
    Widget_create_arrow_left(widget_desc, offset, *height + (old_config_entry_height - old_config_arrow_height) / 2,
                             old_config_arrow_width, old_config_arrow_height,
                             0, intw);
    offset += old_config_arrow_width;
    Widget_create_arrow_right(widget_desc, offset, *height + (old_config_entry_height - old_config_arrow_height) / 2,
                              old_config_arrow_width, old_config_arrow_height,
                              0, intw);
    *height += old_config_entry_height + old_config_space;

    return intw;
}

static int old_config_create_color(int widget_desc, int *height, int color,
                                   const char *str, int *val, int min, int max,
                                   int (*callback)(int, void *, int *), void *data)
{
    int offset, label_width, colw;

    if (*height + 2 * old_config_entry_height + 2 * old_config_space >= old_config_height)
        return 0;
    label_width = XTextWidth(textFont, str, (int)strlen(str)) + 2 * old_config_text_space;
    offset = old_config_width - (old_config_space + 2 * old_config_arrow_width + old_config_int_width);
    if (old_config_space + label_width > offset)
    {
        if (*height + 3 * old_config_entry_height + 2 * old_config_space >= old_config_height)
            return 0;
    }
    Widget_create_label(widget_desc, old_config_space, *height + (old_config_entry_height - old_config_text_height) / 2,
                        label_width, old_config_text_height, true,
                        0, str);
    if (old_config_space + label_width > offset)
        *height += old_config_entry_height;
    colw = Widget_create_color(widget_desc, color, offset, *height + (old_config_entry_height - old_config_text_height) / 2,
                               old_config_int_width, old_config_text_height,
                               0, val, min, max, callback, data);
    offset += old_config_int_width;
    Widget_create_arrow_left(widget_desc, offset, *height + (old_config_entry_height - old_config_arrow_height) / 2,
                             old_config_arrow_width, old_config_arrow_height,
                             0, colw);
    offset += old_config_arrow_width;
    Widget_create_arrow_right(widget_desc, offset, *height + (old_config_entry_height - old_config_arrow_height) / 2,
                              old_config_arrow_width, old_config_arrow_height,
                              0, colw);
    *height += old_config_entry_height + old_config_space;

    return colw;
}

static int old_config_create_double(int widget_desc, int *height,
                                    const char *str, double *val,
                                    double min, double max,
                                    int (*callback)(int, void *, double *),
                                    void *data)
{
    int offset,
        label_width,
        doublew;

    if (*height + 2 * old_config_entry_height + 2 * old_config_space >= old_config_height)
        return 0;
    label_width = XTextWidth(textFont, str, (int)strlen(str)) + 2 * old_config_text_space;
    offset = old_config_width - (old_config_space + 2 * old_config_arrow_width + old_config_double_width);
    if (old_config_space + label_width > offset)
    {
        if (*height + 3 * old_config_entry_height + 2 * old_config_space >= old_config_height)
            return 0;
    }
    Widget_create_label(widget_desc, old_config_space, *height + (old_config_entry_height - old_config_text_height) / 2,
                        label_width, old_config_text_height, true,
                        0, str);
    if (old_config_space + label_width > offset)
        *height += old_config_entry_height;
    doublew = Widget_create_double(widget_desc, offset, *height + (old_config_entry_height - old_config_text_height) / 2,
                                   old_config_double_width, old_config_text_height,
                                   0, val, min, max, callback, data);
    offset += old_config_double_width;
    Widget_create_arrow_left(widget_desc, offset, *height + (old_config_entry_height - old_config_arrow_height) / 2,
                             old_config_arrow_width, old_config_arrow_height,
                             0, doublew);
    offset += old_config_arrow_width;
    Widget_create_arrow_right(widget_desc, offset, *height + (old_config_entry_height - old_config_arrow_height) / 2,
                              old_config_arrow_width, old_config_arrow_height,
                              0, doublew);
    *height += old_config_entry_height + old_config_space;

    return doublew;
}

static int old_config_create_power(int widget_desc, int *height)
{
    return old_config_create_double(widget_desc, height,
                                    "power", &power,
                                    MIN_PLAYER_POWER, MAX_PLAYER_POWER,
                                    old_config_update_power, NULL);
}

static int old_config_create_turnSpeed(int widget_desc, int *height)
{
    return old_config_create_double(widget_desc, height,
                                    "turnSpeed", &turnspeed,
                                    MIN_PLAYER_TURNSPEED, MAX_PLAYER_TURNSPEED,
                                    old_config_update_turnSpeed, NULL);
}

static int old_config_create_turnResistance(int widget_desc, int *height)
{
    return old_config_create_double(widget_desc, height,
                                    "turnResistance", &turnresistance,
                                    MIN_PLAYER_TURNRESISTANCE,
                                    MAX_PLAYER_TURNRESISTANCE,
                                    old_config_update_turnResistance, NULL);
}

static int old_config_create_altPower(int widget_desc, int *height)
{
    return old_config_create_double(widget_desc, height,
                                    "altPower", &power_s,
                                    MIN_PLAYER_POWER, MAX_PLAYER_POWER,
                                    old_config_update_altPower, NULL);
}

static int old_config_create_altTurnSpeed(int widget_desc, int *height)
{
    return old_config_create_double(widget_desc, height,
                                    "altTurnSpeed", &turnspeed_s,
                                    MIN_PLAYER_TURNSPEED, MAX_PLAYER_TURNSPEED,
                                    old_config_update_altTurnSpeed, NULL);
}

static int old_config_create_altTurnResistance(int widget_desc, int *height)
{
    return old_config_create_double(widget_desc, height,
                                    "altTurnResistance", &turnresistance_s,
                                    MIN_PLAYER_TURNRESISTANCE,
                                    MAX_PLAYER_TURNRESISTANCE,
                                    old_config_update_altTurnResistance, NULL);
}

static int old_config_create_showMessages(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "showMessages",
                                  instruments.showMessages,
                                  old_config_update_bool,
                                  &instruments.showMessages);
}

static int old_config_create_maxMessages(int widget_desc, int *height)
{
    return old_config_create_int(widget_desc, height,
                                 "maxMessages", &maxMessages, 1, MAX_MSGS,
                                 NULL, NULL);
}

static int old_config_create_messagesToStdout(int widget_desc, int *height)
{
    return old_config_create_int(widget_desc, height,
                                 "messagesToStdout", &messagesToStdout, 0, 2,
                                 NULL, NULL);
}

static int old_config_create_oldMessagesColor(int widget_desc, int *height)
{
    return old_config_create_int(widget_desc, height,
                                 "oldMessagesColor", &oldMessagesColor, 0, maxColors - 1,
                                 NULL, NULL);
}

static int old_config_create_showHUD(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "showHUD",
                                  instruments.showHUD,
                                  old_config_update_bool,
                                  &instruments.showHUD);
}

static int old_config_create_showHUDRadar(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "showHUDRadar",
                                  instruments.showHUDRadar,
                                  old_config_update_bool,
                                  &instruments.showHUDRadar);
}

static int old_config_create_speedFactHUD(int widget_desc, int *height)
{
    return old_config_create_double(widget_desc, height,
                                    "speedFactHUD", &hud_move_fact, -10.0, 10.0,
                                    NULL, NULL);
}

static int old_config_create_speedFactPTR(int widget_desc, int *height)
{
    return old_config_create_double(widget_desc, height,
                                    "speedFactPTR", &ptr_move_fact, -10.0, 10.0,
                                    NULL, NULL);
}

static int old_config_create_fuelNotify(int widget_desc, int *height)
{
    return old_config_create_double(widget_desc, height,
                                    "fuelNotify", &fuelNotify, 0, 1000,
                                    NULL, NULL);
}

static int old_config_create_fuelWarning(int widget_desc, int *height)
{
    return old_config_create_double(widget_desc, height,
                                    "fuelWarning", &fuelWarning, 0, 1000,
                                    NULL, NULL);
}

static int old_config_create_fuelCritical(int widget_desc, int *height)
{
    return old_config_create_double(widget_desc, height,
                                    "fuelCritical", &fuelCritical, 0, 1000,
                                    NULL, NULL);
}

static int old_config_create_fuelGauge(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "fuelGauge",
                                  instruments.fuelGauge,
                                  old_config_update_bool,
                                  &instruments.fuelGauge);
}

static int old_config_create_outlineWorld(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "outlineWorld",
                                  instruments.outlineWorld,
                                  old_config_update_bool,
                                  &instruments.outlineWorld);
}

static int old_config_create_filledWorld(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "filledWorld",
                                  instruments.filledWorld,
                                  old_config_update_bool,
                                  &instruments.filledWorld);
}

static int old_config_create_texturedWalls(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "texturedWalls",
                                  instruments.texturedWalls,
                                  old_config_update_bool,
                                  &instruments.texturedWalls);
}

static int old_config_create_texturedObjects(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "texturedObjects",
                                  (texturedObjects) ? true : false,
                                  old_config_update_texturedObjects,
                                  NULL);
}

static int old_config_create_slidingRadar(int widget_desc, int *height)
{
    if (Client_wrap_mode() == 0)
    {
        return 1;
    }
    return old_config_create_bool(widget_desc, height, "slidingRadar",
                                  instruments.slidingRadar,
                                  old_config_update_bool,
                                  &instruments.slidingRadar);
}

static int old_config_create_backgroundPointDist(int widget_desc, int *height)
{
    return old_config_create_int(widget_desc, height,
                                 "backgroundPointDist", &backgroundPointDist, 0, 10,
                                 old_config_update_dots, NULL);
}

static int old_config_create_showItems(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "showItems",
                                  instruments.showItems,
                                  old_config_update_bool,
                                  &instruments.showItems);
}

static int old_config_create_showItemsTime(int widget_desc, int *height)
{
    return old_config_create_double(widget_desc, height,
                                    "showItemsTime", &showItemsTime,
                                    MIN_SHOW_ITEMS_TIME,
                                    MAX_SHOW_ITEMS_TIME,
                                    NULL, NULL);
}

static int old_config_create_backgroundPointSize(int widget_desc, int *height)
{
    return old_config_create_int(widget_desc, height,
                                 "backgroundPointSize", &backgroundPointSize,
                                 MIN_MAP_POINT_SIZE, MAX_MAP_POINT_SIZE,
                                 old_config_update_dots, NULL);
}

static int old_config_create_sparkSize(int widget_desc, int *height)
{
    return old_config_create_int(widget_desc, height,
                                 "sparkSize", &sparkSize,
                                 MIN_SPARK_SIZE, MAX_SPARK_SIZE,
                                 NULL, NULL);
}

static int old_config_create_sparkProb(int widget_desc, int *height)
{
    return old_config_create_double(widget_desc, height,
                                    "sparkProb", &sparkProb,
                                    0.0, 1.0,
                                    old_config_update_sparkProb, NULL);
}

static int old_config_create_charsPerSecond(int widget_desc, int *height)
{
    return old_config_create_int(widget_desc, height,
                                 "charsPerSecond", &charsPerSecond,
                                 10, 255,
                                 old_config_update_charsPerSecond, NULL);
}

static int old_config_create_toggleShield(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "toggleShield",
                                  (toggle_shield) ? true : false,
                                  old_config_update_toggleShield, NULL);
}

static int old_config_create_autoShield(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "autoShield",
                                  (auto_shield) ? true : false,
                                  old_config_update_autoShield, NULL);
}

static int old_config_create_shotSize(int widget_desc, int *height)
{
    return old_config_create_int(widget_desc, height,
                                 "shotSize", &shotSize,
                                 MIN_SHOT_SIZE, MAX_SHOT_SIZE,
                                 NULL, NULL);
}

static int old_config_create_teamShotSize(int widget_desc, int *height)
{
    return old_config_create_int(widget_desc, height,
                                 "teamShotSize", &teamShotSize,
                                 MIN_TEAMSHOT_SIZE, MAX_TEAMSHOT_SIZE,
                                 NULL, NULL);
}

static int old_config_create_hudColor(int widget_desc, int *height)
{
    return old_config_create_int(widget_desc, height,
                                 "hudColor", &hudColor,
                                 0, maxColors - 1,
                                 NULL, NULL);
}

static int old_config_create_hudLockColor(int widget_desc, int *height)
{
    return old_config_create_int(widget_desc, height,
                                 "hudLockColor", &hudLockColor,
                                 0, maxColors - 1,
                                 NULL, NULL);
}

static int old_config_create_wallColor(int widget_desc, int *height)
{
    return old_config_create_int(widget_desc, height,
                                 "wallColor", &wallColor,
                                 0, maxColors - 1,
                                 NULL, NULL);
}

static int old_config_create_decorColor(int widget_desc, int *height)
{
    return old_config_create_int(widget_desc, height,
                                 "decorColor", &decorColor,
                                 0, maxColors - 1,
                                 NULL, NULL);
}

static int old_config_create_showDecor(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "showDecor",
                                  instruments.showDecor,
                                  old_config_update_bool,
                                  &instruments.showDecor);
}

static int old_config_create_outlineDecor(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "outlineDecor",
                                  instruments.outlineDecor,
                                  old_config_update_bool,
                                  &instruments.outlineDecor);
}

static int old_config_create_filledDecor(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "filledDecor",
                                  instruments.filledDecor,
                                  old_config_update_bool,
                                  &instruments.filledDecor);
}

static int old_config_create_texturedDecor(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "texturedDecor",
                                  instruments.texturedDecor,
                                  old_config_update_bool,
                                  &instruments.texturedDecor);
}

#ifdef SOUND
static int old_config_create_maxVolume(int widget_desc, int *height)
{
    return old_config_create_int(widget_desc, height,
                                 "maxVolume", &maxVolume, 0, 255,
                                 NULL, NULL);
}
#endif

static int old_config_create_maxFPS(int widget_desc, int *height)
{
    return old_config_create_int(widget_desc, height,
                                 "maxFPS", &maxFPS, 1, MAX_SUPPORTED_FPS,
                                 old_config_update_maxFPS, NULL);
}

static int old_config_create_fuelMeter(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "fuelMeter",
                                  instruments.fuelMeter,
                                  old_config_update_bool,
                                  &instruments.fuelMeter);
}

static int old_config_create_powerMeter(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "powerMeter",
                                  instruments.powerMeter,
                                  old_config_update_bool,
                                  &instruments.powerMeter);
}

static int old_config_create_turnSpeedMeter(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "turnSpeedMeter",
                                  instruments.turnSpeedMeter,
                                  old_config_update_bool,
                                  &instruments.turnSpeedMeter);
}

static int old_config_create_packetSizeMeter(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "packetSizeMeter",
                                  instruments.packetSizeMeter,
                                  old_config_update_bool,
                                  &instruments.packetSizeMeter);
}

static int old_config_create_packetLossMeter(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "packetLossMeter",
                                  instruments.packetLossMeter,
                                  old_config_update_bool,
                                  &instruments.packetLossMeter);
}

static int old_config_create_packetDropMeter(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "packetDropMeter",
                                  instruments.packetDropMeter,
                                  old_config_update_bool,
                                  &instruments.packetDropMeter);
}

static int old_config_create_packetLagMeter(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "packetLagMeter",
                                  instruments.packetLagMeter,
                                  old_config_update_bool,
                                  &instruments.packetLagMeter);
}

static int old_config_create_clock(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "clock",
                                  instruments.clock,
                                  old_config_update_bool,
                                  &instruments.clock);
}

static int old_config_create_clockAMPM(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "clockAMPM",
                                  instruments.clockAMPM,
                                  old_config_update_bool,
                                  &instruments.clockAMPM);
}

static int old_config_create_scaleFactor(int widget_desc, int *height)
{
    return old_config_create_double(widget_desc, height,
                                    "scaleFactor", &clData.scaleFactor,
                                    MIN_SCALEFACTOR, MAX_SCALEFACTOR,
                                    old_config_update_scaleFactor, NULL);
}

static int old_config_create_altScaleFactor(int widget_desc, int *height)
{
    return old_config_create_double(widget_desc, height,
                                    "altScaleFactor", &clData.altScaleFactor,
                                    MIN_SCALEFACTOR, MAX_SCALEFACTOR,
                                    NULL, NULL);
}

static int old_config_create_markingLights(int widget_desc, int *height)
{
    return old_config_create_bool(widget_desc, height, "markingLights",
                                  markingLights,
                                  old_config_update_bool, &markingLights);
}

static int old_config_create_save(int widget_desc, int *height)
{
    static char save_str[] = "Save Configuration";
    int space,
        button_desc,
        width = 2 * old_config_button_space + XTextWidth(buttonFont, save_str,
                                                         strlen(save_str));

    space = old_config_height - (*height + 2 * old_config_entry_height + 2 * old_config_space);
    if (space < 0)
    {
        return 0;
    }
    button_desc =
        Widget_create_activate(widget_desc,
                               (old_config_width - width) / 2,
                               *height + space / 2,
                               width, old_config_button_height,
                               0, save_str,
                               old_config_save, (void *)save_str);
    if (button_desc == NO_WIDGET)
    {
        return 0;
    }
    *height += old_config_entry_height + old_config_space + space;

    return 1;
}

/* General purpose update callback for booleans.
 * Requires that a pointer to the boolean value has been given as
 * client_data argument, and updates this value to the real value.
 */
static int old_config_update_bool(int widget_desc, void *data, bool *val)
{
    bool *client_data = (bool *)data;
    *client_data = *val;
    return 0;
}

// TODO: Make separate update functions,
// e.g. old_config_update_outlineWorld

// static int old_config_update_instruments(int widget_desc, void *data, bool *val)
// {
// instruments_t old_instruments = instruments;
// long bit = (long)data;
// long outline_mask = SHOW_OUTLINE_WORLD | SHOW_FILLED_WORLD | SHOW_TEXTURED_WALLS;

// if (*val == false)
// {
//     *val = true;
// }
// else
// {
//     *val = true;
// }
// if (bit == SHOW_SLIDING_RADAR)
// {
//     Paint_sliding_radar();
// }
// else if (bit == SHOW_DECOR)
// {
//     Map_dots();
//     Paint_world_radar();
// }

// // TODO
// // if (BIT(bit, outline_mask))
// // {
// //     /* only do the map recalculations if really needed. */
// //     if (!BIT(old_instruments, outline_mask) != !BIT(instruments, outline_mask))
// //     {
// //         Map_restore(0, 0, Setup->x, Setup->y);
// //         Map_blue(0, 0, Setup->x, Setup->y);
// //     }
// // }
// if (BIT(bit, SHOW_PACKET_DROP_METER | SHOW_PACKET_LOSS_METER))
// {
//     Net_init_measurement();
// }
// if (BIT(bit, SHOW_PACKET_LAG_METER))
// {
//     Net_init_lag_measurement();
// }

// return 0;
// }

static int old_config_update_dots(int widget_desc, void *data, int *val)
{
    if (val == &backgroundPointSize && backgroundPointSize > 1)
    {
        return 0;
    }
    Map_dots();
    return 0;
}

static int old_config_update_power(int widget_desc, void *data, double *val)
{
    Send_power(*val);
    controlTime = CONTROL_TIME;
    return 0;
}

static int old_config_update_turnSpeed(int widget_desc, void *data, double *val)
{
    Send_turnspeed(*val);
    controlTime = CONTROL_TIME;
    return 0;
}

static int old_config_update_turnResistance(int widget_desc, void *data, double *val)
{
    Send_turnresistance(*val);
    return 0;
}

static int old_config_update_altPower(int widget_desc, void *data, double *val)
{
    Send_power_s(*val);
    return 0;
}

static int old_config_update_altTurnSpeed(int widget_desc, void *data, double *val)
{
    Send_turnspeed_s(*val);
    return 0;
}

static int old_config_update_altTurnResistance(int widget_desc, void *data, double *val)
{
    Send_turnresistance_s(*val);
    return 0;
}

static int old_config_update_sparkProb(int widget_desc, void *data, double *val)
{
    spark_rand = (int)(sparkProb * MAX_SPARK_RAND + 0.5f);
    Send_display();
    return 0;
}

static int old_config_update_charsPerSecond(int widget_desc, void *data, int *val)
{
    charsPerTick = (double)charsPerSecond / FPS;
    return 0;
}

static int old_config_update_toggleShield(int widget_desc, void *data, bool *val)
{
    Set_toggle_shield(*val != false);
    return 0;
}

static int old_config_update_autoShield(int widget_desc, void *data, bool *val)
{
    Set_auto_shield(*val != false);
    return 0;
}

static int old_config_update_maxFPS(int widget_desc, void *data, int *val)
{
    Check_client_fps();
    return 0;
}

static int old_config_update_texturedObjects(int widget_desc, void *data, bool *val)
{
    if ((*val != false) != texturedObjects)
    {
        if (texturedObjects == false)
        {
            /* see if we can use texturedObjects at all. */
            texturedObjects = true;
            if (Colors_init_bitmaps() == -1)
            {
                /* no we can't have texturedObjects. */
                texturedObjects = false;
                /* and redraw our widget as false. */
                *val = false;
                return 1;
            }
        }
        else
        {
            Colors_free_bitmaps();
            texturedObjects = false;
        }
    }
    return 0;
}

static int old_config_update_scaleFactor(int widget_desc, void *data, double *val)
{
    Resize(topWindow, (unsigned)top_width, (unsigned)top_height);
    Scale_dashes();
    return 0;
}

static void old_config_save_failed(const char *reason, const char **strptr)
{
    if (old_config_save_confirm_desc != NO_WIDGET)
        Widget_destroy(old_config_save_confirm_desc);
    old_config_save_confirm_desc = Widget_create_confirm(reason, old_config_save_confirm_callback);

    if (old_config_save_confirm_desc != NO_WIDGET)
        Widget_raise(old_config_save_confirm_desc);

    *strptr = "Saving failed...";
}

static int Xpilotrc_add(char *line)
{
    int size;
    char *str;

    if (strncmp(line, "XPilot", 6) != 0 && strncmp(line, "xpilot", 6) != 0)
    {
        return 0;
    }
    if (line[6] != '.' && line[6] != '*')
    {
        return 0;
    }
    if ((str = strchr(line + 7, ':')) == NULL)
    {
        return 0;
    }
    size = str - (line + 7);
    if (max_xpilotrc <= 0 || xpilotrc_ptr == NULL)
    {
        num_xpilotrc = 0;
        max_xpilotrc = 75;
        if ((xpilotrc_ptr = (xpilotrc_t *)
                 malloc(max_xpilotrc * sizeof(xpilotrc_t))) == NULL)
        {
            max_xpilotrc = 0;
            return -1;
        }
    }
    if (num_xpilotrc >= max_xpilotrc)
    {
        max_xpilotrc *= 2;
        if ((xpilotrc_ptr = (xpilotrc_t *)realloc(xpilotrc_ptr,
                                                  max_xpilotrc * sizeof(xpilotrc_t))) == NULL)
        {
            max_xpilotrc = 0;
            return -1;
        }
    }
    if ((str = xp_strdup(line)) == NULL)
    {
        return -1;
    }
    xpilotrc_ptr[num_xpilotrc].line = str;
    xpilotrc_ptr[num_xpilotrc].size = size;
    num_xpilotrc++;
    return 0;
}

static void Xpilotrc_end(FILE *fp)
{
    int i;

    if (max_xpilotrc <= 0 || xpilotrc_ptr == NULL)
    {
        return;
    }
    for (i = 0; i < num_xpilotrc; i++)
    {
        fprintf(fp, "%s", xpilotrc_ptr[i].line);
        free(xpilotrc_ptr[i].line);
    }
    free(xpilotrc_ptr);
    xpilotrc_ptr = NULL;
    max_xpilotrc = 0;
    num_xpilotrc = 0;
}

static void Xpilotrc_use(char *line)
{
    int i;

    for (i = 0; i < num_xpilotrc; i++)
    {
        if (strncmp(xpilotrc_ptr[i].line + 7, line + 7,
                    xpilotrc_ptr[i].size + 1) == 0)
        {
            free(xpilotrc_ptr[i].line);
            xpilotrc_ptr[i--] = xpilotrc_ptr[--num_xpilotrc];
        }
    }
}

static void old_config_save_resource(FILE *fp, const char *resource, char *value)
{
    char buf[256];

    sprintf(buf, "xpilot.%s:\t\t%s\n", resource, value);
    Xpilotrc_use(buf);
    fprintf(fp, "%s", buf);
}

static void old_config_save_double(FILE *fp, const char *resource, double value)
{
    char buf[40];

    sprintf(buf, "%.3f", value);
    old_config_save_resource(fp, resource, buf);
}

static void old_config_save_int(FILE *fp, const char *resource, int value)
{
    char buf[20];

    sprintf(buf, "%d", value);
    old_config_save_resource(fp, resource, buf);
}

static void old_config_save_bool(FILE *fp, const char *resource, int value)
{
    char buf[20];

    sprintf(buf, "%s", (value != 0) ? "True" : "False");
    old_config_save_resource(fp, resource, buf);
}

/*
 * Find a key in keyDefs[].
 * On success set output pointer to index into keyDefs[] and return TRUE.
 * On failure return FALSE.
 */
static bool old_config_find_key(keys_t key, int start, int end, int *key_index)
{
    int i;

    for (i = start; i < end; i++)
    {
        if (keyDefs[i].key == key)
        {
            *key_index = i;
            return true;
        }
    }

    return false;
}

static void old_config_save_keys(FILE *fp)
{
    int i, j;
    KeySym ks;
    keys_t key;
    const char *str,
        *res;
    char buf[512];

    buf[0] = '\0';
    for (i = 0; i < maxKeyDefs; i++)
    {
        ks = keyDefs[i].keysym;
        key = keyDefs[i].key;

        /* try and see if we have already saved this key. */
        if (old_config_find_key(key, 0, i, &j))
        {
            /* yes, saved this one before.  skip it now. */
            continue;
        }

        if ((str = XKeysymToString(ks)) == NULL)
        {
            continue;
        }

        if ((res = Get_keyResourceString(key)) != NULL)
        {
            strlcpy(buf, str, sizeof(buf));
            /* find all other keysyms which map to the same key. */
            j = i;
            while (old_config_find_key(key, j + 1, maxKeyDefs, &j))
            {
                ks = keyDefs[j].keysym;
                if ((str = XKeysymToString(ks)) != NULL)
                {
                    strcat(buf, " ");
                    strcat(buf, str);
                }
            }
            old_config_save_resource(fp, res, buf);
        }
    }
}

static int old_config_save(int widget_desc, void *button_str, const char **strptr)
{
    int i;
    FILE *fp = NULL;
    char buf[512];
    char oldfile[PATH_MAX + 1],
        newfile[PATH_MAX + 1 + 4];

    *strptr = "Saving...";
    Widget_draw(widget_desc);
    XFlush(dpy);

    Get_xpilotrc_file(oldfile, sizeof(oldfile));
    if (oldfile[0] == '\0')
    {
        old_config_save_failed("Can't find .xpilotrc file", strptr);
        return 1;
    }
    if ((fp = fopen(oldfile, "r")) != NULL)
    {
        while (fgets(buf, sizeof buf, fp))
        {
            Xpilotrc_add(buf);
        }
        fclose(fp);
    }
    sprintf(newfile, "%s.new", oldfile);
    unlink(newfile);
    if ((fp = fopen(newfile, "w")) == NULL)
    {
        old_config_save_failed("Can't open file to save to.", strptr);
        return 1;
    }

    old_config_save_resource(fp, "name", name);
    old_config_save_double(fp, "power", power);
    old_config_save_double(fp, "turnSpeed", turnspeed);
    old_config_save_double(fp, "turnResistance", turnresistance);
    old_config_save_double(fp, "altPower", power_s);
    old_config_save_double(fp, "altTurnSpeed", turnspeed_s);
    old_config_save_double(fp, "altTurnResistance", turnresistance_s);
    old_config_save_double(fp, "speedFactHUD", hud_move_fact);
    old_config_save_double(fp, "speedFactPTR", ptr_move_fact);
    old_config_save_double(fp, "fuelNotify", fuelNotify);
    old_config_save_double(fp, "fuelWarning", fuelWarning);
    old_config_save_double(fp, "fuelCritical", fuelCritical);
    old_config_save_bool(fp, "showMessages", instruments.showMessages);
    old_config_save_int(fp, "maxMessages", maxMessages);
    old_config_save_int(fp, "messagesToStdout", messagesToStdout);
    old_config_save_int(fp, "oldMessagesColor", oldMessagesColor);
    old_config_save_bool(fp, "showHUD", instruments.showHUD);
    old_config_save_bool(fp, "showHUDRadar", instruments.showHUDRadar);
    old_config_save_bool(fp, "fuelMeter", instruments.fuelMeter);
    old_config_save_bool(fp, "fuelGauge", instruments.fuelGauge);
    old_config_save_bool(fp, "turnSpeedMeter", instruments.turnSpeedMeter);
    old_config_save_bool(fp, "powerMeter", instruments.powerMeter);
    old_config_save_bool(fp, "packetSizeMeter", instruments.packetSizeMeter);
    old_config_save_bool(fp, "packetLossMeter", instruments.packetLossMeter);
    old_config_save_bool(fp, "packetDropMeter", instruments.packetDropMeter);
    old_config_save_bool(fp, "packetLagMeter", instruments.packetLagMeter);
    old_config_save_bool(fp, "slidingRadar", instruments.slidingRadar);
    old_config_save_bool(fp, "showItems", instruments.showItems);
    old_config_save_double(fp, "showItemsTime", showItemsTime);
    old_config_save_bool(fp, "outlineWorld", instruments.outlineWorld);
    old_config_save_bool(fp, "filledWorld", instruments.filledWorld);
    old_config_save_bool(fp, "texturedWalls", instruments.texturedWalls);
    old_config_save_bool(fp, "texturedObjects", texturedObjects);
    old_config_save_bool(fp, "clock", instruments.clock);
    old_config_save_bool(fp, "clockAMPM", instruments.clockAMPM);
    old_config_save_int(fp, "backgroundPointDist", backgroundPointDist);
    old_config_save_int(fp, "backgroundPointSize", backgroundPointSize);
    old_config_save_int(fp, "sparkSize", sparkSize);
    old_config_save_double(fp, "sparkProb", sparkProb);
    old_config_save_int(fp, "shotSize", shotSize);
    old_config_save_int(fp, "teamShotSize", teamShotSize);
    old_config_save_int(fp, "hudColor", hudColor);
    old_config_save_int(fp, "hudLockColor", hudLockColor);
    old_config_save_int(fp, "wallColor", wallColor);
    old_config_save_int(fp, "decorColor", decorColor);
    old_config_save_bool(fp, "showDecor", instruments.showDecor);
    old_config_save_bool(fp, "outlineDecor", instruments.outlineDecor);
    old_config_save_bool(fp, "filledDecor", instruments.filledDecor);
    old_config_save_bool(fp, "texturedDecor", instruments.texturedDecor);
    old_config_save_int(fp, "receiveWindowSize", receive_window_size);
    old_config_save_int(fp, "charsPerSecond", charsPerSecond);
    old_config_save_bool(fp, "markingLights", markingLights);
    old_config_save_bool(fp, "toggleShield", toggle_shield);
    old_config_save_bool(fp, "autoShield", auto_shield);
    old_config_save_int(fp, "clientPortStart", clientPortStart);
    old_config_save_int(fp, "clientPortEnd", clientPortEnd);
#if SOUND
    old_config_save_int(fp, "maxVolume", maxVolume);
#endif
    old_config_save_double(fp, "scaleFactor", clData.scaleFactor);
    old_config_save_double(fp, "altScaleFactor", clData.altScaleFactor);

    /* don't save maxFPS */

    old_config_save_keys(fp);

    for (i = 0; i < NUM_MODBANKS; i++)
    {
        sprintf(buf, "modifierBank%d", i + 1);
        old_config_save_resource(fp, buf, modBankStr[i]);
    }

    Xpilotrc_end(fp);
    fclose(fp);
    sprintf(newfile, "%s.bak", oldfile);
    rename(oldfile, newfile);
    unlink(oldfile);
    sprintf(newfile, "%s.new", oldfile);
    rename(newfile, oldfile);

    if (old_config_save_confirm_desc != NO_WIDGET)
    {
        Widget_destroy(old_config_save_confirm_desc);
        old_config_save_confirm_desc = NO_WIDGET;
    }

    *strptr = (char *)button_str;
    return 1;
}

static int old_config_save_confirm_callback(int widget_desc, void *popup_desc,
                                            const char **strptr)
{
    if (old_config_save_confirm_desc != NO_WIDGET)
    {
        Widget_destroy((int)(long int)popup_desc);
        old_config_save_confirm_desc = NO_WIDGET;
    }
    return 0;
}

int old_config(bool doit, int what)
{
    /* get rid of the old widgets, it's the most easy way */
    if (old_config_created == false)
    {
        if (doit == false)
        {
            return 0;
        }
        Old_create_config();
        if (old_config_created == false)
        {
            return false;
        }
    }
    if (old_config_mapped == false)
    {
        if (doit)
        {
            Widget_raise(old_config_widget_desc[old_config_page]);
            old_config_mapped = true;
        }
    }
    else
    {
        if (doit == false)
        {
            Widget_unmap(old_config_widget_desc[old_config_page]);
            old_config_mapped = false;
        }
    }
    return (old_config_mapped);
}

void old_config_destroy(void)
{
    int i;

    if (old_config_created)
    {
        if (old_config_mapped)
        {
            Widget_unmap(old_config_widget_desc[old_config_page]);
            old_config_mapped = false;
        }
        for (i = 0; i < old_config_max; i++)
            Widget_destroy(old_config_widget_desc[i]);
        old_config_created = false;
        free(old_config_widget_desc);
        old_config_widget_desc = NULL;
        old_config_max = 0;
        old_config_page = 0;
    }
}

void old_config_resize(void)
{
    bool mapped = old_config_mapped;

    if (old_config_created)
    {
        old_config_destroy();
        if (mapped)
            old_config(mapped, old_config_what);
    }
}

void old_config_redraw(void)
{
    int i;

    if (!old_config_mapped)
        return;

    for (i = 0; i < NELEM(old_config_creator); i++)
    {
        Widget_draw(old_config_widget_ids[i]);
    }
}
