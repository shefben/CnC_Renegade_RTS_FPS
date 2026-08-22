#ifndef TT_INCLUDE__DX8TEXTURECATEGORYCLASS_H
#define TT_INCLUDE__DX8TEXTURECATEGORYCLASS_H



#include "engine_vector.h"
#include "engine_3dre.h"
#include "RefCountPtr.h"



class MeshClass;
class TextureClass;
class VertexMaterialClass;
class DX8FVFCategoryContainer;
class PolyRenderTaskClass;
class DX8PolygonRendererClass;
class Vertex_Split_Table;
class IndexBufferClass;
template<typename T> class MultiListClass;



class DX8TextureCategoryClass :
	public MultiListObjectClass
{

private:

	int pass; // 0008
	TextureClass* textures[2]; // 000C
	ShaderClass shader; // 0014
	VertexMaterialClass* material; // 0018
	MultiListClass<DX8PolygonRendererClass> PolygonRendererList; // 001C
	DX8FVFCategoryContainer* container; // 0034
	PolyRenderTaskClass* render_task_head; // 0038


public:

	DX8TextureCategoryClass(DX8FVFCategoryContainer* _container, TextureClass** _textures, ShaderClass _shader, VertexMaterialClass* _material, int _pass);
	~DX8TextureCategoryClass();
	void Add_Render_Task(DX8PolygonRendererClass* renderer, MeshClass* mesh);
	bool Add_Polygon_Renderer(DX8PolygonRendererClass* renderer, DX8PolygonRendererClass* previousRenderer);
	bool Remove_Polygon_Renderer(DX8PolygonRendererClass* renderer);
	void Render();
	uint Add_Mesh(Vertex_Split_Table&, uint, uint, IndexBufferClass*, uint);

	TextureClass* Peek_Texture(int textureIndex) { return textures[textureIndex]; }
	VertexMaterialClass* Peek_Material() { return material; }
	DX8FVFCategoryContainer* Get_Container() { return container; }
	ShaderClass Get_Shader() { return shader; }

}; // 003C



#endif