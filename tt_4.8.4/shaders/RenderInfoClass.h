#ifndef TT_INCLUDE__RENDERINFOCLASS_H
#define TT_INCLUDE__RENDERINFOCLASS_H
class CameraClass;
class LightEnvironmentClass;
class MaterialPassClass;
class RenderInfoClass {
public:
	enum RINFO_OVERRIDE_FLAGS {
		RINFO_OVERRIDE_DEFAULT = 0x0,
		RINFO_OVERRIDE_FORCE_TWO_SIDED = 0x1,
		RINFO_OVERRIDE_FORCE_SORTING = 0x2,
		RINFO_OVERRIDE_ADDITIONAL_PASSES_ONLY = 0x4,
		RINFO_OVERRIDE_SHADOW_RENDERING = 0x8,
	};
	CameraClass& Camera; //0
	float fog_scale; //4
	float fog_start; //8
	float fog_end; //12
	LightEnvironmentClass* light_environment; //16
	MaterialPassClass* AdditionalMaterialPassArray[32]; //20
	unsigned int AdditionalMaterialPassCount; //148
	unsigned int SpecialMaterialPassCount; //152
	RINFO_OVERRIDE_FLAGS OverrideFlag[32]; //156
	unsigned int OverrideFlagLevel; //284
	RenderInfoClass& operator=(RenderInfoClass& r); //here to shut the compiler up but never actually called
public:
	RenderInfoClass(CameraClass& cam);
	~RenderInfoClass();
	void Push_Material_Pass(MaterialPassClass* matpass);
	void Pop_Material_Pass();
	int Additional_Pass_Count();
	MaterialPassClass *Peek_Additional_Pass(int i);
	void Push_Override_Flags(RINFO_OVERRIDE_FLAGS flg);
	void Pop_Override_Flags();
	RINFO_OVERRIDE_FLAGS Current_Override_Flags();
};
class VisRasterizerClass;
class BWRenderClass;
class SpecialRenderInfoClass : public RenderInfoClass {
public:
	int RenderType;
	VisRasterizerClass* VisRasterizer;
	BWRenderClass* BWRenderer;
	SpecialRenderInfoClass(CameraClass& cam,int render_type);
	~SpecialRenderInfoClass();
};
#endif
