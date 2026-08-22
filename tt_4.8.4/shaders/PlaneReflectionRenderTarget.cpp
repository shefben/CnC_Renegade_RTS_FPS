#include "General.h"
#include "shader.h"
#include "WW3D.h"
#include "scripts.h"
#include "BackgroundMgrClass.h"
#include "WeatherMgrClass.h"
#include "CombatManager.h"
#include "PhysicsSceneClass.h"
#include "DazzleLayerClass.h"
#include "WW3DAssetManager.h"
#include "PlaneReflectionRenderTarget.h"
#include "WeaponViewClass.h"
#include "SimpleSceneClass.h"
#include "CCameraClass.h"

#pragma warning(push)
#pragma warning(disable: 4351) // warning C4351: new behavior: elements of array will be default initialized - I KNOW THATS WHY I PUT IT THERE

PlaneReflectionRenderTarget::PlaneReflectionRenderTarget(RenderTargetDatabase* tex, WaterSystemClass* water_system): RenderTargetTexture(tex), WaterSystem(NULL), Initialized(false), MeshName(), ShaderToDisable(NULL)
{
	Mesh = NULL;
	Camera = new PlaneReflectionCamera();
	REF_PTR_SET(WaterSystem, water_system);
};

PlaneReflectionRenderTarget::~PlaneReflectionRenderTarget()
{
	REF_PTR_RELEASE(WaterSystem);
	REF_PTR_RELEASE(Camera);
	REF_PTR_RELEASE(Mesh);
	delete[] ShaderToDisable;
	ShaderToDisable = NULL;
};

bool PlaneReflectionRenderTarget::LoadFromINI(INIClass* ini, const char* key)
{
	bool res = RenderTargetTexture::LoadFromINI(ini, key);
	if (!res) return false;
	ini->Get_String(key, "Mesh", "NULL", this->MeshName, sizeof(this->MeshName));

	char temp[256] = {};
	ini->Get_String(key, "DisableShaderDuringPass", "INVALID_SHADER", temp, sizeof(temp));
	if (strcmp(temp, "INVALID_SHADER") != 0)
	{
		this->ShaderToDisable = newstr(temp);
	}
	return true;
};

void PlaneReflectionRenderTarget::SetMesh(const char* mesh_name)
{
	if (this->Mesh) this->Mesh->Release();
	this->Mesh = WW3DAssetManager::TheInstance->Create_Render_Obj(mesh_name);
};

void PlaneReflectionRenderTarget::Initialize()
{
	SetMesh(MeshName);

	const AABoxClass& mesh_aab = Mesh->Get_Bounding_Box();

	static const float height_tweak = 0.02f;

	Vector3 point1 = mesh_aab.Center + Vector3( mesh_aab.Extent.X,	mesh_aab.Extent.Y, mesh_aab.Extent.Z + height_tweak);
	Vector3 point2 = mesh_aab.Center + Vector3(-mesh_aab.Extent.X,	mesh_aab.Extent.Y, mesh_aab.Extent.Z + height_tweak);
	Vector3 point3 = mesh_aab.Center + Vector3(-mesh_aab.Extent.X, -mesh_aab.Extent.Y, mesh_aab.Extent.Z + height_tweak);

	*Camera = *(CameraClass*)COMBAT_CAMERA;

	PlaneClass reflection_plane(point1, point2, point3);
	Camera->Set_Reflection_Plane(reflection_plane);
	WaterSystem->Set_Water_Plane(reflection_plane);
	Initialized = true;
}

void PlaneReflectionRenderTarget::Render()
{
	DebugEventStart(DEBUG_COLOR1,L"PlaneReflectionRenderTarget::Render");
	if (!Initialized) Initialize();

	*Camera = *(CameraClass*)COMBAT_CAMERA;;
	
	IDirect3DSurface9* surface = this->Texture->Get_D3D_Surface_Level(0);
	DX8Wrapper::Set_Render_Target(surface, true);
	SafeRelease(surface);

	ProgrammableShader* disabled_shader = this->ShaderToDisable ? ShaderController.PeekShader(this->ShaderToDisable) : NULL;
	if (disabled_shader)
	{
		disabled_shader->SetDrawingDisabled(true);
	}

	ShaderClass::Invert_Backface_Culling(true);

	bool had_hands_obj = WeaponViewClass::HandsPhysObj && CombatManager::Get_Scene()->Contains(WeaponViewClass::HandsPhysObj);
	if (had_hands_obj) CombatManager::Get_Scene()->Remove_Object(WeaponViewClass::HandsPhysObj);

	BackgroundMgrClass::Set_Lens_Flare_Visible(false);
	BackgroundMgrClass::Update(CombatManager::Get_Scene(), this->Camera);
	WeatherMgrClass::Update(CombatManager::Get_Scene(), this->Camera);


	CombatManager::Get_Scene()->Pre_Render_Processing(*this->Camera);
	Vector3 vector = BackgroundMgrClass::Get_Clear_Color();
	WW3D::Begin_Render(true, true, vector, NULL);

	// Tamper with the texture associate with us, the shader to disable, the cull mode, and the hands
	IDirect3DTexture9* the_texture = this->Texture->D3DTexture;
	this->Texture->D3DTexture = NULL;
	Camera->Set_User_Clip_Plane_Status(false);
	WW3D::Render(CombatManager::Get_Background_Scene(), this->Camera, false, false, vector);
	Camera->Set_User_Clip_Plane_Status(true);
	WW3D::Render(CombatManager::Get_Scene(), this->Camera, false, false, vector);
	if (CombatManager::Get_Dazzle_Layer()) CombatManager::Get_Dazzle_Layer()->Render(this->Camera);

	this->Texture->D3DTexture = the_texture;

	WW3D::End_Render(false);
	CombatManager::Get_Scene()->Post_Render_Processing();
	BackgroundMgrClass::Set_Lens_Flare_Visible(true);

	DX8Wrapper::Set_Render_Target(NULL, true);
	
	// And then undo our tampering
	ShaderClass::Invert_Backface_Culling(false);
	if (had_hands_obj) CombatManager::Get_Scene()->Add_Dynamic_Object(WeaponViewClass::HandsPhysObj);
	if (disabled_shader)
	{
		disabled_shader->SetDrawingDisabled(false);
		disabled_shader = NULL;
	}

	DebugEventEnd();
};

#pragma warning(pop)
