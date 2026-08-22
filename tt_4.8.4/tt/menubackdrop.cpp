#include "General.h"
#include "menubackdrop.h"
#include "ww3d.h"
#include "WW3DAssetManager.h"
#include "HAnimClass.h"
#include "CameraClass.h"
#include "SimpleSceneClass.h"
#include "LightClass.h"
#include "render2d.h"



void MenuBackDropClass::Render()
{
	Vector3 v = Vector3(0,0,0);
	WW3D::Render((SceneClass *)Scene,Camera,ClearScreen,ClearScreen,v);
}



void MenuBackDropClass::Set_Model(const char *name)
{
	Remove_Model();
	Model = WW3DAssetManager::TheInstance->Create_Render_Obj(name);
	if (Model)
	{
		int bone = Model->Get_Bone_Index("CAMERA");
		if (bone > 0)
		{
			Matrix3D transform
			(
				 0,  0, -1,  0,
				-1,  0,  0,  0,
				 0,  1,  0,  0
			);
			Camera->Set_Transform(Model->Get_Bone_Transform(bone) * transform);
		}
		Scene->Add_Render_Object(Model);
	}
	Play_Animation();
}



MenuBackDropClass::MenuBackDropClass() : Scene(0), Camera(0), AnimationName(0), Model(0), Anim(0), ClearScreen(true)
{
	Scene = new SimpleSceneClass();
	Vector3 ambient(1,1,1);
	Scene->Set_Ambient_Light(ambient);
	LightClass *l = new LightClass(LightClass::POINT);
	Vector3 position(0,0,15000);
	l->Set_Position(position);
	l->Intensity = 1.0;
	l->Set_Force_Visible(1);
	l->FarAttenStart = 1000000;
	l->FarAttenEnd = 1000000;
	l->Ambient.Set(0, 0, 0);
	l->Diffuse.Set(1, 1, 1);
	l->Specular.Set(1, 1, 1);
	Scene->Add_Render_Object(l);
	REF_PTR_RELEASE(l);
	Camera = new CameraClass();
	Vector3 pos(0,0,800);
	Camera->Set_Position(pos);

	Camera->Set_View_Plane(DEG2RAD(45.0f), 3.0f / 4.0f * DEG2RAD(45.0f));


	const RectClass& screen_res = Render2DClass::Get_Screen_Resolution();

	float dlg_width = screen_res.Width();
	float dlg_height = screen_res.Height();
	if (screen_res.Width() / screen_res.Height() <= 4.0f / 3.0f)
	{
		// smaller than 4:3, scale height
		dlg_height = screen_res.Width() * 3.0f / 4.0f;
	} 
	else
	{
		// wider than 4:3, scale width
		dlg_width = screen_res.Height() * 4.0f / 3.0f;
	}

	Vector2 screen_center = screen_res.Center();

	RectClass dlg_res;
	dlg_res.Top =		(screen_center.Y - dlg_height / 2) / screen_res.Bottom;
	dlg_res.Bottom =	(screen_center.Y + dlg_height / 2) / screen_res.Bottom;
	dlg_res.Left =		(screen_center.X - dlg_width / 2) / screen_res.Right;
	dlg_res.Right =		(screen_center.X + dlg_width / 2) / screen_res.Right;

	Camera->Set_Viewport(dlg_res.Upper_Left(), dlg_res.Lower_Right());

	Camera->Set_Clip_Planes(5.0, 12000);
}



MenuBackDropClass::~MenuBackDropClass()
{
	Remove_Model();
	REF_PTR_RELEASE(Camera);
	REF_PTR_RELEASE(Scene);
}



void MenuBackDropClass::Remove_Model()
{
	if (Model)
	{
		Model->Remove();
	}
	REF_PTR_RELEASE(Model);
	REF_PTR_RELEASE(Anim);
}



RENEGADE_FUNCTION
void MenuBackDropClass::Set_Animation(const char*)
AT2(0x004E7EB0, 0x004E7750);



RENEGADE_FUNCTION
void MenuBackDropClass::Set_Animation_Percentage(float)
AT2(0x004E7F30, 0x004E77D0);



RENEGADE_FUNCTION
void MenuBackDropClass::Play_Animation()
AT2(0x004E7FC0, 0x004E7860);
