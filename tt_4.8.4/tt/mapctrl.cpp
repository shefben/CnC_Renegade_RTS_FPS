#include "General.h"
#include "mapctrl.h"
#include "texture.h"
#include "WW3DAssetManager.h"
#include "stylemgr.h"
MapCtrlClass::MapCtrlClass() : Zoom(1), ScrollPos(0,0), MapSize(0,0), IsDragging(false), IsZoomingIn(false), IsZoomingOut(false), IsUsingOverlay(false), InitialMousePos(0,0), InitialScrollPos(0,0), ZoomInButtonRect(0,0,0,0), ZoomOutButtonRect(0,0,0,0), PulseDirection(-1), OverlayOpacity(1), MapCenterPoint(0,0), MapScale(1,1), CloudVector(0)
{
	StyleMgrClass::Assign_Font(&TextRenderer,StyleMgrClass::FONT_CONTROLS);
	StyleMgrClass::Configure_Renderer(&ControlRenderer);
	StyleMgrClass::Configure_Renderer(&ButtonRenderer);
	StyleMgrClass::Configure_Renderer(&MapRenderer);
	StyleMgrClass::Configure_Renderer(&MapOverlayRenderer);
	StyleMgrClass::Configure_Renderer(&CloudRenderer);
	StyleMgrClass::Configure_Renderer(&EdgeRenderer);
	StyleMgrClass::Configure_Renderer(&IconRenderer);
	EdgeRenderer.Set_Texture("map_edges.tga");
	ButtonRenderer.Set_Texture("mapicons.tga");
}
void MapCtrlClass::Render(void)
{
	Update_Pulse();
	if (IsDirty)
	{
		Create_Cloud_Renderer();
		Create_Control_Renderers();
		Create_Text_Renderers();
	}
	TextRenderer.Render();
	MapRenderer.Render();
	if (IsUsingOverlay)
	{
		MapOverlayRenderer.Render();
	}
	IconRenderer.Render();
	CloudRenderer.Render();
	EdgeRenderer.Render();
	ButtonRenderer.Render();
	ControlRenderer.Render();
	IsDirty = false;
}
RENEGADE_FUNCTION
void MapCtrlClass::Create_Cloud_Renderer()
AT1(0x004FF550);
void MapCtrlClass::Create_Text_Renderers()
{
	TextRenderer.Reset();
}
RENEGADE_FUNCTION
void MapCtrlClass::Create_Control_Renderers()
AT1(0x004FEE40);
RENEGADE_FUNCTION
void MapCtrlClass::On_LButton_Down(const Vector2 &mouse_pos)
AT1(0x004FFEF0);
void MapCtrlClass::On_LButton_Up(const Vector2 &mouse_pos)
{
	Release_Capture();
	IsDragging = false;
	IsZoomingIn = false;
	IsZoomingOut = false;
}
RENEGADE_FUNCTION
void MapCtrlClass::On_Mouse_Move(const Vector2 &mouse_pos)
AT1(0x00500390);
RENEGADE_FUNCTION
void MapCtrlClass::On_Set_Cursor(const Vector2 &mouse_pos)
AT1(0x004FFC70);
void MapCtrlClass::On_Set_Focus(void)
{
	HasFocus = true;
}
void MapCtrlClass::On_Kill_Focus(DialogControlClass *focus)
{
	IsDragging = false;
	IsZoomingIn = false;
	IsZoomingOut = false;
	HasFocus = false;
}
RENEGADE_FUNCTION
void MapCtrlClass::On_Frame_Update(void)
AT1(0x00500440);
RENEGADE_FUNCTION
void MapCtrlClass::Update_Client_Rect(void)
AT1(0x004FFD30);
MapCtrlClass::~MapCtrlClass()
{
	Free_Cloud_Data();
}
void MapCtrlClass::Free_Cloud_Data()
{
	if (CloudVector)
	{
		delete[] CloudVector;
	}
	CloudSize.I = 0;
	CloudSize.J = 0;
}
RENEGADE_FUNCTION
void MapCtrlClass::Set_Map_Texture (const char *filename)
AT1(0x00500070);
RENEGADE_FUNCTION
void MapCtrlClass::Set_Marker_Texture (const char *filename)
AT1(0x005009A4);
RENEGADE_FUNCTION
int MapCtrlClass::Add_Marker (const WCHAR *name, const Vector3 &pos, const RectClass &uv_rect, int color)
AT1(0x00500530);
uint32 MapCtrlClass::Get_Marker_Data (int index)
{
	return MarkerList[index].Get_User_Data();
}
void MapCtrlClass::Set_Marker_Data (int index, uint32 user_data)
{
	MarkerList[index].Set_User_Data(user_data);
}
RENEGADE_FUNCTION
void MapCtrlClass::Remove_Marker (int index)
AT1(0x00500780);
RENEGADE_FUNCTION
void MapCtrlClass::Center_View_About_Marker (int marker_index)
AT1(0x00500A00);
RENEGADE_FUNCTION
void MapCtrlClass::Initialize_Cloud (int cells_x, int cells_y)
AT1(0x00500DA0);
RENEGADE_FUNCTION
void MapCtrlClass::Reset_Cloud (void)
AT1(0x00500E10);
RENEGADE_FUNCTION
void MapCtrlClass::Set_Cloud_Cell (int cell_x, int cell_y, bool is_visible)
AT1(0x00500E50);
RENEGADE_FUNCTION
void MapCtrlClass::Update_Pulse()
AT1(0x00500FF0);
