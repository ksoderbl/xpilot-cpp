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

#ifndef OPTION_H
#define OPTION_H

#include "list.h"
#include "types.h"

typedef struct options
{
    list_t expandList;     /* List of predefined settings. */
    double Gravity;        /* Power of gravity */
    double ShipMass;       /* Default mass of ship */
    double ballMass;       /* Default mass of balls */
    double ShotsMass;      /* Default mass of shots */
    double ShotsSpeed;     /* Default speed of shots */
    int ShotsLife;         /* Default number of ticks */
                           /* each shot will live */
    int maxRobots;         /* How many robots should enter */
    int minRobots;         /* the game? */
    char *robotFile;       /* Filename for robot parameters */
    int robotsTalk;        /* Do robots talk? */
    int robotsLeave;       /* Do robots leave at all? */
    int robotLeaveLife;    /* Max life per robot (0=off)*/
    int robotLeaveScore;   /* Min score for robot to live (0=off)*/
    int robotLeaveRatio;   /* Min ratio for robot to live (0=off)*/
    int robotTeam;         /* Team for robots */
    bool restrictRobots;   /* Restrict robots to robotTeam? */
    bool reserveRobotTeam; /* Allow only robots in robotTeam? */
    int ShotsMax;          /* Max shots pr. player */
    bool ShotsGravity;     /* Shots affected by gravity */
    int fireRepeatRate;    /* Frames per autorepeat fire (0=off) */

    bool RawMode;      /* Let robots live even if there */
                       /* are no players logged in */
    bool NoQuit;       /* Don't quit even if there are */
                       /* no human players playing */
    bool logRobots;    /* log robots coming and going */
    char *mapFileName; /* Name of mapfile... */
    char *mapData;     /* Raw map data... */
    int mapWidth;      /* Width of the universe */
    int mapHeight;     /* Height of the universe */
    char *mapName;     /* Name of the universe */
    char *mapAuthor;   /* Name of the creator */
    int contactPort;   /* Contact port number */
    char *serverHost;  /* Host name (for multihomed hosts) */

    bool crashWithPlayer;                /* Can players overrun other players? */
    bool bounceWithPlayer;               /* Can players bounce other players? */
    bool playerKillings;                 /* Can players kill each other? */
    bool playerShielding;                /* Can players use shields? */
    bool playerStartsShielded;           /* Players start with shields up? */
    bool shotsWallBounce;                /* Do shots bounce off walls? */
    bool minesWallBounce;                /* Do mines bounce off walls? */
    bool itemsWallBounce;                /* Do items bounce off walls? */
    bool missilesWallBounce;             /* Do missiles bounce off walls? */
    bool sparksWallBounce;               /* Do sparks bounce off walls? */
    bool debrisWallBounce;               /* Do sparks bounce off walls? */
    bool ballsWallBounce;                /* Do balls bounce off walls? */
    bool ballCollisions;                 /* Do balls participate in colls.? */
    bool ballSparkCollisions;            /* Do sparks push balls around? */
    bool asteroidsWallBounce;            /* Do asteroids bounce off walls? */
    bool cloakedExhaust;                 /* Generate exhaust when cloaked? */
    bool cloakedShield;                  /* Allowed to use shields when cloaked? */
    bool ecmsReprogramMines;             /* Do ecms reprogram mines? */
    bool ecmsReprogramRobots;            /* Do ecms reprogram robots? */
    double maxObjectWallBounceSpeed;     /* max object bounce speed */
    double maxShieldedWallBounceSpeed;   /* max shielded bounce speed */
    double maxUnshieldedWallBounceSpeed; /* max unshielded bounce speed */
    double maxShieldedWallBounceAngle;   /* max angle for landing */
    double maxUnshieldedWallBounceAngle; /* max angle for landing */
    double playerWallBrakeFactor;        /* wall lowers speed if less than 1 */
    double objectWallBrakeFactor;        /* wall lowers speed if less than 1 */
    double objectWallBounceLifeFactor;   /* reduce object life */
    double wallBounceFuelDrainMult;      /* Wall bouncing fuel drain factor */
    double wallBounceDestroyItemProb;    /* Wall bouncing item destroy prob */
    bool limitedVisibility;              /* Is visibility limited? */
    double minVisibilityDistance;        /* Minimum visibility when starting */
    double maxVisibilityDistance;        /* Maximum visibility */
    bool limitedLives;                   /* Are lives limited? */
    int worldLives;                      /* If so, what's the max? */
    bool endOfRoundReset;                /* Reset the world when round ends? */
    int resetOnHuman;                    /* Last human to reset round for */
    bool allowAlliances;                 /* Are alliances allowed? */
    bool announceAlliances;              /* Are changes in alliances broadcast? */
    bool teamPlay;                       /* Are teams allowed? */
    bool teamFuel;                       /* Do fuelstations belong to teams? */
    bool teamCannons;                    /* Do cannons belong to teams? */
    int cannonSmartness;                 /* Accuracy of cannonfire */
    bool cannonsUseItems;                /* Do cannons use items? */
    bool cannonsDefend;                  /* Do cannons defend themselves? */
    bool cannonFlak;                     /* Do cannons fire flak? */
    int cannonDeadTime;                  /* How long do cannons stay dead? */
    bool keepShots;                      /* Keep shots when player leaves? */
    bool timing;                         /* Is this a race? */
    bool ballrace;                       /* Do we race with balls? */
    bool ballrace_connect;               /* Need to be connected to ball to pass checkpoints? */
    bool edgeWrap;                       /* Do objects wrap when they cross the edge of the Universe? */
    bool edgeBounce;                     /* Do objects bounce when they hit the edge of the Universe? */
    bool extraBorder;                    /* Give map an extra border? */
    ipos_t gravityPoint;                 /* Where does gravity originate? */
    double gravityAngle;                 /* If gravity is along a uniform line, at what angle is that line? */
    bool gravityPointSource;             /* Is gravity a point source? */
    bool gravityClockwise;               /* If so, is it clockwise? */
    bool gravityAnticlockwise;           /* If not clockwise, anticlockwise? */
    bool gravityVisible;                 /* Is gravity visible? */
    bool wormholeVisible;                /* Are wormholes visible? */
    bool itemConcentratorVisible;        /* Are itemconcentrators visible? */
    bool asteroidConcentratorVisible;    /* Are asteroid concentrators visible? */
    int wormTime;
    char *defaultsFileName;        /* Name of defaults file... */
    char *passwordFileName;        /* Name of password file... */
    char *motdFileName;            /* Name of motd file */
    char *scoreTableFileName;      /* Name of score table file */
    char *adminMessageFileName;    /* Name of admin message file */
    int adminMessageFileSizeLimit; /* Limit on admin message file size */

    int nukeMinSmarts;            /* minimum smarts for a nuke */
    int nukeMinMines;             /* minimum number of mines for nuke */
    double nukeClusterDamage;     /* multiplier for damage from nuke */
                                  /* cluster debris, reduces number */
                                  /* of particles by similar amount */
    int mineFuseTime;             /* Length of time mine is fused */
    int mineLife;                 /* lifetime of mines */
    double minMineSpeed;          /* minimum speed of mines */
    int missileLife;              /* lifetime of missiles */
    int baseMineRange;            /* Distance from base mines may be used */
    int mineShotDetonateDistance; /* When does a shot trigger a mine? */

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

    double movingItemProb;         /* Probability for moving items */
    double randomItemProb;         /* Probability for random-appearing items */
    double dropItemOnKillProb;     /* Probability for players items to */
                                   /* drop when player is killed */
    double detonateItemOnKillProb; /* Probaility for remaining items to */
                                   /* detonate when player is killed */
    double destroyItemInCollisionProb;
    double asteroidItemProb; /* prob. that a broken asteroid will */
    int asteroidMaxItems;    /* have one or more items */
    double rogueHeatProb;    /* prob. that unclaimed rocketpack */
    double rogueMineProb;    /* or minepack will "activate" */
    double itemProbMult;
    double cannonItemProbMult;
    double maxItemDensity;
    double maxAsteroidDensity;
    int itemConcentratorRadius;
    double itemConcentratorProb;
    int asteroidConcentratorRadius;
    double asteroidConcentratorProb;

    int framesPerSecond;

    bool allowSmartMissiles;
    bool allowHeatSeekers;
    bool allowTorpedoes;
    bool allowNukes;
    bool allowClusters;
    bool allowModifiers;
    bool allowLaserModifiers;
    bool allowShipShapes;

    bool playersOnRadar;        /* Are players visible on radar? */
    bool missilesOnRadar;       /* Are missiles visible on radar? */
    bool minesOnRadar;          /* Are mines visible on radar? */
    bool nukesOnRadar;          /* Are nuke weapons radar visible? */
    bool treasuresOnRadar;      /* Are treasure balls radar visible? */
    bool asteroidsOnRadar;      /* Are asteroids radar visible? */
    bool distinguishMissiles;   /* Smarts, heats & torps look diff.? */
    int maxMissilesPerPack;     /* Number of missiles per item. */
    int maxMinesPerPack;        /* Number of mines per item. */
    bool identifyMines;         /* Mines have names displayed? */
    bool shieldedItemPickup;    /* Pickup items with shields up? */
    bool shieldedMining;        /* Detach mines with shields up? */
    bool laserIsStunGun;        /* Is the laser a stun gun? */
    bool reportToMetaServer;    /* Send status to meta-server? */
    bool searchDomainForXPilot; /* Do a DNS lookup for XPilot.domain? */
    char *denyHosts;            /* Computers which are denied service */
    double gameDuration;        /* total duration of game in minutes */
    bool allowViewing;          /* Are players allowed to watch others? */

    bool teamAssign;   /* Assign player to team if not set? */
    bool teamImmunity; /* Is team immune from player action */

    bool targetKillTeam;      /* if your target explodes, you die? */
    bool targetTeamCollision; /* Does team collide with target? */
    bool targetSync;          /* all targets reappear together */
    int targetDeadTime;       /* How long do targgets stay dead? */
    bool treasureKillTeam;    /* die if treasure is destroyed? */
    bool captureTheFlag;      /* must treasure be safe to cash balls? */
    bool treasureCollisionDestroys;
    bool treasureCollisionMayKill;
    bool wreckageCollisionMayKill;
    bool asteroidCollisionMayKill;

    double ballConnectorSpringConstant;
    double ballConnectorDamping;
    double maxBallConnectorRatio;
    double ballConnectorLength;
    bool connectorIsString; /* can the connector get shorter? */

    double friction;           /* friction only affects ships */
    double blockFriction;      /* friction in friction blocks */
    bool blockFrictionVisible; /* if yes, friction blocks are decor; */
                               /* if no, friction blocks are space */
    int coriolis;              /* angle velocity turns each frame */
    double checkpointRadius;   /* in blocks */
    int raceLaps;              /* how many laps per race */
    bool lockOtherTeam;        /* lock ply from other teams when dead? */
    bool loseItemDestroys;     /* destroy item on loseItem? */
    bool useWreckage;          /* destroyed ships leave wreckage? */

    int maxOffensiveItems; /* how many offensive and defensive */
    int maxDefensiveItems; /* items can player carry */

    int maxRoundTime; /* max. duration of each round */
    int roundsToPlay; /* # of rounds to play. */

    int maxVisibleObject; /* how many objects a player can see */
    bool pLockServer;     /* Is server swappable out of memory?  */
    bool ignore20MaxFPS;  /* ignore client maxFPS request if 20 */
    int timerResolution;  /* OS timer resolution (times/sec) */
    char *password;       /* password for operator status */
    int clientPortStart;  /* First UDP port for clients */
    int clientPortEnd;    /* Last one (these are for firewalls) */

    char *robotRealName; /* Real name for robot */
    char *robotHostName; /* Host name for robot */

    char *tankRealName;     /* Real name for tank */
    char *tankHostName;     /* Host name for tank */
    int tankScoreDecrement; /* Amount by which the tank's score */
                            /* is decreased from the player's */

    bool selfImmunity; /* Are players immune to their own weapons? */

    char *defaultShipShape; /* What ship shape is used for players */
                            /* who do not define their own? */
    char *tankShipShape;    /* What ship shape is used for tanks? */
    int maxPauseTime;       /* Max. time you can stay paused for */
    int maxClientsPerIP;    /* Max. number of clients that can login from the same IP */
} options_t;

extern options_t options;

#endif
