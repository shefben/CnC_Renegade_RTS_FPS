#ifndef TT_INCLUDE__TEXTURE_H
#define TT_INCLUDE__TEXTURE_H
#include "engine_3dre.h"
#include "render2d.h"
class TextureLoadTaskClass;


extern unsigned int _MinTextureFilters[8];
extern unsigned int _MagTextureFilters[8];
extern unsigned int _MipMapFilters[8];

class TextureClass: public RefCountClass {
public:
	enum MipCountType
	{
		MIP_LEVELS_ALL = 0x0,
		MIP_LEVELS_1 = 0x1,
		MIP_LEVELS_2 = 0x2,
		MIP_LEVELS_3 = 0x3,
		MIP_LEVELS_4 = 0x4,
		MIP_LEVELS_5 = 0x5,
		MIP_LEVELS_6 = 0x6,
		MIP_LEVELS_7 = 0x7,
		MIP_LEVELS_8 = 0x8,
		MIP_LEVELS_10 = 0x9,
		MIP_LEVELS_11 = 0xa,
		MIP_LEVELS_12 = 0xb,
		MIP_LEVELS_MAX = 0xc,
	};
	enum PoolType
	{
		POOL_DEFAULT = 0x0,
		POOL_MANAGED = 0x1,
		POOL_SYSTEMMEM = 0x2,
	};
	enum FilterType {
		FILTER_TYPE_NONE = 0x0,
		FILTER_TYPE_FAST = 0x1,
		FILTER_TYPE_BEST = 0x2,
		FILTER_TYPE_DEFAULT = 0x3,
		FILTER_TYPE_COUNT = 0x4,
	};
	enum TextureFilterMode
	{
		FILTER_MODE_BILINEAR		= 0x0,
		FILTER_MODE_TRILINEAR		= 0x1,
		FILTER_MODE_ANISOTROPIC2X	= 0x2,
		FILTER_MODE_ANISOTROPIC4X	= 0x3,
		FILTER_MODE_ANISOTROPIC8X	= 0x4,
		FILTER_MODE_ANISOTROPIC16X	= 0x5,
	};
	enum TxtAddrMode {
		TEXTURE_ADDRESS_REPEAT = 0x0,
		TEXTURE_ADDRESS_CLAMP = 0x1,
	};
	FilterType TextureMinFilter;//008
	FilterType TextureMagFilter;//00C
	FilterType MipMapFilter;//010
	TxtAddrMode UAddressMode;//014
	TxtAddrMode VAddressMode;//018
	IDirect3DTexture9 *D3DTexture;//01C
	bool Initialized;//020
	StringClass Name;//024
	StringClass FullPath;//028
	unsigned int texture_id;//02C
	bool IsLightmap;//030
	bool IsProcedural;//031
	bool IsCompressionAllowed;//032
	unsigned int InactivationTime;//034
	unsigned int Time1;//038
	unsigned int Time2;//03C
	unsigned int LastAccessed;//040
	WW3DFormat TextureFormat;//044
	unsigned int Width;//048
	unsigned int Height;//04C
	PoolType Pool;//050
	bool Dirty;//054
	MipCountType MipLevelCount;//058
	TextureLoadTaskClass *ThumbnailTextureLoadTask;//05C
	TextureLoadTaskClass *NormalTextureLoadTask; //060
	virtual ~TextureClass();
	SHADERS_API void Init();
	bool Is_Missing_Texture();
	IDirect3DSurface9* Get_D3D_Surface_Level(uint level);
	SurfaceClass* Get_Surface_Level(unsigned int level);
	SHADERS_API static void Invalidate_Old_Unused_Textures(int textureLifeTime);
	void Invalidate();
	TextureClass(unsigned int width,unsigned int height,WW3DFormat format,MipCountType mip_level_count,PoolType pool,bool rendertarget);
	TextureClass(SurfaceClass *surface,MipCountType mip_level_count);
	TextureClass(char const*name, char const*full_path,MipCountType mip_level_count,WW3DFormat texture_format,bool allow_compression);
	static void SHADERS_API _Init_Filters(TextureFilterMode& mode);
	void Set_Texture_Name(char* name);
	unsigned int Get_Reduction();
	void Apply_New_Surface(IDirect3DTexture9 *texture,bool initialized);
	void Load_Locked_Surface();
	unsigned int Get_Texture_Memory_Usage();
	static int _Get_Total_Locked_Surface_Size();
	static int _Get_Total_Texture_Size();
	static int _Get_Total_Lightmap_Texture_Size();
	static int _Get_Total_Procedural_Texture_Size();
	static int _Get_Total_Locked_Surface_Count();
	static int _Get_Total_Texture_Count();
	static int _Get_Total_Lightmap_Texture_Count();
	static int _Get_Total_Procedural_Texture_Count(); 
};

class SurfaceClass : public RefCountClass {
public:
	IDirect3DSurface9 *D3DSurface;
	WW3DFormat SurfaceFormat;
	struct SurfaceDescription {
		WW3DFormat Format;
		unsigned int Width;
		unsigned int Height;
	};
	SurfaceClass(unsigned int width,unsigned int height,WW3DFormat format);
	SurfaceClass(IDirect3DSurface9 *d3d_surface);
	SurfaceClass(const char *name);
	void FindBB(Vector2i* min,Vector2i* max);
	bool Is_Transparent_Column(unsigned int column);
	void Get_Description(SurfaceDescription& surface_desc);
	void *Lock(int* pitch);
	void *Lock_ReadOnly(int* pitch);
	void Unlock();
	IDirect3DSurface9 *Peek_D3D_Surface();
	WW3DFormat Get_Surface_Format();
	~SurfaceClass();
	void Attach(IDirect3DSurface9 *surface);
	void Detatch();
	int PixelSize(SurfaceDescription const& sd);
};

TextureClass *_stdcall Load_Texture(const char *path,TextureClass::MipCountType mip,WW3DFormat format, bool IsCompressionAllowed); //Load a texture
void _stdcall Free_Texture(TextureClass *texture); //Free a texture
#endif
