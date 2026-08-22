/*	Renegade Scripts.dll
	W3D Bindings
	Copyright 2009 Mark Sararu, Jonathan Wilson

	This file is part of the Renegade scripts.dll
	The Renegade scripts.dll is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2, or (at your option) any later
	version. See the file COPYING for more details.
	In addition, an exemption is given to allow Run Time Dynamic Linking of this code with any closed source module that does not contain code covered by this licence.
	Only the source code to the module(s) containing the licenced code has to be released.
*/
#ifndef TT_INCLUDE_W3DBINDINGS_H
#define TT_INCLUDE_W3DBINDINGS_H
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4.h"
#include "fxshared.fxi"
#include "effect.h"

class TextureClass;
#define DECLARE_NAMESPACED_BINDING(ns, name) class ns##__##name##: public EffectSystem::BindAddress{const char* GetAddress(){return #ns "." #name;}; bool GetData(void** data, unsigned long *size);}; bool ns##__##name##::GetData(void** data,unsigned long *size) 
#define DECLARE_W3D_BINDING(name) DECLARE_NAMESPACED_BINDING(W3D, name)
#define DECLARE_ROOT_BINDING(name) class name##: public EffectSystem::BindAddress{const char* GetAddress(){return #name;};bool GetData(void** data,unsigned long *size);}; bool name##::GetData(void** data,unsigned long *size) 
#define DECLARE_ROOT_BINDING_CACHED(name) class name##: public EffectSystem::BindAddress{const char* GetAddress(){return #name;};bool IsDirty();bool GetData(void** data,unsigned long *size);}; bool name##::GetData(void** data,unsigned long *size) 
#define DECLARE_BINDING_DIRTY_FUNC(name) bool name##::IsDirty()
#define REGISTER_BINDING(name) EffectSystem::GlobalBindings->AddBinding(new name##());
#define REGISTER_NAMESPACED_BINDING(ns, name) EffectSystem::GlobalBindings->AddBinding(new ns##__##name##());

namespace W3DBindings
{
	void Initialize();
	void InvalidateFogRangeCache();
	class SceneTextureBindAddress: public EffectSystem::BindAddress
	{
	protected:
		TextureClass* tex;
	public:
		SceneTextureBindAddress();
		~SceneTextureBindAddress();
		const char* GetAddress();
		bool GetData(void** data, unsigned long *size);
	};
};
#endif