/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderpowerpanel.cpp               $*
 *                                                                                             *
 *	Roadmap Section 42.  See commanderpowerpanel.h.                                             *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commanderpowerpanel.h"

#include "building.h"
#include "buildingdescriptor.h"
#include "buildingdescriptormanager.h"
#include "commandermodecontroller.h"
#include "commanderselection.h"
#include "commandersidebar.h"
#include "gameobjmanager.h"
#include "render2d.h"
#include "render2dsentence.h"
#include "stylemgr.h"
#include "teampowergridservice.h"
#include "widestring.h"

#include <stdio.h>


//	The cCommanderPowerEvent actions, in that event's own order.  Repeated here rather than
//	included because the event is Commando and this model is Combat.
enum { REQUEST_SET_ENABLED = 0, REQUEST_SET_PRIORITY = 1, REQUEST_CLEAR_PRIORITY = 2 };


bool CommanderPowerPanel::Build_State(int building_id,CommanderPowerPanelState * out)
{
	if (out == nullptr) return false;
	*out = CommanderPowerPanelState ();
	if (building_id == 0) return false;

	ScriptableGameObj * scriptable = GameObjManager::Find_ScriptableGameObj (building_id);
	BuildingGameObj * building = (scriptable != nullptr) ? scriptable->As_BuildingGameObj () : nullptr;
	if ((building == nullptr) || building->Is_Destroyed ()) return false;

	out->Valid			= true;
	out->BuildingId	= building_id;
	out->Team			= building->Get_Player_Type ();
	out->Powered		= building->Is_Power_Enabled ();
	out->Name			= building->Get_Definition ().Get_Name ();

	const StringClass & descriptor_name = building->Get_Definition ().Get_Descriptor_Name ();
	const BuildingModelDescriptor * descriptor = (descriptor_name.Get_Length () > 0)
		? BuildingDescriptorManager::Get_Descriptor (descriptor_name.Peek_Buffer ())
		: nullptr;

	if (descriptor != nullptr) {
		out->HasDescriptor		= true;
		out->RequiresPower		= descriptor->Power.RequiresPower;
		out->Generation			= descriptor->Power.Generation;
		out->Consumption			= descriptor->Power.Consumption;
		out->DescriptorPriority	= descriptor->Power.Priority;
		if (descriptor->Build.DisplayName.Get_Length () > 0) {
			out->Name = descriptor->Build.DisplayName;
		}
	}

	out->ManualDisabled = TeamPowerGridService::Is_Manual_Disabled (building_id);

	int override_priority = 0;
	out->HasPriorityOverride = TeamPowerGridService::Get_Priority_Override (building_id,&override_priority);
	out->EffectivePriority = out->HasPriorityOverride ? override_priority : out->DescriptorPriority;

	out->TeamGeneration	= TeamPowerGridService::Get_Generation (out->Team);
	out->TeamRequested	= TeamPowerGridService::Get_Requested_Demand (out->Team);
	out->TeamAllocated	= TeamPowerGridService::Get_Allocated_Demand (out->Team);
	return true;
}


bool CommanderPowerPanel::Get_Request(CommanderPowerPanelButtonType button,const CommanderPowerPanelState & state,
												  int * out_action,int * out_value)
{
	if (!state.Valid || !state.HasDescriptor) return false;

	int action = 0;
	int value = 0;

	switch (button) {

		case POWER_PANEL_TOGGLE:
			action	= REQUEST_SET_ENABLED;
			value		= state.ManualDisabled ? 1 : 0;		// off by hand -> turn it on, and vice versa
			break;

		case POWER_PANEL_PRIORITY_DOWN:
			action	= REQUEST_SET_PRIORITY;
			value		= state.EffectivePriority - PRIORITY_STEP;
			if (value < 0) value = 0;
			break;

		case POWER_PANEL_PRIORITY_UP:
			action	= REQUEST_SET_PRIORITY;
			value		= state.EffectivePriority + PRIORITY_STEP;
			break;

		case POWER_PANEL_PRIORITY_RESET:
			if (!state.HasPriorityOverride) return false;
			action	= REQUEST_CLEAR_PRIORITY;
			break;

		default:
			return false;
	}

	if (out_action != nullptr) *out_action = action;
	if (out_value != nullptr)  *out_value = value;
	return true;
}


const char * CommanderPowerPanel::Get_Button_Label(CommanderPowerPanelButtonType button,
																	const CommanderPowerPanelState & state)
{
	switch (button) {
		case POWER_PANEL_TOGGLE:				return state.ManualDisabled ? "POWER ON" : "POWER OFF";
		case POWER_PANEL_PRIORITY_DOWN:		return "PRIORITY -";
		case POWER_PANEL_PRIORITY_UP:			return "PRIORITY +";
		case POWER_PANEL_PRIORITY_RESET:		return "DEFAULT";
		default:										return "";
	}
}


/***********************************************************************************************
 * CommanderPowerPanelRenderer                                                                   *
 *                                                                                               *
 *	Same construction as CommanderCommandPaletteRenderer -- stamped-metal bevels, amber text,     *
 *	rebuilt in Update and replayed in Render, hit-tested against what was last drawn -- and in     *
 *	the same corner, which is free whenever this is open because the grid only opens for units.    *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static bool							_Available	= false;
static bool							_Open			= false;
static Render2DClass *			_Panel		= nullptr;
static Render2DSentenceClass *	_Text			= nullptr;

static bool			_HitValid = false;
static RectClass	_HitPanel;
static RectClass	_HitButtons [POWER_PANEL_BUTTON_COUNT];
static bool			_ButtonLive [POWER_PANEL_BUTTON_COUNT];

static const unsigned int	COLOR_METAL			= RGBA_TO_INT32 ( 66, 66, 62,235);
static const unsigned int	COLOR_BEVEL_LIGHT	= RGBA_TO_INT32 (140,140,132,255);
static const unsigned int	COLOR_BEVEL_DARK	= RGBA_TO_INT32 ( 18, 18, 16,255);
static const unsigned int	COLOR_BUTTON		= RGBA_TO_INT32 ( 42, 42, 40,235);
static const unsigned int	COLOR_AMBER			= RGB_TO_INT32  (252,204, 92);
static const unsigned int	COLOR_AMBER_DIM	= RGB_TO_INT32  (142,116, 56);
static const unsigned int	COLOR_RED			= RGB_TO_INT32  (232, 72, 56);
static const unsigned int	COLOR_SHADOW		= RGB_TO_INT32  (  0,  0,  0);


static void	Add_Bevel(Render2DClass * renderer,const RectClass & rect,unsigned int fill,bool raised,float thickness)
{
	renderer->Add_Quad (rect,fill);

	unsigned int top_left		= raised ? COLOR_BEVEL_LIGHT : COLOR_BEVEL_DARK;
	unsigned int bottom_right	= raised ? COLOR_BEVEL_DARK  : COLOR_BEVEL_LIGHT;

	renderer->Add_Quad (RectClass (rect.Left,rect.Top,rect.Right,rect.Top + thickness),top_left);
	renderer->Add_Quad (RectClass (rect.Left,rect.Top,rect.Left + thickness,rect.Bottom),top_left);
	renderer->Add_Quad (RectClass (rect.Left,rect.Bottom - thickness,rect.Right,rect.Bottom),bottom_right);
	renderer->Add_Quad (RectClass (rect.Right - thickness,rect.Top,rect.Right,rect.Bottom),bottom_right);
}

static bool	Rect_Contains(const RectClass & rect,float x,float y)
{
	return (x >= rect.Left) && (x <= rect.Right) && (y >= rect.Top) && (y <= rect.Bottom);
}

static void	Add_Line(const char * text,const RectClass & rect,unsigned int color)
{
	WideStringClass wide (text,true);
	StyleMgrClass::Render_Text (wide,_Text,color,COLOR_SHADOW,rect,true,true,StyleMgrClass::LEFT_JUSTIFY,true);
}


void CommanderPowerPanelRenderer::Init(bool render_available)
{
	Shutdown ();

	_Available = render_available;
	if (!_Available) return;

	_Panel = new Render2DClass ();
	_Panel->Enable_Texturing (false);
	StyleMgrClass::Configure_Renderer (_Panel);

	_Text = new Render2DSentenceClass ();
	StyleMgrClass::Assign_Font (_Text,StyleMgrClass::FONT_INGAME_TXT);
}


void CommanderPowerPanelRenderer::Shutdown(void)
{
	if (_Panel != nullptr) { delete _Panel; _Panel = nullptr; }
	if (_Text  != nullptr) { delete _Text;  _Text  = nullptr; }

	_Available	= false;
	_Open			= false;
	_HitValid	= false;
}


void CommanderPowerPanelRenderer::Update(void)
{
	if (!_Available || (_Panel == nullptr) || (_Text == nullptr)) return;

	_Panel->Reset ();
	_Text->Reset ();
	_HitValid	= false;
	_Open			= false;

	if (!CommanderModeController::Is_Active ()) return;

	CommanderPowerPanelState state;
	if (!CommanderPowerPanel::Build_State (CommanderSelection::Get_Selected_Building (),&state)) return;

	_Open = true;

	const RectClass & screen = Render2DClass::Get_Screen_Resolution ();

	float sidebar_width = screen.Width () * 0.235f;
	if (sidebar_width < 232.0f) sidebar_width = 232.0f;
	if (!CommanderSidebarModel::Is_Open ()) sidebar_width = 0.0f;

	const float margin		= 8.0f;
	const float padding		= 4.0f;
	const float line			= 18.0f;
	const float button_h		= 30.0f;
	const float width			= 300.0f;
	const int	text_lines	= 5;

	float height = padding + (line * (float)text_lines) + padding + (button_h * 2.0f) + (padding * 3.0f);

	RectClass panel (screen.Right - sidebar_width - margin - width,screen.Bottom - margin - height,
						  screen.Right - sidebar_width - margin,screen.Bottom - margin);
	Add_Bevel (_Panel,panel,COLOR_METAL,true,3.0f);
	_HitPanel = panel;

	float x0 = panel.Left + padding + 4.0f;
	float x1 = panel.Right - padding;
	float y = panel.Top + padding;
	char text [96];

	::snprintf (text,sizeof (text),"%s",state.Name.Peek_Buffer ());
	Add_Line (text,RectClass (x0,y,x1,y + line),COLOR_AMBER);
	y += line;

	if (!state.HasDescriptor) {
		Add_Line ("NOT ON THE COMMANDER GRID",RectClass (x0,y,x1,y + line),COLOR_AMBER_DIM);
		y += line;
	} else if (state.RequiresPower) {
		::snprintf (text,sizeof (text),"DRAW %.0f   PRIORITY %d%s",state.Consumption,state.EffectivePriority,
						state.HasPriorityOverride ? " (SET)" : "");
		Add_Line (text,RectClass (x0,y,x1,y + line),COLOR_AMBER_DIM);
		y += line;
	} else {
		::snprintf (text,sizeof (text),"GENERATES %.0f",state.Generation);
		Add_Line (text,RectClass (x0,y,x1,y + line),COLOR_AMBER_DIM);
		y += line;
	}

	const char * status = state.ManualDisabled ? "SWITCHED OFF" : (state.Powered ? "POWERED" : "NO POWER");
	Add_Line (status,RectClass (x0,y,x1,y + line),(state.Powered && !state.ManualDisabled) ? COLOR_AMBER : COLOR_RED);
	y += line;

	::snprintf (text,sizeof (text),"GRID %.0f / %.0f DEMAND",state.TeamGeneration,state.TeamRequested);
	Add_Line (text,RectClass (x0,y,x1,y + line),COLOR_AMBER_DIM);
	y += line;

	::snprintf (text,sizeof (text),"SUPPLIED %.0f",state.TeamAllocated);
	Add_Line (text,RectClass (x0,y,x1,y + line),COLOR_AMBER_DIM);
	y += line + padding;

	float button_w = (panel.Width () - padding * 3.0f) * 0.5f;
	for (int index = 0; index < POWER_PANEL_BUTTON_COUNT; index++) {

		int column	= index % 2;
		int row		= index / 2;

		RectClass cell (panel.Left + padding + (float)column * (button_w + padding),
							 y + (float)row * (button_h + padding),
							 panel.Left + padding + (float)column * (button_w + padding) + button_w,
							 y + (float)row * (button_h + padding) + button_h);

		CommanderPowerPanelButtonType button = (CommanderPowerPanelButtonType)index;
		_ButtonLive [index] = CommanderPowerPanel::Get_Request (button,state,nullptr,nullptr);
		_HitButtons [index] = cell;

		Add_Bevel (_Panel,cell,COLOR_BUTTON,true,2.0f);
		WideStringClass label (CommanderPowerPanel::Get_Button_Label (button,state),true);
		StyleMgrClass::Render_Text (label,_Text,_ButtonLive [index] ? COLOR_AMBER : COLOR_AMBER_DIM,COLOR_SHADOW,
											 cell,true,true,StyleMgrClass::CENTER_JUSTIFY,true);
	}

	_HitValid = true;
}


void CommanderPowerPanelRenderer::Render(void)
{
	if (!_Available || !_Open || (_Panel == nullptr) || (_Text == nullptr)) return;

	_Panel->Render ();
	_Text->Render ();
}


bool CommanderPowerPanelRenderer::Hit_Test(float x,float y,CommanderPowerPanelButtonType * out_button,
														 bool * out_inside_panel)
{
	if (out_inside_panel != nullptr) *out_inside_panel = false;

	if (!_Available || !_Open || !_HitValid) return false;
	if (!Rect_Contains (_HitPanel,x,y)) return false;

	if (out_inside_panel != nullptr) *out_inside_panel = true;

	for (int index = 0; index < POWER_PANEL_BUTTON_COUNT; index++) {
		if (!_ButtonLive [index]) continue;
		if (!Rect_Contains (_HitButtons [index],x,y)) continue;
		if (out_button != nullptr) *out_button = (CommanderPowerPanelButtonType)index;
		return true;
	}
	return false;
}


bool CommanderPowerPanelRenderer::Is_Open(void)	{ return _Open; }
