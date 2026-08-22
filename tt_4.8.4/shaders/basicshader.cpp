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
#include "general.h"

#include "basicshader.h"
#include "shaderstatemanager.h"
#include "DX8Wrapper.h"
#include "FileHash.h"
using namespace EffectSystem;

BasicShader::BasicShader(): ProgrammableShader(), LocalBindings()
{
	Effect				= NULL;
}

bool BasicShader::Initialize()
{
	assert(Status == ShaderStatus_Created); // Should only be called once after shader is created, please report this

	for (SLNode<ShaderParameter>* iter = Parameters.GetParameters().Head(); iter; iter = iter->Next())
	{
		ShaderParameter *param = iter->Data();
		assert(param);
		switch (param->GetType())
		{
		case ShaderParameterType_Custom:
			//TODO: Error
		case ShaderParameterType_Float:
		case ShaderParameterType_Int:
		case ShaderParameterType_UInt:
		case ShaderParameterType_Vector2:
		case ShaderParameterType_Vector3:
		case ShaderParameterType_Vector4:
		case ShaderParameterType_Bool:
			LocalBindings.AddBinding(new ShaderParameterBindAddress(param));
			break;
		case ShaderParameterType_String:
			// ignore
			break;
		case ShaderParameterType_Texture:
			LocalBindings.AddBinding(new ShaderParameterTextureBindAddress(param));
			break;
		DEFAULT_UNREACHABLE;
		}
	}

	// Normally this is bad, but it's a one-time init function :)
	char* effectfilename;
	Parameters.GetParameter("EffectFilename")->GetData(&effectfilename); 
	
	/*FileClass *f = Get_Data_File(effectfilename);
	if ((f) && (f->Is_Available()))
	{
		unsigned int size = f->Size();
		char *data = new char[size];
		f->Open(1);
		f->Read(data,size);
		CheckGlobal(data,size,effectfilename,HashFXFile);
		delete[] data;
		f->Close();
		Close_Data_File(f);
	}*/ //TODO: Doesnt work because CheckGlobal isnt filled in yet
	if (EffectFactory->CreateEffect(effectfilename,&LocalBindings,&Effect) != true)
	{
		Status = ShaderStatus_Error;
		return false;
	}
	// The next line is scary. Basically, it asks the shader to initalized (which returns a bool)
	// It takes the return value from that and transforms it into a ShaderStatus. It then compares it to 
	// ShaderStatus_Pending and returns the resulting bool
	return ((Status = Effect->Initialize() ? ShaderStatus_Pending : ShaderStatus_Error) == ShaderStatus_Pending); 
}

bool BasicShader::Destroy()
{
	SafeRelease(Effect);
	Status = ShaderStatus_Destroyed;
	return true;
}

bool BasicShader::OnDeviceLost()
{
	if (Status == ShaderStatus_Usable || Status == ShaderStatus_Pending)
	{
		Effect->OnDeviceLost();		
		Status = ShaderStatus_DeviceLost;
		return true;
	}
	assert(Status == ShaderStatus_Usable || Status == ShaderStatus_Pending); // Should only be called when shader is in usable state.
	return false;
}

bool BasicShader::OnDeviceReset()
{
	if (Status == ShaderStatus_DeviceLost)
	{
		Effect->OnDeviceReset();
		if (Effect->GetStatus() != EffectStatus_Usable)
		{
			Status = ShaderStatus_Error;
			return false;
		}
		Status = ShaderStatus_Usable;
		return true;
	}
	assert(Status == ShaderStatus_DeviceLost); // Should only be called after OnDeviceLost;
	return false;
}

void BasicShader::Draw(unsigned int primitive_type, unsigned short start_index, unsigned short polygon_count, 
			unsigned short min_vertex_index, unsigned short vertex_count)
{
	DebugEventStart(DEBUG_COLOR1,L"BasicShader<%S>::Draw",this->GetName());
	if (!DrawingDisabled)
	{
		ShaderClass &s = DX8Wrapper::render_state.shader;
		s._Apply_Shader();

		StateManager::SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0);
		StateManager::SetTextureStageState(1,D3DTSS_TEXCOORDINDEX,1);
		StateManager::SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,0);
		StateManager::SetTextureStageState(1,D3DTSS_TEXTURETRANSFORMFLAGS,0);

		// Something needs to be made to take care of this
		Buffers_Apply();

		unsigned int passes;
		Effect->Begin(&passes,0);
		for (unsigned int i = 0; i < passes; ++i)
		{
			Effect->BeginPass(i);
			Effect->ApplyChanges();
			::Draw(primitive_type,start_index,polygon_count,min_vertex_index,vertex_count);
			Effect->EndPass();
		}
		Effect->End();
	}
	DebugEventEnd();
}

ShaderRegistrant<BasicShader> BasicShaderRegistrant(CHUNK_BASIC,"BasicShader");
