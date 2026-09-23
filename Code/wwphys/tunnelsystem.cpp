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
 *                 Project Name : wwphys                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwphys/tunnelsystem.cpp                      $*
 *                                                                                             *
 *	V2 roadmap Section 12 / 12A, Phase 10.  See tunnelsystem.h.                                 *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "tunnelsystem.h"

#include "aabox.h"
#include "assetmgr.h"
#include "coltype.h"
#include "crc.h"
#include "dynamicmeshbuilder.h"
#include "escarpment.h"
#include "lightphys.h"
#include "pscene.h"
#include "rendobj.h"
#include "shader.h"
#include "shadermgr.h"
#include "staticphys.h"
#include "vertmaterial.h"
#include "worldboundsservice.h"
#include "worldlightmanager.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"


//	A bore is a floor, walls and a roof; all of them stop everything a floor or a wall stops.
static const int TUNNEL_COLLISION_BITS =
	COLLISION_TYPE_PHYSICAL | COLLISION_TYPE_PROJECTILE |
	COLLISION_TYPE_VEHICLE | COLLISION_TYPE_CAMERA;

//	Modules overlap their neighbour by this much so consecutive boxes leave no seam to snag on.
static const float MODULE_SEAM_OVERLAP = 0.05f;

//	How far past the outer wall a headwall extends each side, as a multiple of wall thickness.
static const float HEADWALL_WING_WALLS = 2.0f;

//	Presentation only (Section 13 of PROJECT_RULES: never authority).  A tunnel may take at most
//	this many interior lights and all tunnels together at most this share of the dynamic budget.
static const int		MAX_LIGHTS_PER_TUNNEL	= 4;
static const int		MAX_TUNNEL_LIGHTS			= WORLD_LIGHT_MAX_DYNAMIC_LIGHTS / 4;
static const float	TUNNEL_LIGHT_INTENSITY	= 0.8f;

//	Cells whose surface is below the floor by more than this are left alone (open ground under
//	an approach); within it they are opened so the tube floor is the only surface there.
static const float	FLOOR_SURFACE_TOLERANCE	= 0.25f;


DynamicVectorClass<TunnelModuleDefinitionClass>	TunnelSystem::Modules;
DynamicVectorClass<TunnelClass>						TunnelSystem::Tunnels;
DynamicVectorClass<StaticPhysClass *>				TunnelSystem::Pieces;
DynamicVectorClass<LightPhysClass *>				TunnelSystem::Lights;
DynamicVectorClass<int>									TunnelSystem::OpenedCells;
int		TunnelSystem::NextID				= 1;
int		TunnelSystem::MissingModels	= 0;
bool		TunnelSystem::GeometryBuilt	= false;
bool		TunnelSystem::CliffCutsApplied	= false;


const char * Tunnel_Class_Name(TunnelClassType type)
{
	switch (type) {
		case TUNNEL_CLASS_INFANTRY_ONLY:	return "INFANTRY_ONLY";
		case TUNNEL_CLASS_VEHICLE:			return "VEHICLE";
		case TUNNEL_CLASS_MIXED:			return "MIXED";
		case TUNNEL_CLASS_UNDERPASS:		return "UNDERPASS";
		case TUNNEL_CLASS_CAVE_SHORTCUT:	return "CAVE_SHORTCUT";
		default:									return "UNKNOWN";
	}
}


bool Tunnel_Class_Carries_Vehicles(TunnelClassType type)
{
	return (type == TUNNEL_CLASS_VEHICLE) || (type == TUNNEL_CLASS_MIXED) || (type == TUNNEL_CLASS_UNDERPASS);
}


/***********************************************************************************************
 * Data classes                                                                                *
 *=============================================================================================*/
TunnelClearanceClass::TunnelClearanceClass(void) :
	InternalWidth(0.0f), InternalHeight(0.0f), LateralSafety(0.0f), VerticalSafety(0.0f),
	MaxGradeRadians(0.0f), MinBendRadius(0.0f), MinPortalStraight(0.0f)
{
}


bool TunnelClearanceClass::operator == (const TunnelClearanceClass & that) const
{
	return (InternalWidth == that.InternalWidth) && (InternalHeight == that.InternalHeight) &&
			 (LateralSafety == that.LateralSafety) && (VerticalSafety == that.VerticalSafety) &&
			 (MaxGradeRadians == that.MaxGradeRadians) && (MinBendRadius == that.MinBendRadius) &&
			 (MinPortalStraight == that.MinPortalStraight);
}


TunnelModuleDefinitionClass::TunnelModuleDefinitionClass(void) :
	Cave(false), Length(0.0f), ClearanceWidth(0.0f), ClearanceHeight(0.0f), MaxTurnRadians(0.0f),
	MaxGradeRadians(0.0f), WallThickness(0.5f), PortalCompatible(true), LightAnchorSpacing(0.0f),
	EntrySocket(true), ExitSocket(true)
{
}


bool TunnelModuleDefinitionClass::operator == (const TunnelModuleDefinitionClass & that) const
{
	return (Name == that.Name) && (Model == that.Model) && (Cave == that.Cave) && (Length == that.Length) &&
			 (ClearanceWidth == that.ClearanceWidth) && (ClearanceHeight == that.ClearanceHeight) &&
			 (MaxTurnRadians == that.MaxTurnRadians) && (MaxGradeRadians == that.MaxGradeRadians) &&
			 (WallThickness == that.WallThickness) && (PortalCompatible == that.PortalCompatible) &&
			 (LightAnchorSpacing == that.LightAnchorSpacing);
}


TunnelModuleInstanceClass::TunnelModuleInstanceClass(void) :
	DefinitionIndex(-1), Transform(true), Length(0.0f), UsesModel(false)
{
}


bool TunnelModuleInstanceClass::operator == (const TunnelModuleInstanceClass & that) const
{
	return (DefinitionIndex == that.DefinitionIndex) && (Transform == that.Transform) &&
			 (Length == that.Length) && (UsesModel == that.UsesModel);
}


TunnelClass::TunnelClass(void) :
	ID(-1), Class(TUNNEL_CLASS_INFANTRY_ONLY), Start(0.0f,0.0f,0.0f), End(0.0f,0.0f,0.0f),
	WallThickness(0.5f), MinCover(1.0f), PortalStationA(0.0f), PortalStationB(0.0f),
	HeadwallTopA(0.0f), HeadwallTopB(0.0f), LayoutBuilt(false)
{
}


bool TunnelClass::operator == (const TunnelClass & that) const
{
	return (ID == that.ID) && (Name == that.Name) && (Class == that.Class) &&
			 (ModuleName == that.ModuleName) && (Start == that.Start) && (End == that.End) &&
			 (Clearance == that.Clearance) && (WallThickness == that.WallThickness) &&
			 (MinCover == that.MinCover);
}


float TunnelClass::Get_Plan_Length(void) const
{
	float dx = End.X - Start.X;
	float dy = End.Y - Start.Y;
	return WWMath::Sqrt((dx * dx) + (dy * dy));
}


float TunnelClass::Get_Length(void) const
{
	return (End - Start).Length();
}


Vector3 TunnelClass::Get_Plan_Axis(void) const
{
	Vector3 axis(End.X - Start.X,End.Y - Start.Y,0.0f);
	float len = axis.Length();
	if (len <= WWMATH_EPSILON) {
		return Vector3(1.0f,0.0f,0.0f);
	}
	return axis / len;
}


float TunnelClass::Get_Grade_Radians(void) const
{
	float plan = Get_Plan_Length();
	if (plan <= WWMATH_EPSILON) {
		return 0.0f;
	}
	return atanf(WWMath::Fabs(End.Z - Start.Z) / plan);
}


float TunnelClass::Get_Floor_Height_At(float plan_station) const
{
	float plan = Get_Plan_Length();
	if (plan <= WWMATH_EPSILON) {
		return Start.Z;
	}
	float t = WWMath::Clamp(plan_station / plan,0.0f,1.0f);
	return Start.Z + ((End.Z - Start.Z) * t);
}


Matrix3D TunnelClass::Get_Frame(void) const
{
	Vector3 x = End - Start;
	float len = x.Length();
	if (len <= WWMATH_EPSILON) {
		x.Set(1.0f,0.0f,0.0f);
	} else {
		x /= len;
	}
	Vector3 up(0.0f,0.0f,1.0f);
	Vector3 y;
	Vector3::Cross_Product(up,x,&y);
	if (y.Length() <= WWMATH_EPSILON) {
		y.Set(0.0f,1.0f,0.0f);
	}
	y.Normalize();
	Vector3 z;
	Vector3::Cross_Product(x,y,&z);
	z.Normalize();
	return Matrix3D(x,y,z,Start);
}


void TunnelClass::Project(float x,float y,float * station_out,float * lateral_out) const
{
	Vector3 axis = Get_Plan_Axis();
	float dx = x - Start.X;
	float dy = y - Start.Y;
	if (station_out != nullptr) *station_out = (dx * axis.X) + (dy * axis.Y);
	if (lateral_out != nullptr) *lateral_out = (dx * -axis.Y) + (dy * axis.X);
}


bool TunnelClass::Is_Point_Inside(const Vector3 & point) const
{
	float station,lateral;
	Project(point.X,point.Y,&station,&lateral);
	if ((station < 0.0f) || (station > Get_Plan_Length())) {
		return false;
	}
	if (WWMath::Fabs(lateral) > (Clearance.Get_Bore_Width() * 0.5f)) {
		return false;
	}
	float floor = Get_Floor_Height_At(station);
	return (point.Z >= (floor - FLOOR_SURFACE_TOLERANCE)) &&
			 (point.Z <= (floor + Clearance.Get_Bore_Height()));
}


void TunnelClass::Get_Interior_Bounds(AABoxClass * bounds_out) const
{
	if (bounds_out == nullptr) {
		return;
	}
	float half = Clearance.Get_Bore_Width() * 0.5f;
	Vector3 axis = Get_Plan_Axis();
	Vector3 side(-axis.Y * half,axis.X * half,0.0f);
	float h = Clearance.Get_Bore_Height();

	Vector3 corners[4] = { Start + side, Start - side, End + side, End - side };
	Vector3 lo = corners[0];
	Vector3 hi = corners[0];
	for (int i = 1; i < 4; i++) {
		lo.Update_Min(corners[i]);
		hi.Update_Max(corners[i]);
	}
	hi.Z += h;
	bounds_out->Init_Min_Max(lo,hi);
}


/***********************************************************************************************
 * TunnelSystem lifecycle and modules                                                          *
 *=============================================================================================*/
void TunnelSystem::Init(void)
{
	Destroy_Geometry();
	Remove_Portal_Cuts();
	Tunnels.Delete_All();
	Modules.Delete_All();
	NextID = 1;
	Define_Default_Modules();
}


void TunnelSystem::Shutdown(void)
{
	Destroy_Geometry();
	Remove_Portal_Cuts();
	Tunnels.Delete_All();
	Modules.Delete_All();
}


void TunnelSystem::Define_Default_Modules(void)
{
	/*
	**	Two families.  Length is the authoring unit an artist builds a straight section to; the
	**	clearance is left at zero because no authored section exists yet, so nothing claims a
	**	bore it cannot prove -- every instance is the procedural tube until the asset arrives and
	**	its real clearance is declared here.  See docs/assets/TunnelModules.md.
	*/
	TunnelModuleDefinitionClass lined;
	lined.Name						= "tunnel_lined";
	lined.Model						= "ow_tunnel_lined_straight";
	lined.Cave						= false;
	lined.Length					= 10.0f;
	lined.MaxTurnRadians			= 0.0f;
	lined.MaxGradeRadians		= DEG_TO_RADF(30.0f);
	lined.WallThickness			= 0.5f;
	lined.PortalCompatible		= true;
	lined.LightAnchorSpacing	= 20.0f;
	lined.ExitSocket.Make_Identity();
	lined.ExitSocket.Set_Translation(Vector3(lined.Length,0.0f,0.0f));
	Define_Module(lined);

	TunnelModuleDefinitionClass cave;
	cave.Name						= "cave_rock";
	cave.Model						= "ow_cave_rock_straight";
	cave.Cave						= true;
	cave.Length						= 10.0f;
	cave.MaxTurnRadians			= 0.0f;
	cave.MaxGradeRadians			= DEG_TO_RADF(30.0f);
	cave.WallThickness			= 1.0f;
	cave.PortalCompatible		= true;
	cave.LightAnchorSpacing		= 0.0f;		// a cave is dark; the mouths light it
	cave.ExitSocket.Make_Identity();
	cave.ExitSocket.Set_Translation(Vector3(cave.Length,0.0f,0.0f));
	Define_Module(cave);
}


bool TunnelSystem::Define_Module(const TunnelModuleDefinitionClass & definition)
{
	if ((definition.Name.Get_Length() == 0) || (definition.Length <= WWMATH_EPSILON)) {
		return false;
	}
	int existing = Find_Module(definition.Name.Peek_Buffer());
	if (existing >= 0) {
		Modules[existing] = definition;
	} else {
		Modules.Add(definition);
	}
	return true;
}


int TunnelSystem::Get_Module_Count(void)
{
	return Modules.Count();
}


const TunnelModuleDefinitionClass * TunnelSystem::Peek_Module(int index)
{
	if ((index < 0) || (index >= Modules.Count())) {
		return nullptr;
	}
	return &Modules[index];
}


int TunnelSystem::Find_Module(const char * name)
{
	if (name == nullptr) {
		return -1;
	}
	for (int i = 0; i < Modules.Count(); i++) {
		if (::strcmp(Modules[i].Name.Peek_Buffer(),name) == 0) {
			return i;
		}
	}
	return -1;
}


/***********************************************************************************************
 * Tunnels                                                                                     *
 *=============================================================================================*/
int TunnelSystem::Add_Tunnel(const TunnelClass & tunnel)
{
	//	Nothing at startup is obliged to call Init, so the default modules arrive with the first
	//	tunnel that needs them.
	if (Modules.Count() == 0) {
		Define_Default_Modules();
	}

	TunnelClass copy = tunnel;
	copy.ID = NextID++;
	copy.LayoutBuilt = false;
	copy.Modules.Delete_All();
	copy.LightAnchors.Delete_All();
	Tunnels.Add(copy);
	return copy.ID;
}


bool TunnelSystem::Remove_Tunnel(int id)
{
	for (int i = 0; i < Tunnels.Count(); i++) {
		if (Tunnels[i].ID == id) {
			Tunnels.Delete(i);
			return true;
		}
	}
	return false;
}


void TunnelSystem::Clear_Tunnels(void)
{
	Destroy_Geometry();
	Remove_Portal_Cuts();
	Tunnels.Delete_All();
}


int TunnelSystem::Get_Tunnel_Count(void)
{
	return Tunnels.Count();
}


const TunnelClass * TunnelSystem::Peek_Tunnel(int index)
{
	if ((index < 0) || (index >= Tunnels.Count())) {
		return nullptr;
	}
	return &Tunnels[index];
}


const TunnelClass * TunnelSystem::Find_Tunnel(int id)
{
	for (int i = 0; i < Tunnels.Count(); i++) {
		if (Tunnels[i].ID == id) {
			return &Tunnels[i];
		}
	}
	return nullptr;
}


/***********************************************************************************************
 * TunnelSystem::Layout_Tunnel -- module chain by sockets, portal stations, headwalls, lights   *
 *                                                                                             *
 *	The chain starts with the first module's entry socket on the tunnel's own frame at Start;    *
 *	every next module's entry socket is the previous one's exit socket, composed.  Whole modules   *
 *	are laid while they fit; the remainder, if any, is one shortened procedural piece, because a    *
 *	fixed-length authored section cannot be cut.                                                    *
 *=============================================================================================*/
bool TunnelSystem::Layout_Tunnel(TunnelClass & tunnel)
{
	tunnel.Modules.Delete_All();
	tunnel.LightAnchors.Delete_All();
	tunnel.LayoutBuilt = false;

	int def_index = Find_Module(tunnel.ModuleName.Peek_Buffer());
	if (def_index < 0) {
		return false;
	}
	const TunnelModuleDefinitionClass & def = Modules[def_index];

	float length = tunnel.Get_Length();
	if ((length <= WWMATH_EPSILON) || !tunnel.Clearance.Is_Valid()) {
		return false;
	}

	//	A model is only used where its own declared clearance covers what this tunnel needs and
	//	its own geometry tolerates the grade.  Otherwise the procedural tube, sized exactly.
	bool model_fits = (def.Model.Get_Length() > 0) &&
							(def.ClearanceWidth >= tunnel.Clearance.Get_Bore_Width()) &&
							(def.ClearanceHeight >= tunnel.Clearance.Get_Bore_Height()) &&
							(def.MaxGradeRadians >= tunnel.Get_Grade_Radians());

	Matrix3D socket = tunnel.Get_Frame();
	float laid = 0.0f;
	while ((length - laid) > WWMATH_EPSILON) {
		TunnelModuleInstanceClass inst;
		inst.DefinitionIndex	= def_index;
		inst.Transform			= socket;
		float remaining = length - laid;
		if (remaining >= def.Length) {
			inst.Length = def.Length;
			inst.UsesModel = model_fits;
			socket = socket * def.ExitSocket;
		} else {
			inst.Length = remaining;
			inst.UsesModel = false;
			Matrix3D partial(true);
			partial.Set_Translation(Vector3(remaining,0.0f,0.0f));
			socket = socket * partial;
		}
		tunnel.Modules.Add(inst);
		laid += inst.Length;
	}

	/*
	**	Portal stations.  Walking in from each end along the centre line, the first station where
	**	the ground stands clear of the roof by MinCover is where the covered bore begins -- the
	**	mouth.  Everything outward of it is approach, whose ground Compute_Portal_Cut_Cells opens.
	*/
	float plan = tunnel.Get_Plan_Length();
	HeightfieldClass * field = WorldTerrainSystem::Peek_Heightfield();
	float step = (field != nullptr) ? WWMath::Max(0.25f,field->Get_Cell_Size() * 0.5f) : 1.0f;
	float roof_clear = tunnel.Get_Outer_Height() + tunnel.MinCover;
	Vector3 axis = tunnel.Get_Plan_Axis();

	tunnel.PortalStationA = plan * 0.5f;
	for (float s = 0.0f; s <= plan; s += step) {
		float h;
		if (!WorldTerrainSystem::Sample_Height(tunnel.Start.X + axis.X * s,tunnel.Start.Y + axis.Y * s,&h)) continue;
		if (h >= (tunnel.Get_Floor_Height_At(s) + roof_clear)) { tunnel.PortalStationA = s; break; }
	}
	tunnel.PortalStationB = plan * 0.5f;
	for (float s = plan; s >= 0.0f; s -= step) {
		float h;
		if (!WorldTerrainSystem::Sample_Height(tunnel.Start.X + axis.X * s,tunnel.Start.Y + axis.Y * s,&h)) continue;
		if (h >= (tunnel.Get_Floor_Height_At(s) + roof_clear)) { tunnel.PortalStationB = s; break; }
	}
	if (tunnel.PortalStationB < tunnel.PortalStationA) {
		tunnel.PortalStationB = tunnel.PortalStationA;
	}

	//	Headwall tops: the highest ground across the mouth just inside each portal, so the wall
	//	closes whatever the cut left between the roof and the surviving hillside.
	float half = tunnel.Get_Outer_Half_Width() + (HEADWALL_WING_WALLS * tunnel.WallThickness);
	float cell = (field != nullptr) ? field->Get_Cell_Size() : 1.0f;
	Vector3 side(-axis.Y,axis.X,0.0f);
	for (int end = 0; end < 2; end++) {
		float s = (end == 0) ? WWMath::Min(plan,tunnel.PortalStationA + cell) : WWMath::Max(0.0f,tunnel.PortalStationB - cell);
		float top = tunnel.Get_Floor_Height_At(s) + tunnel.Get_Outer_Height();
		for (float l = -half; l <= half; l += WWMath::Max(0.5f,cell * 0.5f)) {
			float h;
			if (WorldTerrainSystem::Sample_Height(tunnel.Start.X + axis.X * s + side.X * l,
															  tunnel.Start.Y + axis.Y * s + side.Y * l,&h)) {
				top = WWMath::Max(top,h);
			}
		}
		if (end == 0) tunnel.HeadwallTopA = top; else tunnel.HeadwallTopB = top;
	}

	//	Light anchors: interior only, evenly spaced, capped per tunnel.  The first and last are the
	//	lighting transition at each mouth.
	if (def.LightAnchorSpacing > 0.0f) {
		float covered = tunnel.PortalStationB - tunnel.PortalStationA;
		int count = (int)(covered / def.LightAnchorSpacing) + 1;
		if (count > MAX_LIGHTS_PER_TUNNEL) count = MAX_LIGHTS_PER_TUNNEL;
		for (int i = 0; (i < count) && (covered > 0.0f); i++) {
			float s = tunnel.PortalStationA + (covered * ((float)i + 0.5f) / (float)count);
			Vector3 p(tunnel.Start.X + axis.X * s,tunnel.Start.Y + axis.Y * s,
						 tunnel.Get_Floor_Height_At(s) + (tunnel.Clearance.Get_Bore_Height() * 0.85f));
			tunnel.LightAnchors.Add(p);
		}
	}

	tunnel.LayoutBuilt = true;
	return true;
}


bool TunnelSystem::Build_Layout(void)
{
	bool all = true;
	for (int i = 0; i < Tunnels.Count(); i++) {
		if (!Layout_Tunnel(Tunnels[i])) {
			all = false;
		}
	}
	return all;
}


/***********************************************************************************************
 * TunnelSystem::Compute_Portal_Cut_Cells                                                      *
 *=============================================================================================*/
int TunnelSystem::Compute_Portal_Cut_Cells(const TunnelClass & tunnel,DynamicVectorClass<int> & cells_out)
{
	int count_x = WorldTerrainSystem::Get_Cell_Count_X();
	int count_y = WorldTerrainSystem::Get_Cell_Count_Y();
	HeightfieldClass * field = WorldTerrainSystem::Peek_Heightfield();
	if ((count_x <= 0) || (count_y <= 0) || (field == nullptr)) {
		return 0;
	}

	float cell = field->Get_Cell_Size();
	float half = tunnel.Get_Outer_Half_Width() + (0.5f * cell);
	float plan = tunnel.Get_Plan_Length();
	float roof_clear = tunnel.Get_Outer_Height() + tunnel.MinCover;

	//	Only cells the bore's own plan rectangle can touch are visited.
	AABoxClass box;
	tunnel.Get_Interior_Bounds(&box);
	Vector3 lo = box.Center - box.Extent - Vector3(half,half,0.0f);
	Vector3 hi = box.Center + box.Extent + Vector3(half,half,0.0f);
	int cx0,cy0,cx1,cy1;
	if (!WorldTerrainSystem::World_To_Cell(WWMath::Max(lo.X,field->Get_Origin().X),
														WWMath::Max(lo.Y,field->Get_Origin().Y),&cx0,&cy0)) {
		cx0 = 0; cy0 = 0;
	}
	if (!WorldTerrainSystem::World_To_Cell(hi.X,hi.Y,&cx1,&cy1)) {
		cx1 = count_x - 1;
		cy1 = count_y - 1;
	}

	int added = 0;
	for (int cy = cy0; cy <= cy1; cy++) {
		for (int cx = cx0; cx <= cx1; cx++) {
			Vector3 centre;
			float cell_lo,cell_hi;
			if (!WorldTerrainSystem::Get_Cell_Center(cx,cy,&centre)) continue;
			if (!WorldTerrainSystem::Get_Cell_Height_Range(cx,cy,&cell_lo,&cell_hi)) continue;

			float station,lateral;
			tunnel.Project(centre.X,centre.Y,&station,&lateral);
			if ((station < 0.0f) || (station > plan)) continue;
			if (WWMath::Fabs(lateral) > half) continue;

			float floor = tunnel.Get_Floor_Height_At(station);
			if (cell_hi < (floor - FLOOR_SURFACE_TOLERANCE)) continue;	// open ground below the tube
			if (cell_lo > (floor + roof_clear)) continue;					// solid cover above the roof

			cells_out.Add((cy * count_x) + cx);
			added++;
		}
	}
	return added;
}


int TunnelSystem::Apply_Portal_Cuts(void)
{
	Remove_Portal_Cuts();

	int count_x = WorldTerrainSystem::Get_Cell_Count_X();
	for (int i = 0; i < Tunnels.Count(); i++) {
		DynamicVectorClass<int> cells;
		Compute_Portal_Cut_Cells(Tunnels[i],cells);
		for (int c = 0; c < cells.Count(); c++) {
			int index = cells[c];
			int cx = index % count_x;
			int cy = index / count_x;
			if (WorldTerrainSystem::Is_Cell_Hole(cx,cy)) continue;		// another tunnel's, or already open
			if (WorldTerrainSystem::Set_Cell_Hole(cx,cy,true)) {
				OpenedCells.Add(index);
			}
		}

		//	A cliff face the bore passes through is cut where the centre line crosses it: a disc
		//	of the outer half width plus one wall, centred on each face crossing.
		const TunnelClass & t = Tunnels[i];
		for (int e = 0; e < EscarpmentSystem::Get_Escarpment_Count(); e++) {
			EscarpmentClass * esc = EscarpmentSystem::Peek_Escarpment(e);
			if (esc == nullptr) continue;
			for (int p = 1; p < esc->Get_Point_Count(); p++) {
				const Vector3 & a = esc->Get_Point(p - 1).Position;
				const Vector3 & b = esc->Get_Point(p).Position;
				//	Plan-view segment/segment intersection with the bore's centre line.
				float d1x = t.End.X - t.Start.X, d1y = t.End.Y - t.Start.Y;
				float d2x = b.X - a.X, d2y = b.Y - a.Y;
				float den = (d1x * d2y) - (d1y * d2x);
				if (WWMath::Fabs(den) <= WWMATH_EPSILON) continue;
				float ox = a.X - t.Start.X, oy = a.Y - t.Start.Y;
				float u = ((ox * d2y) - (oy * d2x)) / den;
				float v = ((ox * d1y) - (oy * d1x)) / den;
				if ((u < 0.0f) || (u > 1.0f) || (v < 0.0f) || (v > 1.0f)) continue;
				Vector3 hit(t.Start.X + d1x * u,t.Start.Y + d1y * u,0.0f);
				EscarpmentSystem::Add_Portal_Cut(hit,t.Get_Outer_Half_Width() + t.WallThickness);
				CliffCutsApplied = true;
			}
		}
	}

	if (PhysicsSceneClass::Get_Instance() != nullptr) {
		WorldTerrainSystem::Update_Collision();
		if (CliffCutsApplied && EscarpmentSystem::Has_Geometry()) {
			EscarpmentSystem::Build_Geometry();
		}
	}
	return OpenedCells.Count();
}


void TunnelSystem::Remove_Portal_Cuts(void)
{
	int count_x = WorldTerrainSystem::Get_Cell_Count_X();
	if (count_x > 0) {
		for (int i = 0; i < OpenedCells.Count(); i++) {
			WorldTerrainSystem::Set_Cell_Hole(OpenedCells[i] % count_x,OpenedCells[i] / count_x,false);
		}
	}
	bool had_cells = (OpenedCells.Count() > 0);
	OpenedCells.Delete_All();

	bool had_cliff_cuts = CliffCutsApplied;
	if (CliffCutsApplied) {
		EscarpmentSystem::Clear_Portal_Cuts();
		CliffCutsApplied = false;
	}

	if (PhysicsSceneClass::Get_Instance() != nullptr) {
		if (had_cells) {
			WorldTerrainSystem::Update_Collision();
		}
		if (had_cliff_cuts && EscarpmentSystem::Has_Geometry()) {
			EscarpmentSystem::Build_Geometry();
		}
	}
}


/***********************************************************************************************
 * TunnelSystem::Breaches_Boundary -- Section 12.4                                             *
 *                                                                                             *
 *	Both outer edges of the bore and its centre line must stay out of the cliff band and away    *
 *	from the hard boundary, and so must both mouths.  An unconfigured world has no boundary to     *
 *	breach.                                                                                         *
 *=============================================================================================*/
bool TunnelSystem::Breaches_Boundary(const TunnelClass & tunnel)
{
	if (!WorldBoundsService::Is_Configured()) {
		return false;
	}

	Vector3 axis = tunnel.Get_Plan_Axis();
	float half = tunnel.Get_Outer_Half_Width() + (HEADWALL_WING_WALLS * tunnel.WallThickness);
	Vector2 side(-axis.Y * half,axis.X * half);
	Vector2 a(tunnel.Start.X,tunnel.Start.Y);
	Vector2 b(tunnel.End.X,tunnel.End.Y);

	Vector2 lines[3][2] = { { a,b }, { a + side,b + side }, { a - side,b - side } };
	for (int i = 0; i < 3; i++) {
		if (WorldBoundsService::Segment_Crosses_Hard_Boundary(lines[i][0],lines[i][1])) {
			return true;
		}
		const int SAMPLES = 16;
		for (int k = 0; k <= SAMPLES; k++) {
			float t = (float)k / (float)SAMPLES;
			Vector2 p = lines[i][0] + ((lines[i][1] - lines[i][0]) * t);
			if (WorldBoundsService::Classify_Point(p) >= WORLD_ZONE_CLIFF_BAND) {
				return true;
			}
		}
	}
	return false;
}


/***********************************************************************************************
 * Geometry                                                                                    *
 *=============================================================================================*/
void TunnelSystem::Add_Piece(RenderObjClass * model,const Matrix3D & transform,bool collides)
{
	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	model->Set_Collision_Type(collides ? TUNNEL_COLLISION_BITS : 0);
	StaticPhysClass * phys = new StaticPhysClass;
	phys->Set_Model(model);
	phys->Set_Transform(transform);
	scene->Add_Static_Object(phys);
	Pieces.Add(phys);
}


static DynamicMeshClass * Finish_Mesh(DynamicMeshBuilderClass & builder,bool cave)
{
	DynamicMeshClass * mesh = builder.Detach_Mesh();
	if (mesh == nullptr) {
		return nullptr;
	}
	//	What the surface is drawn in is asked of the layer that owns it: a lined bore is a
	//	structure, the same program a bridge uses; a cave is rock, the terrain's.
	ShaderClass shader;
	VertexMaterialClass * vmat = nullptr;
	if (ShaderManagerClass::Get_Pass_State(cave ? MATERIAL_PROGRAM_TERRAIN : MATERIAL_PROGRAM_BRIDGE,
														0,&shader,&vmat)) {
		mesh->Set_Shader(shader);
		if (vmat != nullptr) {
			mesh->Set_Vertex_Material(vmat,true);
		}
	}
	REF_PTR_RELEASE(vmat);
	mesh->Set_Dirty();
	return mesh;
}


//	The procedural tube for one module, in module space: X along the bore from the entry socket,
//	Y across, Z up with the floor surface at Z = 0.
static void Emit_Tube(DynamicMeshBuilderClass & builder,float length,float bore_half_width,
							 float bore_height,float wall)
{
	float half_len = (length * 0.5f) + MODULE_SEAM_OVERLAP;
	float mid = length * 0.5f;
	float outer_half = bore_half_width + wall;

	builder.Box(half_len,outer_half,0.0f,-wall,Vector3(mid,0.0f,0.0f));										// floor
	builder.Box(half_len,outer_half,bore_height + wall,bore_height,Vector3(mid,0.0f,0.0f));				// roof
	builder.Box(half_len,wall * 0.5f,bore_height,0.0f,Vector3(mid,bore_half_width + (wall * 0.5f),0.0f));	// left wall
	builder.Box(half_len,wall * 0.5f,bore_height,0.0f,Vector3(mid,-(bore_half_width + (wall * 0.5f)),0.0f));	// right wall
}


bool TunnelSystem::Build_Tunnel_Geometry(const TunnelClass & tunnel)
{
	if (!tunnel.LayoutBuilt) {
		return false;
	}

	float bore_half = tunnel.Clearance.Get_Bore_Width() * 0.5f;
	float bore_height = tunnel.Clearance.Get_Bore_Height();
	float wall = tunnel.WallThickness;

	for (int m = 0; m < tunnel.Modules.Count(); m++) {

		const TunnelModuleInstanceClass & inst = tunnel.Modules[m];
		const TunnelModuleDefinitionClass & def = Modules[inst.DefinitionIndex];

		RenderObjClass * model = nullptr;
		if (inst.UsesModel && (WW3DAssetManager::Get_Instance() != nullptr)) {
			model = WW3DAssetManager::Get_Instance()->Create_Render_Obj(def.Model.Peek_Buffer());
		}
		if (inst.UsesModel && (model == nullptr)) {
			MissingModels++;
		}

		if (model == nullptr) {
			DynamicMeshBuilderClass builder;
			builder.Begin_Count();
			Emit_Tube(builder,inst.Length,bore_half,bore_height,wall);
			if (!builder.Begin_Build()) {
				continue;
			}
			Emit_Tube(builder,inst.Length,bore_half,bore_height,wall);
			model = Finish_Mesh(builder,def.Cave);
			if (model == nullptr) {
				continue;
			}
		}

		Add_Piece(model,inst.Transform,true);
		model->Release_Ref();
	}

	//	Headwalls (Section 12.3 "blend into terrain/cliff face"): a slab across each mouth from
	//	the roof up to the highest surviving ground over it, in a flat (unpitched) frame.
	Vector3 axis = tunnel.Get_Plan_Axis();
	Vector3 side(-axis.Y,axis.X,0.0f);
	Vector3 up(0.0f,0.0f,1.0f);
	float wing_half = tunnel.Get_Outer_Half_Width() + (HEADWALL_WING_WALLS * wall);
	const TunnelModuleDefinitionClass & def0 = Modules[tunnel.Modules[0].DefinitionIndex];

	for (int end = 0; end < 2; end++) {
		float s = (end == 0) ? tunnel.PortalStationA : tunnel.PortalStationB;
		float roof = tunnel.Get_Floor_Height_At(s) + bore_height;
		float top = (end == 0) ? tunnel.HeadwallTopA : tunnel.HeadwallTopB;
		if (top <= roof + WWMATH_EPSILON) {
			continue;
		}
		Vector3 origin(tunnel.Start.X + axis.X * s,tunnel.Start.Y + axis.Y * s,roof);
		Matrix3D frame(axis,side,up,origin);

		DynamicMeshBuilderClass builder;
		builder.Begin_Count();
		builder.Box(wall,wing_half,(top - roof) + wall,0.0f);
		if (!builder.Begin_Build()) {
			continue;
		}
		builder.Box(wall,wing_half,(top - roof) + wall,0.0f);
		DynamicMeshClass * mesh = Finish_Mesh(builder,def0.Cave);
		if (mesh == nullptr) {
			continue;
		}
		Add_Piece(mesh,frame,true);
		mesh->Release_Ref();
	}

	//	Interior lighting, presentation only and within budget.
	for (int l = 0; l < tunnel.LightAnchors.Count(); l++) {
		if (Lights.Count() >= MAX_TUNNEL_LIGHTS) {
			break;
		}
		bool mouth = (l == 0) || (l == tunnel.LightAnchors.Count() - 1);
		float radius = WWMath::Max(tunnel.Clearance.Get_Bore_Width(),bore_height) * 1.5f;
		LightPhysClass * light = WorldLightManager::Create_Point_Light(
			tunnel.LightAnchors[l],Vector3(1.0f,0.9f,0.75f),radius * 0.25f,radius,
			mouth ? (TUNNEL_LIGHT_INTENSITY * 0.5f) : TUNNEL_LIGHT_INTENSITY,-1.0f);
		if (light != nullptr) {
			Lights.Add(light);
		}
	}

	return true;
}


bool TunnelSystem::Build_Geometry(void)
{
	Destroy_Geometry();

	if (PhysicsSceneClass::Get_Instance() == nullptr) {
		return false;
	}

	MissingModels = 0;
	int built = 0;
	for (int i = 0; i < Tunnels.Count(); i++) {
		if (!Tunnels[i].LayoutBuilt) {
			Layout_Tunnel(Tunnels[i]);
		}
		if (Build_Tunnel_Geometry(Tunnels[i])) {
			built++;
		}
	}

	GeometryBuilt = (built > 0);
	return GeometryBuilt;
}


void TunnelSystem::Destroy_Geometry(void)
{
	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	for (int i = 0; i < Pieces.Count(); i++) {
		if (scene != nullptr) {
			scene->Remove_Object(Pieces[i]);
		}
		Pieces[i]->Release_Ref();
	}
	Pieces.Delete_All();

	for (int i = 0; i < Lights.Count(); i++) {
		WorldLightManager::Remove_Dynamic_Light(Lights[i]);
	}
	Lights.Delete_All();

	GeometryBuilt = false;
	MissingModels = 0;
}


/***********************************************************************************************
 * Queries                                                                                     *
 *=============================================================================================*/
bool TunnelSystem::Find_Tunnel_At(const Vector3 & point,int * id_out)
{
	for (int i = 0; i < Tunnels.Count(); i++) {
		if (Tunnels[i].Is_Point_Inside(point)) {
			if (id_out != nullptr) *id_out = Tunnels[i].ID;
			return true;
		}
	}
	return false;
}


bool TunnelSystem::Conform_Point(float x,float y,float near_z,float * floor_out,int * id_out)
{
	for (int i = 0; i < Tunnels.Count(); i++) {
		const TunnelClass & t = Tunnels[i];
		float station,lateral;
		t.Project(x,y,&station,&lateral);
		if ((station < 0.0f) || (station > t.Get_Plan_Length())) continue;
		if (WWMath::Fabs(lateral) > (t.Clearance.Get_Bore_Width() * 0.5f)) continue;
		float floor = t.Get_Floor_Height_At(station);
		//	Only a caller actually down in the bore is on this floor; one on the hill above is not.
		if ((near_z < (floor - t.Clearance.Get_Bore_Height())) || (near_z > (floor + t.Clearance.Get_Bore_Height()))) continue;
		if (floor_out != nullptr) *floor_out = floor;
		if (id_out != nullptr) *id_out = t.ID;
		return true;
	}
	return false;
}


bool TunnelSystem::Overlaps_Footprint(const Vector3 & point,float radius,int * id_out)
{
	for (int i = 0; i < Tunnels.Count(); i++) {
		const TunnelClass & t = Tunnels[i];
		float station,lateral;
		t.Project(point.X,point.Y,&station,&lateral);
		float reach = t.Get_Outer_Half_Width() + (HEADWALL_WING_WALLS * t.WallThickness) + radius;
		if (WWMath::Fabs(lateral) > reach) continue;
		if ((station < -radius) || (station > (t.Get_Plan_Length() + radius))) continue;
		if (id_out != nullptr) *id_out = t.ID;
		return true;
	}
	return false;
}


bool TunnelSystem::Overlaps_Opening(const Vector3 & point,float radius,int * id_out)
{
	for (int i = 0; i < Tunnels.Count(); i++) {
		const TunnelClass & t = Tunnels[i];
		float station,lateral;
		t.Project(point.X,point.Y,&station,&lateral);
		float reach = t.Get_Outer_Half_Width() + (HEADWALL_WING_WALLS * t.WallThickness) + radius;
		if (WWMath::Fabs(lateral) > reach) continue;
		bool near_a = (station >= -radius) && (station <= (t.PortalStationA + radius));
		bool near_b = (station >= (t.PortalStationB - radius)) && (station <= (t.Get_Plan_Length() + radius));
		if (near_a || near_b) {
			if (id_out != nullptr) *id_out = t.ID;
			return true;
		}
	}
	return false;
}


unsigned int TunnelSystem::Get_Checksum(void)
{
	CRCEngine crc;
	int count = Tunnels.Count();
	crc(&count,sizeof(count));
	for (int i = 0; i < Tunnels.Count(); i++) {
		const TunnelClass & t = Tunnels[i];
		int q[10] = {
			(int)t.Class,
			(int)(t.Start.X * 100.0f),(int)(t.Start.Y * 100.0f),(int)(t.Start.Z * 100.0f),
			(int)(t.End.X * 100.0f),(int)(t.End.Y * 100.0f),(int)(t.End.Z * 100.0f),
			(int)(t.Clearance.Get_Bore_Width() * 100.0f),(int)(t.Clearance.Get_Bore_Height() * 100.0f),
			t.Modules.Count()
		};
		crc(q,sizeof(q));
	}
	int opened = OpenedCells.Count();
	crc(&opened,sizeof(opened));
	return (unsigned int)crc;
}
