/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/WWPhys/foliagetype.h                         $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	What a kind of foliage is, and where one of them stands.                                    *
 *                                                                                             *
 *	Roadmap Section 21 asks for five categories -- tree, bush, dead tree, grass clump and       *
 *	decorative rock -- and separately for visual-only, infantry blocker, vehicle blocker and    *
 *	destructible.  Those are two different questions about the same thing, so they are two      *
 *	different fields: a category says what it looks like and roughly how far away it stops      *
 *	being worth drawing, and a blocking flag set says what walks through it.  Visual-only is    *
 *	no flags at all rather than a fifth category, because a bush and a tree can each be         *
 *	either, and destructible is a bool for the same reason.                                     *
 *                                                                                             *
 *	A type names one model and one material.  One material per model is a real constraint on    *
 *	the art and it is the same one the donor has: the donor solves it by packing every tree     *
 *	into one mega-texture, and this solves it by asking for it and reporting the models that    *
 *	break it.  See docs/assets/FoliageModels.md.                                                *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef FOLIAGETYPE_H
#define FOLIAGETYPE_H

#include "always.h"
#include "vector3.h"
#include "wwstring.h"


/*
**	The five categories Section 21 lists.  A category is a look and a rough distance scale; it
**	is deliberately not what decides collision.
*/
enum FoliageCategoryType
{
	FOLIAGE_TREE				= 0,
	FOLIAGE_BUSH,
	FOLIAGE_DEAD_TREE,
	FOLIAGE_GRASS_CLUMP,
	FOLIAGE_DECORATIVE_ROCK,

	FOLIAGE_CATEGORY_COUNT
};


/*
**	What walks through it.  A flag set rather than a rank, because "visual-only" is the absence
**	of both flags and a thing can block one without blocking the other -- scrub stops a soldier
**	and not a tank, a low shelf of rock stops a tank and a soldier climbs it.
*/
enum FoliageBlockingBitsType
{
	FOLIAGE_BLOCKS_NOTHING	= 0x00,
	FOLIAGE_BLOCKS_INFANTRY	= 0x01,
	FOLIAGE_BLOCKS_VEHICLES	= 0x02
};


/*
**	Two levels are built and the third is the absence of both.  NEAR is the model's own
**	geometry merged into its cell's batch; FAR is a crossed pair of quads carrying the type's
**	impostor texture, also merged; CULLED is nothing drawn at all.
*/
enum FoliageLodType
{
	FOLIAGE_LOD_NEAR			= 0,
	FOLIAGE_LOD_FAR,

	FOLIAGE_LOD_BUILT_COUNT,

	FOLIAGE_LOD_CULLED		= FOLIAGE_LOD_BUILT_COUNT
};


const char *	Foliage_Category_Name(int category);
const char *	Foliage_Lod_Name(int lod);


class FoliageTypeClass
{
public:

	FoliageTypeClass(void);

	bool	operator == (const FoliageTypeClass & that) const	{ return Name == that.Name; }
	bool	operator != (const FoliageTypeClass & that) const	{ return !(*this == that); }

	void				Set_Name(const char * name)			{ Name = (name != nullptr) ? name : ""; }
	const char *	Get_Name(void) const						{ return Name.Peek_Buffer(); }

	void				Set_Category(int category)				{ Category = category; }
	int				Get_Category(void) const				{ return Category; }

	//	The model whose triangles are merged into the near batch, and the texture that batch
	//	is drawn with.  Either may be unnamed; see the file comment.
	void				Set_Model(const char * name)			{ Model = (name != nullptr) ? name : ""; }
	const char *	Get_Model(void) const					{ return Model.Peek_Buffer(); }
	void				Set_Material(const char * name)		{ Material = (name != nullptr) ? name : ""; }
	const char *	Get_Material(void) const				{ return Material.Peek_Buffer(); }

	//	The far level.  No impostor texture means no far level: a crossed pair of untextured
	//	quads is a white X on a hillside, which is worse than the tree not being there.
	void				Set_Impostor(const char * texture,float width,float height);
	const char *	Get_Impostor(void) const				{ return Impostor.Peek_Buffer(); }
	float				Get_Impostor_Width(void) const		{ return ImpostorWidth; }
	float				Get_Impostor_Height(void) const		{ return ImpostorHeight; }
	bool				Has_Far_Lod(void) const					{ return (Impostor.Get_Length() > 0); }

	//	Which material a level of detail is batched by.  This is the batch key.
	const char *	Get_Lod_Material(int lod) const;

	void				Set_Blocking(unsigned bits)			{ Blocking = bits; }
	unsigned			Get_Blocking(void) const				{ return Blocking; }
	bool				Is_Visual_Only(void) const				{ return (Blocking == FOLIAGE_BLOCKS_NOTHING); }
	bool				Blocks_Infantry(void) const			{ return (Blocking & FOLIAGE_BLOCKS_INFANTRY) != 0; }
	bool				Blocks_Vehicles(void) const			{ return (Blocking & FOLIAGE_BLOCKS_VEHICLES) != 0; }

	void				Set_Destructible(bool onoff,float health);
	bool				Is_Destructible(void) const			{ return Destructible; }
	float				Get_Health(void) const					{ return Health; }

	//	The collision proxy.  A foliage instance is never its own model for collision purposes:
	//	a tree is a post, and its leaves are not something to walk into.
	void				Set_Proxy(float radius,float height);
	float				Get_Proxy_Radius(void) const			{ return ProxyRadius; }
	float				Get_Proxy_Height(void) const			{ return ProxyHeight; }

	void				Set_Distances(float near_distance,float cull_distance);
	float				Get_Near_Distance(void) const			{ return NearDistance; }
	float				Get_Cull_Distance(void) const			{ return CullDistance; }

	void				Set_Scale_Range(float low,float high);
	float				Get_Scale_Min(void) const				{ return ScaleMin; }
	float				Get_Scale_Max(void) const				{ return ScaleMax; }

private:

	StringClass		Name;
	int				Category;

	StringClass		Model;
	StringClass		Material;

	StringClass		Impostor;
	float				ImpostorWidth;
	float				ImpostorHeight;

	unsigned			Blocking;
	bool				Destructible;
	float				Health;

	float				ProxyRadius;
	float				ProxyHeight;

	float				NearDistance;
	float				CullDistance;

	float				ScaleMin;
	float				ScaleMax;
};


/*
**	One plant, standing somewhere.  Small on purpose: a forest is a great many of these, and
**	the point of the phase is that the expensive things are per batch and not per instance.
*/
class FoliageInstanceClass
{
public:

	FoliageInstanceClass(void) :
		Type(-1), Position(0.0f,0.0f,0.0f), Yaw(0.0f), Scale(1.0f),
		Health(0.0f), Cell(-1), Alive(true) { }

	bool	operator == (const FoliageInstanceClass & that) const
	{
		return (Type == that.Type) && (Position == that.Position) &&
				 (Yaw == that.Yaw) && (Scale == that.Scale) && (Alive == that.Alive);
	}
	bool	operator != (const FoliageInstanceClass & that) const	{ return !(*this == that); }

	int			Type;
	Vector3		Position;
	float			Yaw;
	float			Scale;

	float			Health;
	int			Cell;
	bool			Alive;
};

#endif // FOLIAGETYPE_H
