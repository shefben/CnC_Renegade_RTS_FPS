/*	Renegade Scripts.dll
	Effect system
	Copyright 2009 Mark Sararu

	This file is part of the Renegade scripts.dll
	The Renegade scripts.dll is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2, or (at your option) any later
	version. See the file COPYING for more details.
	In addition, an exemption is given to allow Run Time Dynamic Linking of this code with any closed source module that does not contain code covered by this licence.
	Only the source code to the module(s) containing the licenced code has to be released.
*/
#ifndef _EFFECT_H_
#define _EFFECT_H_
#include "engine_vector.h"
#include "SList.h"
namespace EffectSystem
{	
	void Initialize();
	void Destroy();
	class __declspec(novtable) BindAddress: public RefCountClass
	{
	public:
		BindAddress(): RefCountClass(0) {};
		virtual ~BindAddress() {};
		virtual const char* GetAddress() = 0;
		virtual bool GetData(void** data,unsigned long *size) = 0;
		virtual bool IsDirty() {return true;}
	};

	class BoundDataCollection
	{
	private:
		SList<BindAddress> Binds;
	public:
		BoundDataCollection();
		~BoundDataCollection();
		void AddBinding(BindAddress *bind);
		void RemoveBinding(BindAddress *bind);
		BindAddress *FindBindAddressByString(const char *address); // This function is expensive, avoid calling often
	};
	extern BoundDataCollection *GlobalBindings;

	enum EffectStatus
	{
		EffectStatus_Usable = 0,
		EffectStatus_Error,
		EffectStatus_Created,
		EffectStatus_DeviceLost,
	};

	typedef const char * EffectHandle;
	class __declspec(novtable) Effect: public RefCountClass
	{
	protected:
		Effect(): RefCountClass() {};
	public:
		virtual bool Initialize() = 0; // Force an immediate  loading of the effect
		virtual EffectStatus GetStatus() = 0; // Gets the status of the effect
		virtual bool OnDeviceLost() = 0; // Call whenever the render device is lost
		virtual bool OnDeviceReset() = 0; // Call whenever the render device is reset

		virtual bool Begin(unsigned int *passes, DWORD flags) = 0; // Prepare an effect for rendering
		virtual bool BeginPass(unsigned int pass) = 0; // Prepare pass for rendering
		virtual bool ApplyChanges() = 0; // Apply state changes
		virtual bool EndPass() = 0; // Finish rendering pass
		virtual bool End() = 0; // Finish rendering

		virtual bool SetValue(EffectHandle handle, const void *data, unsigned int size) = 0; // Sets a value
		virtual bool SetRawValue(EffectHandle handle, const void *data, unsigned int offset, unsigned int size) = 0; // Sets a value via a memcpy
	};

	class __declspec(novtable) PreprocessorMacroDefinition: public RefCountClass
	{
	public:
		virtual bool GetValue(void* value, size_t& size);

	};

	class __declspec(novtable) EffectFactoryClass: public RefCountClass
	{
	protected:
		EffectFactoryClass(): RefCountClass() {};
	public:
		virtual bool CreateEffect(const char *filename, EffectSystem::Effect **effect)
		{
			return this->CreateEffect(filename,NULL,effect);
		};
		virtual bool CreateEffect(const char *filename, BoundDataCollection *localbindings, Effect** effect) = 0;
		virtual void ResetEffects() {};
	};
	extern EffectFactoryClass *EffectFactory;
};

// If I find that a shader that is using this stuff directly, I will personally terminate the author... -SH (Unless of course I wrote it ^_^)
#ifdef _EFFECTSYSTEM_PRIVATE
namespace EffectSystemPrivate
{
	class EffectImpl;
	class Variable: public RefCountClass
	{
	protected:
		EffectImpl *Effect;
		D3DXHANDLE Handle;
		EffectSystem::BindAddress *BindAddress;
	public:
		Variable();
		~Variable();
		//EffectSystem::BindAddress *GetBindAddress();
		void SetBindAddress(EffectSystem::BindAddress *address);
		void SetEffect(EffectImpl *effect, D3DXHANDLE handle);
		void Apply();
		inline bool IsDirty()
		{
			return BindAddress->IsDirty();
		};
	};

	class EffectIncludeClass : public ID3DXInclude
	{
	public:
		HRESULT __stdcall Open(D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID * ppData, UINT * pBytes);
		HRESULT __stdcall Close(LPCVOID pData);
	};

	class EffectImpl: public EffectSystem::Effect
	{
	friend class EffectFactoryImpl;
	friend class Variable;
	protected:
		bool applyClean;
		const char *_name;
		ID3DXEffect *_effect;
		EffectSystem::EffectStatus _status;
		EffectSystem::BoundDataCollection *LocalBindings;
		SList<Variable> variables;
		void BuildVariableVector();
	public:
		EffectImpl(const char *name, EffectSystem::BoundDataCollection *localbindings);
		~EffectImpl();
		bool Initialize();
		EffectSystem::EffectStatus GetStatus();
		bool OnDeviceLost();
		bool OnDeviceReset();

		bool Begin(unsigned int *passes, DWORD flags);
		bool BeginPass(unsigned int pass);
		bool ApplyChanges();
		bool EndPass();
		bool End();

		bool SetValue(EffectSystem::EffectHandle handle, const void *data, unsigned int size);
		bool SetRawValue(EffectSystem::EffectHandle handle, const void *data, unsigned int offset, unsigned int size);
	};

	class EffectFactoryImpl: public EffectSystem::EffectFactoryClass
	{
	protected:
		SList<EffectSystemPrivate::EffectImpl> effects;
	public:
		EffectFactoryImpl(): EffectSystem::EffectFactoryClass(), effects() {};
		bool CreateEffect(const char *filename, EffectSystem::BoundDataCollection *localbindings, EffectSystem::Effect** effect);
		void ResetEffects();
	};
};

class ShaderStateManager : public ID3DXEffectStateManager {
protected:
	long ref;
public:
	ShaderStateManager();
	virtual HRESULT __stdcall QueryInterface(const IID &iid,LPVOID *ppv);
	virtual unsigned long __stdcall AddRef();
	virtual unsigned long __stdcall Release();
	virtual HRESULT __stdcall LightEnable(DWORD Index,BOOL Enable);
	virtual HRESULT __stdcall SetFVF(DWORD FVF);
	virtual HRESULT __stdcall SetLight(DWORD Index,CONST D3DLIGHT9* pLight);
	virtual HRESULT __stdcall SetMaterial(CONST D3DMATERIAL9* pMaterial);
	virtual HRESULT __stdcall SetNPatchMode(FLOAT nSegments);
	virtual HRESULT __stdcall SetPixelShader(LPDIRECT3DPIXELSHADER9 pShader);
	virtual HRESULT __stdcall SetPixelShaderConstantB(UINT StartRegister, CONST BOOL* pConstantData, UINT RegisterCount);
	virtual HRESULT __stdcall SetPixelShaderConstantF(UINT StartRegister, CONST FLOAT* pConstantData, UINT RegisterCount);
	virtual HRESULT __stdcall SetPixelShaderConstantI(UINT StartRegister, CONST INT* pConstantData, UINT RegisterCount);
	virtual HRESULT __stdcall SetRenderState(D3DRENDERSTATETYPE State, DWORD Value);
	virtual HRESULT __stdcall GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue);
	virtual DWORD   __stdcall GetRenderState(D3DRENDERSTATETYPE State);
	virtual HRESULT __stdcall SetRenderTarget(DWORD RenderTargetIndex,LPDIRECT3DSURFACE9 pRenderTarget);
	virtual HRESULT __stdcall SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value);
	virtual HRESULT __stdcall GetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD *pValue);
	virtual HRESULT __stdcall SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value);
	virtual HRESULT __stdcall GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD *pValue);
	virtual HRESULT __stdcall SetTexture(DWORD Stage, LPDIRECT3DBASETEXTURE9 pTexture);
	virtual HRESULT __stdcall SetTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix);
	virtual HRESULT __stdcall SetVertexShader(LPDIRECT3DVERTEXSHADER9 pShader);
	virtual HRESULT __stdcall SetVertexShaderConstantB(UINT StartRegister,CONST BOOL* pConstantData,UINT RegisterCount);
	virtual HRESULT __stdcall SetVertexShaderConstantF(UINT StartRegister, CONST FLOAT* pConstantData, UINT RegisterCount);
	virtual HRESULT __stdcall SetVertexShaderConstantI(UINT StartRegister, CONST INT* pConstantData, UINT RegisterCount);
};
#endif



#endif // include guard
