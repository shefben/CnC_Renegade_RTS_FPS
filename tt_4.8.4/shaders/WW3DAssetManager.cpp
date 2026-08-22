#include "General.h"
#include "WW3DAssetManager.h"

#include "RefCountPtr.h"
#include "texture.h"



REF_DEF2(WW3DAssetManager::TheInstance, WW3DAssetManager*, 0x00830720, 0x0082F908);

TextureClass* WW3DAssetManager::Real_Get_Texture(const char* textureName, TextureClass::MipCountType mipLevelCount, WW3DFormat ww3dFormat, bool allowCompression)
{
	if (ww3dFormat == WW3D_FORMAT_U8V8)
		mipLevelCount = TextureClass::MIP_LEVELS_1;
	
	if (textureName && textureName != '\0')
	{
		StringClass lowerTextureName = textureName;
		lowerTextureName.toLower();
		
		TextureClass** _texture = TextureHash.Get(lowerTextureName);
		RefCountPtr<TextureClass> texture = Create_Peek(_texture ? *_texture : NULL);
		if (!texture)
		{
			texture = Create_NEW(new TextureClass(lowerTextureName, NULL, mipLevelCount, ww3dFormat, allowCompression));
			TextureHash.Insert(lowerTextureName, texture.Get());
		}
		return texture.Get();
	}
	else
		return NULL;
}
