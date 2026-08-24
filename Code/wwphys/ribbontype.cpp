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

//
//	The kinds of mark, and what ground keeps one.  See ribbontype.h.
//

#include "ribbontype.h"

#include "w3d_file.h"

#include <string.h>


const char *	Ribbon_Category_Name(int category)
{
	switch (category) {
		case RIBBON_TANK_TRACK:			return "tank track";
		case RIBBON_TIRE_TRACK:			return "tire track";
		case RIBBON_HARVESTER_TRACK:	return "harvester track";
		case RIBBON_SCORCH_TRAIL:		return "scorch trail";
		case RIBBON_DRAG_MARK:			return "drag mark";
		default:								break;
	}
	return "unknown";
}


/*
**	Soft ground keeps a mark; hard ground does not.
**
**	The permeable variants are here because a permeable surface is still made of whatever it
**	says it is -- foliage grows out of ground, and a track through undergrowth is a track.
**	Water is deliberately absent: a wake on water is SurfaceEffectsManager's job and already
**	exists, and duplicating it here as a ribbon would be a second answer to the same question.
*/
bool	Ribbon_Surface_Takes_Marks(int surface_type)
{
	switch (surface_type) {
		case SURFACE_TYPE_SAND:
		case SURFACE_TYPE_DIRT:
		case SURFACE_TYPE_MUD:
		case SURFACE_TYPE_GRASS:
		case SURFACE_TYPE_SNOW:
		case SURFACE_TYPE_TIBERIUM_FIELD:
		case SURFACE_TYPE_FOLIAGE_PERMEABLE:
		case SURFACE_TYPE_UNDERWATER_DIRT:
		case SURFACE_TYPE_UNDERWATER_TIBERIUM_DIRT:
			return true;
		default:
			break;
	}
	return false;
}


SurfaceRibbonDefinitionClass::SurfaceRibbonDefinitionClass(void) :
	Name(""),
	Category(RIBBON_TANK_TRACK),
	Texture(""),
	Width(1.0f),
	SegmentLength(1.0f),
	MaxEdges(SURFACE_RIBBON_MAX_EDGES),
	OpaqueEdges(SURFACE_RIBBON_MAX_EDGES / 2),
	Lifetime(20.0f),
	FadeStart(0.5f),
	LiftHeight(0.06f),
	BreakDistance(6.0f),
	TextureLength(4.0f)
{
	return ;
}


bool	SurfaceRibbonDefinitionClass::operator == (const SurfaceRibbonDefinitionClass & that) const
{
	return (::strcmp(Name.Peek_Buffer(),that.Name.Peek_Buffer()) == 0) &&
			 (Category == that.Category) &&
			 (::strcmp(Texture.Peek_Buffer(),that.Texture.Peek_Buffer()) == 0) &&
			 (Width == that.Width) &&
			 (SegmentLength == that.SegmentLength) &&
			 (MaxEdges == that.MaxEdges) &&
			 (OpaqueEdges == that.OpaqueEdges) &&
			 (Lifetime == that.Lifetime) &&
			 (FadeStart == that.FadeStart) &&
			 (LiftHeight == that.LiftHeight) &&
			 (BreakDistance == that.BreakDistance) &&
			 (TextureLength == that.TextureLength);
}


void	SurfaceRibbonDefinitionClass::Set_Name(const char * name)
{
	Name = (name != nullptr) ? name : "";
	return ;
}


void	SurfaceRibbonDefinitionClass::Set_Texture(const char * name)
{
	Texture = (name != nullptr) ? name : "";
	return ;
}


bool	SurfaceRibbonDefinitionClass::Names_A_Texture(void) const
{
	return (Texture.Get_Length() > 0);
}


void	SurfaceRibbonDefinitionClass::Set_Max_Edges(int edges)
{
	if (edges < 2) {
		edges = 2;
	}
	if (edges > SURFACE_RIBBON_MAX_EDGES) {
		edges = SURFACE_RIBBON_MAX_EDGES;
	}
	MaxEdges = edges;

	//	The opaque run cannot be longer than the buffer that holds it.
	if (OpaqueEdges > MaxEdges) {
		OpaqueEdges = MaxEdges;
	}
	return ;
}


void	SurfaceRibbonDefinitionClass::Set_Opaque_Edges(int edges)
{
	if (edges < 0) {
		edges = 0;
	}
	if (edges > MaxEdges) {
		edges = MaxEdges;
	}
	OpaqueEdges = edges;
	return ;
}


void	SurfaceRibbonDefinitionClass::Set_Fade_Start(float fraction)
{
	if (fraction < 0.0f) {
		fraction = 0.0f;
	}
	if (fraction > 1.0f) {
		fraction = 1.0f;
	}
	FadeStart = fraction;
	return ;
}
