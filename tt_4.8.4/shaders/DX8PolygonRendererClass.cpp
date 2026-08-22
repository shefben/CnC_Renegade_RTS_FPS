#include "General.h"
#include "DX8PolygonRendererClass.h"
#include "DX8TextureCategoryClass.h"
#include "sortingrenderer.h"

#include "engine_3d.h"
#include "DX8Wrapper.h"
#include "MeshClass.h"

DX8PolygonRendererClass::DX8PolygonRendererClass(DX8PolygonRendererClass const& src, MeshClass *mesh_) : mesh(mesh_), texture_category(src.texture_category), index_offset(src.index_offset), vertex_offset(src.vertex_offset), index_count(src.index_count), min_vertex_index(src.min_vertex_index), vertex_index_range(src.vertex_index_range), strip(src.strip)
{
	mesh->Add_Polygon_Renderer(this);
}

DX8PolygonRendererClass::DX8PolygonRendererClass(unsigned int index_count_, MeshClass *mesh_, DX8TextureCategoryClass* tex_cat,unsigned int vertex_offset_,unsigned int index_offset_,bool strip_) : mesh(mesh_), texture_category(tex_cat), index_offset(index_offset_), vertex_offset(vertex_offset_), index_count(index_count_), min_vertex_index(0), vertex_index_range(0), strip(strip_)
{
	mesh->Add_Polygon_Renderer(this);
}

DX8PolygonRendererClass::~DX8PolygonRendererClass()
{
	if (texture_category)
	{
		texture_category->Remove_Polygon_Renderer(this);
	}
}

void DX8PolygonRendererClass::Render(int indexBaseOffset)
{
	if (DX8Wrapper::render_state.index_base_offset != (uint16)indexBaseOffset)
	{
		DX8Wrapper::render_state.index_base_offset = (uint16)indexBaseOffset;
		DX8Wrapper::render_state_changed |= INDEX_BUFFER_CHANGED;
	}

	if (strip)
		DX8Wrapper::Draw_Strip((uint16)index_offset, (uint16)index_count - 2, (uint16)min_vertex_index, (uint16)vertex_index_range);
	else
		DX8Wrapper::Draw_Triangles((uint16)index_offset, (uint16)index_count / 3, (uint16)min_vertex_index, (uint16)vertex_index_range);
}

void DX8PolygonRendererClass::Render_Sorted(int indexBaseOffset,const SphereClass &sphere)
{
	if (DX8Wrapper::render_state.index_base_offset != (uint16)indexBaseOffset)
	{
		DX8Wrapper::render_state.index_base_offset = (uint16)indexBaseOffset;
		DX8Wrapper::render_state_changed |= INDEX_BUFFER_CHANGED;
	}

	SortingRendererClass::Insert_Triangles(sphere, (uint16)index_offset, (uint16)(index_count / 3), (uint16)min_vertex_index, (uint16)vertex_index_range);
}
