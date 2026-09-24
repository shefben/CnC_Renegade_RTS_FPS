/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/commanderghostrenderer.cpp            $*
 *                                                                                             *
 *	Roadmap Section 43, Phase 37 -- the drawing half of Commander building ghost placement.     *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "commanderghostrenderer.h"
#include "shadermgr.h"

#include "assetmgr.h"
#include "buildingdescriptormanager.h"
#include "combat.h"
#include "commanderplacement.h"
#include "matinfo.h"
#include "mesh.h"
#include "meshmdl.h"
#include "phys.h"
#include "pscene.h"
#include "rendobj.h"
#include "shader.h"
#include "vertmaterial.h"
#include "wwdebug.h"


static RenderObjClass *	_Ghost				= nullptr;
static StringClass			_GhostModelName	("");
static bool						_GhostInScene		= false;
static int						_LastTintValid		= -1;			// -1 = never tinted
static int						_MissingModelCount	= 0;
static bool						_Available			= false;

//	Section 43 asks for "translucent"; it does not give a number.  Low enough that the ground and
//	anything already standing there reads through the ghost -- which is the whole point of showing
//	an overlap -- and high enough that the building's own shape is legible.
static const float			GHOST_OPACITY			= 0.45f;

//	"Valid placement uses normal/team/neutral translucent tint."  A neutral pale tint rather than a
//	team colour: the ghost is about whether the ground is free, and a Nod-red valid ghost would be
//	one glance away from the invalid state.
static const Vector3		GHOST_TINT_VALID		(0.35f,0.55f,0.75f);
static const Vector3		GHOST_TINT_INVALID	(0.90f,0.10f,0.10f);


/***********************************************************************************************
 * Apply_Ghost_Materials -- make this model's meshes unique, then make them translucent          *
 *                                                                                             *
 *	Recursive over sub-objects, because a building model is a hierarchy and the meshes are its   *
 *	leaves.  Make_Unique first, every time: a model instantiated from the asset manager shares    *
 *	its mesh model -- and therefore its materials and shaders -- with the prototype, so writing   *
 *	a shader into it without unsharing would translucent-red every instance of that building in   *
 *	the level, including real finished ones.                                                      *
 *=============================================================================================*/
static void	Apply_Ghost_Materials(RenderObjClass * object,const Vector3 & tint)
{
	if (object == nullptr) return;

	if (object->Class_ID () == RenderObjClass::CLASSID_MESH) {

		MeshClass * mesh = static_cast<MeshClass *> (object);
		mesh->Make_Unique ();

		MeshModelClass * model = mesh->Peek_Model ();
		if (model != nullptr) {

			/*
			**	One alpha-blended pass carrying one emissive material.  Emissive rather than
			**	diffuse so the ghost reads the same in a shadowed corner as in open sun -- a
			**	placement preview that goes black indoors is a preview that cannot be judged.
			*/
			VertexMaterialClass * material = NEW_REF (VertexMaterialClass,());
			material->Set_Ambient (0.0f,0.0f,0.0f);
			material->Set_Diffuse (0.0f,0.0f,0.0f);
			material->Set_Emissive (tint.X,tint.Y,tint.Z);
			material->Set_Opacity (GHOST_OPACITY);

			//	The pipeline layer's ghost-tint program owns the blend state (D-001 P09); the
			//	tint and opacity above change with the verdict and stay this renderer's.
			ShaderClass shader = ShaderClass::_PresetAlphaShader;
			if (!ShaderManagerClass::Get_Pass_State (MATERIAL_PROGRAM_GHOST_TINT,0,&shader,nullptr)) {
				shader.Set_Depth_Mask (ShaderClass::DEPTH_WRITE_DISABLE);
			}

			model->Set_Pass_Count (1);
			model->Set_Single_Shader (shader,0);
			model->Set_Single_Material (material,0);

			REF_PTR_RELEASE (material);
		}

		//	Translucent geometry has to draw after the world or it blends against nothing.
		mesh->Set_Sort_Level (SORT_LEVEL_BIN3);
	}

	for (int i = 0; i < object->Get_Num_Sub_Objects (); i++) {
		RenderObjClass * child = object->Get_Sub_Object (i);
		if (child != nullptr) {
			Apply_Ghost_Materials (child,tint);
			child->Release_Ref ();
		}
	}
}


void CommanderGhostRenderer::Init(bool render_available)
{
	_Available				= render_available;
	_Ghost					= nullptr;
	_GhostModelName		= "";
	_GhostInScene			= false;
	_LastTintValid			= -1;
	_MissingModelCount	= 0;
}

void CommanderGhostRenderer::Shutdown(void)
{
	Release ();
	_MissingModelCount	= 0;
	_Available				= false;
}


void CommanderGhostRenderer::Release(void)
{
	if (_Ghost == nullptr) {
		_GhostModelName	= "";
		_GhostInScene		= false;
		_LastTintValid		= -1;
		return;
	}

	if (_GhostInScene && (COMBAT_SCENE != nullptr)) {
		COMBAT_SCENE->Remove_Render_Object (_Ghost);
	}

	REF_PTR_RELEASE (_Ghost);
	_GhostModelName	= "";
	_GhostInScene		= false;
	_LastTintValid		= -1;
}


bool CommanderGhostRenderer::Is_Model_Loaded(void)			{ return _Ghost != nullptr; }
int  CommanderGhostRenderer::Get_Missing_Model_Count(void)	{ return _MissingModelCount; }


void CommanderGhostRenderer::Update(void)
{
	if (!_Available) return;

	//	No ghost, or nowhere to draw it: make sure nothing is left on screen and stop.
	if (!CommanderPlacementValidator::Is_Ghost_Active () || (COMBAT_SCENE == nullptr)) {
		Release ();
		return;
	}

	const CommanderPlacementRequest & request = CommanderPlacementValidator::Get_Ghost ();

	/*
	**	Which model this ghost wants.  A descriptor that names no model, or names one the asset
	**	manager does not have, is the project's standard absent-asset case: the ghost simply does
	**	not draw, the placement rules keep working exactly as before, and the miss is counted.
	**	Never a crash and never a stand-in model.
	*/
	StringClass wanted ("");
	const BuildingModelDescriptor * descriptor = nullptr;
	if (request.DescriptorName.Get_Length () > 0) {
		descriptor = BuildingDescriptorManager::Get_Descriptor (request.DescriptorName.Peek_Buffer ());
	}
	if (descriptor != nullptr) {
		wanted = descriptor->ModelName;
	}

	if (wanted.Get_Length () == 0) {
		Release ();
		return;
	}

	if (_GhostModelName.Compare_No_Case (wanted) != 0) {

		Release ();

		RenderObjClass * model = WW3DAssetManager::Get_Instance ()->Create_Render_Obj (wanted.Peek_Buffer ());
		if (model == nullptr) {
			//	Counted once per ghost rather than once per frame, because Release cleared the
			//	remembered name and the next frame will try again -- so remember the failed name
			//	too, and let the miss be a single event a person can see in debug output.
			_GhostModelName = wanted;
			_MissingModelCount++;
			WWDEBUG_SAY (("CommanderGhostRenderer: no model '%s' for descriptor '%s'; "
							  "the ghost will not draw, placement is unaffected\n",
							  wanted.Peek_Buffer (),request.DescriptorName.Peek_Buffer ()));
			return;
		}

		_Ghost			= model;
		_GhostModelName	= wanted;
		_LastTintValid	= -1;

		COMBAT_SCENE->Add_Render_Object (_Ghost);
		_GhostInScene = true;
	}

	if (_Ghost == nullptr) {
		return;			// a model we already know is missing
	}

	//	Where it sits.  Exactly the transform the real building would get if this placement were
	//	accepted, so what a player lines up is where the building lands.
	Matrix3D transform (1);
	if (CommanderPlacementValidator::Build_Placement_Transform (request,
					CommanderPlacementValidator::Get_Ghost_Result (),&transform)) {
		_Ghost->Set_Transform (transform);
	}

	/*
	**	The tint, re-applied only when validity actually changes.  Applying it every frame would
	**	allocate a VertexMaterialClass per mesh per frame, and the tint is a two-state thing --
	**	it changes when the cursor crosses into or out of a legal spot, not continuously.
	*/
	CommanderGhostRenderer::Get_Interaction_Component ().Set_Interaction_Highlight (
		CommanderPlacementValidator::Is_Ghost_Valid ());
}


/*
**	D-001 P26: the ghost's valid/invalid tint is Section 29's world-interaction visual -- a binary
**	highlight on world-space feedback that is not an object's own body.
*/
class GhostInteractionComponentClass : public IWorldInteractionVisualComponent
{
public:
	virtual void	Set_Interaction_Highlight(bool valid) override
	{
		int valid_now = valid ? 1 : 0;
		if ((_Ghost == nullptr) || (valid_now == _LastTintValid)) return;
		Apply_Ghost_Materials (_Ghost,valid_now ? GHOST_TINT_VALID : GHOST_TINT_INVALID);
		_LastTintValid = valid_now;
	}
};

static GhostInteractionComponentClass	_GhostInteraction;

IWorldInteractionVisualComponent & CommanderGhostRenderer::Get_Interaction_Component(void)
{
	return _GhostInteraction;
}
