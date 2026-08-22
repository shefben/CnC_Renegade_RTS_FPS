#include "General.h"
#include "ProjectorManagerClass.h"
#include "Texture.h"
#include "WW3DAssetManager.h"
#include "PhysTexProjectClass.h"
#include "RenderObjClass.h"
#include "PhysicsSceneClass.h"
TextureClass * create_projector_texture_from_filename( const char * filename )
{
	StringClass	tex_name = filename;
	if (strchr(tex_name, '\\') != 0)
	{
		tex_name = strrchr(filename, '\\') + 1;
	}
	TextureClass * texture = WW3DAssetManager::TheInstance->Get_Texture(tex_name,TextureClass::MIP_LEVELS_1,WW3D_FORMAT_UNKNOWN,true);
	return texture;
}
ProjectorManagerClass::ProjectorManagerClass(void) :
	Projector(NULL),
	ProjectorBoneIndex(0)
{
}

ProjectorManagerClass::~ProjectorManagerClass(void)
{
	Free();
}

void ProjectorManagerClass::Init(const ProjectorManagerDefClass & def,RenderObjClass * model)
{
	Free();
	if (model == NULL)
	{
		return;
	}
	if (def.IsEnabled)
	{
		Projector = new PhysTexProjectClass();
		Projector->Peek_Material_Pass()->EnableOnTranslucentMeshes = false;
		if (Projector != NULL)
		{
			if (def.IsPerspective)
			{
				Projector->Set_Perspective_Projection(def.HorizontalFOV,def.VerticalFOV,def.NearZ,def.FarZ);
			}
			else
			{
				Projector->Set_Ortho_Projection(-def.OrthoWidth * 0.5f,def.OrthoWidth * 0.5f,-def.OrthoHeight * 0.5f,def.OrthoHeight * 0.5f,def.NearZ,def.FarZ);
			}
			if (def.IsAdditive)
			{
				Projector->Init_Additive();
			}
			else
			{
				Projector->Init_Multiplicative();
			}
			Projector->Set_Intensity(def.Intensity);
			TextureClass * tex = create_projector_texture_from_filename(def.TextureName);
			if (tex != NULL)
			{
				tex->UAddressMode = TextureClass::TEXTURE_ADDRESS_CLAMP;
				tex->VAddressMode = TextureClass::TEXTURE_ADDRESS_CLAMP;
				Projector->Set_Texture(tex);
				tex->Release_Ref();
			}
			else
			{
				Projector->Release_Ref();
				Projector = NULL;
			}
		}
		if (Projector != NULL)
		{
			ProjectorBoneIndex = (uint16)model->Get_Bone_Index(def.BoneName);
			if (ProjectorBoneIndex == 0xFFFF)
			{
				ProjectorBoneIndex = 0;
			}
			Projector->Set_Transform(model->Get_Bone_Transform(ProjectorBoneIndex));
			Set_Flag(IS_ANIMATED,(def.IsAnimated) && (ProjectorBoneIndex != 0));
			if (Get_Flag(IS_ANIMATED))
			{
				PhysicsSceneClass::Get_Instance()->Add_Dynamic_Texture_Projector(Projector);
			}
			else
			{
				PhysicsSceneClass::Get_Instance()->Add_Static_Texture_Projector(Projector);
			}
		}
	}
}

void ProjectorManagerClass::Free(void)
{
	if (Projector)
	{
		PhysicsSceneClass::Get_Instance()->Remove_Texture_Projector(Projector);
		Projector->Release_Ref();
		Projector = NULL;
	}
	ProjectorBoneIndex = 0;
}

void ProjectorManagerClass::Update_From_Model(RenderObjClass * model)
{
	if ((Projector != NULL) && (ProjectorBoneIndex != -1))
	{
		const Matrix3D & tm = model->Get_Bone_Transform(ProjectorBoneIndex);
		Projector->Set_Transform(tm);
	}
}

ProjectorManagerDefClass::ProjectorManagerDefClass(void) :
	IsEnabled(false),
	IsPerspective(false),
	IsAdditive(false),
	IsAnimated(false),
	OrthoWidth(10.0f),
	OrthoHeight(10.0f),
	HorizontalFOV(DEG_TO_RADF(10.0f)),
	VerticalFOV(DEG_TO_RADF(10.0f)),
	NearZ(5.0f),
	FarZ(20.0f),
	Intensity(1.0f)
{
}

ProjectorManagerDefClass::~ProjectorManagerDefClass(void)
{
}

void ProjectorManagerDefClass::Validate_Parameters(void)
{
	if (HorizontalFOV <= 0.0f) { HorizontalFOV = DEG_TO_RADF(10.0f); }
	if (VerticalFOV <= 0.0f) { VerticalFOV = DEG_TO_RADF(10.0f); }
	if (OrthoWidth <= 0.0f) { OrthoWidth = 10.0f; }
	if (OrthoHeight <= 0.0f) { OrthoHeight = 10.0f; }
	if (NearZ < 0.0f) { NearZ = 0.0f; }
	if (FarZ < NearZ) { FarZ = NearZ + 10.0f; }
}
