#ifndef TT_INCLUDE__DX8MESHRENDERERCLASS_H
#define TT_INCLUDE__DX8MESHRENDERERCLASS_H



#include "engine_vector.h"
#include "HashTemplateClass.h"


class DecalMeshClass;
class DX8FVFCategoryContainer;
class CameraClass;
class MeshClass;
class MeshModelClass;



struct MeshRegKeyStruct
{
	MeshModelClass* meshModel;
	uint* userLightingArray;

	uint32 getHash() const
	{
		return HashTemplateKeyClass<uint>::Get_Hash_Value((uint)meshModel + (uint)userLightingArray);
	}

	friend bool operator ==(const MeshRegKeyStruct& a, const MeshRegKeyStruct& b)
	{
		return a.meshModel == b.meshModel && a.userLightingArray == b.userLightingArray;
	}
};



class SHADERS_API DX8MeshRendererClass
{

private:

	bool enable_lighting; // 0000
	CameraClass* camera; // 0004
	SimpleDynVecClass<MultiListClass<DX8FVFCategoryContainer>*> texture_category_container_lists_rigid; // 0008
	MultiListClass<DX8FVFCategoryContainer>* texture_category_container_list_skin; // 0018
	DecalMeshClass* visible_decal_meshes; // 001C
	HashTemplateClass<MeshRegKeyStruct, MeshClass*> unk0020; // 0020

public:

	DX8MeshRendererClass();
	~DX8MeshRendererClass();
	void Invalidate();
	void Flush();
	void Init();
	void Shutdown();
	void Clear_Pending_Delete_Lists();
	void Render_Decal_Meshes();
	void Add_To_Render_List(DecalMeshClass& decalMesh);
	void Register_Mesh_Type(MeshClass *mesh);
	void Unregister_Mesh_Type(MeshClass* mesh);

	void Set_Camera(CameraClass* cam)
	{
		camera = cam;
	}

	CameraClass* getCamera() const
	{
		return camera;
	}

};


#endif
