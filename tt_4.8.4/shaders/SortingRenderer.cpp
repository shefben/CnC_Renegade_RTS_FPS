#include "General.h"
#include "SortingRenderer.h"
#include "vertexbuffer.h"
#include "indexbuffer.h"
#include "texture.h"
#include "WW3D.h"
#include "DX8Wrapper.h"


struct ShortVectorIStruct {
	unsigned short i;
	unsigned short j;
	unsigned short k;
};
struct TempIndexStruct {
	ShortVectorIStruct tri;
	unsigned short idx;
};
unsigned int overlapping_node_count;
unsigned short overlapping_vertex_count;
unsigned int overlapping_polygon_count;
SortingNodeStruct *overlapping_nodes[7996];
float *vertex_z_array;
unsigned int vertex_z_array_count;
float *polygon_z_array;
unsigned int polygon_z_array_count;
unsigned int *node_id_array;
unsigned int node_id_array_count;
ShortVectorIStruct *polygon_index_array;
unsigned int polygon_index_array_count;
TempIndexStruct *temp_index_array;
unsigned int temp_index_array_count;
REF_DEF1(sorted_list, DLListClass<SortingNodeStruct>, 0x008356C0);
REF_DEF1(clean_list, DLListClass<SortingNodeStruct>, 0x00835698);

void Release_Refs(SortingNodeStruct* state)
{
	SafeRelease(state->sorting_state.vertex_buffer);
	SafeRelease(state->sorting_state.index_buffer);
	SafeRelease(state->sorting_state.material);
	SafeRelease(state->sorting_state.Textures[0]);
	SafeRelease(state->sorting_state.Textures[1]);
}

TempIndexStruct* Get_Temp_Index_Array(unsigned int count)
{
	if (count > temp_index_array_count)
	{
		if (temp_index_array)
		{
			delete[] temp_index_array;
		}
		temp_index_array = new TempIndexStruct[count];
		temp_index_array_count = count;
	}
	return temp_index_array;
}

float* Get_Vertex_Z_Array(unsigned int count)
{
	if (count > vertex_z_array_count)
	{
		if (vertex_z_array)
		{
			delete[] vertex_z_array;
		}
		vertex_z_array = new float[count];
		vertex_z_array_count = count;
	}
	return vertex_z_array;
}

float* Get_Polygon_Z_Array(unsigned int count)
{
	if (count > polygon_z_array_count)
	{
		if (polygon_z_array)
		{
			delete[] polygon_z_array;
		}
		polygon_z_array = new float[count];
		polygon_z_array_count = count;
	}
	return polygon_z_array;
}

unsigned int* Get_Node_Id_Array(unsigned int count)
{
	if (count > node_id_array_count)
	{
		if (node_id_array)
		{
			delete[] node_id_array;
		}
		node_id_array = new unsigned int[count];
		node_id_array_count = count;
	}
	return node_id_array;
}

ShortVectorIStruct* Get_Polygon_Index_Array(unsigned int count)
{
	if (count > polygon_index_array_count)
	{
		if (polygon_index_array)
		{
			delete[] polygon_index_array;
		}
		polygon_index_array = new ShortVectorIStruct[count];
		polygon_index_array_count = count;
	}
	return polygon_index_array;
}

template<typename T, typename K> void QuickSort(T* values, K* keys, int leftarg, int rightarg)
{
  if (leftarg < rightarg) 
  {
	K pivotvalue = keys[leftarg];
	int left = leftarg - 1;
	int right = rightarg + 1;

	for(;;) {

		while (keys[--right] > pivotvalue);
		while (keys[++left] < pivotvalue);

		if (left >= right) break;

		K k_temp = keys[right];
		keys[right] = keys[left];
		keys[left] = k_temp;

		T v_temp = values[right];
		values[right] = values[left];
		values[left] = v_temp;
	}

	int pivot = right;
	QuickSort(values, keys, leftarg, pivot);
	QuickSort(values, keys, pivot + 1, rightarg);
  }
};

template<typename T, typename T2> void Sort(T *values, T2 *keys, int count)
{
	QuickSort(values,keys,0,count - 1);
};

void Apply_Render_State(RenderStateStruct &rs)
{
	if (ShaderClass::ShaderDirty || (DX8Wrapper::render_state.shader != rs.shader))
	{
		DX8Wrapper::render_state_changed |= SHADER_CHANGED;
		DX8Wrapper::render_state.shader = rs.shader;
	}
	if (DX8Wrapper::render_state.material != rs.material)
	{
		if (rs.material)
		{
			rs.material->Add_Ref();
		}
		if (DX8Wrapper::render_state.material)
		{
			DX8Wrapper::render_state.material->Release_Ref();
		}
		DX8Wrapper::render_state.material = rs.material;
		DX8Wrapper::render_state_changed |= MATERIAL_CHANGED;
	}
	if (DX8Wrapper::render_state.Textures[0] != rs.Textures[0])
	{
		if (rs.Textures[0])
		{
			rs.Textures[0]->Add_Ref();
		}
		if (DX8Wrapper::render_state.Textures[0])
		{
			DX8Wrapper::render_state.Textures[0]->Release_Ref();
		}
		DX8Wrapper::render_state.Textures[0] = rs.Textures[0];
		DX8Wrapper::render_state_changed |= TEXTURE0_CHANGED;
	}
	if (DX8Wrapper::render_state.Textures[1] != rs.Textures[1])
	{
		if (rs.Textures[1])
		{
			rs.Textures[1]->Add_Ref();
		}
		if (DX8Wrapper::render_state.Textures[1])
		{
			DX8Wrapper::render_state.Textures[1]->Release_Ref();
		}
		DX8Wrapper::render_state.Textures[1] = rs.Textures[1];
		DX8Wrapper::render_state_changed |= TEXTURE1_CHANGED;
	}
	for (unsigned int i = 0;i < 4;i++)
	{
		if (DX8Wrapper::render_state.LightEnable[i] != rs.LightEnable[i])
		{
			DX8Wrapper::render_state.LightEnable[i] = rs.LightEnable[i];
			DX8Wrapper::render_state_changed |= LIGHT0_CHANGED + i;
		}
		if (memcmp(&DX8Wrapper::render_state.Lights[i],&rs.Lights[i],sizeof(D3DLIGHT8)))
		{
			DX8Wrapper::render_state.Lights[i] = rs.Lights[i];
			DX8Wrapper::render_state_changed |= LIGHT0_CHANGED + i;
		}
	}
	if (memcmp(&DX8Wrapper::render_state.world,&rs.world,sizeof(Matrix4)))
	{
		DX8Wrapper::render_state.world = rs.world;
		DX8Wrapper::render_state_changed &= ~WORLD_IDENTITY;
		DX8Wrapper::render_state_changed |= WORLD_CHANGED;
	}
	if (memcmp(&DX8Wrapper::render_state.view,&rs.view,sizeof(Matrix4)))
	{
		DX8Wrapper::render_state.view = rs.view;
		DX8Wrapper::render_state_changed &= ~VIEW_IDENTITY;
		DX8Wrapper::render_state_changed |= VIEW_CHANGED;
	}
}

bool SortingRendererClass::_EnableTriangleDraw = true;

void SortingRendererClass::Flush_Sorting_Pool()
{
	if (overlapping_node_count)
	{
		unsigned int* node_id_array_ = Get_Node_Id_Array(overlapping_polygon_count);
		ShortVectorIStruct* polygon_idx_array = Get_Polygon_Index_Array(overlapping_polygon_count);
		float* polygon_z_array_ = Get_Polygon_Z_Array(overlapping_polygon_count);
		DynamicVBAccessClass dyn_vb_access = DynamicVBAccessClass(2,0x252,overlapping_vertex_count);
		{
			DynamicVBAccessClass::WriteLockClass lock = DynamicVBAccessClass::WriteLockClass(&dyn_vb_access);
			VertexFormatXYZNDUV2* dest_verts = lock.Get_Formatted_Vertex_Array();
			unsigned short vertex_count = 0;
			unsigned int polygon_count = 0;
			for (unsigned int node_id = 0;node_id < overlapping_node_count;node_id++)
			{
				SortingNodeStruct *node = overlapping_nodes[node_id];
				float* vertex_z_array_ = Get_Vertex_Z_Array(node->vertex_count);

				Matrix4 wv = node->sorting_state.world * node->sorting_state.view;

				VertexFormatXYZNDUV2 *src_verts = ((SortingVertexBufferClass *)node->sorting_state.vertex_buffer)->VertexBuffer;
				VertexFormatXYZNDUV2 *vertex_ptr = &src_verts[node->min_vertex_index + node->sorting_state.vba_offset + node->sorting_state.index_base_offset];
				for (unsigned int i = 0;i < node->vertex_count;i++)
				{
					*dest_verts = *vertex_ptr;
					vertex_z_array_[i] = wv[0].Z * vertex_ptr->x + wv[1].Z * vertex_ptr->y + wv[2].Z * vertex_ptr->z + wv[3].Z;
					dest_verts++;
					vertex_ptr++;
				}
				SortingIndexBufferClass *index_buffer = (SortingIndexBufferClass *)node->sorting_state.index_buffer;
				unsigned short *indices = index_buffer->index_buffer;
				unsigned short *index_ptr = &indices[node->sorting_state.iba_offset + node->start_index];
				for (unsigned int i = 0;i < node->polygon_count;i++)
				{
					unsigned short idx1 = index_ptr[(i * 3)] - node->min_vertex_index;
					unsigned short idx2 = index_ptr[(i * 3) + 1] - node->min_vertex_index;
					unsigned short idx3 = index_ptr[(i * 3) + 2] - node->min_vertex_index;
					polygon_z_array_[polygon_count+i] = (vertex_z_array_[idx3] + vertex_z_array_[idx1] + vertex_z_array_[idx2]) / 3.0f;
					node_id_array_[polygon_count+i] = node_id;
					idx1 += vertex_count;
					idx2 += vertex_count;
					idx3 += vertex_count;
					polygon_idx_array[polygon_count+i].i = idx1;
					polygon_idx_array[polygon_count+i].j = idx2;
					polygon_idx_array[polygon_count+i].k = idx3;
				}
				node->min_vertex_index = vertex_count;
				polygon_count += node->polygon_count;
				vertex_count += node->vertex_count;
			}
		}
		TempIndexStruct* tis = Get_Temp_Index_Array(overlapping_polygon_count);
		unsigned int ix;
		for (ix = 0;ix < overlapping_polygon_count;ix++)
		{
			ShortVectorIStruct *poly = &polygon_idx_array[ix];
			tis[ix].tri.i = poly->i;
			tis[ix].tri.j = poly->j;
			tis[ix].tri.k = poly->k;
			tis[ix].idx = (unsigned short)node_id_array_[ix];
		}
		Sort<TempIndexStruct,float>(tis,polygon_z_array_,overlapping_polygon_count);
		DynamicIBAccessClass dyn_ib_access = DynamicIBAccessClass(2,(unsigned short)overlapping_polygon_count*3);
		{
			DynamicIBAccessClass::WriteLockClass lock(&dyn_ib_access);
			unsigned short *indices = lock.Get_Index_Array();
			for (unsigned int i = 0;i < overlapping_polygon_count;i++)
			{
				indices[i*3] = tis[i].tri.i;
				indices[i*3+1] = tis[i].tri.j;
				indices[i*3+2] = tis[i].tri.k;
			}
		}
		DX8Wrapper::Set_Index_Buffer(dyn_ib_access,0);
		DX8Wrapper::Set_Vertex_Buffer(dyn_vb_access);
		bool enable_triangle_draw = DX8Wrapper::_Is_Triangle_Draw_Enabled();
		DX8Wrapper::_Enable_Triangle_Draw(SortingRendererClass::_EnableTriangleDraw);
		unsigned short poly_count = 1;
		unsigned short start_index = 0;
		unsigned int node_id = tis[0].idx;
		for (unsigned int node = 1;node < overlapping_polygon_count;node++)
		{
			if ( node_id != tis[node].idx )
			{
				SortingNodeStruct *sn = overlapping_nodes[node_id];
				Apply_Render_State(sn->sorting_state);
				DX8Wrapper::Draw(4,start_index*3,poly_count,sn->min_vertex_index,sn->vertex_count);
				poly_count = 0;
				start_index = (unsigned short)node;
				node_id = tis[node].idx;
			}
			poly_count++;
		}
		if (poly_count)
		{
			SortingNodeStruct *sn = overlapping_nodes[node_id];
			Apply_Render_State(sn->sorting_state);
			DX8Wrapper::Draw(4,start_index*3,poly_count,sn->min_vertex_index,sn->vertex_count);
		}
		for (node_id = 0;node_id < overlapping_node_count;node_id++)
		{
			SortingNodeStruct *sn = overlapping_nodes[node_id];
			Release_Refs(sn);
			clean_list.Add_Head(sn);
		}
		overlapping_node_count = 0;
		overlapping_polygon_count = 0;
		overlapping_vertex_count = 0;
		DX8Wrapper::_Enable_Triangle_Draw(enable_triangle_draw);
	}
}

void SortingRendererClass::Insert_To_Sorting_Pool(SortingNodeStruct* state)
{
	if (overlapping_node_count >= 4096)
	{
		Release_Refs(state);
	}
	else
	{
		overlapping_nodes[overlapping_node_count] = state;
		overlapping_vertex_count += state->vertex_count;
		overlapping_polygon_count += state->polygon_count;
		overlapping_node_count++;
	}
}

void SortingRendererClass::Flush()
{
	while (SortingNodeStruct* sn = sorted_list.__Pop_Head())
	{
		if (((sn->sorting_state.index_buffer_type == 1) || (sn->sorting_state.index_buffer_type == 3)) && ((sn->sorting_state.vertex_buffer_type == 1) || (sn->sorting_state.vertex_buffer_type == 3)))
		{
			Insert_To_Sorting_Pool(sn);
		}
		else
		{
			DX8Wrapper::Set_Render_State(sn->sorting_state);
			DX8Wrapper::Draw(4,sn->start_index,sn->polygon_count,sn->min_vertex_index,sn->vertex_count);
			DX8Wrapper::Release_Render_State();
			Release_Refs(sn);
			clean_list.Add_Head(sn);
		}
	}
	Flush_Sorting_Pool();
	DynamicIBAccessClass::_Reset(false);
	DynamicVBAccessClass::_Reset(false);
	DynamicDeclarationAccessClass::_Reset(false);
}

void SortingRendererClass::Deinit()
{
	for (SortingNodeStruct *sn = sorted_list.Head(); sn; sn = sorted_list.Head())
	{
		sorted_list.Remove_Head();
		delete sn;
	}
	for (SortingNodeStruct *sn = clean_list.Head(); sn; sn = clean_list.Head())
	{
		clean_list.Remove_Head();
		delete sn;
	}
	if (vertex_z_array)
	{
		delete[] vertex_z_array;
		vertex_z_array = 0;
		vertex_z_array_count = 0;
	}
	if (polygon_z_array)
	{
		delete[] polygon_z_array;
		polygon_z_array = 0;
		polygon_z_array_count = 0;
	}
	if (node_id_array)
	{
		delete[] node_id_array;
		node_id_array = 0;
		node_id_array_count = 0;
	}
	if (polygon_index_array)
	{
		delete[] polygon_index_array;
		polygon_index_array = 0;
		polygon_index_array_count = 0;
	}
	if (temp_index_array)
	{
		delete[] temp_index_array;
		temp_index_array = 0;
		temp_index_array_count = 0;
	}
}



SortingNodeStruct* SortingRendererClass::Get_Sorting_Struct()
{
	SortingNodeStruct* node = clean_list.__Pop_Head();
	return node ? node : new SortingNodeStruct;
}



void SortingRendererClass::Insert_Triangles(const SphereClass& boundingSphere, uint16 startIndex, uint16 polygonCount, uint16 minVertexIndex, uint16 vertexCount)
{
	if (!WW3D::IsSortingEnabled)
		DX8Wrapper::Draw_Triangles(startIndex, polygonCount, minVertexIndex, vertexCount);
	else
	{
		//Debug_Statistics::Record_Sorting_Polys_And_Vertices(polygonCount, vertexCount);
		
		SortingNodeStruct* sortingStruct = Get_Sorting_Struct();
		sortingStruct->sorting_state = DX8Wrapper::render_state;
		sortingStruct->bounding_sphere = boundingSphere;
		sortingStruct->start_index = startIndex;
		sortingStruct->polygon_count = polygonCount;
		sortingStruct->min_vertex_index = minVertexIndex;
		sortingStruct->vertex_count = vertexCount;
		Vector4 v = sortingStruct->sorting_state.view * sortingStruct->sorting_state.world * sortingStruct->bounding_sphere.Center;
		sortingStruct->transformed_center = Vector3(v.X,v.Y,v.Z);
		
		
		for (SortingNodeStruct* node = sorted_list.Head(); node; node = node->Succ())
		{
			if (sortingStruct->transformed_center.Z > node->transformed_center.Z)
			{
				sortingStruct->Insert_Before(node);
				return;
			}
		}
		
		sorted_list.Add_Tail(sortingStruct);
	}
}



void SortingRendererClass::Insert_Triangles(uint16 startIndex, uint16 polygonCount, uint16 minVertexIndex, uint16 vertexCount)
{
	SphereClass boundingSphere;
	boundingSphere.Center = Vector3(0, 0, 0);
	boundingSphere.Radius = 0;
	Insert_Triangles(boundingSphere, startIndex, polygonCount, minVertexIndex, vertexCount);
}
