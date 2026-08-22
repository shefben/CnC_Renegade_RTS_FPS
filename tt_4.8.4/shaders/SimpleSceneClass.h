#ifndef TT_INCLUDE_SIMPLESCENECLASS_H
#define TT_INCLUDE_SIMPLESCENECLASS_H
#include "SceneClass.h"
#include "engine_vector.h"
class SimpleSceneClass : public SceneClass {
public:
	bool Visibility_Checked;
	RefMultiListClass<RenderObjClass> RenderList;
	RefMultiListClass<RenderObjClass> UpdateList;
	RefMultiListClass<RenderObjClass> LightList;
	RefMultiListClass<RenderObjClass> ReleaseList;
	~SimpleSceneClass();
	void Add_Render_Object(RenderObjClass* obj);
	void Remove_Render_Object(RenderObjClass* obj);
	SceneIterator *Create_Iterator(bool onlyvisible);
	void Destroy_Iterator(SceneIterator* it);
	void Register(class RenderObjClass* obj, RegType for_what);
	void Unregister(class RenderObjClass* obj, RegType for_what);
	float Compute_Point_Visibility(RenderInfoClass& rinfo,Vector3& point);
	void Customized_Render(RenderInfoClass& rinfo);
	void Post_Render_Processing(RenderInfoClass& rinfo);
	virtual void Remove_All_Render_Objects();
	virtual void Visibility_Check(CameraClass* camera);
	SimpleSceneClass();
};
#endif
