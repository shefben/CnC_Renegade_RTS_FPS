/*
**	Command & Conquer Renegade(tm)
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
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/foundationgrading.cpp                 $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "foundationgrading.h"

#include "aabox.h"
#include "buildingdescriptor.h"
#include "buildingdescriptormanager.h"
#include "commanderplacement.h"
#include "generatedworldplacementrules.h"
#include "heightfield.h"
#include "worldterrainsystem.h"
#include "wwmath.h"


/*
**	The smoothstep an apron ramp is shaped with reaches its steepest at the middle of the ring,
**	where its gradient is 1.5x the straight-line average.  So an apron sized by the average
**	would leave a middle 50% steeper than the limit.  Sizing it by this factor is what makes the
**	stated slope limit the real one.
**
**	Not a tuning value: it is the maximum of the derivative of 3t^2 - 2t^3 over [0,1].
*/
static const float	SMOOTHSTEP_PEAK_GRADIENT	= 1.5f;


FoundationGradingLimits::FoundationGradingLimits(void) :
	MaxCutDepth(6.0f),
	MaxFillHeight(6.0f),
	MaxSlopeChangeRadians(0.5f),
	MaxAffectedArea(4096.0f)
{
}


FoundationGradingLimits	FoundationGradingService::Limits;

int		FoundationGradingService::ExecutedCount			= 0;
int		FoundationGradingService::RejectedCount			= 0;
int		FoundationGradingService::LastExecutedSequence	= 0;
bool		FoundationGradingService::HasLastRegion			= false;
Vector3	FoundationGradingService::LastRegionCenter		(0.0f,0.0f,0.0f);
Vector3	FoundationGradingService::LastRegionExtent		(0.0f,0.0f,0.0f);


const char *	Foundation_Grading_Result_Name(FoundationGradingResultType result)
{
	switch (result) {
		case FOUNDATION_GRADING_VALID:						return "valid";
		case FOUNDATION_GRADING_NO_TERRAIN:					return "no generated terrain";
		case FOUNDATION_GRADING_NO_DESCRIPTOR:				return "no such descriptor";
		case FOUNDATION_GRADING_PLACEMENT_INVALID:		return "placement invalid";
		case FOUNDATION_GRADING_DEPTH_EXCEEDED:			return "cut depth exceeded";
		case FOUNDATION_GRADING_HEIGHT_EXCEEDED:			return "fill height exceeded";
		case FOUNDATION_GRADING_SLOPE_CHANGE_EXCEEDED:	return "slope change exceeded";
		case FOUNDATION_GRADING_AREA_EXCEEDED:				return "affected area exceeded";
		case FOUNDATION_GRADING_PROTECTED_FEATURE:		return "protected feature";
		default:													return "unknown";
	}
}


void FoundationGradingService::Init(void)
{
	Limits = FoundationGradingLimits();
	Reset_Counters();
}


void FoundationGradingService::Shutdown(void)
{
	Limits = FoundationGradingLimits();
	Reset_Counters();
}


void FoundationGradingService::Reset_Counters(void)
{
	ExecutedCount			= 0;
	RejectedCount			= 0;
	LastExecutedSequence	= 0;
	HasLastRegion			= false;
	LastRegionCenter		= Vector3(0.0f,0.0f,0.0f);
	LastRegionExtent		= Vector3(0.0f,0.0f,0.0f);
}


void FoundationGradingService::Set_Limits(const FoundationGradingLimits & limits)
{
	Limits = limits;

	//	A limit of zero or less is not a stricter limit, it is a broken one: it would refuse
	//	every grading including the ones that change nothing.  Clamp to the documented defaults
	//	rather than silently accepting a profile that disables the feature by accident.
	FoundationGradingLimits defaults;
	if (!(Limits.MaxCutDepth > 0.0f))				Limits.MaxCutDepth				= defaults.MaxCutDepth;
	if (!(Limits.MaxFillHeight > 0.0f))				Limits.MaxFillHeight				= defaults.MaxFillHeight;
	if (!(Limits.MaxSlopeChangeRadians > 0.0f))	Limits.MaxSlopeChangeRadians	= defaults.MaxSlopeChangeRadians;
	if (!(Limits.MaxAffectedArea > 0.0f))			Limits.MaxAffectedArea			= defaults.MaxAffectedArea;
}


const FoundationGradingLimits &	FoundationGradingService::Get_Limits(void)	{ return Limits; }

int	FoundationGradingService::Get_Executed_Count(void)			{ return ExecutedCount; }
int	FoundationGradingService::Get_Rejected_Count(void)			{ return RejectedCount; }
int	FoundationGradingService::Get_Last_Executed_Sequence(void)	{ return LastExecutedSequence; }


bool FoundationGradingService::Get_Last_Executed_Region(AABoxClass * region_out)
{
	if (!HasLastRegion || (region_out == nullptr)) return false;
	region_out->Center = LastRegionCenter;
	region_out->Extent = LastRegionExtent;
	return true;
}


/***********************************************************************************************
**	The graded height at a point, given the plateau and the ramp.
**
**	Deliberately the same arithmetic HeightfieldClass::Grade_Foundation applies, so that what
**	Evaluate measures is what Execute will build.  If these two ever disagree, the limits stop
**	meaning anything -- which is why the measurement recomputes the shape rather than trusting
**	an estimate of it.
*/
static float Graded_Height(float distance,float flat_radius,float apron_radius,
									float target_height,float original_height)
{
	if (distance <= flat_radius)		return target_height;
	if (distance >= apron_radius)		return original_height;

	float ramp = apron_radius - flat_radius;
	if (!(ramp > 0.0f))					return original_height;

	float t = 1.0f - ((distance - flat_radius) / ramp);
	float weight = t * t * (3.0f - 2.0f * t);

	return original_height + (target_height - original_height) * weight;
}


FoundationGradingReport	FoundationGradingService::Evaluate(const FoundationGradingRequest & request)
{
	FoundationGradingReport report;
	report.SequenceId = request.SequenceId;

	if (!WorldTerrainSystem::Has_Terrain()) {
		report.Result = FOUNDATION_GRADING_NO_TERRAIN;
		return report;
	}

	const BuildingModelDescriptor * descriptor =
		BuildingDescriptorManager::Get_Descriptor(request.DescriptorName.Peek_Buffer());
	if (descriptor == nullptr) {
		report.Result = FOUNDATION_GRADING_NO_DESCRIPTOR;
		return report;
	}

	float flat_radius = 0.0f;
	if (!CommanderPlacementValidator::Get_Footprint_Radius(request.DescriptorName.Peek_Buffer(),&flat_radius) ||
		 !(flat_radius > 0.0f)) {
		report.Result = FOUNDATION_GRADING_NO_DESCRIPTOR;
		return report;
	}

	/*
	**	Step 3 -- "from a valid ghost placement".  Asked of the one shared validator, so that a
	**	grading can never be the way a placement the Commander was refused gets built anyway.
	**
	**	One refusal is treated as gradeable rather than fatal: PLACEMENT_SLOPE_TOO_STEEP.  That
	**	is the one thing this whole operation exists to fix -- a foundation grading that could
	**	only ever level ground already flat enough to build on would change no outcome at all,
	**	and Phase 17's own purpose line is letting the Commander build on generated terrain.
	**	Every other refusal, including all seven generated-world exclusions, still stands: a
	**	river, a cliff, a bridge and a chokepoint are not made acceptable by levelling.
	*/
	CommanderPlacementRequest placement;
	placement.Position			= request.Position;
	placement.RotationDegrees	= request.RotationDegrees;
	placement.Team					= request.Team;
	placement.SequenceId			= request.SequenceId;
	placement.DescriptorName	= request.DescriptorName;

	CommanderPlacementResult placement_result = CommanderPlacementValidator::Validate(placement);
	if (!placement_result.Is_Valid() && (placement_result.Result != PLACEMENT_SLOPE_TOO_STEEP)) {
		report.Result = FOUNDATION_GRADING_PLACEMENT_INVALID;
		return report;
	}

	/*
	**	Step 4's target: the height the footprint is levelled to.  The average ground under the
	**	footprint disc, from the canonical surface query -- which is also the height the
	**	placement validator already resolved for this position, so the building lands where the
	**	ghost said it would.  Averaging rather than taking the centre sample is what keeps a cut
	**	and a fill roughly balanced instead of always digging into a hillside.
	*/
	TerrainPlacementInfoClass surface;
	if (!WorldTerrainSystem::Query_Placement_Surface(request.Position,flat_radius,&surface) ||
		 !surface.IsOnTerrain) {
		report.Result = FOUNDATION_GRADING_NO_TERRAIN;
		return report;
	}

	float target_height = surface.AverageHeight;
	report.TargetHeight = target_height;

	HeightfieldClass * heightfield = WorldTerrainSystem::Peek_Heightfield();
	if (heightfield == nullptr) {
		report.Result = FOUNDATION_GRADING_NO_TERRAIN;
		return report;
	}

	float cell_size = heightfield->Get_Cell_Size();
	if (!(cell_size > 0.0f)) {
		report.Result = FOUNDATION_GRADING_NO_TERRAIN;
		return report;
	}

	/*
	**	The plateau has to be wider than the footprint, by one cell diagonal.
	**
	**	Heights live at grid vertices and the ground between them is interpolated, so a point at
	**	the very edge of the footprint sits in a cell whose far corners are outside it.  Level
	**	only the vertices inside the footprint disc and those corners keep their old height,
	**	which tilts the last cell of the pad -- the building's own rim ends up on a slope, which
	**	is exactly what this operation was asked to remove.  Levelling out to one cell diagonal
	**	further means every vertex any footprint sample can interpolate from is on the plateau.
	**
	**	Derived from the heightfield's own cell size, not chosen: it is the furthest a
	**	contributing vertex can be from a point inside the footprint.
	*/
	float support = cell_size * WWMath::Sqrt(2.0f);
	float plateau_radius = flat_radius + support;

	report.PlateauRadius = plateau_radius;

	float max_gradient = WWMath::Tan(Limits.MaxSlopeChangeRadians);
	if (!(max_gradient > 0.0f)) max_gradient = 0.0001f;

	float cell_area = cell_size * cell_size;
	float apron_width = 0.0f;
	bool  sized = false;

	/*
	**	Sizing the apron is a fixed point, not a formula.
	**
	**	The naive answer -- the height difference under the footprint, divided by the steepest
	**	gradient allowed -- is wrong on any real slope, and wrong in the dangerous direction.
	**	The ramp does not pay off the difference at the pad's edge; it pays off the difference at
	**	its own outer rim, and that rim moves further uphill (or downhill) as the ramp gets
	**	longer.  Sizing once from the footprint leaves a ramp materially steeper than the limit
	**	it was supposed to respect.
	**
	**	So each attempt measures the surface it would actually leave and, if that surface is over
	**	the limit, lengthens the ramp in proportion to how far over it was and measures again.
	**	Bounded, in the same spirit as Section 3C.1's bounded local retry: a request that cannot
	**	be satisfied inside the limits is refused with the limit it broke, never widened forever.
	**
	**	It is also the only reason `MaxSlopeChangeRadians` in the report means anything: it is a
	**	measurement of the graded samples, never a restatement of the number used to size them.
	*/
	static const int MAX_APRON_ATTEMPTS = 8;

	for (int attempt = 0; attempt < MAX_APRON_ATTEMPTS; attempt++) {

		if (attempt == 0) {
			//	First estimate, from the difference under the pad itself -- a lower bound on the
			//	ramp any slope will need, and the exact answer on ground that is already level.
			float worst_difference = 0.0f;

			int steps = (int)WWMath::Ceil(plateau_radius / cell_size);
			if (steps < 1) steps = 1;

			for (int iy = -steps; iy <= steps; iy++) {
				for (int ix = -steps; ix <= steps; ix++) {

					float x = request.Position.X + (ix * cell_size);
					float y = request.Position.Y + (iy * cell_size);

					float dx = x - request.Position.X;
					float dy = y - request.Position.Y;
					if ((dx * dx + dy * dy) > (plateau_radius * plateau_radius)) continue;

					float ground = 0.0f;
					if (!WorldTerrainSystem::Sample_Height(x,y,&ground)) continue;

					worst_difference = WWMath::Max(worst_difference,WWMath::Fabs(target_height - ground));
				}
			}

			apron_width = (worst_difference * SMOOTHSTEP_PEAK_GRADIENT) / max_gradient;

			//	Ground already level needs no ramp, and a zero-width one is a vertical edge.  One
			//	cell is the shortest ramp the heightfield can actually represent.
			apron_width = WWMath::Max(apron_width,cell_size);
		}

		float apron_radius = plateau_radius + apron_width;

		//	Step 5's area limit, in plan, pad plus apron.  Checked before the sample sweep
		//	because an oversized apron is both the cheapest rejection and the most likely one --
		//	and checked on every attempt, because lengthening the ramp is what makes it oversized.
		float affected_area = WWMATH_PI * apron_radius * apron_radius;
		if (affected_area > Limits.MaxAffectedArea) {
			report.ApronWidth		= apron_width;
			report.AffectedArea	= affected_area;
			report.Result			= FOUNDATION_GRADING_AREA_EXCEEDED;
			return report;
		}

		/*
		**	Step 4's volumes and step 5's remaining limits, measured on the real heightfield over
		**	the whole affected disc.  Every sample is a heightfield cell, so the volume is a real
		**	sum over the real grid rather than an analytic estimate of an idealised shape.
		*/
		int sweep = (int)WWMath::Ceil(apron_radius / cell_size);
		if (sweep < 1) sweep = 1;

		DynamicVectorClass<float> graded;			// the graded height at each swept sample, or the
		DynamicVectorClass<int>		graded_valid;	// original where the terrain could not answer
		int span = (sweep * 2) + 1;
		graded.Resize(span * span);
		graded_valid.Resize(span * span);
		for (int i = 0; i < (span * span); i++) {
			graded.Add(0.0f);
			graded_valid.Add(0);
		}

		//	This attempt's measurements, kept separate until the attempt is accepted so that a
		//	rejected one cannot leave half its numbers in the report.
		FoundationGradingReport attempt_report;
		attempt_report.SequenceId		= request.SequenceId;
		attempt_report.TargetHeight	= target_height;
		attempt_report.ApronWidth		= apron_width;
		attempt_report.PlateauRadius	= plateau_radius;
		attempt_report.AffectedArea	= affected_area;

		for (int iy = -sweep; iy <= sweep; iy++) {
			for (int ix = -sweep; ix <= sweep; ix++) {

				float x = request.Position.X + (ix * cell_size);
				float y = request.Position.Y + (iy * cell_size);

				float dx = x - request.Position.X;
				float dy = y - request.Position.Y;
				float distance = WWMath::Sqrt((dx * dx) + (dy * dy));
				if (distance > apron_radius) continue;

				float ground = 0.0f;
				if (!WorldTerrainSystem::Sample_Height(x,y,&ground)) continue;

				float after = Graded_Height(distance,plateau_radius,apron_radius,target_height,ground);
				float delta = after - ground;

				int slot = (ix + sweep) + ((iy + sweep) * span);
				graded[slot]		= after;
				graded_valid[slot]	= 1;

				attempt_report.SampleCount++;

				if (delta < 0.0f) {
					attempt_report.CutVolume	+= (-delta) * cell_area;
					attempt_report.MaxCutDepth	= WWMath::Max(attempt_report.MaxCutDepth,-delta);
				} else if (delta > 0.0f) {
					attempt_report.FillVolume		+= delta * cell_area;
					attempt_report.MaxFillHeight	= WWMath::Max(attempt_report.MaxFillHeight,delta);
				}

				/*
				**	Step 6 -- protected features.  Asked per sample over the whole affected disc,
				**	not only under the footprint: the apron is ground this operation moves too, and
				**	an apron that reaches into a river has cut the river's bank.
				**
				**	The navigation half of the exclusion set is deliberately not asked; see
				**	GeneratedWorldPlacementRules::Classify_Static_Features on why levelling ground
				**	cannot create a chokepoint.
				*/
				if (attempt_report.Result == FOUNDATION_GRADING_VALID) {
					GeneratedWorldExclusionType feature =
						GeneratedWorldPlacementRules::Classify_Static_Features(Vector3(x,y,after),0.0f);
					if (feature != GENERATED_WORLD_EXCLUSION_NONE) {
						attempt_report.Result				= FOUNDATION_GRADING_PROTECTED_FEATURE;
						attempt_report.ProtectedFeature	= Generated_World_Exclusion_Name(feature);
					}
				}
			}
		}

		if (attempt_report.Result == FOUNDATION_GRADING_PROTECTED_FEATURE) {
			return attempt_report;
		}

		/*
		**	The slope the graded surface would actually be left at, measured on the graded
		**	samples by finite difference rather than assumed from the apron formula.
		*/
		for (int iy = -sweep; iy < sweep; iy++) {
			for (int ix = -sweep; ix < sweep; ix++) {

				int slot	= (ix + sweep) + ((iy + sweep) * span);
				int east	= (ix + 1 + sweep) + ((iy + sweep) * span);
				int north	= (ix + sweep) + ((iy + 1 + sweep) * span);

				if ((graded_valid[slot] == 0) || (graded_valid[east] == 0) || (graded_valid[north] == 0)) continue;

				float gx = (graded[east] - graded[slot]) / cell_size;
				float gy = (graded[north] - graded[slot]) / cell_size;

				float gradient = WWMath::Sqrt((gx * gx) + (gy * gy));
				attempt_report.MaxSlopeChangeRadians =
					WWMath::Max(attempt_report.MaxSlopeChangeRadians,WWMath::Atan(gradient));
			}
		}

		report = attempt_report;

		if (report.MaxSlopeChangeRadians <= Limits.MaxSlopeChangeRadians) {
			sized = true;
			break;
		}

		//	Over the limit.  Lengthen in proportion to how far over, plus a margin so that a run
		//	of attempts converges instead of creeping up on the limit from below.
		float over = WWMath::Tan(report.MaxSlopeChangeRadians) / max_gradient;
		if (!(over > 1.0f)) over = 1.1f;
		apron_width = apron_width * over * 1.1f;
	}

	if (!sized) {
		//	Ground this operation cannot level without leaving a face steeper than the profile
		//	permits.  Refused with the measurement, not with a shrug.
		report.Result = FOUNDATION_GRADING_SLOPE_CHANGE_EXCEEDED;
		return report;
	}

	if (report.MaxCutDepth > Limits.MaxCutDepth) {
		report.Result = FOUNDATION_GRADING_DEPTH_EXCEEDED;
		return report;
	}
	if (report.MaxFillHeight > Limits.MaxFillHeight) {
		report.Result = FOUNDATION_GRADING_HEIGHT_EXCEEDED;
		return report;
	}
	//	The slope limit was already satisfied by construction: `sized` is exactly the statement
	//	that the measured surface came in under it.

	report.Result = FOUNDATION_GRADING_VALID;
	return report;
}


bool FoundationGradingService::Execute(const FoundationGradingRequest & request,
													FoundationGradingReport * report_out)
{
	//	Step 7's own precondition, re-measured here rather than taken on trust.  A client's
	//	preview is a preview; the ground this grading is about to move is whatever the ground is
	//	now, which may include another grading executed a moment ago next door.
	FoundationGradingReport report = Evaluate(request);
	if (report_out != nullptr) *report_out = report;

	if (!report.Is_Valid()) {
		RejectedCount++;
		return false;
	}

	//	The shape the measurement resolved, not a second derivation of it.
	float plateau_radius	= report.PlateauRadius;
	float apron_radius	= plateau_radius + report.ApronWidth;

	//	Step 7 -- through the canonical owner.  Nothing in this file writes a height sample.
	WorldTerrainSystem::Apply_Foundation_Grade(Vector3(request.Position.X,request.Position.Y,report.TargetHeight),
															 plateau_radius,apron_radius,report.TargetHeight);

	/*
	**	Step 8 -- rebuild what the moved ground invalidated.
	**
	**	Build_Collision refills only the patches the edit marked dirty (its own header says so),
	**	so this is the affected region and not the world.  The chunk registry is rebuilt because
	**	its per-chunk slope and Buildable flags were computed from ground that has now moved --
	**	against the same slope cap it was built with, so that no chunk nobody touched changes
	**	classification.  The far layer was already invalidated by the edit itself.
	*/
	if (WorldTerrainSystem::Has_Collision()) {
		WorldTerrainSystem::Build_Collision();
	}
	if (WorldTerrainSystem::Has_Chunk_Registry()) {
		WorldTerrainSystem::Build_Chunk_Registry(WorldTerrainSystem::Get_Chunk_Max_Buildable_Slope());
	}

	/*
	**	Step 9 -- revalidate server-side on the ground as it now is.  A grading that left the
	**	footprint unbuildable is a grading that must not be reported as a success, and the only
	**	honest way to know is to ask the same validator again after the fact.
	*/
	CommanderPlacementRequest placement;
	placement.Position			= request.Position;
	placement.RotationDegrees	= request.RotationDegrees;
	placement.Team					= request.Team;
	placement.SequenceId			= request.SequenceId;
	placement.DescriptorName	= request.DescriptorName;

	CommanderPlacementResult after = CommanderPlacementValidator::Validate_Server_Request(placement);
	if (!after.Is_Valid()) {
		report.Result = FOUNDATION_GRADING_PLACEMENT_INVALID;
		if (report_out != nullptr) *report_out = report;
		RejectedCount++;
		return false;
	}

	ExecutedCount++;
	LastExecutedSequence	= request.SequenceId;

	HasLastRegion		= true;
	LastRegionCenter	= Vector3(request.Position.X,request.Position.Y,report.TargetHeight);
	LastRegionExtent	= Vector3(apron_radius,apron_radius,
										 WWMath::Max(report.MaxCutDepth,report.MaxFillHeight) + apron_radius);

	return true;
}
