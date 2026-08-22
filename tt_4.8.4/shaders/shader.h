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
#ifndef TT_INCLUDE_SHADER_H
#define TT_INCLUDE_SHADER_H
#include "effect.h"
#include "engine_3dre.h"
//typedef Vector4 float2;
//typedef Vector4 float3;
//typedef Vector4 float4;
//typedef Matrix4 float4x4;

enum ShaderParameterType
{
	ShaderParameterType_Custom	= 0,
	ShaderParameterType_Float	= 1,
	ShaderParameterType_Int		= 2,
	ShaderParameterType_UInt	= 3,
	ShaderParameterType_Vector2 = 4,
	ShaderParameterType_Vector3 = 5,
	ShaderParameterType_Vector4 = 6,
	ShaderParameterType_String	= 7,
	ShaderParameterType_Texture = 8,
	ShaderParameterType_Bool	= 9,
	ShaderParameterType_Macro	= 10,
};

class ShaderParameter;
class ShaderParameterBindAddress;
class ShaderParameterTextureBindAddress;
class ChunkLoadClass;
class ChunkSaveClass;
class INIClass;
class ShaderParameters
{
protected:
	SList<ShaderParameter> parameters;
public:
	static const int ChunkType;
	ShaderParameters();
	~ShaderParameters();
	ShaderParameter* GetParameter(const char* name);
	void AddParameter(ShaderParameter* param);
	bool Load(ChunkLoadClass *cload);
	bool Save(ChunkSaveClass *csave);
	bool LoadFromINI(INIClass *ini, char *key);
	SList<ShaderParameter>& GetParameters()
	{
		return parameters;
	};
};	

struct TextureShaderParameterData
{
	char texturename[128];
	WW3DFormat format;
};

class ShaderParameter: public RefCountClass
{
friend class ShaderParameterBindAddress;
friend class ShaderParameterTextureBindAddress;
protected:
	char* name;
	ShaderParameterType type;
	void* data;
	uint size;
public:
	ShaderParameter();
	~ShaderParameter();

	TT_INLINE void SetName(const char* _name)
	{
		if (name) delete[] name;
		name = newstr(_name);
	}

	TT_INLINE const char* GetName() const
	{
		return this->name;
	};

	TT_INLINE void SetType(ShaderParameterType type)
	{
		this->type = type;
	};

	TT_INLINE ShaderParameterType GetType() const
	{
		return this->type;
	};

	TT_INLINE size_t GetSize() const
	{
		return this->size;
	};

	template <typename T> TT_INLINE bool GetData(T* data) const
	{
		if (!this->data) return false;
		*data = *((T*)this->data);
		return true;
	};

	template <> TT_INLINE bool GetData<char *>(char** data) const // Added because pointer types are evil >_>
	{
		if (!this->data) return false;
		*data = (char*)this->data;
		return true;
	};

	template <typename T> TT_INLINE void SetData(T* data, uint size)
	{
		this->data = new char[size];
		this->size = size;
		memcpy(this->data,data,size);
	};

	bool Load(ChunkLoadClass *cload);
	bool Save(ChunkSaveClass *csave);
	bool LoadFromINI(INIClass *ini, char *key, char *prefix);
};



#ifdef _EFFECT_H_
class ShaderParameterBindAddress: public EffectSystem::BindAddress
{
private:
	ShaderParameter* param;
public:
	ShaderParameterBindAddress(ShaderParameter* param);
	~ShaderParameterBindAddress();
	const char* GetAddress();
	bool GetData(void** data,unsigned long *size);
};
class ShaderParameterTextureBindAddress: public EffectSystem::BindAddress
{
private:
	TextureClass* tex;
	ShaderParameter* param;
public:
	ShaderParameterTextureBindAddress(ShaderParameter* param);
	~ShaderParameterTextureBindAddress();
	const char* GetAddress();
	bool GetData(void** data,unsigned long *size);
};
#endif

enum ShaderStatus
{
	ShaderStatus_Usable = 0,
	ShaderStatus_Error,
	ShaderStatus_Created,
	ShaderStatus_Pending,
	ShaderStatus_Destroyed,
	ShaderStatus_DeviceLost,
};

class ShaderFactory;

class ProgrammableShader: public RefCountClass
{
protected:
	//unsigned int UID;				// Unique identifier for this shader instance. Should be defined only by the controller as to avoid collisions
	ShaderStatus Status;			// Current status of the shader
	ShaderFactory *Factory;			// Factory that created this shader
	//char *Name;					// Name of the shader 
	bool DrawingDisabled;			// Is drawing disabled for this shader?

	ShaderParameters Parameters;
public:
	ProgrammableShader();			// Performs busy-work such as registering the UID and Name variables, plus adding itself to the controller
	virtual ~ProgrammableShader();  // Takes care of cleaning up the registered variables and removing itself from the controller

	unsigned int GetUID();				// Get UID value
	ShaderStatus GetStatus();			// Get Status value
	ShaderFactory *GetFactory();		// Get the factory of this shader
	void SetFactory(ShaderFactory *factory);	// Set the factory of this shader
	const char *GetName();					// Get the name of the shader

	bool IsDrawingDisabled();
	void SetDrawingDisabled(bool status);

	virtual bool Load(ChunkLoadClass *cload);	// Called to load the ShaderClass
	virtual bool LoadFromINI(INIClass *ini, char *key);	// Called to load the ShaderClass from an ini TBR TESTING ONLY
	virtual bool Save(ChunkSaveClass *csave);	// Called to save the ShaderClass

	virtual bool Initialize() = 0;		// Initialize the ProgrammableShader derivitive
	virtual bool Destroy() = 0;			// Destroy anything created in Initialize
	virtual bool Validate();			// Check to see if this shader can be used at the current time
	virtual bool OnDeviceLost() = 0;	// Called whenever the device is lost
	virtual bool OnDeviceReset() = 0;	// Called after the device is reset
	virtual bool NeedsTangents();		// Does the shader need tangent calculation?
	virtual void Draw(unsigned int primitive_type, unsigned short start_index, unsigned short polygon_count, 
					  unsigned short min_vertex_index, unsigned short vertex_count) = 0; // Called to draw the object(s)
};

struct eqstr
{
  bool operator()(const char* s1, const char* s2) const
  {
    return (s1 == s2) || (s1 && s2 && strcmp(s1, s2) == 0);
  }
};

#define FOURCC_SDB1 (('S') | ('D' << 8) | ('B' << 16) | ('1' << 24))

struct ShaderDatabaseFileHeader
{
	unsigned int Identifier;
	unsigned int Flags;
};

class ShaderControllerObserver;

class ShaderControllerClass
{
private:
	ProgrammableShader *fallback;
	typedef google::dense_hash_map<const char *,ProgrammableShader *,stdext::hash_compare<const char *>,eqstr> HashMap;
	HashMap Shaders;
	SList<ShaderControllerObserver> Observers;
public:
	ShaderControllerClass();
	~ShaderControllerClass();

	void AddShader(ProgrammableShader *shader);
	void DeleteShader(ProgrammableShader *shader);
	ProgrammableShader* PeekShader(const char* name);
	ProgrammableShader* GetShader(const char* name);

	void AddObserver(ShaderControllerObserver* observer);
	void RemoveObserver(ShaderControllerObserver* observer);

	bool LoadDatabase(FileClass *file);
	bool LoadDatabaseFromINI(INIClass *ini); //TBR TESTING ONLY
	bool AppendDatabase(FileClass *file);
	bool SaveDatabase(FileClass *file);
	void UnloadDatabase();

	void SetFallbackShaderFactory(ShaderFactory *factory);

	void Initialize();
	void OnDeviceLost();
	void OnDeviceReset();
	void Render(unsigned int primitive_type, unsigned short start_index,
				unsigned short polygon_count, unsigned short min_vertex_index,		
				unsigned short vertex_count);
};


class ShaderFactory;
class ShaderRegistrar
{

protected:

	SList<ShaderFactory> Factories;
	ShaderRegistrar();

public:

	static ShaderRegistrar* Instance()
	{
		static ShaderRegistrar instance;
		return &instance;
	}

	void RegisterShaderFactory(ShaderFactory* factory);
	ShaderFactory* GetFactory(unsigned int chunkid);
	ShaderFactory* GetFactory(const char* name);
	void RemoveFactory(ShaderFactory* factory);

};



class ShaderFactory 
{
protected:
	unsigned int ChunkID;
	bool Editable;
	const char *EditorName;
public:
	ShaderFactory(unsigned int chunkid, const char* editorname = NULL);
	virtual ~ShaderFactory();
	unsigned int GetChunkID();
	bool IsEditable();
	const char *GetEditorName();
	virtual ProgrammableShader *LoadExisting(ChunkLoadClass *cload) = 0;
	virtual ProgrammableShader *LoadExistingFromINI(INIClass *ini, char *key) = 0; // TBR TESTING ONLY
	virtual ProgrammableShader *CreateNew() =  0;
};

template <class T> class ShaderRegistrant: public ShaderFactory
{
public:
	ShaderRegistrant(unsigned int chunkid,  const char *editorname = NULL): ShaderFactory(chunkid, editorname) {};
	ProgrammableShader *LoadExisting(ChunkLoadClass *cload)
	{
		ProgrammableShader *shader = new T;
		shader->SetFactory(this);
		bool res = shader->Load(cload);
		if (!res) delete shader;
		return !res ? NULL: shader;
	}
	ProgrammableShader *LoadExistingFromINI(INIClass *ini, char *key)  // TBR TESTING ONLY
	{
		ProgrammableShader *shader = new T;
		shader->SetFactory(this);
		bool res = shader->LoadFromINI(ini,key);
		if (!res) delete shader;
		return !res ? NULL: shader;
	}
	ProgrammableShader *CreateNew()
	{
		ProgrammableShader *shader = new T;
		shader->SetFactory(this);
		return shader;
	}
};

template <class T> class DefaultShaderRegistrant: public ShaderFactory
{
public:
	DefaultShaderRegistrant(): ShaderFactory(0) 
	{
		ShaderController.SetFallbackShaderFactory(this);
	}
	~DefaultShaderRegistrant()
	{
		ShaderController.SetFallbackShaderFactory(NULL);
	}
	ProgrammableShader *LoadExisting(ChunkLoadClass *cload)
	{
		return NULL;
	}
	ProgrammableShader *LoadExistingFromINI(INIClass *ini, char *key)
	{
		return NULL;
	}
	ProgrammableShader *CreateNew()
	{
		static ProgrammableShader *shader = new T;
		if (shader->GetFactory() != this) shader->SetFactory(this);
		return shader;
	}
};

extern ShaderControllerClass ShaderController;

#define CHUNK_SHADER 100
#endif
