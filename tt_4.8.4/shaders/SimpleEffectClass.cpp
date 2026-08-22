#include "General.h"
#include "SimpleEffectClass.h"

#include "MaterialPassClass.h"
#include "RenderInfoClass.h"



SimpleEffectClass::SimpleEffectClass(MaterialPassClass* _materialPass)
{
	materialPass =_materialPass;
	materialPass->Add_Ref();
}



SimpleEffectClass::~SimpleEffectClass()
{
	REF_PTR_RELEASE(materialPass);
}



void SimpleEffectClass::Render_Push(RenderInfoClass& renderInfo, PhysClass* phys)
{
	renderInfo.Push_Material_Pass(materialPass);
}



void SimpleEffectClass::Render_Pop(RenderInfoClass& renderInfo)
{
	renderInfo.Pop_Material_Pass();
}
