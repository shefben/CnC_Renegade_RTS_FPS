/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commandercommandpaletterenderer.cpp   $*
 *                                                                                             *
 *	Roadmap Section 45.  See commandercommandpaletterenderer.h.                                 *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commandercommandpaletterenderer.h"

#include "commandersidebar.h"
#include "commanderselection.h"
#include "render2d.h"
#include "render2dsentence.h"
#include "stylemgr.h"
#include "unichar.h"
#include "widestring.h"

#include <stdio.h>


static bool							_Available		= false;
static int							_RebuildCount	= 0;

static Render2DClass *			_Panel	= nullptr;
static Render2DSentenceClass *	_Text		= nullptr;

static bool			_HitValid = false;
static RectClass	_HitPanel;
static RectClass	_HitButtons [PALETTE_COMMAND_COUNT];


//	The sidebar's palette, so the two panels read as one piece of equipment.
static const unsigned int	COLOR_METAL			= RGBA_TO_INT32 ( 66, 66, 62,235);
static const unsigned int	COLOR_BEVEL_LIGHT	= RGBA_TO_INT32 (140,140,132,255);
static const unsigned int	COLOR_BEVEL_DARK	= RGBA_TO_INT32 ( 18, 18, 16,255);
static const unsigned int	COLOR_BUTTON		= RGBA_TO_INT32 ( 42, 42, 40,235);
static const unsigned int	COLOR_BUTTON_ARMED= RGBA_TO_INT32 ( 96, 92, 68,245);
static const unsigned int	COLOR_AMBER			= RGB_TO_INT32  (252,204, 92);
static const unsigned int	COLOR_AMBER_DIM	= RGB_TO_INT32  (142,116, 56);
static const unsigned int	COLOR_SHADOW		= RGB_TO_INT32  (  0,  0,  0);


static void	Add_Bevel(Render2DClass * renderer,const RectClass & rect,unsigned int fill,
							 bool raised,float thickness)
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


void CommanderCommandPaletteRenderer::Init(bool render_available)
{
	Shutdown ();

	_Available		= render_available;
	_RebuildCount	= 0;
	if (!_Available) return;

	_Panel = new Render2DClass ();
	_Panel->Enable_Texturing (false);
	StyleMgrClass::Configure_Renderer (_Panel);

	_Text = new Render2DSentenceClass ();
	StyleMgrClass::Assign_Font (_Text,StyleMgrClass::FONT_INGAME_TXT);
}


void CommanderCommandPaletteRenderer::Shutdown(void)
{
	if (_Panel != nullptr) { delete _Panel; _Panel = nullptr; }
	if (_Text  != nullptr) { delete _Text;  _Text  = nullptr; }

	_Available	= false;
	_HitValid	= false;
}


void CommanderCommandPaletteRenderer::Update(void)
{
	if (!_Available || (_Panel == nullptr) || (_Text == nullptr)) return;

	/*
	**	Rebuilt unconditionally every frame rather than on a change signature.  The armed button
	**	and the selection count both change from input, and the panel is nine boxes and eleven
	**	short strings -- a signature check would cost more to maintain than the rebuild saves.
	*/
	_Panel->Reset ();
	_Text->Reset ();
	_HitValid = false;

	if (!CommanderCommandPalette::Is_Open ()) return;

	_RebuildCount++;

	const RectClass & screen = Render2DClass::Get_Screen_Resolution ();

	/*
	**	Laid out relative to the production sidebar rather than to the screen, so the two never
	**	overlap at any resolution.  The sidebar owns the right edge down the full height; this
	**	sits immediately to its left, against the bottom.
	*/
	float sidebar_width = screen.Width () * 0.235f;
	if (sidebar_width < 232.0f) sidebar_width = 232.0f;
	if (!CommanderSidebarModel::Is_Open ()) sidebar_width = 0.0f;

	float button = screen.Height () * 0.072f;
	if (button < 46.0f) button = 46.0f;
	if (button > 76.0f) button = 76.0f;

	const float padding	= 4.0f;
	const float margin	= 8.0f;
	const float header	= 18.0f;

	const float columns	= (float)(int)CommanderCommandPalette::COLUMNS;
	const float rows		= (float)(int)CommanderCommandPalette::ROWS;

	float grid_width	= (button * columns) + (padding * (columns + 1.0f));
	float grid_height	= (button * rows)    + (padding * (rows    + 1.0f));

	RectClass panel (screen.Right - sidebar_width - margin - grid_width,
						  screen.Bottom - margin - grid_height - header - padding,
						  screen.Right - sidebar_width - margin,
						  screen.Bottom - margin);

	Add_Bevel (_Panel,panel,COLOR_METAL,true,3.0f);
	_HitPanel = panel;

	//	Header: how many units the next click will actually reach.  A Commander needs that number
	//	more than a title, and it is the one thing about the selection the grid cannot show.
	{
		RectClass caption (panel.Left + padding,panel.Top + 2.0f,panel.Right - padding,
								 panel.Top + 2.0f + header);

		char line [40];
		::snprintf (line,sizeof (line),"%d SELECTED",CommanderSelection::Get_Count ());
		WideStringClass text (line,true);

		StyleMgrClass::Render_Text (text,_Text,COLOR_AMBER,COLOR_SHADOW,caption,true,true,
											 StyleMgrClass::CENTER_JUSTIFY,true);
	}

	float grid_top = panel.Top + header + padding;

	for (int index = 0; index < PALETTE_COMMAND_COUNT; index++) {

		int column	= index % CommanderCommandPalette::COLUMNS;
		int row		= index / CommanderCommandPalette::COLUMNS;

		RectClass cell (panel.Left + padding + (float)column * (button + padding),
							 grid_top    + padding + (float)row    * (button + padding),
							 panel.Left + padding + (float)column * (button + padding) + button,
							 grid_top    + padding + (float)row    * (button + padding) + button);

		CommanderPaletteCommandType command = (CommanderPaletteCommandType)index;

		//	The armed button is drawn sunk rather than merely tinted, the same way the sidebar
		//	shows its active mode tab -- a held state has to read as held.
		bool armed = CommanderCommandPalette::Is_Armed () &&
						 (CommanderCommandPalette::Get_Armed () == command);

		Add_Bevel (_Panel,cell,armed ? COLOR_BUTTON_ARMED : COLOR_BUTTON,!armed,2.0f);
		_HitButtons [index] = cell;

		WideStringClass label (CommanderCommandPalette::Get_Label (command),true);
		StyleMgrClass::Render_Text (label,_Text,armed ? COLOR_AMBER : COLOR_AMBER_DIM,COLOR_SHADOW,
											 cell,true,true,StyleMgrClass::CENTER_JUSTIFY,true);
	}

	_HitValid = true;
}


void CommanderCommandPaletteRenderer::Render(void)
{
	if (!_Available || (_Panel == nullptr) || (_Text == nullptr)) return;
	if (!CommanderCommandPalette::Is_Open ()) return;

	_Panel->Render ();
	_Text->Render ();
}


bool CommanderCommandPaletteRenderer::Hit_Test(float x,float y,
															  CommanderPaletteCommandType * out_command,
															  bool * out_inside_panel)
{
	if (out_inside_panel != nullptr) {
		*out_inside_panel = false;
	}

	if (!_Available || !_HitValid) return false;
	if (!CommanderCommandPalette::Is_Open ()) return false;
	if (!Rect_Contains (_HitPanel,x,y)) return false;

	if (out_inside_panel != nullptr) {
		*out_inside_panel = true;
	}

	for (int index = 0; index < PALETTE_COMMAND_COUNT; index++) {
		if (!Rect_Contains (_HitButtons [index],x,y)) continue;
		if (out_command != nullptr) {
			*out_command = (CommanderPaletteCommandType)index;
		}
		return true;
	}

	return false;
}


int CommanderCommandPaletteRenderer::Get_Rebuild_Count(void)	{ return _RebuildCount; }

class CommanderCommandPaletteRendererOverlayClass : public IRTSOverlayComponent
{
public:
	virtual void	Render_RTS_Overlay(RenderInfoClass & /* rinfo */) override	{ CommanderCommandPaletteRenderer::Render (); }
	virtual bool	Is_RTS_Overlay_Visible(void) const override				{ return CommanderCommandPalette::Is_Open (); }
};

static CommanderCommandPaletteRendererOverlayClass	_OverlayComponent;

IRTSOverlayComponent & CommanderCommandPaletteRenderer::Get_Overlay_Component(void)	{ return _OverlayComponent; }
