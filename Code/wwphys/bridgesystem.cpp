/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/WWPhys/bridgesystem.cpp                      $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Functions:                                                                                  *
 *   BridgeSystem::Init -- the definitions a map can use before a map has said anything         *
 *   BridgeSystem::Define_Default_Bridges -- one bridge per road class                          *
 *   BridgeSystem::Build_Layout -- how many spans a gap of that size takes                      *
 *   BridgeSystem::Layout_Bridge -- caps, spans, piers and decoration along one line            *
 *   BridgeSystem::Build_Geometry -- a physics object per section that collides                 *
 *   BridgeSystem::Build_Section_Geometry -- the model, or a slab of the declared size           *
 *   BridgeSystem::Set_Section_State -- damage, one piece at a time                             *
 *   BridgeSystem::Is_Traversable -- whether the far side can still be reached                  *
 *   BridgeSystem::Conform_Point -- where the deck is over a given place on the map             *
 *   BridgeSystem::Connect_Roads -- bind road ends that said they hand over to a bridge         *
 *   BridgeSystem::Get_State_Block -- what the server sends instead of geometry                 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "bridgesystem.h"

#include "assetmgr.h"
#include "coltype.h"
#include "dynamicmeshbuilder.h"
#include "matrix3d.h"
#include "pscene.h"
#include "rendobj.h"
#include "roadspline.h"
#include "roadsystem.h"
#include "shader.h"
#include "staticphys.h"
#include "texture.h"
#include "vertmaterial.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"


/*
**	A deck is a floor, so it collides with everything a floor collides with.  A pier is a thing
**	in the water that a boat or a shell can hit, and it uses the same set: there is no case
**	where you want to walk through the middle of a bridge support.
*/
static const int BRIDGE_COLLISION_BITS =
	COLLISION_TYPE_PHYSICAL | COLLISION_TYPE_PROJECTILE |
	COLLISION_TYPE_VEHICLE | COLLISION_TYPE_CAMERA;

/*
**	How much of a bridge the two abutments are allowed to eat.  A short bridge is mostly
**	abutment, but a bridge that is nothing but abutment has no span to break, and a bridge with
**	no breakable span is a wall.
*/
static const float BRIDGE_MAX_CAP_SHARE = 0.8f;

/*
**	A pier with no ground under it -- because there is no terrain loaded to ask -- still gets
**	built, at this height, because a bridge stands on something whether or not the check knows
**	what.  Where terrain does answer, the real drop is used.
*/
static const float BRIDGE_NOMINAL_PIER_DROP = 6.0f;


DynamicVectorClass<BridgeDefinitionClass>	BridgeSystem::Definitions;
DynamicVectorClass<BridgeClass>				BridgeSystem::Bridges;
DynamicVectorClass<BridgeDebrisClass>		BridgeSystem::Debris;

bool	BridgeSystem::LayoutBuilt = false;
bool	BridgeSystem::GeometryBuilt = false;
bool	BridgeSystem::StateDirty = false;
int	BridgeSystem::MissingModels = 0;

static bool	_ReportedNoScene = false;


/***********************************************************************************************
 * BridgeClass                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
BridgeClass::BridgeClass(void) :
	ID(-1),
	Start(0.0f,0.0f,0.0f),
	End(0.0f,0.0f,0.0f),
	Width(0.0f),
	StartRoadID(-1),
	EndRoadID(-1)
{
}


BridgeClass::BridgeClass(const BridgeClass & that) :
	ID(-1),
	Start(0.0f,0.0f,0.0f),
	End(0.0f,0.0f,0.0f),
	Width(0.0f),
	StartRoadID(-1),
	EndRoadID(-1)
{
	*this = that;
}


BridgeClass::~BridgeClass(void)
{
}


BridgeClass & BridgeClass::operator = (const BridgeClass & that)
{
	if (this == &that) {
		return *this;
	}

	ID				= that.ID;
	Name			= that.Name;
	Definition	= that.Definition;
	Start			= that.Start;
	End			= that.End;
	Width			= that.Width;
	StartRoadID	= that.StartRoadID;
	EndRoadID	= that.EndRoadID;

	Sections.Delete_All();
	for (int i = 0; i < that.Sections.Count(); i++) {
		Sections.Add(that.Sections[i]);
	}

	return *this;
}


bool BridgeClass::operator == (const BridgeClass & that) const
{
	if ((ID != that.ID) || (Name != that.Name) || (Definition != that.Definition) ||
		 (Start != that.Start) || (End != that.End) || (Width != that.Width) ||
		 (StartRoadID != that.StartRoadID) || (EndRoadID != that.EndRoadID)) {
		return false;
	}
	if (Sections.Count() != that.Sections.Count()) {
		return false;
	}
	for (int i = 0; i < Sections.Count(); i++) {
		if (Sections[i] != that.Sections[i]) { return false; }
	}
	return true;
}


void BridgeClass::Reset(void)
{
	ID = -1;
	Name = "";
	Definition = "";
	Start.Set(0.0f,0.0f,0.0f);
	End.Set(0.0f,0.0f,0.0f);
	Width = 0.0f;
	StartRoadID = -1;
	EndRoadID = -1;
	Sections.Delete_All();
}


void BridgeClass::Set_Name(const char * name)
{
	Name = (name != nullptr) ? name : "";
}


void BridgeClass::Set_Definition(const char * name)
{
	Definition = (name != nullptr) ? name : "";
}


void BridgeClass::Set_Endpoints(const Vector3 & start,const Vector3 & end)
{
	Start = start;
	End = end;
	Sections.Delete_All();
}


float BridgeClass::Get_Length(void) const
{
	Vector3 delta = End - Start;
	return delta.Length();
}


Vector3 BridgeClass::Get_Direction(void) const
{
	Vector3 delta = End - Start;
	float length = delta.Length();
	if (length < WWMATH_EPSILON) {
		return Vector3(1.0f,0.0f,0.0f);
	}
	return delta / length;
}


const BridgeSectionClass & BridgeClass::Get_Section(int index) const
{
	static BridgeSectionClass _empty;
	if ((index < 0) || (index >= Sections.Count())) {
		return _empty;
	}
	return Sections[index];
}


BridgeSectionClass * BridgeClass::Peek_Section(int index)
{
	if ((index < 0) || (index >= Sections.Count())) {
		return nullptr;
	}
	return &Sections[index];
}


/*
**	The worst state anything load bearing is in.  A bridge with one broken span is a broken
**	bridge, whatever the rest of it looks like, because the thing anybody wants to know is
**	whether they can cross.
*/
int BridgeClass::Get_Damage_State(void) const
{
	int worst = BRIDGE_STATE_PRISTINE;
	for (int i = 0; i < Sections.Count(); i++) {
		const BridgeSectionClass & section = Sections[i];
		if ((section.Kind != BRIDGE_SECTION_START_CAP) &&
			 (section.Kind != BRIDGE_SECTION_SPAN) &&
			 (section.Kind != BRIDGE_SECTION_END_CAP)) {
			continue;
		}
		if (section.State > worst) { worst = section.State; }
	}
	return worst;
}


/***********************************************************************************************
 * BridgeSystem::Init -- the definitions a map can use before a map has said anything           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void BridgeSystem::Init(void)
{
	Shutdown();
	Define_Default_Bridges();
}


void BridgeSystem::Shutdown(void)
{
	Destroy_Geometry();
	Bridges.Delete_All();
	Definitions.Delete_All();
	Debris.Delete_All();
	LayoutBuilt = false;
	GeometryBuilt = false;
	StateDirty = false;
	MissingModels = 0;
	_ReportedNoScene = false;
}


/***********************************************************************************************
 * BridgeSystem::Define_Default_Bridges -- one bridge per road class                            *
 *                                                                                             *
 *	Section 19's three road classes are a track, a street and a highway, and a road that runs   *
 *	off a bridge is the same road on the other side of it, so the sizes here are the sizes      *
 *	those roads are drawn at.  None of them names a model: no bridge art exists, the names it   *
 *	will want are written down in docs/assets/BridgeSections.md, and until they are made every  *
 *	section here is built as a slab of the size declared below -- which collides, carries       *
 *	traffic and breaks, all of which is what Section 20 asks for.                               *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void BridgeSystem::Define_Default_Bridges(void)
{
	//	A timber trestle over a stream: narrow, short spans, piers close together, and it goes
	//	down if you look at it hard.
	BridgeDefinitionClass timber;
	timber.Set_Name("ow_bridge_timber");
	timber.Set_Width(4.0f);
	timber.Set_Deck_Thickness(0.35f);
	timber.Set_Section_Length(BRIDGE_SECTION_START_CAP,3.0f);
	timber.Set_Section_Length(BRIDGE_SECTION_SPAN,5.0f);
	timber.Set_Section_Length(BRIDGE_SECTION_END_CAP,3.0f);
	timber.Set_Section_Length(BRIDGE_SECTION_SUPPORT,1.0f);
	timber.Set_Support_Spacing(10.0f);
	timber.Set_Support_Clearance(1.5f);
	Define_Bridge(timber);

	//	A concrete road bridge: the ordinary case, and the one a street runs onto.
	BridgeDefinitionClass concrete;
	concrete.Set_Name("ow_bridge_concrete");
	concrete.Set_Width(8.0f);
	concrete.Set_Deck_Thickness(0.6f);
	concrete.Set_Section_Length(BRIDGE_SECTION_START_CAP,6.0f);
	concrete.Set_Section_Length(BRIDGE_SECTION_SPAN,8.0f);
	concrete.Set_Section_Length(BRIDGE_SECTION_END_CAP,6.0f);
	concrete.Set_Section_Length(BRIDGE_SECTION_SUPPORT,2.0f);
	concrete.Set_Support_Spacing(24.0f);
	concrete.Set_Support_Clearance(2.0f);
	Define_Bridge(concrete);

	//	A steel truss carrying a highway: wide, long spans, few piers.
	BridgeDefinitionClass steel;
	steel.Set_Name("ow_bridge_steel_truss");
	steel.Set_Width(16.0f);
	steel.Set_Deck_Thickness(0.8f);
	steel.Set_Section_Length(BRIDGE_SECTION_START_CAP,10.0f);
	steel.Set_Section_Length(BRIDGE_SECTION_SPAN,16.0f);
	steel.Set_Section_Length(BRIDGE_SECTION_END_CAP,10.0f);
	steel.Set_Section_Length(BRIDGE_SECTION_SUPPORT,3.0f);
	steel.Set_Support_Spacing(48.0f);
	steel.Set_Support_Clearance(3.0f);
	Define_Bridge(steel);
}


bool BridgeSystem::Define_Bridge(const BridgeDefinitionClass & definition)
{
	if ((definition.Get_Name() == nullptr) || (definition.Get_Name()[0] == 0)) {
		return false;
	}

	//	Defining a name that is already there replaces it, so a map may retune a bridge without
	//	ending up with two definitions of the same name and a rule about which wins.
	for (int i = 0; i < Definitions.Count(); i++) {
		if (::stricmp(Definitions[i].Get_Name(),definition.Get_Name()) == 0) {
			Definitions[i] = definition;
			return true;
		}
	}

	Definitions.Add(definition);
	return true;
}


int BridgeSystem::Get_Definition_Count(void)
{
	return Definitions.Count();
}


BridgeDefinitionClass * BridgeSystem::Peek_Definition(int index)
{
	if ((index < 0) || (index >= Definitions.Count())) {
		return nullptr;
	}
	return &Definitions[index];
}


BridgeDefinitionClass * BridgeSystem::Find_Definition(const char * name)
{
	if (name == nullptr) {
		return nullptr;
	}
	for (int i = 0; i < Definitions.Count(); i++) {
		if (::stricmp(Definitions[i].Get_Name(),name) == 0) {
			return &Definitions[i];
		}
	}
	return nullptr;
}


int BridgeSystem::Add_Bridge(const BridgeClass & bridge)
{
	int id = Bridges.Count();
	Bridges.Add(bridge);
	Bridges[id].Set_ID(id);
	LayoutBuilt = false;
	return id;
}


BridgeClass * BridgeSystem::Peek_Bridge(int id)
{
	if ((id < 0) || (id >= Bridges.Count())) {
		return nullptr;
	}
	return &Bridges[id];
}


int BridgeSystem::Get_Bridge_Count(void)
{
	return Bridges.Count();
}


void BridgeSystem::Clear_Bridges(void)
{
	Destroy_Geometry();
	Bridges.Delete_All();
	Debris.Delete_All();
	LayoutBuilt = false;
	StateDirty = false;
}


/***********************************************************************************************
 * BridgeSystem::Build_Layout -- how many spans a gap of that size takes                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool BridgeSystem::Build_Layout(void)
{
	Destroy_Geometry();

	bool all = true;

	for (int i = 0; i < Bridges.Count(); i++) {

		BridgeClass & bridge = Bridges[i];
		bridge.Sections.Delete_All();

		BridgeDefinitionClass * definition = Find_Definition(bridge.Get_Definition());
		if (definition == nullptr) {
			//	A bridge naming a kind nobody defined is a map error, not a crash.  It stays in
			//	the list with no sections, so it draws nothing, carries nothing and can be found
			//	and named in a log.
			WWDEBUG_SAY(("BridgeSystem: bridge \"%s\" wants definition \"%s\", which is not defined.\r\n",
							 bridge.Get_Name(),bridge.Get_Definition()));
			all = false;
			continue;
		}

		if (!Layout_Bridge(bridge,*definition)) {
			all = false;
		}
	}

	LayoutBuilt = true;
	return all;
}


bool BridgeSystem::Is_Layout_Built(void)
{
	return LayoutBuilt;
}


/***********************************************************************************************
 * BridgeSystem::Layout_Bridge -- caps, spans, piers and decoration along one line              *
 *                                                                                             *
 *	Variable length, which is the first thing Section 20's acceptance asks for, is this: the    *
 *	two abutments take what the definition says they take, whatever is left is divided into as  *
 *	near as makes no difference the definition's span length, and the spans are then stretched  *
 *	to fit exactly.  A bridge is never short of the far bank and never overshoots it, and no    *
 *	length of gap needs a mesh made for it.                                                     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool BridgeSystem::Layout_Bridge(BridgeClass & bridge,const BridgeDefinitionClass & definition)
{
	float length = bridge.Get_Length();
	if (length < WWMATH_EPSILON) {
		return false;
	}

	Vector3 dir = bridge.Get_Direction();

	float width = (bridge.Get_Width() > 0.0f) ? bridge.Get_Width() : definition.Get_Width();
	float half_width = width * 0.5f;

	float cap_start = definition.Get_Section_Length(BRIDGE_SECTION_START_CAP);
	float cap_end = definition.Get_Section_Length(BRIDGE_SECTION_END_CAP);

	//	Abutments that would not fit are scaled down together rather than one being dropped, so
	//	a bridge over a narrow gap is still a bridge with two ends and something between them.
	float cap_total = cap_start + cap_end;
	float cap_budget = length * BRIDGE_MAX_CAP_SHARE;
	if ((cap_total > cap_budget) && (cap_total > WWMATH_EPSILON)) {
		float scale = cap_budget / cap_total;
		cap_start *= scale;
		cap_end *= scale;
	}

	float span_region = length - cap_start - cap_end;
	if (span_region < WWMATH_EPSILON) {
		span_region = 0.0f;
	}

	int span_count = 1;
	float nominal = definition.Get_Section_Length(BRIDGE_SECTION_SPAN);
	if ((nominal > WWMATH_EPSILON) && (span_region > WWMATH_EPSILON)) {
		span_count = (int)((span_region / nominal) + 0.5f);
		if (span_count < 1) { span_count = 1; }
	}
	float span_length = (span_count > 0) ? (span_region / span_count) : 0.0f;

	//	The deck, from one abutment to the other.
	float travelled = 0.0f;

	if (cap_start > WWMATH_EPSILON) {
		Vector3 a = bridge.Get_Start() + (dir * travelled);
		Vector3 b = bridge.Get_Start() + (dir * (travelled + cap_start));
		Place_Section(bridge,BRIDGE_SECTION_START_CAP,0,a,b,half_width,0.0f);
		travelled += cap_start;
	}

	float span_start_distance = travelled;

	for (int s = 0; s < span_count; s++) {
		Vector3 a = bridge.Get_Start() + (dir * travelled);
		Vector3 b = bridge.Get_Start() + (dir * (travelled + span_length));
		Place_Section(bridge,BRIDGE_SECTION_SPAN,s,a,b,half_width,0.0f);
		travelled += span_length;
	}

	if (cap_end > WWMATH_EPSILON) {
		Vector3 a = bridge.Get_Start() + (dir * travelled);
		Vector3 b = bridge.Get_Start() + (dir * length);
		Place_Section(bridge,BRIDGE_SECTION_END_CAP,0,a,b,half_width,0.0f);
	}

	//	Piers, under the joints between spans.  An abutment stands on the bank and needs none.
	float spacing = definition.Get_Support_Spacing();
	if ((spacing > WWMATH_EPSILON) && (span_count > 1) && (span_length > WWMATH_EPSILON)) {

		int pier = 0;
		float since = 0.0f;

		for (int joint = 1; joint < span_count; joint++) {

			since += span_length;
			if (since < spacing) {
				continue;
			}
			since = 0.0f;

			Vector3 at = bridge.Get_Start() + (dir * (span_start_distance + (joint * span_length)));

			float drop = BRIDGE_NOMINAL_PIER_DROP;
			float ground = 0.0f;
			if (WorldTerrainSystem::Sample_Height(at.X,at.Y,&ground)) {
				drop = at.Z - ground;
				if (drop < definition.Get_Support_Clearance()) {
					//	The ground has come up to meet the deck.  Nothing to hold up.
					continue;
				}
			}

			Place_Section(bridge,BRIDGE_SECTION_SUPPORT,pier,at,at,
							  definition.Get_Section_Length(BRIDGE_SECTION_SUPPORT) * 0.5f,drop);
			pier++;
		}
	}

	//	Decoration, only where somebody has named some.  This is the one section kind that is
	//	absent rather than approximated when its art does not exist: a railing standing in as a
	//	row of boxes down both sides of every bridge is worse than no railing.
	float deco_spacing = definition.Get_Decoration_Spacing();
	if ((deco_spacing > WWMATH_EPSILON) &&
		 (definition.Get_Model(BRIDGE_SECTION_DECORATION,BRIDGE_STATE_PRISTINE)[0] != 0)) {

		Vector3 up(0.0f,0.0f,1.0f);
		Vector3 side = Vector3::Cross_Product(up,dir);
		if (side.Length() > WWMATH_EPSILON) {
			side.Normalize();
		} else {
			side.Set(0.0f,1.0f,0.0f);
		}

		float inset = half_width * definition.Get_Decoration_Inset();
		int ordinal = 0;

		for (float d = span_start_distance; d <= (length - cap_end); d += deco_spacing) {
			Vector3 at = bridge.Get_Start() + (dir * d);
			Vector3 left = at + (side * inset);
			Vector3 right = at - (side * inset);
			Place_Section(bridge,BRIDGE_SECTION_DECORATION,ordinal++,left,left,
							  definition.Get_Section_Length(BRIDGE_SECTION_DECORATION) * 0.5f,0.0f);
			Place_Section(bridge,BRIDGE_SECTION_DECORATION,ordinal++,right,right,
							  definition.Get_Section_Length(BRIDGE_SECTION_DECORATION) * 0.5f,0.0f);
		}
	}

	return (bridge.Get_Section_Count() > 0);
}


void BridgeSystem::Place_Section(BridgeClass & bridge,int kind,int ordinal,
											const Vector3 & start,const Vector3 & end,
											float half_width,float drop)
{
	BridgeSectionClass section;
	section.Kind = kind;
	section.State = BRIDGE_STATE_PRISTINE;
	section.Ordinal = ordinal;
	section.Start = start;
	section.End = end;
	section.HalfWidth = half_width;
	section.Drop = drop;

	Vector3 along = end - start;
	section.Length = along.Length();

	//	A section that goes nowhere -- a pier, a lamp -- still has to be oriented, and the only
	//	direction that means anything for it is the bridge's.
	Vector3 forward = (section.Length > WWMATH_EPSILON) ? (along / section.Length)
																		 : bridge.Get_Direction();

	Vector3 up(0.0f,0.0f,1.0f);
	Vector3 left = Vector3::Cross_Product(up,forward);
	if (left.Length() < WWMATH_EPSILON) {
		left.Set(0.0f,1.0f,0.0f);
	}
	left.Normalize();
	up = Vector3::Cross_Product(forward,left);
	up.Normalize();

	section.Transform.Set(forward,left,up,(start + end) * 0.5f);

	bridge.Sections.Add(section);
}


/***********************************************************************************************
 * BridgeSystem::Build_Geometry -- a physics object per section that collides                   *
 *                                                                                             *
 *	The difference from Section 19 worth stating: a road deliberately does not collide, because *
 *	the ground under it already does and two surfaces a few centimetres apart is a thing to     *
 *	trip on.  A bridge is the opposite case.  There is nothing under it but the drop, so the    *
 *	deck is the floor, and every section whose state says it collides goes into the scene.      *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool BridgeSystem::Build_Geometry(void)
{
	Destroy_Geometry();

	if (!LayoutBuilt) {
		if (!Build_Layout()) {
			//	Some bridge did not lay out.  The ones that did are still worth building.
		}
	}

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene == nullptr) {
		if (!_ReportedNoScene) {
			WWDEBUG_SAY(("BridgeSystem: no physics scene, so the bridges have no geometry.\r\n"));
			_ReportedNoScene = true;
		}
		return false;
	}

	MissingModels = 0;
	int built = 0;

	for (int b = 0; b < Bridges.Count(); b++) {

		BridgeClass & bridge = Bridges[b];

		BridgeDefinitionClass * definition = Find_Definition(bridge.Get_Definition());
		if (definition == nullptr) {
			continue;
		}

		for (int s = 0; s < bridge.Sections.Count(); s++) {
			if (Build_Section_Geometry(bridge.Sections[s],*definition)) {
				built++;
			}
		}
	}

	GeometryBuilt = (built > 0);
	return GeometryBuilt;
}


/*
**	The box a missing section is built as comes from DynamicMeshBuilderClass, which is where
**	the road system and the foliage system get theirs too.  In this section's own space X runs
**	along the bridge, Y across it and Z up, with the deck surface at Z = 0 so that whatever
**	height the endpoints were given is the height a soldier stands at.
*/


/***********************************************************************************************
 * BridgeSystem::Build_Section_Geometry -- the model, or a slab of the declared size            *
 *                                                                                             *
 *	No bridge art exists.  Rather than let that stop the section -- which would make the whole  *
 *	of Section 20 unreachable until somebody models five pieces in four states -- a section     *
 *	whose model is unnamed, or named and not in the mix files, is built as a box of exactly     *
 *	the size the definition declares.  It collides, it carries, it breaks and it is obviously   *
 *	not art, which is the right set of properties for a thing standing in for art.              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool BridgeSystem::Build_Section_Geometry(BridgeSectionClass & section,
														const BridgeDefinitionClass & definition)
{
	Destroy_Section_Geometry(section);

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene == nullptr) {
		return false;
	}

	const BridgeStateRuleClass & rule = definition.Get_Rule(section.Kind,section.State);

	//	A broken span is a hole.  There is nothing standing there to draw or to stand on, and
	//	the debris that came off it was recorded when the state changed.
	if (!rule.Collides && (rule.Model.Get_Length() == 0)) {
		return false;
	}

	RenderObjClass * model = nullptr;
	section.ModelMissing = false;

	if ((rule.Model.Get_Length() > 0) && (WW3DAssetManager::Get_Instance() != nullptr)) {
		model = WW3DAssetManager::Get_Instance()->Create_Render_Obj(rule.Model.Peek_Buffer());
	}

	if (model == nullptr) {

		section.ModelMissing = true;
		MissingModels++;

		float half_length = 0.0f;
		float half_width = section.HalfWidth;
		float top = 0.0f;
		float bottom = -definition.Get_Deck_Thickness();

		switch (section.Kind) {
			case BRIDGE_SECTION_SUPPORT:
				//	A pier is a square post from under the deck down to the ground.
				half_length = section.HalfWidth;
				top = -definition.Get_Deck_Thickness();
				bottom = -(definition.Get_Deck_Thickness() + section.Drop);
				break;

			case BRIDGE_SECTION_DECORATION:
				half_length = section.HalfWidth;
				top = section.HalfWidth * 4.0f;
				bottom = 0.0f;
				break;

			default:
				half_length = section.Length * 0.5f;
				break;
		}

		if ((half_length <= WWMATH_EPSILON) || (half_width <= WWMATH_EPSILON)) {
			return false;
		}

		DynamicMeshBuilderClass builder;

		builder.Begin_Count();
		builder.Box(half_length,half_width,top,bottom);
		if (!builder.Begin_Build()) {
			return false;
		}
		builder.Box(half_length,half_width,top,bottom);

		DynamicMeshClass * mesh = builder.Detach_Mesh();
		if (mesh == nullptr) {
			return false;
		}

		mesh->Set_Shader(ShaderClass::_PresetOpaqueShader);

		VertexMaterialClass * vmat = VertexMaterialClass::Get_Preset(VertexMaterialClass::PRELIT_DIFFUSE);
		if (vmat != nullptr) {
			mesh->Set_Vertex_Material(vmat,true);
			vmat->Release_Ref();
		}

		mesh->Set_Dirty();
		model = mesh;
	}

	model->Set_Collision_Type(rule.Collides ? BRIDGE_COLLISION_BITS : 0);

	StaticPhysClass * phys = new StaticPhysClass;
	phys->Set_Model(model);
	phys->Set_Transform(section.Transform);
	model->Release_Ref();

	scene->Add_Static_Object(phys);
	section.Phys = phys;

	return true;
}


void BridgeSystem::Destroy_Section_Geometry(BridgeSectionClass & section)
{
	if (section.Phys == nullptr) {
		return;
	}

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene != nullptr) {
		scene->Remove_Object(section.Phys);
	}
	section.Phys->Release_Ref();
	section.Phys = nullptr;
}


void BridgeSystem::Destroy_Geometry(void)
{
	for (int b = 0; b < Bridges.Count(); b++) {
		BridgeClass & bridge = Bridges[b];
		for (int s = 0; s < bridge.Sections.Count(); s++) {
			Destroy_Section_Geometry(bridge.Sections[s]);
		}
	}
	GeometryBuilt = false;
	MissingModels = 0;
}


bool BridgeSystem::Has_Geometry(void)
{
	return GeometryBuilt;
}


int BridgeSystem::Get_Instance_Count(void)
{
	int count = 0;
	for (int b = 0; b < Bridges.Count(); b++) {
		const BridgeClass & bridge = Bridges[b];
		for (int s = 0; s < bridge.Get_Section_Count(); s++) {
			if (bridge.Get_Section(s).Phys != nullptr) { count++; }
		}
	}
	return count;
}


int BridgeSystem::Get_Missing_Model_Count(void)
{
	return MissingModels;
}


/***********************************************************************************************
 * BridgeSystem::Set_Section_State -- damage, one piece at a time                               *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool BridgeSystem::Set_Section_State(int bridge_id,int section_index,int state)
{
	BridgeClass * bridge = Peek_Bridge(bridge_id);
	if (bridge == nullptr) {
		return false;
	}
	if ((state < 0) || (state >= BRIDGE_STATE_COUNT)) {
		return false;
	}
	BridgeSectionClass * section = bridge->Peek_Section(section_index);
	if (section == nullptr) {
		return false;
	}
	if (section->State == state) {
		return true;
	}

	BridgeDefinitionClass * definition = Find_Definition(bridge->Get_Definition());
	if (definition == nullptr) {
		return false;
	}

	section->State = state;
	StateDirty = true;

	Note_Debris(*bridge,section_index,*section,*definition);

	//	The geometry of one section is rebuilt, not the bridge's and not the world's.  A shell
	//	landing on a span should cost one mesh, which is the whole reason damage is per section.
	if (GeometryBuilt) {
		Build_Section_Geometry(*section,*definition);
	}

	return true;
}


int BridgeSystem::Get_Section_State(int bridge_id,int section_index)
{
	BridgeClass * bridge = Peek_Bridge(bridge_id);
	if (bridge == nullptr) {
		return -1;
	}
	BridgeSectionClass * section = bridge->Peek_Section(section_index);
	if (section == nullptr) {
		return -1;
	}
	return section->State;
}


bool BridgeSystem::Set_Damage_State(int bridge_id,int state)
{
	BridgeClass * bridge = Peek_Bridge(bridge_id);
	if (bridge == nullptr) {
		return false;
	}
	bool any = false;
	for (int s = 0; s < bridge->Get_Section_Count(); s++) {
		if (Set_Section_State(bridge_id,s,state)) { any = true; }
	}
	return any;
}


bool BridgeSystem::Break_Span_At(int bridge_id,const Vector3 & position)
{
	BridgeClass * bridge = Peek_Bridge(bridge_id);
	if (bridge == nullptr) {
		return false;
	}

	int best = -1;
	float best_distance = 0.0f;

	for (int s = 0; s < bridge->Get_Section_Count(); s++) {
		const BridgeSectionClass & section = bridge->Get_Section(s);
		if (section.Kind != BRIDGE_SECTION_SPAN) { continue; }
		Vector3 delta = section.Get_Center() - position;
		float distance = delta.Length();
		if ((best < 0) || (distance < best_distance)) {
			best = s;
			best_distance = distance;
		}
	}

	if (best < 0) {
		return false;
	}
	return Set_Section_State(bridge_id,best,BRIDGE_STATE_BROKEN);
}


/***********************************************************************************************
 * BridgeSystem::Is_Traversable -- whether the far side can still be reached                    *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool BridgeSystem::Is_Traversable(int bridge_id)
{
	int section = -1;
	return !Find_Break(bridge_id,&section);
}


bool BridgeSystem::Find_Break(int bridge_id,int * section_index_out)
{
	if (section_index_out != nullptr) {
		*section_index_out = -1;
	}

	BridgeClass * bridge = Peek_Bridge(bridge_id);
	if (bridge == nullptr) {
		return true;			// a bridge that is not there is a break in the path
	}

	BridgeDefinitionClass * definition = Find_Definition(bridge->Get_Definition());
	if (definition == nullptr) {
		return true;
	}

	if (bridge->Get_Section_Count() <= 0) {
		return true;
	}

	for (int s = 0; s < bridge->Get_Section_Count(); s++) {

		const BridgeSectionClass & section = bridge->Get_Section(s);
		if ((section.Kind != BRIDGE_SECTION_START_CAP) &&
			 (section.Kind != BRIDGE_SECTION_SPAN) &&
			 (section.Kind != BRIDGE_SECTION_END_CAP)) {
			continue;
		}

		if (!definition->Is_Traversable(section.Kind,section.State)) {
			if (section_index_out != nullptr) { *section_index_out = s; }
			return true;
		}
	}

	return false;
}


/*
**	How far onto the bridge you can get from each end before the deck stops.  A unit that has
**	to turn round wants to know where, and a repair job wants to know what is left.
*/
bool BridgeSystem::Get_Traversable_Extent(int bridge_id,float * from_start_out,float * from_end_out)
{
	if (from_start_out != nullptr) { *from_start_out = 0.0f; }
	if (from_end_out != nullptr) { *from_end_out = 0.0f; }

	BridgeClass * bridge = Peek_Bridge(bridge_id);
	if (bridge == nullptr) {
		return false;
	}
	BridgeDefinitionClass * definition = Find_Definition(bridge->Get_Definition());
	if (definition == nullptr) {
		return false;
	}

	float forward = 0.0f;
	for (int s = 0; s < bridge->Get_Section_Count(); s++) {
		const BridgeSectionClass & section = bridge->Get_Section(s);
		if ((section.Kind != BRIDGE_SECTION_START_CAP) &&
			 (section.Kind != BRIDGE_SECTION_SPAN) &&
			 (section.Kind != BRIDGE_SECTION_END_CAP)) {
			continue;
		}
		if (!definition->Is_Traversable(section.Kind,section.State)) { break; }
		forward += section.Length;
	}

	float backward = 0.0f;
	for (int s = bridge->Get_Section_Count() - 1; s >= 0; s--) {
		const BridgeSectionClass & section = bridge->Get_Section(s);
		if ((section.Kind != BRIDGE_SECTION_START_CAP) &&
			 (section.Kind != BRIDGE_SECTION_SPAN) &&
			 (section.Kind != BRIDGE_SECTION_END_CAP)) {
			continue;
		}
		if (!definition->Is_Traversable(section.Kind,section.State)) { break; }
		backward += section.Length;
	}

	if (from_start_out != nullptr) { *from_start_out = forward; }
	if (from_end_out != nullptr) { *from_end_out = backward; }
	return true;
}


/***********************************************************************************************
 * BridgeSystem::Conform_Point -- where the deck is over a given place on the map               *
 *                                                                                             *
 *	The counterpart of RoadSystem::Conform_Point, and the answer to a different question.  A    *
 *	road conforms to the ground; over a bridge the deck is the ground, and anything that walks, *
 *	drives or paths across has to be told so.  False where no bridge covers the point, and      *
 *	false where one does but the section covering it no longer carries anything -- which is     *
 *	what makes a broken span a hole rather than an invisible floor.                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool BridgeSystem::Conform_Point(float x,float y,float * height_out,int * bridge_id_out)
{
	if (bridge_id_out != nullptr) { *bridge_id_out = -1; }

	bool found = false;
	float best_height = 0.0f;
	int best_bridge = -1;

	for (int b = 0; b < Bridges.Count(); b++) {

		BridgeClass & bridge = Bridges[b];
		BridgeDefinitionClass * definition = Find_Definition(bridge.Get_Definition());
		if (definition == nullptr) { continue; }

		for (int s = 0; s < bridge.Get_Section_Count(); s++) {

			const BridgeSectionClass & section = bridge.Get_Section(s);
			if ((section.Kind != BRIDGE_SECTION_START_CAP) &&
				 (section.Kind != BRIDGE_SECTION_SPAN) &&
				 (section.Kind != BRIDGE_SECTION_END_CAP)) {
				continue;
			}
			if (section.Length <= WWMATH_EPSILON) { continue; }
			if (!definition->Is_Traversable(section.Kind,section.State)) { continue; }

			//	In plan: how far along this section, and how far to the side of it.
			Vector3 along = section.End - section.Start;
			float plan_length = WWMath::Sqrt((along.X * along.X) + (along.Y * along.Y));
			if (plan_length <= WWMATH_EPSILON) { continue; }

			float dx = x - section.Start.X;
			float dy = y - section.Start.Y;

			float t = ((dx * along.X) + (dy * along.Y)) / (plan_length * plan_length);
			if ((t < 0.0f) || (t > 1.0f)) { continue; }

			float lateral = ((dx * -along.Y) + (dy * along.X)) / plan_length;
			if (WWMath::Fabs(lateral) > section.HalfWidth) { continue; }

			float height = section.Start.Z + ((section.End.Z - section.Start.Z) * t);

			//	Bridges may cross each other.  The one you are on is the higher one.
			if (!found || (height > best_height)) {
				found = true;
				best_height = height;
				best_bridge = bridge.Get_ID();
			}
		}
	}

	if (!found) {
		return false;
	}

	if (height_out != nullptr) { *height_out = best_height; }
	if (bridge_id_out != nullptr) { *bridge_id_out = best_bridge; }
	return true;
}


int BridgeSystem::Find_Bridge_At(const Vector3 & position,float max_height_gap)
{
	float height = 0.0f;
	int id = -1;
	if (!Conform_Point(position.X,position.Y,&height,&id)) {
		return -1;
	}
	if (WWMath::Fabs(position.Z - height) > max_height_gap) {
		return -1;
	}
	return id;
}


/***********************************************************************************************
 * BridgeSystem::Connect_Roads -- bind road ends that said they hand over to a bridge           *
 *                                                                                             *
 *	Section 19 left the seam and Section 20 closes it.  A road whose connection type is         *
 *	ROAD_ENDPOINT_BRIDGE already records where its surface stopped; the bridge it meant is      *
 *	whichever one has an abutment standing there.  Nothing about the road geometry changes --   *
 *	the road already trimmed itself back at that end -- what changes is that both sides now     *
 *	know each other's id, so a path can be asked whether the crossing is open.                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int BridgeSystem::Connect_Roads(float max_distance)
{
	int bound = 0;

	for (int r = 0; r < RoadSystem::Get_Road_Count(); r++) {

		RoadSplineClass * road = RoadSystem::Peek_Road(r);
		if (road == nullptr) { continue; }

		for (int end = 0; end < 2; end++) {

			RoadConnectionClass & connection = (end == 0) ? road->Get_Start_Connection()
																	 : road->Get_End_Connection();

			if (connection.Type != ROAD_ENDPOINT_BRIDGE) { continue; }

			int best_bridge = -1;
			bool best_is_start = true;
			float best_distance = max_distance;

			for (int b = 0; b < Bridges.Count(); b++) {

				BridgeClass & bridge = Bridges[b];

				Vector3 to_start = bridge.Get_Start() - connection.Position;
				float start_distance = to_start.Length();
				if (start_distance < best_distance) {
					best_distance = start_distance;
					best_bridge = bridge.Get_ID();
					best_is_start = true;
				}

				Vector3 to_end = bridge.Get_End() - connection.Position;
				float end_distance = to_end.Length();
				if (end_distance < best_distance) {
					best_distance = end_distance;
					best_bridge = bridge.Get_ID();
					best_is_start = false;
				}
			}

			if (best_bridge < 0) { continue; }

			connection.TargetID = best_bridge;

			BridgeClass * bridge = Peek_Bridge(best_bridge);
			if (bridge != nullptr) {
				if (best_is_start) {
					bridge->Set_Start_Road(road->Get_ID());
				} else {
					bridge->Set_End_Road(road->Get_ID());
				}
			}

			bound++;
		}
	}

	return bound;
}


/***********************************************************************************************
 * BridgeSystem::Get_State_Block -- what the server sends instead of geometry                   *
 *                                                                                             *
 *	Section 20 asks that clients replicate state rather than raw geometry where deterministic   *
 *	reconstruction is possible, and here it always is: a client holding the same definition     *
 *	and the same two endpoints lays out exactly the same sections in exactly the same order.    *
 *	So what travels is one byte per section, and the transport is the network phase's business  *
 *	rather than this one's.                                                                     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static void	Put_U16(unsigned char * buffer,int & at,int value)
{
	buffer[at++] = (unsigned char)(value & 0xFF);
	buffer[at++] = (unsigned char)((value >> 8) & 0xFF);
}


static int	Get_U16(const unsigned char * buffer,int & at)
{
	int low = buffer[at++];
	int high = buffer[at++];
	return low | (high << 8);
}


int BridgeSystem::Get_State_Block_Size(void)
{
	int size = 2;			// bridge count
	for (int b = 0; b < Bridges.Count(); b++) {
		size += 4;			// id and section count
		size += Bridges[b].Get_Section_Count();
	}
	return size;
}


bool BridgeSystem::Get_State_Block(unsigned char * buffer,int max_bytes,int * used_out)
{
	if (used_out != nullptr) { *used_out = 0; }

	int needed = Get_State_Block_Size();
	if ((buffer == nullptr) || (max_bytes < needed)) {
		return false;
	}

	int at = 0;
	Put_U16(buffer,at,Bridges.Count());

	for (int b = 0; b < Bridges.Count(); b++) {
		const BridgeClass & bridge = Bridges[b];
		Put_U16(buffer,at,bridge.Get_ID());
		Put_U16(buffer,at,bridge.Get_Section_Count());
		for (int s = 0; s < bridge.Get_Section_Count(); s++) {
			buffer[at++] = (unsigned char)bridge.Get_Section(s).State;
		}
	}

	if (used_out != nullptr) { *used_out = at; }
	return true;
}


bool BridgeSystem::Apply_State_Block(const unsigned char * buffer,int bytes)
{
	if ((buffer == nullptr) || (bytes < 2)) {
		return false;
	}

	//	Applying what the server said is not a local change to send back.
	bool was_dirty = StateDirty;

	int at = 0;
	int count = Get_U16(buffer,at);

	bool all = true;

	for (int b = 0; b < count; b++) {

		if ((at + 4) > bytes) { return false; }

		int id = Get_U16(buffer,at);
		int sections = Get_U16(buffer,at);

		if ((at + sections) > bytes) { return false; }

		BridgeClass * bridge = Peek_Bridge(id);
		if ((bridge == nullptr) || (bridge->Get_Section_Count() != sections)) {
			//	The client laid out a different bridge from the one the server is describing,
			//	which means the definitions or the endpoints differ.  Skip it rather than
			//	scribble states onto the wrong sections, and say so.
			WWDEBUG_SAY(("BridgeSystem: state block describes bridge %d with %d section(s), "
							 "which does not match here.\r\n",id,sections));
			at += sections;
			all = false;
			continue;
		}

		for (int s = 0; s < sections; s++) {
			Set_Section_State(id,s,(int)buffer[at++]);
		}
	}

	StateDirty = was_dirty;
	return all;
}


bool BridgeSystem::Is_State_Dirty(void)
{
	return StateDirty;
}


void BridgeSystem::Clear_State_Dirty(void)
{
	StateDirty = false;
}


void BridgeSystem::Note_Debris(const BridgeClass & bridge,int section_index,
										 const BridgeSectionClass & section,
										 const BridgeDefinitionClass & definition)
{
	const BridgeStateRuleClass & rule = definition.Get_Rule(section.Kind,section.State);
	if ((rule.Debris.Get_Length() == 0) || (rule.DebrisCount <= 0)) {
		return;
	}

	BridgeDebrisClass debris;
	debris.BridgeID = bridge.Get_ID();
	debris.SectionIndex = section_index;
	debris.Count = rule.DebrisCount;
	debris.Position = section.Get_Center();
	debris.Model = rule.Debris;

	Debris.Add(debris);
}


int BridgeSystem::Get_Pending_Debris_Count(void)
{
	return Debris.Count();
}


const BridgeDebrisClass * BridgeSystem::Peek_Pending_Debris(int index)
{
	if ((index < 0) || (index >= Debris.Count())) {
		return nullptr;
	}
	return &Debris[index];
}


void BridgeSystem::Clear_Pending_Debris(void)
{
	Debris.Delete_All();
}
