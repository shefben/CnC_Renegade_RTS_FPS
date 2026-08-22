/*	Renegade Scripts.dll
	Shader State Manager
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

#include "shaderstatemanager.h"
#include "DX8Wrapper.h"
#include "engine_math.h"

unsigned long StateManager::RenderStates[210];
unsigned long StateManager::SamplerStates[SM_CACHEDSAMPLERS][14];
unsigned long StateManager::TextureStates[SM_CACHEDSAMPLERS][33];
IDirect3DBaseTexture9 *StateManager::Textures[SM_CACHEDSAMPLERS];
BOOL StateManager::LightEnables[8];
IDirect3DPixelShader9 *StateManager::PixelShader;
IDirect3DVertexShader9 *StateManager::VertexShader;
float StateManager::NPatchMode;

void StateManager::Initialize()
{
	StateManager::Reset();
};

void StateManager::Reset()
{
	SetDeviceDefaultStates();
	SetRenderState(D3DRS_RANGEFOGENABLE, ShaderCaps::RangeFogSupported);
	SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
	SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
	SetTextureStageState(0,D3DTSS_BUMPENVMAT00,F2DW(1.0f));
	SetTextureStageState(0,D3DTSS_BUMPENVMAT11,F2DW(1.0f));
	SetTextureStageState(1,D3DTSS_BUMPENVLSCALE,F2DW(1.0f));
	VertexShader = 0;
	PixelShader = 0;
	NPatchMode = 1.0f;
	memset(Textures,0,sizeof(Textures));
	memset(LightEnables,0,sizeof(LightEnables));
	for (int i = 0; i < SM_CACHEDSAMPLERS; i++) 
	{
		Textures[i] = 0;
		//SamplerStates[i][D3DSAMP_MAXANISOTROPY]			= 2;
	}
};

void StateManager::Shutdown()
{
	
};

void StateManager::SetDeviceDefaultStates()
{
	RenderStates[D3DRS_ZENABLE]			= D3DZB_TRUE;
	RenderStates[D3DRS_FILLMODE]		= D3DFILL_SOLID;
	RenderStates[D3DRS_SHADEMODE]		= D3DSHADE_GOURAUD;
	RenderStates[D3DRS_ZWRITEENABLE]	= TRUE;
	RenderStates[D3DRS_ALPHATESTENABLE]	= FALSE;
	RenderStates[D3DRS_LASTPIXEL]		= TRUE;
	RenderStates[D3DRS_SRCBLEND]		= D3DBLEND_ONE;
	RenderStates[D3DRS_DESTBLEND]		= D3DBLEND_ZERO;
	RenderStates[D3DRS_CULLMODE]		= D3DCULL_CCW;
	RenderStates[D3DRS_ZFUNC]			= D3DCMP_LESSEQUAL;
	RenderStates[D3DRS_ALPHAREF]		= 0;
	RenderStates[D3DRS_ALPHAFUNC]		= D3DCMP_ALWAYS;
	RenderStates[D3DRS_DITHERENABLE]	= FALSE;
	RenderStates[D3DRS_ALPHABLENDENABLE]= FALSE;
	RenderStates[D3DRS_FOGENABLE]		= FALSE;
	RenderStates[D3DRS_SPECULARENABLE]	= FALSE;
	RenderStates[D3DRS_FOGCOLOR]		= 0;
	RenderStates[D3DRS_FOGTABLEMODE]	= D3DFOG_NONE;
	RenderStates[D3DRS_FOGSTART]		= F2DW(0.0f);
	RenderStates[D3DRS_FOGEND]			= F2DW(1.0f);
	RenderStates[D3DRS_FOGDENSITY]		= F2DW(1.0f);
	RenderStates[D3DRS_RANGEFOGENABLE]	= FALSE;
	RenderStates[D3DRS_STENCILENABLE]	= FALSE;
	RenderStates[D3DRS_STENCILFAIL]		= D3DSTENCILOP_KEEP;
	RenderStates[D3DRS_STENCILZFAIL]	= D3DSTENCILOP_KEEP;
	RenderStates[D3DRS_STENCILPASS]		= D3DSTENCILOP_KEEP;
	RenderStates[D3DRS_STENCILFUNC]		= D3DCMP_ALWAYS;
	RenderStates[D3DRS_STENCILREF]		= 0;
	RenderStates[D3DRS_STENCILMASK]		= 0xFFFFFFFF;
	RenderStates[D3DRS_STENCILWRITEMASK]= 0xFFFFFFFF;
	RenderStates[D3DRS_TEXTUREFACTOR]	= 0xFFFFFFFF;
	RenderStates[D3DRS_WRAP0]			= 0;
	RenderStates[D3DRS_WRAP1]			= 0;
	RenderStates[D3DRS_WRAP2]			= 0;
	RenderStates[D3DRS_WRAP3]			= 0;
	RenderStates[D3DRS_WRAP4]			= 0;
	RenderStates[D3DRS_WRAP5]			= 0;
	RenderStates[D3DRS_WRAP6]			= 0;
	RenderStates[D3DRS_WRAP7]			= 0;
	RenderStates[D3DRS_CLIPPING]		= TRUE;
	RenderStates[D3DRS_LIGHTING]		= TRUE;
	RenderStates[D3DRS_AMBIENT]			= 0;
	RenderStates[D3DRS_FOGVERTEXMODE]	= D3DFOG_NONE;
	RenderStates[D3DRS_COLORVERTEX]		= TRUE;
	RenderStates[D3DRS_LOCALVIEWER]		= TRUE;
	RenderStates[D3DRS_NORMALIZENORMALS]= FALSE;
	RenderStates[D3DRS_DIFFUSEMATERIALSOURCE]	= D3DMCS_COLOR1;
	RenderStates[D3DRS_SPECULARMATERIALSOURCE]	= D3DMCS_COLOR2;
	RenderStates[D3DRS_AMBIENTMATERIALSOURCE]	= D3DMCS_MATERIAL;
	RenderStates[D3DRS_EMISSIVEMATERIALSOURCE]	= D3DMCS_MATERIAL;
	RenderStates[D3DRS_VERTEXBLEND]		= D3DVBF_DISABLE;
	RenderStates[D3DRS_CLIPPLANEENABLE]	= 0;
	RenderStates[D3DRS_POINTSIZE]		= F2DW(64.0f);
	RenderStates[D3DRS_POINTSIZE_MIN]	= F2DW(1.0f);
	RenderStates[D3DRS_POINTSPRITEENABLE]	= FALSE;
	RenderStates[D3DRS_POINTSCALEENABLE]	= FALSE;
	RenderStates[D3DRS_POINTSCALE_A]	= F2DW(1.0f);
	RenderStates[D3DRS_POINTSCALE_B]	= F2DW(0.0f);
	RenderStates[D3DRS_POINTSCALE_C]	= F2DW(0.0f);
	RenderStates[D3DRS_MULTISAMPLEANTIALIAS]= TRUE;
	RenderStates[D3DRS_MULTISAMPLEMASK]	= 0xFFFFFFFF;
	RenderStates[D3DRS_PATCHEDGESTYLE]	= D3DPATCHEDGE_DISCRETE;
	RenderStates[D3DRS_DEBUGMONITORTOKEN]	= D3DDMT_ENABLE;
	RenderStates[D3DRS_POINTSIZE_MAX]	= F2DW(64.0f);
	RenderStates[D3DRS_INDEXEDVERTEXBLENDENABLE]	= FALSE;
	RenderStates[D3DRS_COLORWRITEENABLE]	= 0x0000000F;
	RenderStates[D3DRS_TWEENFACTOR]		= 0;
	RenderStates[D3DRS_BLENDOP]			= D3DBLENDOP_ADD;
	RenderStates[D3DRS_POSITIONDEGREE]	= D3DDEGREE_CUBIC;
	RenderStates[D3DRS_NORMALDEGREE]	= D3DDEGREE_LINEAR;
	RenderStates[D3DRS_SCISSORTESTENABLE]	= FALSE;
	RenderStates[D3DRS_SLOPESCALEDEPTHBIAS]	= 0;
	RenderStates[D3DRS_ANTIALIASEDLINEENABLE]	= FALSE;
	RenderStates[D3DRS_MINTESSELLATIONLEVEL]	= F2DW(1.0f);
	RenderStates[D3DRS_MAXTESSELLATIONLEVEL]	= F2DW(1.0f);
	RenderStates[D3DRS_ADAPTIVETESS_X]	= F2DW(0.0f);
	RenderStates[D3DRS_ADAPTIVETESS_Y]	= F2DW(0.0f);
	RenderStates[D3DRS_ADAPTIVETESS_Z]	= F2DW(1.0f);
	RenderStates[D3DRS_ADAPTIVETESS_W]	= F2DW(0.0f);
	RenderStates[D3DRS_ENABLEADAPTIVETESSELLATION]	= FALSE;
	RenderStates[D3DRS_TWOSIDEDSTENCILMODE]	= FALSE;
	RenderStates[D3DRS_CCW_STENCILFAIL]	= D3DSTENCILOP_KEEP;
	RenderStates[D3DRS_CCW_STENCILZFAIL]= D3DSTENCILOP_KEEP;
	RenderStates[D3DRS_CCW_STENCILPASS]	= D3DSTENCILOP_KEEP;
	RenderStates[D3DRS_CCW_STENCILFUNC]	= D3DCMP_ALWAYS;
	RenderStates[D3DRS_COLORWRITEENABLE1]	= 0x0000000F;
	RenderStates[D3DRS_COLORWRITEENABLE2]	= 0x0000000F;
	RenderStates[D3DRS_COLORWRITEENABLE3]	= 0x0000000F;
	RenderStates[D3DRS_BLENDFACTOR]		= 0xFFFFFFFF;
	RenderStates[D3DRS_SRGBWRITEENABLE]	= 0;
	RenderStates[D3DRS_DEPTHBIAS]		= 0;
	RenderStates[D3DRS_WRAP8]			= 0;
	RenderStates[D3DRS_WRAP9]			= 0;
	RenderStates[D3DRS_WRAP10]			= 0;
	RenderStates[D3DRS_WRAP11]			= 0;
	RenderStates[D3DRS_WRAP12]			= 0;
	RenderStates[D3DRS_WRAP13]			= 0;
	RenderStates[D3DRS_WRAP14]			= 0;
	RenderStates[D3DRS_WRAP15]			= 0;
	RenderStates[D3DRS_SEPARATEALPHABLENDENABLE]= FALSE;
	RenderStates[D3DRS_SRCBLENDALPHA]	= D3DBLEND_ONE;
	RenderStates[D3DRS_DESTBLENDALPHA]	= D3DBLEND_ZERO;
	RenderStates[D3DRS_BLENDOPALPHA]	= D3DBLENDOP_ADD;
	for (int i = 0; i < SM_CACHEDSAMPLERS; i++) 
	{
		SamplerStates[i][D3DSAMP_ADDRESSU] = D3DTADDRESS_WRAP;
		SamplerStates[i][D3DSAMP_ADDRESSV] = D3DTADDRESS_WRAP;
		SamplerStates[i][D3DSAMP_ADDRESSW] = D3DTADDRESS_WRAP;
		SamplerStates[i][D3DSAMP_BORDERCOLOR] = 0;
		SamplerStates[i][D3DSAMP_MAGFILTER] = D3DTEXF_POINT;
		SamplerStates[i][D3DSAMP_MINFILTER] = D3DTEXF_POINT;
		SamplerStates[i][D3DSAMP_MIPFILTER] = D3DTEXF_NONE;
		SamplerStates[i][D3DSAMP_MIPMAPLODBIAS] = 0;
		SamplerStates[i][D3DSAMP_MAXMIPLEVEL] = 0;
		SamplerStates[i][D3DSAMP_MAXANISOTROPY] = 1;
		SamplerStates[i][D3DSAMP_SRGBTEXTURE] = 0;
		SamplerStates[i][D3DSAMP_ELEMENTINDEX] = 0;
		SamplerStates[i][D3DSAMP_DMAPOFFSET] = 0;
		TextureStates[i][D3DTSS_COLOROP] = D3DTOP_DISABLE;
		TextureStates[i][D3DTSS_COLORARG1] = D3DTA_TEXTURE;
		TextureStates[i][D3DTSS_COLORARG2] = D3DTA_CURRENT;
		TextureStates[i][D3DTSS_ALPHAOP] = D3DTOP_DISABLE;
		TextureStates[i][D3DTSS_ALPHAARG1] = D3DTA_DIFFUSE;
		TextureStates[i][D3DTSS_ALPHAARG2] = D3DTA_CURRENT;
		TextureStates[i][D3DTSS_BUMPENVMAT00] = F2DW(0.0f);
		TextureStates[i][D3DTSS_BUMPENVMAT01] = F2DW(0.0f);
		TextureStates[i][D3DTSS_BUMPENVMAT10] = F2DW(0.0f);
		TextureStates[i][D3DTSS_BUMPENVMAT11] = F2DW(0.0f);
		TextureStates[i][D3DTSS_TEXCOORDINDEX] = i;
		TextureStates[i][D3DTSS_BUMPENVLSCALE] = F2DW(0.0f);
		TextureStates[i][D3DTSS_BUMPENVLOFFSET] = F2DW(0.0f);
		TextureStates[i][D3DTSS_TEXTURETRANSFORMFLAGS] = D3DTTFF_DISABLE;
		TextureStates[i][D3DTSS_COLORARG0] = D3DTA_CURRENT;
		TextureStates[i][D3DTSS_ALPHAARG0] = D3DTA_CURRENT;
		TextureStates[i][D3DTSS_RESULTARG] = D3DTA_CURRENT;
		TextureStates[i][D3DTSS_CONSTANT] = 0xFFFFFFFF;
	}
	TextureStates[0][D3DTSS_COLOROP] = D3DTOP_MODULATE;
	TextureStates[0][D3DTSS_ALPHAOP] = D3DTOP_SELECTARG1;
	TextureStates[0][D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
	TextureStates[1][D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
};
HRESULT StateManager::LightEnable(DWORD Index, BOOL Enable)
{
	if (LightEnables[Index] == Enable) return S_OK;
	LightEnables[Index] = Enable;
	return DX8Wrapper::D3DDevice->LightEnable(Index, Enable);
}
HRESULT StateManager::SetFVF(DWORD FVF)
{
	return DX8Wrapper::D3DDevice->SetFVF(FVF);
}
HRESULT StateManager::SetLight(DWORD Index,CONST D3DLIGHT9* pLight)
{
	return DX8Wrapper::D3DDevice->SetLight(Index,pLight);
}
HRESULT StateManager::SetMaterial(CONST D3DMATERIAL9* pMaterial)
{
	return DX8Wrapper::D3DDevice->SetMaterial(pMaterial);
}
HRESULT StateManager::SetNPatchMode(FLOAT nSegments)
{
	if (NPatchMode == nSegments)
	{
		return S_OK;
	}
	NPatchMode = nSegments;
	return DX8Wrapper::D3DDevice->SetNPatchMode(nSegments);
}
HRESULT StateManager::SetPixelShader(LPDIRECT3DPIXELSHADER9 pShader)
{
	if (PixelShader == pShader)
	{
		return S_OK;
	}
	PixelShader = pShader;
	return DX8Wrapper::D3DDevice->SetPixelShader(pShader);
}
HRESULT StateManager::SetPixelShaderConstantB(UINT StartRegister, CONST BOOL* pConstantData, UINT RegisterCount)
{
	return DX8Wrapper::D3DDevice->SetPixelShaderConstantB(StartRegister,pConstantData,RegisterCount);
}
HRESULT StateManager::SetPixelShaderConstantF(UINT StartRegister, CONST FLOAT* pConstantData, UINT RegisterCount)
{
	return DX8Wrapper::D3DDevice->SetPixelShaderConstantF(StartRegister,pConstantData,RegisterCount);
}
HRESULT StateManager::SetPixelShaderConstantI(UINT StartRegister, CONST INT* pConstantData, UINT RegisterCount)
{
	return DX8Wrapper::D3DDevice->SetPixelShaderConstantI(StartRegister,pConstantData,RegisterCount);
}
HRESULT StateManager::SetRenderState(D3DRENDERSTATETYPE State, const DWORD Value)
{
	if (RenderStates[State] == Value)
	{
		return S_OK;
	}
	RenderStates[State] = Value;
	return DX8Wrapper::D3DDevice->SetRenderState(State,Value);
}
HRESULT StateManager::GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue)
{
	*pValue = RenderStates[State];
	return S_OK;
}
DWORD   StateManager::GetRenderState(D3DRENDERSTATETYPE State)
{
	return RenderStates[State];
}
HRESULT StateManager::SetRenderTarget(DWORD RenderTargetIndex,LPDIRECT3DSURFACE9 pRenderTarget)
{
	return DX8Wrapper::D3DDevice->SetRenderTarget(RenderTargetIndex,pRenderTarget);
}
HRESULT StateManager::SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
{
	if (SamplerStates[Sampler][Type] == Value)
	{
		return S_OK;
	}
	SamplerStates[Sampler][Type] = Value;
	return DX8Wrapper::D3DDevice->SetSamplerState(Sampler,Type,Value);
}
HRESULT StateManager::GetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD *pValue)
{
	*pValue = SamplerStates[Sampler][Type];
	return S_OK;
}
HRESULT StateManager::SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
{
	if (TextureStates[Stage][Type] == Value)
	{
		return S_OK;
	}
	TextureStates[Stage][Type] = Value;
	return DX8Wrapper::D3DDevice->SetTextureStageState(Stage,Type,Value);
}
HRESULT StateManager::GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD *pValue)
{
	*pValue = TextureStates[Stage][Type];
	return S_OK;
}
HRESULT StateManager::SetTexture(DWORD Stage, LPDIRECT3DBASETEXTURE9 pTexture)
{
	if (Textures[Stage] == pTexture)
	{
		return S_OK;
	}
	Textures[Stage] = pTexture;
	return DX8Wrapper::D3DDevice->SetTexture(Stage,pTexture);
}
HRESULT StateManager::SetTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix)
{
	return DX8Wrapper::D3DDevice->SetTransform(State,pMatrix);
}
HRESULT StateManager::SetVertexShader(LPDIRECT3DVERTEXSHADER9 pShader)
{
	if (VertexShader == pShader)
	{
		return S_OK;
	}
	VertexShader = pShader;
	return DX8Wrapper::D3DDevice->SetVertexShader(pShader);
}
HRESULT StateManager::SetVertexShaderConstantB(UINT StartRegister,CONST BOOL* pConstantData,UINT RegisterCount)
{
	return DX8Wrapper::D3DDevice->SetVertexShaderConstantB(StartRegister,pConstantData,RegisterCount);
}
HRESULT StateManager::SetVertexShaderConstantF(UINT StartRegister, CONST FLOAT* pConstantData, UINT RegisterCount)
{
	return DX8Wrapper::D3DDevice->SetVertexShaderConstantF(StartRegister,pConstantData,RegisterCount);
}
HRESULT StateManager::SetVertexShaderConstantI(UINT StartRegister, CONST INT* pConstantData, UINT RegisterCount)
{
	return DX8Wrapper::D3DDevice->SetVertexShaderConstantI(StartRegister,pConstantData,RegisterCount);
}
