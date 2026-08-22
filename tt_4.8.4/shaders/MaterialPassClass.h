#ifndef TT_INCLUDE__MATERIALPASSCLASS_H
#define TT_INCLUDE__MATERIALPASSCLASS_H
#include "engine_vector.h"
#include "engine_math.h"
#include "engine_common.h"
#include "engine_threading.h"
#include "engine_string.h"
#include "engine_3dre.h"
#include "texture.h"

class MaterialPassClass : public RefCountClass  {
public:
	TextureClass* Texture[2];
	ShaderClass Shader;
	VertexMaterialClass* Material;
	bool EnableOnTranslucentMeshes;
	bool EnableSortedRender;
	OBBoxClass* CullVolume;
	void Set_Texture(TextureClass* tex,int stage)
	{
		REF_PTR_SET(Texture[stage], tex);
	}
	void Set_Material(VertexMaterialClass* mat)
	{
		REF_PTR_SET(Material, mat);
	}
	MaterialPassClass()
	{
		Shader = ShaderClass(0);
		Material = 0;
		EnableOnTranslucentMeshes = true;
		CullVolume = 0;
		Texture[0] = 0;
		Texture[1] = 0;
	}
	~MaterialPassClass()
	{
		REF_PTR_RELEASE(Texture[0]);
		REF_PTR_RELEASE(Texture[1]);
		REF_PTR_RELEASE(Material);
	}
	void Set_Shader(ShaderClass shader)
	{
		Shader = shader;
	}
	TextureClass *Get_Texture(int stage = 0)
	{
		if (Texture[stage])
		{
			Texture[stage]->Add_Ref();
		}
		return Texture[stage];
	}
	TextureClass *Peek_Texture(int stage = 0)
	{
		return Texture[stage];
	}
	void Install_Materials();
};
#endif