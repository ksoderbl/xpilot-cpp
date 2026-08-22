/*
 * XPilot, a multiplayer gravity war game.  Copyright (C) 1991-2001 by
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

#include <cstddef>
#include <cstdio>
#include <cstring>

#include "checknames.h"
#include "commonmacros.h"
#include "commonproto.h"
#include "clientpack.h"
#include "portability.h"
#include "socklib.h"
#include "xpconfig.h"

#include "caudio.h"
#include "client.h"
#include "gfx2d.h"
#include "messages.h"
#include "netclient.h"
#include "paint.h"
#include "clientoption.h"
#include "clientoptions.h" // 2026 stuff
#include "clientrank.h"

static double hudScale; /* Scale for HUD drawing */

static bool Set_nickName(xp_option_t *opt, const char *value)
{
    assert(value);

    /*
     * This is a hack. User name will be used as nickname,
     * look in Set_userName().
     */
    if (strlen(value) == 0)
        return true;

    strlcpy(connectParam.nick_name, value, sizeof(connectParam.nick_name));
    CAP_LETTER(connectParam.nick_name[0]);
    if (connectParam.nick_name[0] < 'A' || connectParam.nick_name[0] > 'Z')
    {
        warn("Your player name \"%s\" should start with an uppercase letter.",
             connectParam.nick_name);
        connectParam.nick_name[0] = 'X';
    }

    if (Check_nick_name(connectParam.nick_name) == NAME_ERROR)
    {
        char nick[MAX_NAME_LEN];

        strlcpy(nick, connectParam.nick_name, sizeof(nick));
        Fix_nick_name(connectParam.nick_name);
        warn("Fixing nick from \"%s\" to \"%s\".\n",
             nick, connectParam.nick_name);
    }

    /*warn("Nick name set to \"%s\".\n", connectParam.nick_name);*/

    return true;
}

static bool Set_userName(xp_option_t *opt, const char *value)
{
    char *cp = getenv("XPILOTUSER");

    assert(value);

    if (cp)
        strlcpy(connectParam.user_name, cp, sizeof(connectParam.user_name));
    else
        Get_login_name(connectParam.user_name, sizeof(connectParam.user_name));

    if (strlen(value) > 0)
        strlcpy(connectParam.user_name, value, sizeof(connectParam.user_name));

    if (Check_user_name(connectParam.user_name) == NAME_ERROR)
    {
        char user[MAX_NAME_LEN];

        strlcpy(user, connectParam.user_name, sizeof(user));
        Fix_user_name(connectParam.user_name);
        warn("Fixing username from \"%s\" to \"%s\".\n",
             user, connectParam.user_name);
    }

    /* hack - if nickname is not set, set nickname to username */
    if (strlen(connectParam.nick_name) == 0)
        Set_nickName(nullptr, connectParam.user_name);

    /*warn("User name set to \"%s\".\n", connectParam.user_name);*/

    return true;
}

static bool Set_hostName(xp_option_t *opt, const char *value)
{
    char *cp = getenv("XPILOTHOST");

    assert(value);

    connectParam.host_name[0] = '\0';
    if (cp)
        strlcpy(connectParam.host_name, cp, sizeof(connectParam.host_name));
    else
        sock_get_local_hostname(connectParam.host_name,
                                sizeof(connectParam.host_name), 0);

    if (strlen(value) > 0)
        strlcpy(connectParam.host_name, value, sizeof(connectParam.host_name));

    if (Check_host_name(connectParam.host_name) == NAME_ERROR)
    {
        char host[SOCK_HOSTNAME_LENGTH];

        strlcpy(host, connectParam.host_name, sizeof(host));
        Fix_host_name(connectParam.host_name);
        warn("Fixing host from \"%s\" to \"%s\".\n",
             host, connectParam.host_name);
    }

    /*warn("Host name set to \"%s\".\n", connectParam.host_name);*/

    return true;
}

static const char *Get_nickName(xp_option_t *opt)
{
    return connectParam.nick_name;
}

static const char *Get_userName(xp_option_t *opt)
{
    return connectParam.user_name;
}

static const char *Get_hostName(xp_option_t *opt)
{
    return connectParam.host_name;
}

static bool Set_team(xp_option_t *opt, int value)
{
    if (value >= 0 && value < MAX_TEAMS)
        connectParam.team = value;
    else
        connectParam.team = TEAM_NOT_SET;

    return true;
}

static bool Set_texturePath(xp_option_t *opt, const char *value)
{
    XFREE(texturePath);
    texturePath = xp_safe_strdup(value);
    if (realTexturePath == nullptr)
        realTexturePath = xp_safe_strdup(value);
    return true;
}

static const char *Get_texturePath(xp_option_t *opt)
{
    return texturePath;
}

/*
 * Ship shape option handling.
 */
static char *shipShapeSetting = nullptr;
static char *shipShapeFile = nullptr;

/*
 * This function trys to set the shipShape variable.
 *
 * First it looks if shipShapeSetting would be a suitable value.
 * If not, it assumes shipShapeSetting is the name of a shape
 * to be loaded from shipShapeFile.
 */
static void tryToSetShipShape(void)
{
    bool is_shape = false, valid;
    FILE *fp;
    char *ptr, *str, line[1024], *ss_candidate = nullptr;

    /*
     * Let's allow overriding shipshape with another. E.g the first
     * one is from xpilotrc, the other from command line.
     */
    XFREE(shipShape);

    /* If there is no shipShapeSetting, there is nothing we can do. */
    if (shipShapeSetting == nullptr || strlen(shipShapeSetting) == 0)
        return;

    /*
     * Let's determine if shipShapeSetting is the actual ship shape or
     * its name.
     */
    if (strchr(shipShapeSetting, '('))
        is_shape = true;

    if (is_shape)
    {
        valid = Validate_shape_str(shipShapeSetting);
        if (valid)
        {
            shipShape = xp_safe_strdup(shipShapeSetting);
            xpinfo("Your shipShape is valid. Have a nice day.\n");
        }
        else
            warn("Your shipShape isn't valid. Please fix it.");
        return;
    }

    /*
     * shipShapeSetting is the name of the shipshape. Without a
     * shipShapeFile we can't proceed.
     */
    if (!shipShapeFile || strlen(shipShapeFile) == 0)
        return;

    fp = fopen(shipShapeFile, "r");
    if (!fp)
    {
        error("Can't open shipShapeFile \"%s\".", shipShapeFile);
        return;
    }

    while (fgets(line, sizeof line, fp))
    {
        if ((str = strstr(line, "(name:")) != nullptr || (str = strstr(line, "(NM:")) != nullptr)
        {

            str = strchr(str, ':');

            while (*++str == ' ')
                ;

            if ((ptr = strchr(str, ')')) != nullptr)
                *ptr = '\0';
            if (!strcmp(str, shipShapeSetting))
            {
                /* Gotcha */
                if (ptr != nullptr)
                    *ptr = ')';
                ss_candidate = xp_safe_strdup(line);
                break;
            }
        }
    }
    fclose(fp);

    if (!ss_candidate)
    {
        warn("Could not find the ship \"%s\" in shipShapeFile %s.",
             shipShapeSetting, shipShapeFile);
        return;
    }

    valid = Validate_shape_str(ss_candidate);
    if (valid)
    {
        xpinfo("Ship shape \"%s\" is now in use.\n", shipShapeSetting);
        shipShape = ss_candidate;
    }
    else
    {
        XFREE(ss_candidate);
        warn("Your shipShape \"%s\" isn't valid. Please fix it.",
             shipShapeSetting);
    }
}

/*
 * Shipshape options.
 */
static bool Set_shipShape(xp_option_t *opt, const char *value)
{
    XFREE(shipShapeSetting);
    shipShapeSetting = xp_safe_strdup(value);
    tryToSetShipShape();

    return true;
}

static const char *Get_shipShape(xp_option_t *opt)
{
    return shipShapeSetting;
}

static bool Set_shipShapeFile(xp_option_t *opt, const char *value)
{
    XFREE(shipShapeFile);
    shipShapeFile = xp_safe_strdup(value);
    tryToSetShipShape();

    return true;
}

static const char *Get_shipShapeFile(xp_option_t *opt)
{
    return shipShapeFile;
}

static bool Set_power(xp_option_t *opt, double val)
{
    if (Setup)
        Send_power(val);
    power = val;
    controlTime = CONTROL_TIME;
    return true;
}
static bool Set_turnSpeed(xp_option_t *opt, double val)
{
    if (Setup)
        Send_turnspeed(val);
    turnspeed = val;
    controlTime = CONTROL_TIME;
    return true;
}
static bool Set_turnResistance(xp_option_t *opt, double val)
{
    if (Setup)
        Send_turnresistance(val);
    turnresistance = val;
    return true;
}

static bool Set_altPower(xp_option_t *opt, double val)
{
    if (Setup)
        Send_power_s(val);
    power_s = val;
    controlTime = CONTROL_TIME;
    return true;
}
static bool Set_altTurnSpeed(xp_option_t *opt, double val)
{
    if (Setup)
        Send_turnspeed_s(val);
    turnspeed_s = val;
    controlTime = CONTROL_TIME;
    return true;
}
static bool Set_altTurnResistance(xp_option_t *opt, double val)
{
    if (Setup)
        Send_turnresistance_s(val);
    turnresistance_s = val;
    return true;
}

static bool Set_autoShield(xp_option_t *opt, bool val)
{
    Set_auto_shield(val);
    return true;
}

static bool Set_toggleShield(xp_option_t *opt, bool val)
{
    Set_toggle_shield(val);
    return true;
}

static bool Set_maxFPS(xp_option_t *opt, int val)
{
    maxFPS = val;
    // Check_client_fps(); // TODO: ENABLE
    return true;
}

static bool Set_maxMouseTurnsPS(xp_option_t *opt, int val)
{
    maxMouseTurnsPS = val;
    if (maxMouseTurnsPS > 0)
    {
        mouseMovementInterval = 1000000 / maxMouseTurnsPS;
        if (mouseMovementInterval * maxMouseTurnsPS < 1000000)
            mouseMovementInterval++;
    }
    /*warn("mouseMovementInterval = %d", mouseMovementInterval);*/
    return true;
}

static bool Set_sparkProb(xp_option_t *opt, double val)
{
    sparkProb = val;
    spark_rand = (int)(sparkProb * MAX_SPARK_RAND + 0.5);
    // Check_view_dimensions1(); // TODO: ENABLE
    return true;
}

static bool Set_hudScale(xp_option_t *opt, double value)
{
    hudScale = value;
    hudSize = (int)(MIN_HUD_SIZE * hudScale);
    return true;
}

static bool Set_backgroundPointDist(xp_option_t *opt, int val)
{
    backgroundPointDist = val;
    if (oldServer)
        Map_dots();
    return true;
}

static bool Set_backgroundPointSize(xp_option_t *opt, int val)
{
    backgroundPointSize = val;
    if (oldServer)
        Map_dots();
    return true;
}

static bool Set_slidingRadar(xp_option_t *opt, bool val)
{
    clientOptions.instruments.slidingRadar = val;
    Paint_sliding_radar();
    return true;
}

static bool Set_outlineWorld(xp_option_t *opt, bool val)
{
    clientOptions.instruments.outlineWorld = val;
    if (oldServer && Setup)
    {
        /* don't bother to check if recalculations are really needed. */
        Map_restore(0, 0, Setup->x, Setup->y);
        Map_blue(0, 0, Setup->x, Setup->y);
    }
    return true;
}

static bool Set_filledWorld(xp_option_t *opt, bool val)
{
    clientOptions.instruments.filledWorld = val;
    if (oldServer && Setup)
    {
        /* don't bother to check if recalculations are really needed. */
        Map_restore(0, 0, Setup->x, Setup->y);
        Map_blue(0, 0, Setup->x, Setup->y);
    }
    return true;
}

static bool Set_texturedWalls(xp_option_t *opt, bool val)
{
    clientOptions.instruments.texturedWalls = val;

    if (Setup)
    {
        if (oldServer)
        {
            /* don't bother to check if recalculations are really needed. */
            Map_restore(0, 0, Setup->x, Setup->y);
            Map_blue(0, 0, Setup->x, Setup->y);
        }
        else
            Mapdata_setup(Setup->data_url);
    }
    return true;
}

static bool Set_showDecor(xp_option_t *opt, bool val)
{
    clientOptions.instruments.showDecor = val;
    if (!Setup)
        return true;
    if (oldServer)
        Map_dots();
    Paint_world_radar();
    return true;
}

static bool Set_dirPrediction(xp_option_t *opt, bool val)
{
    if (val)
    {
        if (!clientOptions.dirPrediction)
        {
            /* reset pointer movements */
            int m;
            for (m = 0; m < MAX_POINTER_MOVES; m++)
                pointer_moves[m].id = -1;
            pointer_move_next = 0;
            last_keyboard_ack = 0;
            clientOptions.dirPrediction = true;
        }
    }
    else
        clientOptions.dirPrediction = false;

    return true;
}

int protocolVersion = POLYGON_VERSION;
static char protocolVersionStr[32];

static bool Set_protocolVersion(xp_option_t *opt, const char *value)
{
    if (sscanf(value, "%x", &protocolVersion) <= 0)
        return false;
    return true;
}

static const char *Get_protocolVersion(xp_option_t *opt)
{
    snprintf(protocolVersionStr, sizeof protocolVersionStr, "%04x",
             protocolVersion);
    return protocolVersionStr;
}

void defaultCleanup(void)
{
    keydefsVector.clear();
    XFREE(texturePath);
    XFREE(shipShape);

    audioCleanup();
}

xp_option_t default_options[] = {

    XP_NOARG_OPTION(
        "help",
        &xpArgs.help,
        XP_OPTFLAG_NEVER_SAVE,
        "Display this help message.\n"),

    XP_NOARG_OPTION(
        "version",
        &xpArgs.version,
        XP_OPTFLAG_NEVER_SAVE,
        "Show the source code version.\n"),

    XP_NOARG_OPTION(
        "join",
        &xpArgs.auto_connect,
        XP_OPTFLAG_NEVER_SAVE,
        "Join the game immediately, no questions asked.\n"),

    XP_NOARG_OPTION(
        "text",
        &xpArgs.text,
        XP_OPTFLAG_NEVER_SAVE,
        "Use the simple text interface to contact a server\n"
        "instead of the graphical user interface.\n"),

    XP_NOARG_OPTION(
        "list",
        &xpArgs.list_servers,
        XP_OPTFLAG_NEVER_SAVE,
        "List all servers running on the local network.\n"),

    XP_CONST_CHAR_STAR_OPTION(
        "shutdown",
        "",
        xpArgs.shutdown_reason,
        sizeof xpArgs.shutdown_reason,
        nullptr, nullptr, nullptr,
        XP_OPTFLAG_NEVER_SAVE,
        "Shutdown the server with a message.\n"
        "The message used is the first argument to this option.\n"),

    XP_CONST_CHAR_STAR_OPTION(
        "name",
        "",
        nullptr, 0,
        Set_nickName, nullptr, Get_nickName,
        XP_OPTFLAG_KEEP,
        "Set the nickname.\n"),

    XP_CONST_CHAR_STAR_OPTION(
        "user",
        "",
        nullptr, 0,
        Set_userName, nullptr, Get_userName,
        XP_OPTFLAG_KEEP,
        "Set the username.\n"),

    XP_CONST_CHAR_STAR_OPTION(
        "host",
        "",
        nullptr, 0,
        Set_hostName, nullptr, Get_hostName,
        XP_OPTFLAG_KEEP,
        "Set the hostname.\n"),

    XP_INT_OPTION(
        "team",
        TEAM_NOT_SET,
        0,
        TEAM_NOT_SET,
        &connectParam.team,
        Set_team,
        XP_OPTFLAG_KEEP,
        "Set the team to join.\n"),

    XP_INT_OPTION(
        "port",
        SERVER_PORT,
        0,
        65535,
        &connectParam.contact_port,
        nullptr,
        XP_OPTFLAG_KEEP,
        "Set the port number of the server.\n"
        "Almost all servers use the default port, which is the recommended\n"
        "policy.  You can find out about which port is used by a server by\n"
        "querying the XPilot Meta server.\n"),

    XP_INT_OPTION(
        "clientPortStart",
        0,
        0,
        65535,
        &clientPortStart,
        nullptr,
        XP_OPTFLAG_KEEP,
        "Use UDP ports clientPortStart - clientPortEnd (for firewalls).\n"
        /* TODO: describe what value 0 means */),

    XP_INT_OPTION(
        "clientPortEnd",
        0,
        0,
        65535,
        &clientPortEnd,
        nullptr,
        XP_OPTFLAG_KEEP,
        "Use UDP ports clientPortStart - clientPortEnd (for firewalls).\n"),

    XP_DOUBLE_OPTION(
        "power",
        55.0,
        MIN_PLAYER_POWER,
        MAX_PLAYER_POWER,
        &power,
        Set_power,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Set the engine power.\n"
        "Valid values are in the range 5-55.\n"),

    XP_DOUBLE_OPTION(
        "turnSpeed",
        16.0,
        MIN_PLAYER_TURNSPEED,
        MAX_PLAYER_TURNSPEED,
        &turnspeed,
        Set_turnSpeed,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Set the ship's turn speed.\n"
        "Valid values are in the range 4-64.\n"),

    XP_DOUBLE_OPTION(
        "turnResistance",
        0.0,
        MIN_PLAYER_TURNRESISTANCE,
        MAX_PLAYER_TURNRESISTANCE,
        &turnresistance,
        Set_turnResistance,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Set the ship's turn resistance.\n"
        "This determines the speed at which a ship stops turning.\n"
        "Valid values are in the range 0.0-1.0.\n"
        "This should always be 0, other values are for compatibility.\n"
        "See also turnSpeed.\n"),

    XP_DOUBLE_OPTION(
        "scaleFactor",
        1.0,
        MIN_SCALEFACTOR,
        MAX_SCALEFACTOR,
        &clData.scaleFactor,
        Set_scaleFactor,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Specifies scaling factor for the drawing window.\n"),

    XP_DOUBLE_OPTION(
        "altScaleFactor",
        2.0,
        MIN_SCALEFACTOR,
        MAX_SCALEFACTOR,
        &clData.altScaleFactor,
        Set_altScaleFactor,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Specifies alternative scaling factor for the drawing window.\n"),

    XP_INT_OPTION(
        "maxFPS",
        MAX_SUPPORTED_FPS,
        1,
        MAX_SUPPORTED_FPS,
        &maxFPS,
        Set_maxFPS,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Set maximum FPS supported by the client. The server will try to\n"
        "send at most this many frames per second to the client.\n"),

    XP_INT_OPTION(
        "maxMouseTurnsPS",
        0,
        0,
        MAX_SUPPORTED_FPS * 2,
        &maxMouseTurnsPS,
        Set_maxMouseTurnsPS,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Set maximum number of mouse turns sent per second\n"
        "Set to 0 to disable this feature (its mostly useful on modem)\n"),

    XP_INT_OPTION(
        "sparkSize",
        2,
        MIN_SPARK_SIZE,
        MAX_SPARK_SIZE,
        &sparkSize,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Size of sparks in pixels.\n"),

    XP_DOUBLE_OPTION(
        "sparkProb",
        0.4,
        0.0,
        1.0,
        &sparkProb,
        Set_sparkProb,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "The probablilty that sparks are drawn.\n"
        "This gives a sparkling effect.\n"
        "Valid values are in the range [0.0-1.0]\n"),

    XP_INT_OPTION(
        "hudRadarDotSize",
        10,
        1,
        SHIP_SZ,
        &hudRadarDotSize,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Which size to use for drawing the hudradar dots.\n"),

    XP_DOUBLE_OPTION(
        "hudRadarScale",
        1.5,
        0.5,
        4.0,
        &hudRadarScale,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "The relative size of the hudradar.\n"),

    XP_DOUBLE_OPTION(
        "hudRadarLimit",
        0.05,
        0.05,
        5.0,
        &hudRadarLimit,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Hudradar dots closer than this to your ship are not drawn.\n"
        "A value of 1.0 means that the dots are not drawn for ships in\n"
        "your active view area.\n"),

    XP_DOUBLE_OPTION(
        "hudScale",
        2.0,
        1.0,
        6.0,
        &hudScale,
        Set_hudScale,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "How much larger than minimum size to draw the hud.\n"),

    XP_INT_OPTION(
        "baseWarningType",
        3,
        0,
        3,
        &baseWarningType,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Which type of base warning you prefer.\n"
        "A value of 0 disables base warning.\n"
        "A value of 1 draws a time meter on a base when a ship is appearing.\n"
        "A value of 2 makes the base name flash when a ship is appearing.\n"
        "A value of 3 combines the effects of values 1 and 2.\n"),

    XP_INT_OPTION(
        "maxCharsInNames",
        MAX_NAME_LEN,
        0,
        MAX_NAME_LEN,
        &maxCharsInNames,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Maximum number of characters to paint in names on game area.\n"),

    XP_BOOL_OPTION(
        "slidingRadar",
        true,
        &clientOptions.instruments.slidingRadar,
        Set_slidingRadar,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "If the game is in edgewrap mode then the radar will keep your\n"
        "position on the radar in the center and raw the rest of the radar\n"
        "around it.  Note that this requires a fast graphics system.\n"),

    XP_BOOL_OPTION(
        "dirPrediction",
        false,
        &clientOptions.dirPrediction,
        Set_dirPrediction,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Client paints the wanted direction of your ship instead of what\n"
        "the server sent you. Possible differences are corrected in\n"
        "roundtrip time.\n"),

    XP_BOOL_OPTION(
        "showShipShapes",
        true,
        &clientOptions.instruments.showShipShapes,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Should others' shipshapes be displayed or not.\n"),

    XP_BOOL_OPTION(
        "showMyShipShape",
        true,
        &clientOptions.instruments.showMyShipShape,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Should your own shipshape be displayed or not.\n"),

    XP_BOOL_OPTION(
        "showShipShapesHack",
        false,
        &clientOptions.instruments.showShipShapesHack,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Should the ship shapes hack be displayed or not.\n"),

    XP_BOOL_OPTION(
        "showLivesByShip",
        false,
        &clientOptions.instruments.showLivesByShip,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Paint remaining lives next to ships.\n"),

    XP_BOOL_OPTION(
        "showMessages",
        true,
        &clientOptions.instruments.showMessages,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Should game messages appear on screen.\n"),

    XP_BOOL_OPTION(
        "showItems",
        true,
        &clientOptions.instruments.showItems,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Should owned items be displayed permanently on the HUD?\n"),

    XP_DOUBLE_OPTION(
        "showItemsTime",
        5.0,
        MIN_SHOW_ITEMS_TIME,
        MAX_SHOW_ITEMS_TIME,
        &showItemsTime,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "If showItems is false, the time in seconds to display item\n"
        "information on the HUD when it has changed.\n"),

    XP_BOOL_OPTION(
        "filledWorld",
        false,
        &clientOptions.instruments.filledWorld,
        Set_filledWorld,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Draws the walls solid, filled with one color,\n"
        "unless overridden by texture.\n"
        "Be warned that this option needs fast graphics.\n"),

    XP_BOOL_OPTION(
        "texturedWalls",
        true,
        &clientOptions.instruments.texturedWalls,
        Set_texturedWalls,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Allows drawing polygon bitmaps specified by the (new-style) map.\n"
        "Be warned that this needs a reasonably fast graphics system.\n"),

    XP_CONST_CHAR_STAR_OPTION(
        "protocolVersion",
        "",
        nullptr, 0,
        Set_protocolVersion, nullptr, Get_protocolVersion,
        XP_OPTFLAG_KEEP,
        "Which protocol version to prefer when joining servers.\n"),

    XP_BOOL_OPTION(
        "outlineWorld",
        false,
        &clientOptions.instruments.outlineWorld,
        Set_outlineWorld,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Draws only the outline of all the wall blocks\n"
        "on block based maps.\n"),

    XP_BOOL_OPTION(
        "showDecor",
        true,
        &clientOptions.instruments.showDecor,
        Set_showDecor,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Should decorations be displayed on the screen and radar?\n"),

    XP_BOOL_OPTION(
        "outlineDecor",
        false,
        &clientOptions.instruments.outlineDecor,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Draws only the outline of the map decoration.\n"),

    XP_BOOL_OPTION(
        "filledDecor",
        false,
        &clientOptions.instruments.filledDecor,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Draws filled decorations.\n"),

    XP_BOOL_OPTION(
        "texturedDecor",
        false,
        &clientOptions.instruments.texturedDecor,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Draws the map decoration filled with a texture pattern.\n"),

    XP_BOOL_OPTION(
        "clientRanker",
        false,
        &clientOptions.instruments.clientRanker,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Scan messages and make personal kill/death ranking.\n"),

    XP_BOOL_OPTION(
        "clockAMPM",
        false,
        &clientOptions.instruments.clockAMPM,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Use AMPM format for clock display instead of 24 hour format.\n"),

    XP_INT_OPTION(
        "shotSize",
        6,
        MIN_SHOT_SIZE,
        MAX_SHOT_SIZE,
        &shotSize,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "The size of shots in pixels.\n"),

    XP_INT_OPTION(
        "teamShotSize",
        4,
        MIN_TEAMSHOT_SIZE,
        MAX_TEAMSHOT_SIZE,
        &teamShotSize,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "The size of team shots in pixels.\n"
        "Note that team shots are drawn in teamShotColor.\n"),

    XP_INT_OPTION(
        "backgroundPointDist",
        8,
        0,
        10,
        &backgroundPointDist,
        Set_backgroundPointDist,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "The distance between points in the background measured in blocks.\n"
        "These are drawn in empty map regions to keep feeling for which\n"
        "direction the ship is moving to.\n"),

    XP_INT_OPTION(
        "backgroundPointSize",
        2,
        MIN_MAP_POINT_SIZE,
        MAX_MAP_POINT_SIZE,
        &backgroundPointSize,
        Set_backgroundPointSize,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Specifies the size of the background points.  0 means no points.\n"),

    XP_DOUBLE_OPTION(
        "scoreObjectTime",
        4.0,
        0.0,
        10.0,
        &scoreObjectTime,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "How many seconds score objects remain visible on the map.\n"),

    XP_INT_OPTION(
        "charsPerSecond",
        100,
        10,
        255,
        &charsPerSecond,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Rate at which messages appear on screen in characters per second.\n"),

    XP_INT_OPTION(
        "maxMessages",
        8,
        1,
        MAX_MSGS,
        &maxMessages,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "The maximum number of messages to display at the same time.\n"),

    XP_INT_OPTION(
        "messagesToStdout",
        1,
        0,
        2,
        &messagesToStdout,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Send messages to standard output.\n"
        "0: Don't.\n"
        "1: Only player messages.\n"
        "2: Player and status messages.\n"),

    XP_INT_OPTION(
        "maxLinesInHistory",
        32,
        1,
        MAX_HIST_MSGS,
        &maxLinesInHistory,
        nullptr,
        XP_OPTFLAG_DEFAULT,
        "Number of your messages saved in the 'history' of the talk window.\n"
        "'history' is accessible with 'keyTalkCursorUp/Down'.\n"),

    XP_BOOL_OPTION(
        "toggleShield",
        false,
        &clientOptions.toggleShield,
        Set_toggleShield,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Are shields toggled by a keypress only?\n"),

    XP_BOOL_OPTION(
        "autoShield", /* Don auto-shield hack */
        true,
        &clientOptions.autoShield,
        Set_autoShield,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Are shields lowered automatically for weapon fire?\n"),

    XP_DOUBLE_OPTION(
        "fuelNotify",
        500.0,
        0.0,
        1000.0,
        &fuelNotify,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "The limit when the HUD fuel bar will become visible.\n"),

    XP_DOUBLE_OPTION(
        "fuelWarning",
        200.0,
        0.0,
        1000.0,
        &fuelWarning,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "The limit when the HUD fuel bar will start flashing.\n"),

    XP_DOUBLE_OPTION(
        "fuelCritical",
        100.0,
        0.0,
        1000.0,
        &fuelCritical,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "The limit when the HUD fuel bar will flash faster.\n"),

    XP_DOUBLE_OPTION(
        "speedFactHUD",
        0.0,
        -10.0,
        +10.0,
        &hud_move_fact,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "How much to move HUD to indicate the current velocity.\n"),

    XP_DOUBLE_OPTION(
        "speedFactPTR",
        0.0,
        -10.0,
        +10.0,
        &ptr_move_fact,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Uses a red line to indicate the current velocity and direction.\n"),

    XP_INT_OPTION(
        "showScoreDecimals",
        1,
        0,
        2,
        &showScoreDecimals,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "The number of decimals to use when displaying scores.\n"),

    XP_DOUBLE_OPTION(
        "altPower",
        55.0,
        MIN_PLAYER_POWER,
        MAX_PLAYER_POWER,
        &power_s,
        Set_altPower,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Set the ship's alternate engine power.\n"
        "See also the keySwapSettings option.\n"),

    XP_DOUBLE_OPTION(
        "altTurnSpeed",
        16.0,
        MIN_PLAYER_TURNSPEED,
        MAX_PLAYER_TURNSPEED,
        &turnspeed_s,
        Set_altTurnSpeed,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Set the ship's alternate turn speed.\n"
        "See also the keySwapSettings option.\n"),

    XP_DOUBLE_OPTION(
        "altTurnResistance",
        0.0,
        MIN_PLAYER_TURNRESISTANCE,
        MAX_PLAYER_TURNRESISTANCE,
        &turnresistance_s,
        Set_altTurnResistance,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Set the ship's alternate turn resistance.\n"
        "See also the keySwapSettings option.\n"),

#if 0
    /* kps - remove option later */
    XP_INT_OPTION(
    "receiveWindowSize",
    3,
    MIN_RECEIVE_WINDOW_SIZE,
    MAX_RECEIVE_WINDOW_SIZE,
    &receive_window_size,
    nullptr,
    XP_OPTFLAG_DEFAULT,
    "Too complicated.  Keep it on 3.\n"),
#endif

    XP_BOOL_OPTION(
        "markingLights",
        false,
        &clientOptions.markingLights,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Should the fighters have marking lights, just like airplanes?\n"),

    XP_CONST_CHAR_STAR_OPTION(
        "modifierBank1",
        "",
        modBankStr[0], sizeof modBankStr[0],
        nullptr, nullptr, nullptr, /* kps - add set and get functions */
        XP_OPTFLAG_DEFAULT,
        "The default weapon modifier values for the first modifier bank.\n"),

    XP_CONST_CHAR_STAR_OPTION(
        "modifierBank2",
        "",
        modBankStr[1], sizeof modBankStr[1],
        nullptr, nullptr, nullptr,
        XP_OPTFLAG_DEFAULT,
        "The default weapon modifier values for the second modifier bank.\n"),

    XP_CONST_CHAR_STAR_OPTION(
        "modifierBank3",
        "",
        modBankStr[2], sizeof modBankStr[2],
        nullptr, nullptr, nullptr,
        XP_OPTFLAG_DEFAULT,
        "The default weapon modifier values for the third modifier bank.\n"),

    XP_CONST_CHAR_STAR_OPTION(
        "modifierBank4",
        "",
        modBankStr[3], sizeof modBankStr[3],
        nullptr, nullptr, nullptr,
        XP_OPTFLAG_DEFAULT,
        "The default weapon modifier values for the fourth modifier bank.\n"),

    XP_CONST_CHAR_STAR_OPTION(
        "shipShape",
        "",
        nullptr, 0,
        Set_shipShape, nullptr, Get_shipShape,
        XP_OPTFLAG_DEFAULT,
        "Define the ship shape to use.  Because the argument to this option\n"
        "is rather large (up to 500 bytes) the recommended way to set\n"
        "this option is in the .xpilotrc file in your home directory.\n"
        "The exact format is defined in the file doc/README.SHIPS in the\n"
        "XPilot distribution. Note that there is a nifty Unix tool called\n"
        "editss for easy ship creation. There is XPShipEditor for Windows\n"
        "and Ship Shaper for Java.  See the XPilot FAQ for details.\n"
        "See also the \"shipShapeFile\" option below.\n"),

    XP_CONST_CHAR_STAR_OPTION(
        "shipShapeFile",
        CONF_SHIP_FILE,
        nullptr, 0,
        Set_shipShapeFile, nullptr, Get_shipShapeFile,
        XP_OPTFLAG_DEFAULT,
        "An optional file where shipshapes can be stored.\n"
        "If this resource is defined and it refers to an existing file\n"
        "then shipshapes can be referenced to by their name.\n"
        "For instance if you define shipShapeFile to be\n"
        "/home/myself/.shipshapes and this file contains one or more\n"
        "shipshapes then you can select the shipshape by starting xpilot as:\n"
        "    xpilot -shipShape myshipshapename\n"
        "Where \"myshipshapename\" should be the \"name:\" or \"NM:\" of\n"
        "one of the shipshapes defined in /home/myself/.shipshapes.\n"
        "Each shipshape definition should be defined on only one line,\n"
        "where all characters up to the first left parenthesis don't matter.\n"
        /* shipshopshapshepshit getting nuts from all these shpshp-s. */),

    XP_CONST_CHAR_STAR_OPTION(
        "texturePath",
        CONF_TEXTUREDIR,
        nullptr, 0,
        Set_texturePath, nullptr, Get_texturePath,
        XP_OPTFLAG_KEEP,
        "Search path for texture files.\n"
        "This is a list of one or more directories separated by colons.\n"),

/* kps - these should not be needed in the SDL windows client. */
#if 0
    XP_INT_OPTION(
    "radarDivisor",
    1,
    1,
    100,
    &RadarDivisor,
    nullptr,
    XP_OPTFLAG_CONFIG_DEFAULT,
    "Specifies how many frames between radar window updates.\n"),
#endif

    XP_CONST_CHAR_STAR_OPTION(
        "clientRankFile",
        "",
        clientRankFile,
        sizeof clientRankFile,
        nullptr, nullptr, nullptr,
        XP_OPTFLAG_DEFAULT,
        "An optional file where clientside kill/death rank is stored.\n"),

    XP_CONST_CHAR_STAR_OPTION(
        "clientRankHTMLFile",
        "",
        clientRankHTMLFile,
        sizeof clientRankHTMLFile,
        nullptr, nullptr, nullptr,
        XP_OPTFLAG_DEFAULT,
        "An optional file where clientside kill/death rank is\n"
        "published in HTML format.\n"),

    XP_CONST_CHAR_STAR_OPTION(
        "clientRankHTMLNOJSFile",
        "",
        clientRankHTMLNOJSFile,
        sizeof clientRankHTMLNOJSFile,
        nullptr, nullptr, nullptr,
        XP_OPTFLAG_DEFAULT,
        "An optional file where clientside kill/death rank is\n"
        "published in HTML format, w/o JavaScript.\n"),

    XP_CONST_CHAR_STAR_OPTION(
        "soundFile",
        CONF_SOUNDFILE,
        soundFile, sizeof soundFile,
        nullptr, nullptr, nullptr,
        XP_OPTFLAG_DEFAULT,
        "Specifies the sound file.\n"),

    XP_INT_OPTION(
        "maxVolume",
        100,
        0,
        100,
        &maxVolume,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Specifies the volume to play sounds with (0-100%%).\n"),

    XP_BOOL_OPTION(
        "sound",
        true,
        &clientOptions.sound,
        nullptr,
        XP_OPTFLAG_CONFIG_DEFAULT,
        "Is sound enabled? (set to false to mute client).\n"),

};

void Store_default_options(void)
{
    STORE_OPTIONS(default_options);
}
