

#include "general.h"
#include "scripts.h"
#include "shader.h"
#include "TextureController.h"
#include "CombatManager.h"
#include "WW3DAssetManager.h"
#include "PhysicsSceneClass.h"
#include "CastResultStruct.h"
#include "PhysClass.h"
#include "CCameraClass.h"
#include "CameraClass.h"
#include "DX8Wrapper.h"
#include "WW3D.h"
#include "PlaneReflectionRenderTarget.h"
#include "WaterSystemClass.h"
#include "UnderwaterRenderTarget.h"
//#include "ShaderOverrideRenderTarget.h"

RenderTargetDatabase::RenderTargetDatabase(): RefCountClass(), Textures()
{
	this->Textures.set_empty_key(NULL);
};

RenderTargetDatabase::RenderTargetDatabase(const RenderTargetDatabase& old): RefCountClass(), Textures(old.Textures)
{
	this->Textures.set_empty_key(NULL);
};

RenderTargetDatabase::~RenderTargetDatabase()
{
	for (HashMap::iterator it = this->Textures.begin(); it != this->Textures.end(); ++it)
	{
		SafeRelease(it->second);
	};
};

void RenderTargetDatabase::Load(ChunkLoadClass& load)
{
	load;
	//FIXME FIXME FIXME
};

bool RenderTargetDatabase::LoadFromINI(INIClass* ini, const char* key)
{	
	if (!ini) return false;

	WaterSystemClass* water_system = WaterSystemClass::Get_Instance() = ini->Get_Bool("WaterSystem", "Enabled", false) ? new WaterSystemClass() : NULL;

	if (water_system)
	{
		INIClass* ini = Get_INI("water.ini");
		water_system->Load_From_INI("Water", ini);
		Release_INI(ini);
	}

	int count = ini->Get_Int(key,"Count",0);
	for (int i = 1; i <= count; ++i)
	{
		char numBuffer[10];
		sprintf(numBuffer,"%d",i);

		char entry[256];
		ini->Get_String(key, numBuffer, "dummy", entry, 256);
		if (strcmp(entry,"dummy") == 0) continue;

		char rttype[256];
		ini->Get_String(entry,"Type","MissingType",rttype,256);

		RenderTargetTexture* tex = NULL;
		if (_stricmp(rttype,"Camera") == 0)
		{
			tex = new CameraRenderTargetTexture(this);
		}
		else if (_stricmp(rttype, "PlaneReflection") == 0)
		{
			tex = new PlaneReflectionRenderTarget(this, water_system);
		}
		else if (_stricmp(rttype, "Underwater") == 0)
		{
			tex = new UnderwaterRenderTarget(this, water_system);
		}
/*
		else if (_stricmp(rttype, "ShaderOverride") == 0)
		{
			tex = new ShaderOverrideRenderTarget(this);
		};
*/
		if (!tex->LoadFromINI(ini,entry)) 
		{
			tex->Release();
			//FIXME FIXME FIXME
			//Raise hell
			continue;
		};

		this->AddTexture(tex);
	};
	REF_PTR_RELEASE(water_system);
	return true;
};

void RenderTargetDatabase::Save(ChunkSaveClass& save)
{
	save;
	//FIXME FIXME FIXME
};

void RenderTargetDatabase::Unload()
{
	Textures.set_deleted_key("DELETED");
	for (HashMap::iterator it = this->Textures.begin(); it != this->Textures.end(); ++it)
	{
		SafeRelease(it->second);
		Textures.erase(it);
	};
	Textures.clear_deleted_key();
};

void RenderTargetDatabase::AddTexture(RenderTargetTexture* tex)
{
	this->Textures.insert(HashMap::value_type(tex->Name,tex));
	if (!tex->Container) 
	{
		tex->Container = this; 
		this->Add_Ref();
	};
};

void RenderTargetDatabase::RemoveTexture(RenderTargetTexture* tex)
{
	tex;
	//FIXME FIXME FIXME
};

RenderTargetTexture* RenderTargetDatabase::QueryTexture(const char* name)
{
	if (!name) return NULL;
	HashMap::iterator it = this->Textures.find(name);
	return it.pos != it.end ? it->second: NULL;
};

/*
RenderTargetTexture::RenderTargetTexture():
	Name(NULL), 
	AspectRatio(1), 
	TextureWidth(0), 
	TextureHeight(0),
	TextureFormat(D3DFormat_To_WW3DFormat(_PresentParameters.BackBufferFormat)),
	Container(NULL),
	Texture(NULL),
	IdleTime(0),
	LastRenderTime(0)
{
	/* nothing*/
//};


RenderTargetTexture::RenderTargetTexture(RenderTargetDatabase* db): RefCountClass(), DLNodeClass<RenderTargetTexture>()
{
	this->Name = NULL;
	this->AspectRatio = 1;
	this->TextureWidth = this->TextureHeight = 0;
	this->TextureFormat = (_PresentParameters.BackBufferFormat == D3DFMT_X8R8G8B8 ? WW3D_FORMAT_X8R8G8B8 : WW3D_FORMAT_R5G6B5);
	this->Container = db;
	this->Texture = NULL;
	this->IdleTime = 0;
	this->LastRenderTime = 0;
};

RenderTargetTexture::RenderTargetTexture(const RenderTargetTexture& old): RefCountClass(), DLNodeClass<RenderTargetTexture>()
{
	this->Name = old.Name ? newstr(old.Name) : NULL;
	this->AspectRatio = old.AspectRatio;
	this->TextureWidth = old.TextureWidth;
	this->TextureHeight = old.TextureHeight;
	this->TextureFormat = old.TextureFormat;
	this->Container = old.Container;
	this->Texture = old.Texture;
	if (this->Texture)
	{
		this->Texture->Add_Ref();
	}
	this->IdleTime = old.IdleTime;
	this->LastRenderTime = old.LastRenderTime;
};

RenderTargetTexture::~RenderTargetTexture()
{
	SAFE_DELETE_ARRAY(this->Name);
	SafeRelease(this->Texture);
};

void RenderTargetTexture::Load(ChunkLoadClass& load)
{
	load;
	//FIXME FIXME FIXME
};

bool RenderTargetTexture::LoadFromINI(INIClass* ini, const char* key)
{
	SAFE_DELETE(Name);
	char name[256];
	ini->Get_String(key, "Name", "dummy", name, 256);
	if (strcmp(name, "dummy") == 0) return false;
	this->Name = newstr(name);
	this->AspectRatio = ini->Get_Float(key, "AspectRatio", 1);
	this->TextureWidth = ini->Get_Int(key, "TextureWidth", 256);
	this->TextureHeight = ini->Get_Int(key, "TextureHeight", 256);

	{
		char format[64];
		ini->Get_String(key, "TextureFormat","Default",format,64);
		if (_stricmp(format,"R32F") == 0)
		{
			this->TextureFormat = WW3D_FORMAT_R32F;
		}
		else
		{
			this->TextureFormat = (_PresentParameters.BackBufferFormat == D3DFMT_X8R8G8B8 ? WW3D_FORMAT_X8R8G8B8 : WW3D_FORMAT_R5G6B5);
		}
	}

	this->IdleTime = ini->Get_Int(key, "IdleTime", 0); 
	return true;
};

void RenderTargetTexture::Save(ChunkSaveClass& save)
{
	save;
	//FIXME FIXME FIXME
};

unsigned int RenderTargetTexture::GetTextureLastAccessTime()
{
	return Texture ? Texture->LastAccessed : 0;
};

void RenderTargetTexture::SetTexture(TextureClass* tex)
{
	if (tex) tex->Add_Ref();
	if (this->Texture) this->Texture->Release();
	this->Texture = tex;
};

TextureClass* RenderTargetTexture::PeekTexture()
{
	return this->Texture;
};

void RenderTargetTexture::Render()
{
	// base implementation
	// weak, eh? Keep in mind the subclasses create shinies beyond all other shinies on the Renegade engine :)
	LastRenderTime = WW3D::SyncTime;
};

CameraRenderTargetTexture::CameraRenderTargetTexture(RenderTargetDatabase* tex): RenderTargetTexture(tex)
{
	obj = NULL;
};

CameraRenderTargetTexture::~CameraRenderTargetTexture()
{
	SafeRelease(obj);
};

void CameraRenderTargetTexture::AttachObject(PhysClass* obj)
{
	if (obj) obj->Add_Ref();
	if (this->obj) this->obj->Release();
	this->obj = obj;
};

void CameraRenderTargetTexture::Render()
{
	RenderTargetTexture::Render();
};

RenderTargetController::RenderTargetController(): RefCountClass(), pending()
{
	this->cam = new CameraClass();
	this->active_db = NULL;
};

RenderTargetController::RenderTargetController(const RenderTargetController &v): RefCountClass()
{
	TT_INTERRUPT;
};

RenderTargetController::~RenderTargetController()
{
	SafeRelease(cam);
	SafeRelease(active_db);
};

void RenderTargetController::SetCamera(CameraClass* cam)
{
	if (cam) cam->Add_Ref();
	if (this->cam) this->cam->Release();
	this->cam = cam;
};

CameraClass* RenderTargetController::PeekCamera()
{
	return this->cam;
};

DLListClass<RenderTargetTexture>& RenderTargetController::GetPendingTextures()
{
	assert(this->pending.Head() == NULL); // Better use up this entire list every time you get it...

	if (!active_db) return this->pending; // Early out for no loaded database

	for (RenderTargetDatabase::HashMap::iterator it = active_db->Textures.begin(); it != active_db->Textures.end(); ++it)
	{
		RenderTargetTexture* tex = it->second;
		if (tex->GetTextureLastAccessTime() > WW3D::SyncTime + 2500) continue; // No rendering unseen textures
		if (tex->GetLastRenderTime() + tex->GetIdleTime() >= WW3D::SyncTime) continue; // No rendering textures that requested to be idle for x ms
		this->pending.Add_Tail(tex);
	};

	return this->pending;
};

bool RenderTargetController::HasReplacementTexture(const char* filename)
{
	return active_db ? active_db->QueryTexture(filename) != 0 : false;
};

TextureClass* RenderTargetController::GetReplacementTexture(const char* filename)
{
	RenderTargetTexture* tex = active_db->QueryTexture(filename);
	TextureClass* rentex = tex->PeekTexture();
	assert (tex);
	if (rentex)
	{
		rentex->Add_Ref();
		return rentex;
	}
	rentex = DX8Wrapper::Create_Render_Target_NonPow2(tex->TextureWidth,tex->TextureHeight,tex->TextureFormat);
	tex->SetTexture(rentex);
	return rentex;
};

void RenderTargetController::Load(ChunkLoadClass& load)
{
	load;
	//FIXME FIXME FIXME
};

bool RenderTargetController::LoadFromINI(INIClass* ini)
{
	SafeRelease(active_db);
	active_db = new RenderTargetDatabase();
	return active_db->LoadFromINI(ini,"RenderTargets");
};

void RenderTargetController::Save(ChunkSaveClass& save)
{
	save;
	//FIXME FIXME FIXME
};

TextureClass __declspec(dllexport) *_stdcall LoadTexture(const char *filename, TextureClass::MipCountType mip_level_count, WW3DFormat texture_format, bool allow_compression)
{
	if (RenderTargetController::Instance()->HasReplacementTexture(filename))
	{
		return RenderTargetController::Instance()->GetReplacementTexture(filename);
	}
	else
	{
		return WW3DAssetManager::TheInstance->Real_Get_Texture(filename,mip_level_count,texture_format,allow_compression);
	}
};
