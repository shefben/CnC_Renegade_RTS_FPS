#include "General.h"
#include "shaders.h"
#include "shader.h"
#include "WW3D.h"
#include "BackgroundMgrClass.h"
#include "WeatherMgrClass.h"
#include "CombatManager.h"
#include "PhysicsSceneClass.h"
#include "DazzleLayerClass.h"
#include "WW3DAssetManager.h"
#include "WeaponViewClass.h"
#include "SimpleSceneClass.h"
#include "CCameraClass.h"
#include "UnderwaterRenderTarget.h"

void UnderwaterRenderTarget::Initialize()
{
	UnderwaterCamera = new UserClipPlaneCamera();
	*UnderwaterCamera = *(CameraClass*)COMBAT_CAMERA;
	UnderwaterCamera->Set_User_Clip_Plane(WaterSystem->Get_Water_Plane());

	WaterSystem->Set_Underwater_Texture(this->Texture);
	Initialized = true;
}

extern bool AllowFogOverride;
void UnderwaterRenderTarget::Render()
{
	if (!Texture) return;
	DebugEventStart(DEBUG_COLOR1, L"UnderwaterRenderTarget::Render");
	if (!Initialized) Initialize();

	// Keep the underwater camera up to date
	*UnderwaterCamera = *(CameraClass*)COMBAT_CAMERA;

	// Set the render target so we can render to a texture and not the screen
	IDirect3DSurface9* surface = Texture->Get_D3D_Surface_Level(0);
	DX8Wrapper::Set_Render_Target(surface, true);
	SafeRelease(surface);

	// Turn off our texture so we can't accidentally render to it
	IDirect3DTexture9* the_texture = Texture->D3DTexture;
	Texture->D3DTexture = NULL;

	// HACK Remove any active hands
	bool had_hands_obj = WeaponViewClass::HandsPhysObj && CombatManager::Get_Scene()->Contains(WeaponViewClass::HandsPhysObj);
	if (had_hands_obj) CombatManager::Get_Scene()->Remove_Object(WeaponViewClass::HandsPhysObj);

	// HACK turn off the "water" shader
	ProgrammableShader* disabled_shader = ShaderController.PeekShader("water");
	if (disabled_shader) disabled_shader->SetDrawingDisabled(true);

	// Set the fog settings on, I deslike this.
	Vector3 fog_color = WaterSystem->Get_Underwater_Fog_Color();
	SetFogOverride(WaterSystem->Get_Underwater_Fog_Start(), WaterSystem->Get_Underwater_Fog_End());
	SetFogColor(unsigned int(fog_color.X * 255.0f), unsigned int(fog_color.Y * 255.0f), unsigned int(fog_color.Z * 255.0f));

	FogMode fog_mode = FogState.Mode;
	float fog_density = FogState.Density;
	FogState.ChangeFogMode((FogMode)WaterSystem->Get_Underwater_Fog_Mode());
	FogState.ChangeFogDensity(WaterSystem->Get_Underwater_Fog_Density());

	// Prep things for rendering
	COMBAT_SCENE->Pre_Render_Processing(*UnderwaterCamera);
	WW3D::Begin_Render(true, true, fog_color, NULL);

	// Render underwaterness
	AllowFogOverride = true;
	UnderwaterCamera->Set_User_Clip_Plane_Status(true);
	WW3D::Render(COMBAT_SCENE, UnderwaterCamera, false, false, fog_color);
	UnderwaterCamera->Set_User_Clip_Plane_Status(false);

	// Finish off rendering
	WW3D::End_Render(false);
	COMBAT_SCENE->Post_Render_Processing();

	// Clear any fog settings
	AllowFogOverride = false;
	SetFogColor(256, 256, 256);
	ClearFogOverride();
	FogState.ChangeFogDensity(fog_density);
	FogState.ChangeFogMode(fog_mode);

	// HACK turn the "water" shader back on
	if (disabled_shader) disabled_shader->SetDrawingDisabled(false);

	// HACK Glue removed hands back on
	if (had_hands_obj) CombatManager::Get_Scene()->Add_Dynamic_Object(WeaponViewClass::HandsPhysObj);
	
	// Turn our texture back on
	Texture->D3DTexture = the_texture;

	// Clear the render target
	DX8Wrapper::Set_Render_Target(0, true);

	// Done
	DebugEventEnd();
}