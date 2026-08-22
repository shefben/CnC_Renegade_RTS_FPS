#ifndef TT_INCLUDE__SORTINGRENDERER_H
#define TT_INCLUDE__SORTINGRENDERER_H



#include "DLListClass.h"
#include "engine_3d.h"
#include "SphereClass.h"



struct SortingNodeStruct :
	public DLNodeClass<SortingNodeStruct>
{

public:

	RenderStateStruct sorting_state; // 000C
	SphereClass bounding_sphere; // 025C
	Vector3 transformed_center; // 026C
	unsigned short start_index; // 0278
	unsigned short polygon_count; // 027A
	unsigned short min_vertex_index; // 027C
	unsigned short vertex_count; // 027E

}; // 0280



class SortingRendererClass
{

public:

	SHADERS_API static bool _EnableTriangleDraw;

	static SortingNodeStruct* Get_Sorting_Struct();
	static void Insert_Triangles(const SphereClass& boundingSphere, uint16 startIndex, uint16 polygonCount, uint16 minVertexIndex, uint16 vertexCount);
	static void Insert_Triangles(uint16 startIndex, uint16 polygonCount, uint16 minVertexIndex, uint16 vertexCount);
	static void Flush_Sorting_Pool();
	static void Insert_To_Sorting_Pool(SortingNodeStruct* state);
	static void Flush();
	static void Deinit();
};



#endif