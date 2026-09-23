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
 *                     $Archive:: /Commando/Code/wwphys/escarpment.h                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	The vertical face a heightfield cannot hold.                                                *
 *                                                                                             *
 *	Roadmap Section 52B, Phase 64.  HeightfieldClass is a function of x and y, so it cannot     *
 *	represent a vertical face, an overhang or an undercut.  A cliff authored into it is a run   *
 *	of very steep cells: the slope query correctly calls them unwalkable, and they read as      *
 *	stretched ground texture, because there is nothing there that knows it is rock.             *
 *                                                                                             *
 *	An escarpment sits *beside* the heightfield rather than inside it.  It is a polyline with   *
 *	an upper and a lower height at each point, and it owns exactly one thing: the face          *
 *	between those two heights.  The heightfield stays the single authority for ground height    *
 *	on both benches -- Conform_To_Terrain reads the benches out of it rather than asserting     *
 *	them, and nothing here ever answers "how high is the ground at x,y".                        *
 *                                                                                             *
 *	Which side is down.  The face drops to the right of the direction of travel along the       *
 *	polyline, which is cross(direction,up).  Stating it one way and keeping to it is what       *
 *	lets the batter lean the top back into the hill instead of out over the drop.               *
 *                                                                                             *
 *	Missing art degrades the way every world system in Sections 17-23 degrades: with no cliff   *
 *	layer defined and no texture in the mix files, the face is still built, still collides,     *
 *	and is simply untextured.  Never a crash, never a stand-in file in the tree.                *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef ESCARPMENT_H
#define ESCARPMENT_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#ifndef VECTOR3_H
#include "vector3.h"
#endif

#ifndef VECTOR_H
#include "vector.h"
#endif

#ifndef WWSTRING_H
#include "wwstring.h"
#endif

class StaticPhysClass;
class AABoxClass;


/*
**	One station along an escarpment.  The position is on the map; the two heights are the
**	benches either side of the face at that station.
*/
class EscarpmentPointClass
{
public:

	EscarpmentPointClass(void) : Position(0.0f,0.0f,0.0f), UpperHeight(0.0f), LowerHeight(0.0f),
										  FaceInflection(0.0f) { }

	//	Only X and Y of Position are used.  Z is ignored; the two height fields are the answer.
	Vector3	Position;
	float		UpperHeight;
	float		LowerHeight;

	/*
	**	How far the middle of the face at this station bulges out from the straight line
	**	between its own toe and rim, in metres, positive being out over the drop.  Zero -- the
	**	default -- is the flat batter-only face this class built before, so an escarpment that
	**	says nothing about inflection comes out byte-identical to one built previously.  It is
	**	only read when the owning escarpment asks for more than one face row; with a single
	**	row there is no middle to move.
	*/
	float		FaceInflection;

	float		Get_Face_Height(void) const
	{
		float h = UpperHeight - LowerHeight;
		return (h > 0.0f) ? h : 0.0f;
	}

	bool	operator == (const EscarpmentPointClass & that) const
	{
		return (Position == that.Position) &&
				 (UpperHeight == that.UpperHeight) &&
				 (LowerHeight == that.LowerHeight) &&
				 (FaceInflection == that.FaceInflection);
	}
	bool	operator != (const EscarpmentPointClass & that) const	{ return !(*this == that); }
};


/*
**	One escarpment, placed.
*/
class EscarpmentClass
{
public:

	EscarpmentClass(void);
	EscarpmentClass(const EscarpmentClass & that);
	~EscarpmentClass(void);

	EscarpmentClass &	operator = (const EscarpmentClass & that);
	bool					operator == (const EscarpmentClass & that) const;
	bool					operator != (const EscarpmentClass & that) const	{ return !(*this == that); }

	void				Reset(void);

	void				Set_Name(const char * name);
	const char *	Get_Name(void) const					{ return Name.Peek_Buffer(); }
	void				Set_ID(int id)							{ ID = id; }
	int				Get_ID(void) const					{ return ID; }

	/*
	**	The polyline.  Points are in order along the face; the drop is to the right of travel.
	*/
	void				Add_Point(const EscarpmentPointClass & point);
	void				Add_Point(const Vector3 & position,float upper,float lower);
	int				Get_Point_Count(void) const		{ return Points.Count(); }
	const EscarpmentPointClass &	Get_Point(int index) const;
	EscarpmentPointClass *			Peek_Point(int index);
	void				Clear_Points(void);

	/*
	**	How far the top leans back into the hill, as metres of horizontal set-back per metre
	**	of face height.  Zero is a dead vertical wall.  A small batter is worth having: it
	**	keeps the face out of z-fighting range of the very steep heightfield cells that
	**	usually sit right behind it, and real rock is not plumb.
	*/
	void				Set_Batter(float batter)			{ Batter = (batter > 0.0f) ? batter : 0.0f; }
	float				Get_Batter(void) const				{ return Batter; }

	/*
	**	Which terrain layer dresses the face.  -1 means "the first layer of kind
	**	TERRAIN_LAYER_CLIFF", which is what Define_Default_Layers already provides, so an
	**	escarpment that says nothing still comes out as rock.
	*/
	void				Set_Layer_Index(int index)			{ LayerIndex = index; }
	int				Get_Layer_Index(void) const		{ return LayerIndex; }

	/*
	**	How many rows of quads the face is built from between toe and rim.  One -- the default
	**	-- is the single quad per segment this class has always produced.  More rows exist so
	**	a face can inflect between its ends instead of being a flat plane; they cost geometry
	**	and buy nothing on a face whose stations all have zero FaceInflection, so a caller
	**	that does not want shaping should leave this alone.
	*/
	void				Set_Face_Row_Count(int rows)		{ FaceRowCount = (rows > 1) ? rows : 1; }
	int				Get_Face_Row_Count(void) const	{ return FaceRowCount; }

	/*
	**	An apron of ground flaring out from the bottom of the face, in metres measured
	**	horizontally away from the high side.  This is the scree/debris skirt real rock piles
	**	up at its own base; it is drawn as part of the face so it shares the face's material
	**	and needs no second system.  Zero -- the default -- builds no apron at all.
	**
	**	It is deliberately drawn but NOT a separate collision surface: it lies flat on the
	**	lower bench, so the ground under it already stops everything it would stop, and giving
	**	it its own collider would put a second surface a few centimetres above the first.
	*/
	void				Set_Toe_Width(float width)			{ ToeWidth = (width > 0.0f) ? width : 0.0f; }
	float				Get_Toe_Width(void) const			{ return ToeWidth; }

	/*
	**	Total length along the polyline, and the tallest face on it.
	*/
	float				Get_Length(void) const;
	float				Get_Max_Face_Height(void) const;

	/*
	**	The nearest place on the polyline to a point on the map, and the benches there.
	**	Distance is unsigned perpendicular distance in the XY plane.
	*/
	bool				Sample(float x,float y,float * upper_out,float * lower_out,float * distance_out) const;

	/*
	**	Does the segment a..b cross this face?  This is the question a route check asks, and
	**	it is deliberately a 2D crossing test plus a height comparison rather than a mesh
	**	trace: a coarse graph wants to know that a wall is in the way, not where exactly.
	*/
	bool				Does_Segment_Cross(const Vector3 & a,const Vector3 & b,float * face_height_out) const;

	bool				Get_Bounds(AABoxClass * bounds_out) const;

	friend class EscarpmentSystem;

private:

	int				ID;
	StringClass		Name;
	float				Batter;
	int				LayerIndex;
	int				FaceRowCount;
	float				ToeWidth;

	DynamicVectorClass<EscarpmentPointClass>	Points;

	//	The face geometry, one physics object per polyline segment.  Owned by the system,
	//	which builds and destroys it; the escarpment itself is data.
	DynamicVectorClass<StaticPhysClass *>		Faces;
};


class EscarpmentSystem
{
public:

	static void			Init(void);
	static void			Shutdown(void);

	/*
	**	The escarpments on the map.
	*/
	static int			Add_Escarpment(const EscarpmentClass & escarpment);
	static EscarpmentClass *	Peek_Escarpment(int id);
	static int			Get_Escarpment_Count(void);
	static void			Clear_Escarpments(void);

	/*
	**	Read both benches out of the heightfield rather than asserting them.  For every point
	**	of every escarpment, the ground is sampled one inset either side of the polyline and
	**	the higher sample becomes the upper bench, the lower the lower one.  A point the
	**	terrain cannot answer for is left exactly as the caller set it.
	**
	**	Returns how many points were conformed.  Zero with terrain loaded means the polyline
	**	is off the field, which is worth a caller knowing.
	*/
	static int			Conform_To_Terrain(float inset = 2.0f);

	/*
	**	Geometry and collision.  One face strip per escarpment, added to the static culling
	**	system exactly like a bridge section, so everything that already collides with the
	**	world collides with a cliff with no further work.
	*/
	static bool			Build_Geometry(void);
	static void			Destroy_Geometry(void);
	static bool			Has_Geometry(void);
	static int			Get_Face_Count(void);
	static int			Get_Untextured_Face_Count(void);

	/*
	**	Queries.
	*/
	static bool			Sample_Face(float x,float y,float max_distance,
											float * upper_out,float * lower_out,int * id_out = nullptr);

	/*
	**	Is the straight line a..b blocked by a face taller than this locomotor can climb?
	**	Section 52B's traversability graph is the caller: an edge crossing a face it cannot
	**	scale is simply not an edge.  max_climbable_height comes from WorldMetricsClass.
	*/
	static bool			Is_Segment_Blocked(const Vector3 & a,const Vector3 & b,
													float max_climbable_height,int * id_out = nullptr);

	/*
	**	V2 Phase 10.  A tunnel portal through a cliff: the face is not built inside any cut
	**	disc (plan-view centre, radius), so the bore's own portal geometry is what stands there.
	**	Takes effect at the next Build_Geometry.  Is_Segment_Blocked is deliberately unchanged --
	**	the heightfield behind the face is still too steep to walk, and the tunnel supplies its
	**	own traversal edge.
	*/
	static void			Add_Portal_Cut(const Vector3 & center,float radius);
	static void			Clear_Portal_Cuts(void);
	static int			Get_Portal_Cut_Count(void)	{ return PortalCutCenters.Count(); }

private:

	static bool			Build_Escarpment_Geometry(EscarpmentClass & escarpment);
	static void			Destroy_Escarpment_Geometry(EscarpmentClass & escarpment);
	static bool			Find_Cliff_Texture(const EscarpmentClass & escarpment,
													class TextureClass ** texture_out,float * meters_per_tile_out);

	static DynamicVectorClass<EscarpmentClass>	Escarpments;
	static DynamicVectorClass<Vector3>			PortalCutCenters;
	static DynamicVectorClass<float>				PortalCutRadii;
	static bool			GeometryBuilt;
	static int			UntexturedFaces;
};

#endif	// ESCARPMENT_H
