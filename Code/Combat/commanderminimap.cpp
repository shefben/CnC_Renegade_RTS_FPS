/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderminimap.cpp                  $*
 *                                                                                             *
 *	See commanderminimap.h.                                                                      *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commanderminimap.h"

#include "aabox.h"
#include "commandermodecontroller.h"
#include "minimapbaselayer.h"
#include "render2d.h"
#include "worldterrainsystem.h"


/***********************************************************************************************
 * CommanderMinimap                                                                            *
 *=============================================================================================*/

bool CommanderMinimap::Get_World_Extent(float * min_x,float * min_y,float * max_x,float * max_y)
{
	if (!MinimapBaseLayer::Has_Layer ()) return false;

	int count_x = MinimapBaseLayer::Get_Cell_Count_X ();
	int count_y = MinimapBaseLayer::Get_Cell_Count_Y ();
	if ((count_x <= 0) || (count_y <= 0)) return false;

	AABoxClass first;
	AABoxClass last;
	if (!WorldTerrainSystem::Get_Patch_Bounds (0,0,&first)) return false;
	if (!WorldTerrainSystem::Get_Patch_Bounds (count_x - 1,count_y - 1,&last)) return false;

	*min_x = first.Center.X - first.Extent.X;
	*min_y = first.Center.Y - first.Extent.Y;
	*max_x = last.Center.X + last.Extent.X;
	*max_y = last.Center.Y + last.Extent.Y;
	return (*max_x > *min_x) && (*max_y > *min_y);
}


bool CommanderMinimap::World_To_Panel(const Vector3 & world,const RectClass & panel,float * x,float * y)
{
	float min_x,min_y,max_x,max_y;
	if (!Get_World_Extent (&min_x,&min_y,&max_x,&max_y)) return false;

	*x = panel.Left + ((world.X - min_x) / (max_x - min_x)) * panel.Width ();
	*y = panel.Bottom - ((world.Y - min_y) / (max_y - min_y)) * panel.Height ();
	return true;
}


bool CommanderMinimap::Panel_To_World(float x,float y,const RectClass & panel,Vector3 * world)
{
	float min_x,min_y,max_x,max_y;
	if (!Get_World_Extent (&min_x,&min_y,&max_x,&max_y)) return false;
	if ((panel.Width () <= 0.0f) || (panel.Height () <= 0.0f)) return false;

	world->X = min_x + ((x - panel.Left) / panel.Width ()) * (max_x - min_x);
	world->Y = min_y + ((panel.Bottom - y) / panel.Height ()) * (max_y - min_y);
	world->Z = 0.0f;

	float height = 0.0f;
	if (WorldTerrainSystem::Sample_Height (world->X,world->Y,&height)) {
		world->Z = height;
	}
	return true;
}


unsigned int CommanderMinimap::Cell_Color(const MinimapCellClass & cell,float min_height,float max_height)
{
	if (cell.IsWater)			return RGBA_TO_INT32 ( 38, 72,118,255);
	if (cell.IsCliffBand)	return RGBA_TO_INT32 ( 58, 52, 46,255);
	if (cell.HasRoad)			return RGBA_TO_INT32 (150,132, 96,255);

	float span = max_height - min_height;
	float t = (span > 0.001f) ? ((cell.AverageHeight - min_height) / span) : 0.5f;
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;

	//	Low ground green, high ground a lighter olive.
	int r = (int)( 52.0f + t * 70.0f);
	int g = (int)( 84.0f + t * 58.0f);
	int b = (int)( 40.0f + t * 30.0f);
	return RGBA_TO_INT32 (r,g,b,255);
}


unsigned int CommanderMinimap::Marker_Color(int marker_type)
{
	switch (marker_type) {
		case MINIMAP_MARKER_BASE:				return RGBA_TO_INT32 (252,204, 92,255);	// amber, the Commander UI's own colour
		case MINIMAP_MARKER_TIBERIUM_FIELD:	return RGBA_TO_INT32 ( 96,232, 72,255);
		case MINIMAP_MARKER_SETTLEMENT:		return RGBA_TO_INT32 (210,210,210,255);
		case MINIMAP_MARKER_POI:				return RGBA_TO_INT32 (200,120,220,255);
		case MINIMAP_MARKER_BRIDGE:			return RGBA_TO_INT32 (232,160, 64,255);
		case MINIMAP_MARKER_TUNNEL_PORTAL:	return RGBA_TO_INT32 (120,120,120,255);
		default:										return RGBA_TO_INT32 (255,255,255,255);
	}
}


/***********************************************************************************************
 * CommanderMinimapRenderer                                                                    *
 *=============================================================================================*/

static bool					_Available	= false;
static bool					_Open			= false;
static Render2DClass *	_Panel		= nullptr;
static RectClass			_Frame;
static RectClass			_Map;

static const unsigned int	COLOR_METAL			= RGBA_TO_INT32 ( 66, 66, 62,235);
static const unsigned int	COLOR_BEVEL_DARK	= RGBA_TO_INT32 ( 18, 18, 16,255);
static const unsigned int	COLOR_FOCUS			= RGBA_TO_INT32 (255,255,255,255);


void CommanderMinimapRenderer::Init(bool render_available)
{
	Shutdown ();

	_Available = render_available;
	if (!_Available) return;

	_Panel = new Render2DClass ();
	_Panel->Enable_Texturing (false);
	_Panel->Set_Coordinate_Range (Render2DClass::Get_Screen_Resolution ());
}


void CommanderMinimapRenderer::Shutdown(void)
{
	if (_Panel != nullptr) { delete _Panel; _Panel = nullptr; }
	_Available	= false;
	_Open			= false;
}


void CommanderMinimapRenderer::Update(void)
{
	if (!_Available || (_Panel == nullptr)) return;

	_Panel->Reset ();
	_Open = false;

	if (!CommanderModeController::Is_Active () || !MinimapBaseLayer::Has_Layer ()) return;

	int count_x = MinimapBaseLayer::Get_Cell_Count_X ();
	int count_y = MinimapBaseLayer::Get_Cell_Count_Y ();
	if ((count_x <= 0) || (count_y <= 0)) return;

	float min_x,min_y,max_x,max_y;
	if (!CommanderMinimap::Get_World_Extent (&min_x,&min_y,&max_x,&max_y)) return;

	const RectClass & screen = Render2DClass::Get_Screen_Resolution ();
	_Panel->Set_Coordinate_Range (screen);

	//	Bottom-left, sized to the world's aspect inside a square budget.
	const float margin	= 8.0f;
	const float frame		= 4.0f;
	float budget = screen.Height () * 0.28f;
	if (budget < 160.0f) budget = 160.0f;
	float aspect = (max_x - min_x) / (max_y - min_y);
	float map_w = (aspect >= 1.0f) ? budget : budget * aspect;
	float map_h = (aspect >= 1.0f) ? budget / aspect : budget;

	_Map = RectClass (screen.Left + margin + frame,screen.Bottom - margin - frame - map_h,
							screen.Left + margin + frame + map_w,screen.Bottom - margin - frame);
	_Frame = RectClass (_Map.Left - frame,_Map.Top - frame,_Map.Right + frame,_Map.Bottom + frame);

	_Panel->Add_Quad (_Frame,COLOR_METAL);

	//	The height range, once, so shading is relative to this world.
	float low = 1.0e9f;
	float high = -1.0e9f;
	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {
			MinimapCellClass cell;
			if (!MinimapBaseLayer::Get_Cell (px,py,&cell)) continue;
			if (cell.AverageHeight < low) low = cell.AverageHeight;
			if (cell.AverageHeight > high) high = cell.AverageHeight;
		}
	}

	float cell_w = _Map.Width () / (float)count_x;
	float cell_h = _Map.Height () / (float)count_y;
	for (int py = 0; py < count_y; py++) {
		for (int px = 0; px < count_x; px++) {
			MinimapCellClass cell;
			if (!MinimapBaseLayer::Get_Cell (px,py,&cell)) continue;
			float left	= _Map.Left + (float)px * cell_w;
			float bottom	= _Map.Bottom - (float)py * cell_h;
			_Panel->Add_Quad (RectClass (left,bottom - cell_h,left + cell_w,bottom),
									CommanderMinimap::Cell_Color (cell,low,high));
		}
	}

	for (int index = 0; index < MinimapBaseLayer::Get_Marker_Count (); index++) {
		MinimapMarker marker;
		if (!MinimapBaseLayer::Get_Marker (index,&marker)) continue;
		float x,y;
		if (!CommanderMinimap::World_To_Panel (marker.Position,_Map,&x,&y)) continue;
		float half = (marker.Type == MINIMAP_MARKER_BASE) ? 4.0f : 2.5f;
		_Panel->Add_Quad (RectClass (x - half - 1.0f,y - half - 1.0f,x + half + 1.0f,y + half + 1.0f),COLOR_BEVEL_DARK);
		_Panel->Add_Quad (RectClass (x - half,y - half,x + half,y + half),CommanderMinimap::Marker_Color (marker.Type));
	}

	float fx,fy;
	if (CommanderMinimap::World_To_Panel (CommanderModeController::Get_Focus (),_Map,&fx,&fy)) {
		_Panel->Add_Quad (RectClass (fx - 6.0f,fy - 0.75f,fx + 6.0f,fy + 0.75f),COLOR_FOCUS);
		_Panel->Add_Quad (RectClass (fx - 0.75f,fy - 6.0f,fx + 0.75f,fy + 6.0f),COLOR_FOCUS);
	}

	_Open = true;
}


void CommanderMinimapRenderer::Render(void)
{
	if (!_Available || !_Open || (_Panel == nullptr)) return;
	_Panel->Render ();
}


bool CommanderMinimapRenderer::Hit_Test(float x,float y,Vector3 * out_world,bool * out_inside_panel)
{
	if (out_inside_panel != nullptr) *out_inside_panel = false;
	if (!_Available || !_Open) return false;

	if ((x < _Frame.Left) || (x > _Frame.Right) || (y < _Frame.Top) || (y > _Frame.Bottom)) return false;
	if (out_inside_panel != nullptr) *out_inside_panel = true;

	if ((x < _Map.Left) || (x > _Map.Right) || (y < _Map.Top) || (y > _Map.Bottom)) return false;

	Vector3 world;
	if (!CommanderMinimap::Panel_To_World (x,y,_Map,&world)) return false;
	if (out_world != nullptr) *out_world = world;
	return true;
}


bool CommanderMinimapRenderer::Is_Open(void)	{ return _Open; }


class CommanderMinimapOverlayClass : public IRTSOverlayComponent
{
public:
	virtual void	Render_RTS_Overlay(RenderInfoClass & /* rinfo */) override	{ CommanderMinimapRenderer::Render (); }
	virtual bool	Is_RTS_Overlay_Visible(void) const override				{ return CommanderMinimapRenderer::Is_Open (); }
};

static CommanderMinimapOverlayClass	_OverlayComponent;

IRTSOverlayComponent & CommanderMinimapRenderer::Get_Overlay_Component(void)	{ return _OverlayComponent; }
