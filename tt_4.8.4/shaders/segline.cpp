#include "General.h"
#include "Segline.h"
void SegmentedLineClass::Set_Disable_Sorting(int onoff)
{
	LineRenderer.Set_Disable_Sorting(onoff);
}
void SegmentedLineClass::Set_Merge_Intersections(int onoff)
{
	LineRenderer.Set_Merge_Intersections(onoff);
}
void SegmentedLineClass::Set_Texture_Mapping_Mode(SegLineRendererClass::TextureMapMode mode)
{
	LineRenderer.Set_Texture_Mapping_Mode(mode);
}
void SegmentedLineClass::Set_Width(float width)
{
	LineRenderer.Set_Width(width);
}
void SegmentedLineClass::Set_Texture(TextureClass *texture)
{
	LineRenderer.Set_Texture(texture);
}
void SegmentedLineClass::Set_Shader(ShaderClass shader)
{
	LineRenderer.Set_Shader(shader);
}
RENEGADE_FUNCTION
void SegmentedLineClass::Set_Points(unsigned int num_points, Vector3 *locs)
AT1(0x00786D90);
SegmentedLineClass::SegmentedLineClass() : RenderObjClass(), MaxSubdivisionLevels(0), NormalizedScreenArea(0)
{
}
RenderObjClass *SegmentedLineClass::Clone() const
{
	return new SegmentedLineClass(*this);
}
int SegmentedLineClass::Get_Num_Polys() const
{
	return 2 * (PointLocations.Count() - 1) * (1 << LineRenderer.SubdivisionLevel);
}
RENEGADE_FUNCTION
bool SegmentedLineClass::Cast_Ray(RayCollisionTestClass &raytest)
AT1(0x00787B40);
RENEGADE_FUNCTION
void SegmentedLineClass::Get_Obj_Space_Bounding_Sphere(SphereClass &sphere) const
AT1(0x007874E0);
RENEGADE_FUNCTION
void SegmentedLineClass::Get_Obj_Space_Bounding_Box(AABoxClass &box) const
AT1(0x00787540);
RENEGADE_FUNCTION
void SegmentedLineClass::Prepare_LOD(CameraClass &camera)
AT1(0x00787930);
void SegmentedLineClass::Increment_LOD()
{
	unsigned int lod = LineRenderer.SubdivisionLevel + 1;
	if (lod >= MaxSubdivisionLevels)
	{
		LineRenderer.SubdivisionLevel = MaxSubdivisionLevels;
	}
	else
	{
		LineRenderer.SubdivisionLevel = lod;
	}
}
void SegmentedLineClass::Decrement_LOD()
{
	int lod = LineRenderer.SubdivisionLevel;
	if (lod)
	{
		LineRenderer.SubdivisionLevel = lod - 1;
	}
}
RENEGADE_FUNCTION
float SegmentedLineClass::Get_Cost() const
AT1(0x007879E0)
RENEGADE_FUNCTION
float SegmentedLineClass::Get_Value() const
AT1(0x007879F0)
RENEGADE_FUNCTION
float SegmentedLineClass::Get_Post_Increment_Value() const
AT1(0x00787A50)
void SegmentedLineClass::Set_LOD_Level(int lod)
{
	int l = ((lod < 0) - 1) & lod;
	int l2 = MaxSubdivisionLevels;
	if (l < l2)
	{
		LineRenderer.SubdivisionLevel = l;
	}
	else
	{
		LineRenderer.SubdivisionLevel = l2;
	}
}
int SegmentedLineClass::Get_LOD_Level() const
{
	return LineRenderer.SubdivisionLevel;
}
int SegmentedLineClass::Get_LOD_Count() const
{
	return MaxSubdivisionLevels;
}
SegmentedLineClass::~SegmentedLineClass()
{
}
void SegmentedLineClass::Set_Color(const Vector3 &color)
{
	LineRenderer.Set_Color(color);
}
RENEGADE_FUNCTION
void SegmentedLineClass::Render_Seg_Line(RenderInfoClass &rinfo)
AT1(0x00787AF0);
RENEGADE_FUNCTION
void SegmentedLineClass::Render(RenderInfoClass &rinfo)
AT1(0x00787440);
SegmentedLineClass::SegmentedLineClass(const SegmentedLineClass &src) : MaxSubdivisionLevels(src.MaxSubdivisionLevels), NormalizedScreenArea(src.NormalizedScreenArea), PointLocations(src.PointLocations), LineRenderer(src.LineRenderer)
{
}
