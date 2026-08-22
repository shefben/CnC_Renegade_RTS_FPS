#ifndef TT_INCLUDE_DX8TEXMAN_H
#define TT_INCLUDE_DX8TEXMAN_H
#include "multilist.h"
#include "engine_3dre.h"
#include "texture.h"
class DX8TextureTrackerClass : public MultiListObjectClass  {
public:
	unsigned int Width;
	unsigned int Height;
	WW3DFormat Format;
	TextureClass::MipCountType Mip_level_count;
	bool RenderTarget;
	TextureClass* Texture;
	~DX8TextureTrackerClass() {}
};
class DX8TextureManagerClass {
public:
	static void Add(DX8TextureTrackerClass* track);
	static void Remove(TextureClass* tex);
	static void Release_Textures();
	static void Recreate_Textures();
	static MultiListClass<DX8TextureTrackerClass> Managed_Textures;
	static void Shutdown();
};
#endif
