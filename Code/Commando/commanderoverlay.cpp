/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/commanderoverlay.cpp                $*
 *                                                                                             *
 *	See commanderoverlay.h.  Layout, colours and drawing follow the CnC_Renegade_new fork's     *
 *	commanderoverlay.cpp (Render_Header, Render_Build_Sidebar, Render_Queue_Strip,              *
 *	Render_Tooltip, Render_Selection_Box, Render_Cursor, the minimap panel and the cameo       *
 *	table); the data behind them is this tree's Commander layer.                                *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#include "commanderoverlay.h"

#include "commandersidebar.h"
#include "commanderproduction.h"
#include "commanderselection.h"
#include "commandermodecontroller.h"
#include "commandercommandpalette.h"
#include "commanderminimap.h"
#include "commandercamera.h"
#include "strategicviewmanager.h"
#include "render2d.h"
#include "render2dsentence.h"
#include "stylemgr.h"
#include "directinput.h"
#include "cnetwork.h"
#include "player.h"
#include "gameobjmanager.h"
#include "building.h"
#include "smartgameobj.h"
#include "definitionmgr.h"
#include "definition.h"
#include "assetmgr.h"
#include "texture.h"
#include "playertype.h"
#include "rect.h"
#include "dialogmgr.h"
#include "damageablegameobj.h"
#include "damage.h"
#include "combat.h"
#include "camera.h"
#include "wwstring.h"
#include "widestring.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

using namespace CommanderOverlayConstants;


////////////////////////////////////////////////////////////////
//	Static state
////////////////////////////////////////////////////////////////
bool		CommanderOverlay::_IsInitialized		= false;
Vector2	CommanderOverlay::_CursorPos			(DESIGN_WIDTH * 0.5F, DESIGN_HEIGHT * 0.5F);
int		CommanderOverlay::_HoverSlot			= -1;
int		CommanderOverlay::_HoverPageButton	= 0;
int		CommanderOverlay::_HoverCommand		= -1;
int		CommanderOverlay::_Page					= 0;
int		CommanderOverlay::_PageMode			= -1;

//	Draw order: panels, cameo icons, text, then the tooltip and cursor on top of everything.
static Render2DClass *				_PanelRenderer	= nullptr;
static Render2DSentenceClass *	_TextRenderer	= nullptr;
static Render2DClass *				_TopRenderer	= nullptr;
static Render2DSentenceClass *	_TopText			= nullptr;
static Render2DSentenceClass *	_CurrentText	= nullptr;
static bool								_WasActive		= false;

static const float	MINIMAP_SIZE		= 128.0F;
static const float	MINIMAP_BOTTOM		= DESIGN_HEIGHT - 6.0F;

//	The fork's CommanderHealthOverlay, at its selected size: this tree draws a bar only over
//	what the commander has selected.
static const float			HEALTH_BAR_WIDTH				= 30.0F;
static const float			HEALTH_BAR_HEIGHT				= 4.0F;
static const float			HEALTH_BAR_OFFSET_Y			= 14.0F;
static const float			HEALTH_BAR_OFFSET_Y_BUILDING	= 20.0F;
static const float			HEALTH_BAR_FRAME				= 1.0F;
static const float			HEALTH_NAME_OFFSET_Y			= 8.0F;
static const float			HEALTH_CRITICAL				= 0.30F;
static const float			HEALTH_MID						= 0.65F;
static const unsigned int	COLOR_HEALTH_BACKING			= 0xC0000000;
static const unsigned int	COLOR_HEALTH_FRAME			= 0xFF201810;
static const unsigned int	COLOR_HEALTH_OK				= 0xFF50D040;
static const unsigned int	COLOR_HEALTH_MID				= 0xFFD8C030;
static const unsigned int	COLOR_HEALTH_BAD				= 0xFFE04030;


////////////////////////////////////////////////////////////////
//	Cameo icon cache (the fork's Find_Icon_Renderer)
////////////////////////////////////////////////////////////////
struct OverlayIconStruct
{
	StringClass			Name;
	Render2DClass *	Renderer;
	bool					Missing;
};

static const int			MAX_ICONS	= 64;
static OverlayIconStruct	_Icons[MAX_ICONS];
static int					_IconCount	= 0;

static OverlayIconStruct * Find_Icon (const char *texture_name)
{
	if (texture_name == nullptr || texture_name[0] == 0) {
		return nullptr;
	}

	for (int index = 0; index < _IconCount; index ++) {
		if (_Icons[index].Name.Compare_No_Case (texture_name) == 0) {
			return &_Icons[index];
		}
	}

	if (_IconCount >= MAX_ICONS) {
		return nullptr;
	}

	OverlayIconStruct &icon	= _Icons[_IconCount ++];
	icon.Name					= texture_name;
	icon.Renderer				= nullptr;
	icon.Missing				= true;

	TextureClass *texture = WW3DAssetManager::Get_Instance ()->Get_Texture (texture_name);
	if (texture != nullptr) {
		if (!texture->Is_Missing_Texture ()) {
			icon.Renderer = new Render2DClass;
			StyleMgrClass::Configure_Renderer (icon.Renderer);
			icon.Renderer->Set_Texture (texture);
			icon.Missing = false;
		}
		texture->Release_Ref ();
	}
	return &icon;
}

static void Reset_Icons (void)
{
	for (int index = 0; index < _IconCount; index ++) {
		if (_Icons[index].Renderer != nullptr) {
			_Icons[index].Renderer->Reset ();
		}
	}
}

static void Render_Icons (void)
{
	for (int index = 0; index < _IconCount; index ++) {
		if (_Icons[index].Renderer != nullptr) {
			_Icons[index].Renderer->Render ();
		}
	}
}

static void Release_Icons (void)
{
	for (int index = 0; index < _IconCount; index ++) {
		delete _Icons[index].Renderer;
		_Icons[index].Renderer	= nullptr;
		_Icons[index].Name		= "";
	}
	_IconCount = 0;
}


////////////////////////////////////////////////////////////////
//	Cameo_For_Name -- the fork's preset-name -> hud_rsf cameo table
////////////////////////////////////////////////////////////////
struct CameoKeyStruct
{
	const char *	Key;		// matched against the name with spaces/underscores removed
	const char *	Suffix;
};

//	Order matters: the more specific names come before the ones they contain.
static const CameoKeyStruct _CameoKeys[] =
{
	{ "advancedguardtower",	"agt" },
	{ "agt",						"agt" },
	{ "obelisk",				"obe" },
	{ "samsite",				"sam" },
	{ "sam",						"sam" },
	{ "guardtower",			"gtwr" },
	{ "turret",					"tur" },
	{ "silo",					"sil" },
	{ "airstrip",				"air" },
	{ "handofnod",				"hon" },
	{ "barracks",				"bar" },
	{ "warfactory",			"wf" },
	{ "weaponsfactory",		"wf" },
	{ "constructionyard",	"con" },
	{ "conyard",				"con" },
	{ "powerplant",			"pow" },
	{ "refinery",				"ref" },
	{ "communication",		"com" },
	{ "comcenter",				"com" },
	{ "helipad",				"hpad" },
	{ "repair",					"rep" },
	{ "harvester",				"harv" },
};

static bool Cameo_For_Name (const char *name, int team, StringClass &out_texture)
{
	if (name == nullptr || name[0] == 0) {
		return false;
	}

	char squashed[128];
	int length = 0;
	for (const char *cursor = name; *cursor != 0 && length < (int)sizeof (squashed) - 1; cursor ++) {
		if (*cursor != ' ' && *cursor != '_' && *cursor != '-') {
			squashed[length ++] = (char)::tolower ((unsigned char)*cursor);
		}
	}
	squashed[length] = 0;

	for (int index = 0; index < (int)(sizeof (_CameoKeys) / sizeof (_CameoKeys[0])); index ++) {
		if (::strstr (squashed, _CameoKeys[index].Key) != nullptr) {
			out_texture.Format ("hud_rsf_%s_%s.tga", (team == PLAYERTYPE_NOD) ? "nod" : "gdi", _CameoKeys[index].Suffix);
			return true;
		}
	}
	return false;
}


////////////////////////////////////////////////////////////////
//	Local player helpers
////////////////////////////////////////////////////////////////
static int Local_Team (void)
{
	cPlayer *player = cNetwork::Get_My_Player_Object ();
	return (player != nullptr) ? player->Get_Player_Type () : PLAYERTYPE_GDI;
}

static int Local_Credits (void)
{
	cPlayer *player = cNetwork::Get_My_Player_Object ();
	return (player != nullptr) ? (int)player->Get_Money () : 0;
}

static unsigned int Team_Color (int team)
{
	if (team == PLAYERTYPE_NOD) {
		return COLOR_NOD;
	}
	if (team == PLAYERTYPE_GDI) {
		return COLOR_GDI;
	}
	return COLOR_TEXT_DIM;
}


////////////////////////////////////////////////////////////////
//	Init / Shutdown
////////////////////////////////////////////////////////////////
void CommanderOverlay::Init (void)
{
	if (_IsInitialized) {
		return;
	}

	_PanelRenderer = new Render2DClass;
	StyleMgrClass::Configure_Renderer (_PanelRenderer);

	_TopRenderer = new Render2DClass;
	StyleMgrClass::Configure_Renderer (_TopRenderer);

	_TextRenderer = new Render2DSentenceClass;
	StyleMgrClass::Assign_Font (_TextRenderer, StyleMgrClass::FONT_INGAME_TXT);

	_TopText = new Render2DSentenceClass;
	StyleMgrClass::Assign_Font (_TopText, StyleMgrClass::FONT_INGAME_TXT);

	_CursorPos			= Vector2 (DESIGN_WIDTH * 0.5F, DESIGN_HEIGHT * 0.5F);
	_HoverSlot			= -1;
	_HoverPageButton	= 0;
	_Page					= 0;
	_PageMode			= -1;
	_WasActive			= false;
	_IsInitialized		= true;
}

void CommanderOverlay::Shutdown (void)
{
	if (!_IsInitialized) {
		return;
	}

	Release_Icons ();

	delete _PanelRenderer;
	delete _TopRenderer;
	delete _TextRenderer;
	delete _TopText;
	_PanelRenderer	= nullptr;
	_TopRenderer	= nullptr;
	_TextRenderer	= nullptr;
	_TopText			= nullptr;
	_CurrentText	= nullptr;
	_IsInitialized	= false;
}

bool CommanderOverlay::Is_Active (void)
{
	return _IsInitialized && CommanderModeController::Is_Active ();
}


////////////////////////////////////////////////////////////////
//	Design space <-> screen
////////////////////////////////////////////////////////////////
float CommanderOverlay::Scale_X (void)
{
	return Render2DClass::Get_Screen_Resolution ().Width () / DESIGN_WIDTH;
}

float CommanderOverlay::Scale_Y (void)
{
	return Render2DClass::Get_Screen_Resolution ().Height () / DESIGN_HEIGHT;
}

Vector2 CommanderOverlay::To_Screen (float x, float y)
{
	const RectClass &screen = Render2DClass::Get_Screen_Resolution ();
	return Vector2 (screen.Left + x * Scale_X (), screen.Top + y * Scale_Y ());
}

RectClass CommanderOverlay::To_Screen (const RectClass &design_rect)
{
	Vector2 upper_left	= To_Screen (design_rect.Left, design_rect.Top);
	Vector2 lower_right	= To_Screen (design_rect.Right, design_rect.Bottom);
	return RectClass (upper_left.X, upper_left.Y, lower_right.X, lower_right.Y);
}

Vector2 CommanderOverlay::To_Design (const Vector2 &screen_point)
{
	const RectClass &screen = Render2DClass::Get_Screen_Resolution ();
	return Vector2 ((screen_point.X - screen.Left) / Scale_X (), (screen_point.Y - screen.Top) / Scale_Y ());
}


////////////////////////////////////////////////////////////////
//	Layout
////////////////////////////////////////////////////////////////
int CommanderOverlay::Grid_Rows (void)
{
	//	A factory's list shares the sidebar with its queue strip, as in the fork.
	return (CommanderSidebarModel::Get_Mode () == SIDEBAR_MODE_BUILDINGS) ? GRID_ROWS : UNIT_GRID_ROWS;
}

int CommanderOverlay::Items_Per_Page (void)
{
	return GRID_COLUMNS * Grid_Rows ();
}

int CommanderOverlay::Page_Count (void)
{
	const int per_page	= Items_Per_Page ();
	const int count		= CommanderSidebarModel::Get_Card_Count ();
	return (count <= 0) ? 1 : ((count + per_page - 1) / per_page);
}

bool CommanderOverlay::Slot_Rect (int slot, RectClass &rect)
{
	if (slot < 0 || slot >= Items_Per_Page ()) {
		return false;
	}

	const float left	= DESIGN_WIDTH - SIDEBAR_WIDTH + SIDEBAR_PADDING;
	const int column	= slot % GRID_COLUMNS;
	const int row		= slot / GRID_COLUMNS;

	rect.Left	= left + column * (SLOT_SIZE + SLOT_GAP);
	rect.Top		= HEADER_HEIGHT + SIDEBAR_PADDING + row * (SLOT_SIZE + SLOT_GAP);
	rect.Right	= rect.Left + SLOT_SIZE;
	rect.Bottom	= rect.Top + SLOT_SIZE;
	return true;
}

bool CommanderOverlay::Page_Button_Rect (int direction, RectClass &rect)
{
	if (Page_Count () <= 1) {
		return false;
	}

	const float top = HEADER_HEIGHT + SIDEBAR_PADDING + Grid_Rows () * (SLOT_SIZE + SLOT_GAP) + PAGE_BUTTON_GAP;
	if (direction < 0) {
		rect.Left = DESIGN_WIDTH - SIDEBAR_WIDTH + SIDEBAR_PADDING;
	} else {
		rect.Left = DESIGN_WIDTH - SIDEBAR_PADDING - PAGE_BUTTON_WIDTH;
	}
	rect.Top		= top;
	rect.Right	= rect.Left + PAGE_BUTTON_WIDTH;
	rect.Bottom	= top + PAGE_BUTTON_HEIGHT;
	return true;
}

bool CommanderOverlay::Queue_Row_Rect (int row, RectClass &rect)
{
	if (row < 0 || row >= MAX_QUEUE_ROWS) {
		return false;
	}

	rect.Left	= DESIGN_WIDTH - SIDEBAR_WIDTH + SIDEBAR_PADDING;
	rect.Right	= DESIGN_WIDTH - SIDEBAR_PADDING;
	rect.Top		= QUEUE_STRIP_TOP + QUEUE_HEADER_HEIGHT + row * (QUEUE_ROW_HEIGHT + QUEUE_ROW_GAP);
	rect.Bottom	= rect.Top + QUEUE_ROW_HEIGHT;
	return true;
}

RectClass CommanderOverlay::Minimap_Rect (void)
{
	return RectClass (SQUAD_STRIP_LEFT, MINIMAP_BOTTOM - MINIMAP_SIZE, SQUAD_STRIP_LEFT + MINIMAP_SIZE, MINIMAP_BOTTOM);
}

RectClass CommanderOverlay::Status_Strip_Rect (void)
{
	return RectClass (0.0F, BOTTOM_BAR_TOP, DESIGN_WIDTH - SIDEBAR_WIDTH, DESIGN_HEIGHT);
}

bool CommanderOverlay::Command_Button_Rect (int command, RectClass &rect)
{
	if (command < 0 || command >= PALETTE_COMMAND_COUNT) return false;
	const int column = command % CommanderCommandPalette::COLUMNS;
	const int row = command / CommanderCommandPalette::COLUMNS;
	rect.Left = COMMAND_GRID_LEFT + column * (COMMAND_BUTTON_WIDTH + COMMAND_BUTTON_GAP);
	rect.Top = COMMAND_GRID_TOP + row * (COMMAND_BUTTON_HEIGHT + COMMAND_BUTTON_GAP);
	rect.Right = rect.Left + COMMAND_BUTTON_WIDTH;
	rect.Bottom = rect.Top + COMMAND_BUTTON_HEIGHT;
	return true;
}

bool CommanderOverlay::Is_Design_Over_Sidebar (const Vector2 &design)
{
	return design.X >= DESIGN_WIDTH - SIDEBAR_WIDTH;
}


bool CommanderOverlay::Project_To_Design (const Vector3 &world, Vector2 &out_design)
{
	if (COMBAT_CAMERA == nullptr) {
		return false;
	}

	Vector3 view;
	if (COMBAT_CAMERA->Project (view, world) != CameraClass::INSIDE_FRUSTUM) {
		return false;
	}

	out_design.X = (view.X + 1.0F) * 0.5F * DESIGN_WIDTH;
	out_design.Y = (1.0F - view.Y) * 0.5F * DESIGN_HEIGHT;
	return true;
}


////////////////////////////////////////////////////////////////
//	Paging
////////////////////////////////////////////////////////////////
void CommanderOverlay::Change_Page (int direction)
{
	const int pages = Page_Count ();
	_Page = (_Page + direction + pages) % pages;
}

void CommanderOverlay::Reset_Page (void)
{
	_Page = 0;
}


////////////////////////////////////////////////////////////////
//	Cursor -- a software cursor driven by DirectInput mouse deltas, as in the fork (the
//	OS cursor is hidden while commanding and Get_Cursor_Pos is only seeded on Acquire).
////////////////////////////////////////////////////////////////
void CommanderOverlay::Update_Cursor (void)
{
	const float dx = (float)DirectInput::Get_Mouse_Axis (DirectInput::MOUSE_X_AXIS);
	const float dy = (float)DirectInput::Get_Mouse_Axis (DirectInput::MOUSE_Y_AXIS);

	_CursorPos.X += dx * CURSOR_SPEED / Scale_X ();
	_CursorPos.Y += dy * CURSOR_SPEED / Scale_Y ();

	if (_CursorPos.X < 0.0F) _CursorPos.X = 0.0F;
	if (_CursorPos.Y < 0.0F) _CursorPos.Y = 0.0F;
	if (_CursorPos.X > DESIGN_WIDTH - 1.0F) _CursorPos.X = DESIGN_WIDTH - 1.0F;
	if (_CursorPos.Y > DESIGN_HEIGHT - 1.0F) _CursorPos.Y = DESIGN_HEIGHT - 1.0F;
}

void CommanderOverlay::Get_Cursor_Screen (Vector2 &out_screen)
{
	out_screen = To_Screen (_CursorPos.X, _CursorPos.Y);
}

void CommanderOverlay::Update_Hover (void)
{
	_HoverSlot			= -1;
	_HoverPageButton	= 0;
	_HoverCommand		= -1;

	RectClass rect;
	if (CommanderCommandPalette::Is_Open ()) {
		for (int command = 0; command < PALETTE_COMMAND_COUNT; command ++) {
			if (Command_Button_Rect (command, rect) && rect.Contains (_CursorPos)) {
				_HoverCommand = command;
				return;
			}
		}
	}
	for (int slot = 0; slot < Items_Per_Page (); slot ++) {
		if (Slot_Rect (slot, rect) && rect.Contains (_CursorPos)) {
			_HoverSlot = slot;
			return;
		}
	}
	if (Page_Button_Rect (-1, rect) && rect.Contains (_CursorPos)) {
		_HoverPageButton = -1;
	} else if (Page_Button_Rect (1, rect) && rect.Contains (_CursorPos)) {
		_HoverPageButton = 1;
	}
}

void CommanderOverlay::Update (void)
{
	if (!Is_Active ()) {
		_WasActive = false;
		return;
	}

	//	Every entry starts the cursor at the centre of the view, over the focus point.
	if (!_WasActive) {
		_CursorPos	= Vector2 ((DESIGN_WIDTH - SIDEBAR_WIDTH) * 0.5F, DESIGN_HEIGHT * 0.5F);
		_Page			= 0;
		_WasActive	= true;
	}

	const int mode = (int)CommanderSidebarModel::Get_Mode ();
	if (mode != _PageMode) {
		_PageMode	= mode;
		_Page			= 0;
	}
	if (_Page >= Page_Count ()) {
		_Page = Page_Count () - 1;
	}

	Update_Cursor ();
	Update_Hover ();
}


////////////////////////////////////////////////////////////////
//	Hit testing for CommanderUIInput
////////////////////////////////////////////////////////////////
CommanderOverlayHitType CommanderOverlay::Hit_Test (const Vector2 &screen, int *out_card_index, Vector3 *out_world,
	int *out_command_index)
{
	const Vector2 design = To_Design (screen);
	RectClass command_rect;
	if (CommanderCommandPalette::Is_Open ()) {
		for (int command = 0; command < PALETTE_COMMAND_COUNT; command ++) {
			if (Command_Button_Rect (command, command_rect) && command_rect.Contains (design)) {
				if (out_command_index != nullptr) *out_command_index = command;
				return COMMANDER_OVERLAY_HIT_COMMAND;
			}
		}
	}

	RectClass minimap = Minimap_Rect ();
	if (minimap.Contains (design)) {
		Vector3 world;
		if (out_world != nullptr && CommanderMinimap::Panel_To_World (screen.X, screen.Y, To_Screen (minimap), &world)) {
			*out_world = world;
			return COMMANDER_OVERLAY_HIT_MINIMAP;
		}
		return COMMANDER_OVERLAY_HIT_PANEL;
	}

	if (Is_Design_Over_Sidebar (design)) {
		RectClass rect;
		if (Page_Button_Rect (-1, rect) && rect.Contains (design)) {
			return COMMANDER_OVERLAY_HIT_PAGE_PREV;
		}
		if (Page_Button_Rect (1, rect) && rect.Contains (design)) {
			return COMMANDER_OVERLAY_HIT_PAGE_NEXT;
		}
		for (int slot = 0; slot < Items_Per_Page (); slot ++) {
			if (Slot_Rect (slot, rect) && rect.Contains (design)) {
				const int card_index = _Page * Items_Per_Page () + slot;
				if (card_index < CommanderSidebarModel::Get_Card_Count ()) {
					if (out_card_index != nullptr) {
						*out_card_index = card_index;
					}
					return COMMANDER_OVERLAY_HIT_CARD;
				}
				break;
			}
		}
		return COMMANDER_OVERLAY_HIT_PANEL;
	}

	if (Status_Strip_Rect ().Contains (design)) {
		return COMMANDER_OVERLAY_HIT_PANEL;
	}
	return COMMANDER_OVERLAY_HIT_NONE;
}

bool CommanderOverlay::Is_Over_Panel (const Vector2 &screen)
{
	return Hit_Test (screen, nullptr, nullptr) != COMMANDER_OVERLAY_HIT_NONE;
}


////////////////////////////////////////////////////////////////
//	Draw helpers
////////////////////////////////////////////////////////////////
void CommanderOverlay::Draw_Panel (const RectClass &design_rect, unsigned int fill, unsigned int frame)
{
	RectClass screen = To_Screen (design_rect);
	_PanelRenderer->Add_Quad (screen, fill);
	if (frame != 0) {
		_PanelRenderer->Add_Outline (screen, FRAME_WIDTH, frame);
	}
}

void CommanderOverlay::Draw_Text (const char *text, float x, float y, unsigned int color)
{
	if (text == nullptr || text[0] == 0 || _CurrentText == nullptr) {
		return;
	}

	WideStringClass wide_text;
	wide_text.Convert_From (text);
	_CurrentText->Set_Location (To_Screen (x, y));
	_CurrentText->Build_Sentence (wide_text);
	_CurrentText->Draw_Sentence (color);
}

bool CommanderOverlay::Draw_Texture_Icon (const char *texture_name, const RectClass &design_rect, unsigned int color)
{
	OverlayIconStruct *icon = Find_Icon (texture_name);
	if (icon == nullptr || icon->Missing || icon->Renderer == nullptr) {
		return false;
	}

	icon->Renderer->Add_Quad (To_Screen (design_rect), RectClass (0.0F, 0.0F, 1.0F, 1.0F), color);
	return true;
}


////////////////////////////////////////////////////////////////
//	Render
////////////////////////////////////////////////////////////////
void CommanderOverlay::Render (void)
{
	if (!Is_Active ()) {
		return;
	}

	_PanelRenderer->Reset ();
	_TextRenderer->Reset ();
	_TopRenderer->Reset ();
	_TopText->Reset ();
	Reset_Icons ();

	_CurrentText = _TextRenderer;
	Render_Health_Bars ();
	Render_Status_Strip ();
	Render_Minimap ();
	Render_Build_Sidebar ();
	Render_Queue_Strip ();
	Render_Header ();
	Render_Command_Grid ();

	_CurrentText = _TopText;
	Render_Tooltip ();
	Render_Cursor ();

	_PanelRenderer->Render ();
	Render_Icons ();
	_TextRenderer->Render ();
	_TopRenderer->Render ();
	_TopText->Render ();
	_CurrentText = nullptr;
}


void CommanderOverlay::Render_Status_Strip (void)
{
	RectClass strip = Status_Strip_Rect ();
	Draw_Panel (strip, COLOR_PANEL_FILL_DARK, COLOR_FRAME_DIM);
	_PanelRenderer->Add_Line (To_Screen (strip.Left, strip.Top), To_Screen (strip.Right, strip.Top), FRAME_WIDTH, COLOR_FRAME);

	CommanderPowerDisplay power;
	CommanderSidebarModel::Get_Power_Display (&power);

	StringClass text;
	Draw_Text ("POWER GRID", 145.0F, 353.0F, COLOR_FRAME);
	text.Format ("CAPACITY %d", (int)power.Generation);
	Draw_Text (text, 145.0F, 369.0F, COLOR_TEXT);
	text.Format ("IN USE %d", (int)power.AllocatedDemand);
	Draw_Text (text, 145.0F, 383.0F, COLOR_TEXT);
	text.Format ("FREE %d", (int)WWMath::Max (0.0F, power.Headroom));
	Draw_Text (text, 255.0F, 383.0F, COLOR_TEXT_GOOD);

	const RectClass bar (145.0F, 406.0F, 377.0F, 427.0F);
	Draw_Panel (bar, COLOR_SLOT_FILL, COLOR_FRAME);
	if (power.Generation > 0.0F) {
		const float used = WWMath::Clamp (power.AllocatedDemand / power.Generation, 0.0F, 1.0F);
		const float split = bar.Left + 2.0F + (bar.Width () - 4.0F) * used;
		if (split > bar.Left + 2.0F) {
			Draw_Panel (RectClass (bar.Left + 2.0F, bar.Top + 2.0F, split, bar.Bottom - 2.0F),
				COLOR_FRAME, 0);
		}
		if (split < bar.Right - 2.0F) {
			Draw_Panel (RectClass (split, bar.Top + 2.0F, bar.Right - 2.0F, bar.Bottom - 2.0F),
				COLOR_TEXT_GOOD, 0);
		}
	}
	Draw_Text ("USED", 145.0F, 431.0F, COLOR_FRAME);
	Draw_Text ("AVAILABLE", 295.0F, 431.0F, COLOR_TEXT_GOOD);
	text.Format ("SELECTED %d", CommanderSelection::Get_Count ());
	Draw_Text (text, 145.0F, 458.0F, COLOR_TEXT_DIM);

	if (power.Overloaded || power.ShedCount > 0) {
		if (power.ShedCount > 0) {
			text.Format ("DEMAND %d  /  %d OFFLINE", (int)power.RequestedDemand, power.ShedCount);
		} else {
			text.Format ("DEMAND %d  /  LOW POWER", (int)power.RequestedDemand);
		}
		Draw_Text (text, 145.0F, 444.0F, COLOR_TEXT_BAD);
	}
}

void CommanderOverlay::Render_Command_Grid (void)
{
	const RectClass frame (385.0F, 348.0F, 518.0F, 476.0F);
	Draw_Panel (frame, COLOR_PANEL_FILL, COLOR_FRAME);
	Draw_Text ("ORDERS", 390.0F, 347.0F, COLOR_FRAME);

	static const char *short_labels[PALETTE_COMMAND_COUNT] =
		{ "MOV", "ADV", "ATK", "DEF", "HLD", "STP", "LIN", "WDG", "SPR" };
	for (int command = 0; command < PALETTE_COMMAND_COUNT; command ++) {
		RectClass button;
		if (!Command_Button_Rect (command, button)) continue;
		const bool open = CommanderCommandPalette::Is_Open ();
		const bool armed = open && CommanderCommandPalette::Is_Armed () &&
			CommanderCommandPalette::Get_Armed () == (CommanderPaletteCommandType)command;
		const unsigned int fill = armed ? COLOR_SLOT_SELECTED :
			(open && _HoverCommand == command ? COLOR_SLOT_HOVER : COLOR_SLOT_FILL);
		Draw_Panel (button, fill, armed ? COLOR_FRAME : COLOR_FRAME_DIM);
		const unsigned int ink = open ? COLOR_TEXT : COLOR_TEXT_DIM;
		Draw_Command_Icon (command, button, ink);
		Draw_Text (short_labels[command], button.Left + 6.0F, button.Top + 20.0F, ink);
	}
}

void CommanderOverlay::Draw_Command_Icon (int command, const RectClass &button, unsigned int color)
{
	const float x = (button.Left + button.Right) * 0.5F;
	const float y = button.Top + 10.0F;
	const float width = 1.5F;
	if (command == PALETTE_MOVE || command == PALETTE_ATTACK_MOVE) {
		_PanelRenderer->Add_Line (To_Screen (x - 9.0F,y), To_Screen (x + 7.0F,y), width, color);
		_PanelRenderer->Add_Line (To_Screen (x + 3.0F,y - 4.0F), To_Screen (x + 8.0F,y), width, color);
		_PanelRenderer->Add_Line (To_Screen (x + 3.0F,y + 4.0F), To_Screen (x + 8.0F,y), width, color);
		if (command == PALETTE_ATTACK_MOVE) {
			_PanelRenderer->Add_Line (To_Screen (x - 8.0F,y - 5.0F), To_Screen (x - 8.0F,y + 5.0F), width, color);
		}
	} else if (command == PALETTE_ATTACK) {
		_PanelRenderer->Add_Line (To_Screen (x - 8.0F,y), To_Screen (x - 3.0F,y), width, color);
		_PanelRenderer->Add_Line (To_Screen (x + 3.0F,y), To_Screen (x + 8.0F,y), width, color);
		_PanelRenderer->Add_Line (To_Screen (x,y - 7.0F), To_Screen (x,y - 2.0F), width, color);
		_PanelRenderer->Add_Line (To_Screen (x,y + 2.0F), To_Screen (x,y + 7.0F), width, color);
	} else if (command == PALETTE_DEFEND) {
		_PanelRenderer->Add_Line (To_Screen (x - 7.0F,y - 6.0F), To_Screen (x + 7.0F,y - 6.0F), width, color);
		_PanelRenderer->Add_Line (To_Screen (x - 7.0F,y - 6.0F), To_Screen (x - 5.0F,y + 3.0F), width, color);
		_PanelRenderer->Add_Line (To_Screen (x + 7.0F,y - 6.0F), To_Screen (x + 5.0F,y + 3.0F), width, color);
		_PanelRenderer->Add_Line (To_Screen (x - 5.0F,y + 3.0F), To_Screen (x,y + 7.0F), width, color);
		_PanelRenderer->Add_Line (To_Screen (x + 5.0F,y + 3.0F), To_Screen (x,y + 7.0F), width, color);
	} else if (command == PALETTE_HOLD) {
		_PanelRenderer->Add_Quad (To_Screen (RectClass (x - 6.0F,y - 6.0F,x - 2.0F,y + 6.0F)), color);
		_PanelRenderer->Add_Quad (To_Screen (RectClass (x + 2.0F,y - 6.0F,x + 6.0F,y + 6.0F)), color);
	} else if (command == PALETTE_STOP) {
		_PanelRenderer->Add_Outline (To_Screen (RectClass (x - 6.0F,y - 6.0F,x + 6.0F,y + 6.0F)), width, color);
	} else {
		// The three formation silhouettes: abreast, arrowhead, dispersed.
		const float left_y = (command == PALETTE_FORM_WEDGE) ? y + 5.0F : y;
		const float center_y = (command == PALETTE_FORM_WEDGE) ? y - 5.0F : y;
		const float right_y = (command == PALETTE_FORM_WEDGE) ? y + 5.0F : y;
		const float spread = (command == PALETTE_FORM_SCATTER) ? 10.0F : 7.0F;
		_PanelRenderer->Add_Quad (To_Screen (RectClass (x - spread - 2.0F,left_y - 2.0F,x - spread + 2.0F,left_y + 2.0F)), color);
		_PanelRenderer->Add_Quad (To_Screen (RectClass (x - 2.0F,center_y - 2.0F,x + 2.0F,center_y + 2.0F)), color);
		_PanelRenderer->Add_Quad (To_Screen (RectClass (x + spread - 2.0F,right_y - 2.0F,x + spread + 2.0F,right_y + 2.0F)), color);
	}
}


void CommanderOverlay::Render_Header (void)
{
	const float left = DESIGN_WIDTH - SIDEBAR_WIDTH;
	RectClass header (left, 0.0F, DESIGN_WIDTH, HEADER_HEIGHT);
	Draw_Panel (header, COLOR_PANEL_FILL_DARK, COLOR_FRAME);

	const int team = Local_Team ();
	Draw_Text ("COMMANDER", left + SIDEBAR_PADDING, 3.0F, Team_Color (team));

	StringClass text;
	text.Format ("CREDITS %d", Local_Credits ());
	Draw_Text (text, left + SIDEBAR_PADDING, 15.0F, COLOR_TEXT);

	const CommanderSidebarModeType mode = CommanderSidebarModel::Get_Mode ();
	if (mode == SIDEBAR_MODE_BUILDINGS) {
		Draw_Text ("BUILD", left + SIDEBAR_PADDING, 27.0F, COLOR_TEXT);
	} else {
		const CommanderFacilityState &facility = CommanderSidebarModel::Get_Facility_State ();

		const char *name = CommanderSidebarModel::Get_Mode_Name (mode);
		unsigned int color = COLOR_TEXT;
		if (!facility.Bound) {
			color = COLOR_TEXT_DIM;
		} else if (!facility.Alive || !facility.Powered) {
			color = COLOR_TEXT_BAD;
		}
		Draw_Text (name, left + SIDEBAR_PADDING, 27.0F, color);
		Draw_Text ("RCLICK BACK", left + SIDEBAR_PADDING, 39.0F, COLOR_TEXT_DIM);
	}
}


void CommanderOverlay::Render_Build_Sidebar (void)
{
	const float left = DESIGN_WIDTH - SIDEBAR_WIDTH;
	Draw_Panel (RectClass (left, HEADER_HEIGHT, DESIGN_WIDTH, DESIGN_HEIGHT), COLOR_PANEL_FILL, COLOR_FRAME_DIM);

	const int team			= Local_Team ();
	const int per_page	= Items_Per_Page ();
	const int card_count	= CommanderSidebarModel::Get_Card_Count ();

	if (card_count == 0) {
		Draw_Text ("NOTHING TO BUILD", left + SIDEBAR_PADDING, HEADER_HEIGHT + SIDEBAR_PADDING, COLOR_TEXT_DIM);
	}

	for (int slot = 0; slot < per_page; slot ++) {
		RectClass rect;
		Slot_Rect (slot, rect);

		const int card_index = _Page * per_page + slot;
		if (card_index >= card_count) {
			Draw_Panel (rect, COLOR_PANEL_FILL_DARK, COLOR_FRAME_DIM);
			continue;
		}

		const CommanderSidebarCard *card_ptr	= CommanderSidebarModel::Get_Card (card_index);
		if (card_ptr == nullptr) {
			continue;
		}
		const CommanderSidebarCard &card	= *card_ptr;
		const bool available					= (card.Status == CARD_AVAILABLE);
		const bool hovered					= (slot == _HoverSlot);

		unsigned int fill = available ? COLOR_SLOT_FILL : COLOR_SLOT_BLOCKED;
		if (hovered) {
			fill = COLOR_SLOT_HOVER;
		}
		Draw_Panel (rect, fill, hovered ? COLOR_FRAME : COLOR_FRAME_DIM);

		//	Cameo: the card's own icon, else the fork's stock hud_rsf cameo for the preset.
		RectClass icon_rect (rect.Left + 1.0F, rect.Top + 1.0F, rect.Right - 1.0F, rect.Bottom - 1.0F);
		const unsigned int cameo_color = available ? COLOR_CAMEO : COLOR_CAMEO_BLOCKED;
		bool drew_icon = Draw_Texture_Icon (card.IconTexture, icon_rect, cameo_color);
		if (!drew_icon) {
			StringClass cameo;
			if (	Cameo_For_Name (card.DescriptorName, team, cameo) ||
					Cameo_For_Name (card.DisplayName, team, cameo))
			{
				drew_icon = Draw_Texture_Icon (cameo, icon_rect, cameo_color);
			}
		}
		if (!drew_icon) {
			char short_name[10];
			::strncpy (short_name, card.DisplayName, sizeof (short_name) - 1);
			short_name[sizeof (short_name) - 1] = 0;
			Draw_Text (short_name, rect.Left + 2.0F, rect.Top + 16.0F, available ? COLOR_TEXT : COLOR_TEXT_DIM);
		}

		StringClass text;
		text.Format ("%d", card.Cost);
		Draw_Text (text, rect.Left + 2.0F, rect.Bottom - 11.0F, available ? COLOR_TEXT : COLOR_TEXT_BAD);

		if (card.QueuedCount > 0) {
			text.Format ("x%d", card.QueuedCount);
			Draw_Text (text, rect.Right - 16.0F, rect.Top + 1.0F, COLOR_TEXT_GOOD);
		}
	}

	//	Page buttons and "n / m".
	RectClass prev_rect;
	RectClass next_rect;
	if (Page_Button_Rect (-1, prev_rect) && Page_Button_Rect (1, next_rect)) {
		Draw_Panel (prev_rect, (_HoverPageButton < 0) ? COLOR_SLOT_HOVER : COLOR_SLOT_FILL, COLOR_FRAME_DIM);
		Draw_Panel (next_rect, (_HoverPageButton > 0) ? COLOR_SLOT_HOVER : COLOR_SLOT_FILL, COLOR_FRAME_DIM);
		Draw_Text ("<", prev_rect.Left + 4.0F, prev_rect.Top, COLOR_TEXT);
		Draw_Text (">", next_rect.Left + 4.0F, next_rect.Top, COLOR_TEXT);

		StringClass text;
		text.Format ("%d / %d", _Page + 1, Page_Count ());
		Draw_Text (text, (prev_rect.Right + next_rect.Left) * 0.5F - 12.0F, prev_rect.Top, COLOR_TEXT_DIM);
	}
}


void CommanderOverlay::Render_Queue_Strip (void)
{
	if (CommanderSidebarModel::Get_Mode () == SIDEBAR_MODE_BUILDINGS) {
		return;
	}

	const CommanderFacilityState &facility = CommanderSidebarModel::Get_Facility_State ();

	const float left = DESIGN_WIDTH - SIDEBAR_WIDTH + SIDEBAR_PADDING;
	Draw_Text ("QUEUE", left, QUEUE_STRIP_TOP, COLOR_TEXT);
	if (!facility.Bound) {
		return;
	}

	const int queued = CommanderProductionService::Get_Queue_Length (facility.MemberId);
	for (int row = 0; row < MAX_QUEUE_ROWS; row ++) {
		RectClass rect;
		Queue_Row_Rect (row, rect);

		CommanderProductionItem item;
		if (row >= queued || !CommanderProductionService::Get_Queue_Item (facility.MemberId, row, &item)) {
			Draw_Panel (rect, COLOR_PANEL_FILL_DARK, 0);
			continue;
		}

		Draw_Panel (rect, COLOR_SLOT_FILL, 0);

		char name[QUEUE_NAME_CHARS + 1] = "?";
		DefinitionClass *definition = DefinitionMgrClass::Find_Definition (item.DefinitionId, false);
		if (definition != nullptr) {
			::strncpy (name, definition->Get_Name (), QUEUE_NAME_CHARS);
			name[QUEUE_NAME_CHARS] = 0;
		}
		Draw_Text (name, rect.Left + 2.0F, rect.Top, (row == 0) ? COLOR_TEXT : COLOR_TEXT_DIM);

		//	Progress bar along the bottom edge of the row being built.
		if (row == 0 && item.BuildTimeSeconds > 0.0F) {
			float fraction = item.ElapsedSeconds / item.BuildTimeSeconds;
			fraction = WWMath::Clamp (fraction, 0.0F, 1.0F);
			RectClass bar (rect.Left, rect.Bottom - QUEUE_PROGRESS_HEIGHT, rect.Left + rect.Width () * fraction, rect.Bottom);
			Draw_Panel (bar, COLOR_TEXT_GOOD, 0);
		}

		/*
		**	Why the head of the line is not moving.  A held item looked identical to a building
		**	one before this: the same row, and for a blocked exit a full progress bar that never
		**	delivered, with the credits already spent.  The state is the service's own, so the
		**	panel reports it rather than guessing from the bar.
		*/
		if (row == 0) {
			const char *held = nullptr;
			switch (item.State) {
				case PRODUCTION_STATE_WAITING_FOR_POWER:	held = "NO POWER";		break;
				case PRODUCTION_STATE_BLOCKED_EXIT:			held = "EXIT BLOCKED";	break;
				case PRODUCTION_STATE_QUEUED:					held = "HELD";				break;
				default:												held = nullptr;			break;
			}
			if (held != nullptr) {
				Draw_Text (held, rect.Left + 2.0F, rect.Top + 10.0F, COLOR_TEXT_BAD);
			}
		}
	}
}


void CommanderOverlay::Render_Minimap (void)
{
	const RectClass design_rect	= Minimap_Rect ();
	const RectClass screen_rect	= To_Screen (design_rect);
	Draw_Panel (design_rect, COLOR_PANEL_FILL_DARK, COLOR_FRAME);

	//	Buildings as squares, units as dots, in team colour.
	SLNode<BuildingGameObj> *building_node = GameObjManager::Get_Building_Game_Obj_List ()->Head ();
	for ( ; building_node != nullptr; building_node = building_node->Next ()) {
		BuildingGameObj *building = building_node->Data ();
		if (building == nullptr || building->Is_Destroyed ()) {
			continue;
		}
		Vector3 position;
		building->Get_Position (&position);
		float x = 0.0F;
		float y = 0.0F;
		if (CommanderMinimap::World_To_Panel (position, screen_rect, &x, &y)) {
			_PanelRenderer->Add_Quad (RectClass (x - 2.0F, y - 2.0F, x + 2.0F, y + 2.0F), Team_Color (building->Get_Player_Type ()));
		}
	}

	SLNode<SmartGameObj> *smart_node = GameObjManager::Get_Smart_Game_Obj_List ()->Head ();
	for ( ; smart_node != nullptr; smart_node = smart_node->Next ()) {
		SmartGameObj *unit = smart_node->Data ();
		if (unit == nullptr) {
			continue;
		}
		Vector3 position;
		unit->Get_Position (&position);
		float x = 0.0F;
		float y = 0.0F;
		if (CommanderMinimap::World_To_Panel (position, screen_rect, &x, &y)) {
			_PanelRenderer->Add_Quad (RectClass (x - 1.0F, y - 1.0F, x + 1.0F, y + 1.0F), Team_Color (unit->Get_Player_Type ()));
		}
	}

	//	View box: the ground the overhead camera frames around its focus.
	Vector3 focus	= CommanderModeController::Get_Focus ();
	float height	= CommanderModeController::Get_Zoom ();
	if (CommanderCamera::Get_Instance () != nullptr && StrategicViewManager::Is_Overhead_View ()) {
		height = CommanderCamera::Get_Instance ()->Get_Height ();
	}
	const float half = WWMath::Max (height * 0.6F, 10.0F);
	float x0 = 0.0F, y0 = 0.0F, x1 = 0.0F, y1 = 0.0F;
	if (	CommanderMinimap::World_To_Panel (focus - Vector3 (half, half, 0.0F), screen_rect, &x0, &y0) &&
			CommanderMinimap::World_To_Panel (focus + Vector3 (half, half, 0.0F), screen_rect, &x1, &y1))
	{
		RectClass box (WWMath::Min (x0, x1), WWMath::Min (y0, y1), WWMath::Max (x0, x1), WWMath::Max (y0, y1));
		_PanelRenderer->Add_Outline (box, FRAME_WIDTH, COLOR_TEXT);
	}
}


////////////////////////////////////////////////////////////////
//	Health bars (the fork's CommanderHealthOverlay), selected objects only
////////////////////////////////////////////////////////////////
static float Health_Fraction (int object_id)
{
	ScriptableGameObj *object = GameObjManager::Find_ScriptableGameObj (object_id);
	if (object == nullptr) {
		return -1.0F;
	}
	DamageableGameObj *damageable = object->As_DamageableGameObj ();
	if (damageable == nullptr) {
		return -1.0F;
	}
	DefenseObjectClass *defense = damageable->Get_Defense_Object ();
	const float max = defense->Get_Health_Max () + defense->Get_Shield_Strength_Max ();
	if (max <= 0.0F) {
		return -1.0F;
	}
	return WWMath::Clamp ((defense->Get_Health () + defense->Get_Shield_Strength ()) / max, 0.0F, 1.0F);
}

static unsigned int Health_Color (float health)
{
	if (health <= HEALTH_CRITICAL) {
		return COLOR_HEALTH_BAD;
	}
	if (health <= HEALTH_MID) {
		return COLOR_HEALTH_MID;
	}
	return COLOR_HEALTH_OK;
}

bool CommanderOverlay::Draw_Health_Bar (const Vector3 &world_position, float health, float vertical_offset)
{
	Vector2 design;
	if (!Project_To_Design (world_position, design) || Is_Design_Over_Sidebar (design)) {
		return false;
	}

	const float left	= design.X - (HEALTH_BAR_WIDTH * 0.5F);
	const float top	= design.Y - vertical_offset;

	_PanelRenderer->Add_Quad (To_Screen (RectClass (left - HEALTH_BAR_FRAME, top - HEALTH_BAR_FRAME,
		left + HEALTH_BAR_WIDTH + HEALTH_BAR_FRAME, top + HEALTH_BAR_HEIGHT + HEALTH_BAR_FRAME)), COLOR_HEALTH_FRAME);
	_PanelRenderer->Add_Quad (To_Screen (RectClass (left, top, left + HEALTH_BAR_WIDTH, top + HEALTH_BAR_HEIGHT)), COLOR_HEALTH_BACKING);
	if (health > 0.0F) {
		_PanelRenderer->Add_Quad (To_Screen (RectClass (left, top, left + HEALTH_BAR_WIDTH * health, top + HEALTH_BAR_HEIGHT)),
			Health_Color (health));
	}
	return true;
}

void CommanderOverlay::Render_Health_Bars (void)
{
	const int count = CommanderSelection::Get_Count ();
	for (int index = 0; index < count; index ++) {
		const int unit_id		= CommanderSelection::Get_Unit_Id (index);
		const float health	= Health_Fraction (unit_id);
		ScriptableGameObj *unit = GameObjManager::Find_ScriptableGameObj (unit_id);
		if (health < 0.0F || unit == nullptr) {
			continue;
		}

		Vector3 position;
		unit->Get_Position (&position);
		if (!Draw_Health_Bar (position, health, HEALTH_BAR_OFFSET_Y)) {
			continue;
		}

		//	The primary (first) selection is named above its bar, as in the fork.
		if (index == 0) {
			Vector2 design;
			if (Project_To_Design (position, design)) {
				Draw_Text (unit->Get_Definition ().Get_Name (), design.X - (HEALTH_BAR_WIDTH * 0.5F),
					design.Y - HEALTH_BAR_OFFSET_Y - HEALTH_NAME_OFFSET_Y - 4.0F, COLOR_TEXT);
			}
		}
	}

	const int building_id = CommanderSelection::Get_Selected_Building ();
	if (building_id != 0) {
		const float health = Health_Fraction (building_id);
		ScriptableGameObj *building = GameObjManager::Find_ScriptableGameObj (building_id);
		if (health >= 0.0F && building != nullptr) {
			Vector3 position;
			building->Get_Position (&position);
			Draw_Health_Bar (position, health, HEALTH_BAR_OFFSET_Y_BUILDING);
		}
	}
}


void CommanderOverlay::Render_Selection_Box (void)
{
	if (!CommanderSelection::Is_Dragging () || !CommanderSelection::Is_Drag_Significant ()) {
		return;
	}

	Vector2 minimum;
	Vector2 maximum;
	CommanderSelection::Get_Drag_Rect (&minimum, &maximum);

	RectClass box (minimum.X, minimum.Y, maximum.X, maximum.Y);
	_PanelRenderer->Add_Quad (box, COLOR_SELECT_BOX_FILL);
	_PanelRenderer->Add_Outline (box, FRAME_WIDTH, COLOR_SELECT_BOX_EDGE);
}


void CommanderOverlay::Render_Tooltip (void)
{
	if (_HoverCommand >= 0 && CommanderCommandPalette::Is_Open ()) {
		const CommanderPaletteCommandType command = (CommanderPaletteCommandType)_HoverCommand;
		const RectClass tip (138.0F, 303.0F, 382.0F, 339.0F);
		_TopRenderer->Add_Quad (To_Screen (tip), COLOR_PANEL_FILL_DARK);
		_TopRenderer->Add_Outline (To_Screen (tip), FRAME_WIDTH, COLOR_FRAME);
		Draw_Text (CommanderCommandPalette::Get_Label (command), tip.Left + 5.0F, tip.Top + 3.0F, COLOR_TEXT);
		Draw_Text (CommanderCommandPalette::Get_Hint (command), tip.Left + 5.0F, tip.Top + 17.0F, COLOR_TEXT_DIM);
		return;
	}
	if (_HoverSlot < 0) {
		return;
	}

	const int card_index = _Page * Items_Per_Page () + _HoverSlot;
	if (card_index >= CommanderSidebarModel::Get_Card_Count ()) {
		return;
	}

	const CommanderSidebarCard *card_ptr = CommanderSidebarModel::Get_Card (card_index);
	if (card_ptr == nullptr) {
		return;
	}
	const CommanderSidebarCard &card = *card_ptr;

	RectClass slot_rect;
	Slot_Rect (_HoverSlot, slot_rect);

	const float width		= 150.0F;
	const float height	= (card.Status == CARD_AVAILABLE) ? 38.0F : 50.0F;
	float top				= slot_rect.Top;
	if (top + height > DESIGN_HEIGHT) {
		top = DESIGN_HEIGHT - height;
	}
	const float right	= DESIGN_WIDTH - SIDEBAR_WIDTH - 2.0F;
	RectClass panel (right - width, top, right, top + height);

	RectClass screen = To_Screen (panel);
	_TopRenderer->Add_Quad (screen, COLOR_PANEL_FILL_DARK);
	_TopRenderer->Add_Outline (screen, FRAME_WIDTH, COLOR_FRAME);

	const float x = panel.Left + 4.0F;
	Draw_Text (card.DisplayName, x, top + 2.0F, COLOR_TEXT);

	StringClass text;
	text.Format ("COST %d", card.Cost);
	Draw_Text (text, x, top + 14.0F, (card.Status == CARD_INSUFFICIENT_CREDITS) ? COLOR_TEXT_BAD : COLOR_TEXT);

	if (card.PowerGeneration > 0.0F) {
		text.Format ("POWER +%d", (int)card.PowerGeneration);
	} else {
		text.Format ("POWER -%d", (int)card.PowerConsumption);
	}
	Draw_Text (text, x, top + 26.0F, COLOR_TEXT_DIM);

	if (card.Status != CARD_AVAILABLE) {
		Draw_Text (CommanderSidebarModel::Get_Status_Name (card.Status), x, top + 38.0F, COLOR_TEXT_BAD);
	}
}


void CommanderOverlay::Render_Cursor (void)
{
	//	A dialog over Commander Mode brings the OS cursor back; do not draw a second one.
	if (DialogMgrClass::Get_Dialog_Count () > 0) {
		return;
	}

	const Vector2 center	= To_Screen (_CursorPos.X, _CursorPos.Y);
	const float size		= CURSOR_SIZE * Scale_X ();
	const float width		= 1.5F;

	if (Is_Design_Over_Sidebar (_CursorPos) || Status_Strip_Rect ().Contains (_CursorPos)) {
		//	Pointer over the panels.
		_TopRenderer->Add_Line (center, center + Vector2 (0.0F, size), width, COLOR_CURSOR);
		_TopRenderer->Add_Line (center, center + Vector2 (size * 0.7F, size * 0.7F), width, COLOR_CURSOR);
		_TopRenderer->Add_Line (center + Vector2 (0.0F, size), center + Vector2 (size * 0.7F, size * 0.7F), width, COLOR_CURSOR);
		return;
	}

	//	Four corner brackets over the world.
	const float half	= size * 0.5F;
	const float arm	= size * 0.3F;
	for (int corner = 0; corner < 4; corner ++) {
		const float sx = (corner & 1) ? 1.0F : -1.0F;
		const float sy = (corner & 2) ? 1.0F : -1.0F;
		const Vector2 point = center + Vector2 (sx * half, sy * half);
		_TopRenderer->Add_Line (point, point - Vector2 (sx * arm, 0.0F), width, COLOR_CURSOR);
		_TopRenderer->Add_Line (point, point - Vector2 (0.0F, sy * arm), width, COLOR_CURSOR);
	}
	_TopRenderer->Add_Quad (RectClass (center.X - 1.0F, center.Y - 1.0F, center.X + 1.0F, center.Y + 1.0F), COLOR_CURSOR);
}
