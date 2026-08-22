#include "General.h"
#include "treectrl.h"
#include "listiconmgr.h"
#include "stylemgr.h"
#include "mousemgr.h"
#include "dialogbase.h"
TreeCtrlClass::TreeCtrlClass() : SelectedItem(0), RowHeight(0), RowsPerPage(0), ScrollPos(0), IsScrollBarDisplayed(false)
{
	StyleMgrClass::Assign_Font(&TextRenderer,StyleMgrClass::FONT_LISTS);
	StyleMgrClass::Configure_Renderer(&ControlRenderer);
	StyleMgrClass::Configure_Renderer(&PlusRenderer);
	StyleMgrClass::Configure_Renderer(&IconRenderer);
	StyleMgrClass::Configure_Renderer(&HilightRenderer);
	StyleMgrClass::Configure_Hilighter(&HilightRenderer);
	PlusRenderer.Set_Texture("if_treeplus.tga");
	TextRenderer.Set_Texture_Size_Hint(256);
	ScrollBarCtrl.Set_Wants_Focus(0);
	ScrollBarCtrl.Set_Advise_Sink(this);
	ScrollBarCtrl.Set_Is_Embedded(true);
}
TreeCtrlClass::~TreeCtrlClass()
{
	ScrollBarCtrl.Set_Advise_Sink(0);
	if (Parent)
	{
		Parent->Remove_Control(&ScrollBarCtrl);
	}
	IconMgr.Reset_Icons();
}
void TreeCtrlClass::Create_Control_Renderers()
{
	ControlRenderer.Reset();
	ControlRenderer.Enable_Texturing(false);
	int linecol = StyleMgrClass::Get_Line_Color();
	int bkcol = StyleMgrClass::Get_Bk_Color();
	if (!IsEnabled)
	{
		linecol = StyleMgrClass::Get_Disabled_Line_Color();
		bkcol = StyleMgrClass::Get_Disabled_Bk_Color();
	}
	ControlRenderer.Add_Rect(Rect,1.0,linecol,bkcol);
}
RENEGADE_FUNCTION
void TreeCtrlClass::Create_Text_Renderers()
AT1(0x00503750);
void TreeCtrlClass::Render(void)
{
	if (IsDirty)
	{
		Create_Control_Renderers();
		Create_Text_Renderers();
	}
	IconMgr.Render_Icons();
	TextRenderer.Render();
	ControlRenderer.Render();
	PlusRenderer.Render();
	HilightRenderer.Render();
	IsDirty = false;
}
void TreeCtrlClass::On_VScroll(ScrollBarCtrlClass *, int, int new_position)
{
	Set_Scroll_Pos(new_position);
}
RENEGADE_FUNCTION
void TreeCtrlClass::On_LButton_Down(const Vector2 &mouse_pos)
AT1(0x00504570);
RENEGADE_FUNCTION
void TreeCtrlClass::On_LButton_DblClk(const Vector2 &mouse_pos)
AT1(0x00504630);
void TreeCtrlClass::On_LButton_Up(const Vector2 &mouse_pos)
{
}
void TreeCtrlClass::On_Mouse_Wheel(int direction)
{
	if (direction <= 0)
	{
		Set_Scroll_Pos(ScrollPos - 1);
	}
	else
	{
		Set_Scroll_Pos(ScrollPos + 1);
	}
}
void TreeCtrlClass::On_Set_Cursor(const Vector2 &mouse_pos)
{
	MouseMgrClass::Set_Cursor(MouseMgrClass::CURSOR_ACTION);
}
void TreeCtrlClass::On_Set_Focus(void)
{
	Set_Dirty(true);
	HasFocus = true;
}
void TreeCtrlClass::On_Kill_Focus(DialogControlClass *focus)
{
	Set_Dirty(true);
	HasFocus = false;
}
RENEGADE_FUNCTION
bool TreeCtrlClass::On_Key_Down(uint32 key_id, uint32 key_data)
AT1(0x005048F0);
void TreeCtrlClass::On_Create(void)
{
}
RENEGADE_FUNCTION
void TreeCtrlClass::Update_Client_Rect(void)
AT1(0x00503F30);
RENEGADE_FUNCTION
void TreeCtrlClass::Set_Scroll_Pos(int new_pos)
AT1(0x00503D10);
RENEGADE_FUNCTION
TreeItemClass *TreeCtrlClass::Insert_Item(const WCHAR *name, const char *icon_name, const char *selected_icon_name, TreeItemClass *parent)
AT1(0x00504D90);
RENEGADE_FUNCTION
void TreeCtrlClass::Delete_Item(TreeItemClass *item)
AT1(0x00504FF0);
RENEGADE_FUNCTION
void TreeCtrlClass::Delete_All_Items(void)
AT1(0x00505120);
void TreeCtrlClass::Select_Item(TreeItemClass *item)
{
	ADVISE_NOTIFY(On_TreeCtrl_Sel_Change(this,ID,SelectedItem,item));
	SelectedItem = item;
	Set_Dirty(true);
}
TreeItemClass *TreeCtrlClass::Get_Selected_Item(void)
{
	return SelectedItem;
}
RENEGADE_FUNCTION
void TreeCtrlClass::Sort_Children_Alphabetically(TreeItemClass *parent)
AT1(0x00504230);
RENEGADE_FUNCTION
void TreeCtrlClass::Sort_Children(TreeItemClass *parent, TREECTRL_SORT_CALLBACK sort_callback, uint32 user_param)
AT1(0x005042A0);
RENEGADE_FUNCTION
TreeItemClass *TreeCtrlClass::Hit_Test(const Vector2 &mouse_pos, HITTYPE &type)
AT1(0x00504400);
RENEGADE_FUNCTION
void TreeCtrlClass::Ensure_Visible(TreeItemClass *tree_item)
AT1(0x00504B70);
