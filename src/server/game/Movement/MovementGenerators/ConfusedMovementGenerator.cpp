/*
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Creature.h"
#include "MapManager.h"
#include "Opcodes.h"
#include "ConfusedMovementGenerator.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "VMapFactory.h"
#include "PathInfo.h"

#ifdef MAP_BASED_RAND_GEN
#define rand_norm() unit.rand_norm()
#define urand(a, b) unit.urand(a, b)
#endif

template<class T>
void ConfusedMovementGenerator<T>::Initialize(T &unit)
{
<<<<<<< HEAD
    // set initial position
    unit.GetPosition(i_x, i_y, i_z);
=======
    float const wanderDistance = 4;
    float x, y, z;
    x = unit.GetPositionX();
    y = unit.GetPositionY();
    z = unit.GetPositionZ();

    Map const* map = unit.GetBaseMap();

    i_nextMove = 1;

    bool is_water_ok, is_land_ok;
    _InitSpecific(unit, is_water_ok, is_land_ok);

    for (uint8 idx = 0; idx <= MAX_CONF_WAYPOINTS; ++idx)
    {
        float wanderX = x + wanderDistance * (float)rand_norm() - wanderDistance/2;
        float wanderY = y + wanderDistance * (float)rand_norm() - wanderDistance/2;
        Trinity::NormalizeMapCoord(wanderX);
        Trinity::NormalizeMapCoord(wanderY);

        float new_z = map->GetHeight(wanderX, wanderY, z, true);
        if (new_z > INVALID_HEIGHT && unit.IsWithinLOS(wanderX, wanderY, new_z))
        {
            // Don't move in water if we're not already in
            // Don't move on land if we're not already on it either
            bool is_water_now = map->IsInWater(x, y, z);
            bool is_water_next = map->IsInWater(wanderX, wanderY, new_z);
            if ((is_water_now && !is_water_next && !is_land_ok) || (!is_water_now && is_water_next && !is_water_ok))
            {
                i_waypoints[idx][0] = idx > 0 ? i_waypoints[idx-1][0] : x; // Back to previous location
                i_waypoints[idx][1] = idx > 0 ? i_waypoints[idx-1][1] : y;
                i_waypoints[idx][2] = idx > 0 ? i_waypoints[idx-1][2] : z;
                continue;
            }

            // Taken from FleeingMovementGenerator
            if (!(new_z - z) || wanderDistance / fabs(new_z - z) > 1.0f)
            {
                i_waypoints[idx][0] = wanderX;
                i_waypoints[idx][1] = wanderY;
                i_waypoints[idx][2] = new_z;
                continue;
            }
        }
        else    // Back to previous location
        {
            i_waypoints[idx][0] = idx > 0 ? i_waypoints[idx-1][0] : x;
            i_waypoints[idx][1] = idx > 0 ? i_waypoints[idx-1][1] : y;
            i_waypoints[idx][2] = idx > 0 ? i_waypoints[idx-1][2] : z;
            continue;
        }
    }
>>>>>>> eb190eb0bc99cf3011ed4d7a30df5898b29aa441

    unit.SetTarget(0);
    unit.SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_CONFUSED);
    unit.CastStop();
    unit.StopMoving();
    unit.AddUnitMovementFlag(MOVEMENTFLAG_WALKING); // Should actually be splineflag
    unit.AddUnitState(UNIT_STAT_CONFUSED);
}

<<<<<<< HEAD
=======
template<>
void ConfusedMovementGenerator<Creature>::_InitSpecific(Creature &creature, bool &is_water_ok, bool &is_land_ok)
{
    is_water_ok = creature.canSwim();
    is_land_ok  = creature.canWalk();
}

template<>
void ConfusedMovementGenerator<Player>::_InitSpecific(Player &, bool &is_water_ok, bool &is_land_ok)
{
    is_water_ok = true;
    is_land_ok  = true;
}

>>>>>>> eb190eb0bc99cf3011ed4d7a30df5898b29aa441
template<class T>
void ConfusedMovementGenerator<T>::Reset(T &unit)
{
    i_nextMoveTime.Reset(0);
    unit.StopMoving();
}

template<class T>
bool ConfusedMovementGenerator<T>::Update(T &unit, const uint32 diff)
{
    if (unit.HasUnitState(UNIT_STAT_ROOT | UNIT_STAT_STUNNED | UNIT_STAT_DISTRACTED))
        return true;

    if (i_nextMoveTime.Passed())
    {
        // currently moving, update location
        Traveller<T> traveller(unit);
<<<<<<< HEAD
        if (unit.movespline->Finalized())
            i_nextMoveTime.Reset(urand(800, 1500));
=======
        if (i_destinationHolder.UpdateTraveller(traveller, diff))
        {
            if (i_destinationHolder.HasArrived())
            {
                // arrived, stop and wait a bit
                unit.ClearUnitState(UNIT_STAT_MOVE);

                i_nextMove = urand(1, MAX_CONF_WAYPOINTS);
                i_nextMoveTime.Reset(urand(100, 1000));
            }
        }
>>>>>>> eb190eb0bc99cf3011ed4d7a30df5898b29aa441
    }
    else
    {
        // waiting for next move
        i_nextMoveTime.Update(diff);
        if (i_nextMoveTime.Passed())
        {
            // start moving
            float x = i_x + 10.0f*(rand_norm_f() - 0.5f);
            float y = i_y + 10.0f*(rand_norm_f() - 0.5f);
            float z = i_z;

            unit.UpdateAllowedPositionZ(x, y, z);

            PathInfo path(&unit);
            path.calculate(x, y, z);
            if (!(path.getPathType() & PATHFIND_NORMAL))
            {
                i_nextMoveTime.Reset(urand(800, 1000));
                return true;
            }

            Movement::MoveSplineInit init(unit);
            init.MovebyPath(path.getPath());
            init.SetWalk(true);
            init.Launch();
        }
    }
    return true;
}

template<class T>
void ConfusedMovementGenerator<T>::Finalize(T &unit)
{
    unit.RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_CONFUSED);
    unit.ClearUnitState(UNIT_STAT_CONFUSED);

    if (unit.GetTypeId() == TYPEID_UNIT && unit.getVictim())
        unit.SetTarget(unit.getVictim()->GetGUID());
}

template void ConfusedMovementGenerator<Player>::Initialize(Player &player);
template void ConfusedMovementGenerator<Creature>::Initialize(Creature &creature);
template void ConfusedMovementGenerator<Player>::Finalize(Player &player);
template void ConfusedMovementGenerator<Creature>::Finalize(Creature &creature);
template void ConfusedMovementGenerator<Player>::Reset(Player &player);
template void ConfusedMovementGenerator<Creature>::Reset(Creature &creature);
template bool ConfusedMovementGenerator<Player>::Update(Player &player, const uint32 diff);
template bool ConfusedMovementGenerator<Creature>::Update(Creature &creature, const uint32 diff);

