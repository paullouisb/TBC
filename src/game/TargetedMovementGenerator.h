/**
 * This code is part of MaNGOS. Contributor & Copyright details are in AUTHORS/THANKS.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MANGOS_TARGETEDMOVEMENTGENERATOR_H
#define MANGOS_TARGETEDMOVEMENTGENERATOR_H

#include "MovementGenerator.h"
#include "FollowerReference.h"

class PathFinder;

class MANGOS_DLL_SPEC TargetedMovementGeneratorBase
{
    public:
        TargetedMovementGeneratorBase(Unit& target) { i_target.link(&target, this); }
        void stopFollowing() { }
    protected:
        FollowerReference i_target;
};

template<class T, typename D>
class MANGOS_DLL_SPEC TargetedMovementGeneratorMedium
    : public MovementGeneratorMedium< T, D >, public TargetedMovementGeneratorBase
{
    protected:
        TargetedMovementGeneratorMedium(Unit& target, float offset, float angle) :
            TargetedMovementGeneratorBase(target),
            i_recheckDistance(0),
            i_offset(offset), i_angle(angle),
            i_targetAngle(0),
            m_speedChanged(false), i_targetReached(false),
            i_path(NULL)
        {
        }
        ~TargetedMovementGeneratorMedium() { delete i_path; }

    public:
        bool Update(T&, const uint32&);

        bool IsReachable() const;

        Unit* GetTarget() const { return i_target.getTarget(); }

        void unitSpeedChanged() { m_speedChanged = true; }

    protected:
        void _setTargetLocation(T&, bool updateDestination);

        ShortTimeTracker i_recheckDistance;
        float i_offset;
        float i_angle;
        float i_targetAngle;
        bool m_speedChanged : 1;
        bool i_targetReached : 1;

        PathFinder* i_path;
};

template<class T>
class MANGOS_DLL_SPEC ChaseMovementGenerator : public TargetedMovementGeneratorMedium<T, ChaseMovementGenerator<T> >
{
    public:
        ChaseMovementGenerator(Unit& target)
            : TargetedMovementGeneratorMedium<T, ChaseMovementGenerator<T> >(target) {}
        ChaseMovementGenerator(Unit& target, float offset, float angle)
            : TargetedMovementGeneratorMedium<T, ChaseMovementGenerator<T> >(target, offset, angle) {}
        ~ChaseMovementGenerator() {}

        MovementGeneratorType GetMovementGeneratorType() const override { return CHASE_MOTION_TYPE; }

        void Initialize(T&);
        void Finalize(T&);
        void Interrupt(T&);
        void Reset(T&);

        static void _clearUnitStateMove(T& u);
        static void _addUnitStateMove(T& u);
        bool EnableWalking() const { return false;}
        bool _lostTarget(T& u) const;
        void _reachTarget(T&);
};

template<class T>
class MANGOS_DLL_SPEC FollowMovementGenerator : public TargetedMovementGeneratorMedium<T, FollowMovementGenerator<T> >
{
    public:
        FollowMovementGenerator(Unit& target)
            : TargetedMovementGeneratorMedium<T, FollowMovementGenerator<T> >(target) {}
        FollowMovementGenerator(Unit& target, float offset, float angle)
            : TargetedMovementGeneratorMedium<T, FollowMovementGenerator<T> >(target, offset, angle) {}
        ~FollowMovementGenerator() {}

        MovementGeneratorType GetMovementGeneratorType() const override { return FOLLOW_MOTION_TYPE; }

        void Initialize(T&);
        void Finalize(T&);
        void Interrupt(T&);
        void Reset(T&);

        static void _clearUnitStateMove(T& u);
        static void _addUnitStateMove(T& u);
        bool EnableWalking() const;
        bool _lostTarget(T&) const { return false; }
        void _reachTarget(T&) {}
    private:
        void _updateSpeed(T& u);
};

#endif
