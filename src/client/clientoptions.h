/*
 * XPilot, a multiplayer gravity war game.
 *
 * Copyright (C) 2026 Kristian Söderblom
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

#pragma once

#include <string>

#include "const.h"
#include "socklib.h"

#include "clientpack.h"
#include "netclient.h"

struct ConnectParam
{
    int contact_port = SERVER_PORT;
    int server_port = 0;
    int login_port = 0;
    char nick_name[MAX_NAME_LEN] = "";
    char user_name[MAX_NAME_LEN] = "";
    char host_name[SOCK_HOSTNAME_LENGTH] = "";
    char server_addr[MAX_HOST_LEN] = "";
    char server_name[MAX_HOST_LEN] = "";
    char disp_name[MAX_DISP_LEN] = "";
    unsigned server_version = 0;
    int team = TEAM_NOT_SET;
};

struct InstrumentsOptions
{
    bool clientRanker = false;
    bool clockAMPM = false;
    bool filledDecor = false;
    bool filledWorld = false;
    bool outlineDecor = false;
    bool outlineWorld = false;
    bool showDecor = false;
    bool showItems = true;
    bool showLivesByShip = true;
    bool showMessages = true;
    bool showMyShipShape = true;
    bool showShipShapes = true;
    bool showShipShapesHack = false;
    bool slidingRadar = true;
    bool texturedDecor = false;
    bool texturedWalls = false;
};

struct ClientOptions
{
    // Connection params
    ConnectParam connectParam;

    // Instruments on screen
    InstrumentsOptions instruments;

    // TODO: make some substruct for these?
    bool dirPrediction = false;
    bool toggleShield = false;  // Are shields toggled by a press?
    bool autoShield = true;     // shield drops for fire
    bool markingLights = false; // Marking lights on ships
    bool sound = false;

    int clientPortStart = 0;            // First UDP port for clients
    int clientPortEnd = 0;              // Last one (these are for firewalls)
    int maxFPS = MAX_SUPPORTED_FPS;     // Max FPS player wants from server
    int maxMouseTurnsPS = 0;            // Write something intelligent here
    int sparkSize = 1;                  // Size of debris and sparks, legacy value was 2
    int hudRadarDotSize = 8;            // Size for hudradar dot drawing
    int baseWarningType = 1;            // Which type of base warning you prefer
    int maxCharsInNames = MAX_NAME_LEN; // Draw max this many chars for names
    int shotSize = 5;                   // size of shot, legacy value was 3
    int teamShotSize = 3;               // size of team shot, legacy default was 2
    int backgroundPointDist = 20;       // spacing of navigation points, legacy default was 8
    int backgroundPointSize = 2;        // size of navigation points
    int charsPerSecond = 100;           // Message output speed (configurable), legacy value was 50
    int maxMessages = 16;               // Max. number of messages to display, legacy value was 8
    int messagesToStdout = 1;           // Send messages to standard output
    int maxLinesInHistory = 32;         // Number of lines to save in history
    int showScoreDecimals = 0;          // Number of decimals to show in scores
    int maxVolume = 100;                // maximum volume (in percent)
    int meterWidth = 60;                // Width of drawn meters
    int meterHeight = 10;               // Height of drawn meters

    // From X11 client
    bool ignoreWindowManager = false;
    bool fullColor = false;       // Whether to try using colors as close to
                                  // the specified ones as possible, or just
                                  // use a few standard colors for everything.
    bool texturedObjects = false; // Whether to draw bitmaps for some objects.
                                  // Previously this variable determined
                                  // fullColor too.
    int maxColors = 16;           // Max. number of colors to use
    int buttonColor = BLUE;       // Color index for button drawing
    int windowColor = 8;          // Color index for window drawing
    int borderColor = WHITE;      // Color index for border drawing
    int wallColor = BLUE;         // Color index for wall drawing
    int decorColor = 6;           // Color index for decoration drawing

    // From SDL/OpenGL client
    bool smoothLines = true;     // Use antialized smooth lines
    bool texturedBalls = false;  // Draw balls with textures
    bool texturedShips = false;  // Draw ships with textures
                                 // Turned this off because the images drawn
                                 // don't match the actual shipshape used
                                 // for wall collisions by the server.
    int hudRadarEnemyShape = 2;  // The shape of enemy ships on hud radar
    int hudRadarOtherShape = 2;  // The shape of friendly ships on hud radar
    int hudRadarObjectShape = 0; // The shape of small objects on hud radar
    int gameFontSize = 16;
    int mapFontSize = 16;
};

extern ClientOptions clientOptions;
