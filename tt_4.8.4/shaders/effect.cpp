/*	Renegade Scripts.dll
	D3DXEffect Wrapper and related loaders
	Copyright 2009 Mark Sararu

	This file is part of the Renegade scripts.dll
	The Renegade scripts.dll is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2, or (at your option) any later
	version. See the file COPYING for more details.
	In addition, an exemption is given to allow Run Time Dynamic Linking of this code with any closed source module that does not contain code covered by this licence.
	Only the source code to the module(s) containing the licenced code has to be released.
*/
#include "general.h"

#define _EFFECTSYSTEM_PRIVATE
#include "effect.h"
#include "w3dbindings.h"
#include "shaderstatemanager.h"
#include "dx8wrapper.h"
#include "engine_diagnostics.h"
using namespace EffectSystem;
using namespace EffectSystemPrivate;
DebugOutputClass EffectDebug("_shaders.txt");
BoundDataCollection *EffectSystem::GlobalBindings = NULL;
EffectFactoryClass *EffectSystem::EffectFactory = NULL;
ShaderStateManager *D3DStateManager;

void EffectSystem::Initialize()
{
	GlobalBindings = new BoundDataCollection();
	EffectFactory = new EffectFactoryImpl();
	D3DStateManager = new ShaderStateManager();
	W3DBindings::Initialize();
};

void EffectSystem::Destroy()
{
	if (GlobalBindings)
	{
		delete GlobalBindings;
		GlobalBindings = NULL;
	}
	SafeRelease(EffectFactory);
	SafeRelease(D3DStateManager);
};

#pragma region BoundDataCollection
BoundDataCollection::BoundDataCollection(): Binds()
{

};

BoundDataCollection::~BoundDataCollection() // I die, everybody dies!	
{
	for (BindAddress* bind = Binds.Remove_Head(); bind; bind = Binds.Remove_Head())
	{
		// destructive loop
		bind->Release();
	};
};

void BoundDataCollection::AddBinding(EffectSystem::BindAddress *bind)
{
	Binds.Add_Tail(bind);
	bind->Add_Ref();
};

void BoundDataCollection::RemoveBinding(EffectSystem::BindAddress *bind)
{
	Binds.Remove(bind);
	bind->Release_Ref();
};

BindAddress* BoundDataCollection::FindBindAddressByString(const char *address)
{
	if (address == NULL) return NULL; // No address, so no binding

	for (SLNode<BindAddress>* iter = Binds.Head(); iter ; iter = iter->Next())
	{
		BindAddress* bind = iter->Data();
		if (_stricmp(bind->GetAddress(), address) == 0) // This is the one
		{
			bind->Add_Ref();
			return bind;
		};
	}
	return NULL; // AKA no such binding
};
#pragma endregion

Variable::Variable(): RefCountClass()
{
	BindAddress = NULL;
	Effect = NULL;
	Handle = NULL;
};

Variable::~Variable()
{
	SetBindAddress(NULL);
	SetEffect(NULL,NULL);
};

void Variable::SetBindAddress(EffectSystem::BindAddress *address)
{
	if (BindAddress != NULL) BindAddress->Release_Ref();
	if (address != NULL) address->Add_Ref();
	BindAddress = address; 
};

void Variable::SetEffect(EffectImpl *effect, D3DXHANDLE handle)
{
	Effect = effect;
	Handle = handle;
};

void Variable::Apply()
{
	TT_ASSERT(Effect != NULL);
	TT_ASSERT(Handle != NULL);
	TT_ASSERT(BindAddress != NULL);

	void *data = NULL;
	unsigned long size = 0;
	BindAddress->GetData(&data,&size);
#ifdef DEBUG
	bool res = Effect->SetValue(Handle,data,size);
	TT_ASSERT(res);
#else
	Effect->SetValue(Handle,data,size);
#endif

};

HRESULT __stdcall EffectIncludeClass::Open(D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID * ppData, UINT * pBytes)
{
	FileClass *file = Get_Data_File((char*)pFileName);
	if (!(file && file->Open(1)))
	{
		return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}
	int length = file->Size();
	char* data = new char[length];
	file->Read(data,length);
	*ppData = data;
	*pBytes = length;
	Close_Data_File(file);
	return S_OK;
};

HRESULT __stdcall EffectIncludeClass::Close(LPCVOID pData)
{
	delete[] pData;
	return S_OK;
};

EffectImpl::EffectImpl(const char *name, EffectSystem::BoundDataCollection *localbindings): Effect(), variables()
{
	applyClean = true;
	_effect = NULL;
	_name = newstr(name);
	_status = EffectStatus_Created;
	LocalBindings = localbindings;
};

EffectImpl::~EffectImpl()
{
	for (SLNode<Variable>* iter = variables.Head(); iter; iter = iter->Next())
	{
		Variable* var = iter->Data();
		TT_ASSERT(var);
		var->Release();
	};
	variables.Remove_All();

	SafeRelease(_effect);
	if (_name)
	{
		delete[] _name;
		_name = NULL;
	}
};

bool EffectImpl::Initialize()
{
	TT_ASSERT(_status == EffectStatus_Created);

	LPD3DXBUFFER bufferErrors = NULL;
	DWORD flags = D3DXFX_NOT_CLONEABLE;
#ifdef DEBUG
	flags |= D3DXSHADER_DEBUG;
#endif

	//HACK: Doesn't use "filecache" system, it needs to.
	FileClass *file = Get_Data_File((char*)_name);
	if (!(file && file->Open(1)))
	{
		_status = EffectStatus_Error;
		return false;
	}

	unsigned int size = file->Size();
	unsigned char* data = new unsigned char[size];
	file->Read(data,size);
	file->Close();
	Close_Data_File(file);

	static EffectIncludeClass EffectInclude = EffectIncludeClass();

	HRESULT res = D3DXCreateEffect(DX8Wrapper::D3DDevice,data,size,NULL,&EffectInclude,flags,NULL,&_effect,&bufferErrors);
	delete[] data;
	//ENDHACK
	if (FAILED(res) && bufferErrors)
	{
		EffectDebug.ErrorW(DEBUGLEVEL_ERROR,L"[%S] Failed to load. Errors follow this message.\n %S",_name,(LPSTR)bufferErrors->GetBufferPointer());
		bufferErrors->Release();
		_effect = NULL;
		_status = EffectStatus_Error;
		return false;
	}
	SafeRelease(bufferErrors);
	_effect->SetStateManager(D3DStateManager);

	BuildVariableVector();

	_status = EffectStatus_Usable;
	return _status == EffectStatus_Usable ? true: false;
};

void EffectImpl::BuildVariableVector()
{
	D3DXEFFECT_DESC desc;
	_effect->GetDesc(&desc);
	for (unsigned int i = 0; i < desc.Parameters; ++i)
	{
		D3DXHANDLE paramhandle = _effect->GetParameter(NULL,i);
		D3DXPARAMETER_DESC paramdesc;
		_effect->GetParameterDesc(paramhandle,&paramdesc); // TODO: return res checking

		BindAddress *bind = GlobalBindings->FindBindAddressByString(paramdesc.Semantic); // Try to find binding by semantic first, semantics are global only.
		if (!bind)
		{
			D3DXHANDLE bindaddresshandle = _effect->GetAnnotationByName(paramhandle,"SasBindAddress");
			if (bindaddresshandle) 
			{
				const char *bindaddress = NULL;
				_effect->GetString(bindaddresshandle,&bindaddress);
				bind = GlobalBindings->FindBindAddressByString(bindaddress);
			}
			else
			{
				bindaddresshandle = _effect->GetAnnotationByName(paramhandle,"LocalBindAddress");
				if (bindaddresshandle) 
				{
					const char *bindaddress = NULL;
					_effect->GetString(bindaddresshandle,&bindaddress);
					bind = LocalBindings->FindBindAddressByString(bindaddress);
				}
			}
		}

		if (bind == NULL) continue; // This parameter cannot be bound to anything =(
		Variable *var = new Variable();
		var->SetBindAddress(bind);
		var->SetEffect(this,paramhandle);
		variables.Add_Tail(var);
		bind->Release_Ref(); // We are done with it here, Variable::SetBindAddress adds its own reference
	}
}

EffectStatus EffectImpl::GetStatus()
{
	return _status;
};

bool EffectImpl::OnDeviceLost()
{
	TT_ASSERT(_status == EffectStatus_Usable);
	HRESULT hr = _effect->OnLostDevice();
	if (SUCCEEDED(hr))
	{
		_status = EffectStatus_DeviceLost;
		return true;
	}
	_status = EffectStatus_Error;
	return false;
}

bool EffectImpl::OnDeviceReset()
{
	TT_ASSERT(_status == EffectStatus_DeviceLost);
	HRESULT hr = _effect->OnResetDevice();
	if (SUCCEEDED(hr))
	{
		_status = EffectStatus_Usable;
		return true;
	}
	return false;
}


bool EffectImpl::Begin(unsigned int *passes, DWORD flags)
{
	TT_ASSERT(_status == EffectStatus_Usable);
	DebugEventStart(DEBUG_COLOR1,L"Effect<%S>::Begin",_name);
	HRESULT hr = _effect->Begin(passes,flags | D3DXFX_DONOTSAVESTATE);
	for (SLNode<Variable>* iter = variables.Head(); iter; iter = iter->Next())
	{
		Variable* var = iter->Data();
		if (var->IsDirty() || applyClean)
		{
			var->Apply();
		};
	};
	applyClean = false;
	DebugEventEnd();
	return SUCCEEDED(hr) ? true: false;
};

bool EffectImpl::BeginPass(unsigned int pass)
{
	TT_ASSERT(_status == EffectStatus_Usable);
	HRESULT hr = _effect->BeginPass(pass);
	return SUCCEEDED(hr) ? true: false;
};

bool EffectImpl::ApplyChanges()
{
	TT_ASSERT(_status == EffectStatus_Usable);
	HRESULT hr = _effect->CommitChanges();
	return SUCCEEDED(hr) ? true: false;
};

bool EffectImpl::EndPass()
{
	TT_ASSERT(_status == EffectStatus_Usable);
	HRESULT hr = _effect->EndPass();
	return SUCCEEDED(hr) ? true: false;
};

bool EffectImpl::End()
{
	TT_ASSERT(_status == EffectStatus_Usable);
	HRESULT hr = _effect->End();
	return SUCCEEDED(hr) ? true: false;
};

bool EffectImpl::SetValue(EffectSystem::EffectHandle handle, const void *data, unsigned int size)
{
	TT_ASSERT(_status == EffectStatus_Usable);
	HRESULT hr = _effect->SetValue(handle,data,size);
	return SUCCEEDED(hr) ? true: false;
}

bool EffectImpl::SetRawValue(EffectSystem::EffectHandle handle, const void *data, unsigned int offset, unsigned int size)
{
	TT_ASSERT(_status == EffectStatus_Usable);
	HRESULT hr = _effect->SetRawValue(handle,data,offset,size);
	return SUCCEEDED(hr) ? true: false;
}

bool EffectFactoryImpl::CreateEffect(const char *filename, EffectSystem::BoundDataCollection *localbindings, EffectSystem::Effect **effect)
{
	// TODO: This should queue the effect into a creation\initialization queue and return an imposter until the real effect is finished loading
	*effect = new EffectImpl(filename,localbindings);
	effects.Add_Tail((EffectImpl*)*effect);
	return true;
}

void EffectFactoryImpl::ResetEffects()
{
	for (SLNode<EffectSystemPrivate::EffectImpl>* iter = effects.Head(); iter; iter = iter->Next())
	{
		EffectSystemPrivate::EffectImpl* effect = iter->Data();
		TT_ASSERT(effect);

		effect->_effect->Release();
		effect->_status = EffectStatus_Created;
		for (SLNode<Variable>* iter2 = effect->variables.Head(); iter2; iter2 = iter2->Next())
		{
			Variable* var = iter2->Data();
			if (!var) continue;
			var->Release();
		};
		effect->variables.Remove_All();
		effect->Initialize();		
	};
};

ShaderStateManager::ShaderStateManager()
{
	ref = 1;
};

HRESULT ShaderStateManager::QueryInterface(const IID &iid, LPVOID *ppv)
{
	if (iid == IID_IUnknown || iid == IID_ID3DXEffectStateManager)
	{
		*ppv = static_cast<ID3DXEffectStateManager*>(this);
	}
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	reinterpret_cast<IUnknown*>(this)->AddRef();
	return S_OK;
}
unsigned long ShaderStateManager::AddRef()
{
	return (unsigned long)InterlockedIncrement(&ref);
}
unsigned long ShaderStateManager::Release()
{
	if (InterlockedDecrement(&ref) == 0L)
	{
		delete this;
		return 0L;
	}
	return (unsigned long) ref;
}
HRESULT ShaderStateManager::LightEnable(DWORD Index,BOOL Enable)
{
	return StateManager::LightEnable(Index,Enable);
}
HRESULT ShaderStateManager::SetFVF(DWORD FVF)
{
	return StateManager::SetFVF(FVF);
}
HRESULT ShaderStateManager::SetLight(DWORD Index,CONST D3DLIGHT9* pLight)
{
	return StateManager::SetLight(Index,pLight);
}
HRESULT ShaderStateManager::SetMaterial(CONST D3DMATERIAL9* pMaterial)
{
	return StateManager::SetMaterial(pMaterial);
}
HRESULT ShaderStateManager::SetNPatchMode(FLOAT nSegments)
{
	return StateManager::SetNPatchMode(nSegments);
}
HRESULT ShaderStateManager::SetRenderState(D3DRENDERSTATETYPE State, DWORD Value)
{
	return StateManager::SetRenderState(State,Value);
}

HRESULT ShaderStateManager::GetRenderState(D3DRENDERSTATETYPE State, DWORD *pValue)
{
	return StateManager::GetRenderState(State,pValue);
}


DWORD ShaderStateManager::GetRenderState(D3DRENDERSTATETYPE State)
{
	return StateManager::GetRenderState(State);
}

HRESULT ShaderStateManager::SetRenderTarget(DWORD RenderTargetIndex,LPDIRECT3DSURFACE9 pRenderTarget)
{
	return StateManager::SetRenderTarget(RenderTargetIndex,pRenderTarget);
}

HRESULT ShaderStateManager::SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
{
	return StateManager::SetSamplerState(Sampler,Type,Value);
}
HRESULT ShaderStateManager::GetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD *pValue)
{
	return StateManager::GetSamplerState(Sampler,Type,pValue);
}
HRESULT ShaderStateManager::SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
{
	return StateManager::SetTextureStageState(Stage,Type,Value);
}
HRESULT ShaderStateManager::GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD *pValue)
{
	return StateManager::GetTextureStageState(Stage,Type,pValue);
}
HRESULT ShaderStateManager::SetTexture(DWORD Stage, LPDIRECT3DBASETEXTURE9 pTexture)
{
	return StateManager::SetTexture(Stage,pTexture);
}

HRESULT ShaderStateManager::SetTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix)
{
	return StateManager::SetTransform(State,pMatrix);
}

HRESULT ShaderStateManager::SetPixelShader(LPDIRECT3DPIXELSHADER9 pShader)
{
	return StateManager::SetPixelShader(pShader);
}
HRESULT ShaderStateManager::SetPixelShaderConstantB(UINT StartRegister, CONST BOOL* pConstantData, UINT RegisterCount)
{
	return StateManager::SetPixelShaderConstantB(StartRegister,pConstantData,RegisterCount);
}
HRESULT ShaderStateManager::SetPixelShaderConstantF(UINT StartRegister, CONST FLOAT* pConstantData, UINT RegisterCount)
{
	return StateManager::SetPixelShaderConstantF(StartRegister,pConstantData,RegisterCount);
}
HRESULT ShaderStateManager::SetPixelShaderConstantI(UINT StartRegister, CONST INT* pConstantData, UINT RegisterCount)
{
	return StateManager::SetPixelShaderConstantI(StartRegister,pConstantData,RegisterCount);
}

HRESULT ShaderStateManager::SetVertexShader(LPDIRECT3DVERTEXSHADER9 pShader)
{
	return StateManager::SetVertexShader(pShader);
}
HRESULT ShaderStateManager::SetVertexShaderConstantB(UINT StartRegister,CONST BOOL* pConstantData,UINT RegisterCount)
{
	return StateManager::SetVertexShaderConstantB(StartRegister,pConstantData,RegisterCount);
}
HRESULT ShaderStateManager::SetVertexShaderConstantF(UINT StartRegister, CONST FLOAT* pConstantData, UINT RegisterCount)
{
	return StateManager::SetVertexShaderConstantF(StartRegister,pConstantData,RegisterCount);
}
HRESULT ShaderStateManager::SetVertexShaderConstantI(UINT StartRegister, CONST INT* pConstantData, UINT RegisterCount)
{
	return StateManager::SetVertexShaderConstantI(StartRegister,pConstantData,RegisterCount);
}
