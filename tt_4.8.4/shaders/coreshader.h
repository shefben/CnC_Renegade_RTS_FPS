/*	Renegade Scripts.dll
	Default shader class thats used when no other shader handles an object
	Copyright 2009 Mark Sararu, Jonathan Wilson

	This file is part of the Renegade scripts.dll
	The Renegade scripts.dll is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2, or (at your option) any later
	version. See the file COPYING for more details.
	In addition, an exemption is given to allow Run Time Dynamic Linking of this code with any closed source module that does not contain code covered by this licence.
	Only the source code to the module(s) containing the licenced code has to be released.
*/
#ifndef TT_INCLUDE_CORESHADER_H
#define TT_INCLUDE_CORESHADER_H
#include "shader.h"
class FixedFunctionShader: public ProgrammableShader
{
protected:
	inline void ApplyRenderState();
	inline void ApplyTexture(unsigned int stage);
	inline void ApplyNullTexture(unsigned int stage);
	inline void ApplyMaterial();
	inline void ApplyDefaultMaterial();
	inline void ApplyShader();
	inline void ApplyLight(unsigned int light);
	inline void ApplyVertexBuffer();
	inline void ApplyIndexBuffer();
public:
	FixedFunctionShader();
	~FixedFunctionShader();

	bool Load(ChunkLoadClass *cload); // Implemented ONLY to prevent loading and saving of this shader.

	bool Initialize();
	bool Destroy();
	bool Validate();
	bool OnDeviceLost();
	bool OnDeviceReset();
	void Draw(unsigned int primitive_type, unsigned short start_index, unsigned short polygon_count, 
					  unsigned short min_vertex_index, unsigned short vertex_count);
};

#endif
