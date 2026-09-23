/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderplacement.h                  $*
 *                                                                                             *
 *	Roadmap Section 43, Phase 37 -- Commander building ghost / placement validation.            *
 *                                                                                             *
 *	Section 43's real requirement is not the ghost, it is that the ghost cannot lie:            *
 *	"Server reruns placement checks.  Never trust client green/red state."  The only way to     *
 *	make that true rather than promised is for the red/green the client draws and the accept/   *
 *	reject the server decides to be *the same function*, called twice.  So Validate is one      *
 *	static function over one request struct, with no client or server in it at all, and both     *
 *	sides call it: the client every time the cursor moves, the server once when the placement    *
 *	arrives.  They cannot drift because there is nothing to drift from.                          *
 *                                                                                             *
 *	The ghost itself is state -- which definition, where, what rotation, valid or not -- and     *
 *	that state is here too.  What it is *not* is a GameObj: Section 43 says "no gameplay         *
 *	collision/network/save object", and this file contains no code that creates one.  The        *
 *	renderer that draws a translucent model reads Get_Ghost and draws; the RED TRANSLUCENT       *
 *	state Section 43 names in capitals is Is_Ghost_Valid() coming back false.                     *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMMANDERPLACEMENT_H
#define COMMANDERPLACEMENT_H

#include "always.h"
#include "matrix3d.h"
#include "vector3.h"
#include "wwstring.h"

class OBBoxClass;


/*
**	Why a placement was refused.  Section 43's own check list, one value each, so the reason a
**	client greys the ghost out and the reason a server logs a rejection are the same word.
*/
enum CommanderPlacementResultType
{
	PLACEMENT_VALID = 0,
	PLACEMENT_NO_DESCRIPTOR,			// the definition names no loaded descriptor
	PLACEMENT_NOT_BUILDABLE,			// a real descriptor, but not one the Commander may place
	PLACEMENT_OUT_OF_BOUNDS,
	PLACEMENT_BUILDING_OVERLAP,
	PLACEMENT_BLOCKED_GEOMETRY,
	PLACEMENT_UNSUPPORTED_SURFACE,	// no ground under some part of the footprint
	PLACEMENT_SLOPE_TOO_STEEP,
	PLACEMENT_EXIT_BLOCKED,
	PLACEMENT_WRONG_FACTION,

	/*
	**	V2 roadmap Section 34 Phase 17 step 2 -- the generated world's own exclusions.
	**
	**	One value per feature rather than one "generated world says no", because Section 43's
	**	whole point is that the reason a ghost greys out and the reason a server logs are the
	**	same word, and "you cannot build on a bridge" and "you cannot build in the river" are
	**	not the same word.  Each is answered by `GeneratedWorldPlacementRules`, and every one of
	**	them is silent on a hand-authored map, where none of those systems is configured
	**	(directive 1.10).
	*/
	PLACEMENT_ON_WATER,
	PLACEMENT_ON_IMMUTABLE_CLIFF,
	PLACEMENT_ON_TUNNEL,
	PLACEMENT_ON_BRIDGE,
	PLACEMENT_ON_RESOURCE_ROUTE,
	PLACEMENT_BLOCKS_NAV_CHOKEPOINT,

	PLACEMENT_RESULT_COUNT
};


/*
**	Exactly the fields Section 43 says the client sends and the server re-checks, plus the team
**	the server already knows and the client cannot be trusted for.  A sequence id so a reply can
**	name the placement it is answering without the client having to match on position.
*/
struct CommanderPlacementRequest
{
	CommanderPlacementRequest(void) : Position(0.0f,0.0f,0.0f),RotationDegrees(0.0f),
												 Team(-1),SequenceId(0),DescriptorName("") { }

	Vector3		Position;
	float			RotationDegrees;
	int			Team;
	int			SequenceId;
	StringClass	DescriptorName;
};


struct CommanderPlacementResult
{
	CommanderPlacementResult(void) : Result(PLACEMENT_VALID),GroundHeight(0.0f),
												SlopeDegrees(0.0f),SequenceId(0) { }

	bool	Is_Valid(void) const	{ return Result == PLACEMENT_VALID; }

	CommanderPlacementResultType	Result;
	float									GroundHeight;		// where the footprint would actually sit
	float									SlopeDegrees;		// steepest slope under the footprint
	int									SequenceId;
};


class CommanderPlacementValidator
{
	public:

		static void		Init(void);
		static void		Shutdown(void);

		/*
		**	The one shared decision.  Pure with respect to this class -- it reads the descriptor
		**	registry, the spatial index and the terrain, and writes nothing anywhere.  Calling
		**	it a hundred times as a cursor moves costs a hundred spatial queries and changes no
		**	state, which is what makes it safe to be both the ghost's colour and the server's
		**	verdict.
		*/
		static CommanderPlacementResult	Validate(const CommanderPlacementRequest & request);

		/*
		**	The server's entry point for a placement a client sent.  Validate plus bookkeeping:
		**	it records the sequence id of an acceptance and counts rejections, which is what
		**	makes "the server independently rejected the same bad placement" observable rather
		**	than merely true.  Deliberately not where the building is created -- that is Phase
		**	38's BuildingConstructionService, and keeping the two apart is what stops a
		**	validator from growing a side effect.
		*/
		static CommanderPlacementResult	Validate_Server_Request(const CommanderPlacementRequest & request);

		/*
		**	The footprint as the world sees it: the descriptor's width/depth, rotated by the
		**	request's own rotation, centred on the request position.  Exposed because the ghost
		**	renderer wants to draw the same box the validator tested, and Section 43 asks for
		**	the footprint to be shown "through status/waypoint renderer where useful" -- one box,
		**	not the validator's box and a second one drawn near it.
		*/
		static bool		Build_Footprint_Box(const CommanderPlacementRequest & request,OBBoxClass * out_box);

		/*
		**	The disc radius every footprint question in this file is asked over: half the
		**	footprint diagonal, so the whole footprint is inside the disc whatever its rotation.
		**
		**	Public because Phase 17's foundation grading has to measure the same disc this
		**	validator tested.  Two radii for one footprint would be two different buildings --
		**	one the validator accepted and a differently-sized one the ground was levelled for.
		**	False when the name resolves to no loaded descriptor.
		*/
		static bool		Get_Footprint_Radius(const char * descriptor_name,float * radius_out);

		//	The transform the real building would get if this placement were accepted.  The
		//	ghost draws at exactly this, so what a player sees is where the building lands.
		static bool		Build_Placement_Transform(const CommanderPlacementRequest & request,
																 const CommanderPlacementResult & result,
																 Matrix3D * out_transform);

		/*
		**	Ghost state.  Client-side presentation; nothing here is authoritative and nothing
		**	here creates an object.
		*/
		static void		Begin_Ghost(const char * descriptor_name,int team);
		static void		Cancel_Ghost(void);
		static bool		Is_Ghost_Active(void);

		static void		Move_Ghost(const Vector3 & world_point);
		static void		Rotate_Ghost(float degrees_delta);
		static void		Set_Ghost_Rotation(float degrees);

		static const CommanderPlacementRequest &	Get_Ghost(void);

		//	The last Validate result for the ghost's current position -- recomputed by
		//	Move_Ghost/Rotate_Ghost, so a renderer never has to decide when to revalidate.
		static const CommanderPlacementResult &	Get_Ghost_Result(void);
		static bool		Is_Ghost_Valid(void);

		//	Server side: the sequence id of the last placement this validator accepted, so a
		//	caller can confirm an acceptance belongs to the request it sent.
		static int		Get_Last_Accepted_Sequence(void);
		static int		Get_Rejection_Count(void);

		static const char *	Get_Result_Name(CommanderPlacementResultType result);
};

#endif // COMMANDERPLACEMENT_H
