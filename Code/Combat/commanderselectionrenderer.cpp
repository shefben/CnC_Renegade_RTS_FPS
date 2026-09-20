/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderselectionrenderer.cpp        $*
 *                                                                                             *
 *	Roadmap Section 45, Phase 39.  See commanderselectionrenderer.h.                            *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commanderselectionrenderer.h"

#include "camera.h"
#include "ccamera.h"
#include "combat.h"
#include "commandermodecontroller.h"
#include "commanderselection.h"
#include "gameobjmanager.h"
#include "phys.h"
#include "physicalgameobj.h"
#include "render2d.h"
#include "smartgameobj.h"
#include "stylemgr.h"
#include "wwmath.h"


static bool						_Available		= false;
static Render2DClass *		_Renderer		= nullptr;
static int						_BracketCount	= 0;


//	The same amber the sidebar reads in, so the Commander layer is one colour scheme.
static const unsigned int	COLOR_BOX			= RGBA_TO_INT32 (252,204, 92,210);
static const unsigned int	COLOR_BOX_FILL		= RGBA_TO_INT32 (252,204, 92, 28);
static const unsigned int	COLOR_BRACKET		= RGBA_TO_INT32 ( 96,236,120,235);

static const float			BOX_THICKNESS		= 1.0f;
static const float			BRACKET_THICKNESS	= 2.0f;

//	A bracket never shrinks below this or grows past it, whatever the unit's bounds project to.
//	Without the floor a distant unit's bracket collapses into a dot; without the ceiling a
//	vehicle filling the screen draws a bracket bigger than the viewport.
static const float			BRACKET_MIN_HALF	= 6.0f;
static const float			BRACKET_MAX_HALF	= 120.0f;


static void	Add_Outline(Render2DClass * renderer,const RectClass & rect,unsigned int colour,
								float thickness)
{
	renderer->Add_Quad (RectClass (rect.Left,rect.Top,rect.Right,rect.Top + thickness),colour);
	renderer->Add_Quad (RectClass (rect.Left,rect.Bottom - thickness,rect.Right,rect.Bottom),colour);
	renderer->Add_Quad (RectClass (rect.Left,rect.Top,rect.Left + thickness,rect.Bottom),colour);
	renderer->Add_Quad (RectClass (rect.Right - thickness,rect.Top,rect.Right,rect.Bottom),colour);
}


/***********************************************************************************************
 * Add_Bracket -- four corner pieces, not a full rectangle                                       *
 *                                                                                               *
 *	Corners rather than a closed box on purpose: a closed box around every selected unit reads    *
 *	as a second rubber band and competes with the drag box being drawn at the same time.  Corner  *
 *	brackets are what every RTS this panel is modelled on uses, for the same reason.              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void	Add_Bracket(Render2DClass * renderer,const RectClass & rect,unsigned int colour)
{
	float arm = (rect.Right - rect.Left) * 0.3f;
	if (arm < 3.0f) arm = 3.0f;

	const float t = BRACKET_THICKNESS;

	//	Top left / top right / bottom left / bottom right, each an L.
	renderer->Add_Quad (RectClass (rect.Left,rect.Top,rect.Left + arm,rect.Top + t),colour);
	renderer->Add_Quad (RectClass (rect.Left,rect.Top,rect.Left + t,rect.Top + arm),colour);

	renderer->Add_Quad (RectClass (rect.Right - arm,rect.Top,rect.Right,rect.Top + t),colour);
	renderer->Add_Quad (RectClass (rect.Right - t,rect.Top,rect.Right,rect.Top + arm),colour);

	renderer->Add_Quad (RectClass (rect.Left,rect.Bottom - t,rect.Left + arm,rect.Bottom),colour);
	renderer->Add_Quad (RectClass (rect.Left,rect.Bottom - arm,rect.Left + t,rect.Bottom),colour);

	renderer->Add_Quad (RectClass (rect.Right - arm,rect.Bottom - t,rect.Right,rect.Bottom),colour);
	renderer->Add_Quad (RectClass (rect.Right - t,rect.Bottom - arm,rect.Right,rect.Bottom),colour);
}


void CommanderSelectionRenderer::Init(bool render_available)
{
	Shutdown ();

	_Available		= render_available;
	_BracketCount	= 0;
	if (!_Available) return;

	_Renderer = new Render2DClass ();
	_Renderer->Enable_Texturing (false);
	StyleMgrClass::Configure_Renderer (_Renderer);
}


void CommanderSelectionRenderer::Shutdown(void)
{
	if (_Renderer != nullptr) {
		delete _Renderer;
		_Renderer = nullptr;
	}
	_Available		= false;
	_BracketCount	= 0;
}


void CommanderSelectionRenderer::Update(void)
{
	if (!_Available || (_Renderer == nullptr)) return;

	_Renderer->Reset ();
	_BracketCount = 0;

	//	Nothing is drawn outside Commander Mode, so an FPS frame costs one Reset and a return.
	if (!CommanderModeController::Is_Active ()) return;

	const RectClass & screen = Render2DClass::Get_Screen_Resolution ();

	/*
	**	The rubber band.  A faint fill as well as the outline, because an outline alone over busy
	**	terrain is genuinely hard to see, and the fill is what makes the box read as an area
	**	rather than four lines.
	*/
	if (CommanderSelection::Is_Dragging () && CommanderSelection::Is_Drag_Significant ()) {

		Vector2 minimum (0.0f,0.0f);
		Vector2 maximum (0.0f,0.0f);
		CommanderSelection::Get_Drag_Rect (&minimum,&maximum);

		RectClass box (minimum.X,minimum.Y,maximum.X,maximum.Y);
		_Renderer->Add_Quad (box,COLOR_BOX_FILL);
		Add_Outline (_Renderer,box,COLOR_BOX,BOX_THICKNESS);
	}

	/*
	**	One bracket per selected unit, sized from the unit's own cull box rather than a fixed
	**	number of pixels, so a Mammoth Tank's bracket is bigger than a soldier's and both stay
	**	right as the camera zooms.  A unit off screen or behind the camera simply gets no bracket;
	**	it stays selected, because being out of view is not a reason to lose a unit from an order.
	*/
	CameraClass * camera = COMBAT_CAMERA;
	if (camera == nullptr) return;

	int count = CommanderSelection::Get_Count ();
	for (int i = 0; i < count; i++) {

		PhysicalGameObj * unit = GameObjManager::Find_PhysicalGameObj (CommanderSelection::Get_Unit_Id (i));
		if (unit == nullptr) continue;

		Vector3 position (0.0f,0.0f,0.0f);
		unit->Get_Position (&position);

		Vector3 projected (0.0f,0.0f,0.0f);
		if (camera->Project (projected,position) != CameraClass::INSIDE_FRUSTUM) continue;

		//	Projection is normalised -1..1 with Y up; the screen rect is pixels with Y down.
		float centre_x = screen.Left + (projected.X + 1.0f) * 0.5f * screen.Width ();
		float centre_y = screen.Top  + (1.0f - projected.Y) * 0.5f * screen.Height ();

		/*
		**	Half-extent in pixels, measured rather than derived: project a second world point one
		**	unit-radius to the camera's right and take the pixel distance between the two.  That
		**	uses only `Project`, whose -1..1 output every other caller in this tree already
		**	relies on, instead of `Compute_Projected_Sphere_Radius`, which has no caller anywhere
		**	and therefore no established sign convention for its distance argument.
		*/
		float half = BRACKET_MIN_HALF;
		if (unit->Peek_Physical_Object () != nullptr) {

			const AABoxClass & bounds = unit->Peek_Physical_Object ()->Get_Cull_Box ();
			float radius = bounds.Extent.Length ();

			if (radius > 0.01f) {

				Vector3 camera_right = camera->Get_Transform ().Get_X_Vector ();
				Vector3 edge = position + camera_right * radius;

				Vector3 projected_edge (0.0f,0.0f,0.0f);
				if (camera->Project (projected_edge,edge) == CameraClass::INSIDE_FRUSTUM) {
					float edge_x = screen.Left + (projected_edge.X + 1.0f) * 0.5f * screen.Width ();
					float edge_y = screen.Top  + (1.0f - projected_edge.Y) * 0.5f * screen.Height ();
					half = WWMath::Sqrt (((edge_x - centre_x) * (edge_x - centre_x)) +
												((edge_y - centre_y) * (edge_y - centre_y)));
				}
			}
		}

		half = WWMath::Clamp (half,BRACKET_MIN_HALF,BRACKET_MAX_HALF);

		RectClass bracket (centre_x - half,centre_y - half,centre_x + half,centre_y + half);
		Add_Bracket (_Renderer,bracket,COLOR_BRACKET);
		_BracketCount++;
	}
}


void CommanderSelectionRenderer::Render(void)
{
	if (!_Available || (_Renderer == nullptr)) return;
	if (!CommanderModeController::Is_Active ()) return;

	_Renderer->Render ();
}


int CommanderSelectionRenderer::Get_Bracket_Count(void)	{ return _BracketCount; }
