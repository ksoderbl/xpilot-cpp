/*
 * XPilot, a multiplayer gravity war game.
 *
 * Copyright (C) 1991-2001 by
 *
 *      Bjørn Stabell
 *      Ken Ronny Schouten
 *      Bert Gijsbers
 *      Dick Balaska
 *
 * Copyright (C) 2003-2004 Kristian Söderblom
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
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <cmath>
#include <cassert>
#include <cctype>
#include <sys/types.h>

#include "commonmacros.h"
#include "commonproto.h"

#include "xperror.h"
#include "const.h"
#include "version.h"
#include "messages.h"

#include "client.h"
#include "clientoption.h"
#include "xpilotrc.h"

std::vector<xp_option_t> optionsVector;

xp_option_t *Find_option(const char *name)
{
    int i;

    for (i = 0; i < optionsVector.size(); i++)
    {
        if (!strcasecmp(name, optionsVector[i].name))
            return &optionsVector[i];
    }

    return nullptr;
}

static const char *Option_default_value_to_string(xp_option_t *opt)
{
    static char buf[4096];

    switch (opt->type)
    {
    case xp_noarg_option:
        strcpy(buf, "");
        break;
    case xp_bool_option:
        sprintf(buf, "%s", opt->bool_defval ? "yes" : "no");
        break;
    case xp_int_option:
        sprintf(buf, "%d", opt->int_defval);
        break;
    case xp_double_option:
        sprintf(buf, "%.3f", opt->dbl_defval);
        break;
    case xp_const_char_star_option:
        if (opt->str_defval && strlen(opt->str_defval) > 0)
            strlcpy(buf, opt->str_defval, sizeof(buf));
        else
            strcpy(buf, "");
        break;
    case xp_key_option:
        if (opt->key_defval && strlen(opt->key_defval) > 0)
            strlcpy(buf, opt->key_defval, sizeof(buf));
        else
            strcpy(buf, "");
        break;
    default:
        assert(0 && "Unknown option type");
    }
    return buf;
}

static void Print_default_value(xp_option_t *opt)
{
    const char *defval = Option_default_value_to_string(opt);

    switch (opt->type)
    {
    case xp_noarg_option:
        break;
    case xp_bool_option:
    case xp_int_option:
    case xp_double_option:
    case xp_const_char_star_option:
        if (strlen(defval) > 0)
            printf("        The default value is: %s.\n", defval);
        else
            printf("        There is no default value for this option.\n");
        break;

    case xp_key_option:
        if (opt->key_defval && strlen(opt->key_defval) > 0)
            printf("        The default %s: %s.\n",
                   (strchr(opt->key_defval, ' ') == nullptr
                        ? "key is"
                        : "keys are"),
                   opt->key_defval);
        else
            printf("        There is no default value for this option.\n");
        break;
    default:
        assert(0 && "Unknown option type");
    }
}

void Usage(void)
{
    int i;

    printf("Usage: %s [-options ...] [server]\n"
           "Where options include:\n"
           "\n",
           Program_name());
    for (i = 0; i < optionsVector.size(); i++)
    {
        xp_option_t *opt = Option_by_index(i);

        printf("    -%s %s\n", opt->name,
               (opt->type != xp_noarg_option) ? "<value>" : "");
        if (opt->help && opt->help[0])
        {
            const char *str;
            printf("        ");
            for (str = opt->help; *str; str++)
            {
                putchar(*str);
                if (*str == '\n' && str[1])
                    printf("        ");
            }
            if (str[-1] != '\n')
                putchar('\n');
        }
        Print_default_value(opt);
        printf("\n");
    }
    printf("Most of these options can also be set in the .xpilotrc file\n"
           "in your home directory.\n"
           "Each key option may have multiple keys bound to it and\n"
           "one key may be used by multiple key options.\n"
           "If no server is specified on the command line, xpilot will\n"
           "display a welcome screen where you can select a server.\n");

    exit(1);
}

static void Version(void)
{
    printf("%s %s\n", Program_name(), VERSION);
    exit(0);
}

bool Set_noarg_option(xp_option_t *opt, bool value, xp_option_origin_t origin)
{
    assert(opt);
    assert(opt->type == xp_noarg_option);
    assert(opt->noarg_ptr);

    *opt->noarg_ptr = value;
    opt->origin = origin;

    return true;
}

bool Set_bool_option(xp_option_t *opt, bool value, xp_option_origin_t origin)
{
    bool retval = true;

    assert(opt);
    assert(opt->type == xp_bool_option);
    assert(opt->bool_ptr);

    if (opt->bool_setfunc)
        retval = opt->bool_setfunc(opt, value);
    else
        *opt->bool_ptr = value;

    if (retval)
        opt->origin = origin;

    return retval;
}

bool Set_int_option(xp_option_t *opt, int value, xp_option_origin_t origin)
{
    bool retval = true;

    assert(opt);
    assert(opt->type == xp_int_option);
    assert(opt->int_ptr);

    if (origin == xp_option_origin_setcmd)
    {
        char msg[MSG_LEN];

        if (value < opt->int_minval)
        {
            snprintf(msg, sizeof(msg),
                     "Minimum value for option %s is %d. [*Client reply*]",
                     opt->name, opt->int_minval);
            Add_message(msg);
        }
        if (value > opt->int_maxval)
        {
            snprintf(msg, sizeof(msg),
                     "Maximum value for option %s is %d. [*Client reply*]",
                     opt->name, opt->int_maxval);
            Add_message(msg);
        }
    }
    else
    {
        if (!(value >= opt->int_minval && value <= opt->int_maxval))
        {
            warn("Bad value %d for option \"%s\", using default...",
                 value, opt->name);
            value = opt->int_defval;
        }
    }

    LIMIT(value, opt->int_minval, opt->int_maxval);

    if (opt->int_setfunc)
        retval = opt->int_setfunc(opt, value);
    else
        *opt->int_ptr = value;

    if (retval)
        opt->origin = origin;

    return retval;
}

bool Set_double_option(xp_option_t *opt, double value,
                       xp_option_origin_t origin)
{
    bool retval = true;

    assert(opt);
    assert(opt->type == xp_double_option);
    assert(opt->dbl_ptr);

    if (origin == xp_option_origin_setcmd)
    {
        char msg[MSG_LEN];

        if (value < opt->dbl_minval)
        {
            snprintf(msg, sizeof(msg),
                     "Minimum value for option %s is %.3f. [*Client reply*]",
                     opt->name, opt->dbl_minval);
            Add_message(msg);
        }
        if (value > opt->dbl_maxval)
        {
            snprintf(msg, sizeof(msg),
                     "Maximum value for option %s is %.3f. [*Client reply*]",
                     opt->name, opt->dbl_maxval);
            Add_message(msg);
        }
    }
    else
    {
        if (!(value >= opt->dbl_minval && value <= opt->dbl_maxval))
        {
            warn("Bad value %.3f for option \"%s\", using default...",
                 value, opt->name);
            value = opt->dbl_defval;
        }
    }

    LIMIT(value, opt->dbl_minval, opt->dbl_maxval);

    if (opt->dbl_setfunc)
        retval = opt->dbl_setfunc(opt, value);
    else
        *opt->dbl_ptr = value;

    if (retval)
        opt->origin = origin;

    return retval;
}

bool Set_string_option(xp_option_t *opt, const char *value,
                       xp_option_origin_t origin)
{
    bool retval = true;

    assert(opt);
    assert(opt->type == xp_const_char_star_option);
    assert(opt->str_ptr || (opt->str_setfunc && opt->str_getfunc));
    assert(value); /* allow nullptr ? */

    /*
     * The reason string options don't assume a static area is that that
     * would not allow dynamically allocated strings of arbitrary size.
     */
    if (opt->str_setfunc)
        retval = opt->str_setfunc(opt, value);
    else
        strlcpy(opt->str_ptr, value, opt->str_size);

    if (retval)
        opt->origin = origin;

    return retval;
}

std::vector<xp_keydefs_t> keydefsVector;

/*
 * This function is used when platform specific code has an event where
 * the user has pressed or released the key defined by the keysym 'ks'.
 * When the key state has changed, the first call to this function should have
 * 'reset' true, then following calls related to the same event should
 * have 'reset' false. For each returned xpilot key, the calling code
 * should call some handler. The function should be called until it returns
 * KEY_DUMMY.
 */
keys_t Generic_lookup_key(xp_keysym_t ks, bool reset)
{
    warn("Generic_lookup_key: ks = %d, reset = %d, num_keydefs = %d", ks, reset, keydefsVector.size());

    keys_t ret = KEY_DUMMY;
    static int i = 0;

    if (reset)
        i = 0;

    /*
     * Variable 'i' is already initialized.
     * Use brute force linear search to find the key.
     */
    for (; i < keydefsVector.size(); i++)
    {
        if (ks == keydefsVector[i].keysym)
        {
            ret = keydefsVector[i].key;
            i++;
            break;
        }
    }

    return ret;
}

static void Store_keydef(int ks, keys_t key)
{
    int i;
    xp_keydefs_t keydef;

    /*
     * first check if pair (ks, key) already exists
     */
    for (i = 0; i < keydefsVector.size(); i++)
    {
        xp_keydefs_t *kd = &keydefsVector[i];

        if (kd->keysym == ks && kd->key == key)
        {
            warn("Store_keydef: Pair (%d, %d) exist from before", ks, (int)key);
            /*
             * already exists, no need to store
             */
            return;
        }
    }

    keydef.keysym = ks;
    keydef.key = key;

    /*
     * find first KEY_DUMMY after lazy deletion
     */
    for (i = 0; i < keydefsVector.size(); i++)
    {
        xp_keydefs_t *kd = &keydefsVector[i];

        if (kd->key == KEY_DUMMY)
        {
            assert(kd->keysym == XP_KS_UNKNOWN);
            warn("Store_keydef: Found dummy at index %d", i);
            *kd = keydef;
        }
    }

    /*
     * no lazily deleted entry, ok, just store it then
     */
    warn("Storing keydef: %d, %d", keydef.key, keydef.keysym);
    keydefsVector.push_back(keydef);
}

static void Remove_key_from_keydefs(keys_t key)
{
    int i;

    assert(key != KEY_DUMMY);
    for (i = 0; i < keydefsVector.size(); i++)
    {
        xp_keydefs_t *kd = &keydefsVector[i];

        /*
         * lazy deletion
         */
        if (kd->key == key)
        {
            /*warn("Remove_key_from_keydefs: Removing key at index %d", i);*/
            kd->keysym = XP_KS_UNKNOWN;
            kd->key = KEY_DUMMY;
        }
    }
}

static bool Set_key_option(xp_option_t *opt, const char *value,
                           xp_option_origin_t origin)
{
    /*bool retval = true;*/
    char *str, *valcpy;

    assert(opt);
    assert(opt->name);
    assert(opt->type == xp_key_option);
    assert(opt->key != KEY_DUMMY);
    assert(value);

    warn("Set_key_option: Setting key option %s to \"%s\"", opt->name, value);

    /*
     * First remove the old setting.
     */
    XFREE(opt->key_string);
    Remove_key_from_keydefs(opt->key);

    /*
     * Store the new setting.
     */
    opt->key_string = xp_safe_strdup(value);
    valcpy = xp_safe_strdup(value);
    for (str = strtok(valcpy, " \t\r\n");
         str != nullptr;
         str = strtok(nullptr, " \t\r\n"))
    {
        xp_keysym_t ks;

        /*
         * You can write "none" for keys in xpilotrc to disable the key.
         */
        if (!strcasecmp(str, "none"))
            continue;

        ks = String_to_xp_keysym(str);
        if (ks == XP_KS_UNKNOWN)
        {
            warn("Invalid keysym \"%s\" for key \"%s\".\n", str, opt->name);
            continue;
        }

        Store_keydef(ks, opt->key);
    }

    /* in fact if we only get invalid keysyms we should return false */
    opt->origin = origin;
    XFREE(valcpy);
    return true;
}

static bool is_legal_value(xp_option_type_t type, const char *value)
{
    if (type == xp_noarg_option || type == xp_bool_option)
    {
        if (ON(value) || OFF(value))
            return true;
        return false;
    }
    if (type == xp_int_option)
    {
        int foo;

        if (sscanf(value, "%d", &foo) <= 0)
            return false;
        return true;
    }
    if (type == xp_double_option)
    {
        double foo;

        if (sscanf(value, "%lf", &foo) <= 0)
            return false;
        return true;
    }
    return true;
}

bool Set_option(const char *name, const char *value, xp_option_origin_t origin)
{
    xp_option_t *opt;

    opt = Find_option(name);
    if (!opt)
        /* unknown */
        return false;

    if (!is_legal_value(opt->type, value))
    {
        if (origin != xp_option_origin_setcmd)
            warn("Bad value \"%s\" for option %s.", value, opt->name);
        else
        {
            char msg[MSG_LEN];

            snprintf(msg, sizeof(msg),
                     "Bad value \"%s\" for option %s. [*Client reply*]",
                     value, opt->name);
            Add_message(msg);
        }
        return false;
    }

    switch (opt->type)
    {
    case xp_noarg_option:
        return Set_noarg_option(opt, ON(value) ? true : false, origin);
    case xp_bool_option:
        return Set_bool_option(opt, ON(value) ? true : false, origin);
    case xp_int_option:
        return Set_int_option(opt, atoi(value), origin);
    case xp_double_option:
        return Set_double_option(opt, atof(value), origin);
    case xp_const_char_star_option:
        return Set_string_option(opt, value, origin);
    case xp_key_option:
        return Set_key_option(opt, value, origin);
    default:
        assert(0 && "TODO");
    }
    return false;
}

/*
 * kps - these commands need some fine tuning.
 * TODO - unset a value, i.e. set it to empty
 */
/*
 * Handler for \set client command.
 */
void Set_command(const char *args)
{
    char *name, *value, *valcpy;
    xp_option_t *opt;
    char msg[MSG_LEN];

    assert(args);

    valcpy = xp_safe_strdup(args);

    name = strtok(valcpy, " \t\r\n");
    value = strtok(nullptr, "");

    opt = Find_option(name);

    if (!opt)
    {
        snprintf(msg, sizeof(msg),
                 "Unknown option \"%s\". [*Client reply*]", name);
        Add_message(msg);
        goto out;
    }

    if (!value)
    {
        Add_message("Set command needs an option and a value. "
                    "[*Client reply*]");
        goto out;
    }
    else
    {
        const char *newvalue;
        const char *nm = Option_get_name(opt);

        Set_option(name, value, xp_option_origin_setcmd);

        newvalue = Option_value_to_string(opt);
        snprintf(msg, sizeof(msg),
                 "The value of %s is now %s. [*Client reply*]",
                 nm, newvalue);
        Add_message(msg);
    }

out:
    XFREE(valcpy);
}

const char *Option_value_to_string(xp_option_t *opt)
{
    static char buf[MSG_LEN];

    switch (opt->type)
    {
    case xp_noarg_option:
        sprintf(buf, "%s", *opt->noarg_ptr ? "yes" : "no");
        break;
    case xp_bool_option:
        sprintf(buf, "%s", *opt->bool_ptr ? "yes" : "no");
        break;
    case xp_int_option:
        sprintf(buf, "%d", *opt->int_ptr);
        break;
    case xp_double_option:
        sprintf(buf, "%.3f", *opt->dbl_ptr);
        break;
    case xp_const_char_star_option:
        /*
         * Assertion in Store_option guarantees one of these is not nullptr.
         */
        if (opt->str_getfunc)
            return opt->str_getfunc(opt);
        else
            return opt->str_ptr;
    case xp_key_option:
        assert(opt->key_string);
        return opt->key_string;
    default:
        assert(0 && "Unknown option type");
    }
    return buf;
}

/*
 * Handler for \get client command.
 */
void Get_command(const char *args)
{
    char *name, *valcpy;
    xp_option_t *opt;
    char msg[MSG_LEN];

    assert(args);

    valcpy = xp_safe_strdup(args);

    name = strtok(valcpy, " \t\r\n");
    opt = Find_option(name);

    if (opt)
    {
        const char *val = Option_value_to_string(opt);
        const char *nm = Option_get_name(opt);

        if (val && strlen(val) > 0)
            snprintf(msg, sizeof(msg),
                     "The value of %s is %s. [*Client reply*]", nm, val);
        else
            snprintf(msg, sizeof(msg),
                     "The option %s has no value. [*Client reply*]", nm);
        Add_message(msg);
    }
    else
    {
        snprintf(msg, sizeof(msg),
                 "No client option named \"%s\". [*Client reply*]", name);
        Add_message(msg);
    }

    XFREE(valcpy);
}

/*
 * NOTE: Store option assumes the passed pointers will remain valid.
 *
 * TODO: Store option should just store the options in the array, but not call any
 * of the setter functions, these should be called later in a specific order, because
 * some settings might assume other code has been executed.
 */
void Store_option(xp_option_t *opt)
{
    xp_option_t option;

    assert(opt->name);
    assert(strlen(opt->name) > 0);
    assert(opt->help);
    assert(strlen(opt->help) > 0);

    /*
     * Let's not allow several options with the same name
     */
    if (Find_option(opt->name) != nullptr)
    {
        warn("Trying to store duplicate option \"%s\"", opt->name);
        assert(0);
    }

    /*
     * Check that default value is in range
     * NOTE: these assertions will hold also for options of other types
     */
    assert(opt->int_defval >= opt->int_minval);
    assert(opt->int_defval <= opt->int_maxval);
    assert(opt->dbl_defval >= opt->dbl_minval);
    assert(opt->dbl_defval <= opt->dbl_maxval);

    memcpy(&option, opt, sizeof(xp_option_t));

    optionsVector.push_back(option);

    // TODO set all the values later.

    opt = Find_option(opt->name);
    assert(opt);

    /* Set the default value. */
    switch (opt->type)
    {
    case xp_noarg_option:
        Set_noarg_option(opt, false, xp_option_origin_default);
        break;
    case xp_bool_option:
        Set_bool_option(opt, opt->bool_defval, xp_option_origin_default);
        break;
    case xp_int_option:
        Set_int_option(opt, opt->int_defval, xp_option_origin_default);
        break;
    case xp_double_option:
        Set_double_option(opt, opt->dbl_defval, xp_option_origin_default);
        break;
    case xp_const_char_star_option:
        assert(opt->str_defval);
        assert(opt->str_ptr || (opt->str_setfunc && opt->str_getfunc));
        Set_string_option(opt, opt->str_defval, xp_option_origin_default);
        break;
    case xp_key_option:
        assert(opt->key_defval);
        assert(opt->key != KEY_DUMMY);
        Set_key_option(opt, opt->key_defval, xp_option_origin_default);
        break;
    default:
        warn("Could not set default value for option %s", opt->name);
        break;
    }
}

static inline bool is_noarg_option(const char *name)
{
    xp_option_t *opt = Find_option(name);

    if (!opt || opt->type != xp_noarg_option)
        return false;
    return true;
}

void Parse_options(int *argcp, char **argvp)
{
    int arg_ind, num_remaining_args, num_servers = 0, i;

    std::string path = Xpilotrc_get_filename();
    Xpilotrc_read(path.c_str());

    /*
     * Here we step trough argc - 1 arguments, leaving
     * only the arguments that might be server names.
     */
    arg_ind = 1;
    num_remaining_args = *argcp - 1;

    while (num_remaining_args > 0)
    {
        if (argvp[arg_ind][0] == '-')
        {
            char *arg = &argvp[arg_ind][1];

            /*
             * kps -
             * Incomplete GNU style option support, this only works for
             * options with no argument, e.g. --version
             * A complete implementation should also support option given
             * like this:
             * --option=value
             */
            if (arg[0] == '-')
                arg++;

            if (is_noarg_option(arg))
            {
                Set_option(arg, "true", xp_option_origin_cmdline);
                num_remaining_args--;
                for (i = 0; i < num_remaining_args; i++)
                    argvp[arg_ind + i] = argvp[arg_ind + i + 1];
            }
            else
            {
                bool ok = false;

                if (num_remaining_args >= 2)
                {
                    ok = Set_option(arg, argvp[arg_ind + 1],
                                    xp_option_origin_cmdline);
                    if (ok)
                    {
                        num_remaining_args -= 2;
                        for (i = 0; i < num_remaining_args; i++)
                            argvp[arg_ind + i] = argvp[arg_ind + i + 2];
                    }
                }

                if (!ok)
                {
                    warn("Unknown or incomplete option '%s'", argvp[arg_ind]);
                    warn("Type: %s -help to see a list of options", argvp[0]);
                    exit(1);
                }
            }
        }
        else
        {
            /* assume this is a server name. */
            arg_ind++;
            num_remaining_args--;
            num_servers++;
        }
    }

    /*
     * The remaining args are assumed to be names of servers to try to contact.
     * + 1 is for the program name.
     */
    for (i = num_servers + 1; i < *argcp; i++)
        argvp[i] = nullptr;
    *argcp = num_servers + 1;

    if (xpArgs.help)
        Usage();

    if (xpArgs.version)
        Version();

#ifdef SOUND
    audioInit(connectParam.disp_name);
#endif /* SOUND */
}

const char *Get_keyHelpString(keys_t key)
{
    int i;
    char *nl;
    static char buf[MAX_CHARS];

    for (i = 0; i < optionsVector.size(); i++)
    {
        xp_option_t *opt = Option_by_index(i);

        if (opt->key == key)
        {
            strlcpy(buf, opt->help, sizeof buf);
            if ((nl = strchr(buf, '\n')) != nullptr)
                *nl = '\0';
            return buf;
        }
    }

    return nullptr;
}
