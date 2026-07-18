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

#include "move.h"

struct move_parameters mp;
double wallBounceExplosionMult;

// TODO: where should this be?
extern long KILLING_SHOTS;

static void Move_init1(void)
{
    mp.click_width = PIXEL_TO_CLICK(World.width);
    mp.click_height = PIXEL_TO_CLICK(World.height);

    LIMIT(options.maxObjectWallBounceSpeed, 0, World.pixel_hypotenuse);
    LIMIT(options.maxShieldedWallBounceSpeed, 0, World.pixel_hypotenuse);
    LIMIT(options.maxUnshieldedWallBounceSpeed, 0, World.pixel_hypotenuse);

    LIMIT(options.maxShieldedWallBounceAngle, 0, 180);
    LIMIT(options.maxUnshieldedWallBounceAngle, 0, 180);
    LIMIT(options.playerWallBrakeFactor, 0, 1);
    LIMIT(options.objectWallBounceBrakeFactor, 0, 1);
    LIMIT(options.objectWallBounceLifeFactor, 0, 1);
    LIMIT(options.wallBounceFuelDrainMult, 0, 1000);
    wallBounceExplosionMult = sqrt(options.wallBounceFuelDrainMult);

    mp.max_shielded_angle = (int)(options.maxShieldedWallBounceAngle * ANGLE_RESOLUTION / 360);
    mp.max_unshielded_angle = (int)(options.maxUnshieldedWallBounceAngle * ANGLE_RESOLUTION / 360);

    mp.obj_bounce_mask = 0;
    if (options.sparksWallBounce)
        SET_BIT(mp.obj_bounce_mask, OBJ_SPARK_BIT);
    if (options.debrisWallBounce)
        SET_BIT(mp.obj_bounce_mask, OBJ_DEBRIS_BIT);
    if (options.shotsWallBounce)
        SET_BIT(mp.obj_bounce_mask, OBJ_SHOT_BIT | OBJ_CANNON_SHOT_BIT);
    if (options.itemsWallBounce)
        SET_BIT(mp.obj_bounce_mask, OBJ_ITEM_BIT);
    if (options.missilesWallBounce)
        SET_BIT(mp.obj_bounce_mask, OBJ_SMART_SHOT_BIT | OBJ_TORPEDO_BIT | OBJ_HEAT_SHOT_BIT);
    if (options.minesWallBounce)
        SET_BIT(mp.obj_bounce_mask, OBJ_MINE_BIT);
    if (options.ballsWallBounce)
        SET_BIT(mp.obj_bounce_mask, OBJ_BALL_BIT);
    if (options.asteroidsWallBounce)
        SET_BIT(mp.obj_bounce_mask, OBJ_ASTEROID_BIT);

    mp.obj_cannon_mask = (KILLING_SHOTS) | OBJ_MINE_BIT | OBJ_SHOT_BIT | OBJ_PULSE_BIT |
                         OBJ_SMART_SHOT_BIT | OBJ_TORPEDO_BIT | OBJ_HEAT_SHOT_BIT |
                         OBJ_ASTEROID_BIT;
    if (options.cannonsUseItems)
        mp.obj_cannon_mask |= OBJ_ITEM_BIT;
    mp.obj_target_mask = mp.obj_cannon_mask | OBJ_BALL_BIT | OBJ_SPARK_BIT;
    mp.obj_treasure_mask = mp.obj_bounce_mask | OBJ_BALL_BIT | OBJ_PULSE_BIT;
}

static void Move_init2(void)
{
    LIMIT(options.maxObjectWallBounceSpeed, 0, World.pixel_hypotenuse);
    LIMIT(options.maxShieldedWallBounceSpeed, 0, World.pixel_hypotenuse);
    LIMIT(options.maxUnshieldedWallBounceSpeed, 0, World.pixel_hypotenuse);

    LIMIT(options.playerWallBounceBrakeFactor, 0, 1);
    LIMIT(options.playerWallFriction, 0, FLT_MAX);
    LIMIT(options.objectWallBounceBrakeFactor, 0, 1);
    LIMIT(options.objectWallBounceLifeFactor, 0, 1);

    mp.obj_bounce_mask = 0;
    if (options.sparksWallBounce)
        SET_BIT(mp.obj_bounce_mask, OBJ_SPARK_BIT);
    if (options.debrisWallBounce)
        SET_BIT(mp.obj_bounce_mask, OBJ_DEBRIS_BIT);
    if (options.shotsWallBounce)
        SET_BIT(mp.obj_bounce_mask, OBJ_SHOT_BIT | OBJ_CANNON_SHOT_BIT);
    if (options.itemsWallBounce)
        SET_BIT(mp.obj_bounce_mask, OBJ_ITEM_BIT);
    if (options.missilesWallBounce)
        SET_BIT(mp.obj_bounce_mask,
                OBJ_SMART_SHOT_BIT | OBJ_TORPEDO_BIT | OBJ_HEAT_SHOT_BIT);
    if (options.minesWallBounce)
        SET_BIT(mp.obj_bounce_mask, OBJ_MINE_BIT);
    if (options.ballsWallBounce)
        SET_BIT(mp.obj_bounce_mask, OBJ_BALL_BIT);
    if (options.asteroidsWallBounce)
        SET_BIT(mp.obj_bounce_mask, OBJ_ASTEROID_BIT);
    if (options.pulsesWallBounce)
        SET_BIT(mp.obj_bounce_mask, OBJ_PULSE_BIT);

    mp.obj_cannon_mask = (KILLING_SHOTS) | OBJ_MINE_BIT | OBJ_SHOT_BIT | OBJ_PULSE_BIT | OBJ_SMART_SHOT_BIT | OBJ_TORPEDO_BIT | OBJ_HEAT_SHOT_BIT | OBJ_ASTEROID_BIT;
    if (options.cannonsPickupItems)
        mp.obj_cannon_mask |= OBJ_ITEM_BIT;
    mp.obj_target_mask = mp.obj_cannon_mask | OBJ_BALL_BIT | OBJ_SPARK_BIT;
    mp.obj_treasure_mask = mp.obj_bounce_mask | OBJ_BALL_BIT | OBJ_PULSE_BIT;
}

void Move_init(void)
{
    if (!is_polygon_map)
        Move_init1();
    else
        Move_init2();
}