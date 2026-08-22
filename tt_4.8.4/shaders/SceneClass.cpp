#include "general.h"

#include "SceneClass.h"
#include "engine_3d.h"
#include "shaderstatemanager.h"
#include "dx8wrapper.h"
#include "WW3D.h"
void SceneClass::Add_Render_Object
   (RenderObjClass* oObject)
{
   if (!oObject)
   {
      return;
   }

   oObject->Notify_Added (this);
}

extern bool FogOverride;
extern float FogOverrideStart;
extern float FogOverrideEnd;
extern int FogRed;
extern int FogGreen;
extern int FogBlue;
extern bool AllowFogOverride;
void SceneClass::Render(RenderInfoClass& rinfo)
{
	if ((!FogOverride) || (!AllowFogOverride))
	{
		DX8Wrapper::FogEnable = this->FogEnabled;
	}
	else
	{
		DX8Wrapper::FogEnable = true;
	}
	if ((FogCustomColor) && (AllowFogOverride))
	{
		FogState.ChangeFogColor(FogRed,FogGreen,FogBlue);
	}
	else
	{
		FogState.ChangeFogColor(this->FogColor);
	}
	if ((FogOverride) && (AllowFogOverride))
	{
		FogState.ChangeFogRange(FogOverrideStart,FogOverrideEnd);
	}
	else
	{
		FogState.ChangeFogRange(this->FogStart,this->FogEnd);
	}
	if (!this->ExtraPassPolyRenderMode)
	{
		this->Customized_Render(rinfo);
	}
	else
	{
		bool b = WW3D::IsTexturingEnabled;
		Vector3 v = Vector3(0,0,0);
		ApplyDepthBias(0);
		this->Customized_Render(rinfo);
		switch (this->ExtraPassPolyRenderMode)
		{
		case EXTRA_PASS_CLEAR_LINE:
			DX8Wrapper::Clear(true,false,v,1.0,0);
		case EXTRA_PASS_LINE:
			WW3D::IsTexturingEnabled = false;
			StateManager::SetRenderState(D3DRS_FILLMODE,D3DFILL_WIREFRAME);
			this->Customized_Render(rinfo);
			ApplyDepthBias(7);
			break;
		}
		WW3D::IsTexturingEnabled = b;
	}
}
