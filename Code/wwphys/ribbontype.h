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
 *                     $Archive:: /Commando/Code/WWPhys/ribbontype.h                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	What a mark on the ground is, before anything has made one.                                 *
 *                                                                                             *
 *	Roadmap Section 23 lists tank tracks, tire tracks, harvester tracks, scorch trails and drag  *
 *	marks as the first consumers.  Those are five kinds of the same thing -- a strip of ground   *
 *	texture laid down behind something that moved -- so they are five definitions of one type,   *
 *	not five systems, exactly the way six kinds of water are six WaterDefinitionClass rows.      *
 *                                                                                             *
 *	A definition carries the two numbers that decide the cost of a mark as much as its look:     *
 *	MaxEdges, which is the length of the ring buffer a ribbon of this kind gets, and Lifetime,   *
 *	which is how long an edge survives.  Both are bounded here, at the type, so that no amount   *
 *	of driving can make one ribbon cost more than one ribbon.                                   *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef RIBBONTYPE_H
#define RIBBONTYPE_H

#include "always.h"
#include "vector3.h"
#include "wwstring.h"


/*
**	The hard cap on one ribbon's ring buffer.
**
**	This is a compile-time constant and not a tuning value because the edges live inside the
**	ribbon as a plain array.  That is the whole of Section 23's "capped ring buffer" and most of
**	its "without unbounded allocations": a ribbon that has been driven for an hour holds exactly
**	as much memory as one that was bound a second ago, and laying a mark never calls new.
*/
enum { SURFACE_RIBBON_MAX_EDGES = 48 };


/*
**	What kind of mark this is.  The category decides nothing by itself -- every number that
**	matters is on the definition -- but it is what a caller names when it wants "whatever this
**	build calls a tank track" rather than a specific authored row.
*/
enum RibbonCategoryType
{
	RIBBON_TANK_TRACK = 0,
	RIBBON_TIRE_TRACK,
	RIBBON_HARVESTER_TRACK,
	RIBBON_SCORCH_TRAIL,
	RIBBON_DRAG_MARK,
	RIBBON_CATEGORY_COUNT,
};

const char *	Ribbon_Category_Name(int category);


/*
**	Whether ground of this surface type keeps a mark.
**
**	Renegade already tags every polygon with a SURFACE_TYPE, and a wheel already reports the one
**	it is standing on (SuspensionElementClass::Get_Contact_Surface).  So the question "would a
**	track show here" is answered from data that exists rather than from a second authored map of
**	where tracks are allowed: soft ground takes a mark, metal, concrete, rock and glass do not.
*/
bool				Ribbon_Surface_Takes_Marks(int surface_type);


/*
**	A kind of mark: what it looks like, how big it is, and what it costs.
**
**	Several emitters -- every tank on the map -- share one of these, the way several ponds share
**	a WaterDefinitionClass.  Nothing here is per vehicle.
*/
class SurfaceRibbonDefinitionClass
{
public:

	SurfaceRibbonDefinitionClass(void);

	bool				operator == (const SurfaceRibbonDefinitionClass & that) const;
	bool				operator != (const SurfaceRibbonDefinitionClass & that) const	{ return !(*this == that); }

	void				Set_Name(const char * name);
	const char *	Get_Name(void) const						{ return Name.Peek_Buffer(); }

	void				Set_Category(int category)				{ Category = category; }
	int				Get_Category(void) const					{ return Category; }

	//	The strip texture.  Empty on every definition today; see the asset list.
	void				Set_Texture(const char * name);
	const char *	Get_Texture(void) const					{ return Texture.Peek_Buffer(); }
	bool				Names_A_Texture(void) const;

	//	How wide the strip is, in metres.  A tank track is one tread, not the whole hull.
	void				Set_Width(float width)					{ Width = width; }
	float				Get_Width(void) const					{ return Width; }

	//	How far the emitter moves before a new cross edge is laid.  Smaller follows a curve more
	//	closely and spends the ring buffer faster; this is the only knob that trades the two.
	void				Set_Segment_Length(float length)		{ SegmentLength = length; }
	float				Get_Segment_Length(void) const			{ return SegmentLength; }

	//	Length of this kind's ring buffer, clamped to SURFACE_RIBBON_MAX_EDGES.
	void				Set_Max_Edges(int edges);
	int				Get_Max_Edges(void) const					{ return MaxEdges; }

	//	How many of the newest edges stay fully opaque before the tail starts fading out.
	void				Set_Opaque_Edges(int edges);
	int				Get_Opaque_Edges(void) const				{ return OpaqueEdges; }

	//	Seconds an edge survives.  It is fully faded, and dropped, at this age.
	void				Set_Lifetime(float seconds)				{ Lifetime = seconds; }
	float				Get_Lifetime(void) const					{ return Lifetime; }

	//	Fraction of Lifetime an edge holds full alpha for before it starts fading on age.
	void				Set_Fade_Start(float fraction);
	float				Get_Fade_Start(void) const					{ return FadeStart; }

	//	How far above the surface the strip sits, in metres.  A road is drawn on the terrain and
	//	a mark is drawn on the road, so this has to clear both.
	void				Set_Lift_Height(float metres)				{ LiftHeight = metres; }
	float				Get_Lift_Height(void) const				{ return LiftHeight; }

	//	How many segment lengths the emitter may jump in one step before the run is treated as
	//	broken rather than continued.  This is the teleport reset, expressed as a number.
	void				Set_Break_Distance(float segments)		{ BreakDistance = segments; }
	float				Get_Break_Distance(void) const				{ return BreakDistance; }

	//	How many world metres of ribbon one tile of the texture covers, along its length.
	void				Set_Texture_Length(float metres)			{ TextureLength = metres; }
	float				Get_Texture_Length(void) const			{ return TextureLength; }

private:

	StringClass		Name;
	int				Category;
	StringClass		Texture;
	float				Width;
	float				SegmentLength;
	int				MaxEdges;
	int				OpaqueEdges;
	float				Lifetime;
	float				FadeStart;
	float				LiftHeight;
	float				BreakDistance;
	float				TextureLength;
};


/*
**	One cross edge of a laid ribbon: the two corners of the strip, how far along the ribbon they
**	are, how long ago they were laid, and whether the strip starts here.
**
**	Deliberately plain data and deliberately small.  An edge is the unit Section 23 calls a
**	"mark", and its whole cost is these few dozen bytes inside an array that already exists --
**	no object, no reference count, no scene membership, nothing to network.
**
**	StartsRun is how one ribbon holds several unconnected strips.  A vehicle that jumps, is
**	teleported, or drives off soft ground and back onto it leaves marks that must not be joined
**	by a triangle stretched across the gap, and the alternative -- a second ribbon per run --
**	would spend the pool on a vehicle that bounced.
*/
class RibbonEdgeClass
{
public:

	RibbonEdgeClass(void) :
		Left(0.0f,0.0f,0.0f), Right(0.0f,0.0f,0.0f), V(0.0f), Age(0.0f), StartsRun(true) { }

	Vector3		Left;
	Vector3		Right;
	float			V;
	float			Age;
	bool			StartsRun;
};

#endif // RIBBONTYPE_H
