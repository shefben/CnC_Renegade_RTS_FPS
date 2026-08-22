#include "General.h"
#include "DX8TextureCategoryClass.h"



#include "DX8PolygonRendererClass.h"
#include "engine_3d.h"
#include "texture.h"
#include "PolyRenderTaskClass.h"
#include "MeshClass.h"
#include "DX8MeshRendererClass.h"
#include "CameraClass.h"
#include "DX8Wrapper.h"
#include "WW3D.h"
#include "SortingRenderer.h"
#include "dx8fvf.h"
#include "indexbuffer.h"

void Set_Light_Environment(LightEnvironmentClass* light_env);
extern REF_DECL1(texture_category_delete_list, MultiListClass<DX8TextureCategoryClass>);



DX8TextureCategoryClass::DX8TextureCategoryClass(DX8FVFCategoryContainer* _container, TextureClass** _textures, ShaderClass _shader, VertexMaterialClass* _material, int _pass)
{
	pass = _pass;
	shader = _shader;
	material = _material;
	container = _container;
	render_task_head = NULL;
	textures[0] = 0;
	textures[1] = 0;
	REF_PTR_SET(textures[0],_textures[0]);
	REF_PTR_SET(textures[1],_textures[1]);
	material->Add_Ref();
}



DX8TextureCategoryClass::~DX8TextureCategoryClass()
{
	for (DX8PolygonRendererClass* renderer = PolygonRendererList.Get_Head(); renderer; renderer = PolygonRendererList.Get_Head())
		TheDX8MeshRenderer.Unregister_Mesh_Type(renderer->Get_Mesh());
	REF_PTR_RELEASE(textures[0]);
	REF_PTR_RELEASE(textures[1]);
	REF_PTR_RELEASE(material);
}



void DX8TextureCategoryClass::Add_Render_Task(DX8PolygonRendererClass* renderer, MeshClass* mesh)
{
	PolyRenderTaskClass* task = new PolyRenderTaskClass(renderer, mesh);
	task->Set_Next_Visible(render_task_head);
	render_task_head = task;
	container->visible_texture_category_list[pass].Add(this);
	container->AnythingToRender = true;
}



bool DX8TextureCategoryClass::Add_Polygon_Renderer(DX8PolygonRendererClass* renderer, DX8PolygonRendererClass* previousRenderer)
{
	renderer->Set_Texture_Category(this);
	
	if (previousRenderer)
		return PolygonRendererList.Add_After(renderer, previousRenderer, false);
	else
		return PolygonRendererList.Add(renderer);
}



bool DX8TextureCategoryClass::Remove_Polygon_Renderer(DX8PolygonRendererClass* renderer)
{
	PolygonRendererList.Remove(renderer);
	renderer->Set_Texture_Category(NULL);
	
	if (PolygonRendererList.Is_Empty())
	{
		container->Remove_Texture_Category(this);
		return texture_category_delete_list.Add_Tail(this);
	}
	else
		return false;
}



void DX8TextureCategoryClass::Render()
{
	for (uint i = 0; i < 2; i++)
	{
		TextureClass* texture = textures[i];
		if (texture != DX8Wrapper::render_state.Textures[i])
		{
			if (texture)
				texture->Add_Ref();
			
			SafeRelease(DX8Wrapper::render_state.Textures[i]);
			
			DX8Wrapper::render_state.Textures[i] = texture;
			DX8Wrapper::render_state_changed |= TEXTURE0_CHANGED << i;
		}
	}
	
	if (material != DX8Wrapper::render_state.material)
	{
		if (material)
			material->Add_Ref();
		
		SafeRelease(DX8Wrapper::render_state.material);
		
		DX8Wrapper::render_state.material = material;
		DX8Wrapper::render_state_changed |= MATERIAL_CHANGED;
	}
	
	if (ShaderClass::ShaderDirty || shader != DX8Wrapper::render_state.shader)
	{
		DX8Wrapper::render_state.shader = shader;
		DX8Wrapper::render_state_changed |= SHADER_CHANGED;
	}
	
	for (PolyRenderTaskClass* renderTask = render_task_head, *nextRenderTask; renderTask; renderTask = nextRenderTask)
	{
		DX8PolygonRendererClass* polyRenderer = renderTask->Peek_Polygon_Renderer();
		MeshClass* mesh = renderTask->Peek_Mesh();

		if (mesh->getLightEnvironment())
			DX8Wrapper::Set_Light_Environment(mesh->getLightEnvironment());

		TT_ASSERT((mesh->Peek_Model()->Flags & 0xE00) == 0 || (mesh->Peek_Model()->Flags & 0xE00) == 0x200 || (mesh->Peek_Model()->Flags & 0xE00) == 0x400 || (mesh->Peek_Model()->Flags & 0xE00) == 0x800);
		if (mesh->getIsTransformIdentity() || mesh->Peek_Model()->Flags & MeshGeometryClass::SKIN)
		{
			if (!(DX8Wrapper::render_state_changed & WORLD_IDENTITY))
			{
				DX8Wrapper::render_state.world = Matrix4::IDENTITY;
				DX8Wrapper::render_state_changed |= WORLD_CHANGED | WORLD_IDENTITY;
			}
		}
		else
		{
			Matrix4 transformation;
			transformation[3] = Vector4(0, 0, 0, 1); // We will cast to Matrix3D and fill all rows but row 3.

			if (mesh->Peek_Model()->Flags & MeshGeometryClass::ALIGNED)
			{
				Vector3 meshPosition = mesh->Get_Position();
				Vector3 targetPosition = meshPosition + TheDX8MeshRenderer.getCamera()->Get_Transform().getZAxis();
				((Matrix3D&)transformation).Obj_Look_At(meshPosition, targetPosition);
			}
			// Used to do the MeshGeometryClass::SKIN identity thing here instead of above. As long as the assert doesn't hit the current way should be safe and more efficient.
			else if (mesh->Peek_Model()->Flags & MeshGeometryClass::ORIENTED)
			{
				Vector3 meshPosition = mesh->Get_Position();
				Vector3 targetPosition = TheDX8MeshRenderer.getCamera()->Get_Position();
				((Matrix3D&)transformation).Obj_Look_At(meshPosition, targetPosition);
			}
			else
			{
				mesh->Validate_Transform();
				(Matrix3D&)transformation = mesh->Get_Transform();
			}

			DX8Wrapper::Set_Transform(D3DTS_WORLD, transformation);
		}
	
		//if (!(DX8RendererDebugger::Enabled() && mesh->getDebuggerDisable()))
		{
			if (mesh->Peek_Model()->Flags & MeshGeometryClass::SORT && WW3D::IsSortingEnabled)
			{
				polyRenderer->Render_Sorted(mesh->getBaseVertexOffset(),mesh->Get_Bounding_Sphere());				
			}
			else
				polyRenderer->Render(mesh->getBaseVertexOffset());
		}
		
		nextRenderTask = renderTask->Get_Next_Visible();
		delete renderTask;
	}
	
	render_task_head = NULL;
}



uint DX8TextureCategoryClass::Add_Mesh(Vertex_Split_Table& vertexSplitTable, uint vertexOffset, uint usedIndices, IndexBufferClass* indexBuffer, uint pass)
{
	uint homogeneousPolygonCount = 0;
	
	for (int polygonIndex = 0; polygonIndex < vertexSplitTable.polygon_count; ++polygonIndex)
		if (vertexSplitTable.Peek_Texture(polygonIndex, pass, 0) == textures[0] &&
			vertexSplitTable.Peek_Texture(polygonIndex, pass, 1) == textures[1] &&
			*vertexSplitTable.Peek_Material(polygonIndex, pass) == *material &&
			vertexSplitTable.Peek_Shader(polygonIndex, pass) == shader)
			++homogeneousPolygonCount;
	
	if (homogeneousPolygonCount == 0)
		return 0;
	
	else
	{
		uint homogeneousVertexCount = 3 * homogeneousPolygonCount;
		
		DX8PolygonRendererClass* renderer = new DX8PolygonRendererClass(homogeneousVertexCount, vertexSplitTable.mc, this, vertexOffset, usedIndices, 0);
		PolygonRendererList.Add_Tail(renderer);
		
		IndexBufferClass::AppendLockClass lock(indexBuffer, usedIndices, homogeneousVertexCount);
		uint16* indexIterator = lock.Get_Index_Array();
		
		uint16 maxIndex = 0x0000;
		uint16 minIndex = 0xFFFF;
		
		for (int polygonIndex = 0; polygonIndex < vertexSplitTable.polygon_count; ++polygonIndex)
		{
			if (vertexSplitTable.Peek_Texture(polygonIndex, pass, 0) == textures[0] &&
				vertexSplitTable.Peek_Texture(polygonIndex, pass, 1) == textures[1] &&
				*vertexSplitTable.Peek_Material(polygonIndex, pass) == *material &&
				vertexSplitTable.Peek_Shader(polygonIndex, pass) == shader)
			{
				uint16 index;
				
				index = (uint16)vertexOffset + vertexSplitTable.polygon_array[polygonIndex].I;
				minIndex = min(minIndex, index);
				maxIndex = max(maxIndex, index);
				*(indexIterator++) = index;
				
				index = (uint16)vertexOffset + vertexSplitTable.polygon_array[polygonIndex].J;
				minIndex = min(minIndex, index);
				maxIndex = max(maxIndex, index);
				*(indexIterator++) = index;
				
				index = (uint16)vertexOffset + vertexSplitTable.polygon_array[polygonIndex].K;
				minIndex = min(minIndex, index);
				maxIndex = max(maxIndex, index);
				*(indexIterator++) = index;
			}
		}
		
		renderer->min_vertex_index = minIndex;
		renderer->vertex_index_range = maxIndex - minIndex + 1;
		
		return homogeneousVertexCount;
	}
}
