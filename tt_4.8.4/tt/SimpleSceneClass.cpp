#include "General.h"
#include "SimpleSceneClass.h"
#include "RenderInfoClass.h"
#include "DX8Wrapper.h"
SimpleSceneClass::SimpleSceneClass() : Visibility_Checked(false)
{
}
SimpleSceneClass::~SimpleSceneClass()
{
	Remove_All_Render_Objects();
}
void SimpleSceneClass::Add_Render_Object(RenderObjClass* obj)
{
	SceneClass::Add_Render_Object(obj);
	RenderList.Add(obj);
}
void SimpleSceneClass::Remove_Render_Object(RenderObjClass* obj)
{
	SceneClass::Remove_Render_Object(obj);
	RenderList.Remove(obj);
}
void SimpleSceneClass::Remove_All_Render_Objects()
{
	RenderObjClass *obj;
	while ((obj = RenderList.Remove_Head()) != 0)
	{
		if (obj)
		{
			SceneClass::Remove_Render_Object(obj);
			obj->Release_Ref();
		}
	}
}
void SimpleSceneClass::Register(class RenderObjClass* obj, RegType for_what)
{
	RefMultiListClass<RenderObjClass> *renderlist;
	if (for_what == LIGHT)
	{
		renderlist = &LightList;
	}
	else if (for_what == RELEASE)
	{
		renderlist = &ReleaseList;
	}
	else if (for_what == ON_FRAME_UPDATE)
	{
		renderlist = &UpdateList;
	}
	else
	{
		return;
	}
	renderlist->Add(obj);
}
void SimpleSceneClass::Unregister(class RenderObjClass* obj, RegType for_what)
{
	RefMultiListClass<RenderObjClass> *renderlist;
	if (for_what == LIGHT)
	{
		renderlist = &LightList;
	}
	else if (for_what == RELEASE)
	{
		renderlist = &ReleaseList;
	}
	else if (for_what == ON_FRAME_UPDATE)
	{
		renderlist = &UpdateList;
	}
	else
	{
		return;
	}
	renderlist->Remove(obj);
}
RENEGADE_FUNCTION
SceneIterator *SimpleSceneClass::Create_Iterator(bool onlyvisible)
AT1(0x00557650);
void SimpleSceneClass::Destroy_Iterator(SceneIterator* it)
{
	if (it)
	{
		delete it;
	}
}
RENEGADE_FUNCTION
void SimpleSceneClass::Visibility_Check(CameraClass* camera)
AT1(0x005572B0);
RENEGADE_FUNCTION
float SimpleSceneClass::Compute_Point_Visibility(RenderInfoClass& rinfo,Vector3& point)
AT1(0x00557380);
void SimpleSceneClass::Customized_Render(RenderInfoClass& rinfo)
{
	if (!Visibility_Checked)
	{
		Visibility_Check(&rinfo.Camera);
	}
	for (RefMultiListIterator<RenderObjClass> iter (&UpdateList); !iter.Is_Done(); iter.Next())
	{
		RenderObjClass *r = iter.Peek_Obj();
		if (r)
		{
			r->On_Frame_Update();
		}
	}
	DX8Wrapper::Set_Light(0,0);
	DX8Wrapper::Set_Light(1,0);
	DX8Wrapper::Set_Light(2,0);
	DX8Wrapper::Set_Light(3,0);
	int i = 0;
	for (RefMultiListIterator<RenderObjClass> iter (&UpdateList);!iter.Is_Done();iter.Next())
	{
		if (i < 4)
		{
			const LightClass *r = (LightClass *)iter.Peek_Obj();
			if (r)
			{
				DX8Wrapper::Set_Light(i,*r);
			}
		}
		i++;
	}
	for (RefMultiListIterator<RenderObjClass> iter (&RenderList); !iter.Is_Done(); iter.Next())
	{
		RenderObjClass *r = iter.Peek_Obj();
		if (r)
		{
			if (r->Is_Really_Visible())
			{
				r->Render(rinfo);
			}
		}
	}
}
RENEGADE_FUNCTION
void SimpleSceneClass::Post_Render_Processing(RenderInfoClass& rinfo)
AT1(0x005575C0);
void SceneClass::Set_Ambient_Light(Vector3& color)
{
	AmbientLight = color;
}
const Vector3& SceneClass::Get_Ambient_Light()
{
	return AmbientLight;
}
void SceneClass::Set_Fog_Enable(bool set)
{
	FogEnabled = set;
}
bool SceneClass::Get_Fog_Enable()
{
	return FogEnabled;
}
void SceneClass::Set_Fog_Color(Vector3& color)
{
	FogColor = color;
}
Vector3& SceneClass::Get_Fog_Color()
{
	return FogColor;
}
void SceneClass::Set_Fog_Range(float start,float end)
{
	FogStart = start;
	FogEnd = end;
}
void SceneClass::Get_Fog_Range(float* start,float* end)
{
	*start = FogStart;
	*end = FogEnd;
}
float SceneClass::Compute_Point_Visibility(RenderInfoClass& rinfo,Vector3& point)
{
	return 1;
}
RENEGADE_FUNCTION
void SceneClass::Save(ChunkSaveClass& csave)
AT1(0x082878CC);
RENEGADE_FUNCTION
void SceneClass::Load(ChunkLoadClass& cload)
AT1(0x082879BE);
void SceneClass::Pre_Render_Processing(RenderInfoClass& rinfo)
{
}
void SceneClass::Post_Render_Processing(RenderInfoClass& rinfo)
{
}
void SceneClass::Add_Render_Object
   (RenderObjClass* oObject)
{
   if (!oObject)
   {
      return;
   }

   oObject->Notify_Added (this);
}
void SceneClass::Remove_Render_Object
   (RenderObjClass* oObject)
{
   if (!oObject)
   {
      return;
   }

   oObject->Notify_Removed (this);
}
SceneClass::~SceneClass()
{
}
SceneClass::SceneClass() : AmbientLight(1,1,1), PolyRenderMode(FILL), ExtraPassPolyRenderMode(EXTRA_PASS_DISABLE), FogEnabled(false), FogColor(0,0,0), FogStart(0), FogEnd(1000)
{
}
