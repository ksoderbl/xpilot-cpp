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
    bool showMyShipShape = false;
    bool showShipShapes = true;
    bool showShipShapesHack = false;
    bool slidingRadar = true;
    bool texturedDecor = false;
    bool texturedWalls = false;
};

struct ClientOptions
{
    // Instruments on screen
    InstrumentsOptions instruments;

    // TODO: make some substruct for these?
    bool dirPrediction = false;
    bool toggleShield = false;  // Are shields toggled by a press?
    bool autoShield = true;     // shield drops for fire
    bool markingLights = false; // Marking lights on ships
    bool sound = false;
};

extern ClientOptions clientOptions;
