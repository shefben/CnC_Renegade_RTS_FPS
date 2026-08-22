#ifndef TT_INCLUDE_SEGLINE_H
#define TT_INCLUDE_SEGLINE_H
#include "RenderObjClass.h"
#include "seglinerenderer.h"
class SegmentedLineClass : public RenderObjClass
{
public:
	SegmentedLineClass();
	SegmentedLineClass(const SegmentedLineClass & src);
	SegmentedLineClass & SegmentedLineClass::operator = (const SegmentedLineClass &that);
	virtual ~SegmentedLineClass();
	void Reset_Line();
	void Set_Points(unsigned int num_points, Vector3 *locs);
	int Get_Num_Points();
	void Set_Point_Location(unsigned int point_idx, const Vector3 &location);
	void Get_Point_Location(unsigned int point_idx, Vector3 &loc);
	void Add_Point(const Vector3 & location);
	void Delete_Point(unsigned int point_idx);
	TextureClass *Get_Texture();
	ShaderClass Get_Shader();
	float Get_Width();
	void Get_Color(Vector3 &color);
	float Get_Opacity();
	float Get_Noise_Amplitude();
	float Get_Merge_Abort_Factor();
	unsigned int Get_Subdivision_Levels();
	SegLineRendererClass::TextureMapMode Get_Texture_Mapping_Mode();
	float Get_Texture_Tile_Factor();
	Vector2 Get_UV_Offset_Rate();
	int Is_Merge_Intersections();
	int Is_Freeze_Random();
	int Is_Sorting_Disabled();
	int Are_End_Caps_Enabled();
	void Set_Texture(TextureClass *texture);
	void Set_Shader(ShaderClass shader);
	void Set_Width(float width);
	void Set_Color(const Vector3 &color);
	void Set_Opacity(float opacity);
	void Set_Noise_Amplitude(float amplitude);
	void Set_Merge_Abort_Factor(float factor);
	void Set_Subdivision_Levels(unsigned int levels);
	void Set_Texture_Mapping_Mode(SegLineRendererClass::TextureMapMode mode);
	void Set_Texture_Tile_Factor(float factor);
	void Set_UV_Offset_Rate(const Vector2 &rate);
	void Set_Merge_Intersections(int onoff);
	void Set_Freeze_Random(int onoff);
	void Set_Disable_Sorting(int onoff);
	void Set_End_Caps(int onoff);
	virtual RenderObjClass *Clone(void) const;		
	virtual int Class_ID(void) const
	{
		return CLASSID_SEGLINE;
	}
	virtual int Get_Num_Polys(void) const;
	virtual void Render(RenderInfoClass & rinfo);
	virtual void Get_Obj_Space_Bounding_Sphere(SphereClass & sphere) const;
	virtual void Get_Obj_Space_Bounding_Box(AABoxClass & box) const;
	virtual void Prepare_LOD(CameraClass &camera);
	virtual void Increment_LOD(void);
	virtual void Decrement_LOD(void);
	virtual float Get_Cost(void) const;
	virtual float Get_Value(void) const;
	virtual float Get_Post_Increment_Value(void) const;
	virtual void Set_LOD_Level(int lod);
	virtual int Get_LOD_Level(void) const;
	virtual int Get_LOD_Count(void) const;
	virtual bool Cast_Ray(RayCollisionTestClass & raytest);
protected:
	void Render_Seg_Line(RenderInfoClass & rinfo);
private:
	unsigned int MaxSubdivisionLevels;
	float NormalizedScreenArea;
	SimpleDynVecClass<Vector3> PointLocations;
	SegLineRendererClass LineRenderer;
};

#endif