#include "general.h"

#include "RenderInfoClass.h"
#include "MaterialPassClass.h"
RenderInfoClass::RenderInfoClass(CameraClass& cam) : Camera(cam),fog_scale(0),fog_start(0),fog_end(0),light_environment(0),AdditionalMaterialPassCount(0),SpecialMaterialPassCount(0),OverrideFlagLevel(0)
{
	OverrideFlag[0] = RINFO_OVERRIDE_DEFAULT;
}

RenderInfoClass::~RenderInfoClass()
{
}

void RenderInfoClass::Push_Material_Pass(MaterialPassClass* matpass)
{
	if (AdditionalMaterialPassCount < 31)
	{
		matpass->Add_Ref();
		AdditionalMaterialPassArray[AdditionalMaterialPassCount] = matpass;
		AdditionalMaterialPassCount++;
	}
	else
	{
		SpecialMaterialPassCount++;
	}
}

void RenderInfoClass::Pop_Material_Pass()
{
	if (!SpecialMaterialPassCount)
	{
		AdditionalMaterialPassCount--;
		if (AdditionalMaterialPassArray[AdditionalMaterialPassCount])
		{
			AdditionalMaterialPassArray[AdditionalMaterialPassCount]->Release_Ref();
		}
	}
	else
	{
		SpecialMaterialPassCount--;
	}
}

int RenderInfoClass::Additional_Pass_Count()
{
	return AdditionalMaterialPassCount;
}

MaterialPassClass *RenderInfoClass::Peek_Additional_Pass(int i)
{
	return AdditionalMaterialPassArray[i];
}

void RenderInfoClass::Push_Override_Flags(RINFO_OVERRIDE_FLAGS flg)
{
	OverrideFlag[++OverrideFlagLevel] = flg;
}

void RenderInfoClass::Pop_Override_Flags()
{
	--OverrideFlagLevel;
}

RenderInfoClass::RINFO_OVERRIDE_FLAGS RenderInfoClass::Current_Override_Flags()
{
	return OverrideFlag[OverrideFlagLevel];
}

RenderInfoClass& RenderInfoClass::operator=(RenderInfoClass& r)
{
	return *this;
}

SpecialRenderInfoClass::SpecialRenderInfoClass(CameraClass& cam,int render_type) : RenderInfoClass(cam), RenderType(render_type), VisRasterizer(0), BWRenderer(0)
{
}

SpecialRenderInfoClass::~SpecialRenderInfoClass()
{
}
