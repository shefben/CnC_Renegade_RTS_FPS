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
#include "general.h"

#include "w3dbindings.h"
#include "w3dbindings_transform.h"
#include "mapper.h"
#include "shaderstatemanager.h"
#include "DX8Wrapper.h"
#include "WW3D.h"
#include "PhysicsSceneClass.h"
#pragma warning(disable: 6011)

namespace W3DBindings
{
	DECLARE_W3D_BINDING(Fog)
	{
		assert(data != NULL);
		assert(size != NULL);
		static W3DFogStruct fog;
		fog.Mode = FogState.Mode;
		fog.Color = FogState.Color;
		fog.Density = FogState.Density;
		fog.Start = FogState.Start;
		fog.End = FogState.End;
		*data = &fog;
		*size = sizeof(fog);
		return true;
	};

	DECLARE_W3D_BINDING(Fog2)
	{
		assert(data != NULL);
		assert(size != NULL);
		static W3DFogStruct2 fog;
		fog.Mode = FogState.Mode;
		fog.Color = FogState.Color;
		fog.Density = FogState.Density;
		float range = FogState.End - FogState.Start;
		fog.FogEndOverRange = FogState.End / range;
		fog.OneOverRange = 1.0f / range;
		*data = &fog;
		*size = sizeof(fog);
		return true;
	};

	DECLARE_W3D_BINDING(EmissiveColor)
	{
		assert(data != NULL);
		assert(size != NULL);

		*data = &(DX8Wrapper::render_state.material->Material->Emissive);
		*size = sizeof(Vector4);
		return true;
	};

	DECLARE_W3D_BINDING(DiffuseColor)
	{
		assert(data != NULL);
		assert(size != NULL);

		*data = &(DX8Wrapper::render_state.material->Material->Diffuse);
		*size = sizeof(Vector4);
		return true;
	};

	DECLARE_ROOT_BINDING(TexCoord0)
	{
		assert(data != NULL);
		assert(size != NULL);

		static Matrix4 mat;

		TextureMapperClass* mapper = DX8Wrapper::render_state.material->Mapper[0];
		if (mapper)
		{
			TT_ASSERT(mapper->Mapper_ID()); // Not one of our mappers, vtbl may be incorrect
			mapper->ApplyToMatrix(mat, 0);
			*data = &mat;
		}
		else
		{
			*data = (void*)&Matrix4::IDENTITY; // Yes, IDENTITY is const. But I promise not to change it.
		}
		*size = sizeof(Matrix4);
		return true;
	};

	DECLARE_ROOT_BINDING(TexCoord1)
	{
		assert(data != NULL);
		assert(size != NULL);

		static Matrix4 mat;

		TextureMapperClass* mapper = DX8Wrapper::render_state.material->Mapper[1];
		if (mapper)
		{
			TT_ASSERT(mapper->Mapper_ID()); // Not one of our mappers, vtbl may be incorrect
			mapper->ApplyToMatrix(mat, 1);
			*data = &mat;
		}
		else
		{
			*data = (void*)&Matrix4::IDENTITY; // Yes, IDENTITY is const. But I promise not to change it.
		}
		*size = sizeof(Matrix4);
		return true;
	};

	DECLARE_W3D_BINDING(Sampler0Settings)
	{
		assert(data != NULL);
		assert(size != NULL);

		static SamplerSettings sampler = {D3DTADDRESS_WRAP, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE, 0};

		if (DX8Wrapper::render_state.Textures[0])
		{
			TextureClass* t = DX8Wrapper::render_state.Textures[0];
			sampler.AddressU = t->UAddressMode ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP;
			sampler.AddressV = t->VAddressMode ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP;
			sampler.MinFilter = _MinTextureFilters[t->TextureMinFilter];
			sampler.MagFilter = _MagTextureFilters[t->TextureMagFilter];
			sampler.MipFilter = _MipMapFilters[t->MipMapFilter];
			sampler.MaxAnisotropy = 2;
		}

		*data = &sampler;
		*size = sizeof(SamplerSettings);
		return true;
	};

	DECLARE_W3D_BINDING(SoALightData)
	{
		assert(data != NULL);
		assert(size != NULL);
		static SoALightDataStruct soalight;

		for (int i = 0; i < 4; ++i)
		{
			if (DX8Wrapper::render_state.LightEnable[i])
			{
				soalight.light_x[i] = DX8Wrapper::render_state.Lights[i].Direction.x;
				soalight.light_y[i] = DX8Wrapper::render_state.Lights[i].Direction.y;
				soalight.light_z[i] = DX8Wrapper::render_state.Lights[i].Direction.z;
				soalight.light_color[i] = GetColorVector3(DX8Wrapper::render_state.Lights[i].Diffuse); 
			}
			else
			{
				soalight.light_x[i] = 0.0f;
				soalight.light_y[i] = 0.0f;
				soalight.light_z[i] = 0.0f;
				soalight.light_color[i] = Vector3(0,0,0);
			}
		}

		*data = &soalight;
		*size = sizeof(SoALightDataStruct);
		return true;
	};

	DECLARE_W3D_BINDING(LightDirections) // deprecated
	{
		assert(data != NULL);
		assert(size != NULL);
		static Vector4 directions[4]; 
		memset(directions, 0x00, sizeof(directions));
		int j = 0;
		for (int i = 0; i < 4; ++i)
		{
			if (DX8Wrapper::render_state.LightEnable[i])
			{
				//Let's pack these lights as close as possible together
				directions[j] = Vector4(-DX8Wrapper::render_state.Lights[i].Direction.x,
										-DX8Wrapper::render_state.Lights[i].Direction.y,
										-DX8Wrapper::render_state.Lights[i].Direction.z,
										0.0f) ;
				++j;
			}
		};
		*data = &directions;
		*size = sizeof(directions);
		return true;
	};

	DECLARE_W3D_BINDING(LightColors) // deprecated
	{
		assert(data != NULL);
		assert(size != NULL);
		static Vector4 colors[4]; 
		memset(colors, 0x00, sizeof(colors));
		int j = 0;
		for (int i = 0; i < 4; ++i)
		{
			if (DX8Wrapper::render_state.LightEnable[i])
			{
				//Let's pack these lights as close as possible together
				colors[j] = Vector4(DX8Wrapper::render_state.Lights[i].Diffuse.r,
									DX8Wrapper::render_state.Lights[i].Diffuse.g,
									DX8Wrapper::render_state.Lights[i].Diffuse.b,
									DX8Wrapper::render_state.Lights[i].Diffuse.a);
				++j;
			}
		};
		*data = &colors;
		*size = sizeof(colors);
		return true;
	};

	DECLARE_W3D_BINDING(LightEnables) // deprecated
	{
		assert(data != NULL);
		assert(size != NULL);
		static BOOL enables[4]; 
		memset(enables, 0x00, sizeof(enables));
		int j = 0;
		for (int i = 0; i < 4; ++i)
		{
			if (DX8Wrapper::render_state.LightEnable[i])
			{
				//Let's pack these lights as close as possible together
				enables[j] = TRUE;
				++j;
			}
		};
		*data = &enables;
		*size = sizeof(enables);
		return true;
	};

	DECLARE_ROOT_BINDING(AmbientColor)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Vector4 ambientcolor;
		ambientcolor = GetColorVector4(StateManager::GetRenderState(D3DRS_AMBIENT));
		*data = &ambientcolor;
		*size = sizeof(Vector4);
		return true;
	};

	DECLARE_ROOT_BINDING(Time)
	{
		assert(data != NULL);
		assert(size != NULL);
		static float time;
		time = (float)WW3D::SyncTime / 1000.0f;
		*data = &time;
		*size = sizeof(float);
		return true;
	};

	DECLARE_ROOT_BINDING(Texture0)
	{
		assert(data != NULL);
		assert(size != NULL);
		static IDirect3DTexture9* texture;
		TextureClass *tex = DX8Wrapper::render_state.Textures[0];
		if (tex)
		{
			if (tex->Initialized != true)
			{
				tex->Init();
			}
			tex->LastAccessed = WW3D::SyncTime;
			texture = tex->D3DTexture;
		} 
		else
		{
			texture = NULL;
		}

		*data = &texture;
		*size = sizeof(IDirect3DTexture9*);
		return true;
	};

	DECLARE_NAMESPACED_BINDING(Texture0, Info)
	{
		assert(data != NULL);
		assert(size != NULL);
		static TextureInfoStruct texture_info;
		TextureClass* tex = DX8Wrapper::render_state.Textures[0];
		if (tex)
		{
			if (tex->Initialized != true)
			{
				tex->Init();
			}
			tex->LastAccessed = WW3D::SyncTime;
			texture_info.Width = (float)tex->Width;
			texture_info.Height = (float)tex->Height;
			texture_info.PixelWidth = 1.0f / tex->Width;
			texture_info.PixelHeight = 1.0f / tex->Height;
		} 
		else
		{
			texture_info.Width = 0.0f;
			texture_info.Height = 0.0f;
			texture_info.PixelWidth = 0.0f;
			texture_info.PixelHeight = 0.0f;
		};

		*data = &texture_info;
		*size = sizeof(TextureInfoStruct);
		return true;
	};

	DECLARE_ROOT_BINDING(Texture1)
	{
		assert(data != NULL);
		assert(size != NULL);
		static IDirect3DTexture9* texture;
		TextureClass *tex = DX8Wrapper::render_state.Textures[1];
		if (tex)
		{
			if (tex->Initialized != true)
			{
				tex->Init();
			}
			tex->LastAccessed = WW3D::SyncTime;
			texture = tex->D3DTexture;
		} 
		else
		{
			texture = NULL;
		}

		*data = &texture;
		*size = sizeof(IDirect3DTexture9*);
		return true;
	};

	DECLARE_NAMESPACED_BINDING(Texture1, Info)
	{
		assert(data != NULL);
		assert(size != NULL);
		static TextureInfoStruct texture_info;
		TextureClass* tex = DX8Wrapper::render_state.Textures[1];
		if (tex)
		{
			if (tex->Initialized != true)
			{
				tex->Init();
			}
			tex->LastAccessed = WW3D::SyncTime;
			texture_info.Width = (float)tex->Width;
			texture_info.Height = (float)tex->Height;
			texture_info.PixelWidth = 1.0f / tex->Width;
			texture_info.PixelHeight = 1.0f / tex->Height;
		} 
		else
		{
			texture_info.Width = 0.0f;
			texture_info.Height = 0.0f;
			texture_info.PixelWidth = 0.0f;
			texture_info.PixelHeight = 0.0f;
		};

		*data = &texture_info;
		*size = sizeof(TextureInfoStruct);
		return true;
	};

	DECLARE_ROOT_BINDING(SunLightDirection)
	{
		assert(data != NULL);
		assert(size != NULL);
		static Vector3 sun_light_vector;
		PhysicsSceneClass::Get_Instance()->Get_Sun_Light_Vector(&sun_light_vector);
		*data = &sun_light_vector;
		*size = sizeof(Vector3);
		return true;
	};

};

using namespace W3DBindings;

void W3DBindings::Initialize()
{
	// local (aka W3D.<bindaddress>) bindings
	REGISTER_NAMESPACED_BINDING(W3D, Fog);
	REGISTER_NAMESPACED_BINDING(W3D, Fog2);
	REGISTER_NAMESPACED_BINDING(W3D, EmissiveColor);
	REGISTER_NAMESPACED_BINDING(W3D, DiffuseColor);
	REGISTER_NAMESPACED_BINDING(W3D, Sampler0Settings);
	REGISTER_NAMESPACED_BINDING(W3D, SoALightData);

	// root (aka <bindaddress>) bindings
	REGISTER_BINDING(AmbientColor);
	REGISTER_BINDING(Time);
	REGISTER_BINDING(Texture0);
	REGISTER_BINDING(Texture1);
	REGISTER_BINDING(TexCoord0);
	REGISTER_BINDING(TexCoord1);

	REGISTER_BINDING(SunLightDirection);

	// Texture info bindings
	REGISTER_NAMESPACED_BINDING(Texture0, Info);
	REGISTER_NAMESPACED_BINDING(Texture1, Info);

	// Transforms
	W3DBindings::InitializeTransformBindings();

	// deprecated local bindings
	REGISTER_NAMESPACED_BINDING(W3D, LightDirections);
	REGISTER_NAMESPACED_BINDING(W3D, LightColors);
	REGISTER_NAMESPACED_BINDING(W3D, LightEnables);

	//FIXME FIXME FIXME
	EffectSystem::GlobalBindings->AddBinding(new SceneTextureBindAddress());
};


#pragma region SceneTextureBindAddress
SceneTextureBindAddress::SceneTextureBindAddress(): tex(NULL)
{

};

SceneTextureBindAddress::~SceneTextureBindAddress()
{
	SafeRelease(tex);
};

const char* SceneTextureBindAddress::GetAddress()
{
	return "W3D.SceneTexture";
}

bool SceneTextureBindAddress::GetData(void** data, unsigned long *size)
{
	assert(data != NULL);
	assert(size != NULL);
	if (!tex)
	{
		tex = DX8Wrapper::Create_Render_Target(512,512,(_PresentParameters.BackBufferFormat == D3DFMT_X8R8G8B8 ? WW3D_FORMAT_X8R8G8B8 : WW3D_FORMAT_R5G6B5));
	}
	if (tex->Initialized != true)
	{
		tex->Init();
	}
	tex->LastAccessed = WW3D::SyncTime;

	IDirect3DSurface9 *surf = NULL;
	IDirect3DSurface9 *rt = NULL;
	tex->D3DTexture->GetSurfaceLevel(0,&surf);
	DX8Wrapper::D3DDevice->GetRenderTarget(0,&rt);
	DX8Wrapper::D3DDevice->StretchRect(rt,NULL,surf,NULL,D3DTEXF_POINT);
	rt->Release();
	surf->Release();
	
	*data = &tex->D3DTexture;
	*size = sizeof(IDirect3DTexture9*);
	return true;
};
#pragma endregion
