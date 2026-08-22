#ifndef TT_INCLUDE__DX8CAPS_H
#define TT_INCLUDE__DX8CAPS_H



#include "engine_3dre.h"
#include "d3d8caps.h"


class DX8Caps
{

public:

	int WidthLimit; // 0000
	int HeightLimit; // 0004
	D3DCAPS8 Caps; // 0008
	bool UseTnL; // 00DC
	bool SupportDXTC; // 00DD
	bool supportGamma; // 00DE
	bool SupportNPatches; // 00DF
	bool SupportBumpEnvmap; // 00E0
	bool SupportBumpEnvmapLuminance; // 00E1
	bool SupportTextureFormat[25]; // 00E2
	bool SupportRenderToTextureFormat[25]; // 00FB
	bool SupportZBias; // 0114
	bool SupportAnisotropicFiltering; // 0115
	bool SupportMultiTexture; // 0116
	bool SupportFog; // 0117
	unsigned int MaxTexturesPerPass; // 0118
	unsigned int VertexShaderVersion; // 011C
	unsigned int PixelShaderVersion; // 0120
	unsigned int DeviceNumber; // 0124
	unsigned int DriverBuildNum; // 0128
	unsigned int DriverStatus; // 012C
	unsigned int VendorNumber; // 0130
	StringClass DriverFilename; // 0134
	IDirect3D9* D3D; // 0138
	StringClass VideoCardSpecString; // 013C
	StringClass VideoCardName; // 0140

	unsigned int Define_Vendor(unsigned int vendor);
	unsigned int Get_ATI_Device(unsigned int device);
	unsigned int Get_3DLabs_Device(unsigned int device);
	unsigned int Get_NVidia_Device(unsigned int device);
	unsigned int Get_3Dfx_Device(unsigned int device);
	unsigned int Get_Matrox_Device(unsigned int device);
	unsigned int Get_PowerVR_Device(unsigned int device);
	unsigned int Get_S3_Device(unsigned int device);
	unsigned int Get_Intel_Device(unsigned int device);
	DX8Caps(IDirect3D9 *Direct3D, IDirect3DDevice9 *device, WW3DFormat Format, const D3DADAPTER_IDENTIFIER9 &Adapter);
	DX8Caps(IDirect3D9 *Direct3D, const D3DCAPS8 &NewCaps, WW3DFormat Format, const D3DADAPTER_IDENTIFIER9 &Adapter);
	DX8Caps(IDirect3D9 *Direct3D, const D3DCAPS9 &NewCaps, WW3DFormat Format, const D3DADAPTER_IDENTIFIER9 &Adapter);
	~DX8Caps();
	void Init_Caps(IDirect3DDevice9 *device);
	void Compute_Caps(WW3DFormat Format,const D3DADAPTER_IDENTIFIER9 &Adapter);
	void Check_Bumpmap_Support(const D3DCAPS8 &Caps);
	void Check_Texture_Compression_Support(const D3DCAPS8 &Caps);
	void Check_Texture_Format_Support(WW3DFormat Format,const D3DCAPS8 &Caps);
	void Check_Render_To_Texture_Support(WW3DFormat Format,const D3DCAPS8 &Caps);
	void Check_Shader_Support(const D3DCAPS8 &Caps);
	void Check_Driver_Version_Status();
	bool Is_Valid_Display_Format(int a,int b,WW3DFormat Format);
	void Vendor_Specific_Hacks(const D3DADAPTER_IDENTIFIER9 &Adapter);

}; // 0144



#endif