/*
**	OpenW3D
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/ww3d2/shadereffect.cpp                       $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	TT 4.8.4 shaders/effect.cpp and shaders/shaderstatemanager.cpp, merged (D-004).           *
 *	TT's StateManager was a redundant-state cache of its own; here the effect state goes      *
 *	through the DX8Wrapper caches, which already are that cache for everything else.          *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "shadereffect.h"
#include "dx8wrapper.h"
#include "ffactory.h"
#include "wwfile.h"
#include "wwdebug.h"
#include "simplevec.h"
#include <string.h>

#ifdef W3D_HAVE_D3DX
#include <d3dx9effect.h>
#endif


/*
** ShaderBindingsClass
*/
void ShaderBindingsClass::Add_Binding(ShaderBindAddressClass * bind)
{
	if (bind == NULL) return;
	bind->Add_Ref();
	Binds.Add(bind);
}

void ShaderBindingsClass::Remove_Binding(ShaderBindAddressClass * bind)
{
	for (int i = 0; i < Binds.Count(); i++) {
		if (Binds[i] == bind) {
			Binds.Delete(i);
			bind->Release_Ref();
			return;
		}
	}
}

void ShaderBindingsClass::Remove_All(void)
{
	for (int i = 0; i < Binds.Count(); i++) {
		Binds[i]->Release_Ref();
	}
	Binds.Delete_All();
}

ShaderBindAddressClass * ShaderBindingsClass::Find(const char * address) const
{
	if ((address == NULL) || (address[0] == 0)) return NULL;
	for (int i = 0; i < Binds.Count(); i++) {
		if (_stricmp(Binds[i]->Get_Address(),address) == 0) return Binds[i];
	}
	return NULL;
}


static ShaderBindingsClass	_GlobalBindings;

ShaderBindingsClass & ShaderEffectSystemClass::Global_Bindings(void)
{
	return _GlobalBindings;
}


#ifdef W3D_HAVE_D3DX

typedef HRESULT (WINAPI * D3DXCreateEffectFunc)(LPDIRECT3DDEVICE9,LPCVOID,UINT,CONST D3DXMACRO *,LPD3DXINCLUDE,
																DWORD,LPD3DXEFFECTPOOL,LPD3DXEFFECT *,LPD3DXBUFFER *);

static HMODULE						_D3DXModule			= NULL;
static D3DXCreateEffectFunc	_D3DXCreateEffect	= NULL;


//
//	Reads a whole file through the file factory.  The caller frees the buffer with delete [].
//
static char * Read_Whole_File(const char * filename,unsigned int * size)
{
	*size = 0;
	if ((_TheFileFactory == NULL) || (filename == NULL) || (filename[0] == 0)) return NULL;
	file_auto_ptr file(_TheFileFactory,filename);
	if (!file->Is_Available()) return NULL;
	if (!file->Open()) return NULL;
	int length = file->Size();
	if (length <= 0) {
		file->Close();
		return NULL;
	}
	char * data = new char[length + 1];
	int read = file->Read(data,length);
	file->Close();
	if (read != length) {
		delete [] data;
		return NULL;
	}
	data[length] = 0;
	*size = (unsigned int)length;
	return data;
}


//
//	#include inside an .fx file resolves through the file factory, like every other asset.
//
class EffectIncludeClass : public ID3DXInclude
{
public:
	STDMETHOD(Open)(D3DXINCLUDE_TYPE,LPCSTR name,LPCVOID,LPCVOID * data,UINT * bytes) override
	{
		unsigned int size = 0;
		char * buffer = Read_Whole_File(name,&size);
		if (buffer == NULL) {
			WWDEBUG_SAY(("Shader effect include %s not found\n",name));
			return E_FAIL;
		}
		*data = buffer;
		*bytes = size;
		return S_OK;
	}
	STDMETHOD(Close)(LPCVOID data) override
	{
		delete [] (char *)data;
		return S_OK;
	}
};

static EffectIncludeClass	_EffectInclude;


//
//	Effect state through the DX8Wrapper caches.  Stages the wrapper does not cache
//	(MAX_TEXTURE_STAGES and up), shaders and constants go straight to the
//	device; DX8Wrapper::Invalidate_After_Effect_Draw makes the wrapper re-apply its own state
//	on the next draw.
//
class EffectStateManagerClass : public ID3DXEffectStateManager
{
public:
	STDMETHOD(QueryInterface)(REFIID iid,LPVOID * ppv) override
	{
		if ((iid == IID_IUnknown) || (iid == IID_ID3DXEffectStateManager)) {
			*ppv = static_cast<ID3DXEffectStateManager *>(this);
			return S_OK;
		}
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	STDMETHOD_(ULONG,AddRef)(void) override	{ return 1; }
	STDMETHOD_(ULONG,Release)(void) override	{ return 1; }

	STDMETHOD(SetTransform)(D3DTRANSFORMSTATETYPE state,CONST D3DMATRIX * matrix) override
	{
		return DX8Wrapper::_Get_D3D_Device8()->SetTransform(state,matrix);
	}
	STDMETHOD(SetMaterial)(CONST D3DMATERIAL9 * material) override
	{
		DX8Wrapper::Set_DX8_Material(material);
		return S_OK;
	}
	STDMETHOD(SetLight)(DWORD index,CONST D3DLIGHT9 * light) override
	{
		if (index < 4) {
			DX8Wrapper::Set_DX8_Light((int)index,const_cast<D3DLIGHT9 *>(light));
			return S_OK;
		}
		return DX8Wrapper::_Get_D3D_Device8()->SetLight(index,light);
	}
	STDMETHOD(LightEnable)(DWORD index,BOOL enable) override
	{
		return DX8Wrapper::_Get_D3D_Device8()->LightEnable(index,enable);
	}
	STDMETHOD(SetRenderState)(D3DRENDERSTATETYPE state,DWORD value) override
	{
		DX8Wrapper::Set_DX8_Render_State(state,value);
		return S_OK;
	}
	STDMETHOD(SetTexture)(DWORD stage,LPDIRECT3DBASETEXTURE9 texture) override
	{
		if (stage < MAX_TEXTURE_STAGES) {
			DX8Wrapper::Set_DX8_Texture(stage,texture);
			return S_OK;
		}
		return DX8Wrapper::_Get_D3D_Device8()->SetTexture(stage,texture);
	}
	STDMETHOD(SetTextureStageState)(DWORD stage,D3DTEXTURESTAGESTATETYPE type,DWORD value) override
	{
		if (stage < MAX_TEXTURE_STAGES) {
			DX8Wrapper::Set_DX8_Texture_Stage_State(stage,type,value);
			return S_OK;
		}
		return DX8Wrapper::_Get_D3D_Device8()->SetTextureStageState(stage,type,value);
	}
	STDMETHOD(SetSamplerState)(DWORD sampler,D3DSAMPLERSTATETYPE type,DWORD value) override
	{
		if (sampler < MAX_TEXTURE_STAGES) {
			DX8Wrapper::Set_DX8_Texture_Sampler_State(sampler,type,value);
			return S_OK;
		}
		return DX8Wrapper::_Get_D3D_Device8()->SetSamplerState(sampler,type,value);
	}
	STDMETHOD(SetNPatchMode)(FLOAT segments) override
	{
		return DX8Wrapper::_Get_D3D_Device8()->SetNPatchMode(segments);
	}
	STDMETHOD(SetFVF)(DWORD fvf) override
	{
		return DX8Wrapper::_Get_D3D_Device8()->SetFVF(fvf);
	}
	STDMETHOD(SetVertexShader)(LPDIRECT3DVERTEXSHADER9 shader) override
	{
		return DX8Wrapper::_Get_D3D_Device8()->SetVertexShader(shader);
	}
	STDMETHOD(SetVertexShaderConstantF)(UINT reg,CONST FLOAT * data,UINT count) override
	{
		return DX8Wrapper::_Get_D3D_Device8()->SetVertexShaderConstantF(reg,data,count);
	}
	STDMETHOD(SetVertexShaderConstantI)(UINT reg,CONST INT * data,UINT count) override
	{
		return DX8Wrapper::_Get_D3D_Device8()->SetVertexShaderConstantI(reg,data,count);
	}
	STDMETHOD(SetVertexShaderConstantB)(UINT reg,CONST BOOL * data,UINT count) override
	{
		return DX8Wrapper::_Get_D3D_Device8()->SetVertexShaderConstantB(reg,data,count);
	}
	STDMETHOD(SetPixelShader)(LPDIRECT3DPIXELSHADER9 shader) override
	{
		return DX8Wrapper::_Get_D3D_Device8()->SetPixelShader(shader);
	}
	STDMETHOD(SetPixelShaderConstantF)(UINT reg,CONST FLOAT * data,UINT count) override
	{
		return DX8Wrapper::_Get_D3D_Device8()->SetPixelShaderConstantF(reg,data,count);
	}
	STDMETHOD(SetPixelShaderConstantI)(UINT reg,CONST INT * data,UINT count) override
	{
		return DX8Wrapper::_Get_D3D_Device8()->SetPixelShaderConstantI(reg,data,count);
	}
	STDMETHOD(SetPixelShaderConstantB)(UINT reg,CONST BOOL * data,UINT count) override
	{
		return DX8Wrapper::_Get_D3D_Device8()->SetPixelShaderConstantB(reg,data,count);
	}
};

static EffectStateManagerClass	_EffectStateManager;


//
//	One effect parameter and the address it is bound to.
//
struct EffectVariableStruct
{
	D3DXHANDLE						Handle;
	bool								IsTexture;
	unsigned int					Bytes;
	ShaderBindAddressClass *	Bind;
};


class EffectImplClass;
static DynamicVectorClass<EffectImplClass *>	_Effects;


class EffectImplClass : public ShaderEffectClass
{
public:
	EffectImplClass(const char * filename,ShaderBindingsClass * local) :
		Name(filename),
		Local(local),
		Effect(NULL),
		Status(SHADER_EFFECT_CREATED)
	{
		_Effects.Add(this);
	}

	virtual ~EffectImplClass(void)
	{
		Destroy();
		for (int i = 0; i < _Effects.Count(); i++) {
			if (_Effects[i] == this) {
				_Effects.Delete(i);
				break;
			}
		}
	}

	virtual bool Initialize(void) override
	{
		Destroy();
		Status = SHADER_EFFECT_ERROR;
		if ((_D3DXCreateEffect == NULL) || (DX8Wrapper::_Get_D3D_Device8() == NULL)) return false;

		unsigned int size = 0;
		char * data = Read_Whole_File(Name,&size);
		if (data == NULL) {
			WWDEBUG_SAY(("Shader effect %s not found\n",(const char *)Name));
			return false;
		}

		LPD3DXBUFFER errors = NULL;
		HRESULT hr = _D3DXCreateEffect(DX8Wrapper::_Get_D3D_Device8(),data,size,NULL,&_EffectInclude,0,NULL,&Effect,&errors);
		delete [] data;
		if (FAILED(hr) || (Effect == NULL)) {
			WWDEBUG_SAY(("Shader effect %s failed to compile: %s\n",(const char *)Name,
				(errors != NULL) ? (const char *)errors->GetBufferPointer() : "no error text"));
			if (errors != NULL) errors->Release();
			Effect = NULL;
			return false;
		}
		if (errors != NULL) errors->Release();

		D3DXHANDLE technique = NULL;
		if (FAILED(Effect->FindNextValidTechnique(NULL,&technique)) || (technique == NULL)) {
			WWDEBUG_SAY(("Shader effect %s has no technique this device can run\n",(const char *)Name));
			Destroy();
			Status = SHADER_EFFECT_ERROR;
			return false;
		}
		Effect->SetTechnique(technique);
		Effect->SetStateManager(&_EffectStateManager);
		Build_Variables();
		Status = SHADER_EFFECT_USABLE;
		return true;
	}

	virtual ShaderEffectStatus Get_Status(void) const override { return Status; }

	virtual void On_Device_Lost(void) override
	{
		if (Effect != NULL) Effect->OnLostDevice();
		if (Status == SHADER_EFFECT_USABLE) Status = SHADER_EFFECT_DEVICE_LOST;
	}

	virtual void On_Device_Reset(void) override
	{
		if (Effect != NULL) Effect->OnResetDevice();
		if (Status == SHADER_EFFECT_DEVICE_LOST) Status = SHADER_EFFECT_USABLE;
	}

	virtual bool Begin(unsigned int * passes) override
	{
		*passes = 0;
		if ((Status != SHADER_EFFECT_USABLE) || (Effect == NULL)) return false;
		Apply_Variables();
		UINT count = 0;
		if (FAILED(Effect->Begin(&count,D3DXFX_DONOTSAVESTATE))) return false;
		*passes = count;
		return true;
	}

	virtual void Begin_Pass(unsigned int pass) override	{ Effect->BeginPass(pass); }
	virtual void Commit_Changes(void) override				{ Effect->CommitChanges(); }
	virtual void End_Pass(void) override						{ Effect->EndPass(); }
	virtual void End(void) override								{ Effect->End(); }

private:
	void Destroy(void)
	{
		Variables.Delete_All();
		if (Effect != NULL) {
			Effect->Release();
			Effect = NULL;
		}
	}

	//
	//	TT binding order: the parameter's semantic against the global bindings, then its
	//	SasBindAddress annotation against the global bindings, then its LocalBindAddress
	//	annotation against the owner's bindings.  Unbound parameters keep their .fx default.
	//
	const char * Get_String_Annotation(D3DXHANDLE parameter,const char * name)
	{
		D3DXHANDLE annotation = Effect->GetAnnotationByName(parameter,name);
		LPCSTR value = NULL;
		if ((annotation != NULL) && SUCCEEDED(Effect->GetString(annotation,&value))) return value;
		return NULL;
	}

	void Build_Variables(void)
	{
		D3DXEFFECT_DESC effect_desc;
		if (FAILED(Effect->GetDesc(&effect_desc))) return;

		for (UINT i = 0; i < effect_desc.Parameters; i++) {
			D3DXHANDLE handle = Effect->GetParameter(NULL,i);
			D3DXPARAMETER_DESC desc;
			if ((handle == NULL) || FAILED(Effect->GetParameterDesc(handle,&desc))) continue;

			ShaderBindAddressClass * bind = _GlobalBindings.Find(desc.Semantic);
			if (bind == NULL) bind = _GlobalBindings.Find(Get_String_Annotation(handle,"SasBindAddress"));
			if ((bind == NULL) && (Local != NULL)) bind = Local->Find(Get_String_Annotation(handle,"LocalBindAddress"));
			if (bind == NULL) continue;

			EffectVariableStruct variable;
			variable.Handle		= handle;
			variable.IsTexture	= (desc.Type == D3DXPT_TEXTURE) || (desc.Type == D3DXPT_TEXTURE1D) ||
										  (desc.Type == D3DXPT_TEXTURE2D) || (desc.Type == D3DXPT_TEXTURE3D) ||
										  (desc.Type == D3DXPT_TEXTURECUBE);
			variable.Bytes			= desc.Bytes;
			variable.Bind			= bind;
			Variables.Add(variable);
		}
	}

	void Apply_Variables(void)
	{
		for (int i = 0; i < Variables.Count(); i++) {
			EffectVariableStruct & variable = Variables[i];
			if (!variable.Bind->Is_Dirty()) continue;
			const void * data = NULL;
			unsigned int size = 0;
			variable.Bind->Get_Data(&data,&size);
			if (data == NULL) continue;
			if (variable.IsTexture) {
				if (size == sizeof(IDirect3DBaseTexture9 *)) {
					Effect->SetTexture(variable.Handle,*(IDirect3DBaseTexture9 * const *)data);
				}
			} else {
				Effect->SetValue(variable.Handle,data,(size < variable.Bytes) ? size : variable.Bytes);
			}
		}
	}

	StringClass					Name;
	ShaderBindingsClass *	Local;
	LPD3DXEFFECT				Effect;
	ShaderEffectStatus		Status;
	SimpleDynVecClass<EffectVariableStruct>	Variables;
};


void ShaderEffectSystemClass::Init(void)
{
	if (_D3DXModule != NULL) return;
	_D3DXModule = LoadLibraryA("d3dx9_43.dll");
	if (_D3DXModule == NULL) {
		WWDEBUG_SAY(("Shader effects disabled: d3dx9_43.dll not found\n"));
		return;
	}
	_D3DXCreateEffect = (D3DXCreateEffectFunc)GetProcAddress(_D3DXModule,"D3DXCreateEffect");
	if (_D3DXCreateEffect == NULL) {
		WWDEBUG_SAY(("Shader effects disabled: D3DXCreateEffect missing from d3dx9_43.dll\n"));
		FreeLibrary(_D3DXModule);
		_D3DXModule = NULL;
	}
}

void ShaderEffectSystemClass::Shutdown(void)
{
	WWASSERT(_Effects.Count() == 0);
	_GlobalBindings.Remove_All();
	if (_D3DXModule != NULL) {
		FreeLibrary(_D3DXModule);
		_D3DXModule = NULL;
	}
	_D3DXCreateEffect = NULL;
}

bool ShaderEffectSystemClass::Is_Available(void)
{
	return _D3DXCreateEffect != NULL;
}

bool ShaderEffectSystemClass::Create_Effect(const char * filename,ShaderBindingsClass * local,ShaderEffectClass ** effect)
{
	*effect = NULL;
	if (!Is_Available() || (filename == NULL) || (filename[0] == 0)) return false;
	*effect = new EffectImplClass(filename,local);
	return true;
}

void ShaderEffectSystemClass::On_Device_Lost(void)
{
	for (int i = 0; i < _Effects.Count(); i++) _Effects[i]->On_Device_Lost();
}

void ShaderEffectSystemClass::On_Device_Reset(void)
{
	for (int i = 0; i < _Effects.Count(); i++) _Effects[i]->On_Device_Reset();
}

#else	// W3D_HAVE_D3DX

void ShaderEffectSystemClass::Init(void)								{ WWDEBUG_SAY(("Shader effects disabled: built without D3DX\n")); }
void ShaderEffectSystemClass::Shutdown(void)							{ _GlobalBindings.Remove_All(); }
bool ShaderEffectSystemClass::Is_Available(void)					{ return false; }
bool ShaderEffectSystemClass::Create_Effect(const char *,ShaderBindingsClass *,ShaderEffectClass ** effect) { *effect = NULL; return false; }
void ShaderEffectSystemClass::On_Device_Lost(void)					{}
void ShaderEffectSystemClass::On_Device_Reset(void)				{}

#endif	// W3D_HAVE_D3DX
