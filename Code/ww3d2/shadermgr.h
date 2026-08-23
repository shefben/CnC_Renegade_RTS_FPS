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
 *                     $Archive:: /Commando/Code/ww3d2/shadermgr.h                            $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	The one place that decides how a surface is drawn.                                          *
 *                                                                                             *
 *	Roadmap Section 15 asks for a single state and shader management layer, so that the         *
 *	systems arriving later -- terrain, roads, water, foliage, projected shadows, the            *
 *	ghost buildings the Commander places -- share one answer to the question of what state      *
 *	a surface needs, instead of each one reaching into DX8Wrapper and leaving the device in     *
 *	a condition the next one has to guess at.                                                   *
 *                                                                                             *
 *	The important design point is that stock W3D content is not outside this layer.  A          *
 *	Renegade material drawn through ShaderClass is MATERIAL_PROGRAM_LEGACY_W3D: a program       *
 *	like any other, one pass, whose pass setup is precisely what the renderer already does.     *
 *	It is registered rather than special-cased so that there is one layer and not two, and      *
 *	so that a stock model keeps rendering exactly as it renders today.                          *
 *                                                                                             *
 *	A program declares how many passes it needs and sets device state for one of them.  That    *
 *	is deliberately the same shape as the fixed-function multipass the engine already draws     *
 *	with, because that is what the hardware floor here is: a program may use real D3D9 vertex   *
 *	and pixel shaders when the tier says they exist, and may express the same result as         *
 *	several fixed-function passes when it does not.                                             *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef	SHADERMGR_H
#define	SHADERMGR_H

#ifndef	ALWAYS_H
	#include "always.h"
#endif

class TextureClass;


/*
**	The surfaces the engine knows how to draw.
**
**	LEGACY_W3D is every stock Renegade material and is always present.  The rest are the
**	pipelines Section 15 lists; each one becomes available when the system that draws it
**	arrives, and until then it is simply not registered, which Is_Supported reports.
*/
enum MaterialProgramType
{
	MATERIAL_PROGRAM_LEGACY_W3D = 0,
	MATERIAL_PROGRAM_TERRAIN,
	MATERIAL_PROGRAM_TERRAIN_DETAIL,
	MATERIAL_PROGRAM_ROAD,
	MATERIAL_PROGRAM_BRIDGE,
	MATERIAL_PROGRAM_WATER,
	MATERIAL_PROGRAM_FOLIAGE,
	MATERIAL_PROGRAM_PROJECTED_SHADOW,
	MATERIAL_PROGRAM_PARTICLE,
	MATERIAL_PROGRAM_TRACER,
	MATERIAL_PROGRAM_STATUS_MARKER,
	MATERIAL_PROGRAM_GHOST_TINT,
	MATERIAL_PROGRAM_DEBUG_OVERLAY,

	MATERIAL_PROGRAM_COUNT
};


/*
**	What the device can actually do, as much of it as a program has to care about.
**
**	The donor picked its implementations from a table of card names and driver versions,
**	which was reasonable in 2001 and is nothing but a liability now: it answers wrongly
**	for every card released since.  Ask the device instead.  The order matters -- a
**	program supported at a lower tier is supported at every higher one.
*/
enum ShaderTierType
{
	SHADER_TIER_FIXED_FUNCTION = 0,	// texture stages and render state, nothing programmable
	SHADER_TIER_PROGRAMMABLE,			// vertex and pixel shaders are available

	SHADER_TIER_COUNT
};


/***********************************************************************************************
**	One way of drawing a surface.
***********************************************************************************************/
class MaterialProgramClass
{
public:
	virtual ~MaterialProgramClass(void) { }

	/*
	**	For diagnostics and for the self check.  Cheap and constant.
	*/
	virtual const char *	Get_Name(void) const = 0;

	/*
	**	Build whatever the program needs on this device.  Returning false means the
	**	program cannot run at this tier and is not registered, which is a normal answer
	**	rather than a failure: it is how a program that wants pixel shaders declines to
	**	be used on hardware that has none.
	*/
	virtual bool			Init(ShaderTierType tier) = 0;

	/*
	**	Give back anything Init built.  Called once, on the way down.
	*/
	virtual void			Shutdown(void) { }

	/*
	**	How many times the geometry has to be submitted.  One for anything expressible
	**	in a single pass; more when the tier forces the result to be built up.
	*/
	virtual int				Get_Pass_Count(void) const = 0;

	/*
	**	Set the device state for one pass.  Numbered from zero.
	*/
	virtual void			Set_Pass(int pass) = 0;

	/*
	**	Put back anything Set_Pass changed that the rest of the engine assumes.  A
	**	program that leaves state behind is a program that breaks whatever draws next,
	**	which is the failure this layer exists to prevent.
	*/
	virtual void			Reset(void) = 0;
};


/***********************************************************************************************
**	The registry, the current program, and the textures programs read.
***********************************************************************************************/
class ShaderManagerClass
{
public:

	enum { MAX_TEXTURE_STAGES = 8 };

	/*
	**	Detect the tier, then build the programs that can run on it.  Passing a tier
	**	explicitly overrides detection, which is what the self check does: the decisions
	**	this class makes are decisions about programs and passes, and none of them need
	**	a device to be checked.
	*/
	static void						Init(void);
	static void						Init(ShaderTierType tier);
	static void						Shutdown(void);

	static bool						Is_Initialized(void);
	static ShaderTierType		Get_Tier(void);

	/*
	**	What the device says, ignoring whether anything has been initialised yet.  Answers
	**	SHADER_TIER_FIXED_FUNCTION when there is no device, which is the honest answer for
	**	a dedicated server as well as the safe one.
	*/
	static ShaderTierType		Detect_Tier(void);

	/*
	**	Hand a program to the manager, which owns it from then on.  Init is called with
	**	the current tier and the program is deleted, not kept, if it declines.  Returns
	**	whether it was kept.
	*/
	static bool						Register_Program(MaterialProgramType type, MaterialProgramClass * program);

	static bool						Is_Supported(MaterialProgramType type);
	static MaterialProgramClass *	Peek_Program(MaterialProgramType type);
	static const char *			Get_Program_Name(MaterialProgramType type);

	/*
	**	Zero for a program that is not registered, so that a caller which asks for the
	**	passes of something unavailable draws nothing rather than drawing it wrongly.
	*/
	static int						Get_Pass_Count(MaterialProgramType type);

	/*
	**	Make one pass of one program current.  Whatever was current is reset first, so
	**	exactly one program owns the device state at a time and no caller has to know
	**	what the previous one did.
	*/
	static void						Set_Program(MaterialProgramType type, int pass);

	/*
	**	Give the device back.  Safe to call when nothing is current.
	*/
	static void						Reset_Program(void);

	static MaterialProgramType	Get_Current_Program(void);
	static int						Get_Current_Pass(void);

	/*
	**	The textures a program draws with.  Staged here rather than passed through
	**	Set_Program because which stage means what is the business of the program, and
	**	the systems that will use this -- terrain with its noise layers, water with its
	**	reflection -- have more of them than a call signature wants to carry.
	*/
	static void						Set_Texture(int stage, TextureClass * texture);
	static TextureClass *		Peek_Texture(int stage);
	static void						Reset_Textures(void);

private:
	static MaterialProgramClass *	Programs[MATERIAL_PROGRAM_COUNT];
	static TextureClass *			Textures[MAX_TEXTURE_STAGES];
	static MaterialProgramType		CurrentProgram;
	static int							CurrentPass;
	static ShaderTierType			Tier;
	static bool							Initialized;
};

#endif	// SHADERMGR_H
