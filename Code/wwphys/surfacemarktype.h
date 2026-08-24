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
 *                 Project Name : WWPhys                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwphys/surfacemarktype.h                     $*
 *                                                                                             *
 *	Roadmap Section 35, Zero Hour / SAGE feature 22.  The kinds of mark the one bounded         *
 *	surface-mark service knows about, and the shape of a single mark in its pool.               *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef SURFACEMARKTYPE_H
#define SURFACEMARKTYPE_H

#include "always.h"
#include "bittype.h"
#include "vector3.h"
#include "wwstring.h"


/*
**	How many marks may share one definition, and therefore one batched mesh.  This is the second
**	of the two bounds: the pool caps how many marks exist at all, and this caps how much geometry
**	any single texture group can ever be asked to hold, so the mesh for a group can be made once
**	at its largest and never grown.
*/
enum { SURFACE_MARK_MAX_PER_GROUP = 256 };

/*
**	How many distinct texture groups may exist at once.  Content names its own decal textures --
**	an explosion definition and a surface effect each carry one -- so groups are interned as
**	they are asked for rather than declared up front, and this is what stops a level with a
**	thousand differently named decals from interning a thousand meshes.
*/
enum { SURFACE_MARK_MAX_GROUPS = 32 };


/*
**	The mark types roadmap Section 35 names.  The type is what the mark *is*, which is not the
**	same question as which texture draws it: two scorches of different sizes are one type and
**	two groups.  Nothing in the renderer reads the type; it exists so that a caller can ask for
**	"a scorch" without knowing an asset name, and so that a future gameplay query -- what
**	happened here -- has something to match on.
*/
enum SurfaceMarkType
{
	SURFACE_MARK_SCORCH					= 0,
	SURFACE_MARK_BLAST,
	SURFACE_MARK_DIRT,
	SURFACE_MARK_OIL_OR_STAIN,
	SURFACE_MARK_CONSTRUCTION,
	SURFACE_MARK_IMPACT,
	SURFACE_MARK_DECORATIVE_WORLD_MARK,

	SURFACE_MARK_TYPE_COUNT
};

const char *	Surface_Mark_Type_Name(int type);

/*
**	Whether a mark of this type is allowed to outlive its own fade.  Construction marks and
**	decorative world marks are placed deliberately by the world rather than thrown by a weapon,
**	so they are given no lifetime at all and leave only when something needs their slot.
*/
bool				Surface_Mark_Type_Is_Persistent(int type);


/**
** SurfaceMarkDefinitionClass
**
** A kind of mark: one texture, one look, one set of timings.  Every mark that shares a
** definition shares a mesh, which is the whole of the "material/texture grouping" requirement
** and most of the acceptance -- draw calls are counted in definitions, never in marks.
*/
class SurfaceMarkDefinitionClass
{
public:

	SurfaceMarkDefinitionClass(void);

	bool						operator == (const SurfaceMarkDefinitionClass & that) const;
	bool						operator != (const SurfaceMarkDefinitionClass & that) const { return !(*this == that); }

	const char *			Get_Name(void) const						{ return Name.Peek_Buffer(); }
	void						Set_Name(const char * name)			{ Name = (name != nullptr) ? name : ""; }

	int						Get_Type(void) const						{ return Type; }
	void						Set_Type(int type);

	const char *			Get_Texture(void) const					{ return Texture.Peek_Buffer(); }
	void						Set_Texture(const char * texture)	{ Texture = (texture != nullptr) ? texture : ""; }
	bool						Names_A_Texture(void) const			{ return (Texture.Get_Length() > 0); }

	float						Get_Radius(void) const					{ return Radius; }
	void						Set_Radius(float radius);

	float						Get_Lifetime(void) const				{ return Lifetime; }
	void						Set_Lifetime(float seconds)			{ Lifetime = (seconds > 0.0f) ? seconds : 0.0f; }
	bool						Is_Persistent(void) const				{ return (Lifetime <= 0.0f); }

	float						Get_Fade_Start(void) const				{ return FadeStart; }
	void						Set_Fade_Start(float fraction);

	float						Get_Opacity(void) const					{ return Opacity; }
	void						Set_Opacity(float opacity);

	float						Get_Lift_Height(void) const			{ return LiftHeight; }
	void						Set_Lift_Height(float height)			{ LiftHeight = height; }

	bool						Get_Drape(void) const					{ return Drape; }
	void						Set_Drape(bool onoff)					{ Drape = onoff; }

private:

	StringClass				Name;					// what code asks for it by
	int						Type;					// one of SurfaceMarkType
	StringClass				Texture;				// what draws it, empty until art exists
	float						Radius;				// half the width of the quad, in metres
	float						Lifetime;			// seconds before it is gone; zero means persistent
	float						FadeStart;			// fraction of the lifetime spent at full opacity
	float						Opacity;				// alpha at full strength
	float						LiftHeight;			// metres along the surface normal, to clear the ground
	bool						Drape;				// conform the corners to the ground, or keep the quad flat
};


/**
** SurfaceMarkClass
**
** One mark.  Four corners worked out once when it was placed, a clock, and the group it draws
** with.  There is no allocation here and no reference count: a mark is an entry in an array
** that already exists, which is the other half of the acceptance.
**
** A mark that could not be drawn as a draped quad -- a bullet hole on a wall, a mark on a
** staircase -- carries a decal id instead and is drawn by the geometry-clipping projector.  It
** is the same record in the same pool either way, so there is still exactly one place in the
** engine that knows how many marks the world is holding.
*/
class SurfaceMarkClass
{
public:

	SurfaceMarkClass(void)		{ Reset(); }

	void						Reset(void);

	bool						Is_Free(void) const					{ return !InUse; }
	bool						Is_Projected(void) const			{ return Projected; }

	bool						InUse;				// this slot holds a mark
	bool						Projected;			// drawn by the decal projector, not by the batch
	int						Definition;			// index into the manager's definition table
	uint32					Serial;				// bumped on every reuse, so a stale handle is detected
	uint32					DecalID;				// the projector's id, when Projected
	Vector3					Center;				// where it was placed
	Vector3					Normal;				// the surface normal it was placed against
	Vector3					Corner[4];			// draped and lifted, in tri-strip order
	float						Radius;				// half width, in metres
	float						Age;					// seconds since it was placed
	float						Opacity;				// alpha at full strength, before the fade
};


#endif //SURFACEMARKTYPE_H
