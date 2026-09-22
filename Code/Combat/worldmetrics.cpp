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
 *                     $Archive:: /Commando/Code/Combat/worldmetrics.cpp                      $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "worldmetrics.h"

#include "buildingdescriptor.h"
#include "buildingdescriptormanager.h"

#include "assetmgr.h"
#include "rendobj.h"
#include "aabox.h"

#include "definition.h"
#include "definitionmgr.h"
#include "definitionclassids.h"

#include "phys.h"
#include "phys3.h"
#include "movephys.h"
#include "vehiclephys.h"
#include "wheelvehicle.h"

#include "wwmath.h"
#include "wwdebug.h"

#include <string.h>


/*
**	The engine's own numbers, gathered.  Not one of these is invented here: each is the
**	value the engine already used, moved to the one place that now owns it.
*/

//	Phys3Class's own fallback collision envelope, phys3.cpp:460.  Used when a definition's
//	model has no WORLDBOX and the physics would have used this box anyway.
static const Vector3	DEFAULT_ENVELOPE_EXTENT(1.0f,1.0f,1.0f);
static const Vector3	DEFAULT_ENVELOPE_CENTER(0.0f,0.0f,1.0f);

//	SoldierGameObj::Get_Weapon_Height, soldier.cpp.  Eye level, and how far it drops when
//	the soldier crouches.  Those two literals lived in that function and nowhere else, so
//	anything else wanting to know how tall a crouched soldier is had to guess.
static const float	SOLDIER_EYE_HEIGHT		= 1.62f;
static const float	SOLDIER_CROUCH_DROP		= 0.56f;

//	Roadmap Section 3A.1.  See Get_World_Units_Per_Meter's own header comment for why 1:1 is
//	the resolved value rather than an unset placeholder.
static const float	WORLD_UNITS_PER_METER	= 1.0f;

//	A body has to fit between two walls without touching both, or the physics grinds it to a
//	halt in a gap it nominally fits.  Half a step height of air either side is the margin
//	Phys3Class's own stepping already assumes it has.
static const float	CORRIDOR_MARGIN			= 0.25f;

//	Vehicles have no SlideAngle: they are rigid bodies, and what stops them on a slope is
//	tyre friction, not a slide test.  A driven wheel holds until gravity along the slope
//	beats the friction the tyre can produce, which is atan of the friction coefficient --
//	and VehiclePhysDefClass::TractionMultiplier is exactly the term that scales it.
static const float	MIN_TRACTION_MULTIPLIER	= 0.05f;

//	What a vehicle whose steering angle is unusable reports.  Negative, so a caller reading
//	the number alone can still tell it apart from "turns on the spot", which is zero.
static const float	TURNING_RADIUS_UNKNOWN	= -1.0f;


/*
**	Cache.  Resolving extents costs a model load, and placement validation asks in loops.
**	Keyed by definition id; dropped wholesale on Reset, because the thing that invalidates
**	one entry -- the definition database being replaced -- invalidates all of them.
*/
struct MetricsCacheEntryStruct
{
	int						DefinitionID;
	WorldMetricsStruct	Metrics;

	//	DynamicVectorClass::ID compares elements, so the entry needs one.  Identity here is
	//	the key alone: two entries for the same definition are the same entry whatever the
	//	cached answer happens to say.
	bool	operator == (const MetricsCacheEntryStruct & that) const	{ return DefinitionID == that.DefinitionID; }
	bool	operator != (const MetricsCacheEntryStruct & that) const	{ return DefinitionID != that.DefinitionID; }
};

static DynamicVectorClass<MetricsCacheEntryStruct>	_MetricsCache;

//	Per-class worst cases, built on demand from every loaded definition and cached the same
//	way.  _ClassCacheValid is one flag for all of them: they are all built in one sweep.
static WorldMetricsStruct	_ClassMetrics[LOCOMOTION_COUNT];
static bool						_ClassMetricsResolved[LOCOMOTION_COUNT];
static bool						_ClassCacheValid = false;


/***********************************************************************************************
 * WorldMetricsStruct::Reset --                                                                *
 *=============================================================================================*/
void WorldMetricsStruct::Reset(void)
{
	StandHeight			= 0.0f;
	CrouchHeight		= 0.0f;
	ProneHeight			= 0.0f;
	CollisionRadius	= 0.0f;
	Extents.Set(0.0f,0.0f,0.0f);
	ExtentsCenter.Set(0.0f,0.0f,0.0f);

	StepHeight			= 0.0f;
	MaxClimbableSlope	= 0.0f;
	CorridorWidth		= 0.0f;
	OverheadClearance	= 0.0f;
	TurningRadius		= TURNING_RADIUS_UNKNOWN;

	Valid					= false;
	HasExtents			= false;
	HasTurningRadius	= false;
	HasProne				= false;
}


/***********************************************************************************************
 * WorldMetricsClass::Reset -- drop every cached answer                                        *
 *=============================================================================================*/
void WorldMetricsClass::Reset(void)
{
	_MetricsCache.Delete_All();

	for (int i = 0; i < LOCOMOTION_COUNT; i++) {
		_ClassMetrics[i].Reset();
		_ClassMetricsResolved[i] = false;
	}
	_ClassCacheValid = false;
}


/***********************************************************************************************
 * WorldMetricsClass::Get_Soldier_Eye_Height --                                                *
 * WorldMetricsClass::Get_Soldier_Crouch_Drop --                                               *
 *=============================================================================================*/
float WorldMetricsClass::Get_Soldier_Eye_Height(void)
{
	return SOLDIER_EYE_HEIGHT;
}

float WorldMetricsClass::Get_Soldier_Crouch_Drop(void)
{
	return SOLDIER_CROUCH_DROP;
}


float WorldMetricsClass::Get_World_Units_Per_Meter(void)
{
	return WORLD_UNITS_PER_METER;
}


/***********************************************************************************************
 * WorldMetricsClass::Compute_Max_Climbable_Slope --                                           *
 *                                                                                             *
 * Phys3DefClass::SlideAngle is the angle at which the object slides off, so everything below  *
 * it is climbable and the answer is the angle itself.  Clamped into a sane quadrant because   *
 * a definition may carry anything, and a slope of more than vertical is not a slope.          *
 *=============================================================================================*/
float WorldMetricsClass::Compute_Max_Climbable_Slope(float slide_angle_radians)
{
	if (slide_angle_radians <= 0.0f) {
		return 0.0f;
	}
	if (slide_angle_radians > DEG_TO_RADF(89.0f)) {
		return DEG_TO_RADF(89.0f);
	}
	return slide_angle_radians;
}


/***********************************************************************************************
 * WorldMetricsClass::Compute_Turning_Radius --                                                *
 *                                                                                             *
 * Bicycle model: wheelbase over the tangent of the steering limit.  Returns the unknown       *
 * marker rather than a huge number when the locomotor cannot steer, so a caller comparing     *
 * radii does not silently treat "cannot turn" as "turns very wide".                           *
 *=============================================================================================*/
float WorldMetricsClass::Compute_Turning_Radius(float wheelbase,float max_steering_angle_radians)
{
	if ((wheelbase <= 0.0f) || (max_steering_angle_radians <= 0.0f)) {
		return TURNING_RADIUS_UNKNOWN;
	}

	//	At or past a right angle the wheel is across the direction of travel and the model
	//	stops meaning anything; that is a vehicle turning on the spot.
	if (max_steering_angle_radians >= DEG_TO_RADF(89.0f)) {
		return 0.0f;
	}

	float tangent = tanf(max_steering_angle_radians);
	if (tangent <= WWMATH_EPSILON) {
		return TURNING_RADIUS_UNKNOWN;
	}

	return wheelbase / tangent;
}


/***********************************************************************************************
 * WorldMetricsClass::Compute_Corridor_Width --                                                *
 *=============================================================================================*/
float WorldMetricsClass::Compute_Corridor_Width(float collision_radius)
{
	if (collision_radius <= 0.0f) {
		return 0.0f;
	}
	return (collision_radius * 2.0f) + (CORRIDOR_MARGIN * 2.0f);
}


/***********************************************************************************************
 * WorldMetricsClass::Compute_Overhead_Clearance --                                            *
 *                                                                                             *
 * Its own height plus the step it may be riding up as it passes under.  A doorway that only   *
 * clears a soldier standing on flat ground is a doorway it hits its head on at the threshold. *
 *=============================================================================================*/
float WorldMetricsClass::Compute_Overhead_Clearance(float stand_height,float step_height)
{
	if (stand_height <= 0.0f) {
		return 0.0f;
	}
	float step = (step_height > 0.0f) ? step_height : 0.0f;
	return stand_height + step;
}


/***********************************************************************************************
 * WorldMetricsClass::Is_Posture_Supported --                                                  *
 *=============================================================================================*/
bool WorldMetricsClass::Is_Posture_Supported(WorldPostureType posture)
{
	//	HumanStateClass has UPRIGHT and a CROUCHED_FLAG.  There is no third stance in this
	//	engine, and inventing a prone height would be inventing a number, which is the exact
	//	failure this service exists to stop.
	return (posture == POSTURE_STAND) || (posture == POSTURE_CROUCH);
}


/***********************************************************************************************
 * WorldMetricsClass::Classify -- which locomotion class a physics definition belongs to       *
 *                                                                                             *
 * By asking the definition what it is, not by matching model names.  Order matters: the       *
 * vehicle types are all VehiclePhysDef underneath, so the specific ones are tested first.     *
 *=============================================================================================*/
WorldLocomotionClassType WorldMetricsClass::Classify(PhysDefClass * def)
{
	if (def == nullptr) {
		return LOCOMOTION_INVALID;
	}

	if (def->Is_Type("VTOLVehicleDef")) {
		return LOCOMOTION_FLYING;
	}
	if (def->Is_Type("TrackedVehicleDef")) {
		return LOCOMOTION_TRACKED;
	}
	if (def->Is_Type("WheeledVehicleDef") || def->Is_Type("MotorcycleDef")) {
		return LOCOMOTION_WHEELED;
	}

	//	Phys3 is the walker base and HumanPhys is the soldier on top of it.  Anything that
	//	walks is infantry as far as a corridor is concerned.
	if (def->Is_Type("Phys3Def") || def->Is_Type("HumanPhysDef")) {
		return LOCOMOTION_INFANTRY;
	}

	return LOCOMOTION_INVALID;
}


/***********************************************************************************************
 * WorldMetricsClass::Resolve_Extents -- the envelope, from the model's WORLDBOX               *
 *                                                                                             *
 * This is where Phys3Class itself gets its collision box (phys3.cpp, Set_Model), so asking    *
 * the same question of the definition gives the same answer without needing an instance.      *
 *                                                                                             *
 * Returns false when the model cannot be resolved.  That is not an error: a dedicated server  *
 * has no art, and a definition may name a model nobody has made yet.  The caller keeps the    *
 * physics-derived half of the answer and the engine's own default envelope.                   *
 *=============================================================================================*/
bool WorldMetricsClass::Resolve_Extents(PhysDefClass * def,WorldMetricsStruct * out)
{
	WWASSERT(def != nullptr);
	WWASSERT(out != nullptr);

	out->Extents		= DEFAULT_ENVELOPE_EXTENT;
	out->ExtentsCenter	= DEFAULT_ENVELOPE_CENTER;
	out->HasExtents		= false;

	WW3DAssetManager * assets = WW3DAssetManager::Get_Instance();
	if (assets == nullptr) {
		return false;
	}

	const StringClass & model_name = def->Get_Model_Name();
	if (model_name.Is_Empty()) {
		return false;
	}

	RenderObjClass * model = assets->Create_Render_Obj(model_name.Peek_Buffer());
	if (model == nullptr) {
		return false;
	}

	//	A DistLOD keeps its geometry one level down, the same special case Phys3Class makes.
	RenderObjClass * box = nullptr;
	if (model->Class_ID() == RenderObjClass::CLASSID_DISTLOD) {
		RenderObjClass * lod0 = model->Get_Sub_Object(0);
		if (lod0 != nullptr) {
			box = lod0->Get_Sub_Object_By_Name("WORLDBOX");
			lod0->Release_Ref();
		}
	} else {
		box = model->Get_Sub_Object_By_Name("WORLDBOX");
	}

	bool found = false;

	if (box != nullptr) {
		//	Identity transform first, so the box we read is the object-space one rather than
		//	wherever the freshly created instance happens to be sitting.
		Matrix3D old_transform = model->Get_Transform();
		model->Set_Transform(Matrix3D(1));

		AABoxClass world_box = box->Get_Bounding_Box();
		out->Extents		= world_box.Extent;
		out->ExtentsCenter	= world_box.Center;
		out->HasExtents		= true;
		found = true;

		model->Set_Transform(old_transform);
		box->Release_Ref();

	} else {
		//	No WORLDBOX: the whole model's bounds are a worse answer than the physics
		//	default for a collision envelope, but a much better one than nothing for a
		//	clearance question, so take it and say it is not a real collision box.
		AABoxClass model_box = model->Get_Bounding_Box();
		if ((model_box.Extent.X > 0.0f) && (model_box.Extent.Z > 0.0f)) {
			out->Extents		= model_box.Extent;
			out->ExtentsCenter	= model_box.Center;
		}
	}

	model->Release_Ref();
	return found;
}


/***********************************************************************************************
 * WorldMetricsClass::Resolve -- build one definition's complete answer                        *
 *=============================================================================================*/
bool WorldMetricsClass::Resolve(PhysDefClass * def,WorldMetricsStruct * out)
{
	WWASSERT(out != nullptr);
	out->Reset();

	if (def == nullptr) {
		return false;
	}

	WorldLocomotionClassType loco = Classify(def);
	if (loco == LOCOMOTION_INVALID) {
		return false;
	}

	/*
	**	Envelope first, because height and radius both come out of it.
	*/
	Resolve_Extents(def,out);

	out->StandHeight = out->Extents.Z * 2.0f;
	out->CollisionRadius = (out->Extents.X > out->Extents.Y) ? out->Extents.X : out->Extents.Y;

	/*
	**	Postures.  Only infantry has any, and only one beyond standing.
	*/
	out->HasProne = false;
	if (loco == LOCOMOTION_INFANTRY) {
		out->CrouchHeight = out->StandHeight - SOLDIER_CROUCH_DROP;
		if (out->CrouchHeight < 0.0f) {
			out->CrouchHeight = 0.0f;
		}
	} else {
		out->CrouchHeight = out->StandHeight;
	}
	out->ProneHeight = out->CrouchHeight;

	/*
	**	Locomotion.  Where the definition states a value, it is read; where it does not, it
	**	is computed from what the definition does state.
	*/
	if (def->Is_Type("Phys3Def")) {

		Phys3DefClass * p3 = (Phys3DefClass *)def;
		out->StepHeight = p3->Get_Step_Height();
		out->MaxClimbableSlope = Compute_Max_Climbable_Slope(p3->Get_Slide_Angle());
		out->TurningRadius = 0.0f;					// walkers turn on the spot
		out->HasTurningRadius = true;

	} else if (def->Is_Type("VehiclePhysDef")) {

		VehiclePhysDefClass * vp = (VehiclePhysDefClass *)def;

		//	A wheel climbs whatever its suspension can swallow before the chassis grounds
		//	out, and the spring's own travel is the engine's statement of that.
		out->StepHeight = vp->Get_Spring_Length();

		//	Friction-limited slope: see MIN_TRACTION_MULTIPLIER above.
		float traction = vp->Get_Traction_Multiplier();
		if (traction < MIN_TRACTION_MULTIPLIER) {
			traction = MIN_TRACTION_MULTIPLIER;
		}
		out->MaxClimbableSlope = Compute_Max_Climbable_Slope(atanf(traction));

		if (loco == LOCOMOTION_TRACKED) {
			//	Tracks counter-rotate; a tracked vehicle pivots about its own centre.
			out->TurningRadius = 0.0f;
			out->HasTurningRadius = true;

		} else if (loco == LOCOMOTION_WHEELED) {
			//	Wheelbase is not stated anywhere as a number, but the envelope's length is
			//	the axle span plus overhangs, and it is the only measurement of the vehicle
			//	the definitions carry.  Using it makes the radius slightly conservative,
			//	which is the safe direction for a clearance question.
			float wheelbase = out->Extents.Y * 2.0f;
			WheeledVehicleDefClass * wv = (WheeledVehicleDefClass *)def;
			out->TurningRadius = Compute_Turning_Radius(wheelbase,wv->Get_Max_Steering_Angle());
			out->HasTurningRadius = (out->TurningRadius >= 0.0f);

		} else {
			//	VTOL.  It has no ground turning constraint at all.
			out->TurningRadius = 0.0f;
			out->HasTurningRadius = true;
			out->MaxClimbableSlope = DEG_TO_RADF(89.0f);
			out->StepHeight = 0.0f;
		}

	} else {
		return false;
	}

	out->CorridorWidth = Compute_Corridor_Width(out->CollisionRadius);
	out->OverheadClearance = Compute_Overhead_Clearance(out->StandHeight,out->StepHeight);

	out->Valid = true;
	return true;
}


/***********************************************************************************************
 * WorldMetricsClass::Get_Metrics --                                                           *
 *=============================================================================================*/
bool WorldMetricsClass::Get_Metrics(PhysDefClass * def,WorldMetricsStruct * out)
{
	if ((def == nullptr) || (out == nullptr)) {
		return false;
	}
	return Get_Metrics((int)def->Get_ID(),out);
}


bool WorldMetricsClass::Get_Metrics(int definition_id,WorldMetricsStruct * out)
{
	if (out == nullptr) {
		return false;
	}

	for (int i = 0; i < _MetricsCache.Count(); i++) {
		if (_MetricsCache[i].DefinitionID == definition_id) {
			*out = _MetricsCache[i].Metrics;
			return out->Valid;
		}
	}

	DefinitionClass * def = DefinitionMgrClass::Find_Definition((uint32)definition_id,false);

	WorldMetricsStruct metrics;
	bool ok = false;
	if (def != nullptr) {
		ok = Resolve((PhysDefClass *)def,&metrics);
	}
	if (!ok) {
		metrics.Reset();
	}

	//	Cached either way.  A definition that has no answer today still has no answer next
	//	frame, and re-deciding that costs a model load every time it is asked.
	MetricsCacheEntryStruct entry;
	entry.DefinitionID = definition_id;
	entry.Metrics = metrics;
	_MetricsCache.Add(entry);

	*out = metrics;
	return ok;
}


/***********************************************************************************************
 * WorldMetricsClass::Accumulate_Worst -- fold one definition into a class's worst case        *
 *                                                                                             *
 * "Worst" is per field and in the direction that constrains: the largest body, the narrowest  *
 * step, the shallowest climbable slope, the widest corridor needed, the tallest clearance.    *
 * A route that satisfies this answer satisfies every member of the class.                     *
 *=============================================================================================*/
void WorldMetricsClass::Accumulate_Worst(const WorldMetricsStruct & in,WorldMetricsStruct * worst)
{
	WWASSERT(worst != nullptr);

	if (!in.Valid) {
		return;
	}

	if (!worst->Valid) {
		*worst = in;
		return;
	}

	if (in.StandHeight > worst->StandHeight)				{ worst->StandHeight = in.StandHeight; }
	if (in.CrouchHeight > worst->CrouchHeight)			{ worst->CrouchHeight = in.CrouchHeight; }
	if (in.ProneHeight > worst->ProneHeight)				{ worst->ProneHeight = in.ProneHeight; }
	if (in.CollisionRadius > worst->CollisionRadius)	{ worst->CollisionRadius = in.CollisionRadius; }
	if (in.CorridorWidth > worst->CorridorWidth)			{ worst->CorridorWidth = in.CorridorWidth; }
	if (in.OverheadClearance > worst->OverheadClearance){ worst->OverheadClearance = in.OverheadClearance; }

	if (in.Extents.X > worst->Extents.X)					{ worst->Extents.X = in.Extents.X; }
	if (in.Extents.Y > worst->Extents.Y)					{ worst->Extents.Y = in.Extents.Y; }
	if (in.Extents.Z > worst->Extents.Z)					{ worst->Extents.Z = in.Extents.Z; }

	//	The constraining step height is the smallest: a kerb every vehicle in the class can
	//	mount is one the least capable of them can mount.
	if (in.StepHeight < worst->StepHeight)					{ worst->StepHeight = in.StepHeight; }
	if (in.MaxClimbableSlope < worst->MaxClimbableSlope){ worst->MaxClimbableSlope = in.MaxClimbableSlope; }

	//	Likewise the widest turn.  Unknown stays unknown rather than winning the comparison.
	if (in.HasTurningRadius) {
		if (!worst->HasTurningRadius || (in.TurningRadius > worst->TurningRadius)) {
			worst->TurningRadius = in.TurningRadius;
			worst->HasTurningRadius = true;
		}
	}

	worst->HasExtents = worst->HasExtents || in.HasExtents;
}


/***********************************************************************************************
 * WorldMetricsClass::Get_Class_Metrics --                                                     *
 *=============================================================================================*/
bool WorldMetricsClass::Get_Class_Metrics(WorldLocomotionClassType loco,WorldMetricsStruct * out)
{
	if ((out == nullptr) || (loco < 0) || (loco >= LOCOMOTION_COUNT)) {
		return false;
	}

	if (!_ClassCacheValid) {

		for (int i = 0; i < LOCOMOTION_COUNT; i++) {
			_ClassMetrics[i].Reset();
			_ClassMetricsResolved[i] = false;
		}

		for (	DefinitionClass * def = DefinitionMgrClass::Get_First(CLASSID_PHYSICS,DefinitionMgrClass::ID_SUPERCLASS);
				def != nullptr;
				def = DefinitionMgrClass::Get_Next(def,CLASSID_PHYSICS,DefinitionMgrClass::ID_SUPERCLASS))
		{
			PhysDefClass * physdef = (PhysDefClass *)def;
			WorldLocomotionClassType def_loco = Classify(physdef);
			if (def_loco == LOCOMOTION_INVALID) {
				continue;
			}

			WorldMetricsStruct metrics;
			if (!Get_Metrics((int)def->Get_ID(),&metrics)) {
				continue;
			}

			Accumulate_Worst(metrics,&_ClassMetrics[def_loco]);
			_ClassMetricsResolved[def_loco] = true;
		}

		_ClassCacheValid = true;
	}

	*out = _ClassMetrics[loco];
	return _ClassMetricsResolved[loco] && out->Valid;
}


/***********************************************************************************************
 * WorldMetricsClass::Get_Widest --                                                            *
 * WorldMetricsClass::Get_Tallest --                                                           *
 * WorldMetricsClass::Get_Posture_Height --                                                    *
 *=============================================================================================*/
float WorldMetricsClass::Get_Widest(WorldLocomotionClassType loco)
{
	WorldMetricsStruct metrics;
	if (!Get_Class_Metrics(loco,&metrics)) {
		return 0.0f;
	}
	return metrics.CollisionRadius * 2.0f;
}


float WorldMetricsClass::Get_Tallest(WorldLocomotionClassType loco)
{
	WorldMetricsStruct metrics;
	if (!Get_Class_Metrics(loco,&metrics)) {
		return 0.0f;
	}
	return metrics.StandHeight;
}


float WorldMetricsClass::Get_Posture_Height(WorldLocomotionClassType loco,WorldPostureType posture)
{
	WorldMetricsStruct metrics;
	if (!Get_Class_Metrics(loco,&metrics)) {
		return 0.0f;
	}

	switch (posture) {
		case POSTURE_CROUCH:	return metrics.CrouchHeight;
		case POSTURE_PRONE:	return metrics.ProneHeight;
		case POSTURE_STAND:
		default:					return metrics.StandHeight;
	}
}


/***********************************************************************************************
 * WorldMetricsClass::Get_Building_Footprint --                                                *
 *                                                                                             *
 * From the Section 39 descriptor, which already owns the width and depth.  The radius is the  *
 * circumscribing one -- half the diagonal -- because a building may be placed at any heading  *
 * and a clearance test that only allowed for the width would fail on the diagonal.            *
 *=============================================================================================*/
bool WorldMetricsClass::Get_Building_Footprint(const char * descriptor_name,float * radius_out,float * height_out)
{
	if (radius_out != nullptr) { *radius_out = 0.0f; }
	if (height_out != nullptr) { *height_out = 0.0f; }

	if ((descriptor_name == nullptr) || (descriptor_name[0] == '\0')) {
		return false;
	}

	const BuildingModelDescriptor * descriptor = BuildingDescriptorManager::Get_Descriptor(descriptor_name);
	if (descriptor == nullptr) {
		return false;
	}

	float width = descriptor->Footprint.Width;
	float depth = descriptor->Footprint.Depth;
	if ((width <= 0.0f) || (depth <= 0.0f)) {
		return false;
	}

	if (radius_out != nullptr) {
		*radius_out = 0.5f * WWMath::Sqrt((width * width) + (depth * depth));
	}

	//	Height is not in the descriptor -- Section 39 gives a footprint, which is a plan --
	//	so it comes from the model when the model resolves, and stays zero when it does not.
	if (height_out != nullptr) {
		WW3DAssetManager * assets = WW3DAssetManager::Get_Instance();
		if ((assets != nullptr) && (!descriptor->ModelName.Is_Empty())) {
			RenderObjClass * model = assets->Create_Render_Obj(descriptor->ModelName.Peek_Buffer());
			if (model != nullptr) {
				AABoxClass box = model->Get_Bounding_Box();
				*height_out = box.Extent.Z * 2.0f;
				model->Release_Ref();
			}
		}
	}

	return true;
}
