#include "General.h"
#include "decalsys.h"
#include "WW3D.h"
DecalGeneratorClass *DecalSystemClass::Lock_Decal_Generator()
{
	return new DecalGeneratorClass(Generate_Decal_Id(),this);
}

DecalGeneratorClass *MultiFixedPoolDecalSystemClass::Lock_Decal_Generator()
{
	Clear_Decal_Slot(Generator_PoolID,Generator_SlotID);
	return DecalSystemClass::Lock_Decal_Generator();
}

void DecalSystemClass::Unlock_Decal_Generator(DecalGeneratorClass* generator)
{
	if (generator)
	{
		delete generator;
	}
}

void DecalSystemClass::Decal_Mesh_Destroyed(unsigned long decal_id,DecalMeshClass* mesh)
{
}

RENEGADE_FUNCTION
unsigned long DecalSystemClass::Generate_Decal_Id()
AT1(0x005D1ED0);

DecalGeneratorClass::DecalGeneratorClass(uint32 id,DecalSystemClass* system) : System(system), DecalID(id), BackfaceVal(0), ApplyToTranslucentMeshes(false), Material(0)
{
	Material = new MaterialPassClass();
}

DecalGeneratorClass::~DecalGeneratorClass()
{
	REF_PTR_RELEASE(Material);
}

void DecalGeneratorClass::Set_Mesh_Transform(Matrix3D  const&transform)
{
	Matrix3D tmp;
	Matrix3D world_to_texture;
	Matrix4 mesh_to_texture;
	float texsize;
	Transform.Get_Orthogonal_Inverse(tmp);
	Matrix3D::Multiply(tmp,transform,&world_to_texture);
	Matrix4::Multiply(Projection,world_to_texture,&mesh_to_texture);
	if (WW3D::IsTexturingEnabled)
	{
		texsize = 64;
		if (Material->Texture)
		{
			texsize = (float)Material->Texture[0]->Width;
		}
		Mapper->Set_Texture_Transform(mesh_to_texture,texsize);
	}
}

RENEGADE_FUNCTION
void MultiFixedPoolDecalSystemClass::Clear_Decal_Slot(unsigned long pool_id,unsigned long slot_id)
AT1(0x005D2A70);
