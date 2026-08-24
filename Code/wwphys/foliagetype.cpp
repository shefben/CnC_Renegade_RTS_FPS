/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/WWPhys/foliagetype.cpp                       $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	Defaults for a kind of foliage, and the names of the things it can be.                      *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "foliagetype.h"

#include "wwmath.h"


const char * Foliage_Category_Name(int category)
{
	switch (category) {
		case FOLIAGE_TREE:					return "TREE";
		case FOLIAGE_BUSH:					return "BUSH";
		case FOLIAGE_DEAD_TREE:				return "DEAD_TREE";
		case FOLIAGE_GRASS_CLUMP:			return "GRASS_CLUMP";
		case FOLIAGE_DECORATIVE_ROCK:		return "DECORATIVE_ROCK";
		default:									break;
	}
	return "UNKNOWN";
}


const char * Foliage_Lod_Name(int lod)
{
	switch (lod) {
		case FOLIAGE_LOD_NEAR:				return "NEAR";
		case FOLIAGE_LOD_FAR:				return "FAR";
		case FOLIAGE_LOD_CULLED:			return "CULLED";
		default:									break;
	}
	return "UNKNOWN";
}


/***********************************************************************************************
 *	A type nobody has configured is a visual-only tree of ordinary size that draws nothing.     *
 *	Every field has a value that behaves, so a definition that only sets a name and a model is  *
 *	a working definition.                                                                       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
FoliageTypeClass::FoliageTypeClass(void) :
	Category(FOLIAGE_TREE),
	ImpostorWidth(6.0f),
	ImpostorHeight(10.0f),
	Blocking(FOLIAGE_BLOCKS_NOTHING),
	Destructible(false),
	Health(0.0f),
	ProxyRadius(0.75f),
	ProxyHeight(8.0f),
	NearDistance(90.0f),
	CullDistance(400.0f),
	ScaleMin(1.0f),
	ScaleMax(1.0f)
{
	return ;
}


void FoliageTypeClass::Set_Impostor(const char * texture,float width,float height)
{
	Impostor = (texture != nullptr) ? texture : "";
	if (width > 0.0f)		{ ImpostorWidth = width; }
	if (height > 0.0f)	{ ImpostorHeight = height; }
	return ;
}


/***********************************************************************************************
 *	The batch key for a level of detail.                                                        *
 *                                                                                             *
 *	The two levels are drawn with different textures -- the near one with the model's material  *
 *	and the far one with the impostor sheet -- so they batch separately, which is why the       *
 *	level is part of the key rather than something chosen after the batch is made.              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
const char * FoliageTypeClass::Get_Lod_Material(int lod) const
{
	if (lod == FOLIAGE_LOD_FAR) {
		return Impostor.Peek_Buffer();
	}
	return Material.Peek_Buffer();
}


void FoliageTypeClass::Set_Destructible(bool onoff,float health)
{
	Destructible = onoff;
	Health = (health > 0.0f) ? health : 0.0f;
	return ;
}


void FoliageTypeClass::Set_Proxy(float radius,float height)
{
	ProxyRadius = (radius > 0.0f) ? radius : 0.0f;
	ProxyHeight = (height > 0.0f) ? height : 0.0f;
	return ;
}


/***********************************************************************************************
 *	Near before cull, and both positive.  A type whose near distance is past its cull distance  *
 *	would never reach its far level, which is a configuration mistake rather than a look.       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void FoliageTypeClass::Set_Distances(float near_distance,float cull_distance)
{
	NearDistance = (near_distance > 0.0f) ? near_distance : 0.0f;
	CullDistance = (cull_distance > 0.0f) ? cull_distance : 0.0f;

	if (CullDistance < NearDistance) {
		CullDistance = NearDistance;
	}
	return ;
}


void FoliageTypeClass::Set_Scale_Range(float low,float high)
{
	if (low <= 0.0f)	{ low = 1.0f; }
	if (high <= 0.0f)	{ high = low; }

	ScaleMin = WWMath::Min(low,high);
	ScaleMax = WWMath::Max(low,high);
	return ;
}
