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
    // set initial position
    unit.GetPosition(i_x, i_y, i_z);

    unit.StopMoving();
    unit.AddUnitState(UNIT_STAT_CONFUSED);
}

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
        unit.AddUnitState(UNIT_STAT_CONFUSED);

        if (unit.movespline->Finalized())
            i_nextMoveTime.Reset(urand(800, 1500));
    }
    else
    {
        // waiting for next move
        i_nextMoveTime.Update(diff);
        if (i_nextMoveTime.Passed())
        {
            // start moving
            unit.AddUnitState(UNIT_STAT_CONFUSED);

            float x = i_x + 10.0f*(rand_norm_f() - 0.5f);
            float y = i_y + 10.0f*(rand_norm_f() - 0.5f);
            float z = i_z;

            unit.UpdateAllowedPositionZ(x, y, z);

            PathInfo path(&unit);
            path.setPathLengthLimit(30.0f);
            path.calculate(x, y, z);
            if (path.getPathType() & PATHFIND_NORMAL)
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

