#ifndef TT_INCLUDE__POINTGROUPCLASS_H
#define TT_INCLUDE__POINTGROUPCLASS_H



#include "engine_3dre.h"
#include "engine_vector.h"
#include "texture.h"
class Vector3;
class Vector4;
class TextureClass;
class RenderInfoClass;



class PointGroupClass
{
public:
	enum PointModeEnum {
		TRIS,			// each point is a triangle
		QUADS,		// each point is a quad formed out of two triangles
		SCREENSPACE	// each point is a tri placed to affect certain pixels (should be used with 2D camera)
	};
	enum FlagsType {
		TRANSFORM,	// transform points w. modelview matrix (worldspace points)
	};
	PointGroupClass(void);
	virtual ~PointGroupClass(void);
	PointGroupClass & operator = (const PointGroupClass & that);
	void						Set_Arrays(ShareBufferClass<Vector3> *locs,
									ShareBufferClass<Vector4> *diffuse = NULL,																		
									ShareBufferClass<unsigned int> *apt = NULL,
									ShareBufferClass<float> *sizes = NULL,
									ShareBufferClass<unsigned char> *orientations = NULL,
									ShareBufferClass<unsigned char> *frames = NULL,
									int active_point_count = -1,
									float vpxmin = 0.0f, float vpymin = 0.0f,
									float vpxmax = 0.0f, float vpymax = 0.0f);
	void						Set_Point_Size(float size)
	{
		DefaultPointSize = size;
	}
	float						Get_Point_Size(void);
	void						Set_Point_Color(Vector3 color)
	{
		DefaultPointColor.X = color.X;
		DefaultPointColor.Y = color.Y;
		DefaultPointColor.Z = color.Z;
	}
	Vector3				Get_Point_Color(void);
	void						Set_Point_Alpha(float alpha)
	{
		DefaultPointAlpha = alpha;
	}
	float						Get_Point_Alpha(void);
	void						Set_Point_Orientation(unsigned char orientation)
	{
		DefaultPointOrientation = orientation;
	}
	unsigned char			Get_Point_Orientation(void);
	void						Set_Point_Frame(unsigned char frame)
	{
		DefaultPointFrame = frame;
	}
	unsigned char			Get_Point_Frame(void);
	void						Set_Point_Mode(PointModeEnum mode);
	PointModeEnum			Get_Point_Mode(void);
	void						Set_Flag(FlagsType flag, bool onoff);
	int						Get_Flag(FlagsType flag);
	void						Set_Texture(TextureClass* texture);
	TextureClass * 		Get_Texture(void)
	{
		Texture->Add_Ref();
		return Texture;
	}
	TextureClass * 		Peek_Texture(void)
	{
		return Texture;
	}
	void						Set_Shader(ShaderClass shader);
	ShaderClass				Get_Shader(void)
	{
		return Shader;
	}
	unsigned char			Get_Frame_Row_Column_Count_Log2(void)
	{
		return FrameRowColumnCountLog2;
	}
	void						Set_Frame_Row_Column_Count_Log2(unsigned char frccl2);
	int						Get_Polygon_Count(void);
	void						Render(RenderInfoClass &rinfo);
protected:
	void						Update_Arrays(Vector3 *point_loc,
									Vector4 *point_diffuse,									
									float *point_size,
									unsigned char *point_orientation,
									unsigned char *point_frame, 
									int active_points,
									int total_points, 
									int &vnum, 
									int &pnum);
	ShareBufferClass<Vector3> *			PointLoc;	// World/cameraspace point locs
	ShareBufferClass<Vector4> *			PointDiffuse; // (NULL if not used) RGBA values
	ShareBufferClass<unsigned int> *		APT;			// (NULL if not used) active point table
	ShareBufferClass<float> *				PointSize;	// (NULL if not used) size override table
	ShareBufferClass<unsigned char> *	PointOrientation; // (NULL if not used) orientation indices
	ShareBufferClass<unsigned char> *	PointFrame; // (NULL if not used) frame indices
	int											PointCount;	// Active (if APT) or total point count
	unsigned char			FrameRowColumnCountLog2;		// MUST be equal or lesser than 4
	TextureClass*			Texture;
	ShaderClass				Shader;					// (default created in CTor)
	PointModeEnum			PointMode;					// are points tris or quads?
	unsigned int			Flags;						// operation control flags
	float						DefaultPointSize;			// point size (size array overrides if present)
	Vector3					DefaultPointColor;		// point color (color array overrides if present)
	float						DefaultPointAlpha;		// point alpha (alpha array overrides if present)	
	unsigned char			DefaultPointOrientation;// point orientation (orientation array overrides if present)
	unsigned char			DefaultPointFrame;		// point texture frame (frame array overrides if present)
	float						VPXMin;
	float						VPYMin;
	float						VPXMax;
	float						VPYMax;
public:
	static void				_Init(void);
	static void				_Shutdown(void);
private:
	static REF_DECL1(PointMaterial, VertexMaterialClass*);
};

#endif