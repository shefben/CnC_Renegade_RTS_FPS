#include "General.h"
#include "DazzleRenderObjClass.h"
#include "shaderstatemanager.h"
#include "engine_3d.h"
#include "WW3D.h"
#include "SphereClass.h"
extern int WireframeMode;
void FrameStart();
void FrameEnd();

REF_DEF2(types,char **,0x008306B4,0x0082F89C);

DazzleRenderObjClass::DazzleRenderObjClass(char const *dazzle) : succ(0), type(Get_Type_ID(dazzle)), current_dazzle_intensity(0), current_dazzle_size(0), dazzle_color(1,1,1), halo_color(1,1,1), lensflare_intensity(1), visibility(0), on_list(false)
{
	if ((types) && (types[type]))
	{
		char *dazzle_type = types[type];
		radius = *(float *)(dazzle_type + 0xB4);
	}
	else
	{
		radius = 0;
	}
	creation_time = WW3D::SyncTime;
}

DazzleRenderObjClass::DazzleRenderObjClass(const DazzleRenderObjClass&src) : succ(0), type(src.type), current_dazzle_intensity(src.current_dazzle_intensity), current_dazzle_size(src.current_dazzle_size), current_dir(src.current_dir), dazzle_color(src.dazzle_color), halo_color(src.halo_color), lensflare_intensity(src.lensflare_intensity), visibility(src.visibility), on_list(false), radius(src.radius), creation_time(WW3D::SyncTime)
{
}

RenderObjClass *DazzleRenderObjClass::Clone() const
{
	return new DazzleRenderObjClass(*this);
}

DazzleRenderObjClass::~DazzleRenderObjClass()
{
}

int DazzleRenderObjClass::Class_ID() const
{
	return CLASSID_DAZZLE;
}

RENEGADE_FUNCTION
void DazzleRenderObjClass::Render(RenderInfoClass &)
AT2(0x0053ED30,0x0053E5D0);

RENEGADE_FUNCTION
void DazzleRenderObjClass::Special_Render(SpecialRenderInfoClass &)
AT2(0x005413A0,0x00540C40);

RENEGADE_FUNCTION
void DazzleRenderObjClass::Set_Transform(const Matrix3D &)
AT2(0x00540F10,0x005407B0);

void DazzleRenderObjClass::Get_Obj_Space_Bounding_Sphere(SphereClass &sphere) const
{
	sphere.Center.X = 0;
	sphere.Center.Y = 0;
	sphere.Center.Z = 0;
	sphere.Radius = radius;
}

void DazzleRenderObjClass::Get_Obj_Space_Bounding_Box(AABoxClass &box) const
{
	box.Center.X = 0;
	box.Center.Y = 0;
	box.Center.Z = 0;
	box.Extent.X = radius;
	box.Extent.Y = radius;
	box.Extent.Z = radius;
}

void DazzleRenderObjClass::Scale(float scale)
{
	radius *= scale;
}

RENEGADE_FUNCTION
const PersistFactoryClass &DazzleRenderObjClass::Get_Factory() const
AT2(0x00541730,0x00540FD0);

RENEGADE_FUNCTION
unsigned int DazzleRenderObjClass::Get_Type_ID(const char *)
AT2(0x00540FB0,0x00540850);

void DazzleRenderObjClass::Install_Dazzle_Visibility_Handler(const DazzleVisibilityClass* handler)
{
	if (!handler)
	{
		VisibilityHandler = &DefaultVisibilityHandler;
		StateManager::SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
		StateManager::SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);
		FrameEnd();
	}
	else
	{
		VisibilityHandler = (DazzleVisibilityClass*)handler; // TODO: Get rid of const cast
		StateManager::SetRenderState(D3DRS_FILLMODE, WireframeMode);
		StateManager::SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
		FrameStart();
	}
}

RENEGADE_FUNCTION
void DazzleRenderObjClass::Set_Current_Dazzle_Layer(DazzleLayerClass*)
AT1(0x0053ECF0);

RENEGADE_FUNCTION
void DazzleRenderObjClass::Deinit()
AT1(0x0053E0A0);

RENEGADE_FUNCTION
void DazzleRenderObjClass::Init_From_INI(const INIClass *)
AT1(0x0053D3D0);
