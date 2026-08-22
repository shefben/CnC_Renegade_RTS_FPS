#ifndef TT_INCLUDE__DAZZLERENDEROBJCLASS_H
#define TT_INCLUDE__DAZZLERENDEROBJCLASS_H
#include "RenderObjClass.h"


class INIClass;
class DazzleInitClass;
class LensflareInitClass;
class DazzleVisibilityClass;
class DazzleRenderObjClass;
class SphereClass;
class AABoxClass;
class DazzleLayerClass;
class RenderInfoClass;
class CameraClass;
class Matrix3D;
class SpecialRenderInfoClass;
class Vector3;
class DazzleTypeClass;
class LensflareTypeClass;

class DazzleRenderObjClass : public RenderObjClass
{
public:
	DazzleRenderObjClass* succ; //A0
	unsigned int type; //A4
	float current_dazzle_intensity; //A8
	float current_dazzle_size; //AC
	float current_halo_intensity; //B0
	float current_distance; //B4
	Vector4 transformed_loc; //B8,BC,C0,C4
	Vector3 current_vloc; //C8,CC,D0
	Vector3 current_dir; //D4,D8,DC
	Vector3 dazzle_color; //E0,E4,E8
	Vector3 halo_color; //EC,F0,F4
	float lensflare_intensity; //F8
	float visibility; //FC
	bool on_list; //100
	float radius; //104
	unsigned int creation_time; //108
	static void Init_From_INI(INIClass const*);
	static void Init_Type(DazzleInitClass const&);
	static void Init_Lensflare(LensflareInitClass const&);
	static void Deinit();
	static void Install_Dazzle_Visibility_Handler(DazzleVisibilityClass const*);
	static void Set_Current_Dazzle_Layer(DazzleLayerClass*);
	static unsigned int Get_Type_ID(char const*);
	static char* Get_Type_Name(unsigned int);
	static DazzleTypeClass* Get_Type_Class(unsigned int);
	static unsigned int Get_Lensflare_ID(char const*);

	DazzleRenderObjClass(unsigned int);
	SHADERS_API DazzleRenderObjClass(char const*);
	DazzleRenderObjClass(DazzleRenderObjClass const&);
	DazzleRenderObjClass& operator=(DazzleRenderObjClass const&z);
	void Get_Obj_Space_Bounding_Sphere(SphereClass&) const;
	void Get_Obj_Space_Bounding_Box(AABoxClass&) const;
	void Set_Layer(DazzleLayerClass*);
	RenderObjClass* Clone() const;
	void Render(RenderInfoClass&);
	void Render_Dazzle(CameraClass*);
	void Set_Transform(const Matrix3D&);
	static LensflareTypeClass* Get_Lensflare_Class(unsigned int);
	void vis_render_dazzle(SpecialRenderInfoClass&);
	void Special_Render(SpecialRenderInfoClass&);
	const PersistFactoryClass &Get_Factory() const;
	~DazzleRenderObjClass();
	DazzleRenderObjClass* Succ();
	int Class_ID() const;
	void Scale(float);
	void Set_Dazzle_Color(Vector3 const&color)
	{
		dazzle_color = color;
	}
	void Set_Halo_Color(Vector3 const&);
	void Set_Lensflare_Intensity(float intensity)
	{
		lensflare_intensity = intensity;
	}
	unsigned int Get_Dazzle_Type();
	void Enable_Dazzle_Rendering(bool);
	bool Is_Dazzle_Rendering_Enabled();

};



#endif