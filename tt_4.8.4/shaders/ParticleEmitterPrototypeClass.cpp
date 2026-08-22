#include "General.h"
#include "ParticleEmitterPrototypeClass.h"
#include "ParticleEmitterDefClass.h"
#include "ParticleEmitterClass.h"
RenderObjClass *ParticleEmitterPrototypeClass::Create()
{
	//if (using CPU particles)
	//{
	return ParticleEmitterClass::Create_From_Definition(*m_pDefinition);
	//}
	//else
	//{
	//return GPUParticleEmitterClass::Create_From_Definition(*m_pDefiniton);
	//}
}
