#include "general.h"
#include "texture.h"
#include "WW3DAssetManager.h"
#include "HashTemplateIterator.h"
#include "shaderstatemanager.h"
#include "WW3D.h"
#include "dx8texman.h"
#include "missingtexture.h"
#include "TextureLoader.h"
void TextureClass::Init()
{
	if (!Initialized)
	{
		if (InactivationTime)
		{
			if (Time2)
			{
				if (WW3D::SyncTime - Time2 < InactivationTime)
				{
					Time1 = 3 * InactivationTime;
				}
				Time2 = 0;
			}
		}
		if (!D3DTexture)
		{
			if (WW3D::ThumbnailEnabled && MipLevelCount != 1)
			{
				WW3DFormat format = TextureFormat;
				D3DTexture = 0;
				TextureLoader::Request_Thumbnail(this);
				TextureFormat = format;
				Initialized = 0;
			}
			else
			{
				TextureLoader::Request_Foreground_Loading(this);
			}
		}
		if (!Initialized)
		{
			TextureLoader::Request_Background_Loading(this);
		}
		LastAccessed = WW3D::SyncTime;
	}
}

bool TextureClass::Is_Missing_Texture()
{
	bool result = false;
	IDirect3DTexture9 *tex = MissingTexture::_Get_Missing_Texture();
	if (tex == D3DTexture)
	{
		result = true;
	}
	tex->Release();
	return result;
}

unsigned int _MinTextureFilters[8];
unsigned int _MagTextureFilters[8];
unsigned int _MipMapFilters[8];

void TextureClass::_Init_Filters(TextureFilterMode& mode)
{
	_MinTextureFilters[FILTER_TYPE_NONE] =	D3DTEXF_POINT;
	_MagTextureFilters[FILTER_TYPE_NONE] =	D3DTEXF_POINT;
	_MipMapFilters[FILTER_TYPE_NONE] =		D3DTEXF_NONE;
	
	_MinTextureFilters[FILTER_TYPE_FAST] =	D3DTEXF_LINEAR;
	_MagTextureFilters[FILTER_TYPE_FAST] =	D3DTEXF_LINEAR;
	_MipMapFilters[FILTER_TYPE_FAST] =		D3DTEXF_POINT;
	
	_MinTextureFilters[FILTER_TYPE_BEST] =	D3DTEXF_LINEAR;
	_MagTextureFilters[FILTER_TYPE_BEST] =	D3DTEXF_LINEAR;
	_MipMapFilters[FILTER_TYPE_BEST] =		D3DTEXF_POINT;
	
	switch (mode)
	{
	case FILTER_MODE_ANISOTROPIC16X:	if (ShaderCaps::MaxAnisotropy < 16) mode = FILTER_MODE_ANISOTROPIC8X;
	case FILTER_MODE_ANISOTROPIC8X:		if (ShaderCaps::MaxAnisotropy < 8)  mode = FILTER_MODE_ANISOTROPIC4X;
	case FILTER_MODE_ANISOTROPIC4X:		if (ShaderCaps::MaxAnisotropy < 4)  mode = FILTER_MODE_ANISOTROPIC2X;
	case FILTER_MODE_ANISOTROPIC2X:		if (ShaderCaps::MaxAnisotropy < 2)  mode = FILTER_MODE_TRILINEAR;
	}

	switch (mode)
	{
	case FILTER_MODE_ANISOTROPIC2X:
	case FILTER_MODE_ANISOTROPIC4X:
	case FILTER_MODE_ANISOTROPIC8X:
	case FILTER_MODE_ANISOTROPIC16X:
		if (ShaderCaps::Caps.TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC)
		{
			_MinTextureFilters[FILTER_TYPE_BEST] = D3DTEXF_ANISOTROPIC;
		}
		if (ShaderCaps::Caps.TextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC)
		{
			_MagTextureFilters[FILTER_TYPE_BEST] = D3DTEXF_ANISOTROPIC;
		}
		//fall-through
	case FILTER_MODE_TRILINEAR:
		_MipMapFilters[FILTER_TYPE_BEST] = D3DTEXF_LINEAR;
	};

	_MinTextureFilters[FILTER_TYPE_DEFAULT] = _MinTextureFilters[FILTER_TYPE_BEST];
	_MagTextureFilters[FILTER_TYPE_DEFAULT] = _MagTextureFilters[FILTER_TYPE_BEST];
	_MipMapFilters[FILTER_TYPE_DEFAULT] = _MipMapFilters[FILTER_TYPE_BEST];

	memcpy(_MinTextureFilters + FILTER_TYPE_COUNT, _MinTextureFilters, sizeof(uint) * FILTER_TYPE_COUNT);
	memcpy(_MagTextureFilters + FILTER_TYPE_COUNT, _MagTextureFilters, sizeof(uint) * FILTER_TYPE_COUNT);
	memcpy(_MipMapFilters + FILTER_TYPE_COUNT, _MipMapFilters, sizeof(uint) * FILTER_TYPE_COUNT);
	
	if (mode != FILTER_MODE_TRILINEAR && mode != FILTER_MODE_BILINEAR)
	{
		int level = 1 << (mode - 1);
		StateManager::SetSamplerState(0, D3DSAMP_MAXANISOTROPY, level);
		StateManager::SetSamplerState(1, D3DSAMP_MAXANISOTROPY, level);
	};
};


IDirect3DSurface9* TextureClass::Get_D3D_Surface_Level(uint level)
{
	if (D3DTexture)
	{
		IDirect3DSurface9 *surface = 0;
		D3DTexture->GetSurfaceLevel(level,&surface);
		return surface;
	}
	return 0;
}

SurfaceClass::SurfaceClass(unsigned int width,unsigned int height,WW3DFormat format): RefCountClass()
{
	//Add_Ref();
	SurfaceFormat = format;
	D3DSurface = DX8Wrapper::Create_DX8_Surface(width,height,format);
}

void SurfaceClass::Get_Description(SurfaceDescription& surface_desc)
{
	D3DSURFACE_DESC d3d_desc;
	D3DSurface->GetDesc(&d3d_desc);
	surface_desc.Height = d3d_desc.Height;
	surface_desc.Width = d3d_desc.Width;
	surface_desc.Format = SurfaceFormat; //should really use D3DFormat_To_WW3DFormat here
}

void *SurfaceClass::Lock(int* pitch)
{
	D3DLOCKED_RECT r;
	r.Pitch = 0;
	r.pBits = 0;
	D3DSurface->LockRect(&r,NULL,0);
	*pitch = r.Pitch;
	return r.pBits;
}

void *SurfaceClass::Lock_ReadOnly(int* pitch)
{
	D3DLOCKED_RECT r;
	r.Pitch = 0;
	r.pBits = 0;
	D3DSurface->LockRect(&r,NULL,D3DLOCK_READONLY);
	*pitch = r.Pitch;
	return r.pBits;
}

IDirect3DSurface9 *SurfaceClass::Peek_D3D_Surface()
{
	return D3DSurface;
}

WW3DFormat SurfaceClass::Get_Surface_Format()
{
	return SurfaceFormat;
}

SurfaceClass::~SurfaceClass()
{
	SafeRelease(D3DSurface);
}
SurfaceClass::SurfaceClass(IDirect3DSurface9 *d3d_surface)
{
	D3DSurface = d3d_surface;
	if (d3d_surface)
	{
		d3d_surface->AddRef();
	}
	D3DSURFACE_DESC desc;
	memset(&desc,0,sizeof(desc));
	d3d_surface->GetDesc(&desc);
	SurfaceFormat = D3DFormat_To_WW3DFormat(desc.Format);
}

TextureClass* _stdcall Load_Texture(const char *path, TextureClass::MipCountType mip, WW3DFormat format, bool IsCompressionAllowed)
{
	return WW3DAssetManager::TheInstance->Get_Texture(path, mip, format, IsCompressionAllowed);
}

void _stdcall Free_Texture(TextureClass *texture)
{
	WW3DAssetManager::TheInstance->Release_Texture(texture);
}

void TextureClass::Invalidate_Old_Unused_Textures(int textureLifeTime)
{
	unsigned int syncTime = WW3D::SyncTime;

	for (HashTemplateIterator<StringClass, TextureClass*> textureIter(WW3DAssetManager::TheInstance->TextureHash); textureIter; ++textureIter)
	{
		TextureClass* texture = textureIter.getValue();
		if (texture->Initialized && texture->InactivationTime != 0)
		{
			if (syncTime - texture->LastAccessed > (textureLifeTime ? textureLifeTime : texture->Time1 + texture->InactivationTime))
			{
				texture->Invalidate();
				texture->Time2 = syncTime;
			}
		}
	}
}

void TextureClass::Invalidate()
{
	if(!ThumbnailTextureLoadTask && !NormalTextureLoadTask && !IsProcedural)
	{
		SafeRelease(D3DTexture);
		this->Initialized = false;
		this->LastAccessed = WW3D::SyncTime;
	}
}

REF_DEF1(unused_texture_id, unsigned int, 0x008315F4);

TextureClass::TextureClass(unsigned int width,unsigned int height,WW3DFormat format,MipCountType mip_level_count,PoolType pool,bool rendertarget) : RefCountClass(), TextureMinFilter(FILTER_TYPE_DEFAULT), TextureMagFilter(FILTER_TYPE_DEFAULT), MipMapFilter((FilterType)(-(mip_level_count != 0) & 3)), UAddressMode(TEXTURE_ADDRESS_REPEAT), VAddressMode(TEXTURE_ADDRESS_REPEAT), D3DTexture(0), Initialized(true), Name(), FullPath(), Height(height), IsLightmap(false), IsProcedural(true), IsCompressionAllowed(false), InactivationTime(0), Time1(0), Time2(0), TextureFormat(format), Width(width), Pool(pool), Dirty(false), MipLevelCount(mip_level_count), ThumbnailTextureLoadTask(0), NormalTextureLoadTask(0)
{
	texture_id = unused_texture_id++;
	if (format >= 20)
	{
		if (format <= 24)
		{
			IsCompressionAllowed = true;
		}
	}
	
	D3DPOOL d3dpool;
	switch(pool) 
	{
		case POOL_DEFAULT: d3dpool = D3DPOOL_DEFAULT; break;
		case POOL_MANAGED: d3dpool = D3DPOOL_MANAGED; break;
		case POOL_SYSTEMMEM: d3dpool = D3DPOOL_SYSTEMMEM; break;
		DEFAULT_UNREACHABLE;
	}

	D3DTexture = DX8Wrapper::Create_DX8_Texture(width, height, format, mip_level_count, d3dpool, rendertarget);
	if (!pool)
	{
		Dirty = true;
		DX8TextureTrackerClass *tracker = new DX8TextureTrackerClass();
		tracker->Width = width;
		tracker->Height = height;
		tracker->Format = format;
		tracker->Mip_level_count = mip_level_count;
		tracker->RenderTarget = rendertarget;
		tracker->Texture = this;
		DX8TextureManagerClass::Add(tracker);
	}
	LastAccessed = WW3D::SyncTime;
}

TextureClass::~TextureClass()
{
	SAFE_DELETE(ThumbnailTextureLoadTask);
	SAFE_DELETE(NormalTextureLoadTask);
	if (D3DTexture)
	{
		D3DTexture->Release();
		D3DTexture = 0;
	}
	DX8TextureManagerClass::Remove(this);
}

SurfaceClass* TextureClass::Get_Surface_Level(unsigned int level)
{
	if (!D3DTexture)
	{
		return 0;
	}
	IDirect3DSurface9 *surface9 = 0;
	D3DTexture->GetSurfaceLevel(level,&surface9);
	SurfaceClass *surf = new SurfaceClass(surface9);
	surface9->Release();
	return surf;
}

TextureClass::TextureClass(SurfaceClass *surface,MipCountType mip_level_count) : TextureMinFilter(FILTER_TYPE_DEFAULT), TextureMagFilter(FILTER_TYPE_DEFAULT), UAddressMode(TEXTURE_ADDRESS_REPEAT), VAddressMode(TEXTURE_ADDRESS_REPEAT), D3DTexture(0), Initialized(true), Name(0), FullPath(0), IsLightmap(false), IsProcedural(true), IsCompressionAllowed(false), InactivationTime(0), Time1(0), Time2(0), TextureFormat(surface->Get_Surface_Format()), Width(0), Height(0), Pool(POOL_MANAGED), Dirty(false), MipLevelCount(mip_level_count), ThumbnailTextureLoadTask(0), NormalTextureLoadTask(0)
{
	MipMapFilter = (FilterType)(-((mip_level_count - 1) != 0) & 3);
	texture_id = unused_texture_id++;
	SurfaceClass::SurfaceDescription desc;
	surface->Get_Description(desc);
	Width = desc.Width;
	Height = desc.Height;
	if ((desc.Format - 20) <= 4)
		IsCompressionAllowed = true;
	
	D3DTexture = DX8Wrapper::_Create_DX8_Texture(surface->D3DSurface,mip_level_count);
	LastAccessed = WW3D::SyncTime;
}

void TextureClass::Set_Texture_Name(char* name)
{
	Name = name;
}

unsigned int TextureClass::Get_Reduction()
{
	if (MipLevelCount == MIP_LEVELS_1)
	{
		return 0;
	}
	else
	{
		int res = WW3D::Get_Texture_Reduction();
		if (MipLevelCount)
		{
			if (res > MipLevelCount)
			{
				res = MipLevelCount;
			}
		}
		return res;
	}
}

void SurfaceClass::Attach(IDirect3DSurface9 *surface)
{
	Detatch();
	D3DSurface = surface;
}

void SurfaceClass::Detatch()
{
	D3DSurface = 0;
}

void TextureClass::Apply_New_Surface(IDirect3DTexture9 *texture,bool initialized)
{
	if (D3DTexture)
	{
		D3DTexture->Release();
	}
	D3DTexture = texture;
	if (texture)
	{
		texture->AddRef();
	}
	if (initialized)
	{
		Initialized = true;
	}
	D3DSURFACE_DESC desc;
	D3DTexture->GetLevelDesc(0,&desc);
	TextureFormat = D3DFormat_To_WW3DFormat(desc.Format);
	Width = desc.Width;
	Height = desc.Height;
}

void TextureClass::Load_Locked_Surface()
{
	D3DTexture = 0;
	TextureLoader::Request_Thumbnail(this);
	Initialized = false;
}

TextureClass::TextureClass(char const*name, char const*full_path,MipCountType mip_level_count,WW3DFormat texture_format,bool allow_compression) : TextureMinFilter(FILTER_TYPE_DEFAULT), TextureMagFilter(FILTER_TYPE_DEFAULT), MipMapFilter((FilterType)(3 * (mip_level_count != 1))), UAddressMode(TEXTURE_ADDRESS_REPEAT), VAddressMode(TEXTURE_ADDRESS_REPEAT), D3DTexture(0), Initialized(false), Name(0), FullPath(0), IsLightmap(false), IsProcedural(false), IsCompressionAllowed(allow_compression), InactivationTime(20000), Time1(0), Time2(0), TextureFormat(texture_format), Width(0), Height(0), Pool(POOL_MANAGED), Dirty(false), MipLevelCount(mip_level_count), ThumbnailTextureLoadTask(0), NormalTextureLoadTask(0)
{
	texture_id = unused_texture_id;
	unused_texture_id++;
	if (texture_format >= WW3D_FORMAT_U8V8)
	{
		if (texture_format <= WW3D_FORMAT_X8L8V8U8)
		{
			if (DX8Wrapper::IsInitted && ShaderCaps::SupportTextureFormat[TextureFormat])
			{
				IsCompressionAllowed = false;
				MipLevelCount = MIP_LEVELS_1;
				MipMapFilter = FILTER_TYPE_NONE;
			}
			else
			{
				TextureFormat = WW3D_FORMAT_UNKNOWN;
			}
		}
		else
		{
			if (texture_format <= WW3D_FORMAT_DXT5)
			{
				IsCompressionAllowed = true;
			}
		}
	}
	if (strchr(name,'+'))
	{
		IsLightmap = true;
		TextureMinFilter = FILTER_TYPE_FAST;
		TextureMagFilter = FILTER_TYPE_FAST;
		if (mip_level_count != 1)
		{
			MipMapFilter = FILTER_TYPE_FAST;
		}
	}
	Set_Texture_Name((char *)name);
	FullPath = full_path;
	if (!WW3D::IsTexturingEnabled)
	{
		Initialized = true;
		D3DTexture = 0;
	}
	ThumbnailClass *thumbnail;
	StringClass *str = &Name;
	if (!(FullPath.Is_Empty()))
	{
		str = &FullPath;
	}
	for (ThumbnailManagerClass *thumbnailmgr = ThumbnailManagerClass::ThumbnailManagerList.Head();thumbnailmgr;thumbnailmgr = thumbnailmgr->Succ())
	{
		thumbnail = thumbnailmgr->Peek_Thumbnail_Instance(*str);
		if (thumbnail)
		{
			Width = thumbnail->Width;
			Height = thumbnail->Height;
			break;
		}
	}
	LastAccessed = WW3D::SyncTime;
	if (!WW3D::ThumbnailEnabled)
	{
		if (TextureLoader::Is_DX8_Thread())
		{
			Init();
		}
	}
}

int Calculate_Texture_Memory_Usage(TextureClass const *texture,int red_factor)
{
	TT_UNREACHABLE;
}

unsigned int TextureClass::Get_Texture_Memory_Usage()
{
	return Calculate_Texture_Memory_Usage(this,0);
}

int TextureClass::_Get_Total_Locked_Surface_Size()
{
	TT_UNREACHABLE;
}

int TextureClass::_Get_Total_Texture_Size()
{
	TT_UNREACHABLE;
}

int TextureClass::_Get_Total_Lightmap_Texture_Size()
{
	TT_UNREACHABLE;
}

int TextureClass::_Get_Total_Procedural_Texture_Size()
{
	TT_UNREACHABLE;
}

int TextureClass::_Get_Total_Locked_Surface_Count()
{
	TT_UNREACHABLE;
}

int TextureClass::_Get_Total_Texture_Count()
{
	TT_UNREACHABLE;
}

int TextureClass::_Get_Total_Lightmap_Texture_Count()
{
	TT_UNREACHABLE;
}

int TextureClass::_Get_Total_Procedural_Texture_Count()
{
	TT_UNREACHABLE;
}

SurfaceClass::SurfaceClass(const char *name)
{
	D3DSurface = DX8Wrapper::_Create_DX8_Surface(name);
	D3DSURFACE_DESC desc;
	D3DSurface->GetDesc(&desc);
	SurfaceFormat = D3DFormat_To_WW3DFormat(desc.Format);
}

unsigned char Alpha_Bits(WW3DFormat format)
{
	switch (format)
	{
		case WW3D_FORMAT_A8R8G8B8:
		case WW3D_FORMAT_A8:
		case WW3D_FORMAT_A8R3G3B2:
		case WW3D_FORMAT_A8P8:
		case WW3D_FORMAT_A8L8:
			return 8;
		case WW3D_FORMAT_A4R4G4B4:
		case WW3D_FORMAT_A4L4:
		case WW3D_FORMAT_DXT3:
		case WW3D_FORMAT_DXT4:
		case WW3D_FORMAT_DXT5:
			return 4;
		case WW3D_FORMAT_A1R5G5B5:
		case WW3D_FORMAT_DXT2:
			return 1;
		default:
			return 0;
	}
}

int SurfaceClass::PixelSize(SurfaceDescription const& sd)
{
	switch ( sd.Format )
	{
		case WW3D_FORMAT_UNKNOWN:
		default:
			return 0;
		case WW3D_FORMAT_R8G8B8:
			return 3;
		case WW3D_FORMAT_A8R8G8B8:
		case WW3D_FORMAT_X8R8G8B8:
			return 4;
		case WW3D_FORMAT_R5G6B5:
		case WW3D_FORMAT_X1R5G5B5:
		case WW3D_FORMAT_A1R5G5B5:
		case WW3D_FORMAT_A4R4G4B4:
		case WW3D_FORMAT_A8R3G3B2:
		case WW3D_FORMAT_X4R4G4B4:
		case WW3D_FORMAT_A8P8:
		case WW3D_FORMAT_A8L8:
			return 2;
		case WW3D_FORMAT_R3G3B2:
		case WW3D_FORMAT_A8:
		case WW3D_FORMAT_P8:
		case WW3D_FORMAT_L8:
		case WW3D_FORMAT_A4L4:
			return 1;
	}
}

void SurfaceClass::FindBB(Vector2i* min,Vector2i* max)
{
	SurfaceDescription sd;
	Get_Description(sd);
	unsigned char alpha = Alpha_Bits(sd.Format);
	unsigned char alpha2 = 0;
	if (alpha == 4)
	{
		alpha2 = 15;
	}
	else
	{
		if (alpha > 4)
		{
			if (alpha == 8)
			{
				alpha2 = 0xFF;
			}
		}
		else
		{
			if (alpha == 1)
			{
				alpha2 = 1;
			}
		}
	}
	D3DLOCKED_RECT lock_rect;
	RECT rect;
	rect.left = min->I;
	rect.top = min->J;
	rect.right = max->I;
	rect.bottom = max->J;
	lock_rect.pBits = 0;
	lock_rect.Pitch = 0;
	D3DSurface->LockRect(&lock_rect,&rect,D3DLOCK_READONLY);
	int pixelsize = PixelSize(sd);
	int newmini = max->I;
	int newminj = max->J;
	int newmaxi = min->I;
	int newmaxj = min->J;
	int pitch = min->J * lock_rect.Pitch;
	for (int j = min->J;j < max->J;j++)
	{
		int psz = min->I * pixelsize;
		for (int i = min->I;i < max->I;i++)
		{
			int p1 = (pixelsize * min->I);
			int p2 = (psz - p1);
			int p3 = (pixelsize + p2);
			int pitchx = (pitch - (min->J * lock_rect.Pitch));
			int p4 = (pitchx + p3);
			unsigned char *p5 = ((unsigned char *)lock_rect.pBits);
			unsigned char invalpha = (8 - alpha);
			if (alpha2 & (*(p4 + p5 - 1) >> (invalpha)))
			{
				if (newmini >= i)
				{
					newmini = i;
				}
				if (newmaxi <= i)
				{
					newmaxi = i;
				}
				if (newminj >= j)
				{
					newminj = j;
				}
				if (newmaxj <= j)
				{
					newmaxj = j;
				}
			}
			psz += pixelsize;
		}
		pitch += lock_rect.Pitch;
	}
	D3DSurface->UnlockRect();
	max->I = newmaxi;
	max->J = newmaxj;
	min->I = newmini;
	min->J = newminj;
}

bool SurfaceClass::Is_Transparent_Column(unsigned int column)
{
	SurfaceDescription sd;
	Get_Description(sd);
	int alpha = Alpha_Bits(sd.Format);
	int alpha2 = 0;
	if (alpha == 4)
	{
		alpha2 = 15;
	}
	else
	{
		if (alpha > 4)
		{
			if (alpha == 8)
			{
				alpha2 = -1;
			}
		}
		else
		{
			if (alpha == 1)
			{
				alpha2 = 1;
			}
		}
	}
	int pixelsize = PixelSize(sd);
	_D3DLOCKED_RECT lock_rect;
	RECT rect;
	lock_rect.Pitch = 0;
	lock_rect.pBits = 0;
	rect.bottom = sd.Height;
	rect.top = 0;
	rect.left = column;
	rect.right = column + 1;
	D3DSurface->LockRect(&lock_rect,&rect,D3DLOCK_READONLY);
	unsigned char *pos;
	unsigned int i = 0;
	if (sd.Height <= 0)
	{
		D3DSurface->UnlockRect();
		return true;
	}
	else
	{
		pos = ((unsigned char *)lock_rect.pBits + pixelsize - 1);
		while (!(alpha2 & (unsigned char)(*pos >> (8 - alpha))))
		{
			i++;
			pos += lock_rect.Pitch;
			if (i >= sd.Height)
			{
				D3DSurface->UnlockRect();
				return true;
			}
		}
		D3DSurface->UnlockRect();
		return false;
	}
}
