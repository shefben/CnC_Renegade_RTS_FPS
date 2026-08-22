#ifndef TT_INCLUDE_MISSINGTEXTURE_H
#define TT_INCLUDE_MISSINGTEXTURE_H
#include "engine_3d.h"
class MissingTexture {
public:
	static void _Init();
	static void _Deinit();
	static SHADERS_API IDirect3DTexture9 *_Get_Missing_Texture();
	static SHADERS_API IDirect3DSurface9 *_Create_Missing_Surface();
};
#endif