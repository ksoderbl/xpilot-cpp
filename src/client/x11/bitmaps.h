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

#include "gfx2d.h"
#include "types.h"

constexpr int BM_HOLDER_FRIEND = 0;
constexpr int BM_HOLDER_ENEMY = 1;
constexpr int BM_BALL = 2;
constexpr int BM_SHIP_SELF = 3;
constexpr int BM_SHIP_FRIEND = 4;
constexpr int BM_SHIP_ENEMY = 5;
constexpr int BM_BULLET = 6;
constexpr int BM_BULLET_OWN = 7;
constexpr int BM_BASE_DOWN = 8;
constexpr int BM_BASE_LEFT = 9;
constexpr int BM_BASE_UP = 10;
constexpr int BM_BASE_RIGHT = 11;
constexpr int BM_FUELCELL = 12;
constexpr int BM_FUEL = 13;
constexpr int BM_ALL_ITEMS = 14;
constexpr int BM_CANNON_DOWN = 15;
constexpr int BM_CANNON_LEFT = 16;
constexpr int BM_CANNON_UP = 17;
constexpr int BM_CANNON_RIGHT = 18;
constexpr int BM_SPARKS = 19;
constexpr int BM_PAUSED = 20;
constexpr int BM_WALL_TOP = 21;
constexpr int BM_WALL_LEFT = 22;
constexpr int BM_WALL_BOTTOM = 23;
constexpr int BM_WALL_RIGHT = 24;
constexpr int BM_WALL_LU = 25;
constexpr int BM_WALL_RU = 26;
constexpr int BM_WALL_LD = 27;
constexpr int BM_WALL_RD = 28;

constexpr int BM_WALL_FILLED = 29;
constexpr int BM_WALL_UR = 30;
constexpr int BM_WALL_UL = 31;

constexpr int BM_SCORE_BG = 32;
constexpr int BM_LOGO = 33;
constexpr int BM_REFUEL = 34;
constexpr int BM_WORMHOLE = 35;
constexpr int BM_MINE_TEAM = 36;
constexpr int BM_MINE_OTHER = 37;
constexpr int BM_CONCENTRATOR = 38;
constexpr int BM_PLUSGRAVITY = 39;
constexpr int BM_MINUSGRAVITY = 40;
constexpr int BM_CHECKPOINT = 41;
constexpr int BM_METER = 42;
constexpr int BM_ASTEROIDCONC = 43;
constexpr int BM_BALL_GRAY = 44;

constexpr int NUM_OBJECT_BITMAPS = 45;
constexpr int NUM_BITMAPS = 45;

constexpr int BMS_UNINITIALIZED = 0;
constexpr int BMS_INITIALIZED = 1;
constexpr int BMS_READY = 2;
constexpr int BMS_ERROR = -1;

constexpr int BG_IMAGE_HEIGHT = 442;
constexpr int LOGO_HEIGHT = 223;

constexpr int RADAR_TEXTURE_SIZE = 32;

typedef struct
{
    Pixmap bitmap;
    Pixmap mask;
    bbox_t bbox;
    int rgb; /* the color this image is blended with */
} xp_bitmap_t;

/* xp_pixmap_t holds all data related to one "logical" image.
 * One logical image can consists of several rectangular pixel
 * arrays (physical images). All physical images share the same
 * overall dimensions.
 *
 * Note: if the count is negative it means that the other images
 * are rotated copies of the original image.
 */
typedef struct
{
    std::string filename;   /* the file containing the image */
    int count;              /* amount of images (see above) */
    int state;              /* the state of the image (BMS_*) */
    unsigned width, height; /* the (scaled) dimensions */
    bool scalable;          /* should this image be scaled */
    xp_bitmap_t *bitmaps;   /* platform dependent image data */
    xp_picture_t picture;   /* the image data in RGB format */
} xp_pixmap_t;

extern std::vector<xp_pixmap_t> pixmaps;

extern xp_pixmap_t xp_pixmaps[];

int Bitmaps_init(void);
void Bitmaps_cleanup(void);
int Bitmap_create(Drawable d, int img);
void Bitmap_update_scale(void);

xp_bitmap_t *Bitmap_get(Drawable d, int img, int bmp);
void Bitmap_paint(Drawable d, int img, int x, int y, int bmp);
void Bitmap_paint_area(Drawable d, xp_bitmap_t *bit, int x, int y, irec_t *r);
xp_bitmap_t *Bitmap_get_blended(Drawable d, int img, int rgb);
void Bitmap_paint_blended(Drawable d, int img, int x, int y, int rgb);
