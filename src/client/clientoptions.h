/*
 * XPilot, a multiplayer gravity war game.  Copyright (C) 1991-2001 by
 *
 *      Bjørn Stabell
 *      Ken Ronny Schouten
 *      Bert Gijsbers
 *      Dick Balaska
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
    int maxMessages = 8;                // Max. number of messages to display, legacy value was 8
    int messagesToStdout = 1;           // Send messages to standard output
    int maxLinesInHistory = 32;         // Number of lines to save in history
    int showScoreDecimals = 0;          // Number of decimals to show in scores
    int maxVolume = 100;                // maximum volume (in percent)
    int meterWidth = 60;                // Width of drawn meters
    int meterHeight = 10;               // Height of drawn meters

    // From X11 client
    bool ignoreWindowManager = false;
    bool fullColor = false;            // Whether to try using colors as close to
                                       // the specified ones as possible, or just
                                       // use a few standard colors for everything.
    bool texturedObjects = false;      // Whether to draw bitmaps for some objects.
                                       // Previously this variable determined
                                       // fullColor too.
    int maxColors = 16;                // Max. number of colors to use
    int buttonColor = BLUE;            // Color index for button drawing
    int windowColor = 8;               // Color index for window drawing
    int borderColor = WHITE;           // Color index for border drawing
    int wallColor = BLUE;              // Color index for wall drawing
    int decorColor = 6;                // Color index for decoration drawing
    int baseNameColor = BLUE;          // Color index for base name drawing
    int backgroundPointColor = BLUE;   // background point drawing
    int fuelColor = BLUE;              // fuel station drawing, legacy value was RED
    int visibilityBorderColor = BLACK; // visibility border drawing
    int ballColor = WHITE;             // Color index for ball drawing
    int connColor = 4;                 // Color index for connector drawing
    int teamShotColor = BLUE;          // Color index for harmless shot drawing
    int zeroLivesColor = RED;          // Color to associate with 0 lives
    int oneLifeColor = 11;             // Color to associate with 1 life
    int twoLivesColor = 0;             // Color to associate with 2 lives
    int selfLWColor = RED;             // Color index for selfLifeWarning
    int enemyLWColor = RED;            // Color index for enemyLifeWarning
    int teamLWColor = 4;               // Color index for teamLifeWarning
    int shipNameColor = BLUE;          // Color index for ship name drawing
    int mineNameColor = BLUE;          // Color index for mine name drawing
    int shipColor = WHITE;             // Color index for own ships, TODO
    int teamShipColor = BLUE;          // Color index for ships of teammates
    int teamColor0 = 0;                // Color index to associate with team 0
    int teamColor1 = 0;                // Color index to associate with team 1
    int teamColor2 = 0;                // Color index to associate with team 2
    int teamColor3 = 0;                // Color index to associate with team 3
    int teamColor4 = 0;                // Color index to associate with team 4
    int teamColor5 = 0;                // Color index to associate with team 5
    int teamColor6 = 0;                // Color index to associate with team 6
    int teamColor7 = 0;                // Color index to associate with team 7
    int teamColor8 = 0;                // Color index to associate with team 8
    int teamColor9 = 0;                // Color index to associate with team 9
    int hudColor = BLUE;               // Color index for HUD drawing
    int hudHLineColor = BLACK;         // Color index for horiz. HUD line drawing
    int hudVLineColor = BLACK;         // Color index for vert. HUD line drawing
    int hudItemsColor = BLUE;          // Color index for HUD items drawing
    int hudRadarEnemyColor = RED;      // Color index for enemy hudradar dots
    int hudRadarOtherColor = BLUE;     // Color index for other hudradar dots
    int hudLockColor = 0;              // Color index for lock on HUD drawing
    int fuelGaugeColor = 0;            // Color index for fuel gauge drawing
    int dirPtrColor = 0;               // Color index for dirptr drawing
    int messagesColor = 12;            // Color index for messages
    int oldMessagesColor = 13;         // Color index for old messages
    int msgScanBallColor = RED;        // Color index for ball msg
    int msgScanSafeColor = 4;          // Color index for safe msg
    int msgScanCoverColor = BLUE;      // Color index for cover msg
    int msgScanPopColor = 11;          // Color index for pop msg
    int fuelMeterColor = 3;            // Color index for fuel meter
    int powerMeterColor = 0;           // Color index for power meter
    int turnSpeedMeterColor = 0;       // Color index for turnspeed meter
    int packetSizeMeterColor = RED;    // Color index for packet size meter
    int packetLossMeterColor = RED;    // Color index for packet loss meter
    int packetDropMeterColor = RED;    // Color index for packet drop meter
    int packetLagMeterColor = RED;     // Color index for packet lag meter
    int temporaryMeterColor = RED;     // Color index for temporary meter drawing
    int meterBorderColor = BLUE;       // Color index for meter border drawing
    int scoreObjectColor = 11;         // Color index for map score objects
    int wallRadarColor = BLUE;         // Color index for walls on radar
    int targetRadarColor = 4;          // Color index for targets on radar
    int decorRadarColor = 6;           // Color index for decorations on radar
    int spaceColor = 8;                // Space (background) color index
    int clockColor = WHITE;            // Clock color index
    int scoreColor = WHITE;            // Score list color indices
    int scoreSelfColor = RED;          // Score list own score color index
    int scoreInactiveColor = 12;       // Score list inactive player color index
    int scoreInactiveSelfColor = 12;   // Score list inactive self color index
    int scoreOwnTeamColor = 4;         // Score list own team color index
    int scoreEnemyTeamColor = 11;      // Score list enemy team color index

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
