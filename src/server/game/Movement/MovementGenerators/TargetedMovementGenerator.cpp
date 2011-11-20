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

#include "ByteBuffer.h"
#include "TargetedMovementGenerator.h"
#include "Errors.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "World.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"

#define SMALL_ALPHA 0.05f

#include <cmath>

template<class T>
TargetedMovementGenerator<T>::TargetedMovementGenerator(Unit &target, float offset, float angle)
: TargetedMovementGeneratorBase(target)
, i_offset(offset), i_angle(angle), i_recalculateTravel(false)
{
    target.GetPosition(i_targetX, i_targetY, i_targetZ);
}

template<class T>
bool
TargetedMovementGenerator<T>::_setTargetLocation(T &owner)
{
    if (!i_target.isValid() || !i_target->IsInWorld())
        return false;

    if (owner.HasUnitState(UNIT_STAT_ROOT | UNIT_STAT_STUNNED | UNIT_STAT_DISTRACTED))
        return false;

    float x, y, z;
    if (!owner.movespline->Finalized()
    {
        if (i_destinationHolder.HasArrived())
        {
            // prevent redundant micro-movement
            if (!i_offset)
            {
                if (i_target->IsWithinMeleeRange(&owner))
                    return false;
            }
            else if (!i_angle && !owner.HasUnitState(UNIT_STAT_FOLLOW))
            {
                if (i_target->IsWithinDistInMap(&owner, i_offset))
                    return false;
            }
            else
            {
                if (i_target->IsWithinDistInMap(&owner, i_offset + 1.0f))
                    return false;
            }
        }
        else
        {
            bool stop = false;
            if (!i_offset)
            {
                if (i_target->IsWithinMeleeRange(&owner, 0))
                    stop = true;
            }
            else if (!i_angle && !owner.HasUnitState(UNIT_STAT_FOLLOW))
            {
                if (i_target->IsWithinDist(&owner, i_offset * 0.8f))
                    stop = true;
            }

            if (stop)
            {
                owner.GetPosition(x, y, z);
                i_destinationHolder.SetDestination(traveller, x, y, z);
                i_destinationHolder.StartTravel(traveller, false);
                owner.StopMoving();
                return false;
            }
        }

        if (i_target->GetExactDistSq(i_targetX, i_targetY, i_targetZ) < 0.01f)
            return false;
    }

    if (!i_offset)
    {
        // to nearest random contact position
        i_target->GetRandomContactPoint(&owner, x, y, z, 0, MELEE_RANGE - 0.5f);
    }
    else if (!i_angle && !owner.HasUnitState(UNIT_STAT_FOLLOW))
    {
        // caster chase
        i_target->GetContactPoint(&owner, x, y, z, i_offset * urand(80, 95) * 0.01f);
    }
    else
    {
        // to at i_offset distance from target and i_angle from target facing
        i_target->GetClosePoint(x, y, z, owner.GetObjectSize(), i_offset, i_angle);
    }

    /*
        We MUST not check the distance difference and avoid setting the new location for smaller distances.
        By that we risk having far too many GetContactPoint() calls freezing the whole system.
        In TargetedMovementGenerator<T>::Update() we check the distance to the target and at
        some range we calculate a new position. The calculation takes some processor cycles due to vmaps.
        If the distance to the target it too large to ignore,
        but the distance to the new contact point is short enough to be ignored,
        we will calculate a new contact point each update loop, but will never move to it.
        The system will freeze.
        ralf

        //We don't update Mob Movement, if the difference between New destination and last destination is < BothObjectSize
        float  bothObjectSize = i_target->GetObjectSize() + owner.GetObjectSize() + CONTACT_DISTANCE;
        if (i_destinationHolder.HasDestination() && i_destinationHolder.GetDestinationDiff(x, y, z) < bothObjectSize)
            return;
    */

    if (!i_path)
        i_path = new PathInfo(&owner);

    // allow pets following their master to cheat while generating paths
    bool forceDest = (owner.GetTypeId() == TYPEID_UNIT && ((Creature*)&owner)->IsPet()
        && owner.hasUnitState(UNIT_STAT_FOLLOW));
    i_path->calculate(x, y, z, forceDest);
    if (i_path->getPathType() & PATHFIND_NOPATH)
        return;

    owner.AddUnitState(UNIT_STAT_CHASE);
    i_targetReached = false;
    i_recalculateTravel = false;

    Movement::MoveSplineInit init(owner);
    init.MovebyPath(i_path->getPath());
    init.SetWalk(((D*)this)->EnableWalking());
    init.Launch();
    return true;
}

template<class T>
void
TargetedMovementGenerator<T>::Initialize(T &owner)
{
    if (owner.isInCombat())
        owner.RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);

    _setTargetLocation(owner);
}

template<class T>
void
TargetedMovementGenerator<T>::Finalize(T &owner)
{
    owner.ClearUnitState(UNIT_STAT_CHASE);
}

template<class T>
void
TargetedMovementGenerator<T>::Reset(T &owner)
{
    Initialize(owner);
}

template<class T>
bool
TargetedMovementGenerator<T>::Update(T &owner, const uint32 time_diff)
{
    if (!i_target.isValid() || !i_target->IsInWorld())
        return false;

    if (!&owner || !owner.isAlive())
        return true;

    if (owner.HasUnitState(UNIT_STAT_ROOT | UNIT_STAT_STUNNED | UNIT_STAT_FLEEING | UNIT_STAT_DISTRACTED))
        return true;

    // prevent movement while casting spells with cast time or channel time
    if (owner.HasUnitState(UNIT_STAT_CASTING))
    {
        if (!owner.IsStopped())
            owner.StopMoving();
        return true;
    }

    // prevent crash after creature killed pet
    if (!owner.HasUnitState(UNIT_STAT_FOLLOW) && owner.getVictim() != i_target.getTarget())
        return true;

    i_recheckDistance.Update(time_diff);
    if (i_recheckDistance.Passed())
    {
        float allowed_dist = i_target->GetObjectSize() + owner.GetObjectSize()
            + sWorld->getRate(RATE_TARGET_POS_RECALCULATION_RANGE);
        float dist = (owner.movespline->FinalDestination() -
            G3D::Vector3(i_target->GetPositionX(),i_target->GetPositionY(),i_target->GetPositionZ())).squaredLength();
        if (dist >= allowed_dist * allowed_dist)
            _setTargetLocation(owner);
    }

    if (owner.movespline->Finalized())
    {
        if (i_angle == 0.f && !owner.HasInArc(0.01f, i_target.getTarget()))
            owner.SetInFront(i_target.getTarget());
    }

    if (!i_targetReached)
    {
        i_targetReached = true;
        if (owner.IsWithinMeleeRange(i_target.getTarget()) && !owner.HasUnitState(UNIT_STAT_FOLLOW))
            owner.Attack(i_target.getTarget(), true);
    }
    else
    {
        if (i_recalculateTravel)
            _setTargetLocation(owner);
    }

    // Implemented for PetAI to handle resetting flags when pet owner reached
    if (i_destinationHolder.HasArrived())
        MovementInform(owner);

    return true;
}

template<class T>
Unit*
TargetedMovementGenerator<T>::GetTarget() const
{
    return i_target.getTarget();
}

template<class T>
void TargetedMovementGenerator<T>::MovementInform(T & /*unit*/)
{
}

template <> void TargetedMovementGenerator<Creature>::MovementInform(Creature &unit)
{
    // Pass back the GUIDLow of the target. If it is pet's owner then PetAI will handle
    unit.AI()->MovementInform(TARGETED_MOTION_TYPE, i_target.getTarget()->GetGUIDLow());
}

template void TargetedMovementGenerator<Player>::MovementInform(Player&); // Not implemented for players
template TargetedMovementGenerator<Player>::TargetedMovementGenerator(Unit &target, float offset, float angle);
template TargetedMovementGenerator<Creature>::TargetedMovementGenerator(Unit &target, float offset, float angle);
template bool TargetedMovementGenerator<Player>::_setTargetLocation(Player &);
template bool TargetedMovementGenerator<Creature>::_setTargetLocation(Creature &);
template void TargetedMovementGenerator<Player>::Initialize(Player &);
template void TargetedMovementGenerator<Creature>::Initialize(Creature &);
template void TargetedMovementGenerator<Player>::Finalize(Player &);
template void TargetedMovementGenerator<Creature>::Finalize(Creature &);
template void TargetedMovementGenerator<Player>::Reset(Player &);
template void TargetedMovementGenerator<Creature>::Reset(Creature &);
template bool TargetedMovementGenerator<Player>::Update(Player &, const uint32);
template bool TargetedMovementGenerator<Creature>::Update(Creature &, const uint32);
template Unit* TargetedMovementGenerator<Player>::GetTarget() const;
template Unit* TargetedMovementGenerator<Creature>::GetTarget() const;

