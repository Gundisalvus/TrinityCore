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

#ifndef TRINITY_TARGETEDMOVEMENTGENERATOR_H
#define TRINITY_TARGETEDMOVEMENTGENERATOR_H

#include "MovementGenerator.h"
#include "FollowerReference.h"
#include "PathInfo.h"
#include "Unit.h"

class TargetedMovementGeneratorBase
{
    public:
        TargetedMovementGeneratorBase(Unit &target) { i_target.link(&target, this); }
        void stopFollowing() { }
    protected:
        FollowerReference i_target;
};

template<class T>
class TargetedMovementGenerator
: public MovementGeneratorMedium< T, TargetedMovementGenerator<T> >, public TargetedMovementGeneratorBase
{
    public:
        TargetedMovementGenerator(Unit &target, float offset = 0, float angle = 0);
        ~TargetedMovementGenerator() {}

        void Initialize(T &);
        void Finalize(T &);
        void Reset(T &);
        bool Update(T &, const uint32);
        MovementGeneratorType GetMovementGeneratorType() { return TARGETED_MOTION_TYPE; }

        void MovementInform(T &);

        bool IsReachable() const
        {
            return (i_path) ? (i_path->getPathType() & PATHFIND_NORMAL) : true;
        }

        Unit* GetTarget() const;

        void unitSpeedChanged() { i_recalculateTravel=true; }
        bool EnableWalking() const;
    private:

        bool _setTargetLocation(T &);

        TimeTracker i_recheckDistance;
        float i_offset;
        float i_angle;
        bool i_recalculateTravel : 1;
        bool i_targetReached : 1;
        float i_targetX, i_targetY, i_targetZ;
};
#endif

