#include "general.h"

#include "DX8MeshRendererClass.h"
#include "DX8PolygonRendererClass.h"
#include "engine_3d.h"
#include "multilist.h"
#include "DecalMeshClass.h"
#include "vertexbuffer.h"
#include "indexbuffer.h"
#include "DX8TextureCategoryClass.h"
#include "dx8fvf.h"
#include "WW3D.h"
#include "MeshClass.h"

REF_DEF1(texture_category_delete_list, MultiListClass<DX8TextureCategoryClass>, 0x008305D0);
REF_DEF1(fvf_category_container_delete_list, MultiListClass<DX8FVFCategoryContainer>, 0x00830620);



void Render_FVF_Category_Container_List(MultiListClass<DX8FVFCategoryContainer>& containers)
{
	for (MultiListIterator<DX8FVFCategoryContainer> container(&containers); container; ++container)
		container->Render();
}



void Render_FVF_Category_Container_List_Delayed_Passes(MultiListClass<DX8FVFCategoryContainer>& containers)
{
	for (MultiListIterator<DX8FVFCategoryContainer> container(&containers); container; ++container)
		container->Render_Delayed_Procedural_Material_Passes();
}


void Add_Rigid_Mesh_To_Container(MultiListClass<DX8FVFCategoryContainer>* containers, uint fvf, MeshClass* mesh)
{
	MeshModelClass* meshModel = mesh->Peek_Model();
	bool sorting = WW3D::IsSortingEnabled && meshModel->Flags & MeshGeometryClass::SORT && meshModel->SortLevel == 0;
	
	for (MultiListIterator<DX8FVFCategoryContainer> container(containers); container; ++container)
	{
		if (container->sorting == sorting && container->Check_If_Mesh_Fits(meshModel))
		{
			container->Add_Mesh(mesh);
			return;
		}
	}
	
	DX8RigidFVFCategoryContainer* container = new DX8RigidFVFCategoryContainer(fvf, sorting);
	containers->Add_Tail(container);
	container->Add_Mesh(mesh);
}



void Invalidate_FVF_Category_Container_List(MultiListClass<DX8FVFCategoryContainer>& categoryContainer)
{
	while (!categoryContainer.Is_Empty())
		delete categoryContainer.Remove_Head();
}



DX8MeshRendererClass::DX8MeshRendererClass() : enable_lighting(true), camera(0), texture_category_container_list_skin(0), visible_decal_meshes(0)
{
}



DX8MeshRendererClass::~DX8MeshRendererClass()
{
	Shutdown();
	delete texture_category_container_list_skin;
}



void DX8MeshRendererClass::Init()
{
	texture_category_container_list_skin = new MultiListClass<DX8FVFCategoryContainer>;
}



void DX8MeshRendererClass::Shutdown()
{
	Invalidate();
	Clear_Pending_Delete_Lists();
}



void DX8MeshRendererClass::Flush()
{
	if (camera)
	{
		for (int i = 0;i < texture_category_container_lists_rigid.Count();i++)
			Render_FVF_Category_Container_List(*texture_category_container_lists_rigid[i]);
		
		Render_FVF_Category_Container_List(*texture_category_container_list_skin);
		Render_Decal_Meshes();
		for (int i = 0;i < texture_category_container_lists_rigid.Count();i++)
			Render_FVF_Category_Container_List_Delayed_Passes(*texture_category_container_lists_rigid[i]);
	}
	
	DX8Wrapper::Set_Vertex_Buffer(NULL);
	DX8Wrapper::Set_Index_Buffer(NULL, 0);
}



void DX8MeshRendererClass::Register_Mesh_Type(MeshClass* mesh)
{
	MeshModelClass* meshModel = mesh->Peek_Model();
	bool skinned = meshModel->Flags & MeshGeometryClass::SKIN && meshModel->VertexBoneLink;
	
	if (skinned)
	{
		bool sorting = meshModel->Flags & MeshGeometryClass::SORT && WW3D::IsSortingEnabled && meshModel->SortLevel == 0;
		
		for (MultiListIterator<DX8FVFCategoryContainer> category(texture_category_container_list_skin); category; ++category)
		{
			if (sorting == category->sorting && category->Check_If_Mesh_Fits(meshModel))
			{
				category->Add_Mesh(mesh);
				return;
			}
		}
		
		DX8SkinFVFCategoryContainer* category = new DX8SkinFVFCategoryContainer(sorting);
		texture_category_container_list_skin->Add_Tail(category);
		category->Add_Mesh(mesh);
	}
	else
	{
		uint* userLightingArray = mesh->Get_User_Lighting_Array(0);
		const MeshRegKeyStruct key = {meshModel, userLightingArray};
		MeshClass** mesh2 = unk0020.Get(key);
		
		if (mesh2)
		{
			for (MultiListIterator<DX8PolygonRendererClass> polygon(&(*mesh2)->getPolygons()); polygon; ++polygon)
				polygon->Get_Texture_Category()->Add_Polygon_Renderer(new DX8PolygonRendererClass(**polygon, mesh), NULL);
		}
		else
		{
			uint fvf = DX8FVFCategoryContainer::Define_FVF(meshModel, userLightingArray, enable_lighting);
			
			for (int i = 0; i < texture_category_container_lists_rigid.Count(); ++i)
			{
				MultiListClass<DX8FVFCategoryContainer>* containerList = texture_category_container_lists_rigid[i];
				
				DX8FVFCategoryContainer* containerListHead = containerList->Get_Head();
				if (!containerListHead || containerListHead->FVF == fvf)
				{
					Add_Rigid_Mesh_To_Container(containerList, fvf, mesh);
					goto addedToContainerList;
				}
			}
			
			MultiListClass<DX8FVFCategoryContainer>* containerList = new MultiListClass<DX8FVFCategoryContainer>;
			texture_category_container_lists_rigid.Add(containerList);
			Add_Rigid_Mesh_To_Container(containerList, fvf, mesh);
			
addedToContainerList:
			
			if (!mesh->getPolygons().Is_Empty())
				unk0020.Insert(key, mesh);
		}
	}
}



void DX8MeshRendererClass::Unregister_Mesh_Type(MeshClass* mesh)
{
	mesh->Delete_Polygons();
	
	MeshModelClass* meshModel = mesh->Peek_Model();
	MeshRegKeyStruct key = {meshModel, mesh->Get_User_Lighting_Array(0)};
	unk0020.Remove(key, mesh);
	
	if (meshModel->GapFiller)
	{
		delete meshModel->GapFiller;
		meshModel->GapFiller = NULL;
	}
}



void DX8MeshRendererClass::Invalidate()
{
	unk0020.Remove_All();
	
	for (int i = 0; i < texture_category_container_lists_rigid.Count(); ++i)
	{
		Invalidate_FVF_Category_Container_List(*texture_category_container_lists_rigid[i]);
		delete texture_category_container_lists_rigid[i];
	}
	texture_category_container_lists_rigid.Delete_All();
	
	if (texture_category_container_list_skin)
	{
		Invalidate_FVF_Category_Container_List(*texture_category_container_list_skin);
		texture_category_container_list_skin->Reset_List();
	}
	else
		TT_UNTESTED; // The original code created texture_category_container_list_skin here. StealthEye thinks this is not needed. It seems this never occurs though.
}



void DX8MeshRendererClass::Clear_Pending_Delete_Lists()
{
	while (!texture_category_delete_list.Is_Empty())
		delete texture_category_delete_list.Remove_Head();

	while (!fvf_category_container_delete_list.Is_Empty())
		delete fvf_category_container_delete_list.Remove_Head();
}



void DX8MeshRendererClass::Render_Decal_Meshes()
{
	if (visible_decal_meshes)
	{
		ApplyDepthBias(8);
		DecalMeshClass *d = visible_decal_meshes;
		while (d)
		{
			d->Render();
			d = d->Peek_Next_Visible();
		}
		visible_decal_meshes = 0;
		ApplyDepthBias(0);
	}
}



void DX8MeshRendererClass::Add_To_Render_List(DecalMeshClass& decalMesh)
{
	TT_ASSERT(&decalMesh != visible_decal_meshes);

	decalMesh.setNextVisible(visible_decal_meshes);
	visible_decal_meshes = &decalMesh;
}
