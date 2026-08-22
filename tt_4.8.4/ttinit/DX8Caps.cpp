#include "general.h"

#include "engine_3dre.h"
#include "scripts.h"
#include "DX8Caps.h"
#include "DX8Wrapper.h"

static char *VendorNames[13] = {"Unknown","NVIDIA","ATI","Intel","S3","PowerVR","Matrox","3Dfx","3DLabs","CirrusLogic","Rendition","SiS","VIA"};

unsigned int DX8Caps::Define_Vendor(unsigned int vendor)
{
	switch(vendor)
	{
	case 0x10DE:
	case 0x12D2:
	case 0x14AF:
		return 1;
	case 0x1002:
		return 2;
	case 0x8086:
		return 3;
	case 0x5333:
		return 4;
	case 0x104A:
		return 5;
	case 0x102B:
		return 6;
	case 0x109D:
	case 0x1142:
	case 0x121A:
		return 7;
	case 0x104C:
	case 0x3D3D:
		return 8;
	case 0x1039:
		return 11;
	case 0x1040:
		return 12;
	}
	return 0;
}

unsigned int DX8Caps::Get_ATI_Device(unsigned int device) // only devices needed by DX8Caps::Vendor_Specific_Hacks
{
	switch(device)
	{
	//Rage IIc AGP
	case 0x4757:
		return 5;
	//Rage IIc AGP
	case 0x4758:
		return 6;
	//Rage IIc AGP
	case 0x4759:
		return 7;
	//Rage IIc AGP
	case 0x475A:
		return 8;
	//Rage 128 Mobility M3
	case 0x4C45:
		return 9;
	//Rage 128 Mobility M3
	case 0x4C46:
		return 10;
	//Rage 128 Mobility M4
	case 0x4D46:
		return 11;
	//Rage 128 Mobility M4
	case 0x4D4C:
		return 12;
	//Rage 128 PRO ULTRA
	case 0x5446:
		return 13;
	//Rage 128 PRO ULTRA
	case 0x544C:
		return 14;
	//Rage 128 PRO ULTRA
	case 0x5452:
		return 15;
	//Rage 128 PRO ULTRA
	case 0x5453:
		return 16;
	//Rage 128 PRO ULTRA
	case 0x5454:
		return 17;
	//Rage 128 PRO ULTRA
	case 0x5455:
		return 18;
	//Rage 128 4X
	case 0x534E:
		return 19;
	//Rage 128 PRO GL
	case 0x5041:
		return 20;
	//Rage 128 PRO GL
	case 0x5042:
		return 21;
	//Rage 128 PRO GL
	case 0x5043:
		return 22;
	}
	return 0;
}

unsigned int DX8Caps::Get_3Dfx_Device(unsigned int device) // only devices needed by DX8Caps::Vendor_Specific_Hacks
{
	switch (device)
	{
	case 5:
		return 2;
	case 3:
		return 3;
	case 2:
		return 4;
	}
	return 0;
}

unsigned int DX8Caps::Get_S3_Device(unsigned int device) // only devices needed by DX8Caps::Vendor_Specific_Hacks
{
	switch (device)
	{
	case 0x8A22:
		return 2;
	case 0x9102:
		return 3;
	}
	return 0;
}

DX8Caps::~DX8Caps()
{
}

DX8Caps::DX8Caps(IDirect3D9 *Direct3D, IDirect3DDevice9 *device,WW3DFormat Format,const D3DADAPTER_IDENTIFIER9 &Adapter) : DriverFilename(0,false), VideoCardSpecString(0,false), VideoCardName(0,false)
{
	WidthLimit = 0;
	HeightLimit = 0;
	D3D = Direct3D;
	Init_Caps(device);

	Compute_Caps(Format,Adapter);

}

DX8Caps::DX8Caps(IDirect3D9 *Direct3D,const D3DCAPS8 &NewCaps,WW3DFormat Format,const D3DADAPTER_IDENTIFIER9 &Adapter) : DriverFilename(0,false), VideoCardSpecString(0,false), VideoCardName(0,false)
{
	WidthLimit = 0;
	HeightLimit = 0;
	D3D = Direct3D;
	memcpy(&Caps,&NewCaps,sizeof(D3DCAPS8));
	if(Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
	{
		UseTnL = true;
	}
	else
	{
		UseTnL = false;
	}
	Compute_Caps(Format,Adapter);

}

DX8Caps::DX8Caps(IDirect3D9 *Direct3D,const D3DCAPS9 &NewCaps,WW3DFormat Format,const D3DADAPTER_IDENTIFIER9 &Adapter) : DriverFilename(0,false), VideoCardSpecString(0,false), VideoCardName(0,false)
{
	WidthLimit = 0;
	HeightLimit = 0;
	D3D = Direct3D;
	memcpy(&Caps,&NewCaps,sizeof(D3DCAPS8));
	if(Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
	{
		UseTnL = true;
	}
	else
	{
		UseTnL = false;
	}
	Compute_Caps(Format,Adapter);

}

void DX8Caps::Init_Caps(IDirect3DDevice9 *device)
{
	D3DCAPS9 capsx;
	device->GetDeviceCaps(&capsx);
	memcpy(&Caps,&capsx,sizeof(D3DCAPS8));
	if(Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
	{
		UseTnL = true;
	}
	else
	{
		UseTnL = false;
	}
}

StringClass *CapsWorkString;
void DX8Caps::Compute_Caps(WW3DFormat Format,const D3DADAPTER_IDENTIFIER9 &Adapter)
{
	CapsWorkString = new StringClass(0,false);
	SupportMultiTexture = true;
	SupportFog = true;
	VideoCardSpecString = "";
	VideoCardName = "";
	CapsWorkString->Format("Video Card: %s\r\n",Adapter.Description);
	VideoCardSpecString += *CapsWorkString;
	CapsWorkString->Format("Driver: %s\r\n",Adapter.Driver);
	VideoCardSpecString += *CapsWorkString;
	DriverFilename = Adapter.Driver;
	CapsWorkString->Format("Product=%d, Version=%d, SubVersion=%d, Build=%d\r\n",HIWORD(Adapter.DriverVersion.HighPart),LOWORD(Adapter.DriverVersion.HighPart),HIWORD(Adapter.DriverVersion.LowPart),LOWORD(Adapter.DriverVersion.LowPart));
	DriverBuildNum = LOWORD(Adapter.DriverVersion.LowPart);
	VideoCardSpecString += *CapsWorkString;
	VendorNumber = Define_Vendor(Adapter.VendorId);
	if ((!VendorNumber) && (DriverFilename[0] == '3'))
	{
		VendorNumber = 7;
	}
	#pragma warning (suppress: 6385) //accessing 'VendorNames', the readable size is '52' bytes, but '4004' bytes might be read
	CapsWorkString->Format("%s\t",VendorNames[VendorNumber]);
	VideoCardName += *CapsWorkString;
	CapsWorkString->Format("Video Card Chip Vendor: %s\r\n",VendorNames[VendorNumber]);
	VideoCardSpecString += *CapsWorkString;	
	CapsWorkString->Format("Vendor id: 0x%x\r\n",Adapter.VendorId);
	VideoCardSpecString += *CapsWorkString;
	CapsWorkString->Format("Device id: 0x%x\r\n",Adapter.DeviceId);
	VideoCardSpecString += *CapsWorkString;
	CapsWorkString->Format("SubSys id: 0x%x\r\n",Adapter.SubSysId);
	VideoCardSpecString += *CapsWorkString;
	CapsWorkString->Format("Revision: %d\r\n",Adapter.Revision);
	VideoCardSpecString += *CapsWorkString;
	CapsWorkString->Format("GUID = {0x%x, 0x%x, 0x%x}, {0x%2.2x, 0x%2.2x, 0x%2.2x, 0x%2.2x, 0x%2.2x, 0x%2.2x, 0x%2.2x, 0x%2.2x}\r\n",Adapter.DeviceIdentifier.Data1,Adapter.DeviceIdentifier.Data2,Adapter.DeviceIdentifier.Data3,Adapter.DeviceIdentifier.Data4[0],Adapter.DeviceIdentifier.Data4[1],Adapter.DeviceIdentifier.Data4[2],Adapter.DeviceIdentifier.Data4[3],Adapter.DeviceIdentifier.Data4[4],Adapter.DeviceIdentifier.Data4[5],Adapter.DeviceIdentifier.Data4[6],Adapter.DeviceIdentifier.Data4[7]);
	VideoCardSpecString += *CapsWorkString;
	SupportNPatches = Caps.DevCaps & D3DDEVCAPS_NPATCHES;
	SupportZBias = Caps.RasterCaps & D3DPRASTERCAPS_DEPTHBIAS;
	supportGamma = Caps.Caps2 & D3DCAPS2_FULLSCREENGAMMA;

	SupportAnisotropicFiltering = (Caps.TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC) && (Caps.TextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC);

	CapsWorkString->Format("Hardware T&L support: %s\r\n", UseTnL ? "Yes" : "No");
	VideoCardSpecString += *CapsWorkString;

	CapsWorkString->Format("NPatch support: %s\r\n", SupportNPatches ? "Yes" : "No");
	VideoCardSpecString += *CapsWorkString;

	CapsWorkString->Format("Depth bias support: %s\r\n", SupportZBias ? "Yes" : "No");
	VideoCardSpecString += *CapsWorkString;

	CapsWorkString->Format("Gamma support: %s\r\n", supportGamma ? "Yes" : "No");
	VideoCardSpecString += *CapsWorkString;

	CapsWorkString->Format("Anisotropic filtering support: %s\r\n", SupportAnisotropicFiltering ? "Yes" : "No");
	VideoCardSpecString += *CapsWorkString;

	Check_Texture_Format_Support(Format,Caps);
	Check_Render_To_Texture_Support(Format,Caps);
	Check_Texture_Compression_Support(Caps);
	Check_Bumpmap_Support(Caps);
	Check_Shader_Support(Caps);
	Check_Driver_Version_Status();

	MaxTexturesPerPass = Caps.MaxSimultaneousTextures;
	CapsWorkString->Format("Max textures per pass: %d\r\n",MaxTexturesPerPass);
	VideoCardSpecString += *CapsWorkString;

	if (DX8Wrapper::D3DDevice)
	{
		unsigned int aa = DX8Wrapper::AA;
		if (!aa)
		{
			CapsWorkString->Format("Multisample Anti-Aliasing setting: None\r\n");
		}
		else
		{
			CapsWorkString->Format("Multisample Anti-Aliasing setting: %dx\r\n",aa);
		}
	}
	VideoCardSpecString += *CapsWorkString;
	CapsWorkString->Format("VSync enabled: %d\r\n",Get_Registry_Int("VSync",1));
	VideoCardSpecString += *CapsWorkString;
	if ((VertexShaderVersion) && (PixelShaderVersion))
	{
		CapsWorkString->Format("Shader Detail: %d\r\n",Get_Registry_Int("ShaderDetail",2));
		CapsWorkString->Format("Post Processing Detail: %d\r\n",Get_Registry_Int("SceneShaderDetail",0));
	}
	else
	{
		CapsWorkString->Format("Shaders not supported\r\n");
	}
	VideoCardSpecString += *CapsWorkString;
	CapsWorkString->Format("High quality shadows enabled: %d\r\n",Get_Registry_Int("HighQualityShadows",1));
	VideoCardSpecString += *CapsWorkString;
	Vendor_Specific_Hacks(Adapter);
	delete CapsWorkString;
}

void DX8Caps::Check_Bumpmap_Support(const D3DCAPS8 &Caps)
{
	SupportBumpEnvmap =	(Caps.TextureOpCaps & D3DTEXOPCAPS_BUMPENVMAP) != 0;
	SupportBumpEnvmapLuminance = (Caps.TextureOpCaps & D3DTEXOPCAPS_BUMPENVMAPLUMINANCE) != 0;

	CapsWorkString->Format("Bumpmap support: %s\r\n", SupportBumpEnvmap ? "Yes" : "No");
	VideoCardSpecString += *CapsWorkString;

	CapsWorkString->Format("Bumpmap luminance support: %s\r\n", SupportBumpEnvmapLuminance ? "Yes" : "No");
	VideoCardSpecString += *CapsWorkString;
}

void DX8Caps::Check_Texture_Compression_Support(const D3DCAPS8 &Caps)
{
	SupportDXTC = (SupportTextureFormat[24] | SupportTextureFormat[23] | SupportTextureFormat[22] | SupportTextureFormat[21] | SupportTextureFormat[20]);
	char *c;
	if (SupportDXTC)
	{
		c = "Yes";
	}
	else
	{
		c = "No";
	}
	CapsWorkString->Format("Texture compression support: %s\r\n",c);
	VideoCardSpecString += *CapsWorkString;
}

void Get_WW3D_Format_Name(WW3DFormat Format,StringClass &str)
{
	switch(Format)
	{
	case WW3D_FORMAT_R8G8B8:
		str = "R8G8B8";
		break;
	case WW3D_FORMAT_A8R8G8B8:
		str = "A8R8G8B8";
		break;
	case WW3D_FORMAT_X8R8G8B8:
		str = "X8R8G8B8";
		break;
	case WW3D_FORMAT_R5G6B5:
		str = "R5G6B5";
		break;
	case WW3D_FORMAT_X1R5G5B5:
		str = "X1R5G5B5";
		break;
	case WW3D_FORMAT_A1R5G5B5:
		str = "A1R5G5B5";
		break;
	case WW3D_FORMAT_A4R4G4B4:
		str = "A4R4G4B4";
		break;
	case WW3D_FORMAT_R3G3B2:
		str = "R3G3B2";
		break;
	case WW3D_FORMAT_A8:
		str = "A8";
		break;
	case WW3D_FORMAT_A8R3G3B2:
		str = "A8R3G3B2";
		break;
	case WW3D_FORMAT_X4R4G4B4:
		str = "X4R4G4B4";
		break;
	case WW3D_FORMAT_A8P8:
		str = "A8P8";
		break;
	case WW3D_FORMAT_P8:
		str = "P8";
		break;
	case WW3D_FORMAT_L8:
		str = "L8";
		break;
	case WW3D_FORMAT_A8L8:
		str = "A8L8";
		break;
	case WW3D_FORMAT_A4L4:
		str = "A4L4";
		break;
	case WW3D_FORMAT_U8V8:
		str = "U8V8";
		break;
	case WW3D_FORMAT_L6V5U5:
		str = "L6V5U5";
		break;
	case WW3D_FORMAT_X8L8V8U8:
		str = "X8L8V8U8";
		break;
	case WW3D_FORMAT_DXT1:
		str = "DXT1";
		break;
	case WW3D_FORMAT_DXT2:
		str = "DXT2";
		break;
	case WW3D_FORMAT_DXT3:
		str = "DXT3";
		break;
	case WW3D_FORMAT_DXT4:
		str = "DXT4";
		break;
	case WW3D_FORMAT_DXT5:
		str = "DXT5";
		break;
	default:
		str = "Unknown";
		break;
	}
}

void DX8Caps::Check_Texture_Format_Support(WW3DFormat Format,const D3DCAPS8 &Caps)
{
	if (!Format)
	{
		memset(SupportTextureFormat,0,sizeof(SupportTextureFormat));
		return;
	}
	D3DFORMAT D3DFormat = WW3DFormat_To_D3DFormat(Format);
	for (unsigned int i = 0;i < 25;i++)
	{
		if (!i)
		{
			SupportTextureFormat[0] = 0;
		}
		else
		{
			SupportTextureFormat[i] = SUCCEEDED(D3D->CheckDeviceFormat(Caps.AdapterOrdinal,Caps.DeviceType,D3DFormat,0,D3DRTYPE_TEXTURE,WW3DFormat_To_D3DFormat((WW3DFormat)i)));
			if (SupportTextureFormat[i])
			{
				StringClass *tempstring = new StringClass(0,false);
				Get_WW3D_Format_Name((WW3DFormat)i,*tempstring);
				CapsWorkString->Format("Supports texture format: %s\r\n",*tempstring);
				VideoCardSpecString += *CapsWorkString;
				delete tempstring;
			}
		}
	}
}

void DX8Caps::Check_Render_To_Texture_Support(WW3DFormat Format,const D3DCAPS8 &Caps)
{
	if (!Format)
	{
		memset(SupportRenderToTextureFormat,0,sizeof(SupportRenderToTextureFormat));
		return;
	}
	D3DFORMAT D3DFormat = WW3DFormat_To_D3DFormat(Format);
	for (unsigned int i = 0;i < 25;i++)
	{
		if (!i)
		{
			SupportRenderToTextureFormat[0] = 0;
		}
		else
		{
			SupportRenderToTextureFormat[i] = SUCCEEDED(D3D->CheckDeviceFormat(Caps.AdapterOrdinal,Caps.DeviceType,D3DFormat,1,D3DRTYPE_TEXTURE,WW3DFormat_To_D3DFormat((WW3DFormat)i)));
			if (SupportRenderToTextureFormat[i])
			{
				StringClass *tempstring = new StringClass(0,false);
				Get_WW3D_Format_Name((WW3DFormat)i,*tempstring);
				CapsWorkString->Format("Supports render-to-texture format: %s\r\n",*tempstring);
				VideoCardSpecString += *CapsWorkString;
				delete tempstring;
			}
		}
	}
}

void DX8Caps::Check_Shader_Support(const D3DCAPS8 &Caps)
{
	VertexShaderVersion = Caps.VertexShaderVersion;
	PixelShaderVersion = Caps.PixelShaderVersion;
	CapsWorkString->Format("Vertex shader version: %d.%d, pixel shader version: %d.%d\r\n",HIBYTE(LOWORD(Caps.VertexShaderVersion)),LOBYTE(LOWORD(Caps.VertexShaderVersion)),HIBYTE(LOWORD(Caps.PixelShaderVersion)),LOBYTE(LOWORD(Caps.PixelShaderVersion)));
	VideoCardSpecString += *CapsWorkString;
}

void DX8Caps::Check_Driver_Version_Status()
{
	DriverStatus = 0;
	switch(VendorNumber)
	{
	case 1:
		if ((!DriverFilename.Compare_No_Case("nv4.dll")) && (DriverBuildNum != 327))
		{
			DriverStatus = 3;
		}
		if ((!DriverFilename.Compare_No_Case("nv4_disp.dll")) || (!DriverFilename.Compare_No_Case("nvdd32.dll")))
		{
			if (DriverBuildNum < 2000)
			{
				DriverStatus = 3;
			}
			else
			{
				switch (DriverBuildNum)
				{
				case 2181:
				case 2183:
				case 2240:
					DriverStatus = 2;
					break;
				case 2311:
					DriverStatus = 3;
					break;
				default:
					DriverStatus = 1;
					break;
				}
			}
		}
		if ((!DriverFilename.Compare_No_Case("egdad.dll")) || (!DriverFilename.Compare_No_Case("egliid.dll")))
		{
			DriverStatus = 2;
		}
		break;
	case 2:
		if (!DriverFilename.Compare_No_Case("ati2dvag.dll"))
		{
			switch(DriverBuildNum)
			{
			case 3063:
			case 3273:
			case 3276:
				DriverStatus = 3;
				break;
			case 3281:
				DriverStatus = 2;
				break;
			default:
				DriverStatus = 1;
				break;
			}
		}
		if (!DriverFilename.Compare_No_Case("atid32ae.dll"))
		{
			if (DriverBuildNum == 1010)
			{
				DriverStatus = 2;
			}
		}
		break;
	case 3:
		DriverStatus = 1;
		break;
	case 5:
		if (!DriverFilename.Compare_No_Case("pmx2hal.dll"))
		{
			DriverStatus = 0;
		}
		break;
	case 7:
		DriverStatus = 3;
		break;
	}
	switch (DriverStatus)
	{
	case 1:
		CapsWorkString->Format("Driver version status: Good\r\n");
		break;
	case 2:
		CapsWorkString->Format("Driver version status: OK (No known problems)\r\n");
		break;
	case 3:
		CapsWorkString->Format("Driver version status: Bad (Driver update recommended)\r\n");
		break;
	case 0:
		CapsWorkString->Format("Driver version status: Unknown\r\n");
		break;
	}
	VideoCardSpecString += *CapsWorkString;
}

bool DX8Caps::Is_Valid_Display_Format(int width,int height,WW3DFormat Format)
{
	if ((WidthLimit) || (HeightLimit))
	{
		if ((width > WidthLimit) || (height > HeightLimit))
		{
			return false;
		}
	}
	return true;
}

void DX8Caps::Vendor_Specific_Hacks(const D3DADAPTER_IDENTIFIER9 &Adapter)
{
	if (VendorNumber == 1) //NVIDIA
	{
		/* // This might actually happen before too long
		if (SupportNPatches)
		{
			CapsWorkString->Format("NVidia Driver reported N-Patch support, disabling.\r\n");
			VideoCardSpecString += *CapsWorkString;
		}
		*/
		if (HIBYTE(LOWORD(Caps.VertexShaderVersion)) < 2)
		{
			if (SupportTextureFormat[20])
			{
				CapsWorkString->Format("Disabling DXT1 support on older NVidia hardware.\r\n");
				VideoCardSpecString += *CapsWorkString;
			}
			SupportTextureFormat[20] = false;
			SupportDXTC = (SupportTextureFormat[24] | SupportTextureFormat[23] | SupportTextureFormat[22] | SupportTextureFormat[21] | SupportTextureFormat[20]);
		}
	}
	if (VendorNumber == 2) // ATi
	{
		if ((DeviceNumber == 14) || (DeviceNumber == 15))
		{
			CapsWorkString->Format("Disabling multitexturing on ATI Rage Pro\r\n");
			VideoCardSpecString += *CapsWorkString;
			CapsWorkString->Format("Disabling render-to-texture on Rage Pro\r\n");
			VideoCardSpecString += *CapsWorkString;
			MaxTexturesPerPass = 1;
			SupportMultiTexture = false;
			memset(SupportRenderToTextureFormat,0,sizeof(SupportRenderToTextureFormat));
		}
		if (DeviceNumber == 10)
		{
			CapsWorkString->Format("Disabling multitexturing on ATI Rage 128 Pro GL\r\n");
			VideoCardSpecString += *CapsWorkString;
			CapsWorkString->Format("Disabling render-to-texture on ATI Rage 128 Pro GL\r\n");
			VideoCardSpecString += *CapsWorkString;
			MaxTexturesPerPass = 1;
			SupportMultiTexture = false;
			memset(SupportRenderToTextureFormat,0,sizeof(SupportRenderToTextureFormat));
		}
		switch (DeviceNumber)
		{
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
			CapsWorkString->Format("Maximum screen resolution limited to 1280 x 1024 on ATI Rage 128 cards\r\n");
			VideoCardSpecString += *CapsWorkString;
			WidthLimit = 1280;
			HeightLimit = 1024;
			break;
		}
		switch (DeviceNumber)
		{
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
			CapsWorkString->Format("Disabling render-to-texture on Radeon\r\n");
			VideoCardSpecString += *CapsWorkString;
			memset(SupportRenderToTextureFormat,0,sizeof(SupportRenderToTextureFormat));
			break;
		}
		if (DeviceNumber == 18)
		{
			CapsWorkString->Format("Disabling anisotropic filtering on Radeon VE\r\n");
			VideoCardSpecString += *CapsWorkString;
			SupportAnisotropicFiltering = 0;
		}
	}
	if (VendorNumber == 7) //3Dfx
	{
		if ((DeviceNumber == 2) || (DeviceNumber == 3) || (DeviceNumber == 4))
		{
			CapsWorkString->Format("Disabling multitexturing on Voodoo2/Voodoo3/Banshee\r\n");
			VideoCardSpecString += *CapsWorkString;
			CapsWorkString->Format("Maximum screen resolution limited to 1280 x 1024 on Voodoo2/Voodoo3/Banshee\r\n");
			VideoCardSpecString += *CapsWorkString;
			MaxTexturesPerPass = 1;
			WidthLimit = 1280;
			HeightLimit = 1024;
		}
		if (DeviceNumber == 2)
		{
			CapsWorkString->Format("Disabling render-to-texture on Voodoo3\r\n");
			VideoCardSpecString += *CapsWorkString;
			memset(SupportRenderToTextureFormat,0,sizeof(SupportRenderToTextureFormat));
		}
	}
	if (VendorNumber == 5) // PowerVR
	{
		CapsWorkString->Format("Maximum screen resolution limited to 1280 x 1024 on PowerVR Kyro cards\r\n");
		VideoCardSpecString += *CapsWorkString;
		WidthLimit = 1280;
		HeightLimit = 1024;
		SupportFog = false;
	}
	if (VendorNumber == 4) // S3
	{
		if (DeviceNumber == 2)
		{
			CapsWorkString->Format("Maximum screen resolution limited to 1024 x 768 on S3 Savage 4 cards\r\n");
			VideoCardSpecString += *CapsWorkString;
			WidthLimit = 800;
			HeightLimit = 600;
		}
		if (DeviceNumber == 3)
		{
			CapsWorkString->Format("Disabling multitexturing on S3 Savage 2000\r\n");
			VideoCardSpecString += *CapsWorkString;
			MaxTexturesPerPass = 1;
			SupportMultiTexture = false;
		}
	}
}
