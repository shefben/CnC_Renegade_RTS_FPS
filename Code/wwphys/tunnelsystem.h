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
 *                     $Archive:: /Commando/Code/wwphys/tunnelsystem.h                        $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	V2 roadmap Section 12 / 12A, Phase 10 -- the one owner of tunnel geometry.                   *
 *                                                                                             *
 *	The same split the bridge already has: this file owns tunnel data, the module chain, the      *
 *	collision/render pieces, the portal cuts it makes in the ground and in cliff faces, and the     *
 *	interior spatial bounds; Combat's TunnelPlanner decides where tunnels go.  A heightfield        *
 *	cannot represent a bore, so a tunnel is separate geometry: a chain of modules joined by           *
 *	explicit sockets (Section 12A.2), each either an authored W3D section or -- where no authored      *
 *	section exists or fits the required clearance -- the roadmap-approved simple procedural tube      *
 *	(floor, two walls, ceiling) built to exactly the clearance the tunnel's class demands.             *
 *                                                                                             *
 *	The ground above a bore is untouched: a heightfield is a surface, not a solid, so a tube            *
 *	underneath it collides with nothing.  Only where the surface passes through the tube's own          *
 *	volume -- the approaches and the mouths -- are cells opened, through WorldTerrainSystem's            *
 *	stock QF_HIDDEN holes, and any cliff face the bore passes through is cut with                          *
 *	EscarpmentSystem::Add_Portal_Cut.  A headwall at each mouth closes the gap between the tube           *
 *	roof and the ground the cut left above it.                                                             *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef TUNNELSYSTEM_H
#define TUNNELSYSTEM_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "vector.h"
#include "vector3.h"
#include "matrix3d.h"
#include "wwstring.h"

class StaticPhysClass;
class LightPhysClass;
class AABoxClass;


//	Section 12A.1's five classes, named exactly as the roadmap lists them.
enum TunnelClassType
{
	TUNNEL_CLASS_INFANTRY_ONLY = 0,
	TUNNEL_CLASS_VEHICLE,
	TUNNEL_CLASS_MIXED,
	TUNNEL_CLASS_UNDERPASS,
	TUNNEL_CLASS_CAVE_SHORTCUT,
	TUNNEL_CLASS_COUNT
};

const char *	Tunnel_Class_Name(TunnelClassType type);
bool				Tunnel_Class_Carries_Vehicles(TunnelClassType type);


/*
**	Section 12A.1.  Every number is supplied by the caller from WorldMetricsClass and the world
**	profile -- this layer invents none of them.  InternalWidth/InternalHeight are the traversal
**	envelope; the safety margins are added on top to give the clear bore.
*/
class TunnelClearanceClass
{
public:
	TunnelClearanceClass(void);

	bool	operator == (const TunnelClearanceClass & that) const;
	bool	operator != (const TunnelClearanceClass & that) const	{ return !(*this == that); }

	bool	Is_Valid(void) const	{ return (InternalWidth > 0.0f) && (InternalHeight > 0.0f) && (MaxGradeRadians > 0.0f); }

	float	Get_Bore_Width(void) const		{ return InternalWidth + (2.0f * LateralSafety); }
	float	Get_Bore_Height(void) const	{ return InternalHeight + VerticalSafety; }

	float	InternalWidth;
	float	InternalHeight;
	float	LateralSafety;			// per side
	float	VerticalSafety;
	float	MaxGradeRadians;
	float	MinBendRadius;			// 0 = no bend constraint stated
	float	MinPortalStraight;	// straight run required inside each mouth
};


/*
**	Section 12A.2.  A reusable module: sockets, clearance, curvature/grade support, collision
**	bounds, portal compatibility and lighting anchors.  Modules are joined socket to socket --
**	a module's ExitSocket, in its own space, is where the next module's EntrySocket goes --
**	never by bounding-box guessing.
**
**	ClearanceWidth/ClearanceHeight are what an authored model provides.  Zero means no authored
**	model is claimed for this module, so every instance is the procedural tube sized per tunnel.
*/
class TunnelModuleDefinitionClass
{
public:
	TunnelModuleDefinitionClass(void);

	bool	operator == (const TunnelModuleDefinitionClass & that) const;
	bool	operator != (const TunnelModuleDefinitionClass & that) const	{ return !(*this == that); }

	StringClass	Name;
	StringClass	Model;					// ow_ asset name; see docs/assets/TunnelModules.md
	bool			Cave;						// natural rock (terrain material) rather than a lined bore
	float			Length;					// entry socket to exit socket, straight
	float			ClearanceWidth;
	float			ClearanceHeight;
	float			MaxTurnRadians;		// per module; 0 = straight only
	float			MaxGradeRadians;		// grade the module's own geometry tolerates
	float			WallThickness;
	bool			PortalCompatible;		// may sit at a mouth
	float			LightAnchorSpacing;	// world units between interior light anchors; 0 = none
	Matrix3D		EntrySocket;
	Matrix3D		ExitSocket;
};


//	One placed module.  Transform is the module's entry socket in world space.
class TunnelModuleInstanceClass
{
public:
	TunnelModuleInstanceClass(void);

	bool	operator == (const TunnelModuleInstanceClass & that) const;
	bool	operator != (const TunnelModuleInstanceClass & that) const	{ return !(*this == that); }

	int			DefinitionIndex;
	Matrix3D		Transform;
	float			Length;			// may be shorter than the definition's for the last module
	bool			UsesModel;		// false = procedural tube
};


class TunnelClass
{
public:
	TunnelClass(void);

	bool	operator == (const TunnelClass & that) const;
	bool	operator != (const TunnelClass & that) const	{ return !(*this == that); }

	int					Get_ID(void) const					{ return ID; }

	//	Plan-view and 3D geometry of the straight bore, Start to End floor points.
	float					Get_Plan_Length(void) const;
	float					Get_Length(void) const;
	Vector3				Get_Plan_Axis(void) const;		// unit, Z = 0
	float					Get_Grade_Radians(void) const;
	float					Get_Floor_Height_At(float plan_station) const;
	float					Get_Outer_Half_Width(void) const	{ return (Clearance.Get_Bore_Width() * 0.5f) + WallThickness; }
	float					Get_Outer_Height(void) const		{ return Clearance.Get_Bore_Height() + WallThickness; }
	Matrix3D				Get_Frame(void) const;				// origin Start, X along the bore, Z up-ish

	//	Plan station and signed lateral offset of a point relative to the bore's centre line.
	void					Project(float x,float y,float * station_out,float * lateral_out) const;
	bool					Is_Point_Inside(const Vector3 & point) const;
	void					Get_Interior_Bounds(AABoxClass * bounds_out) const;

	int					ID;
	StringClass			Name;
	TunnelClassType	Class;
	StringClass			ModuleName;
	Vector3				Start;
	Vector3				End;
	TunnelClearanceClass	Clearance;
	float					WallThickness;
	float					MinCover;			// ground within this of the roof is opened and headwalled

	//	Filled by TunnelSystem::Build_Layout.
	DynamicVectorClass<TunnelModuleInstanceClass>	Modules;
	DynamicVectorClass<Vector3>						LightAnchors;
	float					PortalStationA;		// plan station where cover begins, from Start
	float					PortalStationB;		// plan station where cover ends, from Start
	float					HeadwallTopA;			// world Z
	float					HeadwallTopB;
	bool					LayoutBuilt;
};


class TunnelSystem
{
public:

	static void			Init(void);
	static void			Shutdown(void);

	/*
	**	Modules.  Define_Default_Modules declares the lined-bore and cave-rock families the
	**	planner uses; neither claims an authored model yet (see the asset doc), so both build as
	**	the procedural tube until an artist supplies one and its clearance is declared.
	*/
	static void			Define_Default_Modules(void);
	static bool			Define_Module(const TunnelModuleDefinitionClass & definition);
	static int			Get_Module_Count(void);
	static const TunnelModuleDefinitionClass *	Peek_Module(int index);
	static int			Find_Module(const char * name);

	static int			Add_Tunnel(const TunnelClass & tunnel);
	static bool			Remove_Tunnel(int id);
	static void			Clear_Tunnels(void);
	static int			Get_Tunnel_Count(void);
	static const TunnelClass *	Peek_Tunnel(int index);
	static const TunnelClass *	Find_Tunnel(int id);

	/*
	**	Module chain, portal stations, headwalls and light anchors, from the tunnel data and the
	**	terrain.  Pure with respect to the scene: the device-less self check lays tunnels out.
	*/
	static bool			Build_Layout(void);
	static bool			Layout_Tunnel(TunnelClass & tunnel);

	/*
	**	The cells a tunnel opens in the ground (Section 12 step 7): every cell inside the bore's
	**	outer footprint whose surface lies within the tube or less than MinCover above its roof.
	**	Ground clear above that stays; ground below the floor stays.  Pure -- returns cell indices
	**	(y * count_x + x) without touching the field.
	*/
	static int			Compute_Portal_Cut_Cells(const TunnelClass & tunnel,DynamicVectorClass<int> & cells_out);

	/*
	**	Opens the ground and every cliff face the bores pass through, for all tunnels, and remembers
	**	exactly which cells it opened so Remove_Portal_Cuts closes only those.  Terrain collision is
	**	refreshed by the caller's next WorldTerrainSystem::Update_Collision, cliff faces by the next
	**	EscarpmentSystem::Build_Geometry -- both of which this calls when a scene exists.
	*/
	static int			Apply_Portal_Cuts(void);
	static void			Remove_Portal_Cuts(void);
	static int			Get_Opened_Cell_Count(void)	{ return OpenedCells.Count(); }

	//	Section 12.4.  True if any part of the bore or its mouths leaves the playable interior.
	static bool			Breaches_Boundary(const TunnelClass & tunnel);

	static bool			Build_Geometry(void);
	static void			Destroy_Geometry(void);
	static bool			Has_Geometry(void)				{ return GeometryBuilt; }
	static int			Get_Piece_Count(void)			{ return Pieces.Count(); }
	static int			Get_Missing_Model_Count(void)	{ return MissingModels; }
	static int			Get_Light_Count(void)			{ return Lights.Count(); }

	//	Interior spatial bounds (Section 34 Phase 10 step 11).
	static bool			Find_Tunnel_At(const Vector3 & point,int * id_out);
	static bool			Conform_Point(float x,float y,float near_z,float * floor_out,int * id_out);
	//	The whole plan footprint -- mouths, approaches and the covered bore -- widened by `radius`.
	//	Anything that reshapes ground (a building's grading, a crater) there could open the roof.
	static bool			Overlaps_Footprint(const Vector3 & point,float radius,int * id_out);

	//	Only the openings -- each mouth and the approach in front of it -- widened by `radius`.  What
	//	does not reshape ground (a tree) is kept out of these alone.
	static bool			Overlaps_Opening(const Vector3 & point,float radius,int * id_out);

	static unsigned int	Get_Checksum(void);

private:

	static bool			Build_Tunnel_Geometry(const TunnelClass & tunnel);
	static void			Add_Piece(class RenderObjClass * model,const Matrix3D & transform,bool collides);

	static DynamicVectorClass<TunnelModuleDefinitionClass>	Modules;
	static DynamicVectorClass<TunnelClass>						Tunnels;
	static DynamicVectorClass<StaticPhysClass *>				Pieces;
	static DynamicVectorClass<LightPhysClass *>				Lights;
	static DynamicVectorClass<int>								OpenedCells;
	static int			NextID;
	static int			MissingModels;
	static bool			GeometryBuilt;
	static bool			CliffCutsApplied;
};

#endif	// TUNNELSYSTEM_H
