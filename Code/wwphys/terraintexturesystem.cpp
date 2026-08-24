/*
**	OpenW3D
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "terraintexturesystem.h"

#include "heightfield.h"
#include "refcount.h"
#include "renegadeterrainmaterialpass.h"
#include "renegadeterrainpatch.h"
#include "terrainmaterial.h"
#include "worldterrainsystem.h"
#include "wwdebug.h"
#include "wwmath.h"


DynamicVectorClass<TerrainLayerClass *>	TerrainTextureSystem::Layers;
TerrainMaskClass *								TerrainTextureSystem::Masks			= nullptr;
bool													TerrainTextureSystem::MasksCreated	= false;


/***********************************************************************************************
 * TerrainLayerRuleClass                                                                       *
 *=============================================================================================*/

TerrainLayerRuleClass::TerrainLayerRuleClass(void) :
	//	Every window starts with its minimum above its maximum, which means off.  A rule that
	//	names nothing therefore fires everywhere at full strength, which is what a base layer is.
	MinHeight(1.0f),			MaxHeight(0.0f),			HeightFeather(0.0f),
	MinSlope(1.0f),			MaxSlope(0.0f),			SlopeFeather(0.0f),
	MinCurvature(1.0f),		MaxCurvature(0.0f),		CurvatureFeather(0.0f),
	MaskType(TERRAIN_MASK_NONE),	MinMask(1.0f),		MaxMask(0.0f),		MaskFeather(0.0f),
	NoiseAmount(0.0f),		NoiseFrequency(0.05f),	NoiseSeed(0),
	Strength(1.0f),			Exclusive(false)
{
}


void TerrainLayerRuleClass::Set_Height_Range(float min_height,float max_height,float feather)
{
	MinHeight = min_height;
	MaxHeight = max_height;
	HeightFeather = feather;
}


void TerrainLayerRuleClass::Set_Slope_Range(float min_slope,float max_slope,float feather)
{
	MinSlope = min_slope;
	MaxSlope = max_slope;
	SlopeFeather = feather;
}


void TerrainLayerRuleClass::Set_Curvature_Range(float min_curvature,float max_curvature,float feather)
{
	MinCurvature = min_curvature;
	MaxCurvature = max_curvature;
	CurvatureFeather = feather;
}


void TerrainLayerRuleClass::Set_Mask_Range(int mask_type,float min_value,float max_value,float feather)
{
	MaskType = mask_type;
	MinMask = min_value;
	MaxMask = max_value;
	MaskFeather = feather;
}


void TerrainLayerRuleClass::Set_Noise(float amount,float frequency,int seed)
{
	NoiseAmount = amount;
	NoiseFrequency = frequency;
	NoiseSeed = seed;
}


/***********************************************************************************************
 * TerrainLayerClass                                                                           *
 *=============================================================================================*/

TerrainLayerClass::TerrainLayerClass(void) :
	Kind(TERRAIN_LAYER_BASE),
	Material(nullptr)
{
}


TerrainLayerClass::~TerrainLayerClass(void)
{
	REF_PTR_RELEASE(Material);
}


/***********************************************************************************************
 * Lifetime                                                                                    *
 *=============================================================================================*/

void TerrainTextureSystem::Init(void)
{
	Clear_Layers();
	Destroy_Masks();
}


void TerrainTextureSystem::Shutdown(void)
{
	Clear_Layers();
	Destroy_Masks();
}


/***********************************************************************************************
 * Masks                                                                                       *
 *=============================================================================================*/

bool TerrainTextureSystem::Create_Masks(void)
{
	Destroy_Masks();

	HeightfieldClass * field = WorldTerrainSystem::Peek_Heightfield();
	if ((field == nullptr) || !field->Is_Valid()) {
		return false;
	}

	Masks = new TerrainMaskClass[TERRAIN_MASK_COUNT];

	for (int index = 0; index < TERRAIN_MASK_COUNT; index++) {
		if (!Masks[index].Create(field->Get_Vertex_Count_X(),field->Get_Vertex_Count_Y(),
										 field->Get_Cell_Size(),field->Get_Origin())) {
			Destroy_Masks();
			return false;
		}
	}

	//	Water distance saturates rather than starting at zero.  A map with no river drawn in it
	//	is a map where nowhere is near water, and a shoreline layer keyed off distance would
	//	otherwise flood the whole world before anyone drew anything.
	Masks[TERRAIN_MASK_WATER_DISTANCE].Clear(10000.0f);

	MasksCreated = true;
	return true;
}


void TerrainTextureSystem::Destroy_Masks(void)
{
	if (Masks != nullptr) {
		delete [] Masks;
		Masks = nullptr;
	}
	MasksCreated = false;
}


bool TerrainTextureSystem::Has_Masks(void)
{
	return MasksCreated;
}


TerrainMaskClass * TerrainTextureSystem::Peek_Mask(int mask_type)
{
	if ((Masks == nullptr) || (mask_type < 0) || (mask_type >= TERRAIN_MASK_COUNT)) {
		return nullptr;
	}
	return &Masks[mask_type];
}


bool TerrainTextureSystem::Update_Water_Distance(float threshold,float max_distance)
{
	if (Masks == nullptr) {
		return false;
	}

	return Masks[TERRAIN_MASK_WATER_DISTANCE].Build_Distance_Field(
				Masks[TERRAIN_MASK_RIVER],threshold,max_distance);
}


/***********************************************************************************************
 * Layers                                                                                      *
 *=============================================================================================*/

int TerrainTextureSystem::Define_Layer(const char * name,int kind,const TerrainLayerRuleClass & rule,
													const char * texture_name,float meters_per_tile,int surface_type)
{
	if (Layers.Count() >= MAX_LAYERS) {
		WWDEBUG_SAY(("TerrainTextureSystem: layer '%s' refused; %d is the limit.\r\n",
						 (name != nullptr) ? name : "?",(int)MAX_LAYERS));
		return -1;
	}

	TerrainLayerClass * layer = new TerrainLayerClass;
	layer->Name = (name != nullptr) ? name : "";
	layer->Kind = kind;
	layer->Rule = rule;

	//	A layer with no texture name has a rule and no content.  That is not a broken layer: a
	//	check runs with no asset manager to load one from, and a generator decides where the road
	//	goes long before it decides what the road looks like.
	if (texture_name != nullptr) {
		layer->Material = new TerrainMaterialClass;
		layer->Material->Set_Texture(texture_name);
		layer->Material->Set_Meters_Per_Tile(meters_per_tile);
		layer->Material->Set_Surface_Type(surface_type);
	}

	int index = Layers.Count();
	Layers.Add(layer);
	return index;
}


void TerrainTextureSystem::Clear_Layers(void)
{
	for (int index = 0; index < Layers.Count(); index++) {
		delete Layers[index];
	}
	Layers.Delete_All();
}


int TerrainTextureSystem::Get_Layer_Count(void)
{
	return Layers.Count();
}


TerrainLayerClass * TerrainTextureSystem::Peek_Layer(int index)
{
	if ((index < 0) || (index >= Layers.Count())) {
		return nullptr;
	}
	return Layers[index];
}


int TerrainTextureSystem::Find_Layer(const char * name)
{
	if (name == nullptr) {
		return -1;
	}

	for (int index = 0; index < Layers.Count(); index++) {
		if (Layers[index]->Name == name) {
			return index;
		}
	}
	return -1;
}


void TerrainTextureSystem::Define_Default_Layers(void)
{
	Clear_Layers();

	//	Ground.  No rule, so it is everywhere, which is what being the ground means.
	{
		TerrainLayerRuleClass rule;
		Define_Layer("ground",TERRAIN_LAYER_BASE,rule);
	}

	//	Macro variation: the same ground, broken up over tens of metres so that a plain is not
	//	one flat colour.  The hash is the whole rule -- there is nothing about the shape of the
	//	land that says where this goes, only that it should not be uniform.
	{
		TerrainLayerRuleClass rule;
		rule.Set_Noise(1.0f,0.03f,7);
		rule.Strength = 0.5f;
		Define_Layer("macro",TERRAIN_LAYER_MACRO,rule);
	}

	//	Rock on anything steeper than about twenty degrees, fading in over the ten degrees below
	//	that so there is no line around every hill.
	{
		TerrainLayerRuleClass rule;
		rule.Set_Slope_Range(DEG_TO_RADF(20.0f),DEG_TO_RADF(90.0f),DEG_TO_RADF(10.0f));
		Define_Layer("rock",TERRAIN_LAYER_CLIFF,rule);
	}

	//	Cliff face: steeper still, and convex, which is what separates the lip of a cliff from
	//	the scree at the bottom of one.  Slope alone cannot tell those apart.
	{
		TerrainLayerRuleClass rule;
		rule.Set_Slope_Range(DEG_TO_RADF(45.0f),DEG_TO_RADF(90.0f),DEG_TO_RADF(10.0f));
		rule.Set_Curvature_Range(0.05f,1000.0f,0.04f);
		Define_Layer("cliff",TERRAIN_LAYER_CLIFF,rule);
	}

	//	Wet ground within a few metres of water.
	{
		TerrainLayerRuleClass rule;
		rule.Set_Mask_Range(TERRAIN_MASK_WATER_DISTANCE,0.0f,4.0f,4.0f);
		Define_Layer("shore",TERRAIN_LAYER_SHORELINE,rule);
	}

	//	Road, wherever the road mask says so, and over everything else there: a road that let the
	//	rock layer show through would be a road with a hill drawn on it.
	{
		TerrainLayerRuleClass rule;
		rule.Set_Mask_Range(TERRAIN_MASK_ROAD,0.5f,1.0f,0.3f);
		rule.Exclusive = true;
		Define_Layer("road",TERRAIN_LAYER_ROAD,rule);
	}
}


/***********************************************************************************************
 * Evaluation                                                                                  *
 *=============================================================================================*/

float TerrainTextureSystem::Evaluate_Window(float value,float min_value,float max_value,float feather)
{
	//	Minimum above maximum means the test is off.
	if (min_value > max_value) {
		return 1.0f;
	}

	if ((value >= min_value) && (value <= max_value)) {
		return 1.0f;
	}

	if (feather <= 0.0f) {
		return 0.0f;
	}

	float distance = (value < min_value) ? (min_value - value) : (value - max_value);
	if (distance >= feather) {
		return 0.0f;
	}

	float t = 1.0f - (distance / feather);
	return t * t * (3.0f - 2.0f * t);
}


float TerrainTextureSystem::Evaluate_Rule(const TerrainLayerRuleClass & rule,int ix,int iy)
{
	HeightfieldClass * field = WorldTerrainSystem::Peek_Heightfield();
	if ((field == nullptr) || !field->Is_Valid()) {
		return 0.0f;
	}

	float weight = rule.Strength;

	//	Height, in world coordinates, so that a rule written for a level means the same thing
	//	whatever the field's origin happens to be.
	if (rule.MinHeight <= rule.MaxHeight) {
		float height = field->Get_Origin().Z + field->Get_Height(ix,iy);
		weight *= Evaluate_Window(height,rule.MinHeight,rule.MaxHeight,rule.HeightFeather);
	}

	//	Slope, from the same vertex normal the renderer lights with, so that what a rule calls
	//	steep and what the ground looks like agree.
	if ((weight > 0.0f) && (rule.MinSlope <= rule.MaxSlope)) {
		Vector3 normal = field->Compute_Vertex_Normal(ix,iy);
		float slope = WWMath::Acos(WWMath::Clamp(normal.Z,-1.0f,1.0f));
		weight *= Evaluate_Window(slope,rule.MinSlope,rule.MaxSlope,rule.SlopeFeather);
	}

	if ((weight > 0.0f) && (rule.MinCurvature <= rule.MaxCurvature)) {
		float curvature = field->Compute_Curvature(ix,iy);
		weight *= Evaluate_Window(curvature,rule.MinCurvature,rule.MaxCurvature,rule.CurvatureFeather);
	}

	if ((weight > 0.0f) && (rule.MaskType != TERRAIN_MASK_NONE) && (rule.MinMask <= rule.MaxMask)) {

		TerrainMaskClass * mask = Peek_Mask(rule.MaskType);
		if (mask == nullptr) {
			//	A rule that asks about a mask that does not exist gets nothing, rather than
			//	getting everything: a shoreline with no water in the level is not a beach.
			return 0.0f;
		}

		weight *= Evaluate_Window(mask->Get(ix,iy),rule.MinMask,rule.MaxMask,rule.MaskFeather);
	}

	//	Variation last, so that it breaks up a layer that already applies rather than putting it
	//	somewhere its rule said it should not be.
	if ((weight > 0.0f) && (rule.NoiseAmount > 0.0f)) {

		float frequency = (rule.NoiseFrequency > 0.0f) ? rule.NoiseFrequency : 1.0f;
		float fx = (float)ix * frequency;
		float fy = (float)iy * frequency;

		int lx = WWMath::Float_To_Int_Floor(fx);
		int ly = WWMath::Float_To_Int_Floor(fy);
		float u = fx - (float)lx;
		float v = fy - (float)ly;
		u = u * u * (3.0f - 2.0f * u);
		v = v * v * (3.0f - 2.0f * v);

		float h00 = TerrainMaskClass::Hash_Value(lx,  ly,  rule.NoiseSeed);
		float h10 = TerrainMaskClass::Hash_Value(lx+1,ly,  rule.NoiseSeed);
		float h01 = TerrainMaskClass::Hash_Value(lx,  ly+1,rule.NoiseSeed);
		float h11 = TerrainMaskClass::Hash_Value(lx+1,ly+1,rule.NoiseSeed);

		float lower = h00 + (h10 - h00) * u;
		float upper = h01 + (h11 - h01) * u;
		float noise = lower + (upper - lower) * v;

		float amount = WWMath::Clamp(rule.NoiseAmount,0.0f,1.0f);
		weight *= (1.0f - amount) + (amount * noise);
	}

	return WWMath::Clamp(weight,0.0f,1.0f);
}


int TerrainTextureSystem::Compute_Weights(int ix,int iy,float * weights_out,int max_weights)
{
	if ((weights_out == nullptr) || (max_weights <= 0)) {
		return 0;
	}

	int count = Layers.Count();
	if (count > max_weights) {
		count = max_weights;
	}

	float strongest_exclusive = 0.0f;

	for (int index = 0; index < count; index++) {

		TerrainLayerClass * layer = Layers[index];

		if (layer->Kind == TERRAIN_LAYER_BASE) {
			//	The base is always fully there.  Something has to be underneath, and the alternative
			//	-- a base whose weight falls where other layers are strong -- is a hole in the world
			//	wherever two layers happen to be weak at once.
			weights_out[index] = 1.0f;
			continue;
		}

		weights_out[index] = Evaluate_Rule(layer->Rule,ix,iy);

		if (layer->Rule.Exclusive && (weights_out[index] > strongest_exclusive)) {
			strongest_exclusive = weights_out[index];
		}
	}

	//	An exclusive layer pushes the others aside in proportion to how strongly it applies, so
	//	a road at full strength is a road and a road fading out at its edge lets what is beside
	//	it come back.  The base is exempt for the same reason it is always one.
	if (strongest_exclusive > 0.0f) {
		for (int index = 0; index < count; index++) {
			TerrainLayerClass * layer = Layers[index];
			if ((layer->Kind == TERRAIN_LAYER_BASE) || layer->Rule.Exclusive) {
				continue;
			}
			weights_out[index] *= (1.0f - strongest_exclusive);
		}
	}

	return count;
}


int TerrainTextureSystem::Get_Dominant_Layer(int ix,int iy)
{
	if (Layers.Count() == 0) {
		return -1;
	}

	float weights[MAX_LAYERS];
	int count = Compute_Weights(ix,iy,weights,MAX_LAYERS);

	//	Three kinds are not candidates.  The base is what is there when nothing else is, so it is
	//	the fallback rather than a contender -- it always scores one and would always win.  Detail
	//	and macro variation are how the base is drawn rather than what the ground is: they apply
	//	almost everywhere by design, and answering "macro variation" to "what am I standing on"
	//	is worse than useless to a footstep sound or a decal.
	int best = -1;
	float best_weight = 0.0f;

	for (int index = 0; index < count; index++) {

		int kind = Layers[index]->Kind;

		if (kind == TERRAIN_LAYER_BASE) {
			if (best == -1) {
				best = index;
			}
			continue;
		}

		if ((kind == TERRAIN_LAYER_DETAIL) || (kind == TERRAIN_LAYER_MACRO)) {
			continue;
		}

		if (weights[index] > best_weight) {
			best_weight = weights[index];
			best = index;
		}
	}

	return (best != -1) ? best : 0;
}


bool TerrainTextureSystem::Get_Dominant_Layer_At(float x,float y,int * layer_out)
{
	if (layer_out == nullptr) {
		return false;
	}

	HeightfieldClass * field = WorldTerrainSystem::Peek_Heightfield();
	if ((field == nullptr) || !field->Is_Valid()) {
		return false;
	}

	float fx = (x - field->Get_Origin().X) / field->Get_Cell_Size();
	float fy = (y - field->Get_Origin().Y) / field->Get_Cell_Size();

	if ((fx < 0.0f) || (fy < 0.0f) ||
		 (fx > (float)(field->Get_Vertex_Count_X() - 1)) ||
		 (fy > (float)(field->Get_Vertex_Count_Y() - 1))) {
		return false;
	}

	//	Nearest vertex, not an interpolation.  "What is the ground made of here" has an answer
	//	that is one of a list; halfway between rock and grass is not a third material.
	int ix = WWMath::Float_To_Int_Floor(fx + 0.5f);
	int iy = WWMath::Float_To_Int_Floor(fy + 0.5f);

	int layer = Get_Dominant_Layer(ix,iy);
	if (layer < 0) {
		return false;
	}

	*layer_out = layer;
	return true;
}


bool TerrainTextureSystem::Get_Surface_Type_At(float x,float y,int * surface_type_out)
{
	if (surface_type_out == nullptr) {
		return false;
	}

	int layer_index = 0;
	if (!Get_Dominant_Layer_At(x,y,&layer_index)) {
		return false;
	}

	TerrainLayerClass * layer = Peek_Layer(layer_index);
	if ((layer == nullptr) || (layer->Material == nullptr)) {
		return false;
	}

	*surface_type_out = layer->Material->Get_Surface_Type();
	return true;
}


/***********************************************************************************************
 * Building                                                                                    *
 *=============================================================================================*/

bool TerrainTextureSystem::Build_Patch_Materials(RenegadeTerrainPatchClass * model,int px,int py)
{
	if ((model == nullptr) || (Layers.Count() == 0)) {
		return false;
	}

	HeightfieldClass * field = WorldTerrainSystem::Peek_Heightfield();
	if ((field == nullptr) || !field->Is_Valid()) {
		return false;
	}
	if ((px < 0) || (py < 0) ||
		 (px >= field->Get_Patch_Count_X()) || (py >= field->Get_Patch_Count_Y())) {
		return false;
	}

	int ix0 = px * HeightfieldClass::PATCH_CELLS;
	int iy0 = py * HeightfieldClass::PATCH_CELLS;
	int ix1 = ix0 + HeightfieldClass::PATCH_CELLS;
	int iy1 = iy0 + HeightfieldClass::PATCH_CELLS;
	if (ix1 > field->Get_Vertex_Count_X() - 1) ix1 = field->Get_Vertex_Count_X() - 1;
	if (iy1 > field->Get_Vertex_Count_Y() - 1) iy1 = field->Get_Vertex_Count_Y() - 1;

	int points_x = ix1 - ix0 + 1;
	int points_y = iy1 - iy0 + 1;
	int vertex_count = points_x * points_y;
	int layer_count = Layers.Count();

	//	Every weight for every vertex of the patch, up front.  A quad reads four vertices and the
	//	patch's quads share three quarters of theirs with a neighbour; recomputing a rule per
	//	quad would evaluate the whole table four times per vertex.
	float * weights = new float[vertex_count * layer_count];

	for (int iy = iy0; iy <= iy1; iy++) {
		for (int ix = ix0; ix <= ix1; ix++) {
			int local = ((iy - iy0) * points_x) + (ix - ix0);
			Compute_Weights(ix,iy,&weights[local * layer_count],layer_count);
		}
	}

	model->Reset_Material_Passes();

	//	The composite is stock Renegade's: the first layer with any influence on a quad draws it
	//	opaque, and each later one draws over it with its share of the running total.  That is
	//	what the level editor's brush produced, and matching it means nothing in the renderer has
	//	to learn a second way to read a terrain patch.
	int col_count = points_x - 1;

	for (int quad_y = 0; quad_y < points_y - 1; quad_y++) {
		for (int quad_x = 0; quad_x < col_count; quad_x++) {

			int quad_index = (quad_y * col_count) + quad_x;

			int v0 = (quad_y * points_x) + quad_x;
			int v1 = v0 + 1;
			int v2 = v0 + points_x + 1;
			int v3 = v0 + points_x;

			float sum0 = 0.0f;
			float sum1 = 0.0f;
			float sum2 = 0.0f;
			float sum3 = 0.0f;
			bool base_taken = false;

			for (int layer = 0; layer < layer_count; layer++) {

				float w0 = weights[v0 * layer_count + layer];
				float w1 = weights[v1 * layer_count + layer];
				float w2 = weights[v2 * layer_count + layer];
				float w3 = weights[v3 * layer_count + layer];

				if ((w0 <= 0.0f) && (w1 <= 0.0f) && (w2 <= 0.0f) && (w3 <= 0.0f)) {
					continue;
				}

				RenegadeTerrainMaterialPassClass * pass =
					model->Get_Material_Pass(layer,Layers[layer]->Material);

				sum0 += w0;
				sum1 += w1;
				sum2 += w2;
				sum3 += w3;

				if (base_taken) {

					pass->QuadList[RenegadeTerrainMaterialPassClass::PASS_ALPHA].Add(quad_index);

					pass->VertexAlpha[v0] = (sum0 > 0.0f) ? (w0 / sum0) : 0.0f;
					pass->VertexAlpha[v1] = (sum1 > 0.0f) ? (w1 / sum1) : 0.0f;
					pass->VertexAlpha[v2] = (sum2 > 0.0f) ? (w2 / sum2) : 0.0f;
					pass->VertexAlpha[v3] = (sum3 > 0.0f) ? (w3 / sum3) : 0.0f;

				} else {

					pass->QuadList[RenegadeTerrainMaterialPassClass::PASS_BASE].Add(quad_index);
					base_taken = true;
				}
			}
		}
	}

	delete [] weights;

	model->Update_Vertex_Render_Lists();
	model->Update_UVs();
	return true;
}


bool TerrainTextureSystem::Build_All_Patch_Materials(void)
{
	HeightfieldClass * field = WorldTerrainSystem::Peek_Heightfield();
	if ((field == nullptr) || !field->Is_Valid()) {
		return false;
	}
	if (!WorldTerrainSystem::Has_Collision()) {
		return false;
	}

	bool built_everything = true;

	for (int py = 0; py < field->Get_Patch_Count_Y(); py++) {
		for (int px = 0; px < field->Get_Patch_Count_X(); px++) {

			RenegadeTerrainPatchClass * model = WorldTerrainSystem::Peek_Patch_Model(px,py);
			if (model == nullptr) {
				built_everything = false;
				continue;
			}

			if (!Build_Patch_Materials(model,px,py)) {
				built_everything = false;
			}
		}
	}

	return built_everything;
}
