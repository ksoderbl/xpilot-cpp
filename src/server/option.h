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

#pragma once

#include <string>
#include <vector>

#include "types.h"

typedef struct options
{
    std::vector<std::string> expandList; /* List of predefined settings. */
    double gravity;
    double shipMass;
    double ballMass;
    double shotMass;
    double shotSpeed;

    // Default number of ticks each shot will live
    int shotLife;

    // How many robots should enter the game?
    int maxRobots;
    int minRobots;
    char *robotFile;
    int robotsTalk;

    // Do robots leave at all?
    int robotsLeave;

    // Max life per robot (0=off)
    int robotLeaveLife;

    // Min score for robot to live (0=off)
    int robotLeaveScore;

    // Min ratio for robot to live (0=off)
    int robotLeaveRatio;

    int robotTeam;

    // Restrict robots to robotTeam?
    bool restrictRobots;

    // Allow only robots in robotTeam?
    bool reserveRobotTeam;

    // Max shots pr. player
    int maxPlayerShots;

    // Shots affected by gravity
    bool shotsGravity;

    // Frames per autorepeat fire (0=off)
    int fireRepeatRate;

    bool treasureCollisionKills;

    bool Log;

    // Let robots live even if there  are no players logged in
    bool RawMode;

    // Don't quit even if there are no human players playing
    bool NoQuit;

    // log robots coming and going
    bool logRobots;

    // Name of mapfile
    char *mapFileName;

    // Raw map data
    char *mapData;

    // Width of the universe
    int mapWidth;

    // Height of the universe
    int mapHeight;

    // Name of the universe
    char *mapName;

    // Name of the creator
    char *mapAuthor;

    // Contact port number
    int contactPort;

    // Host name (for multihomed hosts)
    char *serverHost;

    // Server greeting message to players
    char *greeting;

    // Can players overrun other players?
    bool allowPlayerCrashes;

    // Can players bounce other players?
    bool allowPlayerBounces;

    // Can players kill each other?
    bool allowPlayerKilling;

    // Can players use shields?
    bool allowShields;

    // Players start with shields up?
    bool playerStartsShielded;

    // Do shots bounce off walls?
    bool shotsWallBounce;

    // Do balls bounce off walls?
    bool ballsWallBounce;

    // Do balls participate in colls.?
    bool ballCollisions;

    // Do sparks push balls around?
    bool ballSparkCollisions;

    // Do mines bounce off walls?
    bool minesWallBounce;

    // Do items bounce off walls?
    bool itemsWallBounce;

    // Do missiles bounce off walls?
    bool missilesWallBounce;

    // Do sparks bounce off walls?
    bool sparksWallBounce;

    // Do sparks bounce off walls?
    bool debrisWallBounce;

    // Do asteroids bounce off walls?
    bool asteroidsWallBounce;

    // Do laser pulses bounce off walls?
    bool pulsesWallBounce;

    // Generate exhaust when cloaked?
    bool cloakedExhaust;

    // Allowed to use shields when cloaked?
    bool cloakedShield;

    // Do ecms reprogram mines?
    bool ecmsReprogramMines;

    // Do ecms reprogram robots?
    bool ecmsReprogramRobots;

    // max object bounce speed
    double maxObjectWallBounceSpeed;

    double maxSparkWallBounceSpeed;

    // max shielded bounce speed
    double maxShieldedWallBounceSpeed;

    // max unshielded bounce speed
    double maxUnshieldedWallBounceSpeed;

    // max angle for landing
    double maxShieldedWallBounceAngle;

    // max angle for landing
    double maxUnshieldedWallBounceAngle;

    // wall lowers speed if less than 1
    double playerWallBrakeFactor;

    int playerWallBounceType;
    double playerWallBounceBrakeFactor;
    double playerBallBounceBrakeFactor;
    double playerWallFriction;

    // wall lowers speed if less than 1
    double objectWallBounceBrakeFactor;

    // reduce object life
    double objectWallBounceLifeFactor;

    // Wall bouncing fuel drain factor
    double wallBounceFuelDrainMult;

    // Wall bouncing item destroy prob
    double wallBounceDestroyItemProb;

    // Is visibility limited?
    bool limitedVisibility;

    // Minimum visibility when starting
    double minVisibilityDistance;

    // Maximum visibility
    double maxVisibilityDistance;

    // Are lives limited?
    bool limitedLives;

    // If so, what's the max?
    int worldLives;

    // Reset the world when round ends?
    bool endOfRoundReset;

    // Last human to reset round for
    int resetOnHuman;

    // Are alliances allowed?
    bool allowAlliances;

    // Are changes in alliances broadcast?
    bool announceAlliances;

    // Are teams allowed?
    bool teamPlay;

    // Do fuelstations belong to teams?
    bool teamFuel;

    // Do cannons belong to teams?
    bool teamCannons;

    // Accuracy of cannonfire
    int cannonSmartness;

    // Do cannons use items?
    bool cannonsUseItems;

    // Do cannons defend themselves?
    bool cannonsDefend;

    bool cannonsPickupItems;

    // Do cannons fire flak?
    bool cannonFlak;

    // How long do cannons stay dead?
    int cannonDeadTime;

    double cannonDeadTicks;
    double minCannonShotLife;
    double maxCannonShotLife;
    double survivalScore;

    double cannonShotSpeed;

    // Keep shots when player leaves?
    bool keepShots;

    bool tagGame;

    // Is this a race?
    bool timing;

    // Do we race with balls?
    bool ballrace;

    // Need to be connected to ball to pass checkpoints?
    bool ballrace_connect;

    // Do objects wrap when they cross the edge of the Universe?
    bool edgeWrap;

    // Do objects bounce when they hit the edge of the Universe?
    bool edgeBounce;

    // Give map an extra border?
    bool extraBorder;

    // Where does gravity originate?
    ipos_t gravityPoint;

    // If gravity is along a uniform line, at what angle is that line?
    double gravityAngle;

    // Is gravity a point source?
    bool gravityPointSource;

    // If so, is it clockwise?
    bool gravityClockwise;

    // If not clockwise, anticlockwise?
    bool gravityAnticlockwise;

    // Is gravity visible?
    bool gravityVisible;

    // Are wormholes visible?
    bool wormholeVisible;

    // Are itemconcentrators visible?
    bool itemConcentratorVisible;

    // Are asteroid concentrators visible?
    bool asteroidConcentratorVisible;
    int wormTime;

    // Name of defaults file
    char *defaultsFileName;

    // Name of password file
    char *passwordFileName;

    // Name of motd file
    char *motdFileName;

    // Name of score table file
    char *scoreTableFileName;

    // Name of admin message file
    char *adminMessageFileName;

    // Limit on admin message file size
    int adminMessageFileSizeLimit;

    // minimum smarts for a nuke
    int nukeMinSmarts;

    // minimum number of mines for nuke
    int nukeMinMines;

    // multiplier for damage from nuke cluster debris, reduces number of particles by similar amount
    double nukeClusterDamage;

    // Length of time mine is fused
    int mineFuseTime;

    // lifetime of mines
    int mineLife;

    // minimum speed of mines
    double minMineSpeed;

    // lifetime of missiles
    int missileLife;

    // Distance from base mines may be used
    int baseMineRange;

    // When does a shot trigger a mine?
    int mineShotDetonateDistance;

    double shotKillScoreMult;
    double torpedoKillScoreMult;
    double smartKillScoreMult;
    double heatKillScoreMult;
    double clusterKillScoreMult;
    double laserKillScoreMult;
    double tankKillScoreMult;
    double runoverKillScoreMult;
    double ballKillScoreMult;
    double explosionKillScoreMult;
    double shoveKillScoreMult;
    double crashScoreMult;
    double mineScoreMult;
    double selfKillScoreMult;
    double unownedKillScoreMult;
    double asteroidPoints;
    double cannonPoints;
    double asteroidMaxScore;
    double cannonMaxScore;

    // Probability for moving items
    double movingItemProb;

    // Probability for random-appearing items
    double randomItemProb;

    // Probability for players items to drop when player is killed
    double dropItemOnKillProb;

    // Probaility for remaining items to detonate when player is killed
    double detonateItemOnKillProb;

    double destroyItemInCollisionProb;

    // prob. that a broken asteroid will
    double asteroidItemProb;

    // have one or more items
    int asteroidMaxItems;

    // prob. that unclaimed rocketpack
    double rogueHeatProb;

    // or minepack will "activate"
    double rogueMineProb;
    double itemProbMult;
    double cannonItemProbMult;
    double maxItemDensity;
    double maxAsteroidDensity;
    int itemConcentratorRadius;
    double itemConcentratorProb;
    int asteroidConcentratorRadius;
    double asteroidConcentratorProb;

    int framesPerSecond;

    double tagItKillScoreMult;
    double tagKillItScoreMult;
    bool zeroSumScoring;

    bool allowSmartMissiles;
    bool allowHeatSeekers;
    bool allowTorpedoes;
    bool allowNukes;
    bool allowClusters;
    bool allowModifiers;
    bool allowLaserModifiers;
    bool allowShipShapes;

    // Are players visible on radar?
    bool playersOnRadar;

    // Are missiles visible on radar?
    bool missilesOnRadar;

    // Are mines visible on radar?
    bool minesOnRadar;

    // Are nuke weapons radar visible?
    bool nukesOnRadar;

    // Are treasure balls radar visible?
    bool treasuresOnRadar;

    // Are asteroids radar visible?
    bool asteroidsOnRadar;

    // Smarts, heats & torps look diff.?
    bool distinguishMissiles;

    // Number of missiles per item.
    int maxMissilesPerPack;

    // Number of mines per item.
    int maxMinesPerPack;

    // Mines have names displayed?
    bool identifyMines;

    // Pickup items with shields up?
    bool shieldedItemPickup;

    // Detach mines with shields up?
    bool shieldedMining;

    // Is the laser a stun gun?
    bool laserIsStunGun;

    // Send status to meta-server?
    bool reportToMetaServer;

    // Do a DNS lookup for XPilot.domain?
    bool searchDomainForXPilot;

    // Computers which are denied service
    char *denyHosts;

    // total duration of game in minutes
    double gameDuration;

    // Are players allowed to watch others?
    bool allowViewing;

    // Assign player to team if not set?
    bool teamAssign;

    // Is team immune from player action
    bool teamImmunity;

    // if your target explodes, you die?
    bool targetKillTeam;

    // Does team collide with target?
    bool targetTeamCollision;

    // all targets reappear together
    bool targetSync;

    // How long do targets stay dead?
    int targetDeadTime;

    // die if treasure is destroyed?
    bool treasureKillTeam;

    // must treasure be safe to cash balls?
    bool captureTheFlag;
    int specialBallTeam;
    bool treasureCollisionDestroys;
    bool treasureCollisionMayKill;
    bool wreckageCollisionMayKill;
    bool asteroidCollisionMayKill;

    double ballConnectorSpringConstant;
    double ballConnectorDamping;
    double maxBallConnectorRatio;
    double ballConnectorLength;
    bool connectorIsString; /* can the connector get shorter? */

    char *rankFileName;
    char *rankWebpageFileName;
    char *rankWebpageCSS;

    // friction only affects ships
    double frictionSetting;

    // friction in friction blocks
    double blockFriction;

    // if yes, friction blocks are decor
    // if no, friction blocks are space
    bool blockFrictionVisible;

    // angle velocity turns each frame
    int coriolis;

    // in blocks
    double checkpointRadius;

    // how many laps per race
    int raceLaps;

    // lock ply from other teams when dead?
    bool lockOtherTeam;

    // destroy item on loseItem?
    bool loseItemDestroys;

    // destroyed ships leave wreckage?
    bool useWreckage;

    // how many offensive and defensive items can player carry
    int maxOffensiveItems;
    int maxDefensiveItems;

    // max. duration of each round
    int maxRoundTime;

    // # of rounds to play.
    int roundsToPlay;

    // how many objects a player can see
    int maxVisibleObject;

    // Is server swappable out of memory?
    bool pLockServer;

    // ignore client maxFPS request if 20
    bool ignore20MaxFPS;

    // OS timer resolution (times/sec)
    int timerResolution;

    // password for operator status
    char *password;

    // First UDP port for clients
    int clientPortStart;

    // Last one (these are for firewalls)
    int clientPortEnd;

    // Real name for robot
    char *robotUserName;

    // Host name for robot
    char *robotHostName;

    // Real name for tank
    char *tankUserName;

    // Host name for tank
    char *tankHostName;

    // Amount by which the tank's score is decreased from the player's
    int tankScoreDecrement;

    // Are players immune to their own weapons?
    bool selfImmunity;

    // What ship shape is used for players who do not define their own?
    char *defaultShipShape;

    // What ship shape is used for tanks?
    char *tankShipShape;

    // Max. time you can stay paused for
    int maxPauseTime;
    int maxClientsPerIP; /* Max. number of clients that can login from the same IP */

    int recordMode;
    int recordFlushInterval;

    char *dataURL;
    char *recordFileName;

    double ballRadius;
    bool multipleConnectors;

    double turnPushPersistence;
    double turnGrip;

    double constantSpeed;

    bool polygonMode;

    bool teamcup;
    char *teamcupName;
    char *teamcupMailAddress;
    char *teamcupScoreFileNamePrefix;
    int teamcupMatchNumber;

    double mainLoopTime;
    int cellGetObjectsThreshold;
} options_t;

/*
 * Prototypes for option.c
 */
void Options_parse(void);
void Options_free(void);
bool Convert_string_to_int(const char *value_str, int *int_ptr);
bool Convert_string_to_float(const char *value_str, double *float_ptr);
bool Convert_string_to_bool(const char *value_str, bool *bool_ptr);
void Convert_list_to_string(const std::vector<std::string> &list, char **str);
void Convert_string_to_list(const char *value, std::vector<std::string> *list_ptr);

extern options_t options;
