/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderplacement.cpp                $*
 *                                                                                             *
 *	Roadmap Section 43, Phase 37 -- Commander building ghost / placement validation.            *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commanderplacement.h"

#include "buildingdescriptormanager.h"
#include "generatedworldplacementrules.h"
#include "obbox.h"
#include "phys.h"
#include "pscene.h"
#include "worldboundsservice.h"
#include "worldspatialindex.h"
#include "worldterrainsystem.h"
#include "wwmath.h"

#include <math.h>


static bool										_GhostActive	= false;
static CommanderPlacementRequest			_Ghost;
static CommanderPlacementResult			_GhostResult;
static int										_LastAccepted	= 0;
static int										_RejectionCount	= 0;

//	How tall a footprint box is assumed to be for the overlap test when the descriptor gives
//	only a width and a depth.  Section 39's footprint block is deliberately two-dimensional --
//	it is a ground footprint -- so the third dimension has to come from somewhere, and a fixed
//	generous height is the choice that cannot produce a false *accept*: a box that is too tall
//	can only ever find more overlaps, never fewer.
static const float							FOOTPRINT_TEST_HEIGHT	= 12.0f;

//	The disc radius Query_Placement_Surface samples for slope and support.  Half the footprint
//	diagonal, so the whole footprint is inside the sampled disc whatever its rotation.
static float	Footprint_Sample_Radius(const BuildingModelDescriptor & descriptor)
{
	float half_width	= descriptor.Footprint.Width * 0.5f;
	float half_depth	= descriptor.Footprint.Depth * 0.5f;
	return WWMath::Sqrt ((half_width * half_width) + (half_depth * half_depth));
}


/***********************************************************************************************
 * Peek_Buildable -- the descriptor a request names, if it is one the Commander may place        *
 *=============================================================================================*/
static const BuildingModelDescriptor *	Peek_Buildable(const CommanderPlacementRequest & request,
																			CommanderPlacementResultType * reason)
{
	if (request.DescriptorName.Get_Length () == 0) {
		*reason = PLACEMENT_NO_DESCRIPTOR;
		return nullptr;
	}

	const BuildingModelDescriptor * descriptor =
		BuildingDescriptorManager::Get_Descriptor (request.DescriptorName.Peek_Buffer ());
	if (descriptor == nullptr) {
		*reason = PLACEMENT_NO_DESCRIPTOR;
		return nullptr;
	}

	if (!descriptor->Build.Buildable) {
		*reason = PLACEMENT_NOT_BUILDABLE;
		return nullptr;
	}

	//	A descriptor that names a faction may be placed only by that faction.  Checked here
	//	rather than in the sidebar alone, because the sidebar is the client's list and this
	//	function is also the server's verdict.
	if ((descriptor->Build.Team != -1) && (descriptor->Build.Team != request.Team)) {
		*reason = PLACEMENT_WRONG_FACTION;
		return nullptr;
	}

	return descriptor;
}


void CommanderPlacementValidator::Init(void)
{
	_GhostActive		= false;
	_Ghost				= CommanderPlacementRequest ();
	_GhostResult		= CommanderPlacementResult ();
	_LastAccepted		= 0;
	_RejectionCount	= 0;
}

void CommanderPlacementValidator::Shutdown(void)
{
	Init ();
}


bool CommanderPlacementValidator::Build_Footprint_Box(const CommanderPlacementRequest & request,
																			OBBoxClass * out_box)
{
	if (out_box == nullptr) return false;

	CommanderPlacementResultType reason = PLACEMENT_VALID;
	const BuildingModelDescriptor * descriptor = Peek_Buildable (request,&reason);
	if (descriptor == nullptr) return false;

	Vector3 extent (descriptor->Footprint.Width * 0.5f,
						 descriptor->Footprint.Depth * 0.5f,
						 FOOTPRINT_TEST_HEIGHT * 0.5f);

	//	"Rotated buildings must generate rotated visuals" is Phase 38's line, but the same
	//	thing is true one phase earlier: a rotated footprint is tested rotated, which is why
	//	this is an OBB and not the AABB the naive version would reach for.
	Matrix3 basis (1);
	basis.Rotate_Z (DEG_TO_RADF (request.RotationDegrees));

	Vector3 center = request.Position;
	center.Z += FOOTPRINT_TEST_HEIGHT * 0.5f;		// the box sits on the ground, not through it

	*out_box = OBBoxClass (center,extent,basis);
	return true;
}


bool CommanderPlacementValidator::Get_Footprint_Radius(const char * descriptor_name,float * radius_out)
{
	if ((descriptor_name == nullptr) || (descriptor_name [0] == '\0')) return false;

	const BuildingModelDescriptor * descriptor = BuildingDescriptorManager::Get_Descriptor (descriptor_name);
	if (descriptor == nullptr) return false;

	if (radius_out != nullptr) *radius_out = Footprint_Sample_Radius (*descriptor);
	return true;
}


CommanderPlacementResult CommanderPlacementValidator::Validate(const CommanderPlacementRequest & request)
{
	CommanderPlacementResult result;
	result.SequenceId = request.SequenceId;

	CommanderPlacementResultType reason = PLACEMENT_VALID;
	const BuildingModelDescriptor * descriptor = Peek_Buildable (request,&reason);
	if (descriptor == nullptr) {
		result.Result = reason;
		return result;
	}

	/*
	**	V2 roadmap Section 34 Phase 2 step 10 / Section 5.9 -- the authoritative world boundary,
	**	asked before any terrain question, because "no building in the cliff band" is a rule about
	**	the world's topology rather than about the ground under this particular spot.
	**
	**	`Can_Place_In_Region` is stricter than the actor-legality test on purpose: an actor may
	**	stand anywhere inward of the hard barrier, but nothing may be BUILT in the protected cliff
	**	band, and passing the footprint radius is what stops a building whose centre clears the
	**	band from putting a corner inside it.
	**
	**	No-op on every hand-authored map -- `WorldBoundsService` is unconfigured there and answers
	**	true (directive 1.10).
	*/
	{
		float footprint_radius = Footprint_Sample_Radius (*descriptor);
		if (!WorldBoundsService::Can_Place_In_Region (Vector2 (request.Position.X,request.Position.Y),
																	 footprint_radius)) {
			result.Result = PLACEMENT_OUT_OF_BOUNDS;
			return result;
		}
	}

	/*
	**	Surface support and slope, from the one canonical terrain owner.  Query_Placement_Surface
	**	already samples a disc and reports the steepest slope and whether any of the disc fell
	**	off the field -- exactly Section 43's "surface support", "configured slope limits" and
	**	"world bounds" -- so this phase asks it rather than sampling heights itself.
	**
	**	A level with no generated heightfield answers false, and that is not a refusal: stock
	**	Renegade levels are authored geometry with no WorldTerrainSystem field behind them, and
	**	refusing every placement on one would make the whole feature untestable on real content.
	**	In that case the ground is whatever the request said and the slope check is skipped, with
	**	the static-geometry overlap test below doing the real work.
	*/
	TerrainPlacementInfoClass surface;
	float sample_radius = Footprint_Sample_Radius (*descriptor);

	if (WorldTerrainSystem::Has_Terrain () &&
		 WorldTerrainSystem::Query_Placement_Surface (request.Position,sample_radius,&surface)) {

		if (!surface.IsOnTerrain) {
			result.Result = PLACEMENT_OUT_OF_BOUNDS;
			return result;
		}

		result.GroundHeight	= surface.AverageHeight;
		result.SlopeDegrees	= RAD_TO_DEGF (surface.MaxSlope);

		if (descriptor->Placement.RequiresFoundationSupport &&
			 (result.SlopeDegrees > descriptor->Placement.MaxSlopeDegrees)) {
			result.Result = PLACEMENT_SLOPE_TOO_STEEP;
			return result;
		}

	} else {
		result.GroundHeight	= request.Position.Z;
		result.SlopeDegrees	= 0.0f;
	}

	/*
	**	V2 roadmap Section 34 Phase 17 steps 1-2 -- the generated world's own exclusions, asked
	**	of the one owner that knows about water, cliffs, bridges, resource routes and the sector
	**	graph.  Asked after the surface query because the surface query is what resolves the
	**	ground height this file goes on to use, and before the overlap test because "in the
	**	river" is a better answer than "overlaps something" for a placement that is both.
	**
	**	Silent on every hand-authored map: none of the generated-world owners is configured
	**	there, so Classify answers GENERATED_WORLD_EXCLUSION_NONE everywhere (directive 1.10).
	*/
	{
		float exclusion_radius = Footprint_Sample_Radius (*descriptor);
		GeneratedWorldExclusionType exclusion =
			GeneratedWorldPlacementRules::Classify (request.Position,exclusion_radius);

		switch (exclusion) {
			case GENERATED_WORLD_EXCLUSION_BOUNDARY_CLIFF_BAND:
				result.Result = PLACEMENT_OUT_OF_BOUNDS;			return result;
			case GENERATED_WORLD_EXCLUSION_WATER:
				result.Result = PLACEMENT_ON_WATER;					return result;
			case GENERATED_WORLD_EXCLUSION_IMMUTABLE_CLIFF:
				result.Result = PLACEMENT_ON_IMMUTABLE_CLIFF;		return result;
			case GENERATED_WORLD_EXCLUSION_TUNNEL:
				result.Result = PLACEMENT_ON_TUNNEL;				return result;
			case GENERATED_WORLD_EXCLUSION_BRIDGE:
				result.Result = PLACEMENT_ON_BRIDGE;				return result;
			case GENERATED_WORLD_EXCLUSION_RESOURCE_ROUTE:
				result.Result = PLACEMENT_ON_RESOURCE_ROUTE;		return result;
			case GENERATED_WORLD_EXCLUSION_NAV_CHOKEPOINT:
				result.Result = PLACEMENT_BLOCKS_NAV_CHOKEPOINT;	return result;
			default:
				break;
		}
	}

	/*
	**	Overlap.  One OBB test against the physics scene, through the Section 16 service rather
	**	than against PhysicsSceneClass directly, and at the ground height the surface query just
	**	resolved rather than wherever the cursor ray happened to land.
	**
	**	Buildings and blocking world geometry are the same test at this level -- both are
	**	COLLISION_GROUP_WORLD physical objects -- so the two Section 43 bullets ("building
	**	overlap", "placement-blocking static/dynamic geometry") resolve to one query and one
	**	reason.  Splitting them would mean a second query whose only purpose is to relabel the
	**	same hit.
	*/
	CommanderPlacementRequest grounded = request;
	grounded.Position.Z = result.GroundHeight;

	OBBoxClass footprint;
	if (!Build_Footprint_Box (grounded,&footprint)) {
		result.Result = PLACEMENT_NO_DESCRIPTOR;
		return result;
	}

	if (WorldSpatialIndex::Query_Placement_Overlap (footprint,PhysicsSceneClass::COLLISION_GROUP_WORLD)) {
		result.Result = PLACEMENT_BUILDING_OVERLAP;
		return result;
	}

	/*
	**	"Required production-exit/anchor clearance."  A descriptor that asks for clearance gets
	**	a second box of that depth immediately in front of the footprint -- +Y in the building's
	**	own rotated frame, which is the direction a vehicle_exit anchor faces by convention --
	**	tested the same way.  A descriptor asking for no clearance skips the query entirely
	**	rather than testing a zero-sized box.
	*/
	if (descriptor->Placement.ExitClearance > 0.0f) {

		float half_depth		= descriptor->Footprint.Depth * 0.5f;
		float clearance_half	= descriptor->Placement.ExitClearance * 0.5f;

		Matrix3 basis (1);
		basis.Rotate_Z (DEG_TO_RADF (request.RotationDegrees));

		Vector3 forward = basis.Get_Y_Vector ();
		Vector3 exit_center = grounded.Position + (forward * (half_depth + clearance_half));
		exit_center.Z += FOOTPRINT_TEST_HEIGHT * 0.5f;

		OBBoxClass exit_box (exit_center,
									Vector3 (descriptor->Footprint.Width * 0.5f,clearance_half,
												FOOTPRINT_TEST_HEIGHT * 0.5f),
									basis);

		if (WorldSpatialIndex::Query_Placement_Overlap (exit_box,PhysicsSceneClass::COLLISION_GROUP_WORLD)) {
			result.Result = PLACEMENT_EXIT_BLOCKED;
			return result;
		}
	}

	result.Result = PLACEMENT_VALID;
	return result;
}


CommanderPlacementResult CommanderPlacementValidator::Validate_Server_Request(const CommanderPlacementRequest & request)
{
	CommanderPlacementResult result = Validate (request);

	if (result.Is_Valid ()) {
		_LastAccepted = request.SequenceId;
	} else {
		_RejectionCount++;
	}
	return result;
}


bool CommanderPlacementValidator::Build_Placement_Transform(const CommanderPlacementRequest & request,
																				  const CommanderPlacementResult & result,
																				  Matrix3D * out_transform)
{
	if (out_transform == nullptr) return false;

	Matrix3D transform (1);
	transform.Rotate_Z (DEG_TO_RADF (request.RotationDegrees));

	Vector3 position = request.Position;
	position.Z = result.GroundHeight;
	transform.Set_Translation (position);

	*out_transform = transform;
	return true;
}


void CommanderPlacementValidator::Begin_Ghost(const char * descriptor_name,int team)
{
	if (descriptor_name == nullptr || descriptor_name [0] == '\0') return;

	_GhostActive			= true;
	_Ghost					= CommanderPlacementRequest ();
	_Ghost.DescriptorName	= descriptor_name;
	_Ghost.Team				= team;
	_GhostResult			= Validate (_Ghost);
}

void CommanderPlacementValidator::Cancel_Ghost(void)
{
	_GhostActive	= false;
	_Ghost			= CommanderPlacementRequest ();
	_GhostResult	= CommanderPlacementResult ();
}

bool CommanderPlacementValidator::Is_Ghost_Active(void)	{ return _GhostActive; }


void CommanderPlacementValidator::Move_Ghost(const Vector3 & world_point)
{
	if (!_GhostActive) return;
	_Ghost.Position	= world_point;
	_Ghost.SequenceId++;
	_GhostResult		= Validate (_Ghost);
}

void CommanderPlacementValidator::Rotate_Ghost(float degrees_delta)
{
	if (!_GhostActive) return;
	Set_Ghost_Rotation (_Ghost.RotationDegrees + degrees_delta);
}

void CommanderPlacementValidator::Set_Ghost_Rotation(float degrees)
{
	if (!_GhostActive) return;

	//	Kept in [0,360) so a ghost rotated for a long time cannot accumulate a value that loses
	//	precision, and so a replicated rotation has one representation per orientation.
	while (degrees < 0.0f)			degrees += 360.0f;
	while (degrees >= 360.0f)		degrees -= 360.0f;

	_Ghost.RotationDegrees	= degrees;
	_Ghost.SequenceId++;
	_GhostResult				= Validate (_Ghost);
}

const CommanderPlacementRequest & CommanderPlacementValidator::Get_Ghost(void)		{ return _Ghost; }
const CommanderPlacementResult &  CommanderPlacementValidator::Get_Ghost_Result(void)	{ return _GhostResult; }

bool CommanderPlacementValidator::Is_Ghost_Valid(void)
{
	return _GhostActive && _GhostResult.Is_Valid ();
}


int CommanderPlacementValidator::Get_Last_Accepted_Sequence(void)	{ return _LastAccepted; }
int CommanderPlacementValidator::Get_Rejection_Count(void)			{ return _RejectionCount; }


const char * CommanderPlacementValidator::Get_Result_Name(CommanderPlacementResultType result)
{
	switch (result) {
		case PLACEMENT_VALID:					return "valid";
		case PLACEMENT_NO_DESCRIPTOR:			return "no such descriptor";
		case PLACEMENT_NOT_BUILDABLE:			return "not buildable";
		case PLACEMENT_OUT_OF_BOUNDS:			return "out of world bounds";
		case PLACEMENT_BUILDING_OVERLAP:		return "overlaps a building";
		case PLACEMENT_BLOCKED_GEOMETRY:		return "blocked by geometry";
		case PLACEMENT_UNSUPPORTED_SURFACE:	return "unsupported surface";
		case PLACEMENT_SLOPE_TOO_STEEP:		return "slope too steep";
		case PLACEMENT_ON_WATER:				return "on water";
		case PLACEMENT_ON_IMMUTABLE_CLIFF:	return "on an immutable cliff";
		case PLACEMENT_ON_TUNNEL:				return "on a tunnel portal";
		case PLACEMENT_ON_BRIDGE:				return "on a bridge or its approach";
		case PLACEMENT_ON_RESOURCE_ROUTE:	return "on a protected resource route";
		case PLACEMENT_BLOCKS_NAV_CHOKEPOINT:	return "blocks a navigation chokepoint";
		case PLACEMENT_EXIT_BLOCKED:			return "production exit blocked";
		case PLACEMENT_WRONG_FACTION:			return "wrong faction";
		default:										return "unknown";
	}
}
