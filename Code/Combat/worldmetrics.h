/*
**	OpenW3D
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/worldmetrics.h                        $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	How big is a soldier, what can it climb, how wide is a tank.                                *
 *                                                                                             *
 *	Roadmap Section 52B, Phase 63.  Nothing in the tree answered those questions in one         *
 *	place, so every consumer that needed a dimension re-derived it, and the numbers drifted:    *
 *	STEP_HEIGHT is a file-scope constant in phys3.cpp *and* a per-definition value on           *
 *	Phys3DefClass; the soldier's eye height and its crouch drop are literals inside             *
 *	SoldierGameObj; maximum climbable slope is implicit in Phys3DefClass::SlideAngle and        *
 *	written down nowhere.  A generator handed that situation invents its own numbers, which     *
 *	is exactly how terrain ends up out of scale.                                                *
 *                                                                                             *
 *	This service reads the definitions that already own those facts.  It does not restate       *
 *	them as new constants, and where a value is genuinely implicit in the physics --            *
 *	maximum climbable slope is the clear case -- it is computed once from the locomotor's       *
 *	own parameters and cached rather than typed in.                                             *
 *                                                                                             *
 *	It answers per definition and per locomotion class.  The class-level answer is the most     *
 *	restrictive member in play (the tallest vehicle, the widest vehicle, the infantry           *
 *	envelope that fits everywhere), which is the form terrain shaping and route validation      *
 *	actually need: a corridor has to admit the worst case, not the average one.                 *
 *                                                                                             *
 *	Two honest limits, both recorded rather than papered over:                                  *
 *                                                                                             *
 *	  - Extents come from the model's WORLDBOX sub-object, which is where Phys3Class itself     *
 *	    gets its collision box.  That needs the model resolvable through the asset manager.     *
 *	    When it is not -- a dedicated server with no art, a definition naming a model that      *
 *	    does not exist -- the physics-derived half of the answer is still filled in, the        *
 *	    extents fall back to the engine's own Phys3Class defaults, and Has_Extents() says so.   *
 *	    A missing model degrades the answer; it never blocks it and never throws.               *
 *                                                                                             *
 *	  - Prone is not a posture this engine has.  HumanStateClass has UPRIGHT and a crouch       *
 *	    flag and no third stance, so Get_Prone_Height() returns the crouch height and           *
 *	    Is_Posture_Supported(POSTURE_PRONE) returns false.  Callers that care can ask.          *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef WORLDMETRICS_H
#define WORLDMETRICS_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#ifndef VECTOR3_H
#include "vector3.h"
#endif

class PhysDefClass;


/*
**	What a thing moves like.  This is the axis the service answers along, because a
**	corridor that admits every soldier is a different number from one that admits every
**	tank, and the caller almost always knows which it is asking about.
*/
enum WorldLocomotionClassType
{
	LOCOMOTION_INFANTRY	= 0,
	LOCOMOTION_WHEELED,
	LOCOMOTION_TRACKED,
	LOCOMOTION_FLYING,

	LOCOMOTION_COUNT,
	LOCOMOTION_INVALID	= -1,
};


/*
**	The stances the service will answer a height for.
*/
enum WorldPostureType
{
	POSTURE_STAND	= 0,
	POSTURE_CROUCH,
	POSTURE_PRONE,

	POSTURE_COUNT,
};


/*
**	One complete answer.  Every field is in metres or radians, and every one of them came
**	from a definition or was computed from one -- see the per-field notes in worldmetrics.cpp
**	for which.
*/
struct WorldMetricsStruct
{
	WorldMetricsStruct(void) { Reset(); }

	void	Reset(void);

	//	Body.
	float		StandHeight;			// full height of the collision envelope
	float		CrouchHeight;			// stand height less the engine's own crouch drop
	float		ProneHeight;			// == CrouchHeight; see HasProne
	float		CollisionRadius;		// largest horizontal half-extent
	Vector3	Extents;					// object-space half-extents of the collision envelope
	Vector3	ExtentsCenter;			// object-space centre of that envelope

	//	Locomotion.
	float		StepHeight;				// what it will hop over without jumping
	float		MaxClimbableSlope;	// radians; steeper than this and it slides back
	float		CorridorWidth;			// narrowest gap it will fit through
	float		OverheadClearance;	// lowest ceiling it will pass under
	float		TurningRadius;			// 0 means it turns on the spot; < 0 means unknown

	//	Provenance.  A caller that must not guess can tell what was measured and what
	//	fell back.
	bool		Valid;					// anything at all was resolved
	bool		HasExtents;				// the model's WORLDBOX was found, so extents are real
	bool		HasTurningRadius;		// the locomotor states a steering limit
	bool		HasProne;				// false everywhere today; see the file header
};


class WorldMetricsClass
{
public:

	/*
	**	Drop every cached answer.  Called when the definition database changes underneath
	**	the service -- a level load, or a live descriptor reload.
	*/
	static void		Reset(void);

	/*
	**	One definition's metrics, by definition id.  Cached after the first call, because
	**	resolving extents costs a model load and callers ask in loops.
	*/
	static bool		Get_Metrics(int definition_id,WorldMetricsStruct * out);

	/*
	**	The same, straight from a physics definition the caller already has.
	*/
	static bool		Get_Metrics(PhysDefClass * def,WorldMetricsStruct * out);

	/*
	**	The most restrictive member of a locomotion class: the tallest, the widest, the
	**	least able to climb, the one needing the most clearance.  This is the answer terrain
	**	shaping and route validation want, since a route has to admit the worst case.
	**
	**	Returns false when no definition of that class is loaded, which is the honest answer
	**	on a level with no vehicles rather than a zero that reads as "fits anywhere".
	*/
	static bool		Get_Class_Metrics(WorldLocomotionClassType loco,WorldMetricsStruct * out);

	/*
	**	Single-question forms over the above, for the callers that want one number.
	*/
	static float	Get_Widest(WorldLocomotionClassType loco);
	static float	Get_Tallest(WorldLocomotionClassType loco);
	static float	Get_Posture_Height(WorldLocomotionClassType loco,WorldPostureType posture);

	static bool		Is_Posture_Supported(WorldPostureType posture);

	/*
	**	Which locomotion class a physics definition belongs to, by asking the definition
	**	what type it is rather than by keeping a table of names.
	*/
	static WorldLocomotionClassType	Classify(PhysDefClass * def);

	/*
	**	Building footprint, from the Section 39 descriptor that already owns it.  Radius is
	**	the circumscribing radius of the descriptor's width/depth rectangle, which is what a
	**	placement or clearance test wants; height comes from the model when it resolves.
	*/
	static bool		Get_Building_Footprint(const char * descriptor_name,float * radius_out,float * height_out);

	/*
	**	The pure derivations, exposed because they are the part worth testing without a
	**	device and because a caller with its own numbers (a generator sizing a corridor it
	**	has not built yet) needs them before any definition exists.
	*/

	//	Phys3DefClass::SlideAngle is the angle at which an object slides off.  The steepest
	//	slope it can hold is therefore everything below that, and the service reports the
	//	angle itself rather than the tangent, because callers compare it against
	//	HeightfieldClass::Sample_Slope, which is also in radians.
	static float	Compute_Max_Climbable_Slope(float slide_angle_radians);

	//	A wheeled locomotor's tightest turn.  Bicycle model: the wheelbase over the tangent
	//	of the steering limit.  A zero or negative steering angle means it cannot steer, so
	//	the radius is unbounded and the caller is told by the return, not by a huge number.
	static float	Compute_Turning_Radius(float wheelbase,float max_steering_angle_radians);

	//	The narrowest gap a thing of this half-width will pass, with the margin the physics
	//	actually needs to avoid grinding along both walls at once.
	static float	Compute_Corridor_Width(float collision_radius);

	//	The lowest ceiling it will pass under: its own height plus the step it may be riding
	//	up at the time.
	static float	Compute_Overhead_Clearance(float stand_height,float step_height);

	/*
	**	The soldier posture constants, owned here so that the two literals that used to sit
	**	in SoldierGameObj have one home.  Both are the engine's own existing values.
	*/
	static float	Get_Soldier_Eye_Height(void);
	static float	Get_Soldier_Crouch_Drop(void);

private:

	static bool		Resolve(PhysDefClass * def,WorldMetricsStruct * out);
	static bool		Resolve_Extents(PhysDefClass * def,WorldMetricsStruct * out);
	static void		Accumulate_Worst(const WorldMetricsStruct & in,WorldMetricsStruct * worst);
};

#endif	// WORLDMETRICS_H
