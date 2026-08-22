#include "General.h"
#include "engine_3dre.h"
#include "textureloader.h"
#include "texture.h"
#include "missingtexture.h"
#include "DX8Wrapper.h"
#include "ThreadClass.h"
#include "HashTemplateIterator.h"
#include "CriticalSectionClass.h"
#include "WW3D.h"
#include "SimpleFileFactoryClass.h"
#include "MixFileFactoryClass.h"
#include "BufferedFileClass.h"
#include "DDSFile.h"

SynchronizedTextureLoadTaskListClass ForegroundQueue;
SynchronizedTextureLoadTaskListClass BackgroundQueue;
FastCriticalSectionClass BackgroundCriticalSection;
FastCriticalSectionClass ForegroundCriticalSection;
DLListClass<ThumbnailManagerClass> ThumbnailManagerClass::ThumbnailManagerList;
REF_DEF1(TheWritingFileFactory, FileFactoryClass*, 0x00809E7C);

REF_DEF1(_TextureLoadThread, LoaderThreadClass, 0x008304F0);

void TextureLoader::Init()
{
	ThumbnailManagerClass::Init();
	_TextureLoadThread.Execute();
	_TextureLoadThread.Set_Priority(-4);
}

void TextureLoader::Deinit()
{
	FastCriticalSectionClass::LockClass lock(BackgroundCriticalSection);
	_TextureLoadThread.Stop(3000);
	ThumbnailManagerClass::Deinit();
	TextureLoadTaskClass::Delete_Free_Pool();
}

bool TextureLoader::Is_DX8_Thread()
{
	return ThreadClass::Get_Current_Thread_ID() == DX8Wrapper::_MainThreadID;
}

bool TextureLoader::TextureLoadSuspended;

void TextureLoader::Validate_Texture_Size(uint&width, uint&height)
{
	unsigned int i,j;
	for (i = 1;i < width;i *= 2)
	{
	}
	for (j = 1;j < height;j *= 2)
	{
	}
	if (i > ShaderCaps::MaxTextureWidth)
	{
		i = ShaderCaps::MaxTextureWidth;
	}
	if (j > ShaderCaps::MaxTextureHeight)
	{
		j = ShaderCaps::MaxTextureHeight;
	}
	if (i <= j)
	{
		for (;j / i > 8;i *= 2)
		{
		}
	}
	else
	{
		for (;i / j > 8;j *= 2)
		{
		}
	}
	width = i;
	height = j;
}

WW3DFormat Get_Valid_Texture_Format(WW3DFormat format,bool is_compression_allowed)
{
	WW3DFormat outformat = format;
	if (ShaderCaps::SupportDXTC && is_compression_allowed)
	{
		if (format == WW3D_FORMAT_DXT1)
		{
			if (!ShaderCaps::SupportTextureFormat[WW3D_FORMAT_DXT1])
			{
				if (ShaderCaps::SupportTextureFormat[WW3D_FORMAT_DXT5])
				{
					outformat = WW3D_FORMAT_DXT5;
				}
			}
		}
		else
		{
			if (format >= WW3D_FORMAT_DXT1 && format <= WW3D_FORMAT_DXT5 && !ShaderCaps::SupportTextureFormat[format])
			{
				outformat = WW3D_FORMAT_A8R8G8B8; // No support yet.
			}
		}
	}
	else
	{
		if (format == WW3D_FORMAT_DXT1)
		{
			outformat = WW3D_FORMAT_R8G8B8;
		}
		else
		{
			if (format >= WW3D_FORMAT_DXT1 && format <= WW3D_FORMAT_DXT5)
			{
				outformat = WW3D_FORMAT_A8R8G8B8; // See above comment
			}
		}
	}
	if (outformat == WW3D_FORMAT_R8G8B8)
	{
		outformat = WW3D_FORMAT_X8R8G8B8;
	}
	int depth = WW3D::Get_Texture_Bitdepth();
	if (depth > 16)
	{
		if (!ShaderCaps::SupportTextureFormat[outformat])
		{
			outformat = WW3D_FORMAT_A8R8G8B8;
			if (!ShaderCaps::SupportTextureFormat[WW3D_FORMAT_A8R8G8B8])
			{
				outformat = WW3D_FORMAT_A4R4G4B4;
				if (!ShaderCaps::SupportTextureFormat[WW3D_FORMAT_A4R4G4B4])
				{
					outformat = WW3D_FORMAT_X8R8G8B8;
					if (!ShaderCaps::SupportTextureFormat[WW3D_FORMAT_X8R8G8B8])
					{
						outformat = WW3D_FORMAT_R5G6B5;
					}
				}
			}
		}
	}
	else
	{
		switch(outformat)
		{
		case WW3D_FORMAT_A8R8G8B8:
			outformat = WW3D_FORMAT_A4R4G4B4;
			break;
		case WW3D_FORMAT_R8G8B8:
		case WW3D_FORMAT_X8R8G8B8:
			outformat = WW3D_FORMAT_R5G6B5;
			break;
		default:
			if (!ShaderCaps::SupportTextureFormat[outformat])
			{
				outformat = WW3D_FORMAT_A8R8G8B8;
				if (!ShaderCaps::SupportTextureFormat[WW3D_FORMAT_A8R8G8B8])
				{
					outformat = WW3D_FORMAT_A4R4G4B4;
					if (!ShaderCaps::SupportTextureFormat[WW3D_FORMAT_A4R4G4B4])
					{
						outformat = WW3D_FORMAT_X8R8G8B8;
						if (!ShaderCaps::SupportTextureFormat[WW3D_FORMAT_X8R8G8B8])
						{
							outformat = WW3D_FORMAT_R5G6B5;
						}
					}
				}
			}
			break;
		}
	}
	return outformat;
}

bool Is_DXTC_Format(WW3DFormat texture_format)
{
	return (texture_format >= WW3D_FORMAT_DXT1) && (texture_format <= WW3D_FORMAT_DXT5);
};

char* ThumbFileHeader = "THU6";

IDirect3DTexture9 *TextureLoader::Load_Thumbnail(const StringClass&texture)
{
	ThumbnailClass *thumbnail = 0;
	for (ThumbnailManagerClass *thumbnailmgr = ThumbnailManagerClass::ThumbnailManagerList.Head();thumbnailmgr;thumbnailmgr = thumbnailmgr->Succ())
	{
		thumbnail = thumbnailmgr->Peek_Thumbnail_Instance(texture);
		if (thumbnail)
		{
			break;
		}
	}
	if (!thumbnail)
	{
		return MissingTexture::_Get_Missing_Texture();
	}

	if (Is_DXTC_Format(thumbnail->ThumbnailFormat))
	{
		IDirect3DTexture9* tex = DX8Wrapper::Create_DX8_Texture(thumbnail->Width, thumbnail->Height, thumbnail->ThumbnailFormat, TextureClass::MIP_LEVELS_1, D3DPOOL_MANAGED, false);
		D3DLOCKED_RECT rect = {};
		tex->LockRect(0, &rect, NULL, 0);
		memcpy(rect.pBits, thumbnail->ThumbnailBits, DDSFile::GetDXTCSurfaceSize(thumbnail->Width, thumbnail->Height, thumbnail->Format));
		tex->UnlockRect(0);
		return tex;
	};

	int pitch = 2 * thumbnail->Width;
	WW3DFormat fmt = Get_Valid_Texture_Format(WW3D_FORMAT_A4R4G4B4,0);
	IDirect3DTexture9 *d3dtexture = DX8Wrapper::Create_DX8_Texture(thumbnail->Width, thumbnail->Height, fmt, TextureClass::MIP_LEVELS_ALL,D3DPOOL_MANAGED,false);
	unsigned int LevelCount = d3dtexture->GetLevelCount();
	D3DLOCKED_RECT rects[12];
	int width = thumbnail->Width;
	int height = thumbnail->Height;
	for (unsigned int i = 0;i < LevelCount;i++)
	{
		d3dtexture->LockRect(i,&rects[i],0,0);
	}
	for (unsigned int i = 0;i < LevelCount - 1;i++)
	{
		BitmapHandlerClass::Copy_Image_Generate_Mipmap(width,height,(unsigned char *)rects[i].pBits,rects[i].Pitch,fmt,thumbnail->ThumbnailBits,pitch,WW3D_FORMAT_A4R4G4B4,(unsigned char *)rects[i+1].pBits,rects[i+1].Pitch);
		width >>= 1;
		height >>= 1;
	}
	for (unsigned int i = 0;i < LevelCount;i++)
	{
		d3dtexture->UnlockRect(i);
	}
	return d3dtexture;
}

bool Is_Format_Compressed(WW3DFormat texture_format,bool allow_compression)
{
	if (texture_format)
	{
	    if (texture_format == D3DFMT_R8G8B8
		|| texture_format == D3DFMT_A8R8G8B8
		|| texture_format == D3DFMT_X8R8G8B8
		|| texture_format == D3DFMT_R5G6B5
		|| texture_format == D3DFMT_X1R5G5B5)
		{
			return false; // huh?
		}
	}
	else
	{
		if (ShaderCaps::SupportDXTC && allow_compression)
		{
			return true;
		}
	}
	return false;
}

IDirect3DTexture9 *Load_Compressed_Texture(const StringClass& filename,unsigned int reduction_factor,TextureClass::MipCountType mip_level_count,WW3DFormat dest_format)
{
	DDSFile dds(filename.Peek_Buffer(), reduction_factor);

	IDirect3DSurface9* surface;
	IDirect3DTexture9* texture;
	uint mips = dds.GetMipLevelCount();
	if (mips && dds.Load())
	{
		TT_ASSERT((dest_format == dds.GetPixelFormat()) || (dest_format == WW3D_FORMAT_UNKNOWN));
		texture = DX8Wrapper::Create_DX8_Texture(dds.GetWidth(0), dds.GetHeight(0), dds.GetPixelFormat(), (TextureClass::MipCountType)mips, D3DPOOL_MANAGED, false); 
		
		for (uint i = 0; i < mips; ++i)
		{
			texture->GetSurfaceLevel(i, &surface);
			dds.CopyLevelToSurface(i, surface);
			surface->Release();
		};
		return texture;

	};
	return NULL;
};

void Get_WW3D_Format(WW3DFormat& dest_format,unsigned int& src_bpp,Targa& targa)
{
	dest_format = WW3D_FORMAT_UNKNOWN;
	src_bpp = 0;
	if (targa.Header.PixelDepth == 16)
	{
		dest_format = WW3D_FORMAT_A1R5G5B5;
		src_bpp = 2;
	}
	else
	{
		if (targa.Header.PixelDepth > 16)
		{
			if (targa.Header.PixelDepth == 24)
			{
				dest_format = WW3D_FORMAT_R8G8B8;
				src_bpp = 3;
			}
			else
			{
				if (targa.Header.PixelDepth == 32)
				{
					dest_format = WW3D_FORMAT_A8R8G8B8;
					src_bpp = 4;
				}
			}
		}
		else
		{
			if (targa.Header.PixelDepth == 8)
			{
				src_bpp = 1;
				if (targa.Header.ColorMapType == 1)
				{
					dest_format = WW3D_FORMAT_P8;
				}
				else
				{
					if (targa.Header.ImageType == 3)
					{
						dest_format = WW3D_FORMAT_L8;
					}
					else
					{
						dest_format = WW3D_FORMAT_A8;
					}
				}
			}
		}
	}
}

void Get_WW3D_Format(WW3DFormat& dest_format,WW3DFormat& src_format,unsigned int& src_bpp,Targa& targa)
{
	Get_WW3D_Format(src_format,src_bpp,targa);
	dest_format = src_format;
	if (src_format == WW3D_FORMAT_P8 || src_format == WW3D_FORMAT_L8)
	{
		dest_format = WW3D_FORMAT_X8R8G8B8;
	}
	dest_format = Get_Valid_Texture_Format(dest_format,false);
}

IDirect3DSurface9 *TextureLoader::Load_Surface_Immediate(const StringClass& texture, WW3DFormat texture_format, bool allow_compression)
{
	if (Is_Format_Compressed(texture_format, allow_compression))
	{
		IDirect3DTexture9 *tex = Load_Compressed_Texture(texture, 0, TextureClass::MIP_LEVELS_1, texture_format);
		if (tex)
		{
			IDirect3DSurface9 *surface;
			tex->GetSurfaceLevel(0,&surface);
			tex->Release();
			return surface;
		}
	}
	Targa targa;
	targa.Open(texture.Peek_Buffer(),0);
	targa.Header.ImageDescriptor ^= 0x20u;
	WW3DFormat dest_format;
	WW3DFormat src_format;
	unsigned int src_bpp = 0;
	Get_WW3D_Format(dest_format,src_format,src_bpp,targa);
	if (texture_format)
	{
		dest_format = texture_format;
	}
	char palette[1024];
	targa.SetPalette(palette);
	targa.Load(texture.Peek_Buffer(),1,0);
	int width = targa.Header.Width;
	int height = targa.Header.Height;
	unsigned char *dest_surface = 0;
	unsigned char *src_surface = (unsigned char *)targa.mImage;
	if (src_format == WW3D_FORMAT_A1R5G5B5
		|| src_format == WW3D_FORMAT_R5G6B5
		|| src_format == WW3D_FORMAT_A4R4G4B4
		|| src_format == WW3D_FORMAT_P8
		|| src_format == WW3D_FORMAT_L8)
	{
		dest_surface = new unsigned char[4 * width * height];
		dest_format = Get_Valid_Texture_Format(WW3D_FORMAT_A8R8G8B8,0);
		BitmapHandlerClass::Copy_Image(dest_surface,width,height,4 * width,WW3D_FORMAT_A8R8G8B8,src_surface,width,height,src_bpp * width,src_format,(unsigned char *)targa.mPalette,targa.Header.CMapDepth >> 3,false);
		src_surface = dest_surface;
		src_format = WW3D_FORMAT_A8R8G8B8;
		src_bpp = GetBytesPerPixel(WW3D_FORMAT_A8R8G8B8);
	}
	D3DLOCKED_RECT rect;
	IDirect3DSurface9 *surface = DX8Wrapper::Create_DX8_Surface(width,height,dest_format);
	surface->LockRect(&rect,0,0);
	BitmapHandlerClass::Copy_Image((unsigned char *)rect.pBits,width,height,rect.Pitch,dest_format,src_surface,width,height,src_bpp * width,src_format,(unsigned char *)targa.mPalette,targa.Header.CMapDepth >> 3,false);
	surface->UnlockRect();
	if (dest_surface)
	{
		delete[] dest_surface;
	}
	return surface;
}

void TextureLoader::Request_Thumbnail(TextureClass* texture)
{
	assert(texture != 0);
	FastCriticalSectionClass::LockClass lock(ForegroundCriticalSection);
	if (!texture->D3DTexture)
	{
		if (TextureLoader::Is_DX8_Thread())
		{
			TextureLoader::Load_Thumbnail(texture);
			if (texture->NormalTextureLoadTask)
			{
				ForegroundQueue.Remove(texture->NormalTextureLoadTask);
				texture->NormalTextureLoadTask->Destroy();
			}
		}
		else
		{
			if (!texture->NormalTextureLoadTask)
			{
				if (!texture->ThumbnailTextureLoadTask || texture->ThumbnailTextureLoadTask->CurrentLoadStep <= 1)
				{
					ForegroundQueue.Push_Back(TextureLoadTaskClass::Create(texture,TextureLoadTaskClass::Thumbnail,TextureLoadTaskClass::Background));
				}
			}
		}
	}
}

void TextureLoader::Request_Background_Loading(TextureClass* texture)
{
	assert(texture != 0);
	FastCriticalSectionClass::LockClass lock(ForegroundCriticalSection);
	if (!texture->Initialized)
	{
		if (!texture->ThumbnailTextureLoadTask)
		{
			TextureLoadTaskClass *task = TextureLoadTaskClass::Create(texture,TextureLoadTaskClass::Normal,TextureLoadTaskClass::Background);
			if (TextureLoader::Is_DX8_Thread())
			{
				TextureLoader::Begin_Load_And_Queue(task);
			}
			else
			{
				ForegroundQueue.Push_Back(task);
			}
		}
	}
}

void TextureLoader::Request_Foreground_Loading(TextureClass* texture)
{
	assert(texture != 0);
	FastCriticalSectionClass::LockClass lock(ForegroundCriticalSection);
	if (!texture->Initialized)
	{
		TextureLoadTaskClass *task = texture->ThumbnailTextureLoadTask;
		if (TextureLoader::Is_DX8_Thread())
		{
			if (texture->NormalTextureLoadTask)
			{
				ForegroundQueue.Remove(texture->NormalTextureLoadTask);
				texture->NormalTextureLoadTask->Destroy();
			}
			if (task)
			{
				FastCriticalSectionClass::LockClass lock2(BackgroundCriticalSection);
				ForegroundQueue.Remove(task);
				BackgroundQueue.Remove(task);
			}
			else
			{
				task = TextureLoadTaskClass::Create(texture,TextureLoadTaskClass::Normal,TextureLoadTaskClass::Foreground);
			}
			task->Finish_Load();
			task->Destroy();
		}
		else
		{
			FastCriticalSectionClass::LockClass lock2(BackgroundCriticalSection);
			if (texture->NormalTextureLoadTask)
			{
				texture->NormalTextureLoadTask->CurrentLoadStep = 4;
			}
			if (task)
			{
				if (task->Parent == &BackgroundQueue)
				{
					BackgroundQueue.Remove(task);
					ForegroundQueue.Push_Back(task);
				}
				task->prioritytype = TextureLoadTaskClass::Foreground;
			}
			else
			{
				ForegroundQueue.Push_Back(TextureLoadTaskClass::Create(texture,TextureLoadTaskClass::Normal,TextureLoadTaskClass::Foreground));
			}
		}
	}
}

void TextureLoader::Flush_Pending_Load_Tasks()
{
	int x = true;
	bool b = false;
	while(x)
	{
		{
			FastCriticalSectionClass::LockClass lock(BackgroundCriticalSection);
			if (BackgroundQueue.Head == (TextureLoadTaskClass *)&BackgroundQueue)
			{
				if (ForegroundQueue.Head == (TextureLoadTaskClass *)&ForegroundQueue)
				{
					b = true;
				}
			}
		}
		if (b)
		{
			break;
		}
		TextureLoader::Update(0);
		ThreadClass::Switch_Thread();
	}
}

extern "C" DECLSPEC_IMPORT DWORD WINAPI timeGetTime(void);
void TextureLoader::Update(void (*upd)(void))
{
	if (!TextureLoadSuspended)
	{
		FastCriticalSectionClass::LockClass lock(ForegroundCriticalSection);
		int time = timeGetTime();
		TextureLoadTaskClass *task;
		while ((task = ForegroundQueue.Pop_Front()) != 0)
		{
			if (upd)
			{
				int time2 = timeGetTime();
				if ((time2 - time) > 20)
				{
					upd();
					time2 = time;
				}
			}
			if (task->tasktype == TextureLoadTaskClass::Thumbnail)
			{
				TextureLoader::Process_Foreground_Thumbnail(task);
			}
			else if (task->tasktype == TextureLoadTaskClass::Normal)
			{
				TextureLoader::Process_Foreground_Load(task);
			}
		}
		TextureClass::Invalidate_Old_Unused_Textures(0);
	}
}

void TextureLoader::Suspend_Texture_Load()
{
	TextureLoadSuspended = true;
}

void TextureLoader::Continue_Texture_Load()
{
	TextureLoadSuspended = false;
}

void TextureLoader::Process_Foreground_Thumbnail(TextureLoadTaskClass*task)
{
	switch (task->CurrentLoadStep)
	{
	case 0:
		TextureLoader::Load_Thumbnail(task->Texture);
	case 4:
		task->Destroy();
	}
}

void TextureLoader::Process_Foreground_Load(TextureLoadTaskClass*task)
{
	if (task->prioritytype == 1)
	{
		task->Finish_Load();
		task->Destroy();
		return;
	}
	if (!task->CurrentLoadStep)
	{
		TextureLoader::Begin_Load_And_Queue(task);
		return;
	}
	if (task->CurrentLoadStep == 2)
	{
		task->End_Load();
		task->Destroy();
		return;
	}
}

void TextureLoader::Begin_Load_And_Queue(TextureLoadTaskClass*task)
{
	if (task->Begin_Load())
	{
		BackgroundQueue.Push_Front(task);
	}
	else
	{
		task->Apply_Missing_Texture();
		task->Destroy();
	}
}

void TextureLoader::Load_Thumbnail(TextureClass*texture)
{
	assert(texture != 0);
	const StringClass *str = &texture->Name;
	if (!(texture->FullPath.Is_Empty()))
	{
		str = &texture->FullPath;
	}
	IDirect3DTexture9 *tex = Load_Thumbnail(*str);
	texture->Apply_New_Surface(tex,false);
	tex->Release();
}

TextureLoadTaskListClass _FreeList;

TextureLoadTaskListClass::TextureLoadTaskListClass()
{
	Tail = (TextureLoadTaskClass *)this;
	Head = (TextureLoadTaskClass *)this;
}

void TextureLoadTaskListClass::Push_Front(TextureLoadTaskClass *task)
{
	task->Succ = Head;
	task->Prev = (TextureLoadTaskClass *)this;
	task->Parent = this;
	Head->Prev = task;
	Head = task;
}

void TextureLoadTaskListClass::Push_Back(TextureLoadTaskClass *task)
{
	task->Succ = (TextureLoadTaskClass *)this;
	task->Prev = Tail;
	task->Parent = this;
	Tail->Succ = task;
	Tail = task;
}

TextureLoadTaskClass *TextureLoadTaskListClass::Pop_Front()
{
	if (Head == (TextureLoadTaskClass *)this)
	{
		return 0;
	}
	else
	{
		TextureLoadTaskClass *task = Head;
		Remove(task);
		return task;
	}
}

TextureLoadTaskClass *TextureLoadTaskListClass::Pop_Back()
{
	if (Head == (TextureLoadTaskClass *)this)
	{
		return 0;
	}
	else
	{
		TextureLoadTaskClass *task = Tail;
		Remove(task);
		return task;
	}
}

void TextureLoadTaskListClass::Remove(TextureLoadTaskClass *task)
{
	if (task->Parent == this)
	{
		task->Prev->Succ = task->Succ;
		task->Succ->Prev = task->Prev;
		task->Succ = 0;
		task->Prev = 0;
		task->Parent = 0;
	}
}

SynchronizedTextureLoadTaskListClass::SynchronizedTextureLoadTaskListClass()
{
}

void SynchronizedTextureLoadTaskListClass::Push_Front(TextureLoadTaskClass *task)
{
	FastCriticalSectionClass::LockClass objLock (mutex);
	TextureLoadTaskListClass::Push_Front(task);
}

void SynchronizedTextureLoadTaskListClass::Push_Back(TextureLoadTaskClass *task)
{
	FastCriticalSectionClass::LockClass objLock (mutex);
	TextureLoadTaskListClass::Push_Back(task);
}

TextureLoadTaskClass *SynchronizedTextureLoadTaskListClass::Pop_Front()
{
	FastCriticalSectionClass::LockClass objLock (mutex);
	return TextureLoadTaskListClass::Pop_Front();
}

TextureLoadTaskClass *SynchronizedTextureLoadTaskListClass::Pop_Back()
{
	FastCriticalSectionClass::LockClass objLock (mutex);
	return TextureLoadTaskListClass::Pop_Back();
}

void SynchronizedTextureLoadTaskListClass::Remove(TextureLoadTaskClass *task)
{
	FastCriticalSectionClass::LockClass objLock (mutex);
	TextureLoadTaskListClass::Remove(task);
}

TextureLoadTaskClass::TextureLoadTaskClass() : Prev(0), Succ(0), Texture(0), D3DTexture(0), Format(WW3D_FORMAT_UNKNOWN), Width(0), Height(0), MipLevelCount(0), Reduction(0), tasktype(Thumbnail), prioritytype(Background), CurrentLoadStep(0)
{
	for (int i = 0;i < 12;i++)
	{
		LockedSurfacePtr[i] = 0;
		LockedSurfacePitch[i] = 0;
	}
}

TextureLoadTaskClass::~TextureLoadTaskClass()
{
	Deinit();
}

TextureLoadTaskClass *TextureLoadTaskClass::Create(TextureClass *texture,TaskType tasktype,PriorityType prioritytype)
{
	assert(texture != 0);
	TextureLoadTaskClass *task = _FreeList.Pop_Front();
	if (!task)
	{
		task = new TextureLoadTaskClass();
	}
	task->Init(texture,tasktype,prioritytype);
	return task;
}

void TextureLoadTaskClass::Destroy()
{
	Deinit();
	_FreeList.Push_Front(this);
}

void TextureLoadTaskClass::Delete_Free_Pool()
{
	TextureLoadTaskClass *task;
	while ((task = _FreeList.Pop_Front()) != 0)
	{
		delete task;
	}
}

void TextureLoadTaskClass::Init(TextureClass *texture,TaskType task,PriorityType priority)
{
	assert(texture != 0);
	REF_PTR_SET(Texture,texture);
	tasktype = task;
	prioritytype = priority;
	CurrentLoadStep = 0;
	D3DTexture = 0;
	Format = Texture->TextureFormat;
	Width = 0;
	Height = 0;
	MipLevelCount = Texture->MipLevelCount;
	Reduction = Texture->Get_Reduction();
	for (int i = 0;i < 14;i++)
	{
		LockedSurfacePtr[i] = 0;
		LockedSurfacePitch[i] = 0;
	}
	switch (tasktype)
	{
	case Thumbnail:
		Texture->ThumbnailTextureLoadTask = this;
		break;
	case Normal:
		Texture->NormalTextureLoadTask = this;
		break;
	}
}

void TextureLoadTaskClass::Deinit()
{
	if (Texture)
	{
		switch (tasktype)
		{
		case Thumbnail:
			Texture->ThumbnailTextureLoadTask = 0;
			break;
		case Normal:
			Texture->NormalTextureLoadTask = 0;
			break;
		}
		REF_PTR_RELEASE(Texture);
	}
}

bool TextureLoadTaskClass::Begin_Load()
{
	assert(Texture != 0);
	bool res = false;
	if (Texture->IsCompressionAllowed)
	{
		res = Begin_Compressed_Load();
	}
	if (!res)
	{
		res = Begin_Uncompressed_Load();
	}
	if (res)
	{
		Lock_Surfaces();
		CurrentLoadStep = 1;
	}
	return res;
}

bool TextureLoadTaskClass::Load()
{
	assert(Texture != 0);
	bool res = false;
	if (Texture->IsCompressionAllowed)
	{
		res = Load_Compressed_Mipmap();
	}
	if (!res)
	{
		res = Load_Uncompressed_Mipmap();
	}
	CurrentLoadStep = 2;
	return res;
}

void TextureLoadTaskClass::End_Load()
{
	Unlock_Surfaces();
	Apply(true);
	CurrentLoadStep = 3;
}

void TextureLoadTaskClass::Finish_Load()
{
	switch (CurrentLoadStep)
	{
	case 0:
		if (!Begin_Load())
		{
			Apply_Missing_Texture();
			return;
		}
	case 1:
		Load();
	case 2:
		End_Load();
	}
}

void TextureLoadTaskClass::Apply_Missing_Texture()
{
	D3DTexture = MissingTexture::_Get_Missing_Texture();
	Apply(true);
}

void TextureLoadTaskClass::Apply(bool initialized)
{
	assert(Texture != 0);
	Texture->Apply_New_Surface(D3DTexture,initialized);
	D3DTexture->Release();
	D3DTexture = 0;
}

bool Get_Texture_Information(char const *filename,unsigned int reduction,unsigned int &width,unsigned int &height,WW3DFormat &format,unsigned int &mipcount,bool compressed)
{
	ThumbnailClass *thumbnail;
	for (ThumbnailManagerClass *thumbnailmgr = ThumbnailManagerClass::ThumbnailManagerList.Head();thumbnailmgr;thumbnailmgr = thumbnailmgr->Succ())
	{
		thumbnail = thumbnailmgr->Peek_Thumbnail_Instance(filename);
		if (thumbnail)
		{
			if (!compressed || thumbnail->Format == WW3D_FORMAT_DXT1 || thumbnail->Format == WW3D_FORMAT_DXT2 || thumbnail->Format == WW3D_FORMAT_DXT3 || thumbnail->Format == WW3D_FORMAT_DXT4 || thumbnail->Format == WW3D_FORMAT_DXT5)
			{
				width = thumbnail->MaxWidth >> reduction;
				height = thumbnail->MaxHeight >> reduction;
				mipcount = thumbnail->MipLevels;
				format = thumbnail->Format;
				return true;
			}
			return false;
		}
	}
	if (compressed)
	{
		DDSFile dds(filename, reduction);
		if (dds.GetMipLevelCount())
		{
			width = dds.GetWidth(0);
			height = dds.GetHeight(0);
			format = dds.GetPixelFormat();
			mipcount = dds.GetMipLevelCount() - 1;
			return true;
		}
	}
	else
	{
		Targa targa;
		targa.Open(filename,0);
		WW3DFormat dest_format;
		unsigned int src_bpp;
		Get_WW3D_Format(dest_format, format, src_bpp, targa);
		width = targa.Header.Width >> reduction;
		height = targa.Header.Height >> reduction;
		mipcount = 0;
		return true;
	}
	return false;
}

bool TextureLoadTaskClass::Begin_Compressed_Load()
{
	assert(Texture != 0);
	StringClass *str = &Texture->Name;
	if (!(Texture->FullPath.Is_Empty()))
	{
		str = &Texture->FullPath;
	}

	DDSFile dds(str->Peek_Buffer(), this->Reduction);

	if (dds.GetPixelFormat() == WW3D_FORMAT_UNKNOWN) return false;

	uint mips = dds.GetMipLevelCount();
	uint level = 0;
	while ((dds.GetWidth(level) > ShaderCaps::MaxTextureWidth || dds.GetHeight(level) > ShaderCaps::MaxTextureHeight) && level < mips)
	{
		level++;
	}
	dds.SetBaseMipLevel(level);

	Width = dds.GetWidth(0);
	Height = dds.GetHeight(0);
	MipLevelCount = dds.GetMipLevelCount();
	Reduction = level; 
	Format = dds.GetPixelFormat();

	D3DTexture = DX8Wrapper::Create_DX8_Texture(Width, Height, Format, (TextureClass::MipCountType)MipLevelCount, D3DPOOL_MANAGED, false);
	return D3DTexture ? true: false;
}

bool TextureLoadTaskClass::Begin_Uncompressed_Load()
{
	assert(Texture != 0);
	StringClass *str = &Texture->Name;
	if (!(Texture->FullPath.Is_Empty()))
	{
		str = &Texture->FullPath;
	}
	unsigned int width;
	unsigned int height;
	WW3DFormat format;
	unsigned int mipcount;
	if (Get_Texture_Information(str->Peek_Buffer(),Reduction,width,height,format,mipcount,false))
	{
		WW3DFormat texformat = Get_Valid_Texture_Format(format,false);
		TextureLoader::Validate_Texture_Size(width,height);
		Width = width;
		Height = height;
		WW3DFormat fmt;
		if (Format)
		{
			fmt = Format;
		}
		else
		{
			fmt = texformat;
		}
		Format = Get_Valid_Texture_Format(fmt,false);
		D3DTexture = DX8Wrapper::Create_DX8_Texture(Width,Height,Format,Texture->MipLevelCount,D3DPOOL_MANAGED,false);
		return true;
	}
	return false;
}

void TextureLoadTaskClass::Lock_Surfaces()
{
	MipLevelCount = D3DTexture->GetLevelCount();
	for (unsigned int i = 0;i < MipLevelCount;i++)
	{
		D3DLOCKED_RECT rect;
		D3DTexture->LockRect(i,&rect,0,0);
		LockedSurfacePtr[i] = (unsigned char *)rect.pBits;
		LockedSurfacePitch[i] = rect.Pitch;
	}
}

void TextureLoadTaskClass::Unlock_Surfaces()
{
	for (unsigned int i = 0;i < MipLevelCount;i++)
	{
		if (LockedSurfacePtr[i])
		{
			D3DTexture->UnlockRect(i);
			LockedSurfacePtr[i] = 0;
		}
	}
}

bool TextureLoadTaskClass::Load_Compressed_Mipmap()
{
	assert(Texture != 0);
	StringClass *str = &Texture->Name;
	if (!(Texture->FullPath.Is_Empty()))
	{
		str = &Texture->FullPath;
	}
	DDSFile dds(str->Peek_Buffer(), Reduction);
	uint mips = dds.GetMipLevelCount();
	if (mips && dds.Load())
	{
		for (uint i = 0; i < mips; ++i)
		{
			unsigned char *x = Get_Locked_Surface_Ptr(i);
			dds.CopyLevelToPointer(i, x);
		};
		return true;
	};
	return false;
}

bool TextureLoadTaskClass::Load_Uncompressed_Mipmap()
{
	assert(Texture != 0);
	if (MipLevelCount)
	{
		StringClass *str = &Texture->Name;
		if (!(Texture->FullPath.Is_Empty()))
		{
			str = &Texture->FullPath;
		}
		Targa targa;
		targa.Open(str->Peek_Buffer(),0);
		targa.Header.ImageDescriptor ^= 0x20;
		WW3DFormat dest_format;
		WW3DFormat src_format;
		unsigned int dest_bpp = 0;
		char palette[1024];
		Get_WW3D_Format(dest_format,src_format,dest_bpp,targa);
		if (src_format)
		{
			dest_format = Format;
			targa.SetPalette(palette);
			targa.Load(str->Peek_Buffer(),1,0);
			int srcwidth = targa.Header.Width;
			int srcheight = targa.Header.Height;
			int width = Width;
			int height = Height;
			unsigned char *dest_surface = 0;
			unsigned char *src_surface = (unsigned char *)targa.mImage;
			if (src_format == WW3D_FORMAT_A1R5G5B5
				|| src_format == WW3D_FORMAT_R5G6B5
				|| src_format == WW3D_FORMAT_A4R4G4B4
				|| src_format == WW3D_FORMAT_P8
				|| src_format == WW3D_FORMAT_L8
				|| srcwidth != width
				|| srcheight != height)
			{
				dest_surface = new unsigned char[4 * width * height];
				dest_format = Get_Valid_Texture_Format(WW3D_FORMAT_A8R8G8B8,0);
				BitmapHandlerClass::Copy_Image(dest_surface,width,height,4 * width,WW3D_FORMAT_A8R8G8B8,src_surface,srcwidth,srcheight,dest_bpp * srcwidth,src_format,(unsigned char *)targa.mPalette,targa.Header.CMapDepth >> 3,false);
				src_surface = dest_surface;
				src_format = WW3D_FORMAT_A8R8G8B8;
				srcwidth = width;
				srcheight = height;
				dest_bpp = GetBytesPerPixel(WW3D_FORMAT_A8R8G8B8);
			}
			int pitch = dest_bpp * srcwidth;
			for (unsigned int i = 0;i < MipLevelCount;i++)
			{
				BitmapHandlerClass::Copy_Image(Get_Locked_Surface_Ptr(i),width,height,Get_Locked_Surface_Pitch(i),Format,src_surface,srcwidth,srcheight,pitch,src_format,0,0,true);
				width >>= 1;
				height >>= 1;
				srcwidth >>= 1;
				srcheight >>= 1;
				if ((!width) || (!height) || (!srcwidth) || (!srcheight))
				{
					break;
				}
			}
			if (dest_surface)
			{
				delete[] dest_surface;
			}
			return true;
		}
		return false;
	}
	else
	{
		return false;
	}
}

unsigned char *TextureLoadTaskClass::Get_Locked_Surface_Ptr(int Miplevel)
{
	return LockedSurfacePtr[Miplevel];
}

unsigned int TextureLoadTaskClass::Get_Locked_Surface_Pitch(int Miplevel)
{
	return LockedSurfacePitch[Miplevel];
}

void LoaderThreadClass::Thread_Function()
{
	while (running)
	{
		if (BackgroundQueue.Head != (TextureLoadTaskClass *)&BackgroundQueue)
		{
			FastCriticalSectionClass::LockClass lock(BackgroundCriticalSection);
			TextureLoadTaskClass *task = BackgroundQueue.Pop_Front();
			if (task)
			{
				task->Load();
				ForegroundQueue.Push_Back(task);
			}
		}
		ThreadClass::Switch_Thread();
	}
}

Targa::Targa() : mImage(0), mPalette(0),  mAccess(0), mFlags(0)
{
	Clear_File();
	memset(&Header,0,sizeof(Header));
	memset(&mExtension,0,sizeof(mExtension));
}

void RenFree(void *);

RENEGADE_FUNCTION
void RenFree(void *)
AT1(0x0078EC4F);

Targa::~Targa()
{
	Close();
	if (mPalette && (mFlags & 2))
	{
		RenFree(mPalette);
	}
	if (mImage && (mFlags & 1))
	{
		RenFree(mImage);
	}
}

RENEGADE_FUNCTION
long Targa::Open(const char* name,long mode)
AT1(0x005EE840);

RENEGADE_FUNCTION
void Targa::Close()
AT1(0x005EEAA0);

RENEGADE_FUNCTION
long Targa::Load(const char* name,long flags,bool invert_image)
AT1(0x005EED60);

RENEGADE_FUNCTION
long Targa::Load(const char* name,char* palette,char* image,bool invert_image)
AT1(0x005EEAD0);

RENEGADE_FUNCTION
char*Targa::SetPalette(char* buffer)
AT1(0x005EF370);

void Targa::Clear_File()
{
	TGAFile = 0;
}

bool Targa::Is_File_Open()
{
	return TGAFile != 0;
}

int Targa::File_Seek(int pos,int dir)
{
	return TGAFile->Seek(pos,dir);
}

int Targa::File_Read(void* buffer,int size)
{
	return TGAFile->Read(buffer,size);
}

RENEGADE_FUNCTION
void BitmapHandlerClass::Copy_Image_Generate_Mipmap(unsigned int width, unsigned int height, unsigned char* dest_surface, unsigned int dest_pitch, WW3DFormat dest_format, unsigned char* src_surface, unsigned int src_pitch, WW3DFormat src_format, unsigned char* mip_surface, unsigned int mip_pitch)
AT1(0x00584030);

RENEGADE_FUNCTION
void BitmapHandlerClass::Copy_Image(unsigned char* dest_surface, unsigned int dest_surface_width, unsigned int dest_surface_height, unsigned int dest_surface_pitch, WW3DFormat dest_surface_format, unsigned char* src_surface,unsigned int src_surface_width, unsigned int src_surface_height, unsigned int src_surface_pitch, WW3DFormat src_surface_format, unsigned char* src_palette, unsigned int src_palette_bpp,bool generate_mip_level)
AT1(0x00585180);
 
static bool IsCompressedTextureFormat(WW3DFormat fmt)
{
	return (fmt >= WW3D_FORMAT_DXT1) && (fmt <= WW3D_FORMAT_DXT5);
};
 
static int CalculateSurfaceSize(WW3DFormat fmt, uint width, uint height)
{
	if (IsCompressedTextureFormat(fmt))
	{
		return DDSFile::GetDXTCSurfaceSize(width, height, fmt);
	}
	else
	{
		return GetBytesPerPixel(fmt) * width * height;
	};
};
 
static StringClass Create_Texture_Hash_Name(const StringClass& textureFileName)
{
	StringClass result = textureFileName;
	result.truncateRight(4);
	return result;
}



void ThumbnailManagerClass::Create_Thumbnails()
{
	SimpleFileFactoryClass factory;
	factory.Set_Sub_Directory("Data\\");
	MixFileFactoryClass mixfactory(MixFilename,factory);
	FileFactoryClass *oldfactory = _TheFileFactory;
	_TheFileFactory = &mixfactory;
	DynamicVectorClass<StringClass> list;
	list.Set_Growth_Step(1000);
	mixfactory.Build_Filename_List(list);
	for (int i = 0;i < list.Count();i++)
	{
		int len = list[i].Get_Length() - 3;
		if (_stricmp(&list[i][len],"tga") && _stricmp(&list[i][len],"dds"))
		{
			continue;
		}
		if (Peek_Thumbnail_Instance(list[i]))
		{
			continue;
		}
		new ThumbnailClass(this,list[i]);
	}
	_TheFileFactory = oldfactory;
}

void ThumbnailManagerClass::Load()
{
	Time = 0;
	file_auto_ptr ptr(_TheFileFactory,ThumbFilename);
	if (ptr->Is_Available(0))
	{
		ptr->Open(1);
		Time = ptr->Get_Date_Time();
		unsigned int hdr;
		ptr->Read(&hdr,4);
		if (hdr == *((unsigned int*)ThumbFileHeader))
		{
			unsigned int thumbnailCount;
			unsigned int thumbnailsBitsOffset;
			unsigned int thumbnailsBitsCount;
			ptr->Read(&thumbnailCount, 4);
			ptr->Read(&thumbnailsBitsOffset, 4);
			ptr->Read(&thumbnailsBitsCount, 4);
			ThumbnailBits = new unsigned char[thumbnailsBitsCount];
			for (unsigned int i = 0;i < thumbnailCount;i++)
			{
				unsigned int thumbnailtime;
				unsigned int thumbnailoffset;
				unsigned int thumbnailwidth;
				unsigned int thumbnailheight;
				unsigned int thumbnailmaxwidth;
				unsigned int thumbnailmaxheight;
				unsigned int thumbnailmiplevels;
				WW3DFormat thumbnailformat;
				WW3DFormat thumbnailthumbnailformat;
				unsigned int texturefilenamelength;
				char texturefilename[255];
				ptr->Read(&thumbnailtime,4);
				ptr->Read(&thumbnailoffset,4);
				ptr->Read(&thumbnailwidth,4);
				ptr->Read(&thumbnailheight,4);
				ptr->Read(&thumbnailmaxwidth,4);
				ptr->Read(&thumbnailmaxheight,4);
				ptr->Read(&thumbnailmiplevels,4);
				ptr->Read(&thumbnailformat,4);
				ptr->Read(&thumbnailthumbnailformat, 4);
				ptr->Read(&texturefilenamelength,4);
				ptr->Read(texturefilename,texturefilenamelength);
				texturefilename[texturefilenamelength] = 0;
				bool nochange = true;
				{
					file_auto_ptr texptr(_TheFileFactory,texturefilename);
					if (texptr->Is_Available(0))
					{
						texptr->Open(1);
						if (texptr->Get_Date_Time() != thumbnailtime)
						{
							nochange = false;
						}
						texptr->Close();
					}
					else
					{
						nochange = false;
					}
					new ThumbnailClass(this,texturefilename,ThumbnailBits + thumbnailoffset - thumbnailsBitsOffset,thumbnailwidth,thumbnailheight,thumbnailmaxwidth,thumbnailmaxheight,thumbnailmiplevels,thumbnailformat,thumbnailthumbnailformat,0,thumbnailtime);
				}
			}
			ptr->Read(ThumbnailBits,thumbnailsBitsCount);
		}
		ptr->Close();
	}
	Loading = false;
}

void ThumbnailManagerClass::Save(bool force)
{
	if (Loading || force)
	{
		Loading = false;
		
		uint thumbnailsBitsCount = 0;
		uint thumbnailCount = 0;
		uint thumbnailsBitsOffset = 4 * 4;
		
		for (HashTemplateIterator<StringClass, ThumbnailClass*> iterator(Hash); iterator; ++iterator)
		{
			ThumbnailClass* thumbnail = iterator.getValue();
			uint textureFileNameLength = thumbnail->TextureFilename.Get_Length();
			
			thumbnailsBitsOffset += 4 * 9 + textureFileNameLength;
			thumbnailsBitsCount += 2 * thumbnail->Height * thumbnail->Width;
			++thumbnailCount;
		}
		
		file_auto_ptr file(TheWritingFileFactory, ThumbFilename);
		if (file->Is_Available(0))
			file->Delete();
		
		file->Create();
		file->Open(2);
		
		file->Write(ThumbFileHeader, 4);
		file->Write(&thumbnailCount, 4);
		file->Write(&thumbnailsBitsOffset, 4);
		file->Write(&thumbnailsBitsCount, 4);
		
		uint thumbnailBitsOffset = thumbnailsBitsOffset;
		for (HashTemplateIterator<StringClass, ThumbnailClass*> iterator(Hash); iterator; ++iterator)
		{
			ThumbnailClass* thumbnail = iterator.getValue();
			uint textureFileNameLength = thumbnail->TextureFilename.Get_Length();
			
			file->Write(&thumbnail->Time, 4);
			file->Write(&thumbnailBitsOffset, 4);
			file->Write(&thumbnail->Width, 4);
			file->Write(&thumbnail->Height, 4);
			file->Write(&thumbnail->MaxWidth, 4);
			file->Write(&thumbnail->MaxHeight, 4);
			file->Write(&thumbnail->MipLevels, 4);
			file->Write(&thumbnail->Format, 4);
			file->Write(&thumbnail->ThumbnailFormat, 4);
			file->Write(&textureFileNameLength, 4);
			file->Write(thumbnail->TextureFilename.Peek_Buffer(), textureFileNameLength);
			
			thumbnailBitsOffset += CalculateSurfaceSize(thumbnail->ThumbnailFormat, thumbnail->Width, thumbnail->Height);
		}
		
		for (HashTemplateIterator<StringClass, ThumbnailClass*> iterator(Hash); iterator; ++iterator)
		{
			ThumbnailClass* thumbnail = iterator.getValue();
			file->Write(thumbnail->ThumbnailBits, CalculateSurfaceSize(thumbnail->ThumbnailFormat, thumbnail->Width, thumbnail->Height));
		}
		
		if (Time)
			file->Set_Date_Time(Time);
		
		file->Close();
	}
}

ThumbnailManagerClass::ThumbnailManagerClass(const char *thumbfilename,const char *mixfilename) : ThumbFilename(thumbfilename),MixFilename(mixfilename), ThumbnailBits(0), Loading(false), Time(0)
{
	Load();
}

ThumbnailManagerClass::~ThumbnailManagerClass()
{
	Save(false);
	
	for (HashTemplateIterator<StringClass, ThumbnailClass*> thumbnail(Hash); thumbnail; thumbnail.reset())
		delete thumbnail.getValue();
	
	delete[] ThumbnailBits;
}

void ThumbnailManagerClass::Add_Thumbnail_Manager(const char *thumbfilename, const char *mixfilename)
{
	for (ThumbnailManagerClass* thumbnailmgr = ThumbnailManagerClass::ThumbnailManagerList.Head();thumbnailmgr;thumbnailmgr = thumbnailmgr->Succ())
	{
		if (!strcmp(thumbnailmgr->ThumbFilename,thumbfilename))
		{
			return;
		}
	}
	Update_Thumbnail_File(mixfilename,false);
	ThumbnailManagerClass *manager = new ThumbnailManagerClass(thumbfilename,mixfilename);
	ThumbnailManagerList.Add_Tail(manager);
}

void ThumbnailManagerClass::Remove_Thumbnail_Manager(const char* thumbnailFilename)
{
	ThumbnailManagerClass* thumbnailManager = ThumbnailManagerList.Head();
	while (strcmp(thumbnailManager->ThumbFilename, thumbnailFilename))
		thumbnailManager = thumbnailManager->Succ();
	
	if (thumbnailManager)
		delete thumbnailManager;
}

ThumbnailClass *ThumbnailManagerClass::Peek_Thumbnail_Instance(const StringClass &texture)
{
	ThumbnailClass *thumbnail = Get_From_Hash(texture);
	if (!thumbnail)
	{
		if (unkC)
		{
			thumbnail = new ThumbnailClass(this,texture);
			if (!thumbnail->ThumbnailBits)
			{
				SAFE_DELETE(thumbnail);
			}
		}
	}
	return thumbnail;
}

void ThumbnailManagerClass::Insert_To_Hash(ThumbnailClass *thumbnail)
{
	TT_ASSERT(Hash.Get(Create_Texture_Hash_Name(thumbnail->TextureFilename)) == NULL);

	Loading = true;
	Hash.Insert(Create_Texture_Hash_Name(thumbnail->TextureFilename), thumbnail);
}

ThumbnailClass* ThumbnailManagerClass::Get_From_Hash(const StringClass &texture)
{
	ThumbnailClass** thumbnail = Hash.Get(Create_Texture_Hash_Name(texture));
	return thumbnail ? *thumbnail : NULL;
}

void ThumbnailManagerClass::Remove_From_Hash(ThumbnailClass* thumbnail)
{
	Loading = true;
	Hash.Remove(Create_Texture_Hash_Name(thumbnail->TextureFilename));
}

bool displaymessage = 0;
void ThumbnailManagerClass::Update_Thumbnail_File(const char* thumbnailFileName, bool b)
{
	SimpleFileFactoryClass factory;
	factory.Set_Sub_Directory("Data\\");
	StringClass str = thumbnailFileName;
	int len = str.Get_Length();
	str[len - 3] = 't';
	str[len - 2] = 'h';
	str[len - 1] = 'u';
	FileClass *mix = factory.Get_File(thumbnailFileName);
	FileClass *thu = factory.Get_File(str);
	mix->Open(1);
	thu->Open(1);
	if (mix->Is_Available(0))
	{
		unsigned int time = mix->Get_Date_Time();
		if (thu->Is_Available(0))
		{
			if (time != thu->Get_Date_Time())
			{
				thu->Delete();
			}
			unsigned int hdr;
			thu->Read(&hdr,4);
			if (hdr != *((unsigned int*)ThumbFileHeader))
			{
				thu->Delete();
			}
		}
		if (!thu->Is_Available(0))
		{
			if (b)
			{
				if (!displaymessage)
				{
					displaymessage = true;
					MessageBox(0,"Some or all texture thumbnails need to be updated.\nThis will take a while. The update will only be done once\neach time a mix file changes and thumb database hasn't been\nupdated.","Updating texture thumbnails",0);
				}
			}
			ThumbnailManagerClass *manager = new ThumbnailManagerClass(str.Peek_Buffer(),thumbnailFileName);
			manager->Time = time;
			manager->Create_Thumbnails();
			manager->Save(true);
			SAFE_DELETE(manager);
		}
		mix->Close();
		thu->Close();
	}
	else
	{
		if (thu->Is_Available(0))
		{
			thu->Delete();
		}
		mix->Close();
		thu->Close();
	}
	factory.Return_File(mix);
	factory.Return_File(thu);
}

void ThumbnailManagerClass::Init()
{
	Add_Thumbnail_Manager("always.thu","always.dat");
	Add_Thumbnail_Manager("always2.thu","always2.dat");
}

void ThumbnailManagerClass::Deinit()
{
	while (ThumbnailManagerList.Head())
		delete ThumbnailManagerList.Head();
}

ThumbnailClass::ThumbnailClass(ThumbnailManagerClass *manager,char  const*texture,unsigned char *bits,unsigned int width,unsigned int height,unsigned int maxwidth,unsigned int maxheight,unsigned int miplevels,WW3DFormat format,WW3DFormat thumbnailformat,bool isallocated,unsigned long time) : TextureFilename(texture),ThumbnailBits(bits),Height(height),Width(width),MaxWidth(maxwidth),MaxHeight(maxheight),MipLevels(miplevels),Format(format),ThumbnailFormat(thumbnailformat),Time(time),IsAllocated(isallocated),ThumbnailManager(manager)
{
	manager->Insert_To_Hash(this);
}

ThumbnailClass::ThumbnailClass(ThumbnailManagerClass *manager,const StringClass &texture) : TextureFilename(texture), ThumbnailBits(0), Height(0), Width(0), MaxWidth(0), MaxHeight(0), MipLevels(0), Format(WW3D_FORMAT_UNKNOWN), Time(0), IsAllocated(0), ThumbnailManager(manager)
{
	DDSFile dds(texture.Peek_Buffer(), 0);
	uint mips = dds.GetMipLevelCount();
	if (mips && dds.Load())
	{
		Time = dds.GetTime();
		int len = TextureFilename.Get_Length();
		TextureFilename[len - 3] = 'd';
		TextureFilename[len - 2] = 'd';
		TextureFilename[len - 1] = 's';
		
		uint levels = 0;
		while (levels < mips-1 && (dds.GetWidth(levels) > 32 || dds.GetHeight(levels) > 32))
		{
			levels++;
		}
		MaxWidth = dds.GetWidth(0);
		MaxHeight = dds.GetHeight(0);
		Format = dds.GetPixelFormat();
		MipLevels = dds.GetMipLevelCount();
		Width = dds.GetWidth(levels);
		Height = dds.GetHeight(levels);
		ThumbnailFormat = dds.GetPixelFormat();
		ThumbnailBits = new unsigned char[CalculateSurfaceSize(ThumbnailFormat, Width, Height)];
		IsAllocated = true;

		dds.CopyLevelToPointer(levels, ThumbnailBits);		
		ThumbnailManager->Insert_To_Hash(this);
		return;
	}
	Targa targa;
	targa.Open(texture.Peek_Buffer(),0);
	targa.Header.ImageDescriptor ^= 0x20;
	unsigned int src_bpp;
	WW3DFormat dest_format;
	Get_WW3D_Format(dest_format,src_bpp,targa);
	if (dest_format)
	{
		MaxWidth = targa.Header.Width;
		MaxHeight = targa.Header.Height;
		Format = dest_format;
		Width = targa.Header.Width >> 3;
		Height = targa.Header.Height >> 3;
		MipLevels = 1;
		int i;
		int j;
		for (i = 1,j = 1;i < MaxWidth && j < MaxHeight;i *= 2,j *= 2)
		{
			MipLevels++;
		}
		for (;Height > 0x20 && Width > 0x20;Height >>= 2,Width >>= 2)
		{
		}
		int height;
		for (height = 1;height < Height;height *= 2)
		{
		}
		int width;
		for (width = 1;width < width;width *= 2)
		{
		}
		Height = height;
		Width = width;
		char palette[1024];
		targa.SetPalette(palette);
		targa.Load(texture.Peek_Buffer(),1,0);
		{
			file_auto_ptr ptr(_TheFileFactory,texture.Peek_Buffer());
			ptr->Open(1);
			Time = ptr->Get_Date_Time();
			ptr->Close();
		}
		int len = TextureFilename.Get_Length();
		TextureFilename[len - 3] = 't';
		TextureFilename[len - 2] = 'g';
		TextureFilename[len - 1] = 'a';
		ThumbnailFormat = WW3D_FORMAT_A4R4G4B4;
		ThumbnailBits = new unsigned char[CalculateSurfaceSize(ThumbnailFormat, Width, Height)];
		IsAllocated = true;
		BitmapHandlerClass::Copy_Image(ThumbnailBits,Width,Height,2 * this->Width,WW3D_FORMAT_A4R4G4B4,(unsigned char *)targa.mImage,targa.Header.Width,targa.Header.Height,src_bpp * targa.Header.Width,dest_format,(unsigned char *)targa.mPalette,targa.Header.CMapDepth >> 3,false);
		ThumbnailManager->Insert_To_Hash(this);
	}
}

ThumbnailClass::~ThumbnailClass()
{
	if (IsAllocated)
		delete[] ThumbnailBits;
	
	ThumbnailManager->Remove_From_Hash(this);
}
