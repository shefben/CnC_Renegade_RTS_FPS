#include "General.h"
#include "WW3DAssetManager.h"
#include "viewerctrl.h"
#include "RenderObjClass.h"
#include "dialogbase.h"
#include "dialogmgr.h"
#include "stylemgr.h"
#include "SimpleSceneClass.h"
#include "CameraClass.h"
#include "LightClass.h"
#include "mousemgr.h"
#include "ww3d.h"
#include "Quaternion.h"
#include "wwmath.h"


void ViewerCtrlClass::Set_Model(const char *model)
{
	Set_Model(WW3DAssetManager::TheInstance->Create_Render_Obj(model));
}

void ViewerCtrlClass::Set_Model(RenderObjClass *model)
{
	if (Model)
	{
		Model->Remove();
		Model->Release_Ref();
		Model = 0;
	}
	Model = model;
	ADVISE_NOTIFY(On_ViewerCtrl_Model_Loaded(this,ID,Model));
	Model->Set_LOD_Level(max(Model->Get_LOD_Count() - 1,0));
	Model->Set_Transform(Matrix3D::Identity);
	Model->Add((SceneClass *)Scene);
	IsCameraDirty = true;
}

void ViewerCtrlClass::Set_Animation(const char *animation)
{
	if (Model)
	{
		if (*animation)
		{
			HAnimClass *anim = WW3DAssetManager::TheInstance->Get_HAnim(animation);
			if (anim)
			{
				Model->Set_Animation(anim,0,1);
				Model->Release_Ref();
			}
		}
		else
		{
			Model->Set_Animation();
		}
		IsCameraDirty = true;
	}
}

void ViewerCtrlClass::Set_Interface_Mode(InterfaceModeEnum mode, float rotationrate)
{
	if (InterfaceMode != mode)
	{
		InterfaceMode = mode;
		ZRotation = 0;
	}
	RotationRate = rotationrate * 0.017453292f;
}

ViewerCtrlClass::ViewerCtrlClass() : Scene(0), Camera(0), Model(0), Distance(0), ZRotation(0), RotationRate(0), MinCameraDist(0), IsCameraDirty(true), IsBackgroundVisible(true), InterfaceMode(Z_ROTATION)
{
	LastMousePosition = DialogMgrClass::Get_Last_Mouse_Pos();
	StyleMgrClass::Configure_Renderer(&ControlRenderer);
	Scene = new SimpleSceneClass();
	Camera = new CameraClass();
	Camera->Set_Clip_Planes(0.25,300);
	const RectClass& rect = this->Get_Window_Rect();
	Camera->Set_Aspect_Ratio(rect.Width() / rect.Height());
	Light = new LightClass(LightClass::POINT);
	Light->Intensity = 1.0;
}
ViewerCtrlClass::~ViewerCtrlClass()
{
	if (Model)
	{
		Model->Remove();
		Model->Release_Ref();
		Model = 0;
	}
	if (Light)
	{
		Light->Release_Ref();
		Light = 0;
	}
	if (Camera)
	{
		Camera->Release_Ref();
		Camera = 0;
	}
	if (Scene)
	{
		Scene->Release_Ref();
		Scene = 0;
	}
}
void ViewerCtrlClass::Render(void)
{
	if (IsDirty)
	{
		Create_Control_Renderer();
	}
	if (IsBackgroundVisible)
	{
		ControlRenderer.Render();
	}
	Vector3 v = Vector3(0,0,0);
	WW3D::Render(Scene,Camera,false,false,v);
}
void ViewerCtrlClass::Create_Control_Renderer()
{
	ControlRenderer.Reset();
	ControlRenderer.Enable_Texturing(false);
	ControlRenderer.Set_Z_Value(1.0);
	ShaderClass *s = ControlRenderer.Get_Shader();
	s->Set_Color_Mask(ShaderClass::COLOR_WRITE_ENABLE);
	s->Set_Depth_Mask(ShaderClass::DEPTH_WRITE_ENABLE);
	s->Set_Depth_Compare(ShaderClass::PASS_ALWAYS);
	int linecol = StyleMgrClass::Get_Line_Color();
	int bkcol = StyleMgrClass::Get_Bk_Color();
	if (!IsEnabled)
	{
		linecol = StyleMgrClass::Get_Disabled_Line_Color();
		bkcol = StyleMgrClass::Get_Disabled_Bk_Color();
	}
	ControlRenderer.Add_Rect(Rect,1.0,linecol,bkcol);
}
void ViewerCtrlClass::On_LButton_Down(const Vector2 &mouse_pos)
{
	if (InterfaceMode == VIRTUAL_TRACKBALL)
		RotationRate = 0;
	else
	{
		InterfaceMode = VIRTUAL_TRACKBALL;
		ZRotation = 0;
		RotationRate = 0;
	}
}
void ViewerCtrlClass::On_LButton_Up(const Vector2 &mouse_pos)
{
}
void ViewerCtrlClass::On_Set_Cursor(const Vector2 &mouse_pos)
{
	MouseMgrClass::Set_Cursor(MouseMgrClass::CURSOR_ROTATE);
}



void ViewerCtrlClass::On_Frame_Update()
{
	if (IsCameraDirty)
		Calculate_Camera_Position();
	
	if (InterfaceMode == Z_ROTATION)
	{
		ZRotation -= DialogMgrClass::Get_Frame_Time() / 1000.f * RotationRate;
		
		if (ZRotation < 0.f)
			ZRotation += WWMATH_PI * 2.f;

		Matrix3D cameraTransform = Matrix3D::getXRotationMatrix(WWMATH_PI/2);
		cameraTransform.setPosition(BoundingBox.Center);
		cameraTransform *= Matrix3D::getYRotationMatrix(ZRotation);
		cameraTransform.Translate(Vector3(0, 0, Distance));

		Camera->Set_Transform(cameraTransform);
		Light->Set_Transform(cameraTransform);
	}
	
	else if (InterfaceMode == VIRTUAL_TRACKBALL)
	{
		if (DialogMgrClass::Is_Button_Down(VK_LBUTTON))
		{
			const RectClass& windowRect = Get_Window_Rect();
			const Vector2& mousePos = (const Vector2&)DialogMgrClass::Get_Mouse_Pos();
			
			if (windowRect.Contains(mousePos))
			{
				Vector2i renderTargetResolution;
				int renderTargetBits;
				bool renderTargetWindowed;
				WW3D::Get_Render_Target_Resolution(renderTargetResolution.I, renderTargetResolution.J, renderTargetBits, renderTargetWindowed);
				
				Vector2 viewportSize = Camera->Get_Viewport().Max - Camera->Get_Viewport().Min;
				Vector2 viewportCenter = (Camera->Get_Viewport().Min + Camera->Get_Viewport().Max) / 2.f;
				
				Vector2 oldMousePos = ((Vector2&)LastMousePosition / renderTargetResolution - viewportCenter) / viewportSize;
				Vector2 newMousePos = (mousePos / renderTargetResolution - viewportCenter) / viewportSize;
				
				Quaternion quaternion = Quaternion::Trackball(oldMousePos.X, -oldMousePos.Y, newMousePos.X, -newMousePos.Y, .5f);
				
				Matrix3D cameraTransform = Camera->Get_Transform();
				cameraTransform.setPosition(BoundingBox.Center);
				cameraTransform *= quaternion.Build_Matrix3D();
				cameraTransform.Translate(Vector3(0, 0, Distance));
				
				Camera->Set_Transform(cameraTransform);
				Light->Set_Transform(cameraTransform);
			}
		}
	}
	
	LastMousePosition = DialogMgrClass::Get_Mouse_Pos();
}



void ViewerCtrlClass::Update_Client_Rect()
{
	ClientRect = Rect;
	Vector2 rectSize = Rect.getSize();
	
	const float maxFov = DEG2RAD(45);
	if (rectSize.X >= rectSize.Y)
		Camera->Set_View_Plane(maxFov, maxFov / rectSize.X * rectSize.Y);
	else
		Camera->Set_View_Plane(maxFov / rectSize.Y * rectSize.X, maxFov);
	
	Vector2 screenSize = Render2DClass::Get_Screen_Resolution().getSize();
	Vector2 min = Rect.Upper_Left();
	Vector2 max = Rect.Lower_Right();
	min.Unscale(screenSize);
	max.Unscale(screenSize);
	Camera->Set_Viewport(min, max);
	
	Set_Dirty(true);
}



RENEGADE_FUNCTION
void ViewerCtrlClass::Get_Visible_Bounding_Box(AABoxClass*, RenderObjClass*, bool&)
AT1(0x004F8F20);



void ViewerCtrlClass::Calculate_Camera_Position()
{
	if (Model)
	{
		Model->Update_Obj_Space_Bounding_Volumes();
		Model->Update_Sub_Object_Transforms();
		BoundingBox = AABoxClass(); // Set all members to 0
		
		bool isFirst = true;
		Get_Visible_Bounding_Box(&BoundingBox, Model, isFirst);
		
		Vector2 viewMin;
		Vector2 viewMax;
		Camera->Get_View_Plane(viewMin, viewMax);
		
		Vector2 viewSize = viewMax - viewMin;
		float xyExtentFraction = ((Vector2&)BoundingBox.Extent).Length() / viewSize.X;
		float zExtentFraction = BoundingBox.Extent.Z / viewSize.Y;
		
		Distance = max(xyExtentFraction, zExtentFraction) * (2.f * 1.2f);
		Distance = max(MinCameraDist, Distance);
		
		Matrix3D transform;
		transform.Look_At(BoundingBox.Center + Vector3(Distance, 0, 0), BoundingBox.Center, 0);
		Camera->Set_Transform(transform);
		
		IsCameraDirty = false;
	}
}
