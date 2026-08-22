#ifndef TT_INCLUDE__DX8POLYGONRENDERERCLASS_H
#define TT_INCLUDE__DX8POLYGONRENDERERCLASS_H



#include "engine_vector.h"
#include "SphereClass.h"


class MeshClass;
class DX8TextureCategoryClass;



class DX8PolygonRendererClass :
	public MultiListObjectClass
{

public:

	MeshClass* mesh; // 0008
	DX8TextureCategoryClass* texture_category; // 000C
	unsigned int index_offset; // 0010
	unsigned int vertex_offset; // 0014
	unsigned int index_count; // 0018
	unsigned int min_vertex_index; // 001C
	unsigned int vertex_index_range; // 0020
	bool strip; // 0024

public:

	DX8PolygonRendererClass(DX8PolygonRendererClass const& src, MeshClass *mesh);
	DX8PolygonRendererClass(unsigned int index_count_, MeshClass *mesh, DX8TextureCategoryClass* tex_cat,unsigned int vertex_offset_,unsigned int index_offset_,bool strip_);
	~DX8PolygonRendererClass();
	void Render(int indexBaseOffset);
	void Render_Sorted(int indexBaseOffset,const SphereClass &sphere);
	MeshClass *Get_Mesh()
	{
		return mesh;
	}
	DX8TextureCategoryClass* Get_Texture_Category()
	{
		return texture_category;
	}
	void Set_Texture_Category(DX8TextureCategoryClass *tex_cat)
	{
		texture_category = tex_cat;
	}

}; // 0028



#endif