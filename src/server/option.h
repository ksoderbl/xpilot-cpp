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
    int shotLife;  /* Default number of ticks */
                   /* each shot will live */
    int maxRobots; /* How many robots should enter */
    int minRobots; /* the game? */
    char *robotFile;
    int robotsTalk;
    int robotsLeave;     /* Do robots leave at all? */
    int robotLeaveLife;  /* Max life per robot (0=off)*/
    int robotLeaveScore; /* Min score for robot to live (0=off)*/
    int robotLeaveRatio; /* Min ratio for robot to live (0=off)*/
    int robotTeam;
    bool restrictRobots;   /* Restrict robots to robotTeam? */
    bool reserveRobotTeam; /* Allow only robots in robotTeam? */
    int maxPlayerShots;    /* Max shots pr. player */
    bool shotsGravity;     /* Shots affected by gravity */
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

    // wall lowers speed if less than 1
    double objectWallBrakeFactor;

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

    // Do cannons fire flak?
    bool cannonFlak;

    // How long do cannons stay dead?
    int cannonDeadTime;

    double survivalScore;

    // Keep shots when player leaves?
    bool keepShots;

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

    char *robotUserName; /* Real name for robot */
    char *robotHostName; /* Host name for robot */

    char *tankUserName;     /* Real name for tank */
    char *tankHostName;     /* Host name for tank */
    int tankScoreDecrement; /* Amount by which the tank's score */
                            /* is decreased from the player's */

    bool selfImmunity; /* Are players immune to their own weapons? */

    char *defaultShipShape; /* What ship shape is used for players */
                            /* who do not define their own? */
    char *tankShipShape;    /* What ship shape is used for tanks? */
    int maxPauseTime;       /* Max. time you can stay paused for */
    int maxClientsPerIP;    /* Max. number of clients that can login from the same IP */

    int recordMode;
    int recordFlushInterval;

    char *recordFileName;

    char *rankFileName;
    char *rankWebpageFileName;
    char *rankWebpageCSS;

    bool polygonMode;
} options_t;

/*
 * Prototypes for option.c
 */
// void Options_parse(void);
// void Options_free(void);
// bool Convert_string_to_int(const char *value_str, int *int_ptr);
// bool Convert_string_to_float(const char *value_str, double *float_ptr);
// bool Convert_string_to_bool(const char *value_str, bool *bool_ptr);
// void Convert_list_to_string(list_t list, char **string);
// void Convert_string_to_list(const char *value, list_t *list_ptr);

extern options_t options;
