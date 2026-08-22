#include "General.h"
#include "DDSFile.h"
#include <DDraw.h>

uint DDSFile::GetDXTCBlockSize(WW3DFormat format)
{
	switch(format)
	{
	case WW3D_FORMAT_DXT1: return 8; // 64 bits per 4x4 pixel block
	case WW3D_FORMAT_DXT2: 
	case WW3D_FORMAT_DXT3: 
	case WW3D_FORMAT_DXT4:
	case WW3D_FORMAT_DXT5: return 16; // 128 bits per 4x4 pixel block
	default: TT_UNREACHABLE;
	};
};

uint DDSFile::GetDXTCSurfaceSize(uint width, uint height, WW3DFormat format)
{
	TT_ASSERT(max(4, width) % 4 == 0);  // if these asserts are hit, then the texture can't possibly be DXTC
	TT_ASSERT(max(4, height) % 4 == 0); // and we've got a problem then
	
	uint blocks_h = max(4, width) / 4;
	uint blocks_v = max(4, height) / 4;

	uint blocksize = GetDXTCBlockSize(format);
	return blocksize * blocks_h * blocks_v;
};

inline bool IsBitMask(const LegacyDDPIXELFORMAT& format, unsigned int r, unsigned int g, unsigned int b, unsigned int a)
{
	return format.RBitMask == r && format.GBitMask == g && format.BBitMask == b && format.RGBAlphaBitMask == a;
};

WW3DFormat DDSFile::ConvertPixelFormat(const LegacyDDPIXELFORMAT& format)
{
	if (format.Flags & DDPF_RGB) // rgb formats
	{
		if (format.RGBBitCount == 32)
		{
			if (IsBitMask(format, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000)) return WW3D_FORMAT_A8R8G8B8;
			else if (IsBitMask(format, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000)) return WW3D_FORMAT_X8R8G8B8;
			else if (IsBitMask(format, 0xffffffff, 0x00000000, 0x00000000, 0x00000000)) return WW3D_FORMAT_R32F;
		}
		else if (format.RGBBitCount == 24)
		{
			//if (IsBitMask(format, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000)) return WW3D_FORMAT_R8G8B8; // currently broken on loading, use X8R8G8B8 instead
		}
		else if (format.RGBBitCount == 16)
		{
			if (IsBitMask(format, 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000)) return WW3D_FORMAT_R5G6B5;
		}
	}
	else if (format.Flags & DDPF_LUMINANCE)
	{
		if (format.RGBBitCount == 8) return WW3D_FORMAT_L8;
	}
	else if (format.Flags & DDPF_ALPHA)
	{
		if (format.RGBBitCount == 8) return WW3D_FORMAT_A8;
	}
	else if (format.Flags & DDPF_FOURCC)
	{
		WW3DFormat test = D3DFormat_To_WW3DFormat(D3DFORMAT(format.FourCC));
		if (test != WW3D_FORMAT_UNKNOWN) return test;
	}
	return WW3D_FORMAT_UNKNOWN;
};

DDSFile::DDSFile(const char *name, uint reduction): FileMemory(NULL), MipLevels(0, NULL), PixelFormat(WW3D_FORMAT_UNKNOWN), Time(0), BaseMip(0)
{
	strncpy(Name,name,256);
	int len = strlen(name);
	Name[len - 3] = 'd';
	Name[len - 2] = 'd';
	Name[len - 1] = 's';

	file_auto_ptr ptr(_TheFileFactory,Name);
	if (!ptr->Is_Available(0)) return;

	ptr->Open(1);
	Time = ptr->Get_Date_Time();

	uint32 magic;
	ptr->Read(&magic, sizeof(magic));
	if (magic != *((uint32*)"DDS ")) return;
	
	LegacyDDSURFACEDESC2 header = {};
	ptr->Read(&header, sizeof(header));

	PixelFormat = ConvertPixelFormat(header.PixelFormat);
	if (PixelFormat == WW3D_FORMAT_UNKNOWN) return; // =(

	//MipLevels.Clear();
	{
		MipLevelDesc root_mip;
		root_mip.Width = header.Width;
		root_mip.Height = header.Height;
		root_mip.Offset = sizeof(magic) + sizeof(header);
		root_mip.Pitch = header.Pitch;
		if (!root_mip.Pitch)
		{
			if (IsCompressed())
			{
				root_mip.Pitch = GetDXTCSurfaceSize(root_mip.Width, root_mip.Height, PixelFormat);
			}
			else
			{
				root_mip.Pitch = GetBytesPerPixel(PixelFormat) * root_mip.Width;
			};
		}
		MipLevels.Add(root_mip);
	}

	if (!header.MipMapCount) return;

	uint mips = header.MipMapCount - 1;
	BaseMip = reduction > mips ? mips : reduction;

	if (IsCompressed())
	{
		for (uint i = 0; i < mips; )
		{
			const MipLevelDesc& prev_mip = MipLevels[i++];
			MipLevelDesc mip;
			mip.Width = prev_mip.Width / 2;
			mip.Height = prev_mip.Height / 2;
			mip.Offset = prev_mip.Offset + prev_mip.LinearSize;
			mip.LinearSize = GetDXTCSurfaceSize(mip.Width, mip.Height, PixelFormat);
			MipLevels.Add(mip);
		};
	}
	else
	{
		for (uint i = 0; i < mips; )
		{
			const MipLevelDesc& prev_mip = MipLevels[i++];
			MipLevelDesc mip;
			mip.Width = prev_mip.Width / 2;
			mip.Height = prev_mip.Height / 2;
			mip.Offset = prev_mip.Offset + (prev_mip.Pitch * prev_mip.Height);
			mip.Pitch = GetBytesPerPixel(PixelFormat) * mip.Width;
			MipLevels.Add(mip);
		};
	};
};

DDSFile::~DDSFile()
{
	delete[] FileMemory;
	FileMemory = NULL;
};

bool DDSFile::Load()
{
	if (FileMemory || !MipLevels.Count()) return false;

	file_auto_ptr ptr(_TheFileFactory, Name);
	if (!ptr->Is_Available(0)) return false;
	ptr->Open(1);
	
	int size = ptr->Size();
	if (!size) return false;
	FileMemory = new unsigned char[size];
	ptr->Read(FileMemory, size);
	ptr->Close();
	return true;
};

bool DDSFile::IsCompressed()
{
	return (this->PixelFormat >= WW3D_FORMAT_DXT1) && (this->PixelFormat <= WW3D_FORMAT_DXT5);
};

uint DDSFile::GetWidth(uint level)
{
	TT_ASSERT((level + BaseMip) < (uint)MipLevels.Count());
	return MipLevels[level + BaseMip].Width;
};

uint DDSFile::GetHeight(uint level)
{
	TT_ASSERT((level + BaseMip) < (uint)MipLevels.Count());
	return MipLevels[level + BaseMip].Height;
};

uint DDSFile::GetMipLevelCount()
{
	return MipLevels.Count() - BaseMip;
};

bool DDSFile::CopyToTexture(IDirect3DTexture9* texture) 
{
	//TODO
	return false;
};

bool DDSFile::CopyLevelToSurface(uint level, IDirect3DSurface9* surface) 
{
	TT_ASSERT((level + BaseMip) < (uint)MipLevels.Count());	
	D3DSURFACE_DESC desc = {};
	surface->GetDesc(&desc);
		
	const MipLevelDesc& mip = MipLevels[level + BaseMip];
	if (desc.Format != WW3DFormat_To_D3DFormat(PixelFormat)) return false;	// Something went horrible wrong
	if (desc.Width != mip.Width) return false;								// There was blood everywhere
	if (desc.Height != mip.Height) return false;							// We couldn't save anyone

	D3DLOCKED_RECT rect = {};
	surface->LockRect(&rect, NULL, NULL);

	if ((uint)rect.Pitch != mip.Pitch) return false;

	CopyLevelToPointer(level, rect.pBits);

	surface->UnlockRect();
	return true;
};

void DDSFile::CopyLevelToPointer(uint level, void* ptr) 
{
	TT_ASSERT((level + BaseMip) < (uint)MipLevels.Count());	
	const MipLevelDesc& mip = MipLevels[level + BaseMip];

	if (IsCompressed())
	{
		memcpy(ptr, &FileMemory[mip.Offset], mip.LinearSize);
	}
	else
	{
		memcpy(ptr, &FileMemory[mip.Offset], mip.Pitch * mip.Height);
	};
};