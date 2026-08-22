#ifndef TT_INCLUDE__LINEGROUPCLASS_H
#define TT_INCLUDE__LINEGROUPCLASS_H
#include "engine_3dre.h"
#include "texture.h"
class RenderInfoClass;
class LineGroupClass {
public:
	enum FlagsType {
		TRANSFORM,	// transform points w. modelview matrix (worldspace points)
	};
	enum LineModeType {
		TETRAHEDRON,
		PRISM
	};
	LineGroupClass(void);
	virtual ~LineGroupClass(void);	
	void						Set_Arrays(
									ShareBufferClass<Vector3> *startlocs,
									ShareBufferClass<Vector3> *endlocs,
									ShareBufferClass<Vector4> *diffuse = NULL,
									ShareBufferClass<Vector4> *taildiffuse = NULL,
									ShareBufferClass<unsigned int> *alt = NULL,
									ShareBufferClass<float> *sizes = NULL,
									ShareBufferClass<float> *ucoords = NULL,
									int active_line_count = -1
									);
	void						Set_Line_Size(float size);
	float						Get_Line_Size(void);
	void						Set_Line_Color(const Vector3 &color);
	Vector3					Get_Line_Color(void);
	void						Set_Tail_Diffuse(const Vector4 &tdiffuse);
	Vector4					Get_Tail_Diffuse(void);
	void						Set_Line_Alpha(float alpha);
	float						Get_Line_Alpha(void);
	void						Set_Line_UCoord(float ucoord);
	float						Get_Line_UCoord(void);
	void						Set_Flag(FlagsType flag, bool on);
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
	void						Set_Shader(const ShaderClass &shader);
	ShaderClass				Get_Shader(void)
	{
		return Shader;
	}
	void						Set_Line_Mode(LineModeType linemode);
	LineModeType			Get_Line_Mode(void);
	int						Get_Polygon_Count(void);
	void						Render(RenderInfoClass &rinfo);
protected:
	ShareBufferClass<Vector3> *			StartLineLoc;	// World/cameraspace point locs
	ShareBufferClass<Vector3> *			EndLineLoc;	// World/cameraspace point locs
	ShareBufferClass<Vector4> *			LineDiffuse; // (NULL if not used) RGBA values
	ShareBufferClass<Vector4> *			TailDiffuse; // (NULL if not used) RGBA values
	ShareBufferClass<unsigned int> *		ALT;			// (NULL if not used) active line table
	ShareBufferClass<float> *				LineSize;	// (NULL if not used) size override table	
	ShareBufferClass<float> *				LineUCoord; // (NULL if not used) U coordinates
	int											LineCount;	// Active (if ALT) or total point count
	TextureClass*			Texture;
	ShaderClass				Shader;					// (default created in CTor)
	unsigned int			Flags;						// operation control flags
	float						DefaultLineSize;			// Line size (size array overrides if present)
	Vector3					DefaultLineColor;		// Line color (color array overrides if present)
	float						DefaultLineAlpha;		// Line alpha (alpha array overrides if present)	
	float						DefaultLineUCoord;	// Line texture Ucoord (ucoord array overrides if present)
	Vector4					DefaultTailDiffuse;	// Tail diffuse RGBA
	LineModeType			LineMode;
};

#endif