/*	Renegade Scripts.dll
	Basic Shader
	Copyright 2009 Jonathan Wilson, Mark Sararu

	This file is part of the Renegade scripts.dll
	The Renegade scripts.dll is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2, or (at your option) any later
	version. See the file COPYING for more details.
	In addition, an exemption is given to allow Run Time Dynamic Linking of this code with any closed source module that does not contain code covered by this licence.
	Only the source code to the module(s) containing the licenced code has to be released.
*/
#ifndef TT_INCLUDE_BASICSHADER_H
#define TT_INCLUDE_BASICSHADER_H
#include "shader.h"
class BasicShader: public ProgrammableShader
{
protected:
	EffectSystem::Effect				*Effect;
	EffectSystem::BoundDataCollection	LocalBindings;

public:
	BasicShader();
	bool	Initialize();
	bool	Destroy();
	bool	OnDeviceLost();
	bool	OnDeviceReset();
	void	Draw(unsigned int primitive_type, unsigned short start_index, unsigned short polygon_count, 
					unsigned short min_vertex_index, unsigned short vertex_count);
	bool NeedsTangents()
	{
		return true;
	}
};

#define CHUNK_BASIC 34543
#endif
