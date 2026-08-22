#include "General.h"
#include "hlod.h"

void HLodClass::Render(RenderInfoClass &rinfo)
{
	if (Is_Not_Hidden_At_All())
	{
		Animatable3DObjClass::Render(rinfo);
		ModelArrayClass *mlod = &Lod[CurLod];
		for (int pos = 0;pos < mlod->Count();pos++)
		{
			(*mlod)[pos].Model->Render(rinfo);
		}
		if (!(Bits & SUBOBJS_MATCH_LOD))
		{
			for (int pos = 0;pos < AdditionalModels.Count();pos++)
			{
				AdditionalModels[pos].Model->Render(rinfo);
			}
		}
		else
		{
			for (int pos = 0;pos < AdditionalModels.Count();pos++)
			{
				AdditionalModels[pos].Model->Set_LOD_Level(Get_LOD_Level());
				AdditionalModels[pos].Model->Render(rinfo);
			}
		}
	}
}
