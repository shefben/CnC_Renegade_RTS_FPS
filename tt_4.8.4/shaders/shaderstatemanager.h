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
#ifndef _SHADERS_SHADERSTATEMANAGER_H_
#define _SHADERS_SHADERSTATEMANAGER_H_
#define SM_CACHEDSAMPLERS 4

class SHADERS_API StateManager
{
protected:
	static unsigned long RenderStates[210];
	static unsigned long SamplerStates[SM_CACHEDSAMPLERS][14];
	static unsigned long TextureStates[SM_CACHEDSAMPLERS][33];
	static IDirect3DBaseTexture9 *Textures[SM_CACHEDSAMPLERS];
	static BOOL LightEnables[8];
	static void SetDeviceDefaultStates();
	static IDirect3DPixelShader9 *PixelShader;
	static IDirect3DVertexShader9 *VertexShader;
	static float NPatchMode;
public:

	static void Initialize();
	static void Reset();
	static void Shutdown();

	static HRESULT LightEnable(DWORD Index,BOOL Enable);
	static HRESULT SetFVF(DWORD FVF);
	static HRESULT SetLight(DWORD Index,CONST D3DLIGHT9* pLight);
	static HRESULT SetMaterial(CONST D3DMATERIAL9* pMaterial);
	static HRESULT SetNPatchMode(FLOAT nSegments);
	static HRESULT SetPixelShader(LPDIRECT3DPIXELSHADER9 pShader);
	static HRESULT SetPixelShaderConstantB(UINT StartRegister, CONST BOOL* pConstantData, UINT RegisterCount);
	static HRESULT SetPixelShaderConstantF(UINT StartRegister, CONST FLOAT* pConstantData, UINT RegisterCount);
	static HRESULT SetPixelShaderConstantI(UINT StartRegister, CONST INT* pConstantData, UINT RegisterCount);
	static HRESULT SetRenderState(D3DRENDERSTATETYPE State, DWORD Value);
	static HRESULT GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue);
	static DWORD   GetRenderState(D3DRENDERSTATETYPE State);
	static HRESULT SetRenderTarget(DWORD RenderTargetIndex,LPDIRECT3DSURFACE9 pRenderTarget);
	static HRESULT SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value);
	static HRESULT GetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD *pValue);
	static HRESULT SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value);
	static HRESULT GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD *pValue);
	static HRESULT SetTexture(DWORD Stage, LPDIRECT3DBASETEXTURE9 pTexture);
	static HRESULT SetTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix);
	static HRESULT SetVertexShader(LPDIRECT3DVERTEXSHADER9 pShader);
	static HRESULT SetVertexShaderConstantB(UINT StartRegister,CONST BOOL* pConstantData,UINT RegisterCount);
	static HRESULT SetVertexShaderConstantF(UINT StartRegister, CONST FLOAT* pConstantData, UINT RegisterCount);
	static HRESULT SetVertexShaderConstantI(UINT StartRegister, CONST INT* pConstantData, UINT RegisterCount);
};

#endif
