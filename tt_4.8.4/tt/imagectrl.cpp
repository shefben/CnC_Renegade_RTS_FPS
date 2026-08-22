#include "General.h"
#include "imagectrl.h"
#include "stylemgr.h"
void ImageCtrlClass::Set_Texture(const char *texture)
{
	TextureRenderer.Set_Texture(texture);
	Set_Dirty(true);
}
ImageCtrlClass::ImageCtrlClass()
{
	StyleMgrClass::Configure_Renderer(&ControlRenderer);
	StyleMgrClass::Configure_Renderer(&TextureRenderer);
}
ImageCtrlClass::~ImageCtrlClass()
{
}
void ImageCtrlClass::Create_Control_Renderer()
{
	ControlRenderer.Reset();
	ControlRenderer.Enable_Texturing(false);
	int LineColor = StyleMgrClass::Get_Line_Color();
	if (!IsEnabled)
	{
		LineColor = StyleMgrClass::Get_Disabled_Line_Color();
	}
	if ((Style & 1) == 1)
	{
		ControlRenderer.Add_Outline(Rect,1.0,LineColor);
	}
}
void ImageCtrlClass::Create_Texture_Renderer()
{
	TextureRenderer.Reset();
	if (TextureRenderer.Peek_Texture())
	{
		TextureRenderer.Add_Quad(Rect,0xFFFFFFFF);
	}
}
void ImageCtrlClass::Render()
{
	if (IsDirty)
	{
		Create_Control_Renderer();
		Create_Texture_Renderer();
	}
	TextureRenderer.Render();
	ControlRenderer.Render();
}