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
#include <cstring>
#include <cctype>
#include <cstdio>
#include <csignal>
#include <cerrno>
#include <ctime>
#include <climits>
#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>
#include <sys/time.h>
#include <pwd.h>
#include <sys/param.h>

#include "commonproto.h"

#include "cannon.h"
#include "frame.h"
#include "metaserver.h"
#include "parser.h"
#include "score.h"
#include "server.h"
#include "update.h"
#include "robot.h"

#define SERVER
#include "version.h"
#include "xpconfig.h"
#include "types.h"
#include "serverconst.h"
#include "socklib.h"
#include "map.h"
#include "bit.h"
#include "sched.h"
#include "netserver.h"
#include "xperror.h"
#include "portability.h"
#include "server.h"
#include "rank.h"

#include "target.h"
#include "treasure.h"
#include "walls.h"

// char server_version[] = VERSION;

/*
 * Global variables
 */
int NumPlayers = 0;
int NumAlliances = 0;
int NumOperators = 0;
server_t Server;
char *serverAddr;
int ShutdownServer = -1;
int ShutdownDelay = 1000;
char ShutdownReason[MAX_CHARS];

long main_loops = 0; /* needed in events.c */
int mainLoopTime = 0;

#ifdef LOG
static bool Log = true;
#endif
static bool NoPlayersEnteredYet = true;
int game_lock = false;
time_t gameOverTime = 0;
time_t serverStartTime = 0;

extern void Main_loop(void);
static void Handle_signal(int sig_no);

extern void modifiersUnitTest(void);

int main(int argc, char **argv)
{
    int timer_tick_rate;
    char *addr;

    if (sock_startup() < 0)
    {
        warn("Error initializing sockets\n");
        return 1;
    }

    if (World_init() < 0)
    {
        warn("Error initializing world\n");
        return 1;
    }

    /*
     * Make output always linebuffered.  By default pipes
     * and remote shells cause stdout to be fully buffered.
     */
    setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
    setvbuf(stderr, NULL, _IOLBF, BUFSIZ);

    /*
     * --- Output copyright notice ---
     */

    printf("  " COPYRIGHT ".\n"
           "  " TITLE " comes with ABSOLUTELY NO WARRANTY; "
           "for details see the\n"
           "  provided LICENSE file.\n\n");

    init_error(argv[0]);

    seedMT((unsigned)time((time_t *)0) * Get_process_id());

    warn("Calling argv Parser");
    if (!Parser(argc, argv))
        exit(1);
    warn("Calling argv Parser returned");

    // Unit tests
    modifiersUnitTest();

    /* Lock the server into memory */
    plock_server(options.pLockServer);
    printf("compute gravity\n");
    Compute_gravity();
    printf("find base direction\n");
    Find_base_direction();
    printf("walls init\n");
    Walls_init();

    /* Allocate memory for players, shots and messages */
    Alloc_players(Num_bases() + MAX_PSEUDO_PLAYERS);
    Alloc_shots(MAX_TOTAL_SHOTS);
    Alloc_cells();

    Move_init();
    Robot_init();
    Treasure_init();

    Rank_init_saved_scores();

    /*
     * Get server's official name.
     */
    if (options.serverHost)
    {
        addr = sock_get_addr_by_name(options.serverHost);
        if (addr == NULL)
        {
            warn("Failed name lookup on: %s", options.serverHost);
            exit(1);
        }
        serverAddr = xp_strdup(addr);
        strlcpy(Server.host, options.serverHost, sizeof(Server.host));
    }
    else
        sock_get_local_hostname(Server.host, sizeof Server.host,
                                (options.reportToMetaServer != 0 &&
                                 options.searchDomainForXPilot != 0));

    Get_login_name(Server.owner, sizeof Server.owner);

    /* Log, if enabled. */
    Log_game("START");

    if (!Contact_init())
        End_game();

    Meta_init();

    Timing_setup();

    if (Setup_net_server() == -1)
        End_game();

    if (options.NoQuit)
        signal(SIGHUP, SIG_IGN);
    else
        signal(SIGHUP, Handle_signal);
    signal(SIGTERM, Handle_signal);
    signal(SIGINT, Handle_signal);
    signal(SIGPIPE, SIG_IGN);
#ifdef IGNORE_FPE
    signal(SIGFPE, SIG_IGN);
#endif
    /*
     * Set the time the server started
     */
    serverStartTime = time(NULL);

    printf("%s Server runs at %d frames per second\n",
           showtime(), options.framesPerSecond);

    // printf("timerResolution: %d\n", options.timerResolution);
    if (options.timerResolution > 0)
        timer_tick_rate = options.timerResolution;
    else
        timer_tick_rate = FPS;
    install_timer_tick(Main_loop, timer_tick_rate);

    // printf("calling sched\n");
    sched();
    printf("sched returned!?\n");
    End_game();

    return 1;
}

void Main_loop(void)
{
    struct timeval tv1, tv2;

    gettimeofday(&tv1, NULL);

    main_loops++;

    // if ((main_loops % 1000) == 0)
    // {
    // xpinfo("Main_loop: main_loops: %d", main_loops);
    // }

    if ((main_loops & 0x3F) == 0)
        Meta_update(false);

    /*
     * Check for possible shutdown, the server will
     * shutdown when ShutdownServer (a counter) reaches 0.
     * If the counter is < 0 then no shutdown is in progress.
     */
    if (ShutdownServer >= 0)
    {
        if (ShutdownServer == 0)
            End_game();
        else
            ShutdownServer--;
    }

    Input();

    if (NumPlayers > NumRobots + NumPseudoPlayers || options.RawMode)
    {
        if (NoPlayersEnteredYet)
        {
            if (NumPlayers > NumRobots + NumPseudoPlayers)
            {
                NoPlayersEnteredYet = false;
                if (options.gameDuration > 0.0)
                {
                    printf("%s Server will stop in %g minutes.\n",
                           showtime(), options.gameDuration);
                    gameOverTime = (time_t)(options.gameDuration * 60) + time(NULL);
                }
            }
        }

        Update_objects();

#define CONF_UPDATES_PR_FRAME 1
        if ((main_loops % CONF_UPDATES_PR_FRAME) == 0)
            Frame_update();
    }

    // xpinfo("Main_loop: checking end game");

    if (!options.NoQuit && NumPlayers == NumRobots + NumPseudoPlayers && !login_in_progress && !NumQueuedPlayers)
    {

        if (!NoPlayersEnteredYet)
            End_game();
        if (serverStartTime + 5 * 60 < time(NULL))
        {
            error("First player has yet to show his butt, I'm bored... Bye!");
            Log_game("NOSHOW");
            End_game();
        }
    }

    // xpinfo("Main_loop: queue loop");

    Queue_loop();

    {
        int s, us;

        gettimeofday(&tv2, NULL);

        s = tv2.tv_sec - tv1.tv_sec;
        us = tv2.tv_usec - tv1.tv_usec;

        us += s * 1000000;

        mainLoopTime = us;
    }

    // xpinfo("Main_loop: end");
}

/*
 *  Last function, exit with grace.
 */
void End_game(void)
{
    player_t *pl;
    char msg[MSG_LEN];

    if (ShutdownServer == 0)
    {
        warn("Shutting down...");
        snprintf(msg, sizeof(msg), "shutting down: %s", ShutdownReason);
    }
    else
        snprintf(msg, sizeof(msg), "server exiting");

    while (NumPlayers > 0)
    { /* Kick out all remaining players */
        pl = Player_by_index(NumPlayers - 1);
        if (pl->conn == NULL)
            Delete_player(pl);
        else
            Destroy_connection(pl->conn, msg);
    }

    /* Tell meta server that we are gone. */
    Meta_gone();

    Contact_cleanup();

    /* Ranking. */
    Rank_write_webpage();
    Rank_write_rankfile();

    Free_players();
    Free_shots();
    World_free();
    Free_cells();
    Free_options();
    Log_game("END");

    exit(0);
}

/*
 * Return a good team number for a player.
 *
 * If the team is not specified, the player is assigned
 * to a non-empty team which has space.
 *
 * If there is none or only one team with playing (i.e. non-paused)
 * players the player will be assigned to a randomly chosen empty team.
 *
 * If there is more than one team with playing players,
 * the player will be assigned randomly to a team which
 * has the least number of playing players.
 *
 * If all non-empty teams are full, the player is assigned
 * to a randomly chosen available team.
 *
 * Prefer not to place players in the robotTeam if possible.
 */
int Pick_team(int pick_for_type)
{
    int i, least_players, num_available_teams = 0, playing_teams = 0;
    int losing_team;
    player_t *pl;
    int playing[MAX_TEAMS], free_bases[MAX_TEAMS], available_teams[MAX_TEAMS];
    long team_score[MAX_TEAMS];
    long losing_score;

    for (i = 0; i < MAX_TEAMS; i++)
    {
        free_bases[i] = World.teams[i].NumBases - World.teams[i].NumMembers;
        playing[i] = 0;
        team_score[i] = 0;
        available_teams[i] = 0;
    }
    if (options.restrictRobots)
    {
        if (pick_for_type == PL_TYPE_ROBOT)
        {
            if (free_bases[options.robotTeam] > 0)
                return options.robotTeam;
            else
                return TEAM_NOT_SET;
        }
    }
    if (options.reserveRobotTeam)
    {
        if (pick_for_type != PL_TYPE_ROBOT)
            free_bases[options.robotTeam] = 0;
    }

    /*
     * Find out which teams have actively playing members.
     * Exclude paused players and tanks.
     * And calculate the score for each team.
     */
    for (i = 0; i < NumPlayers; i++)
    {
        pl = Player_by_index(i);
        if (Player_is_tank(pl))
            continue;
        if (Player_is_paused(pl))
            continue;
        if (!playing[pl->team]++)
            playing_teams++;
        if (Player_is_human(pl) || Player_is_robot(pl))
            team_score[pl->team] += Get_Score(pl);
    }
    if (playing_teams <= 1)
    {
        for (i = 0; i < MAX_TEAMS; i++)
        {
            if (!playing[i] && free_bases[i] > 0)
                available_teams[num_available_teams++] = i;
        }
    }
    else
    {
        least_players = NumPlayers;
        for (i = 0; i < MAX_TEAMS; i++)
        {
            /* We fill teams with players first. */
            if (playing[i] > 0 && free_bases[i] > 0)
            {
                if (playing[i] < least_players)
                    least_players = playing[i];
            }
        }

        for (i = 0; i < MAX_TEAMS; i++)
        {
            if (free_bases[i] > 0)
            {
                if (least_players == NumPlayers || playing[i] == least_players)
                    available_teams[num_available_teams++] = i;
            }
        }
    }

    if (!num_available_teams)
    {
        for (i = 0; i < MAX_TEAMS; i++)
        {
            if (free_bases[i] > 0)
                available_teams[num_available_teams++] = i;
        }
    }

    if (num_available_teams == 1)
        return available_teams[0];

    if (num_available_teams > 1)
    {
        losing_team = -1;
        losing_score = LONG_MAX;
        for (i = 0; i < num_available_teams; i++)
        {
            if (team_score[available_teams[i]] < losing_score && available_teams[i] != options.robotTeam)
            {
                losing_team = available_teams[i];
                losing_score = team_score[losing_team];
            }
        }
        return losing_team;
    }

    /*NOTREACHED*/
    return TEAM_NOT_SET;
}

const char *Describe_game_status(void)
{
    return (game_lock && ShutdownServer == -1)    ? "locked"
           : (!game_lock && ShutdownServer != -1) ? "shutting down"
           : (game_lock && ShutdownServer != -1)  ? "locked and shutting down"
                                                  : "ok";
}

/*
 * Return status for server
 *
 * TODO
 */
void Server_info(char *str, size_t max_size)
{
    int i, j, k;
    player_t *pl, **order, *best = NULL;
    double ratio, best_ratio = -1e7;
    char name[MAX_CHARS * 2 + 4];
    char lblstr[MAX_CHARS];
    char msg[MSG_LEN];

    sprintf(str,
            "SERVER VERSION...: %s\n"
            "STATUS...........: %s\n"
            "MAX SPEED........: %d fps\n"
            "WORLD (%3dx%3d)..: %s\n"
            "      AUTHOR.....: %s\n"
            "PLAYERS (%2d/%2d)..:\n",
            VERSION,
            (game_lock && ShutdownServer == -1) ? "locked" : (!game_lock && ShutdownServer != -1) ? "shutting down"
                                                         : (game_lock && ShutdownServer != -1)    ? "locked and shutting down"
                                                                                                  : "ok",
            FPS,
            World.x, World.y, World.name, World.author,
            NumPlayers, Num_bases());

    if (strlen(str) >= max_size)
    {
        errno = 0;
        error("Server_info string overflow (%d)", max_size);
        str[max_size - 1] = '\0';
        return;
    }
    if (NumPlayers <= 0)
    {
        return;
    }

    sprintf(msg,
            "\nNO:  TM: NAME:             LIFE:   SC:    PLAYER:\n"
            "-------------------------------------------------\n");
    if (strlen(msg) + strlen(str) >= max_size)
    {
        return;
    }
    strlcat(str, msg, max_size);

    if ((order = (player_t **)malloc(NumPlayers * sizeof(player_t *))) == NULL)
    {
        error("No memory for order");
        return;
    }
    for (i = 0; i < NumPlayers; i++)
    {
        pl = Player_by_index(i);
        if (BIT(World.rules->mode, LIMITED_LIVES))
        {
            ratio = (double)Get_Score(pl);
        }
        else
        {
            ratio = (double)Get_Score(pl) / (pl->pl_life + 1);
        }
        if ((best == NULL || ratio > best_ratio) && !Player_is_paused(pl))
        {
            best_ratio = ratio;
            best = pl;
        }
        for (j = 0; j < i; j++)
        {
            if (order[j]->score < Get_Score(pl))
            {
                for (k = i; k > j; k--)
                {
                    order[k] = order[k - 1];
                }
                break;
            }
        }
        order[j] = pl;
    }
    for (i = 0; i < NumPlayers; i++)
    {
        pl = order[i];
        strlcpy(name, pl->name, MAX_CHARS);
        if (Player_is_robot(pl))
        {
            if ((k = Robot_war_on_player(Player_by_id(pl->id))) != NO_ID)
            {
                sprintf(name + strlen(name), " (%s)", Player_by_id(k)->name);
                if (strlen(name) >= 19)
                {
                    strcpy(&name[17], ")");
                }
            }
        }
        sprintf(lblstr, "%c%c %-19s%03d%6d",
                (pl == best) ? '*' : pl->mychar,
                (pl->team == TEAM_NOT_SET) ? ' ' : (pl->team + '0'),
                name, (int)pl->pl_life, (int)Get_Score(pl));
        sprintf(msg, "%2d... %-36s%s@%s\n",
                i + 1, lblstr, pl->username,
                Player_is_human(pl)
                    ? pl->hostname
                    : "xpilot.org");
        if (strlen(msg) + strlen(str) >= max_size)
            break;
        strlcat(str, msg, max_size);
    }
    free(order);
}

static void Handle_signal(int sig_no)
{
    errno = 0;

    switch (sig_no)
    {

    case SIGHUP:
        if (options.NoQuit)
        {
            signal(SIGHUP, SIG_IGN);
            return;
        }
        error("Caught SIGHUP, terminating.");
        End_game();
        break;
    case SIGINT:
        error("Caught SIGINT, terminating.");
        End_game();
        break;
    case SIGTERM:
        error("Caught SIGTERM, terminating.");
        End_game();
        break;

    default:
        error("Caught unkown signal: %d", sig_no);
        End_game();
        break;
    }
    _exit(sig_no); /* just in case */
}

/* kps - is this useful??? */
void Log_game(const char *heading)
{
    char str[1024];
    FILE *fp;
    char timenow[81];
    struct tm *ptr;
    time_t lt;

    if (!options.Log)
        return;

    lt = time(NULL);
    ptr = localtime(&lt);
    strftime(timenow, 79, "%I:%M:%S %p %Z %A, %B %d, %Y", ptr);

    snprintf(str, sizeof(str),
             "%-50.50s\t%10.10s@%-15.15s\tWorld: %-25.25s\t%10.10s\n",
             timenow, Server.owner, Server.host, World.name, heading);

    if ((fp = fopen(Conf_logfile(), "a")) == NULL)
    {
        error("Couldn't open log file, contact %s", Conf_localguru());
        return;
    }

    fputs(str, fp);
    fclose(fp);
}

void Game_Over(void)
{
    world_t *world = &World;
    double maxsc, minsc;
    int i, win_team = TEAM_NOT_SET, lose_team = TEAM_NOT_SET;
    char msg[MSG_LEN];
    player_t *win_pl = NULL, *lose_pl = NULL;

    Set_message("Game over...");

    /*
     * Hack to prevent Compute_Game_Status from starting over again...
     */
    options.gameDuration = -1.0;

    if (Team_play(world))
    {
        double teamscore[MAX_TEAMS];

        for (i = 0; i < MAX_TEAMS; i++)
            teamscore[i] = FLT_MAX; /* These teams are not used... */

        for (i = 0; i < NumPlayers; i++)
        {
            player_t *pl = Player_by_index(i);
            int team;

            if (Player_is_paused(pl))
                continue;

            if (Player_is_human(pl) || Player_is_robot(pl))
            {
                team = pl->team;
                if (teamscore[team] == FLT_MAX)
                    teamscore[team] = 0;
                teamscore[team] += Get_Score(pl);
            }
        }

        maxsc = -FLT_MAX;
        minsc = FLT_MAX;

        for (i = 0; i < MAX_TEAMS; i++)
        {
            if (teamscore[i] != FLT_MAX)
            {
                if (teamscore[i] > maxsc)
                {
                    maxsc = teamscore[i];
                    win_team = i;
                }
                if (teamscore[i] < minsc)
                {
                    minsc = teamscore[i];
                    lose_team = i;
                }
            }
        }

        if (win_team != TEAM_NOT_SET)
        {
            snprintf(msg, sizeof(msg), "Best team (%.2f Pts): Team %d",
                     maxsc, win_team);
            Set_message(msg);
            printf("%s\n", msg);
        }

        if (lose_team != TEAM_NOT_SET && lose_team != win_team)
        {
            snprintf(msg, sizeof(msg), "Worst team (%.2f Pts): Team %d",
                     minsc, lose_team);
            Set_message(msg);
            printf("%s\n", msg);
        }
    }

    maxsc = -FLT_MAX;
    minsc = FLT_MAX;

    for (i = 0; i < NumPlayers; i++)
    {
        player_t *pl_i = Player_by_index(i);

        if (Player_is_paused(pl_i))
            continue;

        Player_set_state(pl_i, PL_STATE_DEAD);
        if (Player_is_human(pl_i))
        {
            if (Get_Score(pl_i) > maxsc)
            {
                maxsc = Get_Score(pl_i);
                win_pl = pl_i;
            }
            if (Get_Score(pl_i) < minsc)
            {
                minsc = Get_Score(pl_i);
                lose_pl = pl_i;
            }
        }
    }
    if (win_pl)
    {
        snprintf(msg, sizeof(msg), "Best human player: %s", win_pl->name);
        Set_message(msg);
        printf("%s\n", msg);
    }
    if (lose_pl && lose_pl != win_pl)
    {
        snprintf(msg, sizeof(msg), "Worst human player: %s", lose_pl->name);
        Set_message(msg);
        printf("%s\n", msg);
    }
}

void Server_shutdown(const char *user_name, int delay, const char *reason)
{
    Set_message_f("|*******| %s (%s) |*******| \"%s\" [*Server notice*]",
                  (delay > 0) ? "SHUTTING DOWN" : "SHUTDOWN STOPPED",
                  user_name, reason);
    strlcpy(ShutdownReason, reason, sizeof(ShutdownReason));
    if (delay > 0)
    {
        /* delay is in seconds */;
        ShutdownServer = delay * FPS;
        ShutdownDelay = ShutdownServer;
    }
    else
        ShutdownServer = -1;
}

void Server_log_admin_message(player_t *pl, const char *str)
{
    /*
     * Only log the message if logfile already exists,
     * is writable and less than some KBs in size.
     */
    const char *logfilename = options.adminMessageFileName;
    const int logfile_size_limit = options.adminMessageFileSizeLimit;
    FILE *fp;
    struct stat st;
    char msg[MSG_LEN * 2];

    if ((logfilename != NULL) &&
        (logfilename[0] != '\0') &&
        (logfile_size_limit > 0) &&
        (access(logfilename, 2) == 0) &&
        (stat(logfilename, &st) == 0) &&
        (st.st_size + 80 < logfile_size_limit) &&
        ((size_t)(logfile_size_limit - st.st_size - 80) > strlen(str)) &&
        ((fp = fopen(logfilename, "a")) != NULL))
    {
        fprintf(fp,
                "%s[%s]{%s@%s(%s)|%s}:\n"
                "\t%s\n",
                showtime(),
                pl->name,
                pl->username, pl->hostname,
                Player_get_addr(pl),
                Player_get_dpy(pl),
                str);
        fclose(fp);
        snprintf(msg, sizeof(msg), "%s [%s]:[%s]", str, pl->name, "GOD");
        Set_player_message(pl, msg);
    }
    else
        Set_player_message(pl, " < GOD doesn't seem to be listening>");
}

#if defined(PLOCKSERVER) && defined(__linux__)
/*
 * Patches for Linux plock support by Steve Payne <srp20@cam.ac.uk>
 * also added the -pLockServer command line option.
 * All messed up by BG again, with thanks and apologies to Steve.
 */
/* Linux doesn't seem to have plock(2).  *sigh* (BG) */
#if !defined(PROCLOCK) || !defined(UNLOCK)
#define PROCLOCK 0x01
#define UNLOCK 0x00
#endif
static int plock(int op)
{
#if defined(MCL_CURRENT) && defined(MCL_FUTURE)
    return op ? mlockall(MCL_CURRENT | MCL_FUTURE) : munlockall();
#else
    return -1;
#endif
}
#endif

/*
 * Lock the server process data and code segments into memory
 * if this program has been compiled with the PLOCKSERVER flag.
 * Or unlock the server process if the argument is false.
 */
int plock_server(bool on)
{
#ifdef PLOCKSERVER
    int op;

    if (on)
        op = PROCLOCK;
    else
        op = UNLOCK;

    if (plock(op) == -1)
    {
        static int num_plock_errors;
        if (++num_plock_errors <= 3)
            error("Can't plock(%d)", op);
        return -1;
    }
    return on ? 1 : 0;
#else
    if (on)
        printf("Can't plock: Server was not compiled with plock support\n");
    return 0;
#endif
}

/* kps - this is really ugly */
extern bool in_move_player;

bool Friction_area_hitfunc(group_t *groupptr, const move_t *move)
{
    if (in_move_player)
        return true;
    return false;
}

/*
 * Handling of group properties
 */
void Team_immunity_init(void)
{
    int group;

    for (group = 0; group < num_groups; group++)
    {
        group_t *gp = groupptr_by_id(group);

        if (gp->type == CANNON)
        {
            cannon_t *cannon = Cannon_by_index(gp->mapobj_ind);

            assert(cannon->group == group);
            Cannon_set_hitmask(group, cannon);
        }
    }

#if 0
    /* change hitmask of all cannons */
    P_grouphack(CANNON, Cannon_set_hitmask);
#endif
}

/* kps - called at server startup to initialize hit masks */
void Hitmasks_init(void)
{
    Target_init();
    Team_immunity_init();
}
