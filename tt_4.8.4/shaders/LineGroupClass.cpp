#include "General.h"
#include "LineGroupClass.h"

LineGroupClass::LineGroupClass() : StartLineLoc(0), EndLineLoc(0), LineDiffuse(0), TailDiffuse(0), ALT(0), LineSize(0), LineUCoord(0), LineCount(0), Texture(0), Flags(0), DefaultLineSize(0), DefaultLineColor(1,1,1), DefaultLineAlpha(1), DefaultTailDiffuse(0,0,0,0), LineMode(TETRAHEDRON), Shader(ShaderClass::_PresetAdditiveSpriteShader)
{
}
LineGroupClass::~LineGroupClass()
{
	REF_PTR_RELEASE(StartLineLoc);
	REF_PTR_RELEASE(EndLineLoc);
	REF_PTR_RELEASE(LineDiffuse);
	REF_PTR_RELEASE(TailDiffuse);
	REF_PTR_RELEASE(ALT);
	REF_PTR_RELEASE(LineSize);
	REF_PTR_RELEASE(LineUCoord);
}
void LineGroupClass::Set_Flag(FlagsType flag,bool onoff)
{
	if (onoff)
	{
		Flags |= 1 << flag;
	}
	else
	{
		Flags &= ~(1 << flag);
	}
}
void LineGroupClass::Set_Texture(TextureClass* texture)
{
	REF_PTR_SET(Texture,texture);
}
void LineGroupClass::Set_Shader(const ShaderClass &shader)
{
	Shader = shader;
}
void LineGroupClass::Set_Line_Mode(LineModeType mode)
{
	LineMode = mode;
}

RENEGADE_FUNCTION
void LineGroupClass::Render(RenderInfoClass &rinfo)
AT1(0x005ADB80)

RENEGADE_FUNCTION
void LineGroupClass::Set_Arrays(ShareBufferClass<Vector3> *startlocs, ShareBufferClass<Vector3> *endlocs, ShareBufferClass<Vector4> *diffuse, ShareBufferClass<Vector4> *taildiffuse, ShareBufferClass<unsigned int> *alt, ShareBufferClass<float> *sizes, ShareBufferClass<float> *ucoords, int active_line_count)
AT1(0x005AD8B0);

void LineGroupClass::Set_Tail_Diffuse(const Vector4 &tdiffuse)
{
	DefaultTailDiffuse = tdiffuse;
}

void LineGroupClass::Set_Line_UCoord(float ucoord)
{
	DefaultLineUCoord = ucoord;
}

void LineGroupClass::Set_Line_Size(float size)
{
	DefaultLineSize = size;
}

void LineGroupClass::Set_Line_Color(const Vector3 &color)
{
	DefaultLineColor = color;
}

void LineGroupClass::Set_Line_Alpha(float alpha)
{
	DefaultLineAlpha = alpha;
}
