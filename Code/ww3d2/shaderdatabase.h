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
 *                     $Archive:: /Commando/Code/ww3d2/shaderdatabase.h                       $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	The TT 4.8.4 shader database (shaders/shader.h, basicshader.h), merged into ww3d2 (D-004). *
 *                                                                                             *
 *	A database maps a texture name to a programmable shader.  When a mesh is drawn with that  *
 *	texture on stage 0, DX8Wrapper::Draw hands the draw to the shader instead of issuing it    *
 *	fixed-function.  A shader is a list of named, typed parameters; BasicShader (the only     *
 *	shader TT ships) takes an EffectFilename parameter naming an .fx file and exposes every   *
 *	other parameter to that effect through LocalBindAddress annotations.                      *
 *                                                                                             *
 *	Databases load from, in order:                                                             *
 *		shaders.shd		TT binary database ('SDB1' header then one chunk per shader)          *
 *		shaders.ini		TT text database ([Shaders] Count / 1..N, one section per shader)     *
 *	and per map from <map>.shd / <map>_shaders.ini, which add to the global set and are        *
 *	dropped again when the map unloads.  No database file means no shader and no change to    *
 *	rendering.  The layer is off below the PROGRAMMABLE tier and when D3DX is unavailable.     *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef SHADERDATABASE_H
#define SHADERDATABASE_H

#include "always.h"
#include "refcount.h"
#include "vector.h"
#include "wwstring.h"
#include "vector3.h"
#include "ww3dformat.h"
#include "shadereffect.h"

class ChunkLoadClass;
class ChunkSaveClass;
class INIClass;
class TextureClass;

//
//	TT numbering; it is the on-disk type field of a .shd parameter chunk.
//
enum ShaderParameterType
{
	SHADER_PARAM_CUSTOM = 0,
	SHADER_PARAM_FLOAT,
	SHADER_PARAM_INT,
	SHADER_PARAM_UINT,
	SHADER_PARAM_VECTOR2,
	SHADER_PARAM_VECTOR3,
	SHADER_PARAM_VECTOR4,
	SHADER_PARAM_STRING,
	SHADER_PARAM_TEXTURE,
	SHADER_PARAM_BOOL,
	SHADER_PARAM_MACRO,
};

//
//	On-disk texture parameter data (TT TextureShaderParameterData).
//
struct ShaderTextureParameterData
{
	char				TextureName[128];
	WW3DFormat		Format;
};

class ShaderParameterClass
{
public:
	ShaderParameterClass(void);
	~ShaderParameterClass(void);

	const char *			Get_Name(void) const					{ return Name; }
	ShaderParameterType	Get_Type(void) const					{ return Type; }
	const void *			Peek_Data(void) const				{ return Data; }
	unsigned int			Get_Size(void) const					{ return Size; }
	const char *			Get_String(void) const;			// STRING / MACRO, else NULL

	void						Set(const char * name,ShaderParameterType type,const void * data,unsigned int size);

	bool						Load(ChunkLoadClass & cload);
	bool						Save(ChunkSaveClass & csave) const;
	bool						Load_From_INI(INIClass & ini,const char * section,const char * name);

	//	Texture parameters load their texture at first use; the bound value is the
	//	IDirect3DBaseTexture9 pointer.
	TextureClass *			Peek_Texture(void);

private:
	StringClass				Name;
	ShaderParameterType	Type;
	unsigned char *		Data;
	unsigned int			Size;
	TextureClass *			Texture;
};

class ShaderParameterListClass
{
public:
	~ShaderParameterListClass(void)		{ Delete_All(); }

	int							Count(void) const			{ return Parameters.Count(); }
	ShaderParameterClass *	Get(int index) const		{ return Parameters[index]; }
	ShaderParameterClass *	Find(const char * name) const;
	void							Add(ShaderParameterClass * param)	{ Parameters.Add(param); }
	void							Delete_All(void);

	bool							Load(ChunkLoadClass & cload);
	bool							Save(ChunkSaveClass & csave) const;
	bool							Load_From_INI(INIClass & ini,const char * section);

private:
	DynamicVectorClass<ShaderParameterClass *>	Parameters;
};

enum ShaderStatus
{
	SHADER_STATUS_USABLE = 0,
	SHADER_STATUS_ERROR,
	SHADER_STATUS_CREATED,
	SHADER_STATUS_PENDING,
	SHADER_STATUS_DESTROYED,
	SHADER_STATUS_DEVICE_LOST,
};

//
//	The indexed draw DX8Wrapper::Draw was about to issue.
//
struct ShaderDrawStruct
{
	unsigned int		PrimitiveType;		// D3DPRIMITIVETYPE
	int					BaseVertexIndex;
	unsigned int		MinVertexIndex;
	unsigned int		VertexCount;
	unsigned int		StartIndex;
	unsigned int		PolygonCount;
};

class ShaderFactoryClass;

class ProgrammableShaderClass : public RefCountClass
{
public:
	ProgrammableShaderClass(ShaderFactoryClass * factory);

	const char *				Get_Name(void) const;			// the "Name" parameter, or NULL
	ShaderStatus				Get_Status(void) const		{ return Status; }
	ShaderFactoryClass *		Get_Factory(void) const		{ return Factory; }
	ShaderParameterListClass &	Get_Parameters(void)		{ return Parameters; }

	//	Used by render targets (reflection, underwater) to keep a shader out of their pass.
	bool							Is_Drawing_Disabled(void) const		{ return DrawingDisabled; }
	void							Set_Drawing_Disabled(bool onoff)		{ DrawingDisabled = onoff; }

	virtual bool				Load(ChunkLoadClass & cload)							{ return Parameters.Load(cload); }
	virtual bool				Load_From_INI(INIClass & ini,const char * section)	{ return Parameters.Load_From_INI(ini,section); }
	virtual bool				Save(ChunkSaveClass & csave) const					{ return Parameters.Save(csave); }

	virtual bool				Initialize(void);
	virtual void				Destroy(void)				{ Status = SHADER_STATUS_DESTROYED; }
	virtual bool				Validate(void)				{ return Status == SHADER_STATUS_USABLE; }
	virtual void				On_Device_Lost(void)		{}
	virtual void				On_Device_Reset(void)	{}
	virtual bool				Needs_Tangents(void) const	{ return false; }

	//	Draws the indexed primitive with the vertex and index buffers, transforms and
	//	material DX8Wrapper has already applied.
	virtual void				Draw(const ShaderDrawStruct & draw) = 0;

protected:
	virtual ~ProgrammableShaderClass(void)	{}

	ShaderFactoryClass *		Factory;
	ShaderStatus				Status;
	bool							DrawingDisabled;
	ShaderParameterListClass	Parameters;
};

class ShaderFactoryClass
{
public:
	ShaderFactoryClass(unsigned int chunk_id,const char * editor_name) : ChunkID(chunk_id), EditorName(editor_name) {}
	virtual ~ShaderFactoryClass(void) {}

	unsigned int							Get_Chunk_ID(void) const		{ return ChunkID; }
	const char *							Get_Editor_Name(void) const	{ return EditorName; }
	virtual ProgrammableShaderClass *	Create(void) = 0;

private:
	unsigned int	ChunkID;
	const char *	EditorName;
};

//
//	TT BasicShader: one .fx effect named by the EffectFilename parameter.
//
class BasicShaderClass : public ProgrammableShaderClass
{
public:
	enum { CHUNK_ID = 34543 };

	BasicShaderClass(ShaderFactoryClass * factory);

	virtual bool				Initialize(void) override;
	virtual void				Destroy(void) override;
	virtual void				On_Device_Lost(void) override;
	virtual void				On_Device_Reset(void) override;
	virtual bool				Needs_Tangents(void) const override	{ return true; }
	virtual void				Draw(const ShaderDrawStruct & draw) override;

protected:
	virtual ~BasicShaderClass(void);

	ShaderEffectClass *		Effect;
	ShaderBindingsClass		LocalBindings;
};


class ShaderControllerClass
{
public:
	//	Called by ShaderManagerClass::Init/Shutdown with the device up.  Init loads the
	//	global database when the PROGRAMMABLE tier is active and D3DX is present.
	static void							Init(bool programmable);
	static void							Shutdown(void);
	static bool							Is_Active(void)		{ return Active; }

	static void							Register_Factory(ShaderFactoryClass * factory);
	static ShaderFactoryClass *	Find_Factory(unsigned int chunk_id);
	static ShaderFactoryClass *	Find_Factory(const char * editor_name);

	//	Database files.  Append_* adds to the current set; returns false when the file is
	//	absent or malformed (nothing is added from a malformed binary file).
	static bool							Append_Database(const char * filename,bool map_shaders);
	static bool							Append_Database_From_INI(const char * filename,bool map_shaders);
	static bool							Save_Database(const char * filename);
	static void							Unload_Database(void);

	//	TT MapLoaded / MapUnloaded: <map>.shd or <map>_shaders.ini, dropped at unload.
	static void							Map_Loaded(const char * map_name);
	static void							Map_Unloaded(void);

	static int							Get_Shader_Count(void);
	static ProgrammableShaderClass *	Peek_Shader(int index);
	static ProgrammableShaderClass *	Peek_Shader(const char * name);

	static void							On_Device_Lost(void);
	static void							On_Device_Reset(void);

	//	DX8Wrapper::Draw calls this for every DX8-buffer draw.  Returns true when a shader drew
	//	it (or deliberately drew nothing); false means draw fixed-function as before.
	static bool							Draw(const ShaderDrawStruct & draw);

	//	Owned by the scene, not the renderer; PhysicsSceneClass publishes it each frame.
	static void							Set_Sun_Light_Direction(const Vector3 & dir)	{ SunLightDirection = dir; }
	static const Vector3 &			Get_Sun_Light_Direction(void)						{ return SunLightDirection; }

private:
	static bool							Add_Shader(ProgrammableShaderClass * shader,bool map_shader);
	static void							Register_W3D_Bindings(void);

	static bool							Active;
	static Vector3						SunLightDirection;
};

#endif
