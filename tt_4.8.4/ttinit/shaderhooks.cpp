#include "General.h"
#include "hooksupport.h"
#include "shaders.h"
#include "DX8Caps.h"
#include "DX8Wrapper.h"
#include "WW3D.h"
#include "WW3DAssetManager.h"
#include "engine_3d.h"
#include "render2d.h"
#include "vertexbuffer.h"
#include "combatmanager.h"
#include "mapper.h"
#include "texture.h"
#include "PhysicsSceneClass.h"
#include "SystemSettings.h"
#include "Subtitle.h"
#include "bink.h"
#include "projector.h"
#include "dx8fvf.h"
#include "textureloader.h"
#include "ProjectorManagerClass.h"
#include "MoveablePhysClass.h"
#include "WWAudioClass.h"
#include "AudibleSoundClass.h"
#include "HUDClass.h"
#include "decalsys.h"
#include "CameraClass.h"

REF_DEF1(CurrentCaps, DX8Caps*, 0x008313E0);

void CreateDX8CapsFunc(D3DCAPS9* caps, D3DADAPTER_IDENTIFIER9& identifier)
{
	if (CurrentCaps) delete CurrentCaps;
	CurrentCaps = new DX8Caps(DX8Wrapper::D3DInterface, DX8Wrapper::D3DDevice, D3DFormat_To_WW3DFormat(_PresentParameters.BackBufferFormat), identifier);
};

DX8VertexBufferClass *Vertex_Buffer_Allocate()
{
	unsigned int vertex_count_;
	_asm {
		mov vertex_count_,edx
	}
	VertexBufferAllocate((unsigned short)vertex_count_);
}

HRESULT _stdcall Create_Index_Buffer_1(D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer9** ppIndexBuffer)
{
	UINT len;
	DWORD Usage;
	_asm {
		mov Usage, edi
		mov len, eax
	}
	return DX8Wrapper::D3DDevice->CreateIndexBuffer(len,Usage,Format,Pool,ppIndexBuffer,NULL);
}


HRESULT _stdcall Create_Index_Buffer_2(D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer9** ppIndexBuffer)
{
	UINT len;
	DWORD Usage;
	_asm {
		mov Usage, edi
		shl edx, 1
		mov len, edx
	}
	return DX8Wrapper::D3DDevice->CreateIndexBuffer(len,Usage,Format,Pool,ppIndexBuffer,NULL);
}

void InitShaderHooks()
{
	if (!Exe)
	{
		SetDX8CapsCreateFunction(&CreateDX8CapsFunc);

		hookAsJump(0x00529B50, 0x00000000, WW3D::Set_Texture_Filter); //hook WW3D::Create_Texture_Filter

		unsigned char vbhook[7] = {0x89,0x44,0x8B,0x7C,0x8B,0x55,0xFC};
		WriteMemory((char *)0x00644854,vbhook,sizeof(vbhook)); //hooks to make all vertex buffers use vertex declarations
		WriteCall((char *)0x00644854+sizeof(vbhook),Vertex_Buffer_Allocate,0x1C); //hooks to make all vertex buffers use vertex declarations
		hookAsJump(0x00554070, 0x00000000, DynamicVBAccessClass::Allocate_Declaration_Dynamic_Buffer); //hooks to make all vertex buffers use vertex declarations
		hookAsJump(0x005543F0, 0x00000000, DynamicVBAccessClass::WriteLockClass::Allocate); //hooks to make all vertex buffers use vertex declarations
		
		hookAsCall2(0x0053EFF7, 0x00000000, GetProjectionTransform, 6); //hooks to make all GetTransform(D3DTS_PROJECTION) calls go through shaders.dll for DazzleRenderObjClass::Render
		hookAsCall2(0x0053FA52, 0x00000000, GetProjectionTransform, 6); //hooks to make all GetTransform(D3DTS_PROJECTION) calls go through shaders.dll for DazzleRenderObjClass::Render_Dazzle
		WriteNop((void *)0x005408ED,2); //hooks to make all SetTransform(D3DTS_PROJECTION) calls go through shaders.dll for DazzleRenderObjClass::Render_Dazzle
		WriteNop((void *)0x00540CAE,2); //hooks to make all SetTransform(D3DTS_PROJECTION) calls go through shaders.dll for DazzleRenderObjClass::Render_Dazzle
		hookAsCall2(0x005408F0, 0x00000000, SetProjectionTransform, 4); //hooks to make all SetTransform(D3DTS_PROJECTION) calls go through shaders.dll for DazzleRenderObjClass::Render_Dazzle
		hookAsCall2(0x00540CB1, 0x00000000, SetProjectionTransform, 4); //hooks to make all SetTransform(D3DTS_PROJECTION) calls go through shaders.dll for DazzleRenderObjClass::Render_Dazzle
		hookAsCall2(0x00551FA2, 0x00000000, Create_Index_Buffer_1, 1); //hooks to make all CreateIndexBuffer calls go through shaders.dll for DX8IndexBufferClass::DX8IndexBufferClass
		hookAsCall2(0x00551FCF, 0x00000000, Create_Index_Buffer_2, 3); //hooks to make all CreateIndexBuffer calls go through shaders.dll for DX8IndexBufferClass::DX8IndexBufferClass
		WriteNop((void *)0x00551F8F,2); //hooks to make all CreateIndexBuffer calls go through shaders.dll for DX8IndexBufferClass::DX8IndexBufferClass
		WriteNop((void *)0x00551FCA,2); //hooks to make all CreateIndexBuffer calls go through shaders.dll for DX8IndexBufferClass::DX8IndexBufferClass

		hookAsJump(0x00530920, 0x00000000, FontCharsClass::Store_GDI_Char); // font hook
		hookAsJump(0x00530BB0, 0x00000000, FontCharsClass::Create_GDI_Font); // font hook


		hookAsJump(0x00529B80, 0x00000000, WW3D::Begin_Render); //hook WW3D::Begin_Render
		hookAsJump(0x0052A310, 0x00000000, WW3D::End_Render); //hook WW3D::End_Render
		hookAsJump(0x0052AC60, 0x00000000, WW3D::Get_Texture_Bitdepth); //hook WW3D::Get_Texture_Bitdepth

		hookAsJump(0x00542CE0, 0x00000000, WW3DAssetManager::Load_3D_Assets); //hook for w3d file hash checking

		hookAsJump(0x00622F40, 0x00000000, PhysicsSceneClass::Customized_Render); //hook PhysicsSceneClass::Customized_Render
		hookAsJump(0x006324E0, 0x00000000, PhysicsSceneClass::Get_Sun_Light_Vector); //hook PhysicsSceneClass::Get_Sun_Light_Vector

		hookAsJump(0x00529620, 0x00000000, WW3D::Init); //hook WW3D::Init
		hookAsJump(0x0054B660, 0x00000000, DX8Wrapper::Set_Light_Environment); //hook DX8Wrapper::Set_Light_Environment
		hookAsJump(0x0054A150, 0x00000000, DX8Wrapper::Flip_To_Primary); //hook DX8Wrapper::Flip_To_Primary
		hookAsJump(0x00548E60, 0x00000000, DX8Wrapper::Set_Device_Resolution); //hook DX8Wrapper::Set_Device_Resoltion
		hookAsJump(0x00549180, 0x00000000, DX8Wrapper::Get_Render_Target_Resolution); //hook DX8Wrapper::Get_Render_Target_Resolution
		hookAsCall2(0x005408D9, 0x00000000, DX8Wrapper::ClearZ, 15); //hooking DX8Wrapper::ZNear/ZFar
		hookAsCall2(0x00540C9A, 0x00000000, DX8Wrapper::ClearZ, 15); //hooking DX8Wrapper::ZNear/ZFar
		hookAsJump(0x0054A8D0, 0x00000000, DX8Wrapper::Draw);  //hooking DX8Wrapper::Draw
		hookAsJump(0x0054B0D0, 0x00000000, DX8Wrapper::_Create_DX8_Texture);  //hooking DX8Wrapper::_Create_DX8_Texture
		hookAsJump(0x0054ADD0, 0x00000000, DX8Wrapper::Create_DX8_Texture);  //hooking DX8Wrapper::Create_DX8_Texture
		hookAsJump(0x0054B1E0, 0x00000000, DX8Wrapper::Create_DX8_Surface);  //hooking DX8Wrapper::_Create_DX8_Surface
		hookAsJump(0x0054BD40, 0x00000000, DX8Wrapper::Set_Gamma);  //hooking DX8Wrapper::Set_Gamma

		hookAsJump(0x0058A830, 0x00000000, VertexMaterialClass::Load_W3D); //hook for texture mappers
		hookAsJump(0x005D6430, 0x00000000, MatrixMapperClass::Apply); //hook for texture mappers
		hookAsJump(0x00628EA0, 0x00000000, PhysicsSceneClass::Generate_Static_Shadow_Projectors); //hook for shadows
		hookAsJump(0x00627EA0, 0x00000000, PhysicsSceneClass::Set_Max_Simultaneous_Shadows); //hook for shadows
		hookAsJump(0x006280E0, 0x00000000, PhysicsSceneClass::Set_Shadow_Mode); //hook for shadows
		hookAsJump(0x00628530, 0x00000000, PhysicsSceneClass::Apply_Projectors); //hook for shadows
		hookAsJump(0x006321C0, 0x00000000, PhysicsSceneClass::Allocate_Decal_Resources); //hook for decals
		hookAsJump(0x006321F0, 0x00000000, PhysicsSceneClass::Release_Decal_Resources); //hook for decals
		hookAsJump(0x00622070, 0x00621910, PhysicsSceneClass::Add_Dynamic_Object); //hook for physics bits

		hookAsJump(0x00670AD0, 0x00000000, MapUnloaded);  //Shaders.dll level end hook

		hookAsJump(0x0041A960, 0x0041A960, SystemSettings::Add_Console_Functions); //hook for SystemSettings stuff
		hookAsJump(0x0041A900, 0x0041A900, SystemSettings::Registry_Load); // These are the addresses of Registry_Load, which is called after Init.

		hookAsJump(0x0076FF00, 0x00000000, Subtitle::Create); //bink hooks
		hookAsJump(0x007701B0, 0x00000000, Subtitle::Build); //bink hooks
			
		hookAsJump(0x0076F680, 0x00000000, BinkMovie::Create); //bink hooks
		hookAsJump(0x0076F6E0, 0x00000000, BinkMovie::Destroy); //bink hooks
		hookAsJump(0x0076F710, 0x00000000, BinkMovie::Think); //bink hooks
		hookAsJump(0x0076F740, 0x00000000, BinkMovie::Render); //bink hooks

		hookAsJump(0x00640D80, 0x00640620, PhysClass::Update_Sun_Status); //hook PhysClass::Update_Sun_Status
		hookAsJump(0x005502B0,        0x0,        TextureClass::Invalidate_Old_Unused_Textures); //texture/surface hooks
		hookAsJump(0x00582A60,        0x0,        ThumbnailManagerClass::Add_Thumbnail_Manager); //texture/surface hooks
		hookAsJump(0x0052C4C0,        0x0,        LoaderThreadClass::Thread_Function); //texture/surface hooks
		hookAsJump(0x00529AB0,        0x0,        WW3D::_Invalidate_Textures); //texture/surface hooks
		hookAsJump(0x0052AAC0,        0x0,        WW3D::Set_Texture_Reduction); //texture/surface hooks
		hookAsJump(0x005503E0,        0x0,        TextureClass::Init); //texture/surface hooks
		hookAsJump(0x00546260,        0x0,        Font3DDataClass::Load_Font_Image); //texture/surface hooks
		hookAsJump(0x0052FD90,        0x0,        Render2DSentenceClass::Allocate_New_Surface); //texture/surface hooks
		hookAsJump(0x0052ECB0,        0x0,        Render2DSentenceClass::Reset); //texture/surface hooks
		hookAsJump(0x0052F7F0,        0x0,        Render2DSentenceClass::Draw_Sentence); //texture/surface hooks
		hookAsJump(0x0052FFF0,        0x0,        Render2DSentenceClass::Build_Sentence); //texture/surface hooks

		hookAsJump(0x00669580,        0x0,        ProjectorManagerClass::Init); //shadow hooks
		hookAsJump(0x005D1F10,        0x0,        DecalSystemClass::Lock_Decal_Generator); //shadow hooks
		hookAsJump(0x005D2790,        0x0,        MultiFixedPoolDecalSystemClass::Lock_Decal_Generator); //shadow hooks
		hookAsJump(0x0066A0E0,        0x0,        DynamicShadowManagerClass::Allocate_Shadow); //shadow hooks
		
		hookAsJump(0x005D21C0, 0x00000000, DecalGeneratorClass::Set_Mesh_Transform); //shadow hooks
		hookAsJump(0x00670080, 0x00000000, CombatManager::Init); //hook so we own CCameraClass
		hookAsJump(0x0058E320, 0x00000000, MeshModelClass::get_deformed_screenspace_vertices); //mesh hooks
		hookAsJump(0x0051E2F0, 0x00000000, AudibleSoundClass::Play); //sound hooks
		hookAsJump(0x00513BC0, 0x00000000, WWAudioClass::Add_To_Playlist); //sound hooks
		hookAsJump(0x00513C60, 0x00000000, WWAudioClass::Remove_From_Playlist); //sound hooks
		hookAsJump(0x005D34C0, 0x00000000, ProjectorClass::Compute_Texture_Coordinate); //shadow hooks
		
		 // Init the client side hooks for overloaded functions

		WriteJump(0x00529d20, (WW3DErrorType (*)(SceneClass*, CameraClass*, bool, bool, Vector3 const&))&WW3D::Render); //hook WW3D::Render

		WriteJump(0x0052a0d0, (WW3DErrorType (*)(RenderObjClass&, RenderInfoClass&))&WW3D::Render); //hook WW3D::Render

		WriteJump(0x005299e0, (WW3DErrorType (*)(char const*))&WW3D::Registry_Save_Render_Device); //hook WW3D::Registry_Save_Render_Device

		WriteJump(0x00549310, (bool (*)(char const *, bool))&DX8Wrapper::Registry_Load_Render_Device); //hook WW3D::Registry_Load_Render_Device
		hookAsJump(0x006ACEF0, 0x00000000, HUDClass::Shutdown); //HUD hooks
		hookAsJump(0x006AD2C0, 0x00000000, HUDClass::Reset); //HUD hooks
		hookAsJump(0x006B42A0, 0x00000000, HUDClass::Save); //HUD hooks
		hookAsJump(0x006B42E0, 0x00000000, HUDClass::Load); //HUD hooks
		hookAsJump(0x006B3970, 0x00000000, HUDClass::Is_Enabled); //HUD hooks
		hookAsJump(0x006B3980, 0x00000000, HUDClass::Enable); //HUD hooks
		hookAsJump(0x006AC530, 0x00000000, HUDClass::Force_Weapon_Chart_Update); //HUD hooks
		hookAsJump(0x006AC540, 0x00000000, HUDClass::Force_Weapon_Chart_Display); //HUD hooks
		hookAsJump(0x006B3990, 0x00000000, HUDClass::Add_Powerup_Weapon); //HUD hooks
		hookAsJump(0x006B3C90, 0x00000000, HUDClass::Add_Powerup_Ammo); //HUD hooks
		hookAsJump(0x006B3D10, 0x00000000, HUDClass::Add_Shield_Grant); //HUD hooks
		hookAsJump(0x006B3DD0, 0x00000000, HUDClass::Add_Health_Grant); //HUD hooks
		hookAsJump(0x006B3E90, 0x00000000, HUDClass::Add_Shield_Upgrade_Grant); //HUD hooks
		hookAsJump(0x006B3F20, 0x00000000, HUDClass::Add_Health_Upgrade_Grant); //HUD hooks
		hookAsJump(0x006B3FB0, 0x00000000, HUDClass::Add_Key_Grant); //HUD hooks
		hookAsJump(0x006B4050, 0x00000000, HUDClass::Add_Objective); //HUD hooks
		hookAsJump(0x006B4160, 0x00000000, HUDClass::Add_Data_Link); //HUD hooks
		hookAsJump(0x006B4200, 0x00000000, HUDClass::Add_Map_Reveal); //HUD hooks
		hookAsJump(0x006AC550, 0x00000000, HUDClass::Damage_Render); //HUD hooks

		hookAsJump(0x00555820, 0x00000000, CameraClass::Update_Frustum_Wrapper); // CameraClass::Update_Frustum hook

		unsigned char ret[1] = {0xC3};
		WriteMemory((char *)0x0042A300,ret,sizeof(ret)); //disable OverlayGameModeClass
		WriteMemory((char *)0x0042A4B0,ret,sizeof(ret)); //disable Overlay3DGameModeClass
		WriteNop((char *)0x006A74F0,5); //disable MessageWindowClass 3D bits
		WriteVtable2(0x007DB1E8,LoadTexture); //hook to catch texture loading
	}

};
