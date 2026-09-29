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
 *                     $Archive:: /Commando/Code/ww3d2/shaderdatabase.cpp                     $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	TT 4.8.4 shaders/shader.cpp, basicshader.cpp, w3dbindings.cpp, w3dbindings_transform.cpp  *
 *	and the database half of shaders.cpp, merged (D-004).                                      *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "shaderdatabase.h"
#include "dx8wrapper.h"
#include "assetmgr.h"
#include "texture.h"
#include "vertmaterial.h"
#include "ww3d.h"
#include "chunkio.h"
#include "ini.h"
#include "ffactory.h"
#include "wwfile.h"
#include "wwdebug.h"
#include "matrix4.h"
#include "vector4.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


//	TT chunk ids and file header; a .shd written by TT's editor loads here unchanged.
static const uint32	CHUNK_SHADER_PARAMETERS		= 0xD996;
static const uint32	CHUNK_SHADER_PARAMETER		= 0xD997;
static const uint32	MICROCHUNK_PARAMETER_NAME	= 1;
static const uint32	MICROCHUNK_PARAMETER_TYPE	= 2;
static const uint32	MICROCHUNK_PARAMETER_DATA	= 3;
static const uint32	SHADER_DATABASE_FOURCC		= ('S') | ('D' << 8) | ('B' << 16) | ('1' << 24);

struct ShaderDatabaseHeaderStruct
{
	uint32	Identifier;
	uint32	Flags;
};


/***********************************************************************************************
** ShaderParameterClass
***********************************************************************************************/

ShaderParameterClass::ShaderParameterClass(void) :
	Type(SHADER_PARAM_CUSTOM),
	Data(NULL),
	Size(0),
	Texture(NULL)
{
}

ShaderParameterClass::~ShaderParameterClass(void)
{
	delete [] Data;
	REF_PTR_RELEASE(Texture);
}

void ShaderParameterClass::Set(const char * name,ShaderParameterType type,const void * data,unsigned int size)
{
	Name = name;
	Type = type;
	delete [] Data;
	Data = NULL;
	Size = 0;
	REF_PTR_RELEASE(Texture);
	if ((data != NULL) && (size > 0)) {
		Data = new unsigned char[size];
		memcpy(Data,data,size);
		Size = size;
	}
}

const char * ShaderParameterClass::Get_String(void) const
{
	if ((Type != SHADER_PARAM_STRING) && (Type != SHADER_PARAM_MACRO)) return NULL;
	if ((Data == NULL) || (Size == 0) || (Data[Size - 1] != 0)) return NULL;
	return (const char *)Data;
}

TextureClass * ShaderParameterClass::Peek_Texture(void)
{
	if (Type != SHADER_PARAM_TEXTURE) return NULL;
	if ((Texture == NULL) && (Data != NULL) && (Size >= sizeof(ShaderTextureParameterData))) {
		const ShaderTextureParameterData * texdata = (const ShaderTextureParameterData *)Data;
		char name[sizeof(texdata->TextureName) + 1];
		memcpy(name,texdata->TextureName,sizeof(texdata->TextureName));
		name[sizeof(texdata->TextureName)] = 0;
		if (name[0] != 0) {
			Texture = WW3DAssetManager::Get_Instance()->Get_Texture(name,TextureClass::MIP_LEVELS_ALL,texdata->Format,true);
		}
	}
	if ((Texture != NULL) && !Texture->Is_Initialized()) Texture->Init();
	return Texture;
}

bool ShaderParameterClass::Load(ChunkLoadClass & cload)
{
	StringClass name;
	int type = SHADER_PARAM_CUSTOM;
	unsigned char * data = NULL;
	unsigned int size = 0;

	while (cload.Open_Micro_Chunk()) {
		switch (cload.Cur_Micro_Chunk_ID()) {
			case MICROCHUNK_PARAMETER_NAME:
			{
				unsigned int length = cload.Cur_Micro_Chunk_Length();
				char * buffer = name.Get_Buffer(length + 1);
				cload.Read(buffer,length);
				buffer[length] = 0;
				break;
			}
			case MICROCHUNK_PARAMETER_TYPE:
				cload.Read(&type,sizeof(type));
				break;
			case MICROCHUNK_PARAMETER_DATA:
			{
				cload.Read(&size,sizeof(size));
				unsigned int remaining = cload.Cur_Micro_Chunk_Length() - sizeof(size);
				if (size > remaining) size = remaining;
				delete [] data;
				data = (size > 0) ? new unsigned char[size] : NULL;
				if (data != NULL) cload.Read(data,size);
				break;
			}
		}
		cload.Close_Micro_Chunk();
	}

	bool ok = (name.Get_Length() > 0) && (type >= SHADER_PARAM_CUSTOM) && (type <= SHADER_PARAM_MACRO);
	if (ok) Set(name,(ShaderParameterType)type,data,size);
	delete [] data;
	return ok;
}

bool ShaderParameterClass::Save(ChunkSaveClass & csave) const
{
	csave.Begin_Micro_Chunk(MICROCHUNK_PARAMETER_NAME);
	csave.Write((const char *)Name,Name.Get_Length() + 1);
	csave.End_Micro_Chunk();

	int type = Type;
	csave.Begin_Micro_Chunk(MICROCHUNK_PARAMETER_TYPE);
	csave.Write(&type,sizeof(type));
	csave.End_Micro_Chunk();

	csave.Begin_Micro_Chunk(MICROCHUNK_PARAMETER_DATA);
	csave.Write(&Size,sizeof(Size));
	if (Size > 0) csave.Write(Data,Size);
	csave.End_Micro_Chunk();
	return true;
}

//
//	"a|b|c" into up to count floats; missing components are 0.
//
static void Parse_Floats(const char * text,float * out,int count)
{
	for (int i = 0; i < count; i++) out[i] = 0.0f;
	const char * p = text;
	for (int i = 0; (i < count) && (p != NULL) && (*p != 0); i++) {
		out[i] = (float)atof(p);
		p = strchr(p,'|');
		if (p != NULL) p++;
	}
}

static WW3DFormat Parse_Texture_Format(const char * text)
{
	static const struct { const char * Name; WW3DFormat Format; } formats[] = {
		{ "R8G8B8",		WW3D_FORMAT_R8G8B8 },
		{ "A8R8G8B8",	WW3D_FORMAT_A8R8G8B8 },
		{ "A4R4G4B4",	WW3D_FORMAT_A4R4G4B4 },
		{ "DXT1",		WW3D_FORMAT_DXT1 },
		{ "DXT2",		WW3D_FORMAT_DXT2 },
		{ "DXT3",		WW3D_FORMAT_DXT3 },
		{ "DXT4",		WW3D_FORMAT_DXT4 },
		{ "DXT5",		WW3D_FORMAT_DXT5 },
	};
	for (int i = 0; i < (int)(sizeof(formats) / sizeof(formats[0])); i++) {
		if (_stricmp(text,formats[i].Name) == 0) return formats[i].Format;
	}
	return WW3D_FORMAT_UNKNOWN;
}

bool ShaderParameterClass::Load_From_INI(INIClass & ini,const char * section,const char * name)
{
	StringClass key;
	char type_text[64];
	char value[512];

	key.Format("%sType",name);
	ini.Get_String(section,key,"",type_text,sizeof(type_text));
	key.Format("%sValue",name);
	ini.Get_String(section,key,"",value,sizeof(value));

	if (_stricmp(type_text,"float") == 0) {
		float f = (float)atof(value);
		Set(name,SHADER_PARAM_FLOAT,&f,sizeof(f));
	} else if (_stricmp(type_text,"int") == 0) {
		int i = atoi(value);
		Set(name,SHADER_PARAM_INT,&i,sizeof(i));
	} else if (_stricmp(type_text,"uint") == 0) {
		unsigned int u = (unsigned int)strtoul(value,NULL,0);
		Set(name,SHADER_PARAM_UINT,&u,sizeof(u));
	} else if (_stricmp(type_text,"vector2") == 0) {
		float v[2];
		Parse_Floats(value,v,2);
		Set(name,SHADER_PARAM_VECTOR2,v,sizeof(v));
	} else if (_stricmp(type_text,"vector3") == 0) {
		float v[3];
		Parse_Floats(value,v,3);
		Set(name,SHADER_PARAM_VECTOR3,v,sizeof(v));
	} else if (_stricmp(type_text,"vector4") == 0) {
		float v[4];
		Parse_Floats(value,v,4);
		Set(name,SHADER_PARAM_VECTOR4,v,sizeof(v));
	} else if (_stricmp(type_text,"string") == 0) {
		Set(name,SHADER_PARAM_STRING,value,(unsigned int)strlen(value) + 1);
	} else if (_stricmp(type_text,"macro") == 0) {
		Set(name,SHADER_PARAM_MACRO,value,(unsigned int)strlen(value) + 1);
	} else if (_stricmp(type_text,"bool") == 0) {
		key.Format("%sValue",name);
		BOOL b = ini.Get_Bool(section,key,false) ? TRUE : FALSE;		// HLSL bool is 4 bytes
		Set(name,SHADER_PARAM_BOOL,&b,sizeof(b));
	} else if (_stricmp(type_text,"texture") == 0) {
		ShaderTextureParameterData texdata;
		memset(&texdata,0,sizeof(texdata));
		strncpy(texdata.TextureName,value,sizeof(texdata.TextureName) - 1);
		char format[32];
		key.Format("%sFormat",name);
		ini.Get_String(section,key,"Unknown",format,sizeof(format));
		texdata.Format = Parse_Texture_Format(format);
		Set(name,SHADER_PARAM_TEXTURE,&texdata,sizeof(texdata));
	} else {
		WWDEBUG_SAY(("Shader %s parameter %s has unknown type '%s'\n",section,name,type_text));
		return false;
	}
	return true;
}


/***********************************************************************************************
** ShaderParameterListClass
***********************************************************************************************/

ShaderParameterClass * ShaderParameterListClass::Find(const char * name) const
{
	if (name == NULL) return NULL;
	for (int i = 0; i < Parameters.Count(); i++) {
		if (_stricmp(Parameters[i]->Get_Name(),name) == 0) return Parameters[i];
	}
	return NULL;
}

void ShaderParameterListClass::Delete_All(void)
{
	for (int i = 0; i < Parameters.Count(); i++) delete Parameters[i];
	Parameters.Delete_All();
}

//
//	Called inside the shader's chunk.  The parameters sit in one CHUNK_SHADER_PARAMETERS
//	chunk holding a CHUNK_SHADER_PARAMETER chunk each.
//
bool ShaderParameterListClass::Load(ChunkLoadClass & cload)
{
	bool ok = true;
	while (cload.Open_Chunk()) {
		if (cload.Cur_Chunk_ID() == CHUNK_SHADER_PARAMETERS) {
			while (cload.Open_Chunk()) {
				if (cload.Cur_Chunk_ID() == CHUNK_SHADER_PARAMETER) {
					ShaderParameterClass * param = new ShaderParameterClass;
					if (param->Load(cload)) {
						Add(param);
					} else {
						delete param;
						ok = false;
					}
				}
				cload.Close_Chunk();
			}
		}
		cload.Close_Chunk();
	}
	return ok;
}

bool ShaderParameterListClass::Save(ChunkSaveClass & csave) const
{
	csave.Begin_Chunk(CHUNK_SHADER_PARAMETERS);
	for (int i = 0; i < Parameters.Count(); i++) {
		csave.Begin_Chunk(CHUNK_SHADER_PARAMETER);
		Parameters[i]->Save(csave);
		csave.End_Chunk();
	}
	csave.End_Chunk();
	return true;
}

bool ShaderParameterListClass::Load_From_INI(INIClass & ini,const char * section)
{
	int count = ini.Get_Int(section,"SPCount",0);
	for (int i = 0; i < count; i++) {
		StringClass key;
		key.Format("SP%d",i);
		char name[128];
		ini.Get_String(section,key,"",name,sizeof(name));
		if (name[0] == 0) {
			WWDEBUG_SAY(("Shader %s: %s missing\n",section,(const char *)key));
			return false;
		}
		ShaderParameterClass * param = new ShaderParameterClass;
		if (!param->Load_From_INI(ini,section,name)) {
			delete param;
			return false;
		}
		Add(param);
	}
	return true;
}


/***********************************************************************************************
** ProgrammableShaderClass
***********************************************************************************************/

ProgrammableShaderClass::ProgrammableShaderClass(ShaderFactoryClass * factory) :
	Factory(factory),
	Status(SHADER_STATUS_CREATED),
	DrawingDisabled(false)
{
}

const char * ProgrammableShaderClass::Get_Name(void) const
{
	ShaderParameterClass * name = Parameters.Find("Name");
	return (name != NULL) ? name->Get_String() : NULL;
}

bool ProgrammableShaderClass::Initialize(void)
{
	Status = SHADER_STATUS_USABLE;
	return true;
}


/***********************************************************************************************
** BasicShaderClass
***********************************************************************************************/

//
//	A shader parameter exposed to its effect under the parameter's name (LocalBindAddress).
//
class ShaderParameterBindAddressClass : public ShaderBindAddressClass
{
public:
	ShaderParameterBindAddressClass(ShaderParameterClass * param) : Param(param) {}

	virtual const char * Get_Address(void) const override { return Param->Get_Name(); }

	virtual void Get_Data(const void ** data,unsigned int * size) override
	{
		if (Param->Get_Type() == SHADER_PARAM_TEXTURE) {
			TextureClass * texture = Param->Peek_Texture();
			D3DTexture = (texture != NULL) ? texture->Peek_DX8_Texture() : NULL;
			*data = &D3DTexture;
			*size = sizeof(D3DTexture);
		} else {
			*data = Param->Peek_Data();
			*size = Param->Get_Size();
		}
	}

private:
	ShaderParameterClass *		Param;
	IDirect3DBaseTexture9 *		D3DTexture;
};

BasicShaderClass::BasicShaderClass(ShaderFactoryClass * factory) :
	ProgrammableShaderClass(factory),
	Effect(NULL)
{
}

BasicShaderClass::~BasicShaderClass(void)
{
	Destroy();
}

bool BasicShaderClass::Initialize(void)
{
	Destroy();
	Status = SHADER_STATUS_ERROR;

	for (int i = 0; i < Parameters.Count(); i++) {
		ShaderParameterClass * param = Parameters.Get(i);
		switch (param->Get_Type()) {
			case SHADER_PARAM_FLOAT:
			case SHADER_PARAM_INT:
			case SHADER_PARAM_UINT:
			case SHADER_PARAM_VECTOR2:
			case SHADER_PARAM_VECTOR3:
			case SHADER_PARAM_VECTOR4:
			case SHADER_PARAM_BOOL:
			case SHADER_PARAM_TEXTURE:
			{
				ShaderParameterBindAddressClass * bind = new ShaderParameterBindAddressClass(param);
				LocalBindings.Add_Binding(bind);
				bind->Release_Ref();
				break;
			}
			default:
				break;
		}
	}

	ShaderParameterClass * filename = Parameters.Find("EffectFilename");
	const char * name = (filename != NULL) ? filename->Get_String() : NULL;
	if (name == NULL) {
		WWDEBUG_SAY(("BasicShader %s has no EffectFilename\n",(Get_Name() != NULL) ? Get_Name() : "(unnamed)"));
		return false;
	}
	if (!ShaderEffectSystemClass::Create_Effect(name,&LocalBindings,&Effect)) return false;
	if (!Effect->Initialize()) return false;
	Status = SHADER_STATUS_USABLE;
	return true;
}

void BasicShaderClass::Destroy(void)
{
	REF_PTR_RELEASE(Effect);
	LocalBindings.Remove_All();
	Status = SHADER_STATUS_DESTROYED;
}

void BasicShaderClass::On_Device_Lost(void)
{
	if (Effect != NULL) Effect->On_Device_Lost();
	if (Status == SHADER_STATUS_USABLE) Status = SHADER_STATUS_DEVICE_LOST;
}

void BasicShaderClass::On_Device_Reset(void)
{
	if (Effect != NULL) Effect->On_Device_Reset();
	if ((Status == SHADER_STATUS_DEVICE_LOST) && (Effect != NULL) && (Effect->Get_Status() == SHADER_EFFECT_USABLE)) {
		Status = SHADER_STATUS_USABLE;
	}
}

void BasicShaderClass::Draw(const ShaderDrawStruct & draw)
{
	if (DrawingDisabled) return;

	//	TT: the effect addresses both stages with their own texture coordinates, untransformed.
	DX8Wrapper::Set_DX8_Texture_Stage_State(0,D3DTSS_TEXCOORDINDEX,0);
	DX8Wrapper::Set_DX8_Texture_Stage_State(1,D3DTSS_TEXCOORDINDEX,1);
	DX8Wrapper::Set_DX8_Texture_Stage_State(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);
	DX8Wrapper::Set_DX8_Texture_Stage_State(1,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);

	unsigned int passes = 0;
	if (!Effect->Begin(&passes)) return;
	IDirect3DDevice9 * device = DX8Wrapper::_Get_D3D_Device8();
	for (unsigned int pass = 0; pass < passes; pass++) {
		Effect->Begin_Pass(pass);
		Effect->Commit_Changes();
		device->DrawIndexedPrimitive((D3DPRIMITIVETYPE)draw.PrimitiveType,draw.BaseVertexIndex,
			draw.MinVertexIndex,draw.VertexCount,draw.StartIndex,draw.PolygonCount);
		Effect->End_Pass();
	}
	Effect->End();
	DX8Wrapper::Invalidate_After_Effect_Draw();
}


/***********************************************************************************************
** W3D global bindings (TT w3dbindings.cpp, w3dbindings_transform.cpp)
**
** Values come from the state DX8Wrapper has just applied for this draw.  Transforms are
** uploaded in D3D (row-vector) form, the form render_state keeps, so an effect writes
** mul(position,WorldViewProjection) exactly as in TT's .fx files.
***********************************************************************************************/

typedef void (*W3DBindFunc)(const void ** data,unsigned int * size);

class W3DBindAddressClass : public ShaderBindAddressClass
{
public:
	W3DBindAddressClass(const char * address,W3DBindFunc func) : Address(address), Func(func) {}
	virtual const char *	Get_Address(void) const override											{ return Address; }
	virtual void			Get_Data(const void ** data,unsigned int * size) override		{ Func(data,size); }

private:
	const char *	Address;
	W3DBindFunc		Func;
};

static Matrix4 Get_Projection_D3D(void)
{
	Matrix4 projection;
	DX8Wrapper::Get_Transform(D3DTS_PROJECTION,projection);		// W3D (column-vector) form
	return projection.Transpose();
}

enum TransformVariantType { TRANSFORM_PLAIN, TRANSFORM_TRANSPOSE, TRANSFORM_INVERSE, TRANSFORM_INVERSE_TRANSPOSE };

static void Emit_Transform(const Matrix4 & m,TransformVariantType variant,const void ** data,unsigned int * size)
{
	static Matrix4 result;
	switch (variant) {
		case TRANSFORM_PLAIN:					result = m; break;
		case TRANSFORM_TRANSPOSE:				result = m.Transpose(); break;
		case TRANSFORM_INVERSE:					result = m.Inverse(); break;
		case TRANSFORM_INVERSE_TRANSPOSE:	result = m.Inverse().Transpose(); break;
	}
	*data = &result;
	*size = sizeof(result);
}

#define W3D_TRANSFORM_BINDINGS(name,expr)																				\
	static void Bind_##name(const void ** d,unsigned int * s)						{ Emit_Transform(expr,TRANSFORM_PLAIN,d,s); }				\
	static void Bind_##name##Transpose(const void ** d,unsigned int * s)			{ Emit_Transform(expr,TRANSFORM_TRANSPOSE,d,s); }			\
	static void Bind_##name##Inverse(const void ** d,unsigned int * s)			{ Emit_Transform(expr,TRANSFORM_INVERSE,d,s); }			\
	static void Bind_##name##InverseTranspose(const void ** d,unsigned int * s)	{ Emit_Transform(expr,TRANSFORM_INVERSE_TRANSPOSE,d,s); }

#define RS	DX8Wrapper::Peek_Render_State()

W3D_TRANSFORM_BINDINGS(World,					RS.world)
W3D_TRANSFORM_BINDINGS(View,					RS.view)
W3D_TRANSFORM_BINDINGS(Projection,			Get_Projection_D3D())
W3D_TRANSFORM_BINDINGS(WorldView,				RS.world * RS.view)
W3D_TRANSFORM_BINDINGS(ViewProjection,		RS.view * Get_Projection_D3D())
W3D_TRANSFORM_BINDINGS(WorldViewProjection,	RS.world * RS.view * Get_Projection_D3D())

//	TT took the mapper's texture matrix; OpenW3D mappers apply through the fixed-function
//	stage, which an effect bypasses, so the effect gets the identity.
static void Bind_TexCoord(const void ** data,unsigned int * size)
{
	static Matrix4 identity(true);
	*data = &identity;
	*size = sizeof(identity);
}

static float Render_State_Float(D3DRENDERSTATETYPE state)
{
	unsigned int bits = DX8Wrapper::Get_DX8_Render_State(state);
	float value;
	memcpy(&value,&bits,sizeof(value));
	return value;
}

static void Color_To_Floats(unsigned int color,float * out)
{
	out[0] = ((color >> 16) & 0xff) / 255.0f;
	out[1] = ((color >> 8) & 0xff) / 255.0f;
	out[2] = (color & 0xff) / 255.0f;
	out[3] = ((color >> 24) & 0xff) / 255.0f;
}

static int Fog_Mode(void)
{
	if (!DX8Wrapper::Get_DX8_Render_State(D3DRS_FOGENABLE)) return D3DFOG_NONE;
	unsigned int mode = DX8Wrapper::Get_DX8_Render_State(D3DRS_FOGTABLEMODE);
	if (mode == D3DFOG_NONE) mode = DX8Wrapper::Get_DX8_Render_State(D3DRS_FOGVERTEXMODE);
	return (mode <= D3DFOG_LINEAR) ? (int)mode : D3DFOG_NONE;
}

//	fxshared.fxi W3DFogStruct
struct W3DFogStruct { int Mode; float Start; float End; float Density; float Color[3]; };
static void Bind_Fog(const void ** data,unsigned int * size)
{
	static W3DFogStruct fog;
	float color[4];
	Color_To_Floats(DX8Wrapper::Get_DX8_Render_State(D3DRS_FOGCOLOR),color);
	fog.Mode		= Fog_Mode();
	fog.Start	= Render_State_Float(D3DRS_FOGSTART);
	fog.End		= Render_State_Float(D3DRS_FOGEND);
	fog.Density	= Render_State_Float(D3DRS_FOGDENSITY);
	memcpy(fog.Color,color,sizeof(fog.Color));
	*data = &fog;
	*size = sizeof(fog);
}

//	fxshared.fxi W3DFogStruct2
struct W3DFog2Struct { int Mode; float OneOverRange; float FogEndOverRange; float Density; float Color[3]; };
static void Bind_Fog2(const void ** data,unsigned int * size)
{
	static W3DFog2Struct fog;
	float color[4];
	Color_To_Floats(DX8Wrapper::Get_DX8_Render_State(D3DRS_FOGCOLOR),color);
	float start = Render_State_Float(D3DRS_FOGSTART);
	float end = Render_State_Float(D3DRS_FOGEND);
	float range = end - start;
	if (fabsf(range) < 0.0001f) range = 0.0001f;
	fog.Mode					= Fog_Mode();
	fog.OneOverRange		= 1.0f / range;
	fog.FogEndOverRange	= end / range;
	fog.Density				= Render_State_Float(D3DRS_FOGDENSITY);
	memcpy(fog.Color,color,sizeof(fog.Color));
	*data = &fog;
	*size = sizeof(fog);
}

static void Material_Color(bool emissive,const void ** data,unsigned int * size)
{
	static Vector4 color;
	VertexMaterialClass * material = RS.material;
	Vector3 rgb(1.0f,1.0f,1.0f);
	float opacity = 1.0f;
	if (material != NULL) {
		if (emissive) material->Get_Emissive(&rgb);
		else material->Get_Diffuse(&rgb);
		opacity = material->Get_Opacity();
	} else if (emissive) {
		rgb.Set(0.0f,0.0f,0.0f);
	}
	color.Set(rgb.X,rgb.Y,rgb.Z,opacity);
	*data = &color;
	*size = sizeof(color);
}
static void Bind_Emissive_Color(const void ** d,unsigned int * s)	{ Material_Color(true,d,s); }
static void Bind_Diffuse_Color(const void ** d,unsigned int * s)	{ Material_Color(false,d,s); }

//	fxshared.fxi SamplerSettings, stage 0 as applied.
struct SamplerSettingsStruct { int AddressU; int AddressV; int MagFilter; int MinFilter; int MipFilter; int MaxAnisotropy; };
static void Bind_Sampler0_Settings(const void ** data,unsigned int * size)
{
	static SamplerSettingsStruct settings;
	IDirect3DDevice9 * device = DX8Wrapper::_Get_D3D_Device8();
	DWORD value = 0;
	device->GetSamplerState(0,D3DSAMP_ADDRESSU,&value);		settings.AddressU = (int)value;
	device->GetSamplerState(0,D3DSAMP_ADDRESSV,&value);		settings.AddressV = (int)value;
	device->GetSamplerState(0,D3DSAMP_MAGFILTER,&value);		settings.MagFilter = (int)value;
	device->GetSamplerState(0,D3DSAMP_MINFILTER,&value);		settings.MinFilter = (int)value;
	device->GetSamplerState(0,D3DSAMP_MIPFILTER,&value);		settings.MipFilter = (int)value;
	device->GetSamplerState(0,D3DSAMP_MAXANISOTROPY,&value);	settings.MaxAnisotropy = (int)value;
	*data = &settings;
	*size = sizeof(settings);
}

//	fxshared.fxi SoALightDataStruct: the four lights' directions by component, then colours.
struct SoALightDataStruct { float LightX[4]; float LightY[4]; float LightZ[4]; float Color[4][3]; };
static void Bind_SoA_Light_Data(const void ** data,unsigned int * size)
{
	static SoALightDataStruct soa;
	for (int i = 0; i < 4; i++) {
		const D3DLIGHT9 & light = RS.Lights[i];
		bool on = RS.LightEnable[i];
		soa.LightX[i]		= on ? light.Direction.x : 0.0f;
		soa.LightY[i]		= on ? light.Direction.y : 0.0f;
		soa.LightZ[i]		= on ? light.Direction.z : 0.0f;
		soa.Color[i][0]	= on ? light.Diffuse.r : 0.0f;
		soa.Color[i][1]	= on ? light.Diffuse.g : 0.0f;
		soa.Color[i][2]	= on ? light.Diffuse.b : 0.0f;
	}
	*data = &soa;
	*size = sizeof(soa);
}

static void Bind_Light_Directions(const void ** data,unsigned int * size)
{
	static Vector4 dirs[4];
	for (int i = 0; i < 4; i++) {
		const D3DLIGHT9 & light = RS.Lights[i];
		dirs[i].Set(-light.Direction.x,-light.Direction.y,-light.Direction.z,0.0f);
	}
	*data = dirs;
	*size = sizeof(dirs);
}

static void Bind_Light_Colors(const void ** data,unsigned int * size)
{
	static Vector4 colors[4];
	for (int i = 0; i < 4; i++) {
		const D3DLIGHT9 & light = RS.Lights[i];
		colors[i].Set(light.Diffuse.r,light.Diffuse.g,light.Diffuse.b,light.Diffuse.a);
	}
	*data = colors;
	*size = sizeof(colors);
}

static void Bind_Light_Enables(const void ** data,unsigned int * size)
{
	static BOOL enables[4];
	for (int i = 0; i < 4; i++) enables[i] = RS.LightEnable[i] ? TRUE : FALSE;
	*data = enables;
	*size = sizeof(enables);
}

static void Bind_Ambient_Color(const void ** data,unsigned int * size)
{
	static float color[4];
	Color_To_Floats(DX8Wrapper::Get_DX8_Render_State(D3DRS_AMBIENT),color);
	*data = color;
	*size = sizeof(color);
}

static void Bind_Time(const void ** data,unsigned int * size)
{
	static float seconds;
	seconds = WW3D::Get_Sync_Time() / 1000.0f;
	*data = &seconds;
	*size = sizeof(seconds);
}

static void Bind_Sun_Light_Direction(const void ** data,unsigned int * size)
{
	static Vector3 dir;
	dir = ShaderControllerClass::Get_Sun_Light_Direction();
	*data = &dir;
	*size = sizeof(dir);
}

static void Texture_Stage(int stage,const void ** data,unsigned int * size)
{
	static IDirect3DBaseTexture9 * textures[MAX_TEXTURE_STAGES];
	TextureClass * texture = RS.Textures[stage];
	textures[stage] = (texture != NULL) ? texture->Peek_DX8_Texture() : NULL;
	*data = &textures[stage];
	*size = sizeof(textures[stage]);
}
static void Bind_Texture0(const void ** d,unsigned int * s)	{ Texture_Stage(0,d,s); }
static void Bind_Texture1(const void ** d,unsigned int * s)	{ Texture_Stage(1,d,s); }

//	fxshared.fxi TextureInfoStruct
struct TextureInfoStruct { float Width; float Height; float PixelWidth; float PixelHeight; };
static void Texture_Info(int stage,const void ** data,unsigned int * size)
{
	static TextureInfoStruct infos[MAX_TEXTURE_STAGES];
	TextureInfoStruct & info = infos[stage];
	info.Width = info.Height = info.PixelWidth = info.PixelHeight = 0.0f;
	TextureClass * texture = RS.Textures[stage];
	IDirect3DTexture9 * d3d = (texture != NULL) ? texture->Peek_DX8_Texture() : NULL;
	D3DSURFACE_DESC desc;
	if ((d3d != NULL) && SUCCEEDED(d3d->GetLevelDesc(0,&desc)) && (desc.Width > 0) && (desc.Height > 0)) {
		info.Width			= (float)desc.Width;
		info.Height			= (float)desc.Height;
		info.PixelWidth	= 1.0f / desc.Width;
		info.PixelHeight	= 1.0f / desc.Height;
	}
	*data = &info;
	*size = sizeof(info);
}
static void Bind_Texture0_Info(const void ** d,unsigned int * s)	{ Texture_Info(0,d,s); }
static void Bind_Texture1_Info(const void ** d,unsigned int * s)	{ Texture_Info(1,d,s); }

#undef RS

void ShaderControllerClass::Register_W3D_Bindings(void)
{
	static const struct { const char * Address; W3DBindFunc Func; } bindings[] = {
#define W3D_TRANSFORM_ENTRIES(name)																	\
		{ #name, Bind_##name }, { #name "Transpose", Bind_##name##Transpose },					\
		{ #name "Inverse", Bind_##name##Inverse }, { #name "InverseTranspose", Bind_##name##InverseTranspose },
		W3D_TRANSFORM_ENTRIES(World)
		W3D_TRANSFORM_ENTRIES(View)
		W3D_TRANSFORM_ENTRIES(Projection)
		W3D_TRANSFORM_ENTRIES(WorldView)
		W3D_TRANSFORM_ENTRIES(ViewProjection)
		W3D_TRANSFORM_ENTRIES(WorldViewProjection)
#undef W3D_TRANSFORM_ENTRIES
		{ "TexCoord0",					Bind_TexCoord },
		{ "TexCoord1",					Bind_TexCoord },
		{ "W3D.Fog",					Bind_Fog },
		{ "W3D.Fog2",					Bind_Fog2 },
		{ "W3D.EmissiveColor",		Bind_Emissive_Color },
		{ "W3D.DiffuseColor",		Bind_Diffuse_Color },
		{ "W3D.Sampler0Settings",	Bind_Sampler0_Settings },
		{ "W3D.SoALightData",		Bind_SoA_Light_Data },
		{ "W3D.LightDirections",	Bind_Light_Directions },
		{ "W3D.LightColors",			Bind_Light_Colors },
		{ "W3D.LightEnables",		Bind_Light_Enables },
		{ "AmbientColor",				Bind_Ambient_Color },
		{ "Time",						Bind_Time },
		{ "SunLightDirection",		Bind_Sun_Light_Direction },
		{ "Texture0",					Bind_Texture0 },
		{ "Texture1",					Bind_Texture1 },
		{ "Texture0.Info",			Bind_Texture0_Info },
		{ "Texture1.Info",			Bind_Texture1_Info },
	};

	ShaderBindingsClass & global = ShaderEffectSystemClass::Global_Bindings();
	for (int i = 0; i < (int)(sizeof(bindings) / sizeof(bindings[0])); i++) {
		W3DBindAddressClass * bind = new W3DBindAddressClass(bindings[i].Address,bindings[i].Func);
		global.Add_Binding(bind);
		bind->Release_Ref();
	}
}


/***********************************************************************************************
** ShaderControllerClass
***********************************************************************************************/

class BasicShaderFactoryClass : public ShaderFactoryClass
{
public:
	BasicShaderFactoryClass(void) : ShaderFactoryClass(BasicShaderClass::CHUNK_ID,"BasicShader") {}
	virtual ProgrammableShaderClass * Create(void) override { return new BasicShaderClass(this); }
};

static BasicShaderFactoryClass								_BasicShaderFactory;
static DynamicVectorClass<ShaderFactoryClass *>		_Factories;
static DynamicVectorClass<ProgrammableShaderClass *>	_Shaders;
static DynamicVectorClass<bool>							_ShaderIsMapShader;

bool		ShaderControllerClass::Active = false;
Vector3	ShaderControllerClass::SunLightDirection(0.0f,0.0f,-1.0f);

void ShaderControllerClass::Register_Factory(ShaderFactoryClass * factory)
{
	if ((factory != NULL) && (Find_Factory(factory->Get_Chunk_ID()) == NULL)) _Factories.Add(factory);
}

ShaderFactoryClass * ShaderControllerClass::Find_Factory(unsigned int chunk_id)
{
	for (int i = 0; i < _Factories.Count(); i++) {
		if (_Factories[i]->Get_Chunk_ID() == chunk_id) return _Factories[i];
	}
	return NULL;
}

ShaderFactoryClass * ShaderControllerClass::Find_Factory(const char * editor_name)
{
	if (editor_name == NULL) return NULL;
	for (int i = 0; i < _Factories.Count(); i++) {
		if (_stricmp(_Factories[i]->Get_Editor_Name(),editor_name) == 0) return _Factories[i];
	}
	return NULL;
}

void ShaderControllerClass::Init(bool programmable)
{
	Register_Factory(&_BasicShaderFactory);
	Active = false;
	if (!programmable) {
		WWDEBUG_SAY(("Programmable shaders off: fixed-function tier\n"));
		return;
	}
	ShaderEffectSystemClass::Init();
	if (!ShaderEffectSystemClass::Is_Available()) return;
	Register_W3D_Bindings();
	if (!Append_Database("shaders.shd",false)) {
		Append_Database_From_INI("shaders.ini",false);
	}
	Active = true;
	WWDEBUG_SAY(("Programmable shaders on: %d in the database\n",_Shaders.Count()));
}

void ShaderControllerClass::Shutdown(void)
{
	Unload_Database();
	Active = false;
	ShaderEffectSystemClass::Shutdown();
}

bool ShaderControllerClass::Add_Shader(ProgrammableShaderClass * shader,bool map_shader)
{
	const char * name = shader->Get_Name();
	if (name == NULL) {
		WWDEBUG_SAY(("Shader database entry without a Name parameter skipped\n"));
		return false;
	}

	//	A later database (the map's) replaces a global shader of the same name.
	for (int i = 0; i < _Shaders.Count(); i++) {
		if (_stricmp(_Shaders[i]->Get_Name(),name) == 0) {
			_Shaders[i]->Destroy();
			_Shaders[i]->Release_Ref();
			_Shaders.Delete(i);
			_ShaderIsMapShader.Delete(i);
			break;
		}
	}

	if (!shader->Initialize()) {
		WWDEBUG_SAY(("Shader %s failed to initialize; its textures draw fixed-function\n",name));
	}
	shader->Add_Ref();
	_Shaders.Add(shader);
	_ShaderIsMapShader.Add(map_shader);
	return true;
}

bool ShaderControllerClass::Append_Database(const char * filename,bool map_shaders)
{
	if (_TheFileFactory == NULL) return false;
	file_auto_ptr file(_TheFileFactory,filename);
	if (!file->Is_Available() || !file->Open()) return false;

	ShaderDatabaseHeaderStruct header;
	if ((file->Read(&header,sizeof(header)) != sizeof(header)) || (header.Identifier != SHADER_DATABASE_FOURCC)) {
		WWDEBUG_SAY(("Shader database %s: not an SDB1 file\n",filename));
		file->Close();
		return false;
	}

	//	Parse everything first; a malformed file adds nothing.
	DynamicVectorClass<ProgrammableShaderClass *> loaded;
	bool ok = true;
	ChunkLoadClass cload(file);
	while (cload.Open_Chunk()) {
		ShaderFactoryClass * factory = Find_Factory(cload.Cur_Chunk_ID());
		if (factory == NULL) {
			WWDEBUG_SAY(("Shader database %s: unknown shader chunk %u\n",filename,cload.Cur_Chunk_ID()));
			ok = false;
		} else {
			ProgrammableShaderClass * shader = factory->Create();
			if (shader->Load(cload)) {
				loaded.Add(shader);
			} else {
				shader->Release_Ref();
				ok = false;
			}
		}
		cload.Close_Chunk();
	}
	file->Close();

	for (int i = 0; i < loaded.Count(); i++) {
		if (ok) Add_Shader(loaded[i],map_shaders);
		loaded[i]->Release_Ref();
	}
	if (!ok) {
		WWDEBUG_SAY(("Shader database %s malformed; nothing loaded from it\n",filename));
	}
	return ok;
}

bool ShaderControllerClass::Append_Database_From_INI(const char * filename,bool map_shaders)
{
	if (_TheFileFactory == NULL) return false;
	file_auto_ptr file(_TheFileFactory,filename);
	if (!file->Is_Available()) return false;
	INIClass ini;
	ini.Load(*file);

	int count = ini.Get_Int("Shaders","Count",0);
	for (int i = 1; i <= count; i++) {
		char key[16];
		char section[128];
		sprintf(key,"%d",i);
		ini.Get_String("Shaders",key,"",section,sizeof(section));
		if (section[0] == 0) continue;

		char type[64];
		ini.Get_String(section,"Type","BasicShader",type,sizeof(type));
		ShaderFactoryClass * factory = Find_Factory(type);
		if (factory == NULL) {
			WWDEBUG_SAY(("Shader %s: unknown Type %s\n",section,type));
			continue;
		}
		ProgrammableShaderClass * shader = factory->Create();
		if (shader->Load_From_INI(ini,section)) {
			Add_Shader(shader,map_shaders);
		}
		shader->Release_Ref();
	}
	return count > 0;
}

bool ShaderControllerClass::Save_Database(const char * filename)
{
	if (_TheWritingFileFactory == NULL) return false;
	FileClass * file = _TheWritingFileFactory->Get_File(filename);
	if (file == NULL) return false;
	bool ok = false;
	if (file->Open(FileClass::WRITE)) {
		ShaderDatabaseHeaderStruct header;
		header.Identifier	= SHADER_DATABASE_FOURCC;
		header.Flags		= 0;
		ok = (file->Write(&header,sizeof(header)) == sizeof(header));
		ChunkSaveClass csave(file);
		for (int i = 0; ok && (i < _Shaders.Count()); i++) {
			if (_ShaderIsMapShader[i]) continue;
			csave.Begin_Chunk(_Shaders[i]->Get_Factory()->Get_Chunk_ID());
			ok = _Shaders[i]->Save(csave);
			csave.End_Chunk();
		}
		file->Close();
	}
	_TheWritingFileFactory->Return_File(file);
	return ok;
}

void ShaderControllerClass::Unload_Database(void)
{
	for (int i = 0; i < _Shaders.Count(); i++) {
		_Shaders[i]->Destroy();
		_Shaders[i]->Release_Ref();
	}
	_Shaders.Delete_All();
	_ShaderIsMapShader.Delete_All();
}

void ShaderControllerClass::Map_Loaded(const char * map_name)
{
	if (!Active || (map_name == NULL) || (map_name[0] == 0)) return;
	StringClass base(map_name);
	char * dot = strrchr(base.Peek_Buffer(),'.');
	if (dot != NULL) *dot = 0;

	StringClass filename;
	filename.Format("%s.shd",(const char *)base);
	if (!Append_Database(filename,true)) {
		filename.Format("%s_shaders.ini",(const char *)base);
		Append_Database_From_INI(filename,true);
	}
}

void ShaderControllerClass::Map_Unloaded(void)
{
	for (int i = _Shaders.Count() - 1; i >= 0; i--) {
		if (!_ShaderIsMapShader[i]) continue;
		_Shaders[i]->Destroy();
		_Shaders[i]->Release_Ref();
		_Shaders.Delete(i);
		_ShaderIsMapShader.Delete(i);
	}
}

int ShaderControllerClass::Get_Shader_Count(void)
{
	return _Shaders.Count();
}

ProgrammableShaderClass * ShaderControllerClass::Peek_Shader(int index)
{
	return ((index >= 0) && (index < _Shaders.Count())) ? _Shaders[index] : NULL;
}

ProgrammableShaderClass * ShaderControllerClass::Peek_Shader(const char * name)
{
	if ((name == NULL) || (name[0] == 0)) return NULL;
	for (int i = 0; i < _Shaders.Count(); i++) {
		if (_stricmp(_Shaders[i]->Get_Name(),name) == 0) return _Shaders[i];
	}
	return NULL;
}

void ShaderControllerClass::On_Device_Lost(void)
{
	for (int i = 0; i < _Shaders.Count(); i++) _Shaders[i]->On_Device_Lost();
	ShaderEffectSystemClass::On_Device_Lost();
}

void ShaderControllerClass::On_Device_Reset(void)
{
	ShaderEffectSystemClass::On_Device_Reset();
	for (int i = 0; i < _Shaders.Count(); i++) _Shaders[i]->On_Device_Reset();
}

bool ShaderControllerClass::Draw(const ShaderDrawStruct & draw)
{
	if (_Shaders.Count() == 0) return false;
	TextureClass * texture = DX8Wrapper::Peek_Render_State().Textures[0];
	if (texture == NULL) return false;
	ProgrammableShaderClass * shader = Peek_Shader(texture->Get_Texture_Name());
	if ((shader == NULL) || !shader->Validate()) return false;
	shader->Draw(draw);
	return true;
}
