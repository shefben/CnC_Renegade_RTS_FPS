/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/WWPhys/watersystem.cpp                       $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	Roadmap Section 22.  See watersystem.h for why there is no second collision system here.    *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "watersystem.h"

#include "aabox.h"
#include "assetmgr.h"
#include "coltype.h"
#include "dynamicmeshbuilder.h"
#include "matrix3d.h"
#include "mesh.h"
#include "pscene.h"
#include "rendobj.h"
#include "shader.h"
#include "staticphys.h"
#include "terraintexturesystem.h"
#include "terrainmask.h"
#include "texture.h"
#include "vertmaterial.h"
#include "w3d_file.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"

#include <string.h>


DynamicVectorClass<WaterDefinitionClass>	WaterSystem::Definitions;
DynamicVectorClass<WaterAreaClass>			WaterSystem::Areas;
int					WaterSystem::NextID			= 0;
int					WaterSystem::MissingTextures	= 0;

//	A closed area's interior is carved in scanline strips rather than one huge region, capped
//	the same way FoliageSystem caps its cell grid: a fixed row budget that the spacing doubles
//	to stay under, so a large lake costs more metres per row rather than an unbounded number of
//	terrain edits.
static const float	WATER_MIN_CARVE_SPACING	= 4.0f;
static const int		WATER_MAX_CARVE_ROWS		= 256;


/***********************************************************************************************
 *	The shader.  Water is translucent and moving, which is a different thing from foliage's      *
 *	alpha-tested cutout: there is no edge to cut around, only a surface to blend over whatever    *
 *	is beneath it.                                                                                *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
static ShaderClass Water_Shader(void)
{
	ShaderClass shader = ShaderClass::_PresetOpaqueShader;
	shader.Set_Alpha_Test(ShaderClass::ALPHATEST_DISABLE);
	shader.Set_Depth_Mask(ShaderClass::DEPTH_WRITE_DISABLE);
	shader.Set_Src_Blend_Func(ShaderClass::SRCBLEND_SRC_ALPHA);
	shader.Set_Dst_Blend_Func(ShaderClass::DSTBLEND_ONE_MINUS_SRC_ALPHA);
	shader.Set_Cull_Mode(ShaderClass::CULL_MODE_DISABLE);
	return shader;
}


/***********************************************************************************************
 *	WaterSystem::Init / Shutdown                                                                *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void WaterSystem::Init(void)
{
	Shutdown();
}


void WaterSystem::Shutdown(void)
{
	Destroy_All_Geometry();
	Definitions.Delete_All();
	Areas.Delete_All();
	NextID = 0;
	MissingTextures = 0;
}


/***********************************************************************************************
 * WaterSystem::Define_Default_Water -- one kind per category, none of them naming any art      *
 *=============================================================================================*/
void WaterSystem::Define_Default_Water(void)
{
	struct DefaultRow { const char * name; int category; float scroll; float wave; float feather; float depth; };

	static const DefaultRow rows[] = {
		{ "ow_water_ocean",	WATER_OCEAN,	0.3f,	0.35f,	6.0f,	0.0f },
		{ "ow_water_coast",	WATER_COAST,	0.25f,	0.25f,	4.0f,	0.0f },
		{ "ow_water_lake",	WATER_LAKE,		0.05f,	0.10f,	2.0f,	3.0f },
		{ "ow_water_pond",	WATER_POND,		0.02f,	0.05f,	1.0f,	1.5f },
		{ "ow_water_river",	WATER_RIVER,	0.6f,		0.05f,	1.5f,	2.5f },
		{ "ow_water_stream",	WATER_STREAM,	0.5f,		0.02f,	0.75f,	1.0f },
	};

	for (int i = 0; i < (int)(sizeof(rows) / sizeof(rows[0])); i++) {

		WaterDefinitionClass def;
		def.Set_Name(rows[i].name);
		def.Set_Category(rows[i].category);
		def.Set_Scroll_Speed(rows[i].scroll);
		def.Set_Wave_Amplitude(rows[i].wave);
		def.Set_Shore_Feather(rows[i].feather);
		def.Set_Bed_Depth(rows[i].depth);
		Define_Definition(def);
	}
}


int WaterSystem::Define_Definition(const WaterDefinitionClass & def)
{
	int index = Find_Definition_Index(def.Get_Name());
	if (index >= 0) {
		Definitions[index] = def;
		return index;
	}
	Definitions.Add(def);
	return Definitions.Count() - 1;
}


static WaterDefinitionClass _EmptyDefinition;

const WaterDefinitionClass & WaterSystem::Peek_Definition(int index)
{
	if ((index < 0) || (index >= Definitions.Count())) {
		return _EmptyDefinition;
	}
	return Definitions[index];
}


int WaterSystem::Find_Definition_Index(const char * name)
{
	if (name == nullptr) { return -1; }
	for (int i = 0; i < Definitions.Count(); i++) {
		if (::strcmp(Definitions[i].Get_Name(),name) == 0) { return i; }
	}
	return -1;
}


WaterDefinitionClass * WaterSystem::Find_Definition(const char * name)
{
	int index = Find_Definition_Index(name);
	return (index >= 0) ? &Definitions[index] : nullptr;
}


/***********************************************************************************************
 *	Placed areas                                                                                 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int WaterSystem::Add_Area(const WaterAreaClass & area)
{
	WaterAreaClass placed = area;
	placed.Phys = nullptr;
	placed.TextureMissing = false;
	placed.Set_ID(NextID++);
	Areas.Add(placed);
	return placed.Get_ID();
}


void WaterSystem::Clear_Areas(void)
{
	Destroy_All_Geometry();
	Areas.Delete_All();
}


static WaterAreaClass _EmptyArea;

const WaterAreaClass & WaterSystem::Peek_Area(int index)
{
	if ((index < 0) || (index >= Areas.Count())) {
		return _EmptyArea;
	}
	return Areas[index];
}


int WaterSystem::Find_Area_Index(const char * name)
{
	if (name == nullptr) { return -1; }
	for (int i = 0; i < Areas.Count(); i++) {
		if (::strcmp(Areas[i].Get_Name(),name) == 0) { return i; }
	}
	return -1;
}


WaterAreaClass * WaterSystem::Find_Area(const char * name)
{
	int index = Find_Area_Index(name);
	return (index >= 0) ? &Areas[index] : nullptr;
}


/***********************************************************************************************
 * Carve_Water_Basin -- scanline-fill the interior of a closed area down to its bed             *
 *                                                                                             *
 * The boundary itself is carved by the caller with one Apply_River_Cut around the ring; this   *
 * is what levels the middle of a lake instead of leaving a rim around an untouched hilltop.    *
 * Even-odd scanline fill handles a concave shoreline correctly, which a single centroid-based   *
 * cut would not.                                                                                 *
 *=============================================================================================*/
static void Carve_Water_Basin(const WaterAreaClass & area,float depth)
{
	int count = area.Get_Station_Count();
	if (count < 3) { return; }

	AABoxClass bounds;
	if (!area.Get_Bounds(&bounds)) { return; }

	float min_y = bounds.Center.Y - bounds.Extent.Y;
	float max_y = bounds.Center.Y + bounds.Extent.Y;
	float span_y = max_y - min_y;
	if (span_y < WWMATH_EPSILON) { return; }

	float spacing = WATER_MIN_CARVE_SPACING;
	int rows = (int)WWMath::Ceil(span_y / spacing) + 1;
	while (rows > WATER_MAX_CARVE_ROWS) {
		spacing *= 2.0f;
		rows = (int)WWMath::Ceil(span_y / spacing) + 1;
	}

	float height = area.Get_Height();

	for (int r = 0; r <= rows; r++) {

		float y = min_y + spacing * (float)r;
		if (y > max_y) { y = max_y; }

		DynamicVectorClass<float> crossings;
		for (int i = 0; i < count; i++) {
			const Vector3 & a = area.Get_Station(i).Position;
			const Vector3 & b = area.Get_Station((i + 1) % count).Position;
			bool crosses = ((a.Y > y) != (b.Y > y));
			if (crosses) {
				float x = a.X + (y - a.Y) * (b.X - a.X) / (b.Y - a.Y);
				crossings.Add(x);
			}
		}
		if (crossings.Count() < 2) { continue; }

		//	Crossings per row are few; insertion sort is the right tool for a handful of floats.
		for (int i = 1; i < crossings.Count(); i++) {
			float key = crossings[i];
			int j = i - 1;
			while ((j >= 0) && (crossings[j] > key)) { crossings[j+1] = crossings[j]; j--; }
			crossings[j+1] = key;
		}

		for (int i = 0; i + 1 < crossings.Count(); i += 2) {
			Vector3 line[2];
			line[0] = Vector3(crossings[i],y,height);
			line[1] = Vector3(crossings[i+1],y,height);
			//	A little wider than the row spacing so adjacent rows overlap and leave no seam.
			WorldTerrainSystem::Apply_River_Cut(line,2,spacing * 1.5f,depth);
		}
	}
}


/***********************************************************************************************
 * WaterSystem::Shape_Terrain -- carve a bed for one area                                       *
 *=============================================================================================*/
bool WaterSystem::Shape_Terrain(int index)
{
	if ((index < 0) || (index >= Areas.Count())) { return false; }

	const WaterAreaClass & area = Areas[index];
	const WaterDefinitionClass * def = Find_Definition(area.Get_Definition());
	int category = (def != nullptr) ? def->Get_Category() : WATER_LAKE;
	float depth = (def != nullptr) ? def->Get_Bed_Depth() : 2.0f;

	//	Ocean and coast are not carved: there is no boundary for a channel to be cut along, and
	//	the level's own terrain is assumed to already sit below the declared surface height.
	if (!Water_Category_Carves_Terrain(category)) { return true; }

	int count = area.Get_Station_Count();
	if (count < 2) { return false; }

	if (!area.Is_Closed()) {

		DynamicVectorClass<Vector3> points;
		float width = 0.0f;
		for (int i = 0; i < count; i++) {
			points.Add(area.Get_Station(i).Position);
			if (area.Get_Station(i).Width > width) { width = area.Get_Station(i).Width; }
		}
		if (width <= 0.0f) { width = 2.0f; }

		WorldTerrainSystem::Apply_River_Cut(&points[0],points.Count(),width,depth);
		return true;
	}

	if (count < 3) { return false; }

	DynamicVectorClass<Vector3> ring;
	for (int i = 0; i < count; i++) {
		Vector3 p = area.Get_Station(i).Position;
		p.Z = area.Get_Height();
		ring.Add(p);
	}
	ring.Add(ring[0]);
	WorldTerrainSystem::Apply_River_Cut(&ring[0],ring.Count(),WATER_MIN_CARVE_SPACING,depth);

	Carve_Water_Basin(area,depth);
	return true;
}


int WaterSystem::Shape_All_Terrain(void)
{
	int shaped = 0;
	for (int i = 0; i < Areas.Count(); i++) {
		if (Shape_Terrain(i)) { shaped++; }
	}
	return shaped;
}


/***********************************************************************************************
 * WaterSystem::Stamp_Mask -- tell the terrain texture system where the shoreline is            *
 *                                                                                             *
 * TERRAIN_MASK_RIVER and TERRAIN_MASK_WATER_DISTANCE were built in Section 18 with nothing to  *
 * stamp them; this is the writer they were left waiting for.  A closed area stamps its ring     *
 * rather than filling its interior -- the interior is beneath the water mesh and never seen,    *
 * so only the edge, where dry ground shows through, needs a blend at all.                       *
 *=============================================================================================*/
bool WaterSystem::Stamp_Mask(int index)
{
	if ((index < 0) || (index >= Areas.Count())) { return false; }
	if (!TerrainTextureSystem::Has_Masks()) { return false; }

	TerrainMaskClass * mask = TerrainTextureSystem::Peek_Mask(TERRAIN_MASK_RIVER);
	if ((mask == nullptr) || !mask->Is_Valid()) { return false; }

	const WaterAreaClass & area = Areas[index];
	const WaterDefinitionClass * def = Find_Definition(area.Get_Definition());
	float feather = (def != nullptr) ? def->Get_Shore_Feather() : 2.0f;

	int count = area.Get_Station_Count();
	if (count < 2) { return false; }

	DynamicVectorClass<Vector3> line;
	float width = 0.0f;
	for (int i = 0; i < count; i++) {
		line.Add(area.Get_Station(i).Position);
		if (area.Get_Station(i).Width > width) { width = area.Get_Station(i).Width; }
	}
	if (area.Is_Closed()) {
		line.Add(area.Get_Station(0).Position);
		width = WATER_MIN_CARVE_SPACING;
	}
	if (width <= 0.0f) { width = 2.0f; }

	mask->Stamp_Polyline(&line[0],line.Count(),width,1.0f,feather);
	return true;
}


int WaterSystem::Stamp_All_Masks(void)
{
	int stamped = 0;
	for (int i = 0; i < Areas.Count(); i++) {
		if (Stamp_Mask(i)) { stamped++; }
	}
	if (stamped > 0) {
		TerrainTextureSystem::Update_Water_Distance();
	}
	return stamped;
}


/***********************************************************************************************
 * WaterSystem::Build_Area_Geometry -- one surface mesh, permeable and untraversable            *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool WaterSystem::Build_Area_Geometry(WaterAreaClass & area,const WaterDefinitionClass & def)
{
	Destroy_Area_Geometry(area);

	if (!def.Names_Any_Texture()) {
		area.TextureMissing = true;
		MissingTextures++;
		return false;
	}

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene == nullptr) { return false; }

	int count = area.Get_Station_Count();
	if (count < 2) { return false; }

	AABoxClass bounds;
	if (!area.Get_Bounds(&bounds)) { return false; }
	Vector3 origin = bounds.Center;

	const float UV_SCALE = 0.1f;		// arbitrary tiling; art is authored to repeat seamlessly

	DynamicMeshBuilderClass builder;
	builder.Begin_Count();

	for (int pass = 0; pass < 2; pass++) {

		if ((pass == 1) && !builder.Begin_Build()) { return false; }

		if (area.Is_Closed()) {

			if (count < 3) { return false; }

			//	A fan from the centroid.  Correct for the convex ponds and lakes this targets;
			//	a concave shoreline fans incorrectly rather than crashing, a stated limitation.
			Vector3 center(origin.X,origin.Y,area.Get_Height());
			for (int i = 0; i < count; i++) {

				Vector3 a = area.Get_Station(i).Position;			a.Z = area.Get_Height();
				Vector3 b = area.Get_Station((i + 1) % count).Position;	b.Z = area.Get_Height();

				builder.Triangle(center - origin,a - origin,b - origin,
										center.X * UV_SCALE,center.Y * UV_SCALE,
										a.X * UV_SCALE,a.Y * UV_SCALE,
										b.X * UV_SCALE,b.Y * UV_SCALE);
			}

		} else {

			float distance = 0.0f;
			for (int i = 0; i < count - 1; i++) {

				const WaterStationClass & sa = area.Get_Station(i);
				const WaterStationClass & sb = area.Get_Station(i + 1);

				Vector3 span = sb.Position - sa.Position;
				float length = span.Length();
				if (length < WWMATH_EPSILON) { continue; }

				Vector3 tangent = span / length;
				Vector3 normal(-tangent.Y,tangent.X,0.0f);

				Vector3 a0 = sa.Position - normal * (sa.Width * 0.5f) - origin;
				Vector3 a1 = sa.Position + normal * (sa.Width * 0.5f) - origin;
				Vector3 b0 = sb.Position - normal * (sb.Width * 0.5f) - origin;
				Vector3 b1 = sb.Position + normal * (sb.Width * 0.5f) - origin;

				float v0 = distance * UV_SCALE;
				float v1 = (distance + length) * UV_SCALE;

				builder.Begin_Strip();
				builder.Vertex(a0,0.0f,v0);
				builder.Vertex(a1,1.0f,v0);
				builder.Vertex(b0,0.0f,v1);
				builder.Vertex(b1,1.0f,v1);

				distance += length;
			}
		}

		if ((pass == 0) && (builder.Get_Poly_Count() <= 0)) { return false; }
	}

	DynamicMeshClass * mesh = builder.Detach_Mesh();
	if (mesh == nullptr) { return false; }

	mesh->Set_Shader(Water_Shader());

	VertexMaterialClass * vmat = VertexMaterialClass::Get_Preset(VertexMaterialClass::PRELIT_DIFFUSE);
	if (vmat != nullptr) {
		mesh->Set_Vertex_Material(vmat,true);
		vmat->Release_Ref();
	}

	if ((def.Get_Surface_Texture()[0] != 0) && (WW3DAssetManager::Get_Instance() != nullptr)) {
		TextureClass * texture = WW3DAssetManager::Get_Instance()->Get_Texture(def.Get_Surface_Texture());
		if (texture != nullptr) {
			mesh->Set_Texture(texture,true);
			texture->Release_Ref();
		}
	}

	//	Permeable to movement, visible to rays and bullets -- see watersystem.h for why this is
	//	not a second floor.  Every poly is tagged water, the same way any authored geometry is.
	mesh->Set_Collision_Type(COLLISION_TYPE_PROJECTILE);
	int poly_count = mesh->Get_Num_Polys();
	uint8 * surface = mesh->Peek_Model()->Get_Poly_Surface_Type_Array();
	for (int p = 0; p < poly_count; p++) { surface[p] = SURFACE_TYPE_WATER_PERMEABLE; }
	mesh->Set_Dirty();

	StaticPhysClass * phys = new StaticPhysClass;
	phys->Set_Model(mesh);
	phys->Set_Transform(Matrix3D(origin));
	mesh->Release_Ref();

	scene->Add_Static_Object(phys);
	area.Phys = phys;
	area.TextureMissing = false;

	return true;
}


bool WaterSystem::Build_Geometry(int index)
{
	if ((index < 0) || (index >= Areas.Count())) { return false; }

	const WaterDefinitionClass * def = Find_Definition(Areas[index].Get_Definition());
	if (def == nullptr) { return false; }

	return Build_Area_Geometry(Areas[index],*def);
}


int WaterSystem::Build_All_Geometry(void)
{
	int built = 0;
	for (int i = 0; i < Areas.Count(); i++) {
		if (Build_Geometry(i)) { built++; }
	}
	return built;
}


void WaterSystem::Destroy_Area_Geometry(WaterAreaClass & area)
{
	if (area.Phys == nullptr) { return; }

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene != nullptr) {
		scene->Remove_Object(area.Phys);
	}
	area.Phys->Release_Ref();
	area.Phys = nullptr;
}


void WaterSystem::Destroy_Geometry(int index)
{
	if ((index < 0) || (index >= Areas.Count())) { return; }
	Destroy_Area_Geometry(Areas[index]);
}


void WaterSystem::Destroy_All_Geometry(void)
{
	for (int i = 0; i < Areas.Count(); i++) {
		Destroy_Area_Geometry(Areas[i]);
	}
	MissingTextures = 0;
}


bool WaterSystem::Has_Geometry(int index)
{
	if ((index < 0) || (index >= Areas.Count())) { return false; }
	return (Areas[index].Phys != nullptr);
}


int WaterSystem::Get_Object_Count(void)
{
	int count = 0;
	for (int i = 0; i < Areas.Count(); i++) {
		if (Areas[i].Phys != nullptr) { count++; }
	}
	return count;
}


/***********************************************************************************************
 *	Headless queries                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int WaterSystem::Find_Area_At(float x,float y)
{
	for (int i = 0; i < Areas.Count(); i++) {
		if (Areas[i].Contains_Point(x,y)) { return i; }
	}
	return -1;
}


bool WaterSystem::Get_Water_Height(float x,float y,float * height_out)
{
	int index = Find_Area_At(x,y);
	if (index < 0) { return false; }
	return Areas[index].Get_Surface_Height(x,y,height_out);
}


bool WaterSystem::Get_Water_Depth(float x,float y,float ground_z,float * depth_out)
{
	if (depth_out == nullptr) { return false; }

	float height;
	if (!Get_Water_Height(x,y,&height)) { return false; }

	float depth = height - ground_z;
	*depth_out = (depth > 0.0f) ? depth : 0.0f;
	return true;
}


bool WaterSystem::Get_Flow(float x,float y,Vector3 * direction_out,float * speed_out)
{
	int index = Find_Area_At(x,y);
	if (index < 0) { return false; }

	Vector3 direction(0.0f,0.0f,0.0f);
	Areas[index].Get_Flow_Direction(x,y,&direction);

	const WaterDefinitionClass * def = Find_Definition(Areas[index].Get_Definition());
	float speed = (def != nullptr) ? def->Get_Scroll_Speed() : 0.0f;
	if (direction.Length2() < WWMATH_EPSILON) { speed = 0.0f; }

	if (direction_out != nullptr) { *direction_out = direction; }
	if (speed_out != nullptr) { *speed_out = speed; }
	return true;
}


bool WaterSystem::Is_Navigable(float x,float y,float ground_z,float min_depth)
{
	float depth;
	if (!Get_Water_Depth(x,y,ground_z,&depth)) { return false; }
	return (depth >= min_depth);
}
