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
 *                     $Archive:: /Commando/Code/ww3d2/shadereffect.h                         $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	HLSL effect files and the names their parameters bind to (D-004).                         *
 *                                                                                             *
 *	This is the TT 4.8.4 shaders.dll effect system (shaders/effect.h) merged into ww3d2        *
 *	instead of hooked in from a DLL.  An effect is a D3DX .fx file.  Each of its parameters   *
 *	is bound, once, to a named address: the parameter's semantic or its SasBindAddress        *
 *	annotation against the global bindings (World, W3D.Fog, Texture0 ...), or its             *
 *	LocalBindAddress annotation against the bindings of the shader that owns the effect.     *
 *	At draw time the bound values are uploaded when dirty.                                    *
 *                                                                                             *
 *	D3DX is loaded at run time (d3dx9_43.dll).  Without it, or without D3DX headers at        *
 *	build time, Is_Available is false, every effect fails to create and the shader layer     *
 *	draws everything through the fixed-function path exactly as before.                      *
 *                                                                                             *
 *	Render, texture-stage, texture and light state set by an effect is routed through the    *
 *	DX8Wrapper caches (an ID3DXEffectStateManager), so the wrapper's view of the device stays *
 *	true after an effect draw.                                                                 *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef SHADEREFFECT_H
#define SHADEREFFECT_H

#include "always.h"
#include "refcount.h"
#include "vector.h"
#include "wwstring.h"

//
//	A named source of parameter data.  Get_Data returns a pointer valid until the next call.
//
class ShaderBindAddressClass : public RefCountClass
{
public:
	virtual const char *	Get_Address(void) const = 0;
	virtual void			Get_Data(const void ** data,unsigned int * size) = 0;
	virtual bool			Is_Dirty(void) const { return true; }
};

//
//	A set of bind addresses searched by name (case-insensitive).  Holds a reference to each.
//
class ShaderBindingsClass
{
public:
	~ShaderBindingsClass(void) { Remove_All(); }

	void								Add_Binding(ShaderBindAddressClass * bind);
	void								Remove_Binding(ShaderBindAddressClass * bind);
	void								Remove_All(void);
	ShaderBindAddressClass *	Find(const char * address) const;

private:
	DynamicVectorClass<ShaderBindAddressClass *>	Binds;
};

enum ShaderEffectStatus
{
	SHADER_EFFECT_USABLE = 0,
	SHADER_EFFECT_ERROR,
	SHADER_EFFECT_CREATED,
	SHADER_EFFECT_DEVICE_LOST,
};

class ShaderEffectClass : public RefCountClass
{
public:
	virtual bool					Initialize(void) = 0;
	virtual ShaderEffectStatus	Get_Status(void) const = 0;
	virtual void					On_Device_Lost(void) = 0;
	virtual void					On_Device_Reset(void) = 0;

	//	Begin uploads dirty bound values, then begins the technique.  Returns false (and draws
	//	nothing) when the effect is not usable.
	virtual bool					Begin(unsigned int * passes) = 0;
	virtual void					Begin_Pass(unsigned int pass) = 0;
	virtual void					Commit_Changes(void) = 0;
	virtual void					End_Pass(void) = 0;
	virtual void					End(void) = 0;
};

class ShaderEffectSystemClass
{
public:
	//	Loads D3DX.  Safe to call without a device; effects are created lazily at first use.
	static void							Init(void);
	static void							Shutdown(void);
	static bool							Is_Available(void);

	static ShaderBindingsClass &	Global_Bindings(void);

	//	Each call makes a new effect (the local bindings belong to one shader).  The returned
	//	effect carries a reference for the caller.  Returns false when D3DX is missing or the
	//	file cannot be read.
	static bool							Create_Effect(const char * filename,ShaderBindingsClass * local,ShaderEffectClass ** effect);

	static void							On_Device_Lost(void);
	static void							On_Device_Reset(void);

};

#endif
