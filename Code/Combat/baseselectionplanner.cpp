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
 *                     $Archive:: /Commando/Code/Combat/baseselectionplanner.cpp              $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "baseselectionplanner.h"

#include "bridgeplanner.h"
#include "crc.h"
#include "playertype.h"
#include "roadgraphgenerator.h"
#include "strategicplanner.h"
#include "watersystem.h"
#include "worldboundsservice.h"
#include "worldmetrics.h"
#include "worldterrainsystem.h"
#include "worldtraversability.h"
#include "wwdebug.h"
#include "wwmath.h"


bool				BaseSelectionPlanner::Built				= false;
unsigned int	BaseSelectionPlanner::Checksum			= 0;

SelectedBaseSite	BaseSelectionPlanner::GdiSite;
SelectedBaseSite	BaseSelectionPlanner::NodSite;
bool					BaseSelectionPlanner::HasGdiSite		= false;
bool					BaseSelectionPlanner::HasNodSite		= false;

BaseFairnessMetrics	BaseSelectionPlanner::LastMetrics;
DynamicVectorClass<BaseRepairLogEntry>	BaseSelectionPlanner::RepairLog;
int				BaseSelectionPlanner::RejectedPairCount	= 0;


//	Section 3B.4 quantization, the same 1000.0 every other generator in this chain already uses.
static const float CHECKSUM_QUANTUM = 1000.0f;

//	Step 1's "buildable area"/"expansion room" are two different radii around the same site --
//	the first, whether a base fits at all, sized directly from the fairness threshold that
//	already names it; the second, whether it can grow, a fixed multiple of the first, since no
//	profile field distinguishes the two.
static const float EXPANSION_ROOM_RADIUS_MULTIPLIER = 2.0f;

//	Step 1's "high-ground exposure": how much of the ring at the base's own footprint radius
//	stands meaningfully higher than the site itself.  Reuses the footprint radius rather than a
//	second, unjustified "weapon range" constant this project does not define anywhere else.
static const float HIGH_GROUND_EPSILON = 1.0f;			// world units; ignores terrain noise below this
static const float HIGH_GROUND_RING_TOLERANCE = 0.25f;	// +/- fraction of the ring radius sampled


const char * Base_Repair_Operation_Name(BaseRepairOperationType op)
{
	switch (op) {
		case BASE_REPAIR_NONE:							return "none";
		case BASE_REPAIR_ADJUST_ROAD_EDGE:			return "adjust_road_edge";
		case BASE_REPAIR_ADD_RAMP:						return "add_ramp";
		case BASE_REPAIR_ADD_TUNNEL_CANDIDATE:		return "add_tunnel_candidate";
		case BASE_REPAIR_MOVE_RESOURCE_CANDIDATE:	return "move_resource_candidate";
		case BASE_REPAIR_MOVE_BASE:						return "move_base";
		case BASE_REPAIR_ADJUST_LOCAL_TERRAIN:		return "adjust_local_terrain";
		default:												return "unknown";
	}
}


SelectedBaseSite::SelectedBaseSite(void) :
	Faction(0), Position(0.0f,0.0f,0.0f), ProtectedRadius(0.0f),
	InitialResourceTravelTimeSeconds(-1.0f), VehicleRouteCount(-1), InfantryRouteCount(-1)
{ }

bool SelectedBaseSite::operator == (const SelectedBaseSite & that) const
{
	return (Id == that.Id) && (Faction == that.Faction) && (Position == that.Position) &&
			 (ProtectedRadius == that.ProtectedRadius) &&
			 (InitialResourceTravelTimeSeconds == that.InitialResourceTravelTimeSeconds) &&
			 (VehicleRouteCount == that.VehicleRouteCount) && (InfantryRouteCount == that.InfantryRouteCount);
}


BaseFairnessMetrics::BaseFairnessMetrics(void) :
	ResourceTravelTimeDifferenceSeconds(0.0f), BaseToBaseTravelTimeSeconds(0.0f),
	SingleBridgeDependency(false), PassedThresholds(false)
{ }


BaseRepairLogEntry::BaseRepairLogEntry(void) :
	PairAttemptIndex(-1), Operation(BASE_REPAIR_NONE), Applied(false)
{ }

bool BaseRepairLogEntry::operator == (const BaseRepairLogEntry & that) const
{
	return (PairAttemptIndex == that.PairAttemptIndex) && (Operation == that.Operation) &&
			 (TargetId == that.TargetId) && (Applied == that.Applied);
}


BaseSelectionParams::BaseSelectionParams(void) :
	Seed(1),
	MaxInitialResourceTravelTimeDifferenceSeconds(15.0f),
	MaxBaseToBaseTravelTimeDifferenceSeconds(20.0f),
	MinBaseToBaseTravelTimeSeconds(0.0f),
	MinBaseExpansionArea(4096.0f),
	AllowSingleBridgeDependency(false),
	MaxRepairAttemptsPerPair(8),
	ReferenceVehicleSpeed(WorldMetricsClass::Get_Reference_Vehicle_Speed()),
	ReferenceInfantrySpeed(WorldMetricsClass::Get_Reference_Infantry_Speed())
{ }


/***********************************************************************************************
 * local helpers                                                                                 *
 *=============================================================================================*/

//	Fraction of the chunks within `radius` of `position` that are buildable and dry -- the same
//	two eligibility tests StrategicPlanner's own chunk scan already applies, just centred on one
//	site instead of walked over the whole grid.
static float Compute_Buildable_Fraction(const Vector3 & position,float radius)
{
	int count_x = WorldTerrainSystem::Get_Chunk_Count_X();
	int count_y = WorldTerrainSystem::Get_Chunk_Count_Y();

	int scanned = 0;
	int buildable = 0;

	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {

			AABoxClass bounds;
			if (!WorldTerrainSystem::Get_Patch_Bounds(px,py,&bounds)) continue;

			float dx = bounds.Center.X - position.X;
			float dy = bounds.Center.Y - position.Y;
			if (WWMath::Sqrt((dx * dx) + (dy * dy)) > radius) continue;

			TerrainChunkInfoClass info;
			if (!WorldTerrainSystem::Get_Chunk_Info(px,py,&info)) continue;

			scanned++;
			if (info.Buildable && !WaterSystem::Is_Point_In_Water(bounds.Center.X,bounds.Center.Y)) {
				buildable++;
			}
		}
	}

	return (scanned > 0) ? ((float)buildable / (float)scanned) : 0.0f;
}


//	Fraction of a ring around `position`, at `ring_radius`, that stands more than
//	HIGH_GROUND_EPSILON above the site's own height -- step 1's "high-ground exposure" made
//	concrete as "how much of the surrounding terrain could look down on this base".
static float Compute_High_Ground_Exposure(const Vector3 & position,float ring_radius)
{
	float base_height = position.Z;
	WorldTerrainSystem::Sample_Height(position.X,position.Y,&base_height);

	int count_x = WorldTerrainSystem::Get_Chunk_Count_X();
	int count_y = WorldTerrainSystem::Get_Chunk_Count_Y();

	int samples = 0;
	int higher = 0;

	float inner = ring_radius * (1.0f - HIGH_GROUND_RING_TOLERANCE);
	float outer = ring_radius * (1.0f + HIGH_GROUND_RING_TOLERANCE);

	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {

			AABoxClass bounds;
			if (!WorldTerrainSystem::Get_Patch_Bounds(px,py,&bounds)) continue;

			float dx = bounds.Center.X - position.X;
			float dy = bounds.Center.Y - position.Y;
			float dist = WWMath::Sqrt((dx * dx) + (dy * dy));
			if ((dist < inner) || (dist > outer)) continue;

			float h;
			if (!WorldTerrainSystem::Sample_Height(bounds.Center.X,bounds.Center.Y,&h)) continue;

			samples++;
			if (h > (base_height + HIGH_GROUND_EPSILON)) higher++;
		}
	}

	return (samples > 0) ? ((float)higher / (float)samples) : 0.0f;
}


//	One base candidate's own composite site score, 0..1 -- ranks pairs against each other; never
//	itself a pass/fail gate (the named fairness thresholds below are).
struct SiteScore
{
	SiteScore(void) : Footprint(0.0f), Expansion(0.0f), HighGround(0.0f), AirRoom(0.0f),
							 Boundary(0.0f), Composite(0.0f) { }

	float	Footprint;
	float	Expansion;
	float	HighGround;		// already inverted -- higher is *less* exposed
	float	AirRoom;
	float	Boundary;
	float	Composite;

	//	DynamicVectorClass<T> (wwlib/vector.h) requires T to support == and !=.
	bool	operator == (const SiteScore & that) const
	{
		return (Footprint == that.Footprint) && (Expansion == that.Expansion) &&
				 (HighGround == that.HighGround) && (AirRoom == that.AirRoom) &&
				 (Boundary == that.Boundary) && (Composite == that.Composite);
	}
	bool	operator != (const SiteScore & that) const	{ return !(*this == that); }
};


static SiteScore Compute_Site_Score(const Vector3 & position,float footprint_radius,
												 float boundary_reference)
{
	SiteScore score;

	score.Footprint = Compute_Buildable_Fraction(position,footprint_radius);
	score.Expansion = Compute_Buildable_Fraction(position,footprint_radius * EXPANSION_ROOM_RADIUS_MULTIPLIER);

	float exposure = Compute_High_Ground_Exposure(position,footprint_radius);
	score.HighGround = 1.0f - exposure;

	float ceiling = WorldBoundsService::Get_Air_Ceiling();
	float ground = position.Z;
	WorldTerrainSystem::Sample_Height(position.X,position.Y,&ground);
	score.AirRoom = (ceiling > 0.0f) ? WWMath::Clamp((ceiling - ground) / ceiling,0.0f,1.0f) : 0.0f;

	float boundary_distance = WorldBoundsService::Get_Inward_Distance(Vector2(position.X,position.Y));
	score.Boundary = WWMath::Clamp(boundary_distance / boundary_reference,0.0f,1.0f);

	score.Composite = (score.Footprint		* 0.30f) +
							 (score.Expansion		* 0.20f) +
							 (score.HighGround		* 0.15f) +
							 (score.AirRoom			* 0.15f) +
							 (score.Boundary			* 0.20f);

	return score;
}


//	The unique path between two anchor nodes -- real road distance, not Phase 7's own
//	straight-line Get_Travel_Cost stand-in, now that a real road graph exists to measure one on.
//	A thin wrapper over RoadGraphGenerator::Find_Anchor_Path (the graph's own owner), not a
//	second copy of the breadth-first walk that answers it.
struct TreePath
{
	TreePath(void) : Found(false), Length(0.0f) { }

	bool								Found;
	float								Length;
	DynamicVectorClass<int>	EdgeIndices;	// indices into RoadGraphGenerator's own edge list
};


static void Find_Tree_Path(int from_node,int to_node,TreePath * out)
{
	out->EdgeIndices.Delete_All();
	out->Found = RoadGraphGenerator::Find_Anchor_Path(from_node,to_node,&out->EdgeIndices,&out->Length);
}


//	How many edges on `path` carry a bridge that actually counts as connectivity -- since the
//	road graph is a tree, any one of these is already a hard single point of failure within the
//	road network itself (there is no second tree edge to route around it on).
static int Count_Bridge_Edges_On_Path(const TreePath & path)
{
	int count = 0;
	for (int i = 0; i < path.EdgeIndices.Count(); i++) {
		RoadGraphEdge edge;
		if (!RoadGraphGenerator::Get_Edge(path.EdgeIndices[i],&edge)) continue;
		for (int b = 0; b < BridgePlanner::Get_Bridge_Count(); b++) {
			ResolvedBridge bridge;
			if (!BridgePlanner::Get_Bridge(b,&bridge)) continue;
			if ((bridge.RoadEdgeId == edge.Id) && bridge.CountsAsConnectivity) { count++; break; }
		}
	}
	return count;
}


//	Real road distance from `base_node` to the nearest RESOURCE_CANDIDATE node, excluding any
//	node index already listed in `excluded`.  -1 when none remain reachable.
static int Find_Nearest_Resource_Node(int base_node,const DynamicVectorClass<int> & excluded,float * length_out)
{
	int best_node = -1;
	float best_length = -1.0f;

	for (int n = 0; n < StrategicPlanner::Get_Node_Count(); n++) {
		bool is_excluded = false;
		for (int x = 0; x < excluded.Count(); x++) {
			if (excluded[x] == n) { is_excluded = true; break; }
		}
		if (is_excluded) continue;

		StrategicNode node;
		if (!StrategicPlanner::Get_Node(n,&node)) continue;
		if (node.Type != STRATEGIC_NODE_RESOURCE_CANDIDATE) continue;

		TreePath path;
		Find_Tree_Path(base_node,n,&path);
		if (!path.Found) continue;

		if ((best_length < 0.0f) || (path.Length < best_length)) {
			best_length = path.Length;
			best_node = n;
		}
	}

	*length_out = best_length;
	return best_node;
}


/*
**	One candidate pair's full evaluation -- steps 2/3's own per-faction metrics, plus the
**	structural facts step 4's repair loop needs to know what to try next.
*/
struct PairEvaluation
{
	PairEvaluation(void) : Valid(false), PathLength(0.0f), BridgeEdgesOnPath(0),
									GdiResourceNode(-1), NodResourceNode(-1),
									GdiResourceLength(-1.0f), NodResourceLength(-1.0f) { }

	bool		Valid;					// the tree path itself resolved
	float		PathLength;
	int		BridgeEdgesOnPath;
	int		GdiResourceNode;
	int		NodResourceNode;
	float		GdiResourceLength;
	float		NodResourceLength;
};


static PairEvaluation Evaluate_Pair(int gdi_node,int nod_node,
												 const DynamicVectorClass<int> & gdi_resource_exclusions,
												 const DynamicVectorClass<int> & nod_resource_exclusions)
{
	PairEvaluation eval;

	TreePath path;
	Find_Tree_Path(gdi_node,nod_node,&path);
	eval.Valid = path.Found;
	if (!eval.Valid) return eval;

	eval.PathLength = path.Length;
	eval.BridgeEdgesOnPath = Count_Bridge_Edges_On_Path(path);

	eval.GdiResourceNode = Find_Nearest_Resource_Node(gdi_node,gdi_resource_exclusions,&eval.GdiResourceLength);
	eval.NodResourceNode = Find_Nearest_Resource_Node(nod_node,nod_resource_exclusions,&eval.NodResourceLength);

	return eval;
}


static bool Thresholds_Pass(const PairEvaluation & eval,const BaseSelectionParams & params,
									  BaseFairnessMetrics * metrics_out)
{
	metrics_out->PassedThresholds = false;

	if (!eval.Valid) return false;
	if ((eval.GdiResourceLength < 0.0f) || (eval.NodResourceLength < 0.0f)) return false;

	float gdi_time = eval.GdiResourceLength / params.ReferenceVehicleSpeed;
	float nod_time = eval.NodResourceLength / params.ReferenceVehicleSpeed;
	metrics_out->ResourceTravelTimeDifferenceSeconds = WWMath::Fabs(gdi_time - nod_time);

	metrics_out->BaseToBaseTravelTimeSeconds = eval.PathLength / params.ReferenceVehicleSpeed;

	metrics_out->SingleBridgeDependency = (eval.BridgeEdgesOnPath >= 1);

	bool resource_ok	= metrics_out->ResourceTravelTimeDifferenceSeconds <=
								 params.MaxInitialResourceTravelTimeDifferenceSeconds;
	bool travel_ok		= metrics_out->BaseToBaseTravelTimeSeconds >= params.MinBaseToBaseTravelTimeSeconds &&
		metrics_out->BaseToBaseTravelTimeSeconds <= params.MaxBaseToBaseTravelTimeDifferenceSeconds;
	bool bridge_ok		= params.AllowSingleBridgeDependency || !metrics_out->SingleBridgeDependency;

	metrics_out->PassedThresholds = resource_ok && travel_ok && bridge_ok;
	return metrics_out->PassedThresholds;
}


/***********************************************************************************************
 * Init/Shutdown/Clear                                                                           *
 *=============================================================================================*/
void BaseSelectionPlanner::Init(void)		{ Clear(); }
void BaseSelectionPlanner::Shutdown(void)	{ Clear(); }

void BaseSelectionPlanner::Clear(void)
{
	Built = false;
	Checksum = 0;
	GdiSite = SelectedBaseSite();
	NodSite = SelectedBaseSite();
	HasGdiSite = false;
	HasNodSite = false;
	LastMetrics = BaseFairnessMetrics();
	RepairLog.Delete_All();
	RejectedPairCount = 0;
}


/***********************************************************************************************
 * Generate -- Phase 11 steps 1-9                                                                *
 *=============================================================================================*/
bool BaseSelectionPlanner::Generate(const BaseSelectionParams & params,StringClass * error_out)
{
	#define FAIL(msg) { if (error_out) *error_out = msg; Clear(); return false; }

	Clear();

	if (!StrategicPlanner::Has_Plan())		FAIL("no strategic plan -- generate base candidates first");
	if (!RoadGraphGenerator::Has_Graph())	FAIL("no road graph -- connect the candidates first");
	if (!BridgePlanner::Has_Plan())			FAIL("no bridge plan -- resolve crossings first");
	if (params.MinBaseExpansionArea <= 0.0f)		FAIL("min base expansion area must be positive");
	if (params.MinBaseToBaseTravelTimeSeconds < 0.0f ||
		params.MinBaseToBaseTravelTimeSeconds > params.MaxBaseToBaseTravelTimeDifferenceSeconds)
		FAIL("invalid base-to-base travel time interval");
	if (params.MaxRepairAttemptsPerPair < 0)		FAIL("max repair attempts per pair must not be negative");
	if (params.ReferenceVehicleSpeed <= 0.0f)		FAIL("reference vehicle speed must be positive");

	//	Steps 1-2: gather every candidate.  Fewer than one per faction is a correct, unremarkable
	//	outcome for a plan too small to have produced one -- Has_Selection() is how a caller
	//	learns whether that happened, not a failure this generator reports.
	DynamicVectorClass<int> gdi_nodes;
	DynamicVectorClass<int> nod_nodes;
	for (int n = 0; n < StrategicPlanner::Get_Node_Count(); n++) {
		StrategicNode node;
		if (!StrategicPlanner::Get_Node(n,&node)) continue;
		if (node.Type != STRATEGIC_NODE_BASE_CANDIDATE) continue;
		if (node.Faction == PLAYERTYPE_GDI)			gdi_nodes.Add(n);
		else if (node.Faction == PLAYERTYPE_NOD)	nod_nodes.Add(n);
	}
	if ((gdi_nodes.Count() == 0) || (nod_nodes.Count() == 0)) {
		Built = true;			// correctly nothing to select, not a failure
		return true;
	}

	float footprint_radius = WWMath::Sqrt(params.MinBaseExpansionArea / WWMATH_PI);
	float boundary_reference = footprint_radius * 4.0f;		// scores to full weight at 4x the footprint

	//	Step 1's own site score, computed once per candidate rather than once per pair.
	DynamicVectorClass<SiteScore> gdi_scores;
	for (int i = 0; i < gdi_nodes.Count(); i++) {
		StrategicNode node;
		StrategicPlanner::Get_Node(gdi_nodes[i],&node);
		gdi_scores.Add(Compute_Site_Score(node.Position,footprint_radius,boundary_reference));
	}
	DynamicVectorClass<SiteScore> nod_scores;
	for (int i = 0; i < nod_nodes.Count(); i++) {
		StrategicNode node;
		StrategicPlanner::Get_Node(nod_nodes[i],&node);
		nod_scores.Add(Compute_Site_Score(node.Position,footprint_radius,boundary_reference));
	}

	//	Step 2: rank every GDI x Nod pair by combined site score, descending; ties break on the
	//	lower (gdi_index, nod_index) pair, the same ascending-index tie-break every other ranked
	//	pass in this chain already uses.
	DynamicVectorClass<int> ranked_gdi;
	DynamicVectorClass<int> ranked_nod;
	DynamicVectorClass<float> ranked_score;
	for (int g = 0; g < gdi_nodes.Count(); g++) {
		for (int n = 0; n < nod_nodes.Count(); n++) {
			ranked_gdi.Add(g);
			ranked_nod.Add(n);
			ranked_score.Add((gdi_scores[g].Composite + nod_scores[n].Composite) * 0.5f);
		}
	}
	for (int a = 1; a < ranked_score.Count(); a++) {
		int keyg = ranked_gdi[a], keyn = ranked_nod[a];
		float keys = ranked_score[a];
		int b = a - 1;
		while ((b >= 0) &&
				 ((ranked_score[b] < keys) ||
				  ((ranked_score[b] == keys) &&
					((ranked_gdi[b] > keyg) || ((ranked_gdi[b] == keyg) && (ranked_nod[b] > keyn)))))) {
			ranked_gdi[b + 1]		= ranked_gdi[b];
			ranked_nod[b + 1]		= ranked_nod[b];
			ranked_score[b + 1]	= ranked_score[b];
			b--;
		}
		ranked_gdi[b + 1]		= keyg;
		ranked_nod[b + 1]		= keyn;
		ranked_score[b + 1]	= keys;
	}

	//	Steps 3-7: try each ranked pair, with step 4's own bounded local repair before giving up
	//	on it (step 6) and moving to the next.
	bool selected = false;
	int selected_gdi_idx = -1, selected_nod_idx = -1;
	BaseFairnessMetrics selected_metrics;

	for (int p = 0; (p < ranked_gdi.Count()) && !selected; p++) {

		int gdi_slot = ranked_gdi[p];
		int nod_slot = ranked_nod[p];

		DynamicVectorClass<int> gdi_resource_exclusions;
		DynamicVectorClass<int> nod_resource_exclusions;

		int attempts = 0;
		while (true) {

			int gdi_node = gdi_nodes[gdi_slot];
			int nod_node = nod_nodes[nod_slot];

			PairEvaluation eval = Evaluate_Pair(gdi_node,nod_node,
															 gdi_resource_exclusions,nod_resource_exclusions);

			BaseFairnessMetrics metrics;
			if (Thresholds_Pass(eval,params,&metrics)) {
				selected = true;
				selected_gdi_idx = gdi_slot;
				selected_nod_idx = nod_slot;
				selected_metrics = metrics;
				break;
			}

			if (attempts >= params.MaxRepairAttemptsPerPair) break;

			//	Step 4: pick the one repair this file can actually perform for real -- see the
			//	class comment and docs/procedural_world/BaseSelection.md for the operations named
			//	in the enum but never attempted here (no owner exists yet for a road edit, a ramp,
			//	a tunnel candidate or a terrain edit made from this generator).
			BaseRepairLogEntry entry;
			entry.PairAttemptIndex = p;

			if (!eval.Valid) {
				//	The tree itself does not connect this pair -- should not happen once
				//	RoadGraphGenerator succeeded with >= 2 anchors, but if it ever does there is
				//	nothing this file can repair; record and give up on this pair.
				entry.Operation = BASE_REPAIR_ADJUST_ROAD_EDGE;
				entry.TargetId = "";
				entry.Applied = false;
				RepairLog.Add(entry);
				break;
			}

			if ((eval.GdiResourceLength < 0.0f) || (eval.NodResourceLength < 0.0f) ||
				 (WWMath::Fabs((eval.GdiResourceLength / params.ReferenceVehicleSpeed) -
									(eval.NodResourceLength / params.ReferenceVehicleSpeed)) >
				  params.MaxInitialResourceTravelTimeDifferenceSeconds)) {

				//	Substitute whichever side's own resource route is worse (or missing) for its
				//	own next-nearest resource candidate -- StrategicPlanner, not this file, owns
				//	where a resource candidate sits, so "move" here means "try the next one
				//	already in the plan", not translate a node this file does not own.
				bool gdi_worse = (eval.GdiResourceLength < 0.0f) ||
										((eval.NodResourceLength >= 0.0f) && (eval.GdiResourceLength > eval.NodResourceLength));
				if (gdi_worse && (eval.GdiResourceNode >= 0)) {
					gdi_resource_exclusions.Add(eval.GdiResourceNode);
					entry.TargetId = "gdi_resource";
					entry.Applied = true;
				} else if (!gdi_worse && (eval.NodResourceNode >= 0)) {
					nod_resource_exclusions.Add(eval.NodResourceNode);
					entry.TargetId = "nod_resource";
					entry.Applied = true;
				} else {
					entry.TargetId = "";
					entry.Applied = false;
				}
				entry.Operation = BASE_REPAIR_MOVE_RESOURCE_CANDIDATE;
				RepairLog.Add(entry);

				if (!entry.Applied) break;
				attempts++;
				continue;
			}

			bool bridge_fails = !params.AllowSingleBridgeDependency && (eval.BridgeEdgesOnPath >= 1);
			float base_travel_time = eval.PathLength / params.ReferenceVehicleSpeed;
			bool travel_fails = base_travel_time < params.MinBaseToBaseTravelTimeSeconds ||
				base_travel_time > params.MaxBaseToBaseTravelTimeDifferenceSeconds;

			if (bridge_fails || travel_fails) {
				//	Hold the better-scoring side fixed and substitute the weaker side's own site
				//	for its next-ranked same-faction alternative -- step 4's "move base within
				//	candidate region": a different already-generated candidate, not a coordinate
				//	this file invents.  Neither RoadGraphGenerator's own tree nor BridgeSystem's
				//	own committed bridges are touched -- rerouting an edge that already carries a
				//	committed bridge is exactly the scope boundary BridgePlanner's own fallback
				//	chain already refuses (see bridgeplanner.cpp), and this file has no safer
				//	answer to that question than it does.
				bool move_gdi = gdi_scores[gdi_slot].Composite <= nod_scores[nod_slot].Composite;
				bool moved = false;
				if (move_gdi) {
					if ((gdi_slot + 1) < gdi_nodes.Count()) { gdi_slot++; moved = true; }
				} else {
					if ((nod_slot + 1) < nod_nodes.Count()) { nod_slot++; moved = true; }
				}

				entry.Operation = BASE_REPAIR_MOVE_BASE;
				entry.TargetId = move_gdi ? "gdi_base" : "nod_base";
				entry.Applied = moved;
				RepairLog.Add(entry);

				if (!moved) break;
				attempts++;
				continue;
			}

			//	Every named failure was already handled above; reaching here means
			//	Thresholds_Pass refused for a reason this loop did not classify, which should not
			//	happen given the three checks above are exhaustive over what Thresholds_Pass
			//	tests.  Stop rather than spin.
			break;
		}

		if (!selected) RejectedPairCount++;
	}

	if (!selected) {
		FAIL("every candidate pair failed fairness thresholds, with repair exhausted on each");
	}

	StrategicNode gdi_node, nod_node;
	StrategicPlanner::Get_Node(gdi_nodes[selected_gdi_idx],&gdi_node);
	StrategicPlanner::Get_Node(nod_nodes[selected_nod_idx],&nod_node);

	GdiSite.Id			= gdi_node.Id;
	GdiSite.Faction	= gdi_node.Faction;
	GdiSite.Position	= gdi_node.Position;
	GdiSite.ProtectedRadius = footprint_radius;

	NodSite.Id			= nod_node.Id;
	NodSite.Faction	= nod_node.Faction;
	NodSite.Position	= nod_node.Position;
	NodSite.ProtectedRadius = footprint_radius;

	float gdi_resource_length, nod_resource_length;
	DynamicVectorClass<int> no_exclusions;
	Find_Nearest_Resource_Node(gdi_nodes[selected_gdi_idx],no_exclusions,&gdi_resource_length);
	Find_Nearest_Resource_Node(nod_nodes[selected_nod_idx],no_exclusions,&nod_resource_length);
	GdiSite.InitialResourceTravelTimeSeconds =
		(gdi_resource_length >= 0.0f) ? (gdi_resource_length / params.ReferenceVehicleSpeed) : -1.0f;
	NodSite.InitialResourceTravelTimeSeconds =
		(nod_resource_length >= 0.0f) ? (nod_resource_length / params.ReferenceVehicleSpeed) : -1.0f;

	//	Steps 1/3's own approach-count criteria, enriched with a real off-road answer when a
	//	wheeled-vehicle graph can be built at all -- honestly -1 (not "0", which would read as
	//	"no approaches exist") when it cannot, the same degradation TransportationNetworkValidator
	//	already established for exactly this question.
	int vehicle_routes = -1;
	int infantry_routes = -1;
	if (WorldTraversabilityGraph::Build(LOCOMOTION_WHEELED)) {
		int from_node = WorldTraversabilityGraph::Find_Node(GdiSite.Position);
		int to_node		= WorldTraversabilityGraph::Find_Node(NodSite.Position);
		if ((from_node >= 0) && (to_node >= 0)) {
			vehicle_routes = WorldTraversabilityGraph::Count_Distinct_Routes(from_node,to_node);
			if (vehicle_routes >= 2) selected_metrics.SingleBridgeDependency = false;
		}
		WorldTraversabilityGraph::Destroy();
	}
	if (WorldTraversabilityGraph::Build(LOCOMOTION_INFANTRY)) {
		int from_node = WorldTraversabilityGraph::Find_Node(GdiSite.Position);
		int to_node		= WorldTraversabilityGraph::Find_Node(NodSite.Position);
		if ((from_node >= 0) && (to_node >= 0)) {
			infantry_routes = WorldTraversabilityGraph::Count_Distinct_Routes(from_node,to_node);
		}
		WorldTraversabilityGraph::Destroy();
	}
	GdiSite.VehicleRouteCount = vehicle_routes;	NodSite.VehicleRouteCount = vehicle_routes;
	GdiSite.InfantryRouteCount = infantry_routes;	NodSite.InfantryRouteCount = infantry_routes;

	HasGdiSite = true;
	HasNodSite = true;
	LastMetrics = selected_metrics;

	{
		CRCEngine crc;
		int q_gdi_x = (int)(GdiSite.Position.X * CHECKSUM_QUANTUM);
		int q_gdi_y = (int)(GdiSite.Position.Y * CHECKSUM_QUANTUM);
		int q_nod_x = (int)(NodSite.Position.X * CHECKSUM_QUANTUM);
		int q_nod_y = (int)(NodSite.Position.Y * CHECKSUM_QUANTUM);
		int q_radius = (int)(footprint_radius * CHECKSUM_QUANTUM);
		crc(&q_gdi_x,sizeof(q_gdi_x));
		crc(&q_gdi_y,sizeof(q_gdi_y));
		crc(&q_nod_x,sizeof(q_nod_x));
		crc(&q_nod_y,sizeof(q_nod_y));
		crc(&q_radius,sizeof(q_radius));
		Checksum = (unsigned int)crc;
	}

	Built = true;

	WWDEBUG_SAY(("BaseSelectionPlanner: selected %s / %s -- %d pair(s) rejected, %d repair(s) logged, "
					 "base-to-base %.1fs, resource difference %.1fs, single-bridge-dependency %s\r\n",
					 GdiSite.Id.Peek_Buffer(),NodSite.Id.Peek_Buffer(),RejectedPairCount,RepairLog.Count(),
					 LastMetrics.BaseToBaseTravelTimeSeconds,LastMetrics.ResourceTravelTimeDifferenceSeconds,
					 LastMetrics.SingleBridgeDependency ? "yes" : "no"));

	#undef FAIL
	return true;
}


bool BaseSelectionPlanner::Get_Selected_Site(int faction,SelectedBaseSite * out)
{
	if (out == nullptr) return false;
	if ((faction == PLAYERTYPE_GDI) && HasGdiSite)	{ *out = GdiSite; return true; }
	if ((faction == PLAYERTYPE_NOD) && HasNodSite)	{ *out = NodSite; return true; }
	return false;
}


bool BaseSelectionPlanner::Get_Repair_Log_Entry(int index,BaseRepairLogEntry * out)
{
	if ((out == nullptr) || (index < 0) || (index >= RepairLog.Count())) return false;
	*out = RepairLog[index];
	return true;
}


/***********************************************************************************************
 * self-check accessors                                                                          *
 *=============================================================================================*/
int BaseSelectionPlanner::Check_Both_Bases_Have_Build_Area(StringClass * report)
{
	int violations = 0;
	if (!Built || !HasGdiSite || !HasNodSite) return violations;

	float gdi_fraction = Compute_Buildable_Fraction(GdiSite.Position,GdiSite.ProtectedRadius);
	float nod_fraction = Compute_Buildable_Fraction(NodSite.Position,NodSite.ProtectedRadius);

	if (gdi_fraction <= 0.0f) {
		violations++;
		if (report) *report += "GDI base site has no buildable area within its own protected radius\n";
	}
	if (nod_fraction <= 0.0f) {
		violations++;
		if (report) *report += "Nod base site has no buildable area within its own protected radius\n";
	}
	return violations;
}


int BaseSelectionPlanner::Check_Both_Bases_Have_Resource_Access(StringClass * report)
{
	int violations = 0;
	if (!Built || !HasGdiSite || !HasNodSite) return violations;

	if (GdiSite.InitialResourceTravelTimeSeconds < 0.0f) {
		violations++;
		if (report) *report += "GDI base site has no reachable resource candidate\n";
	}
	if (NodSite.InitialResourceTravelTimeSeconds < 0.0f) {
		violations++;
		if (report) *report += "Nod base site has no reachable resource candidate\n";
	}
	return violations;
}


int BaseSelectionPlanner::Check_Connectivity_Requirements(bool allow_single_bridge_dependency,StringClass * report)
{
	int violations = 0;
	if (!Built || !HasGdiSite || !HasNodSite) return violations;

	int gdi_node = -1, nod_node = -1;
	StrategicPlanner::Find_Node_By_Id(GdiSite.Id,&gdi_node);
	StrategicPlanner::Find_Node_By_Id(NodSite.Id,&nod_node);

	TreePath path;
	if ((gdi_node >= 0) && (nod_node >= 0)) Find_Tree_Path(gdi_node,nod_node,&path);

	if (!path.Found) {
		violations++;
		if (report) *report += "no road-graph route between the selected base pair\n";
	}

	if (!allow_single_bridge_dependency && path.Found && (Count_Bridge_Edges_On_Path(path) >= 1)) {
		violations++;
		if (report) *report += "selected base pair depends on a single bridge with no alternate route\n";
	}

	return violations;
}


int BaseSelectionPlanner::Check_Travel_Time_Within_Threshold(float max_resource_time_difference,
																					float max_base_to_base_time,StringClass * report)
{
	int violations = 0;
	if (!Built || !HasGdiSite || !HasNodSite) return violations;

	if (LastMetrics.ResourceTravelTimeDifferenceSeconds > max_resource_time_difference) {
		violations++;
		if (report) *report += "resource travel-time difference exceeds the configured threshold\n";
	}
	if (LastMetrics.BaseToBaseTravelTimeSeconds > max_base_to_base_time) {
		violations++;
		if (report) *report += "base-to-base travel time exceeds the configured threshold\n";
	}
	return violations;
}
