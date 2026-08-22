#ifndef TT_INCLUDE__SYSTEMSETTINGENTRYTEXTUREFILTERMODE_H
#define TT_INCLUDE__SYSTEMSETTINGENTRYTEXTUREFILTERMODE_H



#include "SystemSettingEntryEnum.h"
#include "WW3D.h"



class SystemSettingEntryTextureFilterMode :
	public SystemSettingEntryEnum
{

public:

	enum MODE
	{
		MODE_Bilinear,
		MODE_Trilinear,
		MODE_Anisotropic2x,
		MODE_Anisotropic4x,
		MODE_Anisotropic8x,
		MODE_Anisotropic16x,
		MODE_Count
	};
	
	virtual const char* Get_Name() { return "Texture_Filter_Mode"; }
	virtual const char* Get_Help() { return "TEXTURE_FILTER_MODE <mode> - 0=bilinear 1=trilinear 2=anisotropic 2x 3=anisotropic 4x 4=anisotropic 8x 5=anisotropic 16x"; }
	virtual int Get_Enum() { return WW3D::TextureFilter; }
	virtual void Set_Enum(int value) { WW3D::Set_Texture_Filter(value); }
	virtual int Get_Enum_Count() { return MODE_Count; }
	virtual const char* Get_Enum_Name(int value);

};



#endif