#include "General.h"
#include "shaders.h"
#include "WaterSystemClass.h"

SHADERS_API WaterSystemClass* WaterSystemClass::Instance;

SHADERS_API void WaterSystemClass::Load_From_INI(const char *section, INIClass *ini)
{
	if (!ini) return;
	char temp[256];
	ini->Get_String(section, "UnderwaterColor", "0.25|0.2|0.01", temp, sizeof(temp));
	Vector3 underwater_color = Vector3(0.25f, 0.2f, 0.01f);
	if (sscanf(temp,"%f|%f|%f", &underwater_color.X, &underwater_color.Y, &underwater_color.Z) == 3) UnderwaterColor = underwater_color;

	UnderwaterFogMode =		ini->Get_Int(section, "UnderwaterFogMode", 3);
	UnderwaterFogStart =	ini->Get_Float(section, "UnderwaterFogStart", 10.0f);
	UnderwaterFogEnd =		ini->Get_Float(section, "UnderwaterFogEnd", 25.0f);
	UnderwaterFogDensity =	ini->Get_Float(section, "UnderwaterFogDensity", 0.5f);
}