/*	Renegade Scripts.dll
	Base Class for shaders
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

#include "shader.h"
#include "WW3D.h"
#include "engine_diagnostics.h"
using namespace stdext;
using namespace google;

ShaderControllerClass ShaderController;

DebugOutputClass ShaderDebug("_shaders.txt");

#pragma warning(disable: 6031) // warning C6031: Return value ignored: 'sscanf'

const int ShaderParameters::ChunkType = 0x0000D996;

ShaderParameters::ShaderParameters(): parameters()
{

};

ShaderParameters::~ShaderParameters()
{
	for (ShaderParameter* param = parameters.Remove_Head(); param; param = parameters.Remove_Head())
	{
		// destructive loop
		param->Release();
	};
};

ShaderParameter* ShaderParameters::GetParameter(const char* name) 
{ 
	for (SLNode<ShaderParameter>* iter = parameters.Head(); iter ; iter = iter->Next())
	{
		ShaderParameter* param = iter->Data();
		if (_stricmp(param->GetName(),name) == 0)
		{
			return param;
		}
	}
	return NULL; 
};

void ShaderParameters::AddParameter(ShaderParameter* param)
{
	parameters.Add_Tail(param);
};

bool ShaderParameters::Load(ChunkLoadClass *cload) 
{
	for (;cload->Open_Chunk();cload->Close_Chunk())
	{
		switch(cload->Cur_Chunk_ID())
		{
		case ShaderParameters::ChunkType + 1:
			{
				ShaderParameter* param = new ShaderParameter();
				if(param->Load(cload))
				{
					parameters.Add_Tail(param);     
				}
				else
				{
					delete param;
					// FIXME
					// needs to raise error -SH
				}
			}
			break;
		DEFAULT_UNREACHABLE;
		}
	}
	return false;
};

bool ShaderParameters::Save(ChunkSaveClass *csave) 
{
	csave->Begin_Chunk(ShaderParameters::ChunkType);

		for (SLNode<ShaderParameter>* iter = parameters.Head(); iter != NULL; iter = iter->Next())
		{
			ShaderParameter* param = iter->Data();
			param->Save(csave);
		}	

	csave->End_Chunk();
	return false;
};

bool ShaderParameters::LoadFromINI(INIClass *ini, char *key)  
{
	uint count = ini->Get_Int(key,"SPCount",0);
	char temp[256];
	char prefix[256];
	for (uint i = 0; i < count; ++i)
	{
		sprintf(temp,"SP%d",i);
		ini->Get_String(key,temp,"MissingName",prefix,256);

		ShaderParameter* param = new ShaderParameter();
		if (param->LoadFromINI(ini,key,prefix))
		{
			parameters.Add_Tail(param);   
		}
		else
		{
			REF_PTR_RELEASE(param);
			// FIXME FIXME FIXME
			// needs to raise error -SH
		}
	}
	return true;
};

ShaderParameter::ShaderParameter(): name(NULL), type(ShaderParameterType_Custom), data(NULL), size(0)
{

};

ShaderParameter::~ShaderParameter()
{
	if (name){ delete[] name; name = 0;}
	if (data){ delete[] data; data = 0;}
};

bool ShaderParameter::Load(ChunkLoadClass *cload)
{
	for (;cload->Open_Micro_Chunk();cload->Close_Micro_Chunk())
	{
		switch(cload->Cur_Micro_Chunk_ID())
		{
		case 1:
			{
				int len = cload->Cur_Micro_Chunk_Length();
				name = new char[len];
				cload->Read(name,len);
			}
			break;
		case 2:
			cload->Read(&type,sizeof(ShaderParameterType));
			break;
		case 3:
			{
				cload->Read(&size,sizeof(uint));
				data = new char[size];
				cload->Read(data,size);
			}
			break;
		DEFAULT_UNREACHABLE;
		}
	}
	return true;
};

bool ShaderParameter::Save(ChunkSaveClass *csave)
{
	csave->Begin_Chunk(ShaderParameters::ChunkType + 1);
		csave->Begin_Micro_Chunk(1);
		{
			uint len = strlen(name) + 1;
			csave->Write(name,len);
		}
		csave->End_Micro_Chunk();
		csave->Begin_Micro_Chunk(2);
		csave->Write(&type,sizeof(ShaderParameterType));
		csave->End_Micro_Chunk();
		csave->Begin_Micro_Chunk(3);
		csave->Write(&size,sizeof(uint));
		csave->Write(data,size);
		csave->End_Micro_Chunk();
	csave->End_Chunk();
	return true;
};

bool ShaderParameter::LoadFromINI(INIClass *ini, char *key, char *prefix)
{
	this->SetName(prefix);
	
	char temp[512];
	sprintf(temp,"%sType",prefix);
	char type[512];
	ini->Get_String(key,temp,"InvalidType",type,512);


	sprintf(temp,"%sValue",prefix);
	if (strcmp(type,"InvalidType") == 0)
	{
		// TODO: Error
		return false;
	}
	else if (_stricmp(type,"float") == 0) 
	{
		this->SetType(ShaderParameterType_Float);

		float val = ini->Get_Float(key,temp,0);
		this->SetData(&val,sizeof(float));
	}
	else if (_stricmp(type,"int") == 0)
	{
		this->SetType(ShaderParameterType_Int);
		
		int val = ini->Get_Int(key,temp,0);
		this->SetData(&val,sizeof(int));
	}
	else if (_stricmp(type,"uint") == 0) // Hackish. Oh well
	{	
		this->SetType(ShaderParameterType_UInt);

		char temp2[256];
		ini->Get_String(key,temp,"0",temp2,256);
		uint val = 0;
		sscanf(temp2,"%u",&val);
		this->SetData(&val,sizeof(uint));
	}
	else if (_stricmp(type,"vector2") == 0) // Even more so
	{
		this->SetType(ShaderParameterType_Vector2);

		char temp2[256];
		ini->Get_String(key,temp,"0.0|0.0",temp2,256);
		Vector2 val = Vector2(0,0);
		sscanf(temp2,"%f|%f",&val.X,&val.Y);
		this->SetData(&val,sizeof(Vector2));
	}
	else if (_stricmp(type,"vector3") == 0) 
	{
		this->SetType(ShaderParameterType_Vector3);

		char temp2[256];
		ini->Get_String(key,temp,"0.0|0.0|0.0",temp2,256);
		Vector3 val = Vector3(0,0,0);
		sscanf(temp2,"%f|%f|%f",&val.X,&val.Y,&val.Z);
		this->SetData(&val,sizeof(Vector3));
	}
	else if (_stricmp(type,"vector4") == 0) 
	{
		this->SetType(ShaderParameterType_Vector4);

		char temp2[256];
		ini->Get_String(key,temp,"0.0|0.0|0.0|0.0",temp2,256);
		Vector4 val = Vector4(0,0,0,0);
		sscanf(temp2,"%f|%f|%f|%f",&val.X,&val.Y,&val.Z,&val.W);
		this->SetData(&val,sizeof(Vector4));
	}
	else if (_stricmp(type,"string") == 0) 
	{
		this->SetType(ShaderParameterType_String);

		char temp2[512];
		ini->Get_String(key,temp,"",temp2,512);

		this->SetData(&temp2,strlen(temp2) + 1);
	}
	else if (_stricmp(type,"texture") == 0) 
	{
		this->SetType(ShaderParameterType_Texture);


		TextureShaderParameterData temp2;
		ini->Get_String(key,temp,"",temp2.texturename,512);


		char format[64];
		sprintf(temp,"%sFormat",prefix);
		ini->Get_String(key,temp,"Unknown",format,64);
		if (_stricmp(format,"Unknown") == 0)
		{
			temp2.format = WW3D_FORMAT_UNKNOWN;
		}
		else if (_stricmp(format,"R8G8B8") == 0)
		{
			temp2.format = WW3D_FORMAT_R8G8B8;
		}
		else if (_stricmp(format,"A8R8G8B8") == 0)
		{
			temp2.format = WW3D_FORMAT_A8R8G8B8;
		}
		else if (_stricmp(format,"A4R4G4B4") == 0)
		{
			temp2.format = WW3D_FORMAT_A4R4G4B4;
		}
		else if (_stricmp(format,"DXT1") == 0)
		{
			temp2.format = WW3D_FORMAT_DXT1;
		}
		else if (_stricmp(format,"DXT2") == 0)
		{
			temp2.format = WW3D_FORMAT_DXT2;
		}
		else if (_stricmp(format,"DXT3") == 0)
		{
			temp2.format = WW3D_FORMAT_DXT3;
		}
		else if (_stricmp(format,"DXT4") == 0)
		{
			temp2.format = WW3D_FORMAT_DXT4;
		}
		else if (_stricmp(format,"DXT5") == 0)
		{
			temp2.format = WW3D_FORMAT_DXT5;
		}

		this->SetData(&temp2,sizeof(temp2));
	}
	else if (_stricmp(type,"bool") == 0) 
	{
		this->SetType(ShaderParameterType_Bool);

		int val = ini->Get_Bool(key,temp,false) ? 1 : 0;
		this->SetData(&val,sizeof(int));
	}
	else if (_stricmp(type,"macro") == 0)
	{
		this->SetType(ShaderParameterType_Macro);

		char temp2[512];
		ini->Get_String(key,temp,"",temp2,512);

		this->SetData(&temp2,strlen(temp2) + 1);
	}

	return true;
};


ShaderParameterBindAddress::ShaderParameterBindAddress(ShaderParameter* param)
{
	this->param = param;
	param->Add_Ref();
};

ShaderParameterBindAddress::~ShaderParameterBindAddress()
{
	SafeRelease(param);
};

const char* ShaderParameterBindAddress::GetAddress()
{
	return param->GetName();
};

bool ShaderParameterBindAddress::GetData(void** data,unsigned long *size)
{
	assert(data != NULL);
	assert(size != NULL);
	assert(param->data != NULL);
	assert(param->size != NULL);
	*data = param->data;
	*size = param->size;
	return true;
};


ShaderParameterTextureBindAddress::ShaderParameterTextureBindAddress(ShaderParameter* param): tex(NULL)
{
	this->param = param;
	param->Add_Ref();
};

ShaderParameterTextureBindAddress::~ShaderParameterTextureBindAddress()
{
	SafeRelease(param);
	SafeRelease(tex);
};

const char* ShaderParameterTextureBindAddress::GetAddress()
{
	return param->GetName();
};

bool ShaderParameterTextureBindAddress::GetData(void** data,unsigned long *size)
{
	assert(data != NULL);
	assert(size != NULL);
	assert(param->data != NULL);
	if (!tex)
	{
		TextureShaderParameterData* _data = (TextureShaderParameterData*) param->data;
		tex = Load_Texture(_data->texturename,TextureClass::MIP_LEVELS_ALL,_data->format,true);
	}
	if (tex->Initialized != true)
	{
		tex->Init();
	}
	tex->LastAccessed = WW3D::SyncTime;

	*data = &tex->D3DTexture;
	*size = sizeof(IDirect3DTexture9*);
	return true;
};

ShaderRegistrar::ShaderRegistrar(): Factories()
{
	// nothing
};


void ShaderRegistrar::RegisterShaderFactory(ShaderFactory *factory)
{
	Factories.Add_Tail(factory);
}

ShaderFactory *ShaderRegistrar::GetFactory(unsigned int chunkid)
{
	for (SLNode<ShaderFactory>* iter = Factories.Head(); iter; iter = iter->Next())
	{
		ShaderFactory* fac = iter->Data();
		if (fac->GetChunkID() == chunkid)
		{
			return fac;
		}
	}
	return NULL;

}
ShaderFactory *ShaderRegistrar::GetFactory(const char* name)
{
	for (SLNode<ShaderFactory>* iter = Factories.Head(); iter; iter = iter->Next())
	{
		ShaderFactory* fac = iter->Data();
		if (!fac->IsEditable()) continue; // Only "editable" shaders can be found this way
		if (_stricmp(fac->GetEditorName(), name) == 0) // This is the one we are looking for
		{
			return fac;
		};
	}
	return NULL;
}

void ShaderRegistrar::RemoveFactory(ShaderFactory* factory)
{
	Factories.Remove(factory);
}

ShaderFactory::ShaderFactory(unsigned int chunkid, const char *editorname)
{
	ChunkID = chunkid;
	Editable = editorname != NULL; 
	EditorName = editorname;
	ShaderRegistrar* inst = ShaderRegistrar::Instance();
	inst->RegisterShaderFactory(this);
}

ShaderFactory::~ShaderFactory()
{
	ShaderRegistrar* inst = ShaderRegistrar::Instance();
	inst->RemoveFactory(this);
}

unsigned int ShaderFactory::GetChunkID()
{
	return ChunkID;
}

bool ShaderFactory::IsEditable()
{
	return Editable;
}

const char * ShaderFactory::GetEditorName()
{
	return EditorName;
}

ProgrammableShader::ProgrammableShader(): Parameters()
{
	Status = ShaderStatus_Created;
	Factory = 0;
	DrawingDisabled = false;
}

ProgrammableShader::~ProgrammableShader()
{

}


unsigned int ProgrammableShader::GetUID()
{
	ShaderParameter *param = Parameters.GetParameter("UID");
	if (!param)
	{
		return 0;
	}
	uint uid = 0;
	param->GetData(&uid);
	return uid;
}

ShaderStatus ProgrammableShader::GetStatus()
{
	return Status;
}

ShaderFactory *ProgrammableShader::GetFactory()
{
	return Factory;	
}

void ProgrammableShader::SetFactory(ShaderFactory *factory)
{
	Factory = factory;
}

const char *ProgrammableShader::GetName()
{
	ShaderParameter *param = Parameters.GetParameter("Name");
	if (!param)
	{
		return NULL;
	}
	char* name = NULL;
	param->GetData(&name);
	return name;
}

bool ProgrammableShader::IsDrawingDisabled()
{
	return DrawingDisabled;
};

void ProgrammableShader::SetDrawingDisabled(bool status)
{
	DrawingDisabled = status;
};

bool ProgrammableShader::Load(ChunkLoadClass *cload)
{
	return Parameters.Load(cload);
}

bool ProgrammableShader::LoadFromINI(INIClass *ini, char *key)
{
	return Parameters.LoadFromINI(ini,key);
}

bool ProgrammableShader::Save(ChunkSaveClass *csave)
{
	return Parameters.Save(csave);
}

bool ProgrammableShader::Initialize()
{
	if (Status == ShaderStatus_Created) 
	{ 
		Status = ShaderStatus_Pending;
		return true;
	}
	assert(Status == ShaderStatus_Created); // Should only be called once after shader is created, please report this
	return false;
}

bool ProgrammableShader::Destroy()
{
	assert(Status != ShaderStatus_Destroyed);
	return true;
}

bool ProgrammableShader::Validate()
{
	if (Status == ShaderStatus_Usable)
	{
		return true;
	} 
	else if (Status == ShaderStatus_Pending)
	{
		Status = ShaderStatus_Usable;
		return true;
	}
	else if (Status == ShaderStatus_Created)
	{
		this->Initialize();
		if (Status == ShaderStatus_Pending)
		{
			return true;
		}
	}
	return false;	
}

bool ProgrammableShader::OnDeviceLost()
{
	if (!Status)
	{
		Status = ShaderStatus_DeviceLost;
		return true;
	}
	assert(Status == ShaderStatus_Usable); // Should only be called when shader is in usable state.
	return false;
}

bool ProgrammableShader::OnDeviceReset()
{
	if (Status == ShaderStatus_DeviceLost)
	{
		Status = ShaderStatus_Usable;
		return true;
	}
	assert(Status == ShaderStatus_DeviceLost); // Should only be called after OnDeviceLost;
	return false;
}

bool ProgrammableShader::NeedsTangents()
{
	return false;
}

ShaderControllerClass::ShaderControllerClass(): Shaders(), Observers()
{
	Shaders.set_empty_key(NULL);
};

ShaderControllerClass::~ShaderControllerClass()
{
	SafeRelease(fallback);
	UnloadDatabase();
};

void ShaderControllerClass::AddShader(ProgrammableShader *shader)
{
	TT_ASSERT(shader != NULL);
	Shaders.insert(HashMap::value_type(shader->GetName(),shader));
};

void ShaderControllerClass::DeleteShader(ProgrammableShader *shader)
{
	if (!shader) return;
	Shaders.erase(shader->GetName());
};

ProgrammableShader* ShaderControllerClass::PeekShader(const char* name)
{
	TT_ASSERT(name != NULL);
	HashMap::iterator it = this->Shaders.find(name);
	return it.pos != it.end ? it->second: NULL;
};

ProgrammableShader* ShaderControllerClass::GetShader(const char* name)
{
	ProgrammableShader* shader = PeekShader(name);
	if (shader) shader->Add_Ref();
	return shader;
};

void ShaderControllerClass::AddObserver(ShaderControllerObserver* observer)
{
	TT_ASSERT(observer != NULL);
	Observers.Add_Tail(observer);
};

void ShaderControllerClass::RemoveObserver(ShaderControllerObserver* observer)
{
	TT_ASSERT(observer != NULL);
	Observers.Remove(observer);
};

bool ShaderControllerClass::LoadDatabase(FileClass *file)
{
	UnloadDatabase();
	return AppendDatabase(file);
}

bool ShaderControllerClass::LoadDatabaseFromINI(INIClass *ini)
{
	if (!ini) return false;

	int count = ini->Get_Int("Shaders","Count",0);
	for (int i = 1; i <= count; ++i)
	{
		char numBuffer[10]; // I'm sorry, we don't support more that 999,999,999 shaders loaded via INI ;)
		sprintf(numBuffer,"%d",i);
		char entry[256];
		ini->Get_String("Shaders", numBuffer, "dummy", entry, 256);
		if (strcmp(entry,"dummy") == 0) continue;

		char shadertype[256];
		ini->Get_String(entry,"Type","MissingType",shadertype,256);

		ShaderFactory* fac = ShaderRegistrar::Instance()->GetFactory(shadertype);
		if (!fac) // invalid type
		{
			char message[256];
			sprintf(message,"Hey! I have a bone to pick with you! You told me that I was a shader of type '%s'. YOU LIED TO ME! The controller told me there is no such type! >=|\n",shadertype);
			MessageBox(NULL,message,entry,MB_OK|MB_ICONEXCLAMATION);
			continue;
		};

		ProgrammableShader *shader = fac->LoadExistingFromINI(ini,entry);
		if (!shader)
		{
			char message[256];
			sprintf(message,"Somehow '%s' failed to load. Try checking _shaders.txt to find out why.",entry);
			MessageBox(NULL,message,entry,MB_OK|MB_ICONEXCLAMATION);
		}
		else
		{
			AddShader(shader);
		};
	}

	for (SLNode<ShaderControllerObserver>* iter = Observers.Head(); iter; iter = iter->Next())
	{
		ShaderControllerObserver* observer = iter->Data();
		observer->OnShaderDatabaseLoad();
	};

	return true;
}
bool ShaderControllerClass::AppendDatabase(FileClass *file)
{
	if (!(file && file->Open(1)))
	{
		return false;
	}

	ShaderDatabaseFileHeader header = {0,0};
	file->Read(&header,sizeof(ShaderDatabaseFileHeader));
	if (header.Identifier == FOURCC_SDB1)
	{
		ChunkLoadClass *cload = new ChunkLoadClass(file);
		while(cload->Open_Chunk())
		{
			ShaderFactory *factory = ShaderRegistrar::Instance()->GetFactory(cload->Cur_Chunk_ID());
			if (factory)
			{
				ProgrammableShader *shader = factory->LoadExisting(cload);
				AddShader(shader);	
			}
			else {
				ShaderDebug.Error(DEBUGLEVEL_WARN,"Unexpected shader encountered, chunkid is '0x%x'",cload->Cur_Chunk_ID());
			}
			cload->Close_Chunk();
		}	
		delete cload;
		file->Close();
		return true;
	}
	return false;
}

bool ShaderControllerClass::SaveDatabase(FileClass *file)
{
	if (Shaders.size() == 0)
	{
		return false;
	}
	if (!(file && file->Open(2)))
	{
		return false;
	}

	ChunkSaveClass *csave = new ChunkSaveClass(file);
	ShaderDatabaseFileHeader header = {0,0};
	header.Identifier = FOURCC_SDB1;
	header.Flags = 0;
	csave->Write(&header,sizeof(ShaderDatabaseFileHeader));

	for (HashMap::iterator it = Shaders.begin();  it != Shaders.end(); ++it)
	{
		ProgrammableShader *shader = it->second;
		if (shader)
		{
			unsigned int chunk = shader->GetFactory()->GetChunkID();
			if (chunk)
			{
				csave->Begin_Chunk(chunk);
				shader->Save(csave);
				csave->End_Chunk();
			}
		}
	}
	delete csave;
	file->Close();
	return S_OK;
}

void ShaderControllerClass::UnloadDatabase()
{
	Shaders.set_deleted_key("DELETED");

	for (HashMap::iterator it = Shaders.begin();  it != Shaders.end(); ++it)
	{
		ProgrammableShader *shader = it->second;
		if (shader)
		{
			bool res = shader->Destroy();
			ShaderDebug.Assert(res == true,"[%s] Shader failed to destroy itself\n",shader->GetName());
			SafeRelease(it->second);
		}
		Shaders.erase(it);
	}

	TT_ASSERT(Shaders.size() == 0);
	Shaders.clear_deleted_key();

	for (SLNode<ShaderControllerObserver>* iter = Observers.Head(); iter; iter = iter->Next())
	{
		ShaderControllerObserver* observer = iter->Data();
		observer->OnShaderDatabaseUnload();
	};
}

void ShaderControllerClass::SetFallbackShaderFactory(ShaderFactory *factory)
{
	SafeRelease(fallback);
	if (factory) fallback = factory->CreateNew();
}

void ShaderControllerClass::Initialize()
{
	for (HashMap::iterator it = Shaders.begin();  it != Shaders.end(); ++it)
	{
		ProgrammableShader *shader = it->second;
		if (shader)
		{
			bool res = shader->Initialize();
			ShaderDebug.Assert(res == true,"[%s] Shader failed to initialize\n",shader->GetName());
		}
	}
}

void ShaderControllerClass::OnDeviceLost()
{
	for (HashMap::iterator it = Shaders.begin();  it != Shaders.end(); ++it)
	{
		ProgrammableShader *shader = it->second;
		if (shader)
		{
			bool res = shader->OnDeviceLost();
			ShaderDebug.Assert(res == true,"[%s] Shader failed to handle OnDeviceLost\n",shader->GetName());
		}
	}
}

void ShaderControllerClass::OnDeviceReset()
{
	for (HashMap::iterator it = Shaders.begin();  it != Shaders.end(); ++it)
	{
		ProgrammableShader *shader = it->second;
		if (shader)
		{
			bool res = shader->OnDeviceReset();
			ShaderDebug.Assert(res == true,"[%s] Shader failed to handle OnDeviceReset\n",shader->GetName());
		}
	}
}

void ShaderControllerClass::Render(unsigned int primitive_type, unsigned short start_index, unsigned short polygon_count, unsigned short min_vertex_index, unsigned short vertex_count)
{
	if ((DX8Wrapper::render_state.material) && (DX8Wrapper::render_state.material->Mapper[0]))
	{
		ProgrammableShader *shader = DX8Wrapper::render_state.material->Mapper[0]->Peek_Shader();
		if (shader)
		{
			if (shader->Validate() == true)
			{
				shader->Draw(primitive_type,start_index,polygon_count,min_vertex_index,vertex_count);
				return;
			}
		}
	} 
	if ((DX8Wrapper::render_state.Textures[0]) && (DX8Wrapper::render_state.Textures[0]->Name))
	{
		ProgrammableShader *shader = PeekShader(DX8Wrapper::render_state.Textures[0]->Name);
		if (shader)
		{
			if (shader->Validate() == true)
			{
				shader->Draw(primitive_type,start_index,polygon_count,min_vertex_index,vertex_count);
				return;
			}
		}
		fallback->Draw(primitive_type,start_index,polygon_count,min_vertex_index,vertex_count);
	}
	else
	{

		fallback->Draw(primitive_type,start_index,polygon_count,min_vertex_index,vertex_count);
	}
}
