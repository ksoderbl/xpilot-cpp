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

#include <climits>
#include <cmath>

#include "types.h"

/*
 * FLT_MAX is ANSI C standard, but some systems (BSD) use
 * MAXFLOAT instead.
 */
#ifndef FLT_MAX
#if defined(__sgi) || defined(__FreeBSD__)
#include <float.h> /* FLT_MAX for SGI Personal Iris or FreeBSD */
#endif
#if !defined(FLT_MAX)
#if defined(MAXFLOAT)
#define FLT_MAX MAXFLOAT
#else
#define FLT_MAX 1e30f /* should suffice :-) */
#endif
#endif
#endif

/* Not everyone has PI (or M_PI defined). */
#ifndef M_PI
#define PI 3.14159265358979323846
#else
#define PI M_PI
#endif

/* Not everyone has LINE_MAX either, *sigh* */
#ifndef LINE_MAX
#define LINE_MAX 2048
#endif

#define ANGLE_RESOLUTION 128

#define BLOCK_SZ 35

#define TABLE_SIZE ANGLE_RESOLUTION

#define NELEM(a) ((int)(sizeof(a) / sizeof((a)[0])))

#undef ABS
#define ABS(x) ((x) < 0 ? -(x) : (x))
#ifndef MAX
#define MIN(x, y) ((x) > (y) ? (y) : (x))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif
#define sqr(x) ((x) * (x))
#define LENGTH(x, y) (hypot((double)(x), (double)(y)))
#define VECTOR_LENGTH(v) (hypot((double)(v).x, (double)(v).y))
#define QUICK_LENGTH(x, y) (ABS(x) + ABS(y)) /*-BA Only approx, but v. quick */
#define LIMIT(val, lo, hi) (val = (val) > (hi) ? (hi) : ((val) < (lo) ? (lo) : (val)))

#ifndef MOD2
#define MOD2(x, m) ((x) & ((m) - 1))
#endif /* MOD2 */

/* Do NOT change these! */
#define OLD_MAX_CHECKS 26
#define MAX_TEAMS 10

#define EXPIRED_MINE_ID 4096 /* assume no player has this id */

#define MAX_CHARS 80
#define MSG_LEN 256

#define NUM_MODBANKS 4

#define SPEED_LIMIT 65.0
#define MAX_PLAYER_TURNSPEED 64.0
#define MIN_PLAYER_TURNSPEED 0.0
#define MAX_PLAYER_POWER 55.0
#define MIN_PLAYER_POWER 5.0
#define MAX_PLAYER_TURNRESISTANCE 1.0
#define MIN_PLAYER_TURNRESISTANCE 0.0

#define FUEL_SCALE_BITS 8
#define FUEL_SCALE_FACT (1 << FUEL_SCALE_BITS)
#define FUEL_MASS(fuel) ((fuel) * 0.005)
// #define MAX_STATION_FUEL_TIMES_256 (500 << FUEL_SCALE_BITS)
// #define START_STATION_FUEL_TIMES_256 MAX_STATION_FUEL_TIMES_256
// #define STATION_REGENERATION_TIMES_256 (0.06 * FUEL_SCALE_FACT)
// #define MAX_PLAYER_FUEL_TIMES_256 (2600 << FUEL_SCALE_BITS)
// #define MIN_PLAYER_FUEL_TIMES_256 (350 << FUEL_SCALE_BITS)
// #define REFUEL_RATE_TIMES_256 (5 << FUEL_SCALE_BITS)

constexpr double MAX_PLAYER_FUEL = 2600.0;

// #define TARGET_DAMAGE_TIMES_256 (250 << FUEL_SCALE_BITS)
// #define TARGET_FUEL_REPAIR_PER_FRAME_TIMES_256 (TARGET_DAMAGE_TIMES_256 / (FPS * 10))
// #define TARGET_REPAIR_PER_TICK_TIMES_256 (TARGET_DAMAGE_TIMES_256 / (FPS * 600))
// #define TARGET_UPDATE_DELAY (TARGET_DAMAGE_TIMES_256 / (TARGET_REPAIR_PER_TICK_TIMES_256 * BLOCK_SZ))

constexpr double MAX_STATION_FUEL = 500.0;
constexpr double TARGET_DAMAGE = 250.0;
constexpr double SELF_DESTRUCT_DELAY = 150.0;

/*
 * Size (pixels) of radius for legal HIT!
 * Was 14 until 4.2. Increased due to 'analytical collision detection'
 * which inspects a real circle and not just a square anymore.
 */
#define SHIP_SZ 16

#define VISIBILITY_DISTANCE 1000.0

#define BALL_RADIUS 10

#define MISSILE_LEN 15

#define TEAM_NOT_SET 0xffff
#define TEAM_NOT_SET_STR "4095"

constexpr std::size_t DEBRIS_TYPES = (8 * 4 * 4);

/*
 * The server supports only 4 colors, except for spark/debris, which
 * may have 8 different colors.
 */
#define NUM_COLORS 4

#define BLACK 0
#define WHITE 1
#define BLUE 2
#define RED 3

/*
 * The minimum and maximum playing window sizes supported by the server.
 */
#define MIN_VIEW_SIZE 384
#define MAX_VIEW_SIZE 1024
#define DEF_VIEW_SIZE 1024

/*
 * Spark rand limits.
 */
#define MIN_SPARK_RAND 0    /* Not display spark */
#define MAX_SPARK_RAND 0x80 /* Always display spark */
#define DEF_SPARK_RAND 0x55 /* 66% */

#define DSIZE 4 /* Size of diamond (on radar) */

/*
 * Polygon style flags
 */
#define STYLE_FILLED (1U << 0)
#define STYLE_TEXTURED (1U << 1)
#define STYLE_INVISIBLE (1U << 2)
#define STYLE_INVISIBLE_RADAR (1U << 3)
