#ifndef TT_INCLUDE__PARTICLEEMITTERPROTOTYPECLASS_H
#define TT_INCLUDE__PARTICLEEMITTERPROTOTYPECLASS_H
#include "prototypeclass.h"
class ParticleEmitterDefClass;
class ParticleEmitterPrototypeClass : public PrototypeClass {
public:
	ParticleEmitterPrototypeClass(ParticleEmitterDefClass *pdef);
	~ParticleEmitterPrototypeClass();
	const char *Get_Name() const;
	int Get_Class_ID() const;
	RenderObjClass SHADERS_API *Create();
	ParticleEmitterDefClass *Get_Definition() const;
protected:
	ParticleEmitterDefClass* m_pDefinition;
};

#endif
