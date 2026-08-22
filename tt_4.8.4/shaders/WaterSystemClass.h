#ifndef _WATERSYSTEMCLASS_H_
#define _WATERSYSTEMCLASS_H_

class WaterSystemClass: public RefCountClass
{
protected:
	PlaneClass		WaterPlane;

	Vector3			UnderwaterColor;
	int				UnderwaterFogMode;
	float			UnderwaterFogStart;
	float			UnderwaterFogEnd;
	float			UnderwaterFogDensity;

	TextureClass*	UnderwaterTexture;
	//TextureClass*	ReflectionTexture;

	bool			Dirty;

	SHADERS_API static WaterSystemClass*	Instance;
public:
	WaterSystemClass(): 
		WaterPlane(0, 0, 1, 0),
		UnderwaterColor(Vector3(0.25f, 0.2f, 0.01f)), 
		UnderwaterFogMode(D3DFOG_LINEAR),
		UnderwaterFogStart(10.0f), 
		UnderwaterFogEnd(25.0f),
		UnderwaterFogDensity(0.2f),
		UnderwaterTexture(NULL)
	{
		/* nothing */
		if (!Instance) Instance = this;
	};

	WaterSystemClass(const WaterSystemClass& that):
		WaterPlane(that.WaterPlane),
		UnderwaterColor(that.UnderwaterColor), 
		UnderwaterFogMode(that.UnderwaterFogMode),
		UnderwaterFogStart(that.UnderwaterFogStart), 
		UnderwaterFogEnd(that.UnderwaterFogEnd),
		UnderwaterFogDensity(that.UnderwaterFogDensity),
		UnderwaterTexture(NULL)

	{
		REF_PTR_SET(UnderwaterTexture, that.UnderwaterTexture);
	};
	
	~WaterSystemClass()
	{
		REF_PTR_RELEASE(UnderwaterTexture);
		//REF_PTR_RELEASE(ReflectionTexture);
	};

	static WaterSystemClass*& Get_Instance()
	{
		return Instance;
	}

	SHADERS_API void Load_From_INI(const char* section, INIClass* ini);

	PlaneClass Get_Water_Plane()
	{
		return WaterPlane;
	}

	Vector3 Get_Underwater_Fog_Color()
	{
		return UnderwaterColor;
	}

	int Get_Underwater_Fog_Mode()
	{
		return UnderwaterFogMode;
	}

	float Get_Underwater_Fog_Start()
	{
		return UnderwaterFogStart;
	}

	float Get_Underwater_Fog_End()
	{
		return UnderwaterFogEnd;
	}

	float Get_Underwater_Fog_Density()
	{
		return UnderwaterFogDensity;
	}

	TextureClass* Peek_Underwater_Texture()
	{
		return UnderwaterTexture;
	}

	void Set_Water_Plane(PlaneClass plane)
	{
		WaterPlane = plane;
		Dirty = true;
	}

	void Set_Underwater_Fog_Color(Vector3 color)
	{
		UnderwaterColor = color;
		Dirty = true;
	}

	void Set_Underwater_Fog_Mode(int mode)
	{
		UnderwaterFogMode = mode;
		Dirty = true;
	}

	void Set_Underwater_Fog_Start(float start)
	{
		UnderwaterFogStart = start;
		Dirty = true;
	}

	void Set_Underwater_Fog_End(float end)
	{
		UnderwaterFogEnd = end;
		Dirty = true;
	}

	void Set_Underwater_Fog_Density(float density)
	{
		UnderwaterFogDensity = density;
		Dirty = true;
	}

	void Set_Underwater_Texture(TextureClass* texture)
	{
		REF_PTR_SET(UnderwaterTexture, texture);
	}

	void Set_Reflection_Texture(TextureClass* texture);

};





#endif