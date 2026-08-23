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
 *                     $Archive:: /Commando/Code/ww3d2/shadermgr.cpp                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   ShaderManagerClass::Init -- detect the tier and build the programs                        *
 *   ShaderManagerClass::Shutdown -- give everything back                                      *
 *   ShaderManagerClass::Detect_Tier -- ask the device what it can do                          *
 *   ShaderManagerClass::Register_Program -- hand a program to the manager                     *
 *   ShaderManagerClass::Get_Pass_Count -- how many submissions a program needs                *
 *   ShaderManagerClass::Set_Program -- make one pass of one program current                   *
 *   ShaderManagerClass::Reset_Program -- give the device back                                 *
 *   ShaderManagerClass::Set_Texture -- stage a texture for the programs to read               *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "shadermgr.h"

#include "dx8wrapper.h"
#include "dx8caps.h"
#include "texture.h"
#include "wwdebug.h"


MaterialProgramClass *	ShaderManagerClass::Programs[MATERIAL_PROGRAM_COUNT] = { nullptr };
TextureClass *				ShaderManagerClass::Textures[ShaderManagerClass::MAX_TEXTURE_STAGES] = { nullptr };
MaterialProgramType		ShaderManagerClass::CurrentProgram = MATERIAL_PROGRAM_COUNT;
int							ShaderManagerClass::CurrentPass = 0;
ShaderTierType				ShaderManagerClass::Tier = SHADER_TIER_FIXED_FUNCTION;
bool							ShaderManagerClass::Initialized = false;


/*
** The names, in enumeration order, for diagnostics and for the self check.  Kept next to
** the enum it mirrors rather than inside a function, so that adding a program without
** naming it is a compile error and not a silent blank.
*/
static const char * _ProgramNames[MATERIAL_PROGRAM_COUNT] =
{
	"legacy_w3d",
	"terrain",
	"terrain_detail",
	"road",
	"bridge",
	"water",
	"foliage",
	"projected_shadow",
	"particle",
	"tracer",
	"status_marker",
	"ghost_tint",
	"debug_overlay",
};


/***********************************************************************************************
**	Stock W3D content.
**
**	One pass, and that pass sets nothing.  That is not a placeholder: a W3D material already
**	carries its own ShaderClass and its own VertexMaterialClass, and the renderer applies
**	them as it draws.  What this program contributes is the guarantee that it is a program,
**	so a stock Renegade model goes through the same layer as everything else and the layer
**	has nothing to say about it.  A model exported in 2002 renders in exactly the state it
**	rendered in before this class existed.
***********************************************************************************************/
class LegacyW3DProgramClass : public MaterialProgramClass
{
public:
	virtual const char *	Get_Name(void) const override		{ return _ProgramNames[MATERIAL_PROGRAM_LEGACY_W3D]; }
	virtual bool			Init(ShaderTierType) override		{ return true; }
	virtual int				Get_Pass_Count(void) const override	{ return 1; }
	virtual void			Set_Pass(int pass) override		{ WWASSERT(pass == 0); (void)pass; }
	virtual void			Reset(void) override				{ }
};


/***********************************************************************************************
 * ShaderManagerClass::Detect_Tier -- ask the device what it can do                            *
 *                                                                                             *
 * There is no device on a dedicated server and none before the renderer comes up, and         *
 * fixed function is the right answer in both cases: it is what every program has to be        *
 * able to fall back to anyway.                                                                *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
ShaderTierType	ShaderManagerClass::Detect_Tier(void)
{
	if (DX8Wrapper::_Get_D3D_Device8() == nullptr) {
		return SHADER_TIER_FIXED_FUNCTION;
	}

	const DX8Caps * caps = DX8Wrapper::Get_Current_Caps();
	if (caps == nullptr) {
		return SHADER_TIER_FIXED_FUNCTION;
	}

	//	Both halves have to be there.  A vertex shader with no pixel shader to receive
	//	its output is not a tier any program here wants to write against.
	if ((caps->Get_Vertex_Shader_Majon_Version() >= 1)
		&& (caps->Get_Pixel_Shader_Majon_Version() >= 1)) {
		return SHADER_TIER_PROGRAMMABLE;
	}

	return SHADER_TIER_FIXED_FUNCTION;
}


/***********************************************************************************************
 * ShaderManagerClass::Init -- detect the tier and build the programs                          *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void	ShaderManagerClass::Init(void)
{
	Init(Detect_Tier());
}


void	ShaderManagerClass::Init(ShaderTierType tier)
{
	WWASSERT(tier >= 0 && tier < SHADER_TIER_COUNT);

	if (Initialized) {
		Shutdown();
	}

	Tier = tier;
	Initialized = true;

	CurrentProgram = MATERIAL_PROGRAM_COUNT;
	CurrentPass = 0;

	//	Stock content first and always.  Everything else is registered by the system that
	//	draws it, when that system arrives.
	Register_Program(MATERIAL_PROGRAM_LEGACY_W3D,new LegacyW3DProgramClass);
}


/***********************************************************************************************
 * ShaderManagerClass::Shutdown -- give everything back                                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void	ShaderManagerClass::Shutdown(void)
{
	Reset_Program();
	Reset_Textures();

	for (int i=0; i<MATERIAL_PROGRAM_COUNT; i++) {
		if (Programs[i] != nullptr) {
			Programs[i]->Shutdown();
			delete Programs[i];
			Programs[i] = nullptr;
		}
	}

	Tier = SHADER_TIER_FIXED_FUNCTION;
	Initialized = false;
}


bool	ShaderManagerClass::Is_Initialized(void)
{
	return Initialized;
}


ShaderTierType	ShaderManagerClass::Get_Tier(void)
{
	return Tier;
}


/***********************************************************************************************
 * ShaderManagerClass::Register_Program -- hand a program to the manager                       *
 *                                                                                             *
 * The manager owns the program from the moment it is offered, including when it declines      *
 * the tier.  A caller that had to remember to delete a program the manager refused would      *
 * be a caller that eventually forgets.                                                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool	ShaderManagerClass::Register_Program(MaterialProgramType type, MaterialProgramClass * program)
{
	WWASSERT(type >= 0 && type < MATERIAL_PROGRAM_COUNT);

	if (program == nullptr) {
		return false;
	}

	//	Replacing a registered program is a mistake rather than an override: two systems
	//	claiming one pipeline is exactly the duplicate path this layer exists to prevent.
	WWASSERT(Programs[type] == nullptr);
	if (Programs[type] != nullptr) {
		delete program;
		return false;
	}

	if (!program->Init(Tier)) {
		delete program;
		return false;
	}

	Programs[type] = program;
	return true;
}


bool	ShaderManagerClass::Is_Supported(MaterialProgramType type)
{
	WWASSERT(type >= 0 && type < MATERIAL_PROGRAM_COUNT);
	return (Programs[type] != nullptr);
}


MaterialProgramClass *	ShaderManagerClass::Peek_Program(MaterialProgramType type)
{
	WWASSERT(type >= 0 && type < MATERIAL_PROGRAM_COUNT);
	return Programs[type];
}


const char *	ShaderManagerClass::Get_Program_Name(MaterialProgramType type)
{
	WWASSERT(type >= 0 && type < MATERIAL_PROGRAM_COUNT);
	return _ProgramNames[type];
}


/***********************************************************************************************
 * ShaderManagerClass::Get_Pass_Count -- how many submissions a program needs                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int	ShaderManagerClass::Get_Pass_Count(MaterialProgramType type)
{
	WWASSERT(type >= 0 && type < MATERIAL_PROGRAM_COUNT);

	if (Programs[type] == nullptr) {
		return 0;
	}

	return Programs[type]->Get_Pass_Count();
}


/***********************************************************************************************
 * ShaderManagerClass::Set_Program -- make one pass of one program current                     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void	ShaderManagerClass::Set_Program(MaterialProgramType type, int pass)
{
	WWASSERT(type >= 0 && type < MATERIAL_PROGRAM_COUNT);

	MaterialProgramClass * program = Programs[type];
	if (program == nullptr) {
		//	Asking for a pipeline the build does not have is a caller mistake, but it must
		//	not leave the previous program current and half-owned.
		WWASSERT(0);
		Reset_Program();
		return ;
	}

	WWASSERT(pass >= 0 && pass < program->Get_Pass_Count());

	//	Anything that was current gives the device back first, so state never leaks from
	//	one pipeline into the next.  Passes within one program do not: a program setting
	//	up its own second pass knows what its first one did.
	if ((CurrentProgram != type) && (CurrentProgram != MATERIAL_PROGRAM_COUNT)) {
		Reset_Program();
	}

	CurrentProgram = type;
	CurrentPass = pass;

	program->Set_Pass(pass);
}


/***********************************************************************************************
 * ShaderManagerClass::Reset_Program -- give the device back                                   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void	ShaderManagerClass::Reset_Program(void)
{
	if (CurrentProgram == MATERIAL_PROGRAM_COUNT) {
		return ;
	}

	MaterialProgramClass * program = Programs[CurrentProgram];

	CurrentProgram = MATERIAL_PROGRAM_COUNT;
	CurrentPass = 0;

	if (program != nullptr) {
		program->Reset();
	}
}


MaterialProgramType	ShaderManagerClass::Get_Current_Program(void)
{
	return CurrentProgram;
}


int	ShaderManagerClass::Get_Current_Pass(void)
{
	return CurrentPass;
}


/***********************************************************************************************
 * ShaderManagerClass::Set_Texture -- stage a texture for the programs to read                 *
 *                                                                                             *
 * A reference is held for as long as the stage holds the texture.  The donor stored raw       *
 * pointers here and relied on the caller outliving the draw, which is true right up until      *
 * a level release happens between staging and drawing.                                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void	ShaderManagerClass::Set_Texture(int stage, TextureClass * texture)
{
	WWASSERT(stage >= 0 && stage < MAX_TEXTURE_STAGES);

	REF_PTR_SET(Textures[stage],texture);
}


TextureClass *	ShaderManagerClass::Peek_Texture(int stage)
{
	WWASSERT(stage >= 0 && stage < MAX_TEXTURE_STAGES);
	return Textures[stage];
}


void	ShaderManagerClass::Reset_Textures(void)
{
	for (int i=0; i<MAX_TEXTURE_STAGES; i++) {
		REF_PTR_RELEASE(Textures[i]);
	}
}
