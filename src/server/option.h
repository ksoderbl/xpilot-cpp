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

    // friction only affects ships
    double frictionSetting;
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

    double ballRadius;
    bool multipleConnectors;

    char *rankFileName;
    char *rankWebpageFileName;
    char *rankWebpageCSS;

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
