/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commandersidebarrenderer.cpp          $*
 *                                                                                             *
 *	Roadmap Section 42, Phase 36 -- the drawing half of the Commander production sidebar.       *
 *                                                                                             *
 *	Visual language: the original Command & Conquer RTS sidebar.  That is a deliberate choice   *
 *	and it costs nothing, because Section 42 only fixes the *content* -- "two rows", "2 x 4      *
 *	cells", paging, power bars, per-card cost and reason -- and asks the panel to be "visually   *
 *	compatible with Renegade's existing UI/purchase imagery".  Renegade's own UI is already the  *
 *	C&C house style, so the two requirements point the same way.                                 *
 *                                                                                             *
 *	What is taken from Tiberian Dawn / Red Alert specifically:                                  *
 *                                                                                             *
 *	  - a beveled metal strip, not a flat translucent rectangle: every box is drawn with a       *
 *	    light top/left edge and a dark bottom/right edge, so it reads as stamped metal;           *
 *	  - the signature **vertical** power bar down the left edge of the strip, with a sliding      *
 *	    drain marker rather than a second bar, going green -> amber -> red as the load climbs;    *
 *	  - amber/gold text on dark grey, and the cost stamped on the icon itself;                    *
 *	  - recessed (inverted-bevel) build cells, dimmed rather than hidden when unavailable.        *
 *                                                                                             *
 *	What is *not* taken: C&C's two-column vertical icon list.  Section 42 fixes two rows of       *
 *	four, and the roadmap wins over the reference where they disagree.                           *
 *                                                                                             *
 *	This file owns pixels and nothing else.  It re-derives no rule; in particular it never        *
 *	decides whether a card is buyable -- it colours what `CommanderSidebarModel` called buyable,  *
 *	so a greyed card and a refused purchase can never disagree.                                  *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commandersidebarrenderer.h"

#include "assetmgr.h"
#include "commandersidebar.h"
#include "playertype.h"
#include "purchaseavailability.h"
#include "purchasesettings.h"
#include "render2d.h"
#include "render2dsentence.h"
#include "stylemgr.h"
#include "texture.h"
#include "unichar.h"
#include "widestring.h"
#include "wwmath.h"

#include <stdio.h>


static bool							_Available			= false;
static bool							_Dirty				= true;
static int							_RebuildCount		= 0;
static int							_MissingIconCount	= 0;

static Render2DClass *			_Panel				= nullptr;		// frame, cells, bars: untextured
static Render2DClass *			_Icons [CommanderSidebarModel::PAGE_SIZE] = { nullptr };
static Render2DSentenceClass *	_Text					= nullptr;


/*
**	The C&C sidebar palette.  Grey stamped metal, amber readouts, and a power bar that is the
**	only saturated colour on the strip -- which is what makes an overload impossible to miss
**	without anything having to flash.
*/
static const unsigned int	COLOR_METAL				= RGBA_TO_INT32 ( 66, 66, 62,235);
static const unsigned int	COLOR_METAL_DARK		= RGBA_TO_INT32 ( 34, 34, 32,240);
static const unsigned int	COLOR_BEVEL_LIGHT		= RGBA_TO_INT32 (140,140,132,255);
static const unsigned int	COLOR_BEVEL_DARK		= RGBA_TO_INT32 ( 18, 18, 16,255);

static const unsigned int	COLOR_CELL				= RGBA_TO_INT32 ( 26, 26, 24,240);
static const unsigned int	COLOR_CELL_DIM			= RGBA_TO_INT32 ( 20, 20, 18,240);

static const unsigned int	COLOR_AMBER				= RGB_TO_INT32  (252,204, 92);
static const unsigned int	COLOR_AMBER_DIM		= RGB_TO_INT32  (142,116, 56);
static const unsigned int	COLOR_TEXT_BAD			= RGB_TO_INT32  (236,104, 84);
static const unsigned int	COLOR_SHADOW			= RGB_TO_INT32  (  0,  0,  0);

static const unsigned int	COLOR_TAB_ON			= RGBA_TO_INT32 ( 96, 92, 68,245);
static const unsigned int	COLOR_TAB_OFF			= RGBA_TO_INT32 ( 42, 42, 40,235);

static const unsigned int	COLOR_POWER_GOOD		= RGBA_TO_INT32 ( 64,216, 92,245);
static const unsigned int	COLOR_POWER_WARN		= RGBA_TO_INT32 (238,196, 60,245);
static const unsigned int	COLOR_POWER_BAD		= RGBA_TO_INT32 (232, 62, 48,250);
static const unsigned int	COLOR_POWER_TRACK		= RGBA_TO_INT32 ( 14, 16, 14,245);
static const unsigned int	COLOR_POWER_MARKER	= RGBA_TO_INT32 (255,255,255,235);

//	Icon tints.  The greyed-out value is the one `MerchandiseCtrlClass` already uses for a
//	disabled Purchase Terminal card, so a Commander card and a PT card grey out identically.
static const unsigned int	COLOR_ICON_ON			= 0xFFFFFFFF;
static const unsigned int	COLOR_ICON_OFF			= RGB_TO_INT32  ( 96, 96, 96);
static const unsigned int	COLOR_ICON_BLOCKED	= RGBA_TO_INT32 (120, 62, 54,255);


/***********************************************************************************************
 * Add_Bevel -- one stamped-metal box: raised, or recessed when `raised` is false                *
 *                                                                                             *
 *	The whole C&C sidebar is this shape repeated.  Two L-shaped highlights rather than a border  *
 *	rectangle, because a uniform outline reads as a modern flat panel and the light/dark split   *
 *	is the entire reason the original reads as a physical object.                                *
 *=============================================================================================*/
static void	Add_Bevel(Render2DClass * renderer,const RectClass & rect,unsigned int fill,
							 bool raised,float thickness = 2.0f)
{
	if (renderer == nullptr) return;

	unsigned int top_left		= raised ? COLOR_BEVEL_LIGHT : COLOR_BEVEL_DARK;
	unsigned int bottom_right	= raised ? COLOR_BEVEL_DARK  : COLOR_BEVEL_LIGHT;

	renderer->Add_Quad (rect,fill);

	//	Top and left.
	renderer->Add_Quad (RectClass (rect.Left,rect.Top,rect.Right,rect.Top + thickness),top_left);
	renderer->Add_Quad (RectClass (rect.Left,rect.Top,rect.Left + thickness,rect.Bottom),top_left);

	//	Bottom and right.
	renderer->Add_Quad (RectClass (rect.Left,rect.Bottom - thickness,rect.Right,rect.Bottom),bottom_right);
	renderer->Add_Quad (RectClass (rect.Right - thickness,rect.Top,rect.Right,rect.Bottom),bottom_right);
}


/*
**	What the last rebuild was built from.  The panel is a lot of geometry and almost none of it
**	changes on a typical frame, so a rebuild happens when one of these actually moves rather than
**	every frame.  Credits change constantly, and what they change is a *card status*, which is
**	why every visible card's status is in the signature rather than the credit figure itself.
*/
struct SidebarSignature
{
	SidebarSignature(void) : Open(false),Mode(SIDEBAR_MODE_BUILDINGS),Page(0),CardCount(0),
									 Team(PLAYERTYPE_NEUTRAL),Generation(-1.0f),Requested(-1.0f),
									 Allocated(-1.0f),ScreenWidth(0.0f),ScreenHeight(0.0f),
									 FacilityState(-1)
	{
		for (int i = 0; i < CommanderSidebarModel::PAGE_SIZE; i++) {
			CardId [i] = 0;
			CardStatus [i] = -1;
			CardQueued [i] = -1;
		}
	}

	bool	Matches(const SidebarSignature & other) const
	{
		if (Open != other.Open || Mode != other.Mode || Page != other.Page) return false;
		if (CardCount != other.CardCount || Team != other.Team) return false;
		if (FacilityState != other.FacilityState) return false;
		if (WWMath::Fabs (Generation - other.Generation) > 0.01f) return false;
		if (WWMath::Fabs (Requested  - other.Requested)  > 0.01f) return false;
		if (WWMath::Fabs (Allocated  - other.Allocated)  > 0.01f) return false;
		if (WWMath::Fabs (ScreenWidth  - other.ScreenWidth)  > 0.5f) return false;
		if (WWMath::Fabs (ScreenHeight - other.ScreenHeight) > 0.5f) return false;
		for (int i = 0; i < CommanderSidebarModel::PAGE_SIZE; i++) {
			if (CardId [i] != other.CardId [i]) return false;
			if (CardStatus [i] != other.CardStatus [i]) return false;
			if (CardQueued [i] != other.CardQueued [i]) return false;
		}
		return true;
	}

	bool								Open;
	CommanderSidebarModeType	Mode;
	int								Page;
	int								CardCount;
	int								Team;
	float								Generation;
	float								Requested;
	float								Allocated;
	float								ScreenWidth;
	float								ScreenHeight;
	int								FacilityState;
	int								CardId     [CommanderSidebarModel::PAGE_SIZE];
	int								CardStatus [CommanderSidebarModel::PAGE_SIZE];
	int								CardQueued [CommanderSidebarModel::PAGE_SIZE];
};

static SidebarSignature	_LastSignature;


static bool	Card_Is_Buyable(const CommanderSidebarCard * card)
{
	//	Not a second copy of the rule: every status but AVAILABLE means "not now", and which
	//	kind of "not now" it is gets printed, never recomputed.
	return (card != nullptr) && (card->Status == CARD_AVAILABLE);
}


/***********************************************************************************************
 * Purchase_Page_For_Mode -- which stock purchase page a unit mode is showing, or nullptr        *
 *                                                                                             *
 *	Only so the renderer can fetch a card's *localized* name, which the model deliberately does  *
 *	not carry (a page's name is a unicode string id; the model holds narrow strings).            *
 *=============================================================================================*/
static PurchaseSettingsDefClass *	Purchase_Page_For_Mode(CommanderSidebarModeType mode,int team)
{
	PurchaseSettingsDefClass::TEAM page_team = (team == PLAYERTYPE_GDI)
															? PurchaseSettingsDefClass::TEAM_GDI
															: PurchaseSettingsDefClass::TEAM_NOD;
	switch (mode) {
		case SIDEBAR_MODE_INFANTRY:
			return PurchaseSettingsDefClass::Find_Definition (PurchaseSettingsDefClass::TYPE_CLASSES,page_team);
		case SIDEBAR_MODE_GROUND_VEHICLES:
			return PurchaseSettingsDefClass::Find_Definition (PurchaseSettingsDefClass::TYPE_VEHICLES,page_team);
		default:
			return nullptr;			// BUILDINGS has no purchase page; AIR_VEHICLES has no content
	}
}


static void	Resolve_Card_Label(const CommanderSidebarCard * card,CommanderSidebarModeType mode,
											int team,WideStringClass & out)
{
	out = U_CHAR ("");
	if (card == nullptr) return;

	//	A building card carries its own name straight from the descriptor.
	if (card->DisplayName.Get_Length () > 0) {
		out = WideStringClass (card->DisplayName.Peek_Buffer (),true);
		return;
	}

	/*
	**	A unit card carries a preset id, and the localized name lives on the purchase page beside
	**	it.  Matched by definition id rather than by index, because the model's card list skips
	**	empty page slots and so its indices are not the page's.
	*/
	PurchaseSettingsDefClass * page = Purchase_Page_For_Mode (mode,team);
	if (page == nullptr) return;

	//	PurchaseSettingsDefClass::MAX_ENTRIES is protected; PurchaseAvailabilityClass::ENTRY_COUNT
	//	is the same number, public, and is already the bound CommanderSidebarModel itself walks
	//	the page with -- so the two agree by sharing a constant rather than by coincidence.
	for (int i = 0; i < PurchaseAvailabilityClass::ENTRY_COUNT; i++) {
		if (page->Get_Definition (i) != card->DefinitionId) continue;
		const unichar_t * name = page->Get_Name (i);
		if (name != nullptr) out = name;
		return;
	}
}


static void	Set_Icon_Texture(Render2DClass * renderer,const StringClass & name)
{
	if (renderer == nullptr) return;

	if (name.Get_Length () == 0) {
		//	"Missing art never blocks engine work": a card with no icon still draws its cell, its
		//	name, its cost and its status -- it simply has no picture.
		renderer->Set_Texture ((TextureClass *)nullptr);
		return;
	}

	TextureClass * texture = WW3DAssetManager::Get_Instance ()->Get_Texture (name.Peek_Buffer (),
																									 TextureClass::MIP_LEVELS_1);
	if (texture == nullptr) {
		_MissingIconCount++;
		renderer->Set_Texture ((TextureClass *)nullptr);
		return;
	}
	renderer->Set_Texture (texture);
	REF_PTR_RELEASE (texture);
}


void CommanderSidebarRenderer::Init(bool render_available)
{
	Shutdown ();

	_Available			= render_available;
	_Dirty				= true;
	_MissingIconCount	= 0;
	if (!_Available) return;

	_Panel = new Render2DClass ();
	_Panel->Enable_Texturing (false);
	StyleMgrClass::Configure_Renderer (_Panel);

	for (int i = 0; i < CommanderSidebarModel::PAGE_SIZE; i++) {
		_Icons [i] = new Render2DClass ();
		_Icons [i]->Enable_Texturing (true);
		StyleMgrClass::Configure_Renderer (_Icons [i]);
	}

	//	Render2DSentenceClass positions in raw screen pixels and has no coordinate range of its
	//	own -- it installs Get_Screen_Resolution internally -- so it is only given a font.
	_Text = new Render2DSentenceClass ();
	StyleMgrClass::Assign_Font (_Text,StyleMgrClass::FONT_INGAME_TXT);
}


void CommanderSidebarRenderer::Shutdown(void)
{
	if (_Panel != nullptr) { delete _Panel; _Panel = nullptr; }
	for (int i = 0; i < CommanderSidebarModel::PAGE_SIZE; i++) {
		if (_Icons [i] != nullptr) { delete _Icons [i]; _Icons [i] = nullptr; }
	}
	if (_Text != nullptr) { delete _Text; _Text = nullptr; }

	_Available		= false;
	_Dirty			= true;
	_LastSignature	= SidebarSignature ();
}


void CommanderSidebarRenderer::Invalidate(void)					{ _Dirty = true; }
int  CommanderSidebarRenderer::Get_Rebuild_Count(void)			{ return _RebuildCount; }
int  CommanderSidebarRenderer::Get_Missing_Icon_Count(void)	{ return _MissingIconCount; }


static int	Facility_State_Code(void)
{
	const CommanderFacilityState & facility = CommanderSidebarModel::Get_Facility_State ();
	if (!facility.Bound)						return 0;
	if (!facility.Alive)						return 1;
	if (!facility.ConstructionComplete)	return 2;
	if (!facility.Powered)					return 3;
	return 4;
}


static SidebarSignature	Build_Signature(void)
{
	SidebarSignature signature;
	signature.Open				= CommanderSidebarModel::Is_Open ();
	signature.Mode				= CommanderSidebarModel::Get_Mode ();
	signature.Page				= CommanderSidebarModel::Get_Page ();
	signature.CardCount		= CommanderSidebarModel::Get_Card_Count ();
	signature.Team				= CommanderSidebarModel::Get_Team ();
	signature.FacilityState	= Facility_State_Code ();

	const RectClass & screen = Render2DClass::Get_Screen_Resolution ();
	signature.ScreenWidth	= screen.Width ();
	signature.ScreenHeight	= screen.Height ();

	CommanderPowerDisplay power;
	CommanderSidebarModel::Get_Power_Display (&power);
	signature.Generation	= power.Generation;
	signature.Requested	= power.RequestedDemand;
	signature.Allocated	= power.AllocatedDemand;

	const CommanderSidebarCard * cards [CommanderSidebarModel::PAGE_SIZE] = { nullptr };
	int count = CommanderSidebarModel::Get_Visible_Cards (cards,CommanderSidebarModel::PAGE_SIZE);
	for (int i = 0; i < count; i++) {
		if (cards [i] == nullptr) continue;
		signature.CardId     [i] = (cards [i]->DefinitionId != 0)
											? cards [i]->DefinitionId
											: (int)cards [i]->DescriptorName.Get_Length ();
		signature.CardStatus [i] = (int)cards [i]->Status;
		signature.CardQueued [i] = cards [i]->QueuedCount;
	}
	return signature;
}


/***********************************************************************************************
 * Draw_Power_Column -- the C&C power bar: vertical, with a drain marker, not a second bar       *
 *                                                                                             *
 *	The original shows capacity as the bar and consumption as a sliding notch, and recolours the *
 *	whole bar as the notch climbs.  That is a better fit for Section 42's list than two stacked  *
 *	horizontal bars would be: "headroom or overload" is the gap between the fill and the notch,  *
 *	read at a glance, and an overload is the notch leaving the top of the bar *and* the bar      *
 *	going red.                                                                                  *
 *=============================================================================================*/
static void	Draw_Power_Column(Render2DClass * panel,Render2DSentenceClass * text,const RectClass & strip)
{
	CommanderPowerDisplay power;
	CommanderSidebarModel::Get_Power_Display (&power);

	Add_Bevel (panel,strip,COLOR_POWER_TRACK,false,2.0f);

	RectClass track (strip.Left + 3.0f,strip.Top + 3.0f,strip.Right - 3.0f,strip.Bottom - 3.0f);

	//	Both the fill and the notch are measured against the same reference, so the notch can
	//	legitimately sit above the fill -- which is exactly what an overload looks like.
	float reference = power.Generation;
	if (power.RequestedDemand > reference) reference = power.RequestedDemand;
	if (reference <= 0.0f) reference = 1.0f;

	unsigned int bar_colour = COLOR_POWER_GOOD;
	if (power.Overloaded) {
		bar_colour = COLOR_POWER_BAD;
	} else if (power.Generation > 0.0f && (power.RequestedDemand / power.Generation) > 0.8f) {
		bar_colour = COLOR_POWER_WARN;
	}

	//	Capacity fills from the bottom up, the way the original does.
	if (power.Generation > 0.0f) {
		float fraction = power.Generation / reference;
		RectClass fill = track;
		fill.Top = track.Bottom - (track.Height () * fraction);
		panel->Add_Quad (fill,bar_colour);
	}

	//	The drain notch.  Drawn even at zero demand, sitting on the floor of the track, so its
	//	absence is never mistaken for "the bar is broken".
	{
		float fraction = power.RequestedDemand / reference;
		if (fraction > 1.0f) fraction = 1.0f;
		float y = track.Bottom - (track.Height () * fraction);
		RectClass notch (track.Left - 2.0f,y - 1.5f,track.Right + 2.0f,y + 1.5f);
		panel->Add_Quad (notch,COLOR_POWER_MARKER);
	}

	//	"PWR" down at the base, so the column is identifiable the first time it is seen.
	RectClass caption (strip.Left - 2.0f,strip.Bottom + 1.0f,strip.Right + 2.0f,strip.Bottom + 15.0f);
	StyleMgrClass::Render_Text (U_CHAR ("PWR"),text,
										 power.Overloaded ? COLOR_TEXT_BAD : COLOR_AMBER_DIM,COLOR_SHADOW,
										 caption,true,true,StyleMgrClass::CENTER_JUSTIFY,true);
}


void CommanderSidebarRenderer::Update(void)
{
	if (!_Available || _Panel == nullptr || _Text == nullptr) return;

	SidebarSignature signature = Build_Signature ();
	if (!_Dirty && signature.Matches (_LastSignature)) {
		return;			// nothing the panel shows has moved; replay last frame's geometry
	}
	_LastSignature	= signature;
	_Dirty			= false;
	_RebuildCount++;

	_Panel->Reset ();
	_Text->Reset ();
	for (int i = 0; i < CommanderSidebarModel::PAGE_SIZE; i++) {
		if (_Icons [i] != nullptr) _Icons [i]->Reset ();
	}

	//	Section 42: "The sidebar exists in Commander Mode only."  Closed means no geometry at all,
	//	so the ordinary FPS HUD is untouched -- not a hidden panel that still costs draws.
	if (!CommanderSidebarModel::Is_Open ()) return;

	const RectClass & screen = Render2DClass::Get_Screen_Resolution ();

	/*
	**	Layout.  Proportional to the screen rather than fixed pixels, so the strip keeps its
	**	share of the frame at any resolution; what Section 42 fixes is the eight cells (2 x 4),
	**	not the pixels they occupy.  The strip runs the full height and is flush to the right
	**	edge, the way the original sits against the side of the screen.
	*/
	float panel_width = screen.Width () * 0.235f;
	if (panel_width < 232.0f) panel_width = 232.0f;

	RectClass panel (screen.Right - panel_width,screen.Top,screen.Right,screen.Bottom);
	Add_Bevel (_Panel,panel,COLOR_METAL,true,3.0f);

	//	The power column owns the left edge of the strip; everything else lives to the right of
	//	it.  That is the original's arrangement and it is why the bar is always in the same place
	//	no matter which mode is showing.
	float power_width	= 16.0f;
	float content_left	= panel.Left + 6.0f + power_width + 8.0f;
	float content_right	= panel.Right - 8.0f;
	float content_width	= content_right - content_left;
	float cursor_y			= panel.Top + 8.0f;

	/*
	**	Mode tabs.  Four raised buttons, the active one sunk -- pressed, rather than merely a
	**	different colour, which is how the original shows a held state.
	*/
	{
		float tab_height	= 22.0f;
		float tab_width	= content_width / (float)SIDEBAR_MODE_COUNT;
		CommanderSidebarModeType mode = CommanderSidebarModel::Get_Mode ();

		for (int m = 0; m < SIDEBAR_MODE_COUNT; m++) {

			RectClass tab (content_left + (tab_width * (float)m),cursor_y,
								content_left + (tab_width * (float)(m + 1)) - 2.0f,cursor_y + tab_height);

			bool active = ((int)mode == m);
			Add_Bevel (_Panel,tab,active ? COLOR_TAB_ON : COLOR_TAB_OFF,!active,2.0f);

			//	The mode's own name, abbreviated to what fits a quarter of the strip.
			const char * name = CommanderSidebarModel::Get_Mode_Name ((CommanderSidebarModeType)m);
			char label [8];
			::snprintf (label,sizeof (label),"%.4s",name);
			WideStringClass wide (label,true);

			StyleMgrClass::Render_Text (wide,_Text,active ? COLOR_AMBER : COLOR_AMBER_DIM,COLOR_SHADOW,
												 tab,true,true,StyleMgrClass::CENTER_JUSTIFY,true);
		}
		cursor_y += tab_height + 5.0f;
	}

	/*
	**	The facility readout.  Section 42 requires the panel to make it immediately obvious when
	**	a bound facility has stopped being usable, so this says which state it is in rather than
	**	simply going blank.
	*/
	{
		RectClass line (content_left,cursor_y,content_right,cursor_y + 18.0f);
		Add_Bevel (_Panel,line,COLOR_CELL,false,1.0f);

		const CommanderFacilityState & facility = CommanderSidebarModel::Get_Facility_State ();
		WideStringClass text;
		unsigned int colour = COLOR_AMBER_DIM;

		if (!facility.Bound) {
			text = U_CHAR ("NO FACILITY");
		} else if (!facility.Alive) {
			text = U_CHAR ("FACILITY DESTROYED");				colour = COLOR_TEXT_BAD;
		} else if (!facility.ConstructionComplete) {
			text = U_CHAR ("FACILITY BUILDING");				colour = COLOR_TEXT_BAD;
		} else if (!facility.Powered) {
			text = U_CHAR ("FACILITY OFFLINE");					colour = COLOR_TEXT_BAD;
		} else {
			text = WideStringClass (facility.DescriptorName.Peek_Buffer (),true);
			colour = COLOR_AMBER;
		}

		StyleMgrClass::Render_Text (text,_Text,colour,COLOR_SHADOW,line,true,true,
											 StyleMgrClass::CENTER_JUSTIFY,true);
		cursor_y += 22.0f;
	}

	/*
	**	The build cells: two rows of four, Section 42's own shape, drawn as recessed slots in the
	**	metal.  Eight is the visible page size and not a content cap, which is what the page line
	**	under the grid is for.
	*/
	const CommanderSidebarCard * cards [CommanderSidebarModel::PAGE_SIZE] = { nullptr };
	int visible = CommanderSidebarModel::Get_Visible_Cards (cards,CommanderSidebarModel::PAGE_SIZE);

	float cell_width	= content_width / (float)CommanderSidebarModel::VISIBLE_COLUMNS;
	float cell_height	= cell_width * 1.22f;
	float grid_top		= cursor_y;

	for (int slot = 0; slot < CommanderSidebarModel::PAGE_SIZE; slot++) {

		int column	= slot % CommanderSidebarModel::VISIBLE_COLUMNS;
		int row		= slot / CommanderSidebarModel::VISIBLE_COLUMNS;

		RectClass cell (content_left + (cell_width * (float)column) + 2.0f,
							 grid_top    + (cell_height * (float)row)  + 2.0f,
							 content_left + (cell_width * (float)(column + 1)) - 2.0f,
							 grid_top    + (cell_height * (float)(row + 1))  - 2.0f);

		const CommanderSidebarCard * card = (slot < visible) ? cards [slot] : nullptr;

		if (card == nullptr) {
			//	An empty slot is drawn as an empty slot.  A half-full page must still read as a
			//	2 x 4 grid, or it stops reading as a grid at all.
			Add_Bevel (_Panel,cell,COLOR_CELL_DIM,false,2.0f);
			continue;
		}

		bool buyable = Card_Is_Buyable (card);
		Add_Bevel (_Panel,cell,buyable ? COLOR_CELL : COLOR_CELL_DIM,false,2.0f);

		//	Icon: the upper part of the slot, the split MerchandiseCtrlClass already uses for a
		//	Purchase Terminal card.
		RectClass icon (cell.Left + 3.0f,cell.Top + 3.0f,cell.Right - 3.0f,
							 cell.Top + ((cell.Bottom - cell.Top) * 0.60f));

		Set_Icon_Texture (_Icons [slot],card->IconTexture);
		if (_Icons [slot]->Peek_Texture () != nullptr) {
			unsigned int tint = buyable ? COLOR_ICON_ON
												: ((card->Status == CARD_INSUFFICIENT_CREDITS)
														? COLOR_ICON_OFF : COLOR_ICON_BLOCKED);
			_Icons [slot]->Add_Quad (icon,RectClass (0.0f,0.0f,1.0f,1.0f),tint);
		}

		/*
		**	The cost, stamped over the bottom of the icon on its own dark strip -- the original's
		**	arrangement, and it keeps the cost legible whatever the icon behind it happens to be.
		*/
		{
			RectClass cost_strip (icon.Left,icon.Bottom - 14.0f,icon.Right,icon.Bottom);
			_Panel->Add_Quad (cost_strip,COLOR_METAL_DARK);

			WideStringClass cost;
			cost.Format (U_CHAR ("%d"),card->Cost);
			StyleMgrClass::Render_Text (cost,_Text,buyable ? COLOR_AMBER : COLOR_AMBER_DIM,COLOR_SHADOW,
												 cost_strip,true,true,StyleMgrClass::RIGHT_JUSTIFY,true);
		}

		//	Name.
		RectClass name_rect (cell.Left + 2.0f,icon.Bottom + 1.0f,cell.Right - 2.0f,icon.Bottom + 15.0f);
		WideStringClass label;
		Resolve_Card_Label (card,CommanderSidebarModel::Get_Mode (),CommanderSidebarModel::Get_Team (),label);
		if (label.Get_Length () > 0) {
			StyleMgrClass::Render_Text (label,_Text,buyable ? COLOR_AMBER : COLOR_AMBER_DIM,COLOR_SHADOW,
												 name_rect,true,true,StyleMgrClass::CENTER_JUSTIFY,true);
		}

		/*
		**	The bottom line carries the building's effect on the grid when it can be built, and
		**	the *reason* when it cannot -- Section 42's "prerequisite/disabled reason", printed
		**	from the model's own status name, so there is exactly one wording of each reason in
		**	the codebase.
		*/
		RectClass detail (cell.Left + 2.0f,name_rect.Bottom,cell.Right - 2.0f,cell.Bottom - 3.0f);

		if (!buyable) {
			WideStringClass reason (CommanderSidebarModel::Get_Status_Name (card->Status),true);
			StyleMgrClass::Render_Text (reason,_Text,COLOR_TEXT_BAD,COLOR_SHADOW,detail,true,true,
												 StyleMgrClass::CENTER_JUSTIFY,true);
		} else if (card->PowerGeneration > 0.0f) {
			WideStringClass power;
			power.Format (U_CHAR ("+%d PWR"),(int)card->PowerGeneration);
			StyleMgrClass::Render_Text (power,_Text,COLOR_POWER_GOOD,COLOR_SHADOW,detail,true,true,
												 StyleMgrClass::CENTER_JUSTIFY,true);
		} else if (card->PowerConsumption > 0.0f) {
			WideStringClass power;
			power.Format (U_CHAR ("-%d PWR"),(int)card->PowerConsumption);
			StyleMgrClass::Render_Text (power,_Text,COLOR_AMBER_DIM,COLOR_SHADOW,detail,true,true,
												 StyleMgrClass::CENTER_JUSTIFY,true);
		}

		//	Queue badge, upper-left over the icon, only when something of this kind is queued.
		if (card->QueuedCount > 0) {
			RectClass badge (cell.Left + 3.0f,cell.Top + 3.0f,cell.Left + 21.0f,cell.Top + 18.0f);
			Add_Bevel (_Panel,badge,COLOR_TAB_ON,true,1.0f);
			WideStringClass count;
			count.Format (U_CHAR ("%d"),card->QueuedCount);
			StyleMgrClass::Render_Text (count,_Text,COLOR_AMBER,COLOR_SHADOW,badge,true,true,
												 StyleMgrClass::CENTER_JUSTIFY,true);
		}
	}

	cursor_y = grid_top + (cell_height * (float)CommanderSidebarModel::VISIBLE_ROWS) + 5.0f;

	/*
	**	Paging.  Two stubby arrow buttons flanking the readout, which is how the original offers
	**	a list longer than the strip.  They are drawn pressed-out whether or not there is another
	**	page, and the readout is what says which page of how many -- a disabled-looking arrow on
	**	a single-page list would be more confusing than an inert one.
	*/
	{
		float button = 18.0f;
		RectClass up   (content_left,cursor_y,content_left + button,cursor_y + button);
		RectClass down (content_right - button,cursor_y,content_right,cursor_y + button);
		RectClass label (up.Right + 4.0f,cursor_y,down.Left - 4.0f,cursor_y + button);

		Add_Bevel (_Panel,up,COLOR_TAB_OFF,true,2.0f);
		Add_Bevel (_Panel,down,COLOR_TAB_OFF,true,2.0f);
		StyleMgrClass::Render_Text (U_CHAR ("<"),_Text,COLOR_AMBER,COLOR_SHADOW,up,true,true,
											 StyleMgrClass::CENTER_JUSTIFY,true);
		StyleMgrClass::Render_Text (U_CHAR (">"),_Text,COLOR_AMBER,COLOR_SHADOW,down,true,true,
											 StyleMgrClass::CENTER_JUSTIFY,true);

		WideStringClass text;
		text.Format (U_CHAR ("%d / %d"),CommanderSidebarModel::Get_Page () + 1,
						 CommanderSidebarModel::Get_Page_Count ());
		StyleMgrClass::Render_Text (text,_Text,COLOR_AMBER_DIM,COLOR_SHADOW,label,true,true,
											 StyleMgrClass::CENTER_JUSTIFY,true);
		cursor_y += button + 6.0f;
	}

	/*
	**	The numeric power readout, under the grid.  The column on the left says the shape of the
	**	situation at a glance; these say what the numbers actually are, which is what Section 42
	**	asks for by name.
	*/
	{
		CommanderPowerDisplay power;
		CommanderSidebarModel::Get_Power_Display (&power);

		RectClass box (content_left,cursor_y,content_right,cursor_y + 38.0f);
		Add_Bevel (_Panel,box,COLOR_CELL,false,2.0f);

		RectClass line_one (box.Left + 3.0f,box.Top + 2.0f,box.Right - 3.0f,box.Top + 18.0f);
		WideStringClass text;
		text.Format (U_CHAR ("GEN %d   REQ %d   USE %d"),
						 (int)power.Generation,(int)power.RequestedDemand,(int)power.AllocatedDemand);
		StyleMgrClass::Render_Text (text,_Text,COLOR_AMBER,COLOR_SHADOW,line_one,true,true,
											 StyleMgrClass::CENTER_JUSTIFY,true);

		RectClass line_two (box.Left + 3.0f,box.Top + 18.0f,box.Right - 3.0f,box.Bottom - 2.0f);
		WideStringClass status;
		if (power.Overloaded) {
			status.Format (U_CHAR ("LOW POWER -- %d OFFLINE"),power.ShedCount);
		} else {
			status.Format (U_CHAR ("HEADROOM %d   OFFLINE %d"),(int)power.Headroom,power.ShedCount);
		}
		StyleMgrClass::Render_Text (status,_Text,power.Overloaded ? COLOR_TEXT_BAD : COLOR_AMBER_DIM,
											 COLOR_SHADOW,line_two,true,true,StyleMgrClass::CENTER_JUSTIFY,true);

		cursor_y = box.Bottom + 6.0f;
	}

	//	The power column last, now that the content height is known: it spans the tabs down to
	//	the numeric readout, so it is always as tall as the strip's working area.
	{
		RectClass strip (panel.Left + 6.0f,grid_top,panel.Left + 6.0f + power_width,cursor_y - 18.0f);
		if (strip.Height () > 24.0f) {
			Draw_Power_Column (_Panel,_Text,strip);
		}
	}
}


void CommanderSidebarRenderer::Render(void)
{
	if (!_Available || _Panel == nullptr || _Text == nullptr) return;
	if (!CommanderSidebarModel::Is_Open ()) return;

	//	Panel first, then icons over their slots, then every label over both.
	_Panel->Render ();
	for (int i = 0; i < CommanderSidebarModel::PAGE_SIZE; i++) {
		if (_Icons [i] != nullptr) _Icons [i]->Render ();
	}
	_Text->Render ();
}
