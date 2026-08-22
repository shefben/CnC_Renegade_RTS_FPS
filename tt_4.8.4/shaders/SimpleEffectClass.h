#ifndef TT_INCLUDE__SIMPLEEFFECTCLASS_H
#define TT_INCLUDE__SIMPLEEFFECTCLASS_H



#include "MaterialEffectClass.h"



class MaterialPassClass;
class RenderInfoClass;
class PhysClass;



class SimpleEffectClass :
	public MaterialEffectClass
{

private:

	UNK unk002C;
	MaterialPassClass* materialPass;

public:

	SimpleEffectClass(MaterialPassClass*);
	virtual ~SimpleEffectClass();
	virtual void Render_Push(RenderInfoClass&, PhysClass*);
	virtual void Render_Pop(RenderInfoClass&);

};



#endif