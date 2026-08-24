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
 *                     $Archive:: /Commando/Code/wwphys/surfacemarktype.cpp                   $*
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "surfacemarktype.h"


const char * Surface_Mark_Type_Name(int type)
{
	switch (type)
	{
		case SURFACE_MARK_SCORCH:						return "SCORCH";
		case SURFACE_MARK_BLAST:						return "BLAST";
		case SURFACE_MARK_DIRT:							return "DIRT";
		case SURFACE_MARK_OIL_OR_STAIN:				return "OIL_OR_STAIN";
		case SURFACE_MARK_CONSTRUCTION:				return "CONSTRUCTION";
		case SURFACE_MARK_IMPACT:						return "IMPACT";
		case SURFACE_MARK_DECORATIVE_WORLD_MARK:	return "DECORATIVE_WORLD_MARK";
		default:												return "UNKNOWN";
	}
}


/***********************************************************************************************
 *	Surface_Mark_Type_Is_Persistent -- which marks are placed rather than thrown                 *
 *                                                                                             *
 *	A scorch is the record of something that happened and should stop being interesting; a       *
 *	construction mark is part of how the world looks and has no reason to expire.  Persistent    *
 *	does not mean permanent: it means the mark has no clock of its own and leaves only when      *
 *	the pool needs its slot, so the memory bound is unaffected either way.                       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool Surface_Mark_Type_Is_Persistent(int type)
{
	switch (type)
	{
		case SURFACE_MARK_CONSTRUCTION:
		case SURFACE_MARK_DECORATIVE_WORLD_MARK:
			return true;

		default:
			return false;
	}
}


SurfaceMarkDefinitionClass::SurfaceMarkDefinitionClass(void) :
	Name(""),
	Type(SURFACE_MARK_SCORCH),
	Texture(""),
	Radius(1.0f),
	Lifetime(45.0f),
	FadeStart(0.75f),
	Opacity(1.0f),
	LiftHeight(0.05f),
	Drape(true)
{
}


bool SurfaceMarkDefinitionClass::operator == (const SurfaceMarkDefinitionClass & that) const
{
	return	(Name == that.Name) &&
				(Type == that.Type) &&
				(Texture == that.Texture) &&
				(Radius == that.Radius) &&
				(Lifetime == that.Lifetime) &&
				(FadeStart == that.FadeStart) &&
				(Opacity == that.Opacity) &&
				(LiftHeight == that.LiftHeight) &&
				(Drape == that.Drape);
}


void SurfaceMarkDefinitionClass::Set_Type(int type)
{
	if ((type < 0) || (type >= SURFACE_MARK_TYPE_COUNT)) {
		type = SURFACE_MARK_SCORCH;
	}
	Type = type;
	return ;
}


void SurfaceMarkDefinitionClass::Set_Radius(float radius)
{
	//	A mark with no width is not a mark, and one the size of a level would drape across
	//	terrain it cannot follow.  Both ends are clamped rather than asserted because the radius
	//	arrives from content -- an explosion definition's DecalSize -- and content is allowed
	//	to be wrong without taking the game down.
	if (radius < 0.05f) { radius = 0.05f; }
	if (radius > 32.0f) { radius = 32.0f; }
	Radius = radius;
	return ;
}


void SurfaceMarkDefinitionClass::Set_Fade_Start(float fraction)
{
	if (fraction < 0.0f) { fraction = 0.0f; }
	if (fraction > 1.0f) { fraction = 1.0f; }
	FadeStart = fraction;
	return ;
}


void SurfaceMarkDefinitionClass::Set_Opacity(float opacity)
{
	if (opacity < 0.0f) { opacity = 0.0f; }
	if (opacity > 1.0f) { opacity = 1.0f; }
	Opacity = opacity;
	return ;
}


void SurfaceMarkClass::Reset(void)
{
	InUse			= false;
	Projected	= false;
	Definition	= -1;
	DecalID		= 0;
	Center.Set(0.0f,0.0f,0.0f);
	Normal.Set(0.0f,0.0f,1.0f);
	for (int i = 0; i < 4; i++) {
		Corner[i].Set(0.0f,0.0f,0.0f);
	}
	Radius	= 0.0f;
	Age		= 0.0f;
	Opacity	= 1.0f;

	//	Serial is deliberately not reset.  It is what makes a handle to a slot that has since
	//	been recycled fail to match instead of silently addressing somebody else's mark.
	return ;
}
