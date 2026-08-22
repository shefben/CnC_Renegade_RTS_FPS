#include "General.h"
#include "PhysDecalSysClass.h"
int PhysDecalSysClass::decalpoolsize = 50;
void PhysDecalSysClass::allocate_resources()
{
	vertmat = new VertexMaterialClass;
	vertmat->Set_Ambient(0,0,0);
	vertmat->Set_Diffuse(0,0,0);
	vertmat->Set_Specular(0,0,0);
	vertmat->Set_Emissive(1,1,1);
	vertmat->Set_Opacity(1);
	vertmat->Set_Shininess(0);
	shader = ShaderClass::_PresetAlphaShader;
}

void PhysDecalSysClass::release_resources()
{
	REF_PTR_RELEASE(vertmat);
}

RENEGADE_FUNCTION
void PhysDecalSysClass::Set_Temporary_Decal_Pool_Size(int size)
AT1(0x0063E6F0);

RENEGADE_FUNCTION
void PhysDecalSysClass::Unlock_Decal_Generator(DecalGeneratorClass* generator)
AT1(0x0063E4D0);

RENEGADE_FUNCTION
void PhysDecalSysClass::Decal_Mesh_Destroyed(unsigned long decal_id,DecalMeshClass* mesh)
AT1(0x0063E6D0);

unsigned long PhysDecalSysClass::Generate_Decal_Id()
{
	if (is_permanent)
	{
		return DecalSystemClass::Generate_Decal_Id() | 0x80000000;
	}
	else
	{
		return count;
	}
}
