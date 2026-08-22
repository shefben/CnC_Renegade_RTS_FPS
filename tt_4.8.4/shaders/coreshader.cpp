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
#include "general.h"

#include "coreshader.h"
#include "shaderstatemanager.h"
#include "vertexbuffer.h"
#include "indexbuffer.h"
#include "DX8Wrapper.h"
#include "WW3D.h"
#include "Engine_Math.h"
FixedFunctionShader::FixedFunctionShader(): ProgrammableShader()
{
	//UID = 0;
	//Name = NULL;
	Status = ShaderStatus_Usable;
}

FixedFunctionShader::~FixedFunctionShader()
{

}

bool FixedFunctionShader::Load(ChunkLoadClass *cload)
{
	return false;
}

bool FixedFunctionShader::Initialize()
{
	// Nothing really
	return true;
}

bool FixedFunctionShader::Destroy()
{
	// Same
	return true;
}

bool FixedFunctionShader::Validate()
{
	// Always valid
	return true;
}

bool FixedFunctionShader::OnDeviceLost()
{
	// Nothing to release
	return true;
}

bool FixedFunctionShader::OnDeviceReset()
{
	// Nothing to recreate
	return true;
}

void FixedFunctionShader::Draw(unsigned int primitive_type, unsigned short start_index, unsigned short polygon_count, unsigned short min_vertex_index, unsigned short vertex_count)
{
	ApplyRenderState();
	::Draw(primitive_type,start_index,polygon_count,min_vertex_index,vertex_count);
}

// Begin protected interfaces of FixedFunctionShader
void FixedFunctionShader::ApplyRenderState()
{
	StateManager::SetPixelShader(NULL);
	StateManager::SetVertexShader(NULL);
	if (Transforms.changed & TS_World)
	{
		DX8Wrapper::D3DDevice->SetTransform(D3DTS_WORLD,(D3DMATRIX *)&Transforms.world);
		Transforms.changed &= ~TS_World;
	}
	if (Transforms.changed & TS_View)
	{
		DX8Wrapper::D3DDevice->SetTransform(D3DTS_VIEW,(D3DMATRIX *)&Transforms.view);
		Transforms.changed &= ~TS_View;
	}
	if (Transforms.changed & TS_Projection)
	{
		DX8Wrapper::D3DDevice->SetTransform(D3DTS_PROJECTION,(D3DMATRIX *)&Transforms.projection);
		Transforms.changed &= ~TS_Projection;
	}
	if (DX8Wrapper::render_state_changed & LIGHT0_CHANGED)
	{
		ApplyLight(0);
		DX8Wrapper::render_state_changed &= ~LIGHT0_CHANGED;
	}
	if (DX8Wrapper::render_state_changed & LIGHT1_CHANGED)
	{
		ApplyLight(1);
		DX8Wrapper::render_state_changed &= ~LIGHT1_CHANGED;
	}
	if (DX8Wrapper::render_state_changed & LIGHT2_CHANGED)
	{
		ApplyLight(2);
		DX8Wrapper::render_state_changed &= ~LIGHT2_CHANGED;
	}
	if (DX8Wrapper::render_state_changed & LIGHT3_CHANGED)
	{
		ApplyLight(3);
		DX8Wrapper::render_state_changed &= ~LIGHT3_CHANGED;
	}
	if (DX8Wrapper::render_state_changed & MATERIAL_CHANGED)
	{
		ApplyMaterial();
		DX8Wrapper::render_state_changed &= ~MATERIAL_CHANGED;
	}
	if (DX8Wrapper::render_state_changed & VERTEX_BUFFER_CHANGED)
	{
		ApplyVertexBuffer();
		DX8Wrapper::render_state_changed &= ~VERTEX_BUFFER_CHANGED;
	}
	if (DX8Wrapper::render_state_changed & INDEX_BUFFER_CHANGED)
	{
		ApplyIndexBuffer();
		DX8Wrapper::render_state_changed &= ~INDEX_BUFFER_CHANGED;
	}
	if (DX8Wrapper::render_state_changed & SHADER_CHANGED)
	{
		ApplyShader();
		DX8Wrapper::render_state_changed &= ~SHADER_CHANGED;
	}
	ApplyTexture(0);
	ApplyTexture(1);
	DX8Wrapper::render_state_changed &= ~TEXTURE0_CHANGED & ~TEXTURE1_CHANGED;
	{

		static uint current_fog_bits = 0x0;
		current_fog_bits = FogState.DirtyBits;
		uint bits = FogState.DirtyBits;
		if (bits & 1)
		{
			StateManager::SetRenderState(D3DRS_FOGENABLE,FogState.Enabled);
			//current_fog_bits &= ~1;
		}
		if (bits & 1 << 1)
		{
			switch(FogState.Mode)
			{
			case FogMode_None:
				StateManager::SetRenderState(D3DRS_FOGVERTEXMODE,D3DFOG_NONE);
				break;
			case FogMode_Linear:
				StateManager::SetRenderState(D3DRS_FOGVERTEXMODE,D3DFOG_LINEAR);
				break;
			case FogMode_Exp:
				StateManager::SetRenderState(D3DRS_FOGVERTEXMODE,D3DFOG_EXP);
				break;
			case FogMode_Exp2:
				StateManager::SetRenderState(D3DRS_FOGVERTEXMODE,D3DFOG_EXP2);
				break;
			DEFAULT_UNREACHABLE;
			}
			//current_fog_bits &= ~(1<<1);
		}
		if (bits & 1 << 2)
		{
			StateManager::SetRenderState(D3DRS_FOGCOLOR,FogState.FFColor);
			//current_fog_bits &= ~(1<<2);
		}
		if (bits & 1 << 3)
		{
			StateManager::SetRenderState(D3DRS_FOGSTART,F2DW(&FogState.Start));
			StateManager::SetRenderState(D3DRS_FOGEND,F2DW(&FogState.End));
			//current_fog_bits &= ~(1<<3);
		}
		if (bits & 1 << 4)
		{
			StateManager::SetRenderState(D3DRS_FOGDENSITY,F2DW(&FogState.Density));
			//current_fog_bits &= ~(1<<4);
		}
		FogState.DirtyBits = 0;
	}
}

void FixedFunctionShader::ApplyTexture(unsigned int stage)
{
	if (!DX8Wrapper::render_state.Textures[stage])
	{
		ApplyNullTexture(stage);
		return;
	}
	DebugEventStart(DEBUG_COLOR1,L"Texture[%u]<%S>::Apply",stage,DX8Wrapper::render_state.Textures[stage]->Name);
	TextureClass *t = DX8Wrapper::render_state.Textures[stage];
	if (!t->Initialized)
	{
		t->Init();
	}
	t->LastAccessed = WW3D::SyncTime;
	if (WW3D::IsTexturingEnabled)
	{
		StateManager::SetTexture(stage,t->D3DTexture);
	}
	else
	{
		ApplyNullTexture(stage);
		DebugEventEnd();
		return;
	}
	StateManager::SetSamplerState(stage,D3DSAMP_MINFILTER,_MinTextureFilters[t->TextureMinFilter + stage * 4]);
	StateManager::SetSamplerState(stage,D3DSAMP_MAGFILTER,_MagTextureFilters[t->TextureMagFilter + stage * 4]);
	StateManager::SetSamplerState(stage,D3DSAMP_MIPFILTER,_MipMapFilters[t->MipMapFilter + stage * 4]);
	if (!t->UAddressMode)
	{
		StateManager::SetSamplerState(stage,D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP);
	}
	else if (t->UAddressMode == 1)
	{
		StateManager::SetSamplerState(stage,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);
	}
	if (!t->VAddressMode)
	{
		StateManager::SetSamplerState(stage,D3DSAMP_ADDRESSV,D3DTADDRESS_WRAP);
	}
	else if (t->VAddressMode == 1)
	{
		StateManager::SetSamplerState(stage,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
	}
	DebugEventEnd();
}

void FixedFunctionShader::ApplyNullTexture(unsigned int stage)
{
	DebugEventStart(DEBUG_COLOR1,L"Texture[%u]<null>::Apply",stage);
	StateManager::SetTexture(stage,0);
	DebugEventEnd();
}

void FixedFunctionShader::ApplyMaterial()
{
	if (!DX8Wrapper::render_state.material)
	{
		ApplyDefaultMaterial();
		return;
	}
	DebugEventStart(DEBUG_COLOR1,L"Material<%S>::Apply",DX8Wrapper::render_state.material->Name);
	VertexMaterialClass *m = DX8Wrapper::render_state.material;
	StateManager::SetMaterial(m->Material);
	StateManager::SetRenderState(D3DRS_LIGHTING,m->UseLighting);
	StateManager::SetRenderState(D3DRS_AMBIENTMATERIALSOURCE,m->AmbientColorSource);
	StateManager::SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE,m->DiffuseColorSource);
	StateManager::SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE,m->EmissiveColorSource);
	if (m->Mapper[0])
	{
		DebugEventStart(DEBUG_COLOR2,L"Mapper[0]<%i>::Apply",m->Mapper[0]->Mapper_ID());
		m->Mapper[0]->Apply(m->UVSource[0]);
		DebugEventEnd();
	}
	else
	{
		StateManager::SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,m->UVSource[0]);
		StateManager::SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,0);
	}
	if (m->Mapper[1])
	{
		DebugEventStart(DEBUG_COLOR2,L"Mapper[1]<%i>::Apply",m->Mapper[1]->Mapper_ID());
		m->Mapper[1]->Apply(m->UVSource[1]);
		DebugEventEnd();
	}
	else
	{
		StateManager::SetTextureStageState(1,D3DTSS_TEXCOORDINDEX,m->UVSource[1]);
		StateManager::SetTextureStageState(1,D3DTSS_TEXTURETRANSFORMFLAGS,0);
	}
	DebugEventEnd();
}

void FixedFunctionShader::ApplyDefaultMaterial()
{
	DebugEventStart(DEBUG_COLOR1,L"Material<default>::Apply");
	StateManager::SetMaterial(&DefaultMaterial);
	StateManager::SetRenderState(D3DRS_LIGHTING,0);
	StateManager::SetRenderState(D3DRS_AMBIENTMATERIALSOURCE,0);
	StateManager::SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE,0);
	StateManager::SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE,0);
	StateManager::SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0);
	StateManager::SetTextureStageState(1,D3DTSS_TEXCOORDINDEX,1);
	StateManager::SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,0);
	StateManager::SetTextureStageState(1,D3DTSS_TEXTURETRANSFORMFLAGS,0);
	DebugEventEnd();
}

void FixedFunctionShader::ApplyShader()
{
	DebugEventStart(DEBUG_COLOR1,L"ShaderClass<%d>::Apply",DX8Wrapper::render_state.shader.Get_Bits());
	ShaderClass *s = &(DX8Wrapper::render_state.shader);
	s->Apply();
	DebugEventEnd();
}

void FixedFunctionShader::ApplyLight(unsigned int light)
{
	if (DX8Wrapper::render_state.LightEnable[light])
	{
		DX8Wrapper::Set_Light(light, (D3DLIGHT9*)&DX8Wrapper::render_state.Lights[light]);
	}
	else
	{
		DX8Wrapper::Set_Light(light, NULL);
	}
}

void FixedFunctionShader::ApplyVertexBuffer()
{
	if (DX8Wrapper::render_state.vertex_buffer)
	{
		if (DX8Wrapper::render_state.vertex_buffer_type == StreamVertexBufferClass::TYPE)
		{
			StreamVertexBufferClass* vb = (StreamVertexBufferClass*)DX8Wrapper::render_state.vertex_buffer;
			vb->Apply();
		} 
		else if ((!DX8Wrapper::render_state.vertex_buffer_type) || (DX8Wrapper::render_state.vertex_buffer_type == 2))
		{
			DX8Wrapper::D3DDevice->SetStreamSource(0,((DX8VertexBufferClass *)DX8Wrapper::render_state.vertex_buffer)->Get_DX8_Vertex_Buffer(),0,((DeclarationVertexBufferClass *)DX8Wrapper::render_state.vertex_buffer)->DeclarationSize);
			DX8Wrapper::D3DDevice->SetVertexDeclaration(((DeclarationVertexBufferClass *)DX8Wrapper::render_state.vertex_buffer)->VertexDecl);
		}
	}
	else
	{
		DX8Wrapper::D3DDevice->SetStreamSource(0,0,0,0);
	}
}

void FixedFunctionShader::ApplyIndexBuffer()
{
	if (DX8Wrapper::render_state.index_buffer)
	{
		if ((!DX8Wrapper::render_state.index_buffer_type) || (DX8Wrapper::render_state.index_buffer_type == 2))
		{
			DX8Wrapper::D3DDevice->SetIndices(((DX8IndexBufferClass *)DX8Wrapper::render_state.index_buffer)->Get_DX8_Index_Buffer());
		}
	}
	else
	{
		DX8Wrapper::D3DDevice->SetIndices(0);
	}
}

DefaultShaderRegistrant<FixedFunctionShader> FixedFunctionShaderRegistrant;
