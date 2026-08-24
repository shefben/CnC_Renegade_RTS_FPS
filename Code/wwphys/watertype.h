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
 *                     $Archive:: /Commando/Code/WWPhys/watertype.h                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	What a body of water is, and where somebody said it goes.                                   *
 *                                                                                             *
 *	Roadmap Section 22 asks for ocean/coast, lake, pond, river and stream.  A definition is the  *
 *	first three words of that: what kind of water this is and what it looks like, independent   *
 *	of where any particular one sits.  A placed area is the rest: a name, a definition, and the  *
 *	points that say its shape -- a closed ring for a still body, an open line for a flowing one, *
 *	exactly the "two points and a kind" shape a bridge is and the "a line somebody drew" shape a *
 *	road is, generalised to a boundary instead of a spline.                                      *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef WATERTYPE_H
#define WATERTYPE_H

#include "always.h"
#include "vector.h"
#include "vector3.h"
#include "wwstring.h"

class AABoxClass;
class StaticPhysClass;


/*
**	What kind of water this is.  Ocean and coast do not reshape the ground -- the level's own
**	terrain is assumed to already be low enough, because there is no boundary to an ocean for a
**	channel to be cut along.  Lake, pond, river and stream are all bounded and all carve a bed.
*/
enum WaterCategoryType
{
	WATER_OCEAN = 0,
	WATER_COAST,
	WATER_LAKE,
	WATER_POND,
	WATER_RIVER,
	WATER_STREAM,
	WATER_CATEGORY_COUNT,
};

const char *	Water_Category_Name(int category);

//	River and stream are open lines with flow; everything else is a closed, still body.
bool				Water_Category_Flows(int category);

//	Lake, pond, river and stream are bounded enough to carve a bed for.  Ocean and coast are not.
bool				Water_Category_Carves_Terrain(int category);


/*
**	A kind of water: what it is for, what it is drawn with, and how its bed is cut.  Several
**	placed areas -- every pond on a map -- can share one of these the way several bridges share
**	a BridgeDefinitionClass.
*/
class WaterDefinitionClass
{
public:

	WaterDefinitionClass(void);

	bool				operator == (const WaterDefinitionClass & that) const;
	bool				operator != (const WaterDefinitionClass & that) const	{ return !(*this == that); }

	void				Set_Name(const char * name);
	const char *	Get_Name(void) const						{ return Name.Peek_Buffer(); }

	void				Set_Category(int category)				{ Category = category; }
	int				Get_Category(void) const					{ return Category; }

	//	Asset names.  Both empty is the state of every definition today; see the asset list.
	void				Set_Surface_Texture(const char * name);
	const char *	Get_Surface_Texture(void) const			{ return SurfaceTexture.Peek_Buffer(); }
	void				Set_Flow_Texture(const char * name);
	const char *	Get_Flow_Texture(void) const				{ return FlowTexture.Peek_Buffer(); }
	bool				Names_Any_Texture(void) const;

	//	Metres per second the flow texture would scroll, once something drives it per frame.
	void				Set_Scroll_Speed(float speed)			{ ScrollSpeed = speed; }
	float				Get_Scroll_Speed(void) const				{ return ScrollSpeed; }

	//	Cosmetic bob, for a shader that reads it later.  Not applied by anything yet.
	void				Set_Wave_Amplitude(float amplitude)	{ WaveAmplitude = amplitude; }
	float				Get_Wave_Amplitude(void) const			{ return WaveAmplitude; }

	//	How far in from open water the surface fades towards the bank, in metres of vertex alpha.
	void				Set_Shore_Feather(float metres)			{ ShoreFeather = metres; }
	float				Get_Shore_Feather(void) const			{ return ShoreFeather; }

	//	How far below the authored surface height the carved bed sits.
	void				Set_Bed_Depth(float depth)				{ BedDepth = depth; }
	float				Get_Bed_Depth(void) const					{ return BedDepth; }

private:

	StringClass		Name;
	int				Category;
	StringClass		SurfaceTexture;
	StringClass		FlowTexture;
	float				ScrollSpeed;
	float				WaveAmplitude;
	float				ShoreFeather;
	float				BedDepth;
};


/*
**	One point on a water area's boundary or centre line.  Z is the surface height at this point,
**	which is how a river slopes downhill: every station can sit lower than the one before it.
**	Width only means anything on a flowing area -- a closed area's edge is the boundary itself.
*/
class WaterStationClass
{
public:

	WaterStationClass(void) : Position(0.0f,0.0f,0.0f), Width(0.0f) { }

	bool	operator == (const WaterStationClass & that) const
	{
		return (Position == that.Position) && (Width == that.Width);
	}
	bool	operator != (const WaterStationClass & that) const	{ return !(*this == that); }

	Vector3	Position;
	float		Width;
};


/*
**	One body of water, placed.  A name, a definition, and the points that describe its shape.
**
**	Closed means the points are a ring around a still body and Height is the one flat surface a
**	still body actually has -- a lake cannot slope, so unlike a river's per-station Z there is
**	exactly one number.  Not closed means the points are an open centre line and each station's
**	own Z is the surface height there, which is what lets a river run downhill.
*/
class WaterAreaClass
{
public:

	WaterAreaClass(void);
	WaterAreaClass(const WaterAreaClass & that);
	~WaterAreaClass(void);

	WaterAreaClass &	operator = (const WaterAreaClass & that);
	bool					operator == (const WaterAreaClass & that) const;
	bool					operator != (const WaterAreaClass & that) const	{ return !(*this == that); }

	void				Reset(void);

	void				Set_Name(const char * name);
	const char *	Get_Name(void) const					{ return Name.Peek_Buffer(); }
	void				Set_ID(int id)							{ ID = id; }
	int				Get_ID(void) const					{ return ID; }

	void				Set_Definition(const char * name);
	const char *	Get_Definition(void) const			{ return Definition.Peek_Buffer(); }

	void				Set_Closed(bool closed)				{ Closed = closed; }
	bool				Is_Closed(void) const					{ return Closed; }

	//	Only meaningful when closed.  A still body has one surface height, not one per station.
	void				Set_Height(float height)				{ Height = height; }
	float				Get_Height(void) const					{ return Height; }

	void				Clear_Stations(void)					{ Stations.Delete_All(); }
	void				Add_Station(const Vector3 & position,float width);
	int				Get_Station_Count(void) const		{ return Stations.Count(); }
	const WaterStationClass &	Get_Station(int index) const;

	float				Get_Length(void) const;
	bool				Get_Bounds(AABoxClass * bounds_out) const;

	//	Point-in-water tests, from the authoring data rather than from any collision scene.  A
	//	closed area is a polygon test; an open one is a capsule test against the centre line.
	bool				Contains_Point(float x,float y) const;
	bool				Get_Surface_Height(float x,float y,float * height_out) const;

	//	Direction alone.  Speed is a property of the kind of water, not of this area's shape, so
	//	it lives on the definition; WaterSystem::Get_Flow joins the two for a caller.
	bool				Get_Flow_Direction(float x,float y,Vector3 * direction_out) const;

	StringClass							Name;
	StringClass							Definition;
	int									ID;
	bool									Closed;
	float									Height;
	DynamicVectorClass<WaterStationClass>	Stations;

	//	Built and owned by WaterSystem; not part of the authored shape and not compared by ==.
	StaticPhysClass *					Phys;
	bool									TextureMissing;
};

#endif	// WATERTYPE_H
