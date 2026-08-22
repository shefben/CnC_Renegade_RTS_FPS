#ifndef _SHADERS_DDSFILE_H_
#define _SHADERS_DDSFILE_H_
#include "engine_vector.h"
#include "engine_3d.h"
struct LegacyDDCOLORKEY {
	unsigned int ColorSpaceLowValue; //0
	unsigned int ColorSpaceHighValue; //4
};

struct LegacyDDPIXELFORMAT {
	unsigned int Size; //0
	unsigned int Flags; //4
	unsigned int FourCC; //8
	union {
		unsigned int RGBBitCount; //C
		unsigned int YUVBitCount; //C
		unsigned int ZBufferBitDepth; //C
		unsigned int AlphaBitDepth; //C
		unsigned int LuminanceBitCount; //C
		unsigned int BumpBitCount; //C
	};
	union {
		unsigned int BumpBitCount; //10
		unsigned int RBitMask; //10
		unsigned int YBitMask; //10
		unsigned int StencilBitDepth; //10
		unsigned int LuminanceBitMask; //10
		unsigned int BumpDuBitMask; //10
	};
	union {
		unsigned int GBitMask; //14
		unsigned int UBitMask; //14
		unsigned int ZBitMask; //14
		unsigned int BumpDvBitMask; //14
	};
	union {
		unsigned int BBitMask; //18
		unsigned int VBitMask; //18
		unsigned int StencilBitMask; //18
		unsigned int BumpLuminanceBitMask; //18
	};
	union {
		unsigned int RGBAlphaBitMask; //1C
		unsigned int YUVAlphaBitMask; //1C
		unsigned int LuminanceAlphaBitMask; //1C
		unsigned int RGBZBitMask; //1C
		unsigned int YUVZBitMask; //1C
	};
};

struct LegacyDDSCAPS2 {
	unsigned int Caps; //0
	unsigned int Caps2; //4
	unsigned int Caps3; //8
	unsigned int Caps4; //C
};

struct LegacyDDSURFACEDESC2 {
	unsigned int Size; //0
	unsigned int Flags; //4
	unsigned int Height; //8
	unsigned int Width; //C
	union {
		unsigned int Pitch; //10
		unsigned int LinearSize; //10
	};
	unsigned int BackBufferCount; //14
	union {
		unsigned int MipMapCount; //18
		unsigned int RefreshRate; //18
	};
	unsigned int AlphaBitDepth; //1C
	unsigned int Reserved; //20
	void* Surface; //24
	union {
		LegacyDDCOLORKEY CKDestOverlay; //28
		unsigned int EmptyFaceColor; //28
	};
	LegacyDDCOLORKEY CKDestBlt; //30
	LegacyDDCOLORKEY CKSrcOverlay; //38
	LegacyDDCOLORKEY CKSrcBlt; //40
	LegacyDDPIXELFORMAT PixelFormat; //48
	LegacyDDSCAPS2 Caps; //68
	unsigned int TextureStage; //78
};

class DDSFile
{
	struct MipLevelDesc: NoEqualsClass<MipLevelDesc>
	{
		uint Width;			// Mip level height
		uint Height;		// Mip level width
		uint Offset;		// Mip level offset 
		union
		{
			uint Pitch;			// (uncompressed formats) Length in bytes of one scanline
			uint LinearSize;	// (compressed formats) Total number of bytes in the top level image
		};
	};
	
	char Name[256];			// Filename
	uint Time;				// File creation time
	WW3DFormat PixelFormat; // WW3D_FORMAT_UNKNOWN if unable to load 
	DynamicVectorClass<MipLevelDesc> MipLevels; 
	uint BaseMip;			// "Reduction"

	unsigned char* FileMemory;

public:
	static uint GetDXTCBlockSize(WW3DFormat format);
	static uint GetDXTCSurfaceSize(uint width, uint height, WW3DFormat format);
	static WW3DFormat ConvertPixelFormat(const LegacyDDPIXELFORMAT& format);

	DDSFile(const char* name, uint reduction);
	~DDSFile();
	bool Load();

	bool IsCompressed();
	uint GetTime() { return Time; };
	uint GetWidth(uint level);
	uint GetHeight(uint level);
	uint GetMipLevelCount();
	WW3DFormat GetPixelFormat() { return PixelFormat; };

	uint GetBaseMipLevel() { return BaseMip; };
	void SetBaseMipLevel(uint level) { BaseMip = level; };

	bool CopyToTexture(IDirect3DTexture9* texture);
	bool CopyLevelToSurface(uint level, IDirect3DSurface9* surface);
	void CopyLevelToPointer(uint level, void* ptr);
};


#endif