/*	Renegade Scripts.dll
	Shaders.dll header file
	Copyright 2009 Jonathan Wilson, Mark Sararu

	This file is part of the Renegade scripts.dll
	The Renegade scripts.dll is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2, or (at your option) any later
	version. See the file COPYING for more details.
	In addition, an exemption is given to allow Run Time Dynamic Linking of this code with any closed source module that does not contain code covered by this licence.
	Only the source code to the module(s) containing the licenced code has to be released.
*/

#ifndef SHADERS_INCLUDE__SHADERS_H
#define SHADERS_INCLUDE__SHADERS_H

#include "engine_3dre.h"
#include "texture.h"
#include "scripts.h"

//Developer mode features
//TODO TODO TODO
//Implement :P
bool SHADERS_API QueryShaderDeveloperMode();

#if 0
bool SHADERS_API ShaderDeveloper_LoadDatabase(const char* file, bool reset);
bool SHADERS_API ShaderDeveloper_LoadDatabaseINI(const char* file, bool reset);
bool SHADERS_API ShaderDeveloper_UnloadDatabases();
char SHADERS_API *ShaderDeveloper_QueryShaderErrors();

bool SHADERS_API ShaderDeveloper_LoadCameraDatabase(const char* file, bool reset);
bool SHADERS_API ShaderDeveloper_LoadCameraDatabaseINI(const char* file, bool reset);
bool SHADERS_API ShaderDeveloper_UnloadCameraDatabases();
char SHADERS_API *ShaderDeveloper_QueryCameraErrors();
#endif




void SHADERS_API ForceDeviceReset(); // Call to force a device reset (usually called after changing presentation parameters)


typedef void (*updfunc)(void);
void __declspec(dllimport) MapLoaded();
void __declspec(dllimport) MapUnloaded();
void __declspec(dllimport) ScopeTrigger(bool enabled);
void __declspec(dllimport) ScopeChange(int scope);
void __declspec(dllimport) ShaderSet(const char *parameter,const char *value);
void __declspec(dllimport) ShaderSetObj(const char *parameter,const char *value,GameObject *obj);
void __declspec(dllimport) ShaderDetailChanged(int detail);
void __declspec(dllimport) SceneShaderDetailChanged(int detail);
class DX8RigidFVFCategoryContainer;
class DX8SkinFVFCategoryContainer;
class MeshClass;
class CameraClass;
class RenderInfoClass;
struct RenderStateStruct;
class SceneClass;
class TextureClass;
class Render2DClass;
class ChunkLoadClass;
void __declspec(dllimport) ResetGeometry(MeshModelClass *m,int polycount, int vertcount);

// Direct3D state related hooks
void __declspec(dllimport) SetWireframeMode(int mode);

class DynamicVBAccessClass;
class DX8VertexBufferClass;
class DazzleVisibilityClass;
class PointGroupClass;
DX8VertexBufferClass __declspec(dllimport) *VertexBufferAllocate(unsigned short vertex_count_);
TextureClass __declspec(dllimport) *_stdcall LoadTexture(const char *filename, TextureClass::MipCountType mip_level_count, WW3DFormat texture_format, bool allow_compression);


void SHADERS_API			SetFogColor(unsigned int red,unsigned int green,unsigned int blue);
void SHADERS_API			SetFogMode(unsigned int mode);
void SHADERS_API			SetFogDensity(float density);
void SHADERS_API __stdcall	SetProjectionTransform(D3DMATRIX *matrix);
void SHADERS_API __stdcall	GetProjectionTransform(D3DMATRIX *matrix);
void SHADERS_API			SetFogOverride(float start, float end);
void SHADERS_API			ClearFogOverride();

void __declspec(dllimport)	Cleanup_HUD2();
void __declspec(dllimport)	ReadHUDBits(INIClass *hudini);
void __declspec(dllimport)	ReadRuntimeINI(const char *ini);
void __declspec(dllimport)	Update_Radar_Map(float scale,float offsetx,float offsety,const char *texture);
void __declspec(dllimport)	Send_HUD_Number(int number);

void __declspec(dllimport)	Shutdown();
void __declspec(dllimport)	SetScriptNotify(ScriptNotify notify);


#endif
