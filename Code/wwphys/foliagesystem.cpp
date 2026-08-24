/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : OpenW3D                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/WWPhys/foliagesystem.cpp                     $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	Roadmap Section 21.  See foliagesystem.h for the shape and why it is not the donor's.       *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "foliagesystem.h"

#include "assetmgr.h"
#include "coltype.h"
#include "dynamicmeshbuilder.h"
#include "matrix3d.h"
#include "mesh.h"
#include "meshmdl.h"
#include "pscene.h"
#include "rendobj.h"
#include "shader.h"
#include "staticphys.h"
#include "texture.h"
#include "vector2.h"
#include "vertmaterial.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"

#include <string.h>


DynamicVectorClass<FoliageTypeClass>		FoliageSystem::Types;
DynamicVectorClass<FoliageInstanceClass>	FoliageSystem::Instances;
DynamicVectorClass<FoliageCellClass>		FoliageSystem::Cells;
DynamicVectorClass<FoliageBatchClass>		FoliageSystem::Batches;
DynamicVectorClass<FoliageProxyClass>		FoliageSystem::Proxies;

DynamicVectorClass<int>							FoliageSystem::CellInstances;
DynamicVectorClass<int>							FoliageSystem::BatchInstances;
DynamicVectorClass<int>							FoliageSystem::Grid;

float		FoliageSystem::CellSize				= 32.0f;
float		FoliageSystem::GridOriginX			= 0.0f;
float		FoliageSystem::GridOriginY			= 0.0f;
int		FoliageSystem::GridWidth			= 0;
int		FoliageSystem::GridHeight			= 0;

bool		FoliageSystem::CellsBuilt			= false;
bool		FoliageSystem::BatchesBuilt		= false;
bool		FoliageSystem::GeometryBuilt		= false;

int		FoliageSystem::VisibleCells		= 0;
int		FoliageSystem::VisibleBatches		= 0;
int		FoliageSystem::VisibleInstances	= 0;

int		FoliageSystem::MissingModels		= 0;
int		FoliageSystem::MultiMaterialModels = 0;
int		FoliageSystem::OversizedBatches	= 0;
int		FoliageSystem::Destroyed			= 0;


/*
**	A batch is one mesh, and a mesh is drawn through a sixteen bit index buffer, so a batch has
**	a hard ceiling however many plants stand in a cell.  The instance cap is applied when the
**	batches are planned, so the number the acceptance is measured by is the number that will
**	actually be built; the vertex ceiling is checked again when the geometry is made, because
**	only then is it known how heavy a model turned out to be.
*/
static const int	FOLIAGE_MAX_BATCH_INSTANCES	= 256;
static const int	FOLIAGE_MAX_BATCH_VERTICES		= 65000;

//	Above this many grid squares the grid is coarsened instead.  A map is not obliged to be
//	small, and a sparse forest across a large one should not allocate a square per hectare.
static const int	FOLIAGE_MAX_GRID_SQUARES		= 65536;

/*
**	What blocks what, in the engine's own collision bits.  Anything that stops a body also
**	stops a bullet: a tree thick enough to walk into is thick enough to hide behind, and a
**	forest that shots pass through is a worse lie than a forest that is not there.
*/
static int Foliage_Collision_Bits(unsigned blocking)
{
	int bits = 0;

	if ((blocking & FOLIAGE_BLOCKS_INFANTRY) != 0) {
		bits |= COLLISION_TYPE_PHYSICAL;
	}
	if ((blocking & FOLIAGE_BLOCKS_VEHICLES) != 0) {
		bits |= COLLISION_TYPE_VEHICLE;
	}
	if (bits != 0) {
		bits |= COLLISION_TYPE_PROJECTILE;
	}

	return bits;
}


/*
**	Foliage is cut-out art: a leaf sheet is mostly transparent and both of its faces are seen,
**	so the batches are alpha tested and not culled.  Not culling is also what makes the far
**	level's crossed quads work from every side without emitting each of them twice.
*/
static ShaderClass Foliage_Shader(void)
{
	ShaderClass shader = ShaderClass::_PresetOpaqueShader;
	shader.Set_Alpha_Test(ShaderClass::ALPHATEST_ENABLE);
	shader.Set_Cull_Mode(ShaderClass::CULL_MODE_DISABLE);
	return shader;
}


static unsigned Foliage_Next_Random(unsigned & state)
{
	state = (state * 1664525u) + 1013904223u;
	return state;
}


static float Foliage_Random_Unit(unsigned & state)
{
	return (float)(Foliage_Next_Random(state) >> 8) / (float)(1 << 24);
}


float FoliageCellClass::Get_Radius(void) const
{
	Vector3 extent = (Max - Min) * 0.5f;
	return extent.Length();
}


void FoliageSystem::Init(void)
{
	Shutdown();
	Define_Default_Foliage();
	return ;
}


void FoliageSystem::Shutdown(void)
{
	Clear_Instances();
	Types.Delete_All();
	return ;
}


/***********************************************************************************************
 *	Types.                                                                                      *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool FoliageSystem::Define_Type(const FoliageTypeClass & type)
{
	if (::strlen(type.Get_Name()) == 0) {
		return false;
	}

	for (int i = 0; i < Types.Count(); i++) {
		if (::stricmp(Types[i].Get_Name(),type.Get_Name()) == 0) {
			Types[i] = type;
			return true;
		}
	}

	Types.Add(type);
	return true;
}


int FoliageSystem::Get_Type_Count(void)
{
	return Types.Count();
}


FoliageTypeClass * FoliageSystem::Peek_Type(int index)
{
	if ((index < 0) || (index >= Types.Count())) {
		return nullptr;
	}
	return &Types[index];
}


int FoliageSystem::Find_Type_Index(const char * name)
{
	if (name == nullptr) {
		return -1;
	}

	for (int i = 0; i < Types.Count(); i++) {
		if (::stricmp(Types[i].Get_Name(),name) == 0) {
			return i;
		}
	}
	return -1;
}


FoliageTypeClass * FoliageSystem::Find_Type(const char * name)
{
	return Peek_Type(Find_Type_Index(name));
}


/***********************************************************************************************
 *	FoliageSystem::Define_Default_Foliage -- one of each category, naming no art                *
 *                                                                                             *
 *	Section 21 lists five categories and four blocking behaviours, and these five types are     *
 *	one of each category chosen so that between them they are all four behaviours: a tree       *
 *	stops everything, a dead tree stops a soldier and is destructible, scrub is destructible    *
 *	and stops nobody, grass is visual only, and a boulder stops everything and does not break.  *
 *                                                                                             *
 *	None of them names a model, a material or an impostor.  A forest of them plans exactly the  *
 *	same batches and draws nothing at all, which is the honest state of the system until there  *
 *	is art; see docs/assets/FoliageModels.md.                                                   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void FoliageSystem::Define_Default_Foliage(void)
{
	{
		FoliageTypeClass type;
		type.Set_Name("ow_tree_conifer");
		type.Set_Category(FOLIAGE_TREE);
		type.Set_Blocking(FOLIAGE_BLOCKS_INFANTRY | FOLIAGE_BLOCKS_VEHICLES);
		type.Set_Proxy(0.7f,9.0f);
		type.Set_Distances(90.0f,400.0f);
		type.Set_Scale_Range(0.85f,1.25f);
		Define_Type(type);
	}

	{
		FoliageTypeClass type;
		type.Set_Name("ow_bush_scrub");
		type.Set_Category(FOLIAGE_BUSH);
		type.Set_Blocking(FOLIAGE_BLOCKS_NOTHING);
		type.Set_Destructible(true,25.0f);
		type.Set_Proxy(0.0f,0.0f);
		type.Set_Distances(40.0f,120.0f);
		type.Set_Scale_Range(0.7f,1.4f);
		Define_Type(type);
	}

	{
		FoliageTypeClass type;
		type.Set_Name("ow_tree_dead");
		type.Set_Category(FOLIAGE_DEAD_TREE);
		type.Set_Blocking(FOLIAGE_BLOCKS_INFANTRY);
		type.Set_Destructible(true,150.0f);
		type.Set_Proxy(0.5f,6.0f);
		type.Set_Distances(80.0f,350.0f);
		type.Set_Scale_Range(0.8f,1.2f);
		Define_Type(type);
	}

	{
		FoliageTypeClass type;
		type.Set_Name("ow_grass_clump");
		type.Set_Category(FOLIAGE_GRASS_CLUMP);
		type.Set_Blocking(FOLIAGE_BLOCKS_NOTHING);
		type.Set_Proxy(0.0f,0.0f);
		type.Set_Distances(25.0f,60.0f);
		type.Set_Scale_Range(0.6f,1.5f);
		Define_Type(type);
	}

	{
		FoliageTypeClass type;
		type.Set_Name("ow_rock_boulder");
		type.Set_Category(FOLIAGE_DECORATIVE_ROCK);
		type.Set_Blocking(FOLIAGE_BLOCKS_INFANTRY | FOLIAGE_BLOCKS_VEHICLES);
		type.Set_Proxy(1.2f,1.6f);
		type.Set_Distances(80.0f,400.0f);
		type.Set_Scale_Range(0.6f,1.8f);
		Define_Type(type);
	}

	return ;
}


/***********************************************************************************************
 *	Instances.                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int FoliageSystem::Add_Instance(const FoliageInstanceClass & instance)
{
	if ((instance.Type < 0) || (instance.Type >= Types.Count())) {
		return -1;
	}

	FoliageInstanceClass added = instance;
	added.Cell = -1;
	if (added.Health <= 0.0f) {
		added.Health = Types[added.Type].Get_Health();
	}

	//	The grid describes where everything is, and this is somewhere new.  Guarded because a
	//	forest is added one plant at a time and clearing an empty grid per plant is quadratic.
	if (CellsBuilt || BatchesBuilt) {
		Clear_Batches();
		Clear_Cells();
	}

	Instances.Add(added);
	return Instances.Count() - 1;
}


int FoliageSystem::Add_Instance(const char * type,const Vector3 & position,float yaw,float scale)
{
	int index = Find_Type_Index(type);
	if (index < 0) {
		return -1;
	}

	FoliageInstanceClass instance;
	instance.Type = index;
	instance.Position = position;
	instance.Yaw = yaw;
	instance.Scale = (scale > 0.0f) ? scale : 1.0f;

	return Add_Instance(instance);
}


/***********************************************************************************************
 *	FoliageSystem::Scatter -- a forest from a seed                                              *
 *                                                                                             *
 *	The generator is here rather than in whatever wanted a forest because the same seed has to  *
 *	produce the same trees on the server and on every client.  Section 21 has no network        *
 *	requirement of its own, but the moment a tree blocks a soldier its position is gameplay,    *
 *	and a forest reconstructed from four numbers is a forest nobody has to send.                *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int FoliageSystem::Scatter(const char * type,const Vector3 & center,float radius,
									int count,unsigned seed)
{
	int index = Find_Type_Index(type);
	if ((index < 0) || (count <= 0) || (radius <= 0.0f)) {
		return 0;
	}

	const FoliageTypeClass & definition = Types[index];

	unsigned state = seed;
	int added = 0;

	for (int i = 0; i < count; i++) {

		//	Square rooted so the plants are spread evenly over the area rather than crowded
		//	into the middle, which is what a uniform radius would do.
		float distance = radius * WWMath::Sqrt(Foliage_Random_Unit(state));
		float angle = Foliage_Random_Unit(state) * 2.0f * WWMATH_PI;

		FoliageInstanceClass instance;
		instance.Type = index;
		instance.Position.X = center.X + (distance * WWMath::Cos(angle));
		instance.Position.Y = center.Y + (distance * WWMath::Sin(angle));
		instance.Position.Z = center.Z;
		instance.Yaw = Foliage_Random_Unit(state) * 2.0f * WWMATH_PI;

		float t = Foliage_Random_Unit(state);
		instance.Scale = definition.Get_Scale_Min() +
								(t * (definition.Get_Scale_Max() - definition.Get_Scale_Min()));

		//	Where there is ground to stand on, stand on it.  Where there is not -- a check with
		//	no world loaded -- the height the caller gave is the height it wanted.
		float ground = 0.0f;
		if (WorldTerrainSystem::Sample_Height(instance.Position.X,instance.Position.Y,&ground)) {
			instance.Position.Z = ground;
		}

		if (Add_Instance(instance) >= 0) {
			added++;
		}
	}

	return added;
}


int FoliageSystem::Get_Instance_Count(void)
{
	return Instances.Count();
}


int FoliageSystem::Get_Live_Instance_Count(void)
{
	int live = 0;
	for (int i = 0; i < Instances.Count(); i++) {
		if (Instances[i].Alive) { live++; }
	}
	return live;
}


const FoliageInstanceClass * FoliageSystem::Peek_Instance(int index)
{
	if ((index < 0) || (index >= Instances.Count())) {
		return nullptr;
	}
	return &Instances[index];
}


void FoliageSystem::Clear_Instances(void)
{
	Clear_Batches();
	Clear_Cells();
	Instances.Delete_All();
	Destroyed = 0;
	return ;
}


/***********************************************************************************************
 *	FoliageSystem::Build_Cells -- the grid                                                      *
 *                                                                                             *
 *	The grid covers the instances and nothing else, so a handful of trees in a corner of a very *
 *	large map costs a handful of cells.  If the extents and the cell size would ask for an      *
 *	unreasonable number of squares the cell size is doubled until they do not, which keeps a    *
 *	sparse scatter over a whole map from allocating a square per hectare.                       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool FoliageSystem::Build_Cells(float cell_size)
{
	Clear_Batches();
	Clear_Cells();

	if (Instances.Count() == 0) {
		return false;
	}

	if (cell_size <= 1.0f) {
		cell_size = 32.0f;
	}

	Vector3 min = Instances[0].Position;
	Vector3 max = Instances[0].Position;

	for (int i = 1; i < Instances.Count(); i++) {
		const Vector3 & at = Instances[i].Position;
		if (at.X < min.X) { min.X = at.X; }
		if (at.Y < min.Y) { min.Y = at.Y; }
		if (at.Z < min.Z) { min.Z = at.Z; }
		if (at.X > max.X) { max.X = at.X; }
		if (at.Y > max.Y) { max.Y = at.Y; }
		if (at.Z > max.Z) { max.Z = at.Z; }
	}

	CellSize = cell_size;
	GridOriginX = min.X;
	GridOriginY = min.Y;

	for (;;) {
		GridWidth = (int)((max.X - min.X) / CellSize) + 1;
		GridHeight = (int)((max.Y - min.Y) / CellSize) + 1;

		if ((GridWidth * GridHeight) <= FOLIAGE_MAX_GRID_SQUARES) {
			break;
		}
		CellSize *= 2.0f;
	}

	Grid.Resize(GridWidth * GridHeight);
	for (int g = 0; g < (GridWidth * GridHeight); g++) {
		Grid.Add(-1);
	}

	//	Which square each instance is in, and how many are in each square.
	DynamicVectorClass<int> counts;
	counts.Resize(GridWidth * GridHeight);
	for (int c = 0; c < (GridWidth * GridHeight); c++) {
		counts.Add(0);
	}

	for (int i = 0; i < Instances.Count(); i++) {

		int x = (int)((Instances[i].Position.X - GridOriginX) / CellSize);
		int y = (int)((Instances[i].Position.Y - GridOriginY) / CellSize);

		if (x < 0) { x = 0; }
		if (y < 0) { y = 0; }
		if (x >= GridWidth) { x = GridWidth - 1; }
		if (y >= GridHeight) { y = GridHeight - 1; }

		int square = (y * GridWidth) + x;
		Instances[i].Cell = square;
		counts[square]++;
	}

	//	Only squares that hold something become cells, so the cell list is dense even when the
	//	grid that indexes it is not.
	int first = 0;
	for (int square = 0; square < (GridWidth * GridHeight); square++) {

		if (counts[square] == 0) {
			continue;
		}

		FoliageCellClass cell;
		cell.X = square % GridWidth;
		cell.Y = square / GridWidth;
		cell.First = first;
		cell.Count = 0;
		//	The Z extremes start crossed so that the first instance in the cell replaces both.
		cell.Min = Vector3(GridOriginX + (cell.X * CellSize),
								 GridOriginY + (cell.Y * CellSize),
								 WWMATH_FLOAT_MAX);
		cell.Max = Vector3(cell.Min.X + CellSize,cell.Min.Y + CellSize,-WWMATH_FLOAT_MAX);

		Grid[square] = Cells.Count();
		Cells.Add(cell);

		first += counts[square];
	}

	CellInstances.Resize(Instances.Count());
	for (int s = 0; s < Instances.Count(); s++) {
		CellInstances.Add(-1);
	}

	for (int i = 0; i < Instances.Count(); i++) {

		int cell_index = Grid[Instances[i].Cell];
		if (cell_index < 0) {
			continue;
		}

		FoliageCellClass & cell = Cells[cell_index];
		CellInstances[cell.First + cell.Count] = i;
		cell.Count++;

		//	The cell's own extent in Z is whatever is standing in it, so a cell in a valley is
		//	not as tall as the map.  A type with no proxy still has a height worth covering, so
		//	the impostor height stands in for one that names no post.
		float z = Instances[i].Position.Z;
		const FoliageTypeClass & type = Types[Instances[i].Type];
		float tall = WWMath::Max(type.Get_Proxy_Height(),type.Get_Impostor_Height());
		if (z < cell.Min.Z) { cell.Min.Z = z; }
		if ((z + tall) > cell.Max.Z) { cell.Max.Z = z + tall; }

		//	Instances remember the cell they are in, not the grid square.
		Instances[i].Cell = cell_index;
	}

	CellsBuilt = true;
	return true;
}


bool FoliageSystem::Are_Cells_Built(void)
{
	return CellsBuilt;
}


int FoliageSystem::Get_Cell_Count(void)
{
	return Cells.Count();
}


const FoliageCellClass * FoliageSystem::Peek_Cell(int index)
{
	if ((index < 0) || (index >= Cells.Count())) {
		return nullptr;
	}
	return &Cells[index];
}


float FoliageSystem::Get_Cell_Size(void)
{
	return CellSize;
}


int FoliageSystem::Find_Cell(float x,float y)
{
	if (!CellsBuilt || (GridWidth <= 0) || (GridHeight <= 0)) {
		return -1;
	}

	int gx = (int)((x - GridOriginX) / CellSize);
	int gy = (int)((y - GridOriginY) / CellSize);

	if ((gx < 0) || (gy < 0) || (gx >= GridWidth) || (gy >= GridHeight)) {
		return -1;
	}

	return Grid[(gy * GridWidth) + gx];
}


int FoliageSystem::Get_Cell_Instance(int cell,int n)
{
	if ((cell < 0) || (cell >= Cells.Count())) {
		return -1;
	}
	if ((n < 0) || (n >= Cells[cell].Count)) {
		return -1;
	}
	return CellInstances[Cells[cell].First + n];
}


void FoliageSystem::Clear_Cells(void)
{
	Cells.Delete_All();
	CellInstances.Delete_All();
	Grid.Delete_All();
	GridWidth = 0;
	GridHeight = 0;
	CellsBuilt = false;

	for (int i = 0; i < Instances.Count(); i++) {
		Instances[i].Cell = -1;
	}
	return ;
}


/***********************************************************************************************
 *	FoliageSystem::Build_Batches -- the acceptance                                              *
 *                                                                                             *
 *	Within a cell, everything sharing a material and a level of detail becomes one batch, and   *
 *	one batch is one thing to draw.  This is the whole of Section 21's acceptance and it is     *
 *	answerable here, before any geometry exists and without a graphics device, because it is a  *
 *	question about how the instances group and not about what they look like.                   *
 *                                                                                             *
 *	A batch is capped at a number of instances rather than left to grow, so that one dense cell *
 *	cannot produce a mesh too large for a sixteen bit index buffer.  A capped cell produces      *
 *	several batches of the same material, which is still a small number.                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool FoliageSystem::Build_Batches(void)
{
	Clear_Batches();

	if (!CellsBuilt) {
		return false;
	}

	for (int c = 0; c < Cells.Count(); c++) {

		const FoliageCellClass & cell = Cells[c];

		for (int lod = 0; lod < FOLIAGE_LOD_BUILT_COUNT; lod++) {

			//	Every instance in the cell that is alive, has this level of detail at all, and
			//	has not already been put in a batch of this material.
			DynamicVectorClass<int> pending;

			for (int n = 0; n < cell.Count; n++) {

				int index = CellInstances[cell.First + n];
				if ((index < 0) || !Instances[index].Alive) { continue; }

				const FoliageTypeClass & type = Types[Instances[index].Type];
				if ((lod == FOLIAGE_LOD_FAR) && !type.Has_Far_Lod()) { continue; }

				pending.Add(index);
			}

			//	An instance is taken out of the pending list by being set to -1 rather than by
			//	being removed, so that the list keeps its shape while it is being walked.
			int remaining = pending.Count();

			while (remaining > 0) {

				int seed = -1;
				for (int i = 0; i < pending.Count(); i++) {
					if (pending[i] >= 0) { seed = i; break; }
				}
				if (seed < 0) { break; }

				StringClass material = Types[Instances[pending[seed]].Type].Get_Lod_Material(lod);

				FoliageBatchClass batch;
				batch.Cell = c;
				batch.Lod = lod;
				batch.Material = material;
				batch.First = BatchInstances.Count();
				batch.Count = 0;
				batch.NearDistance = 0.0f;
				batch.CullDistance = 0.0f;

				bool first = true;

				for (int p = seed; p < pending.Count(); p++) {

					if (pending[p] < 0) { continue; }
					if (batch.Count >= FOLIAGE_MAX_BATCH_INSTANCES) { break; }

					const FoliageTypeClass & type = Types[Instances[pending[p]].Type];
					if (material != type.Get_Lod_Material(lod)) { continue; }

					//	The batch keeps its detail until the first thing in it would drop it, and
					//	survives until the last thing in it would go.  Merging means the batch is
					//	drawn or not; erring towards drawing is the error nobody sees.
					if (first) {
						batch.NearDistance = type.Get_Near_Distance();
						batch.CullDistance = type.Get_Cull_Distance();
						first = false;
					} else {
						batch.NearDistance = WWMath::Min(batch.NearDistance,type.Get_Near_Distance());
						batch.CullDistance = WWMath::Max(batch.CullDistance,type.Get_Cull_Distance());
					}

					BatchInstances.Add(pending[p]);
					batch.Count++;
					pending[p] = -1;
					remaining--;
				}

				if (batch.Count == 0) {
					break;
				}

				Batches.Add(batch);
			}
		}
	}

	Build_Proxies();

	BatchesBuilt = true;
	return true;
}


bool FoliageSystem::Are_Batches_Built(void)
{
	return BatchesBuilt;
}


int FoliageSystem::Get_Batch_Count(void)
{
	return Batches.Count();
}


const FoliageBatchClass * FoliageSystem::Peek_Batch(int index)
{
	if ((index < 0) || (index >= Batches.Count())) {
		return nullptr;
	}
	return &Batches[index];
}


int FoliageSystem::Get_Batch_Instance(int batch,int n)
{
	if ((batch < 0) || (batch >= Batches.Count())) {
		return -1;
	}
	if ((n < 0) || (n >= Batches[batch].Count)) {
		return -1;
	}
	return BatchInstances[Batches[batch].First + n];
}


/***********************************************************************************************
 *	FoliageSystem::Build_Proxies -- the optional collision                                      *
 *                                                                                             *
 *	Section 21 calls collision proxies optional and they are: a cell with nothing solid in it   *
 *	gets none.  Where they exist they are merged the same way the drawn batches are, one per    *
 *	set of collision bits, so a wood that stops infantry and vehicles is two collision objects  *
 *	per cell rather than one per tree.                                                          *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool FoliageSystem::Build_Proxies(void)
{
	for (int c = 0; c < Cells.Count(); c++) {

		const FoliageCellClass & cell = Cells[c];

		//	The distinct blocking behaviours standing in this cell.
		DynamicVectorClass<unsigned> kinds;

		for (int n = 0; n < cell.Count; n++) {

			int index = CellInstances[cell.First + n];
			if ((index < 0) || !Instances[index].Alive) { continue; }

			const FoliageTypeClass & type = Types[Instances[index].Type];
			if (type.Is_Visual_Only()) { continue; }
			if ((type.Get_Proxy_Radius() <= 0.0f) || (type.Get_Proxy_Height() <= 0.0f)) { continue; }

			bool known = false;
			for (int k = 0; k < kinds.Count(); k++) {
				if (kinds[k] == type.Get_Blocking()) { known = true; break; }
			}
			if (!known) {
				kinds.Add(type.Get_Blocking());
			}
		}

		for (int k = 0; k < kinds.Count(); k++) {

			FoliageProxyClass proxy;
			proxy.Cell = c;
			proxy.Blocking = kinds[k];
			proxy.Count = 0;

			for (int n = 0; n < cell.Count; n++) {
				int index = CellInstances[cell.First + n];
				if ((index < 0) || !Instances[index].Alive) { continue; }
				const FoliageTypeClass & type = Types[Instances[index].Type];
				if (type.Get_Blocking() != kinds[k]) { continue; }
				if ((type.Get_Proxy_Radius() <= 0.0f) || (type.Get_Proxy_Height() <= 0.0f)) { continue; }
				proxy.Count++;
			}

			if (proxy.Count > 0) {
				Proxies.Add(proxy);
			}
		}
	}

	return (Proxies.Count() > 0);
}


int FoliageSystem::Get_Proxy_Count(void)
{
	return Proxies.Count();
}


const FoliageProxyClass * FoliageSystem::Peek_Proxy(int index)
{
	if ((index < 0) || (index >= Proxies.Count())) {
		return nullptr;
	}
	return &Proxies[index];
}


void FoliageSystem::Clear_Batches(void)
{
	Destroy_Geometry();

	Batches.Delete_All();
	Proxies.Delete_All();
	BatchInstances.Delete_All();

	BatchesBuilt = false;
	VisibleCells = 0;
	VisibleBatches = 0;
	VisibleInstances = 0;
	return ;
}


/***********************************************************************************************
 *	FoliageSystem::Update_Visibility -- what is worth drawing from here                         *
 *                                                                                             *
 *	Cells are culled first, by distance and by a cone about the view direction, and a culled    *
 *	cell takes every batch in it with it.  That is the point of having cells: the test that     *
 *	would be per tree is per cell, and the answer covers everything standing in it.             *
 *                                                                                             *
 *	Then each surviving batch picks its level of detail from the same distance, and the two     *
 *	batches of a cell that has both never draw at once.                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void FoliageSystem::Update_Visibility(const Vector3 & eye,const Vector3 & forward,
												  float half_angle_cos,float max_distance)
{
	VisibleCells = 0;
	VisibleBatches = 0;
	VisibleInstances = 0;

	Vector3 look = forward;
	if (look.Length2() > WWMATH_EPSILON) {
		look.Normalize();
	} else {
		half_angle_cos = -1.0f;
	}

	for (int c = 0; c < Cells.Count(); c++) {

		FoliageCellClass & cell = Cells[c];

		Vector3 to_cell = cell.Get_Center() - eye;
		float radius = cell.Get_Radius();
		float distance = to_cell.Length() - radius;
		if (distance < 0.0f) { distance = 0.0f; }

		cell.Distance = distance;
		cell.Visible = true;

		if ((max_distance > 0.0f) && (distance > max_distance)) {
			cell.Visible = false;
		}

		//	The cone is widened by the cell's own size, so a cell whose centre is off to the
		//	side but whose near corner is straight ahead is not thrown away.
		if (cell.Visible && (half_angle_cos > -1.0f)) {
			float length = to_cell.Length();
			if (length > radius) {
				float cosine = Vector3::Dot_Product(to_cell,look) / length;
				float slack = radius / length;
				if (cosine < (half_angle_cos - slack)) {
					cell.Visible = false;
				}
			}
		}

		if (cell.Visible) {
			VisibleCells++;
		}
	}

	for (int b = 0; b < Batches.Count(); b++) {

		FoliageBatchClass & batch = Batches[b];
		batch.Visible = false;

		if ((batch.Cell < 0) || (batch.Cell >= Cells.Count())) { continue; }

		const FoliageCellClass & cell = Cells[batch.Cell];
		if (!cell.Visible) { continue; }

		if (cell.Distance > batch.CullDistance) { continue; }

		int wanted = (cell.Distance <= batch.NearDistance) ? FOLIAGE_LOD_NEAR : FOLIAGE_LOD_FAR;
		if (batch.Lod != wanted) { continue; }

		batch.Visible = true;
		VisibleBatches++;
		VisibleInstances += batch.Count;
	}

	Apply_Visibility();
	return ;
}


void FoliageSystem::Apply_Visibility(void)
{
	for (int b = 0; b < Batches.Count(); b++) {
		if (Batches[b].Phys == nullptr) { continue; }
		RenderObjClass * model = Batches[b].Phys->Peek_Model();
		if (model != nullptr) {
			model->Set_Hidden(Batches[b].Visible ? 0 : 1);
		}
	}
	return ;
}


int FoliageSystem::Get_Visible_Cell_Count(void)
{
	return VisibleCells;
}


int FoliageSystem::Get_Visible_Batch_Count(void)
{
	return VisibleBatches;
}


int FoliageSystem::Get_Visible_Instance_Count(void)
{
	return VisibleInstances;
}


/***********************************************************************************************
 *	Geometry.                                                                                   *
 *                                                                                             *
 *	A near batch is the contributing models' own triangles, transformed by each instance's yaw, *
 *	scale and position and written into one mesh.  Reading a model's triangles once and         *
 *	stamping them many times is the donor's idea and the reason any of this is faster than      *
 *	instancing; what is different is that the stamping happens once, into a mesh that stays,    *
 *	rather than every frame into a buffer that is refilled.                                     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

namespace
{

/*
**	One type's model, flattened into a triangle list in model space.  Three positions and three
**	texture coordinates per triangle, nothing shared: the merged mesh writes every vertex of
**	every instance anyway, so an index would be one more thing to remap.
*/
class FoliageModelCacheClass
{
public:

	FoliageModelCacheClass(void) : Type(-1), Missing(true), MultiMaterial(false) { }

	int											Type;
	bool											Missing;
	bool											MultiMaterial;
	DynamicVectorClass<Vector3>			Positions;
	DynamicVectorClass<Vector2>			UVs;
};


void Collect_Mesh(FoliageModelCacheClass & cache,RenderObjClass * object,const Matrix3D & transform)
{
	if (object == nullptr) {
		return ;
	}

	if (object->Class_ID() == RenderObjClass::CLASSID_MESH) {

		MeshClass * mesh = (MeshClass *)object;
		MeshModelClass * model = mesh->Peek_Model();

		if (model != nullptr) {

			if ((model->Get_Pass_Count() > 1) ||
				 (model->Get_Texture_Array(0,0,false) != nullptr)) {
				cache.MultiMaterial = true;
			}

			const TriIndex * polys = model->Get_Polygon_Array();
			Vector3 * verts = model->Get_Vertex_Array();
			const Vector2 * uvs = model->Get_UV_Array(0,0);

			if ((polys != nullptr) && (verts != nullptr)) {
				for (int p = 0; p < model->Get_Polygon_Count(); p++) {
					for (int v = 0; v < 3; v++) {
						int index = polys[p][v];
						cache.Positions.Add(transform * verts[index]);
						cache.UVs.Add((uvs != nullptr) ? uvs[index] : Vector2(0.0f,0.0f));
					}
				}
			}
		}
	}

	for (int s = 0; s < object->Get_Num_Sub_Objects(); s++) {
		RenderObjClass * sub = object->Get_Sub_Object(s);
		if (sub != nullptr) {
			Collect_Mesh(cache,sub,transform * sub->Get_Transform());
			sub->Release_Ref();
		}
	}

	return ;
}


DynamicVectorClass<FoliageModelCacheClass *>	_ModelCache;


FoliageModelCacheClass * Get_Model_Cache(int type,const char * model_name)
{
	for (int i = 0; i < _ModelCache.Count(); i++) {
		if (_ModelCache[i]->Type == type) {
			return _ModelCache[i];
		}
	}

	FoliageModelCacheClass * cache = new FoliageModelCacheClass;
	cache->Type = type;

	if ((model_name != nullptr) && (::strlen(model_name) > 0) &&
		 (WW3DAssetManager::Get_Instance() != nullptr)) {

		RenderObjClass * object = WW3DAssetManager::Get_Instance()->Create_Render_Obj(model_name);
		if (object != nullptr) {
			Matrix3D identity(1);
			Collect_Mesh(*cache,object,identity);
			object->Release_Ref();
		}
	}

	cache->Missing = (cache->Positions.Count() == 0);

	_ModelCache.Add(cache);
	return cache;
}


void Clear_Model_Cache(void)
{
	for (int i = 0; i < _ModelCache.Count(); i++) {
		delete _ModelCache[i];
	}
	_ModelCache.Delete_All();
	return ;
}

}	// anonymous namespace


/***********************************************************************************************
 *	FoliageSystem::Build_Batch_Geometry -- one cell's worth of one material, as one mesh        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool FoliageSystem::Build_Batch_Geometry(FoliageBatchClass & batch)
{
	Destroy_Batch_Geometry(batch);

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if ((scene == nullptr) || (batch.Cell < 0) || (batch.Cell >= Cells.Count())) {
		return false;
	}

	//	Everything in the batch is written relative to the middle of its cell, so a forest at
	//	the far corner of a large map is not built out of large numbers.
	Vector3 origin = Cells[batch.Cell].Get_Center();

	DynamicMeshBuilderClass builder;
	builder.Begin_Count();

	for (int pass = 0; pass < 2; pass++) {

		if (pass == 1) {
			if (builder.Get_Vert_Count() > FOLIAGE_MAX_BATCH_VERTICES) {
				OversizedBatches++;
				return false;
			}
			if (!builder.Begin_Build()) {
				return false;
			}
		}

		for (int n = 0; n < batch.Count; n++) {

			int index = BatchInstances[batch.First + n];
			if ((index < 0) || !Instances[index].Alive) { continue; }

			const FoliageInstanceClass & instance = Instances[index];
			const FoliageTypeClass & type = Types[instance.Type];

			float sine = WWMath::Sin(instance.Yaw);
			float cosine = WWMath::Cos(instance.Yaw);
			Vector3 offset = instance.Position - origin;

			if (batch.Lod == FOLIAGE_LOD_FAR) {

				//	Two quads crossed at a right angle.  Nothing here is camera facing: a batch
				//	is built once and a merged mesh cannot turn its plants to face anybody, so
				//	the cross is what makes an impostor read from every direction.
				float half_width = type.Get_Impostor_Width() * instance.Scale * 0.5f;
				float height = type.Get_Impostor_Height() * instance.Scale;

				if ((half_width <= 0.0f) || (height <= 0.0f)) { continue; }

				Vector3 across[2];
				across[0] = Vector3(cosine * half_width,sine * half_width,0.0f);
				across[1] = Vector3(-sine * half_width,cosine * half_width,0.0f);

				for (int q = 0; q < 2; q++) {
					Vector3 low_left = offset - across[q];
					Vector3 low_right = offset + across[q];
					Vector3 up = Vector3(0.0f,0.0f,height);

					builder.Begin_Strip();
					builder.Vertex(low_left + up,0.0f,0.0f);
					builder.Vertex(low_right + up,1.0f,0.0f);
					builder.Vertex(low_left,0.0f,1.0f);
					builder.Vertex(low_right,1.0f,1.0f);
				}

			} else {

				FoliageModelCacheClass * cache = Get_Model_Cache(instance.Type,type.Get_Model());
				if ((cache == nullptr) || cache->Missing) { continue; }

				for (int t = 0; t + 2 < cache->Positions.Count(); t += 3) {

					Vector3 corner[3];
					for (int v = 0; v < 3; v++) {
						const Vector3 & source = cache->Positions[t + v];
						Vector3 scaled = source * instance.Scale;
						corner[v] = Vector3((cosine * scaled.X) - (sine * scaled.Y),
													(sine * scaled.X) + (cosine * scaled.Y),
													scaled.Z) + offset;
					}

					builder.Triangle(corner[0],corner[1],corner[2],
											cache->UVs[t + 0].U,cache->UVs[t + 0].V,
											cache->UVs[t + 1].U,cache->UVs[t + 1].V,
											cache->UVs[t + 2].U,cache->UVs[t + 2].V);
				}
			}
		}

		if (pass == 0) {
			if (builder.Get_Poly_Count() <= 0) {
				batch.PolyCount = 0;
				return false;
			}
		}
	}

	DynamicMeshClass * mesh = builder.Detach_Mesh();
	if (mesh == nullptr) {
		return false;
	}

	mesh->Set_Shader(Foliage_Shader());

	VertexMaterialClass * vmat = VertexMaterialClass::Get_Preset(VertexMaterialClass::PRELIT_DIFFUSE);
	if (vmat != nullptr) {
		mesh->Set_Vertex_Material(vmat,true);
		vmat->Release_Ref();
	}

	if ((batch.Material.Get_Length() > 0) && (WW3DAssetManager::Get_Instance() != nullptr)) {
		TextureClass * texture = WW3DAssetManager::Get_Instance()->Get_Texture(batch.Material.Peek_Buffer());
		if (texture != nullptr) {
			mesh->Set_Texture(texture,true);
			texture->Release_Ref();
		}
	}

	//	Nothing drawn collides.  What a soldier walks into is the proxy, which is a post where
	//	the trunk is and not the shape of the leaves.
	mesh->Set_Collision_Type(0);
	mesh->Set_Dirty();

	batch.PolyCount = builder.Get_Poly_Count();

	StaticPhysClass * phys = new StaticPhysClass;
	phys->Set_Model(mesh);
	phys->Set_Transform(Matrix3D(origin));
	mesh->Release_Ref();

	scene->Add_Static_Object(phys);
	batch.Phys = phys;

	return true;
}


/***********************************************************************************************
 *	FoliageSystem::Build_Proxy_Geometry -- the posts                                            *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool FoliageSystem::Build_Proxy_Geometry(FoliageProxyClass & proxy)
{
	Destroy_Proxy_Geometry(proxy);

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if ((scene == nullptr) || (proxy.Cell < 0) || (proxy.Cell >= Cells.Count())) {
		return false;
	}

	const FoliageCellClass & cell = Cells[proxy.Cell];
	Vector3 origin = cell.Get_Center();

	DynamicMeshBuilderClass builder;
	builder.Begin_Count();

	for (int pass = 0; pass < 2; pass++) {

		if ((pass == 1) && !builder.Begin_Build()) {
			return false;
		}

		for (int n = 0; n < cell.Count; n++) {

			int index = CellInstances[cell.First + n];
			if ((index < 0) || !Instances[index].Alive) { continue; }

			const FoliageInstanceClass & instance = Instances[index];
			const FoliageTypeClass & type = Types[instance.Type];

			if (type.Get_Blocking() != proxy.Blocking) { continue; }

			float radius = type.Get_Proxy_Radius() * instance.Scale;
			float height = type.Get_Proxy_Height() * instance.Scale;
			if ((radius <= 0.0f) || (height <= 0.0f)) { continue; }

			builder.Box(radius,radius,height,0.0f,instance.Position - origin);
		}

		if ((pass == 0) && (builder.Get_Poly_Count() <= 0)) {
			return false;
		}
	}

	DynamicMeshClass * mesh = builder.Detach_Mesh();
	if (mesh == nullptr) {
		return false;
	}

	mesh->Set_Collision_Type(Foliage_Collision_Bits(proxy.Blocking));
	mesh->Set_Dirty();

	//	A proxy is a collision shape and not a thing to look at.  What is seen is the batch.
	mesh->Set_Hidden(1);

	StaticPhysClass * phys = new StaticPhysClass;
	phys->Set_Model(mesh);
	phys->Set_Transform(Matrix3D(origin));
	mesh->Release_Ref();

	scene->Add_Static_Object(phys);
	proxy.Phys = phys;

	return true;
}


void FoliageSystem::Destroy_Batch_Geometry(FoliageBatchClass & batch)
{
	if (batch.Phys == nullptr) {
		return ;
	}

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene != nullptr) {
		scene->Remove_Object(batch.Phys);
	}
	batch.Phys->Release_Ref();
	batch.Phys = nullptr;
	batch.PolyCount = 0;
	return ;
}


void FoliageSystem::Destroy_Proxy_Geometry(FoliageProxyClass & proxy)
{
	if (proxy.Phys == nullptr) {
		return ;
	}

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	if (scene != nullptr) {
		scene->Remove_Object(proxy.Phys);
	}
	proxy.Phys->Release_Ref();
	proxy.Phys = nullptr;
	return ;
}


bool FoliageSystem::Build_Geometry(void)
{
	Destroy_Geometry();

	if (!BatchesBuilt) {
		return false;
	}
	if (PhysicsSceneClass::Get_Instance() == nullptr) {
		return false;
	}

	MissingModels = 0;
	MultiMaterialModels = 0;
	OversizedBatches = 0;

	int built = 0;

	for (int b = 0; b < Batches.Count(); b++) {
		if (Build_Batch_Geometry(Batches[b])) {
			built++;
		}
	}

	for (int p = 0; p < Proxies.Count(); p++) {
		if (Build_Proxy_Geometry(Proxies[p])) {
			built++;
		}
	}

	//	What the models turned out to be, reported rather than guessed at.  A missing model is
	//	a batch that plans and draws nothing; a model with more than one material is a model
	//	drawn with the first of them, which is a constraint on the art and not a bug here.
	for (int i = 0; i < Get_Type_Count(); i++) {
		for (int m = 0; m < _ModelCache.Count(); m++) {
			if (_ModelCache[m]->Type != i) { continue; }
			if (_ModelCache[m]->Missing) { MissingModels++; }
			if (_ModelCache[m]->MultiMaterial) { MultiMaterialModels++; }
		}
	}

	Clear_Model_Cache();

	GeometryBuilt = (built > 0);
	return GeometryBuilt;
}


void FoliageSystem::Destroy_Geometry(void)
{
	for (int b = 0; b < Batches.Count(); b++) {
		Destroy_Batch_Geometry(Batches[b]);
	}
	for (int p = 0; p < Proxies.Count(); p++) {
		Destroy_Proxy_Geometry(Proxies[p]);
	}

	Clear_Model_Cache();
	GeometryBuilt = false;
	return ;
}


bool FoliageSystem::Has_Geometry(void)
{
	return GeometryBuilt;
}


int FoliageSystem::Get_Object_Count(void)
{
	int count = 0;
	for (int b = 0; b < Batches.Count(); b++) {
		if (Batches[b].Phys != nullptr) { count++; }
	}
	for (int p = 0; p < Proxies.Count(); p++) {
		if (Proxies[p].Phys != nullptr) { count++; }
	}
	return count;
}


int FoliageSystem::Get_Missing_Model_Count(void)
{
	return MissingModels;
}


int FoliageSystem::Get_Multi_Material_Model_Count(void)
{
	return MultiMaterialModels;
}


int FoliageSystem::Get_Oversized_Batch_Count(void)
{
	return OversizedBatches;
}


/***********************************************************************************************
 *	Destruction.                                                                                *
 *                                                                                             *
 *	Felling one tree rebuilds the meshes of the cell it stood in and nothing else.  That is the *
 *	other half of why cells are the size they are: a cell is small enough that rebuilding one   *
 *	is cheap, and large enough that there are not many of them.                                 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool FoliageSystem::Apply_Damage(int instance,float amount)
{
	if ((instance < 0) || (instance >= Instances.Count())) {
		return false;
	}
	if (!Instances[instance].Alive) {
		return false;
	}

	const FoliageTypeClass & type = Types[Instances[instance].Type];
	if (!type.Is_Destructible()) {
		return false;
	}

	Instances[instance].Health -= amount;
	if (Instances[instance].Health > 0.0f) {
		return false;
	}

	return Destroy_Instance(instance);
}


bool FoliageSystem::Destroy_Instance(int instance)
{
	if ((instance < 0) || (instance >= Instances.Count())) {
		return false;
	}
	if (!Instances[instance].Alive) {
		return false;
	}

	Instances[instance].Alive = false;
	Instances[instance].Health = 0.0f;
	Destroyed++;

	Rebuild_Cell_Geometry(Instances[instance].Cell);
	return true;
}


void FoliageSystem::Rebuild_Cell_Geometry(int cell)
{
	if ((cell < 0) || (cell >= Cells.Count())) {
		return ;
	}
	if (!GeometryBuilt) {
		return ;
	}

	for (int b = 0; b < Batches.Count(); b++) {
		if (Batches[b].Cell == cell) {
			Build_Batch_Geometry(Batches[b]);
		}
	}
	for (int p = 0; p < Proxies.Count(); p++) {
		if (Proxies[p].Cell == cell) {
			Build_Proxy_Geometry(Proxies[p]);
		}
	}

	Clear_Model_Cache();
	Apply_Visibility();
	return ;
}


int FoliageSystem::Get_Destroyed_Count(void)
{
	return Destroyed;
}


/***********************************************************************************************
 *	Queries.                                                                                    *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int FoliageSystem::Find_Instances_Near(const Vector3 & position,float radius,
													DynamicVectorClass<int> & found)
{
	found.Delete_All();

	if (radius <= 0.0f) {
		return 0;
	}

	//	With a grid, only the squares the circle touches are looked at.  Without one there is
	//	nothing to do but look at everything, which is what an unbuilt world deserves.
	if (!CellsBuilt) {
		for (int i = 0; i < Instances.Count(); i++) {
			if (!Instances[i].Alive) { continue; }
			if ((Instances[i].Position - position).Length() <= radius) {
				found.Add(i);
			}
		}
		return found.Count();
	}

	int low_x = (int)((position.X - radius - GridOriginX) / CellSize);
	int high_x = (int)((position.X + radius - GridOriginX) / CellSize);
	int low_y = (int)((position.Y - radius - GridOriginY) / CellSize);
	int high_y = (int)((position.Y + radius - GridOriginY) / CellSize);

	if (low_x < 0) { low_x = 0; }
	if (low_y < 0) { low_y = 0; }
	if (high_x >= GridWidth) { high_x = GridWidth - 1; }
	if (high_y >= GridHeight) { high_y = GridHeight - 1; }

	for (int y = low_y; y <= high_y; y++) {
		for (int x = low_x; x <= high_x; x++) {

			int cell = Grid[(y * GridWidth) + x];
			if (cell < 0) { continue; }

			for (int n = 0; n < Cells[cell].Count; n++) {
				int index = CellInstances[Cells[cell].First + n];
				if ((index < 0) || !Instances[index].Alive) { continue; }
				if ((Instances[index].Position - position).Length() <= radius) {
					found.Add(index);
				}
			}
		}
	}

	return found.Count();
}


int FoliageSystem::Find_Nearest_Instance(const Vector3 & position,float radius)
{
	DynamicVectorClass<int> found;
	Find_Instances_Near(position,radius,found);

	int nearest = -1;
	float best = 0.0f;

	for (int i = 0; i < found.Count(); i++) {
		float distance = (Instances[found[i]].Position - position).Length();
		if ((nearest < 0) || (distance < best)) {
			nearest = found[i];
			best = distance;
		}
	}

	return nearest;
}


/***********************************************************************************************
 *	FoliageSystem::Is_Blocked -- would something with these bits get through                    *
 *                                                                                             *
 *	Answered from the proxies' own shapes rather than from the physics scene, so that a         *
 *	path-finder can ask before any geometry exists and a server with no scene at all can        *
 *	still refuse to walk a soldier through a tree.                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool FoliageSystem::Is_Blocked(const Vector3 & position,unsigned blocking_bits)
{
	if (blocking_bits == FOLIAGE_BLOCKS_NOTHING) {
		return false;
	}

	DynamicVectorClass<int> found;
	Find_Instances_Near(position,CellSize,found);

	for (int i = 0; i < found.Count(); i++) {

		const FoliageInstanceClass & instance = Instances[found[i]];
		const FoliageTypeClass & type = Types[instance.Type];

		if ((type.Get_Blocking() & blocking_bits) == 0) { continue; }

		float radius = type.Get_Proxy_Radius() * instance.Scale;
		float height = type.Get_Proxy_Height() * instance.Scale;
		if ((radius <= 0.0f) || (height <= 0.0f)) { continue; }

		if ((position.Z < instance.Position.Z) ||
			 (position.Z > (instance.Position.Z + height))) {
			continue;
		}

		float dx = WWMath::Fabs(position.X - instance.Position.X);
		float dy = WWMath::Fabs(position.Y - instance.Position.Y);

		if ((dx <= radius) && (dy <= radius)) {
			return true;
		}
	}

	return false;
}
