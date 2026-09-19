/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/constructionrenderer.cpp              $*
 *                                                                                             *
 *	Roadmap Section 44, Phase 38 -- the drawing half of procedural construction build-up.       *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "constructionrenderer.h"

#include "buildingconstruction.h"
#include "combat.h"
#include "dynamesh.h"
#include "phys.h"
#include "pscene.h"
#include "shader.h"
#include "vector.h"
#include "vector3.h"
#include "vertmaterial.h"


//	A box is six quads; each quad is a triangle fan of four vertices, so two triangles.
enum { VERTS_PER_BLOCK = 24, POLYS_PER_BLOCK = 12 };


struct ConstructionBatchRecord
{
	ConstructionBatchRecord(void) : InUse(false),BuildingId(0),Settled(nullptr),Active(nullptr),
											  SettledRow(-2) { }

	bool	operator == (const ConstructionBatchRecord &) const	{ return false; }
	bool	operator != (const ConstructionBatchRecord &) const	{ return true; }

	bool						InUse;
	int						BuildingId;
	DynamicMeshClass *	Settled;
	DynamicMeshClass *	Active;

	//	The active row the settled batch was last built against.  -2 means "never built", which
	//	is distinct from -1, the service's own "no row is assembling yet".
	int						SettledRow;
};

static DynamicVectorClass<ConstructionBatchRecord>	_Records;

static int	_RenderedBlocks	= 0;
static int	_DroppedBlocks		= 0;
static bool	_Available			= false;


/***********************************************************************************************
 * Make_Block_Material -- one unlit, vertex-coloured, vertex-alpha material                      *
 *                                                                                             *
 *	Emissive and diffuse both come from the per-vertex colour, and everything else is black, so  *
 *	a block is exactly the colour it was given regardless of where it stands -- a construction    *
 *	site inside a shadow must still read as a construction site.  The per-vertex alpha is what    *
 *	`_PresetAlphaShader` blends with, which is how each block fades in on its own schedule        *
 *	without needing a material of its own.                                                       *
 *=============================================================================================*/
static VertexMaterialClass *	Make_Block_Material(void)
{
	VertexMaterialClass * material = NEW_REF (VertexMaterialClass,());
	material->Set_Ambient (0.0f,0.0f,0.0f);
	material->Set_Diffuse (0.0f,0.0f,0.0f);
	material->Set_Specular (0.0f,0.0f,0.0f);
	material->Set_Emissive (1.0f,1.0f,1.0f);
	material->Set_Opacity (1.0f);
	material->Set_Emissive_Color_Source (VertexMaterialClass::COLOR1);
	material->Set_Diffuse_Color_Source (VertexMaterialClass::COLOR1);
	material->Set_Lighting (false);
	return material;
}


static DynamicMeshClass *	Make_Batch(void)
{
	DynamicMeshClass * mesh = NEW_REF (DynamicMeshClass,
		(BuildingConstructionRenderer::MAX_BLOCKS_PER_BATCH * POLYS_PER_BLOCK,
		 BuildingConstructionRenderer::MAX_BLOCKS_PER_BATCH * VERTS_PER_BLOCK));

	VertexMaterialClass * material = Make_Block_Material ();
	mesh->Set_Vertex_Material (material,true);
	REF_PTR_RELEASE (material);

	mesh->Set_Shader (ShaderClass::_PresetAlphaShader);

	//	Translucent geometry draws after the opaque world, or it blends against whatever happens
	//	to have been drawn first.
	mesh->Set_Sort_Level (SORT_LEVEL_BIN3);
	return mesh;
}


/***********************************************************************************************
 * Emit_Block -- one axis-aligned box in the block's own oriented frame                          *
 *                                                                                             *
 *	The transform the service hands back already carries the building's rotation, so the box is   *
 *	built from the transform's own axes rather than from world X/Y/Z -- which is what makes       *
 *	"rotated buildings must generate rotated visuals" true here as well as in the layout.         *
 *=============================================================================================*/
static void	Emit_Block(DynamicMeshClass * mesh,const BuildingConstructionBlockRenderState & state,
							  const Vector3 & tint)
{
	Vector3 center	= state.Transform.Get_Translation ();
	Vector3 x		= state.Transform.Get_X_Vector () * state.HalfExtent.X;
	Vector3 y		= state.Transform.Get_Y_Vector () * state.HalfExtent.Y;
	Vector3 z		= state.Transform.Get_Z_Vector () * state.HalfExtent.Z;

	Vector3 corner [8];
	corner [0] = center - x - y - z;
	corner [1] = center + x - y - z;
	corner [2] = center + x + y - z;
	corner [3] = center - x + y - z;
	corner [4] = center - x - y + z;
	corner [5] = center + x - y + z;
	corner [6] = center + x + y + z;
	corner [7] = center - x + y + z;

	//	Six faces, counter-clockwise seen from outside.
	static const int FACE [6][4] = {
		{ 0,3,2,1 },		// bottom (-Z)
		{ 4,5,6,7 },		// top    (+Z)
		{ 0,1,5,4 },		// -Y
		{ 2,3,7,6 },		// +Y
		{ 1,2,6,5 },		// +X
		{ 3,0,4,7 },		// -X
	};

	//	A flat shade difference per face, so a block reads as a solid object rather than as a
	//	silhouette.  Cheap, and it needs no light: the material is unlit by design.
	static const float FACE_SHADE [6] = { 0.55f,1.00f,0.75f,0.75f,0.85f,0.85f };

	for (int f = 0; f < 6; f++) {

		float shade = FACE_SHADE [f];

		mesh->Begin_Tri_Fan ();
		for (int v = 0; v < 4; v++) {
			const Vector3 & position = corner [FACE [f][v]];
			mesh->Begin_Vertex ();
			mesh->Location (position.X,position.Y,position.Z);
			mesh->UV (0.0f,0.0f);
			mesh->Color (tint.X * shade,tint.Y * shade,tint.Z * shade,state.Alpha);
			mesh->End_Vertex ();
		}
	}
}


static ConstructionBatchRecord *	Find_Or_Make_Record(int building_id)
{
	for (int i = 0; i < _Records.Count (); i++) {
		if (_Records [i].InUse && _Records [i].BuildingId == building_id) return &_Records [i];
	}

	int slot = -1;
	for (int i = 0; i < _Records.Count (); i++) {
		if (!_Records [i].InUse) { slot = i; break; }
	}
	if (slot < 0) {
		_Records.Add (ConstructionBatchRecord ());
		slot = _Records.Count () - 1;
	}

	ConstructionBatchRecord & record = _Records [slot];
	record.InUse		= true;
	record.BuildingId	= building_id;
	record.SettledRow	= -2;
	record.Settled		= Make_Batch ();
	record.Active		= Make_Batch ();

	if (COMBAT_SCENE != nullptr) {
		COMBAT_SCENE->Add_Render_Object (record.Settled);
		COMBAT_SCENE->Add_Render_Object (record.Active);
	}
	return &record;
}


static void	Retire_Record(ConstructionBatchRecord & record)
{
	if (!record.InUse) return;

	if (COMBAT_SCENE != nullptr) {
		if (record.Settled != nullptr) COMBAT_SCENE->Remove_Render_Object (record.Settled);
		if (record.Active != nullptr)  COMBAT_SCENE->Remove_Render_Object (record.Active);
	}
	REF_PTR_RELEASE (record.Settled);
	REF_PTR_RELEASE (record.Active);

	record.InUse		= false;
	record.BuildingId	= 0;
	record.SettledRow	= -2;
}


void BuildingConstructionRenderer::Init(bool render_available)
{
	_Available = render_available;
	for (int i = 0; i < _Records.Count (); i++) Retire_Record (_Records [i]);
	_Records.Delete_All ();
	_RenderedBlocks	= 0;
	_DroppedBlocks		= 0;
}

void BuildingConstructionRenderer::Shutdown(void)
{
	Init (false);
}


void BuildingConstructionRenderer::Update(void)
{
	if (!_Available || COMBAT_SCENE == nullptr) return;

	_RenderedBlocks	= 0;
	_DroppedBlocks		= 0;

	//	Retire the batches of anything that finished or was cancelled since the last frame.  Done
	//	first, so a building that completed this frame stops drawing blocks immediately rather
	//	than leaving a shell standing over the finished model for a frame.
	for (int i = 0; i < _Records.Count (); i++) {
		if (!_Records [i].InUse) continue;
		if (!BuildingConstructionService::Is_Under_Construction (_Records [i].BuildingId)) {
			Retire_Record (_Records [i]);
		}
	}

	int active_count = BuildingConstructionService::Get_Active_Count ();
	for (int index = 0; index < active_count; index++) {

		int building_id = BuildingConstructionService::Get_Active_Building (index);
		if (building_id == 0) continue;

		ConstructionBatchRecord * record = Find_Or_Make_Record (building_id);
		if (record == nullptr || record->Settled == nullptr || record->Active == nullptr) continue;

		int active_row	= BuildingConstructionService::Get_Active_Row (building_id);
		int block_count	= BuildingConstructionService::Get_Block_Count (building_id);

		float progress = 0.0f;
		BuildingConstructionService::Get_Progress (building_id,&progress);

		/*
		**	During the final fade every block's alpha changes every frame, so the settled batch
		**	stops being static and has to be rebuilt with the active one.  Outside the fade it is
		**	rebuilt only when the active row advances, which is the whole point of splitting the
		**	two.
		*/
		bool fading			= (progress >= 0.85f);
		bool rebuild_settled	= fading || (record->SettledRow != active_row);

		if (rebuild_settled) {
			record->Settled->Reset_Mesh_Counters ();
			record->Settled->Reset_Native_Mesh ();
			record->Settled->Reset_Flags ();
		}
		record->Active->Reset_Mesh_Counters ();
		record->Active->Reset_Native_Mesh ();
		record->Active->Reset_Flags ();

		int settled_emitted	= 0;
		int active_emitted	= 0;

		//	A neutral construction grey-blue.  Not a team colour: a half-built building is not yet
		//	anybody's asset, and Section 44 asks for a procedural visual, not a faction banner.
		static const Vector3 BLOCK_TINT (0.55f,0.62f,0.70f);

		for (int b = 0; b < block_count; b++) {

			BuildingConstructionBlock block;
			if (!BuildingConstructionService::Get_Block (building_id,b,&block)) continue;

			BuildingConstructionBlockRenderState state;
			if (!BuildingConstructionService::Get_Block_Render_State (building_id,b,&state)) continue;
			if (!state.Visible) continue;

			bool is_active_row = (block.Row >= active_row);

			if (is_active_row) {
				if (active_emitted >= MAX_BLOCKS_PER_BATCH) { _DroppedBlocks++; continue; }
				Emit_Block (record->Active,state,BLOCK_TINT);
				active_emitted++;
			} else {
				if (!rebuild_settled) continue;			// still valid from a previous frame
				if (settled_emitted >= MAX_BLOCKS_PER_BATCH) { _DroppedBlocks++; continue; }
				Emit_Block (record->Settled,state,BLOCK_TINT);
				settled_emitted++;
			}
		}

		if (rebuild_settled) {
			record->SettledRow = active_row;
		}

		_RenderedBlocks += active_emitted + settled_emitted;
	}
}


int BuildingConstructionRenderer::Get_Batch_Count(void)
{
	int count = 0;
	for (int i = 0; i < _Records.Count (); i++) {
		if (_Records [i].InUse) count += 2;
	}
	return count;
}

int BuildingConstructionRenderer::Get_Rendered_Block_Count(void)	{ return _RenderedBlocks; }
int BuildingConstructionRenderer::Get_Dropped_Block_Count(void)	{ return _DroppedBlocks; }
