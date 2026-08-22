#ifndef TT_INCLUDE_DX8FVF_H
#define TT_INCLUDE_DX8FVF_H



#include "multilist.h"
#include "engine_3dre.h"
#include "RefCountPtr.h"



class DX8TextureCategoryClass;
class MaterialPassClass;
class MeshClass;
class IndexBufferClass;
class VertexBufferClass;
class MeshModelClass;



class Vertex_Split_Table
{

public:

	MeshClass* mc; // 0000
	MeshModelClass* mmc; // 0004
	bool npatch_enable; // 0008
	int polygon_count; // 000C
	Vector3i16* polygon_array; // 0010
	bool allocated_polygon_array; // 0014

	Vertex_Split_Table(MeshClass *Mesh);
	~Vertex_Split_Table()
	{
		if (allocated_polygon_array)
			delete[] polygon_array;
	}


	TextureClass* Peek_Texture(int polygonIndex, int pass, int textureIndex) const
	{
		if (!mmc->CurMatDesc->TextureArray[pass][textureIndex])
			return mmc->CurMatDesc->Texture[pass][textureIndex];
		else if (polygonIndex < mmc->PolyCount)
			return mmc->CurMatDesc->Peek_Texture(polygonIndex, pass, textureIndex);
		else
			return mmc->GapFiller->TextureArray[pass][textureIndex][polygonIndex - mmc->PolyCount];
	}


	VertexMaterialClass* Peek_Material(int polygonIndex, int pass) const
	{
		if (!mmc->CurMatDesc->MaterialArray[pass])
			return mmc->CurMatDesc->Material[pass];
		else if (polygonIndex < mmc->PolyCount)
			return mmc->CurMatDesc->Peek_Material(mmc->Poly->Get_Element(polygonIndex).I, pass);
		else
			return mmc->GapFiller->MaterialArray[pass][polygonIndex - mmc->PolyCount];
	}


	ShaderClass Peek_Shader(int polygonIndex, int pass) const
	{
		ShaderClass result;
		if (!mmc->CurMatDesc->ShaderArray[pass])
			result = mmc->CurMatDesc->Shader[pass];
		else if (polygonIndex < mmc->PolyCount)
			result = mmc->CurMatDesc->Peek_Shader(polygonIndex, pass);
		else
			result = mmc->GapFiller->ShaderArray[pass][polygonIndex - mmc->PolyCount];
		
		if (npatch_enable)
			result.Set_NPatch_Enable(ShaderClass::NPATCH_ENABLE);
		
		return result;
	}

};



class MatPassTaskClass : public AutoPoolClass<MatPassTaskClass,256> {
private:
	MaterialPassClass* MaterialPass;
	MeshClass* Mesh;
	MatPassTaskClass* NextVisible;
public:
	MatPassTaskClass(MaterialPassClass*,MeshClass*);
	~MatPassTaskClass();
	MaterialPassClass* Peek_Material_Pass()
	{
		return MaterialPass;
	}
	MeshClass* Peek_Mesh()
	{
		return Mesh;
	}
	MatPassTaskClass* Get_Next_Visible()
	{
		return NextVisible;
	}
	void Set_Next_Visible(MatPassTaskClass* pass)
	{
		NextVisible = pass;
	}
};



class DX8FVFCategoryContainer : public MultiListObjectClass {
public:
	MultiListClass<DX8TextureCategoryClass> texture_category_list[4]; // 0008
	MultiListClass<DX8TextureCategoryClass> visible_texture_category_list[4]; // 0068
	MatPassTaskClass* visible_matpass_head; // 00C8
	MatPassTaskClass* visible_matpass_tail; // 00CC
	RefCountPtr<IndexBufferClass> index_buffer; // 00D0
	int used_indices; // 00D4
	unsigned int FVF; // 00D8
	unsigned int passes; // 00DC
	unsigned int uv_coordinate_channels; // 00E0
	bool sorting; // 00E4
	bool AnythingToRender; // 00E5
	bool DelayedAnythingToRender; // 00E6

	DX8FVFCategoryContainer(unsigned int FVF_,bool sorting_);
	~DX8FVFCategoryContainer();
	virtual void Render() = 0;
	virtual void Add_Mesh(MeshClass* mesh) = 0;
	virtual void Log(bool only_visible) = 0;
	virtual bool Check_If_Mesh_Fits(MeshModelClass *mmc) = 0;
	virtual void Add_Delayed_Visible_Material_Pass(MaterialPassClass *,MeshClass *) = 0;
	virtual void Render_Delayed_Procedural_Material_Passes() = 0;
	void Generate_Texture_Categories(Vertex_Split_Table *split,unsigned int vertex_offset);
	void Render_Procedural_Material_Passes();
	void Add_Visible_Material_Pass(MaterialPassClass *,MeshClass *);
	static uint Define_FVF(MeshModelClass* model, uint* userLightingArray, bool enableLighting);
	void Remove_Texture_Category(DX8TextureCategoryClass* tex_category);
	void SHADERS_API Insert_To_Texture_Category(Vertex_Split_Table& split_table,TextureClass** texs,VertexMaterialClass* mat,ShaderClass shader,int pass,unsigned int vertex_offset);
}; // 00E8



class DX8SkinFVFCategoryContainer : public DX8FVFCategoryContainer {
public:
	unsigned int VisibleVertexCount;
	MeshClass *VisibleSkinHead;
	~DX8SkinFVFCategoryContainer();
	void SHADERS_API Render();
	void SHADERS_API Add_Mesh(MeshClass* mesh);
	void Log(bool only_visible);
	bool Check_If_Mesh_Fits(MeshModelClass *mmc);
	void SHADERS_API Add_Delayed_Visible_Material_Pass(MaterialPassClass *,MeshClass *);
	void Render_Delayed_Procedural_Material_Passes();
	void Add_Visible_Skin(MeshClass *);
	DX8SkinFVFCategoryContainer(bool sorting);
};



class DX8RigidFVFCategoryContainer : public DX8FVFCategoryContainer {
public:
	VertexBufferClass* vertex_buffer;
	int used_vertices;
	MatPassTaskClass* delayed_visible_matpass_head;
	MatPassTaskClass* delayed_visible_matpass_tail;
	~DX8RigidFVFCategoryContainer();
	void SHADERS_API Render();
	void SHADERS_API Add_Mesh(MeshClass* mesh);
	void Log(bool only_visible);
	bool Check_If_Mesh_Fits(MeshModelClass *mmc);
	void SHADERS_API Add_Delayed_Visible_Material_Pass(MaterialPassClass *,MeshClass *);
	void SHADERS_API Render_Delayed_Procedural_Material_Passes();
	DX8RigidFVFCategoryContainer(unsigned int FVF,bool sorting_);
};



#endif