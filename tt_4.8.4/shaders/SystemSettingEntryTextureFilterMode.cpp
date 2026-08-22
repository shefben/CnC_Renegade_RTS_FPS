#include "General.h"
#include "SystemSettingEntryTextureFilterMode.h"



const char* SystemSettingEntryTextureFilterMode::Get_Enum_Name(int value)
{
	switch (value)
	{
		case 0: return "Bilinear";
		case 1: return "Trilinear";
		case 2: return "Anisotropic 2X";
		case 3: return "Anisotropic 4X";
		case 4: return "Anisotropic 8X";
		case 5: return "Anisotropic 16X";
		default: TT_UNREACHABLE;
	}
}
