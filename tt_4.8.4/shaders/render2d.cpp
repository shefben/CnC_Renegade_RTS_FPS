#include "general.h"

#include "render2d.h"
#include "WW3D.h"
#include "WW3DAssetManager.h"
#include "vertexbuffer.h"
#include "indexbuffer.h"
Render2DSentenceClass *CreateRender2DSentenceClass(unsigned int font)
{
	Render2DSentenceClass *render2D = new Render2DSentenceClass();
	FontCharsClass *f = Fonts[font];
	render2D->Set_Font(f);
	return render2D;
}

void Render2DSentenceDrawSentence(Render2DSentenceClass *r,const wchar_t *sentence,Vector2 *position,unsigned long color)
{
	r->Reset();
	r->Build_Sentence(sentence);
	r->Set_Location(*position);
	r->Draw_Sentence(color);
	r->Render();
}

Render2DTextClass *CreateRender2DTextClass(const char *texture)
{
	Font3DInstanceClass *font = WW3DAssetManager::TheInstance->Get_Font3DInstance(texture);
	Render2DTextClass *render2D = new Render2DTextClass(font);
	font->Release_Ref();
	render2D->Set_Coordinate_Range(Render2DClass::Get_Screen_Resolution());
	return render2D;
}

void Render2DTextClass::Reset()
{
	Render2DClass::Reset();
	Cursor.X = Location.X;
	Cursor.Y = Location.Y;
	WrapWidth = 0;
	DrawExtents.Left = 0;
	DrawExtents.Top = 0;
	DrawExtents.Right = 0;
	DrawExtents.Bottom = 0;
	TotalExtents.Left = 0;
	TotalExtents.Top = 0;
	TotalExtents.Right = 0;
	TotalExtents.Bottom = 0;
	ClipRect.Left = 0;
	ClipRect.Right = 0;
	ClipRect.Top = 0;
	ClipRect.Bottom = 0;
	IsClippedEnabled = false;
}

Render2DTextClass::~Render2DTextClass()
{
	if (Font)
	{
		Font->Release_Ref();
	}
	Font = 0;
}

RENEGADE_FUNCTION
void Render2DTextClass::Set_Font(Font3DInstanceClass *font)
AT1(0x00536230);

RENEGADE_FUNCTION
void Render2DTextClass::Draw_Text(const WCHAR * text,unsigned long color)
AT1(0x00536720);

RENEGADE_FUNCTION
void Render2DTextClass::Draw_Block(const RectClass& screen,unsigned long color)
AT1(0x00536890);

RENEGADE_FUNCTION
void Render2DTextClass::Draw_Char(wchar_t ch,unsigned long color)
AT1(0x00536350);

Vector2 Render2DTextClass::Get_Text_Extents(const WCHAR *text)
{
	Vector2 extents;
	extents.X = 0;
	extents.Y = Font->Char_Height();
	while (*text)
	{
		extents.Y += Font->Char_Spacing(*text);
	}
	return extents;
}

void Render2DTextClass::Draw_Text(char const * text, unsigned long color)
{
	WideStringClass w;
	w.Convert_From(text);
	Draw_Text(w.Peek_Buffer(),color);
}

Render2DTextClass::Render2DTextClass(Font3DInstanceClass *font) : Render2DClass(0), Font(0), Location(), Cursor(), WrapWidth(0), ClipRect(), IsClippedEnabled(false)
{
	RectClass r = RectClass(-320,-240,320,240);
	Set_Coordinate_Range(r);
	Set_Font(font);
}

Render2DClass *CreateRender2DClass()
{
	Render2DClass *render2D = new Render2DClass(0);
	render2D->Set_Coordinate_Range(Render2DClass::Get_Screen_Resolution());
	return render2D;
}

#ifdef SHADERS_EXPORTS
void Render2DClass::Render()
{
	if (Indices.Count() && !IsHidden)
	{
		D3DVIEWPORT9 vp;
		vp.X = 0;
		vp.Y = 0;
		vp.Width = DX8Wrapper::ResolutionWidth;
		vp.Height = DX8Wrapper::ResolutionHeight;
		vp.MinZ = 0;
		vp.MaxZ = 1.0;
		DX8Wrapper::D3DDevice->SetViewport(&vp);
		if (ShaderClass::ShaderDirty || DX8Wrapper::render_state.shader != Shader)
		{
			DX8Wrapper::render_state.shader = Shader;
			(DX8Wrapper::render_state_changed) |= SHADER_CHANGED;
		}
		if (Texture != DX8Wrapper::render_state.Textures[0])
		{
			if (Texture)
			{
				Texture->Add_Ref();
			}
			if (DX8Wrapper::render_state.Textures[0])
			{
				DX8Wrapper::render_state.Textures[0]->Release_Ref();
			}
			DX8Wrapper::render_state.Textures[0] = Texture;
			DX8Wrapper::render_state_changed |= TEXTURE0_CHANGED;
		}
		VertexMaterialClass *material = VertexMaterialClass::Get_Preset(VertexMaterialClass::PRELIT_DIFFUSE);
		if (material != DX8Wrapper::render_state.material)
		{
			if (material)
			{
				material->Add_Ref();
			}
			if (DX8Wrapper::render_state.material)
			{
				DX8Wrapper::render_state.material->Release_Ref();
			}
			DX8Wrapper::render_state.material = material;
			DX8Wrapper::render_state_changed |= MATERIAL_CHANGED;
		}
		material->Release_Ref();
		if (!((DX8Wrapper::render_state_changed) & WORLD_IDENTITY))
		{
			DX8Wrapper::render_state.world.Make_Identity();
			DX8Wrapper::render_state_changed |= WORLD_IDENTITY | WORLD_CHANGED;
		}
		if (!((DX8Wrapper::render_state_changed) & VIEW_IDENTITY))
		{
			DX8Wrapper::render_state.view.Make_Identity();
			DX8Wrapper::render_state_changed |= VIEW_IDENTITY | VIEW_CHANGED;
		}

		Transforms.SetProjectionIdentity();

		DX8Wrapper::ZFar = 0;
		DX8Wrapper::ZNear = 0;
		DynamicVBAccessClass vb = DynamicVBAccessClass(2,0x252,(unsigned short)Vertices.Count());
		{
			DynamicVBAccessClass::WriteLockClass lock(&vb);
			VertexFormatXYZNDUV2 *vf = lock.Get_Formatted_Vertex_Array();
			for (int i = 0;i < Vertices.Count();i++)
			{
				vf[i].x = Vertices[i].X;
				vf[i].y = Vertices[i].Y;
				vf[i].z = ZValue;
				vf[i].diffuse = Colors[i];
				vf[i].u1 = UVCoordinates[i].X;
				vf[i].v1 = UVCoordinates[i].Y;
			}
		}
		DynamicIBAccessClass ib = DynamicIBAccessClass(2,(unsigned short)Indices.Count());
		{
			DynamicIBAccessClass::WriteLockClass lock(&ib);
			unsigned short *ind = lock.Get_Index_Array();
			for (int i = 0;i < Indices.Count();i++)
			{
				ind[i] = Indices[i];
			}
		}
		DX8Wrapper::Set_Vertex_Buffer(vb);
		DX8Wrapper::Set_Index_Buffer(ib,0);
		DX8Wrapper::Draw(4,0,(unsigned short)(Indices.Count() / 3),0,(unsigned short)Vertices.Count());
	}
}
#endif

Render2DClass::Render2DClass(TextureClass *texture) : CoordinateScale(1.0,1.0), CoordinateOffset(0,0), Texture(0), Indices(60,PreAllocatedIndices), Vertices(60,PreAllocatedVertices), UVCoordinates(60,PreAllocatedUVCoordinates), Colors(60,PreAllocatedColors), IsHidden(false), ZValue(0)
{
	Set_Texture(texture);
	Shader = Get_Default_Shader();
}

void Render2DClass::Update_Bias()
{
	BiasedCoordinateOffset.X = CoordinateOffset.X;
	BiasedCoordinateOffset.Y = CoordinateOffset.Y;
	if (WW3D::IsScreenUVBiased)
	{
		float f = (float)(-0.5 / ((ScreenResolution.Right - ScreenResolution.Left) * 0.5));
		float f2 = (float)(-0.5 / ((ScreenResolution.Bottom - ScreenResolution.Top) * -0.5));
		BiasedCoordinateOffset.X += f;
		BiasedCoordinateOffset.Y += f2;
	}
}

void Render2DClass::Reset()
{
	Vertices.Reset_Active();
	UVCoordinates.Reset_Active();
	Colors.Reset_Active();
	Indices.Reset_Active();
	Update_Bias();
}

void Render2DClass::Set_Coordinate_Range(RectClass const &r)
{
	CoordinateScale.X = (float)(2.0 / (r.Right - r.Left));
	CoordinateScale.Y = (float)(-2.0 / (r.Bottom - r.Top));
	CoordinateOffset.X = (float)((-(CoordinateScale.X * r.Left)) - 1.0);
	CoordinateOffset.Y = (float)(1.0 - (r.Top * CoordinateScale.Y));
	Update_Bias();
}

void Render2DClass::Set_Texture(char const *texture)
{
	TextureClass *t = WW3DAssetManager::TheInstance->Get_Texture(texture,TextureClass::MIP_LEVELS_1,WW3D_FORMAT_UNKNOWN,true);
	Set_Texture(t);
	if (t)
	{
		t->Release_Ref();
	}
}

void Render2DClass::Set_Texture_Init(char const *texture)
{
	TextureClass *t = WW3DAssetManager::TheInstance->Get_Texture(texture,TextureClass::MIP_LEVELS_1,WW3D_FORMAT_UNKNOWN,true);
	Set_Texture(t);
	if (t)
	{
		if (!t->Initialized)
		{
			t->Init();
		}
		t->Release_Ref();
	}
}

void Render2DClass::Set_Texture(TextureClass *texture)
{
	if (texture)
	{
		texture->Add_Ref();
	}
	if (Texture)
	{
		Texture->Release_Ref();
	}
	Texture = texture;
}

Render2DClass::~Render2DClass()
{
	if (Texture)
	{
		Texture->Release_Ref();
	}
	Texture = 0;
}

void Render2DClass::Set_Screen_Resolution(const RectClass& rect)
{
	ScreenResolution = rect;
}

ShaderClass Render2DClass::Get_Default_Shader()
{
	ShaderClass sh;
	sh = ShaderClass(0x984B7);
	return sh;
}

RENEGADE_FUNCTION
void Render2DClass::Enable_Additive(bool b)
AT1(0x00532AB0);

RENEGADE_FUNCTION
void Render2DClass::Enable_Alpha(bool b)
AT1(0x00532A70);

void Render2DClass::Enable_Texturing(bool b)
{
	if (b)
	{
		Shader.Set_Texturing(ShaderClass::TEXTURING_ENABLE);
	}
	else
	{
		Shader.Set_Texturing(ShaderClass::TEXTURING_DISABLE);
	}
}

REF_DEF1(Render2DClass::ScreenResolution, RectClass, 0x008305C0);

void Render2DClass::Force_Color(int color)
{
	for (int i = 0; i < Colors.Count(); i++)
		Colors[i] = color;
}

RectClass &Render2DClass::Get_Screen_Resolution()
{
	return ScreenResolution;
}

void Render2DClass::Convert_Vert(Vector2 & vert_out, float x_in, float y_in)
{
	vert_out.X = x_in * CoordinateScale.X + BiasedCoordinateOffset.X;
	vert_out.Y = y_in * CoordinateScale.Y + BiasedCoordinateOffset.Y;
}

void Render2DClass::Convert_Vert(Vector2 & vert_out, const Vector2 & vert_in)
{
	vert_out.X = vert_in.X * CoordinateScale.X + BiasedCoordinateOffset.X;
	vert_out.Y = vert_in.Y * CoordinateScale.Y + BiasedCoordinateOffset.Y;
}

void Render2DClass::Add_Outline(const RectClass& rect,float width,unsigned long color)
{
	RectClass r = RectClass(0,0,1.0,1.0);
	Add_Outline(rect,width,r,color);
}

RENEGADE_FUNCTION
void Render2DClass::Add_Line(const Vector2& a, const Vector2& b, float width, const RectClass& uv, unsigned long color)
AT1(0x005346A0);

void Render2DClass::Force_Alpha(float alpha)
{
	if (alpha < 0)
	{
		alpha = 0;
	}
	if (alpha > 1)
	{
		alpha = 1;
	}
	alpha *= 255;
	int alphaval = (int)alpha;
	alphaval <<= 24;
	for (int i = 0;i < Colors.Count();i++)
	{
		Colors[i] &= 0xFFFFFF;
		Colors[i] |= alphaval;
	}
}

void Render2DClass::Move(const Vector2 & a)
{
	Vector2 scaled_move = a;
	scaled_move.Scale(CoordinateScale);
	for (int i = 0;i < Vertices.Count();i++)
	{
		Vertices[i].Scale(scaled_move);
	}
}

void Render2DClass::Add_Tri(const Vector2 & v0, const Vector2 & v1, const Vector2 & v2, const Vector2 & uv0, const Vector2 & uv1, const Vector2 & uv2, unsigned long color)
{
	int startindex = Vertices.Count();
	Convert_Vert(*Vertices.Uninitialized_Add(),v0);
	Convert_Vert(*Vertices.Uninitialized_Add(),v1);
	Convert_Vert(*Vertices.Uninitialized_Add(),v2);
	Vector2 *uv;
	uv = UVCoordinates.Uninitialized_Add();
	uv->X = uv0.X;
	uv->Y = uv0.Y;
	uv = UVCoordinates.Uninitialized_Add();
	uv->X = uv1.X;
	uv->Y = uv1.Y;
	uv = UVCoordinates.Uninitialized_Add();
	uv->X = uv2.X;
	uv->Y = uv2.Y;
	*Colors.Uninitialized_Add() = color;
	*Colors.Uninitialized_Add() = color;
	*Colors.Uninitialized_Add() = color;
	*Indices.Uninitialized_Add() = (unsigned short)startindex;
	*Indices.Uninitialized_Add() = (unsigned short)startindex + 1;
	*Indices.Uninitialized_Add() = (unsigned short)startindex + 2;
}

void Render2DClass::Add_Outline(const RectClass& rect,float width,const RectClass& uv,unsigned long color)
{
	Vector2 v1;
	Vector2 v2;
	v1.X = rect.Left;
	v1.Y = rect.Top - 1;
	v2.X = rect.Left;
	v2.Y = rect.Bottom;
	Add_Line(v2,v1,width,uv,color);
	v2.X = rect.Right;
	v2.Y = rect.Top;
	v1.X = rect.Left;
	v1.Y = rect.Top;
	Add_Line(v1,v2,width,uv,color);
	v2.X = rect.Right;
	v2.Y = rect.Bottom;
	v1.X = rect.Right;
	v1.Y = rect.Top;
	Add_Line(v1,v2,width,uv,color);
	v2.X = rect.Left - 1;
	v2.Y = rect.Bottom;
	v1.X = rect.Right;
	v1.Y = rect.Bottom;
	Add_Line(v1,v2,width,uv,color);
}

void Render2DClass::Add_Rect(const RectClass & rect, float border_width, uint32 border_color, uint32 fill_color)
{
	Add_Outline(rect,border_width,border_color);
	RectClass r;
	r.Left = rect.Left;
	r.Top = rect.Top;
	r.Bottom = rect.Bottom - border_width;
	r.Right = rect.Right - border_width;
	Add_Quad(r,fill_color);
}

void Render2DClass::Internal_Add_Quad_Vertices(const RectClass& screen)
{
	Convert_Vert(*Vertices.Uninitialized_Add(),screen.Left,screen.Top);
	Convert_Vert(*Vertices.Uninitialized_Add(),screen.Left,screen.Bottom);
	Convert_Vert(*Vertices.Uninitialized_Add(),screen.Right,screen.Top);
	Convert_Vert(*Vertices.Uninitialized_Add(),screen.Right,screen.Bottom);
}

void Render2DClass::Internal_Add_Quad_Vertices(const Vector2 & v0, const Vector2 & v1, const Vector2 & v2, const Vector2 & v3)
{
	Convert_Vert(*Vertices.Uninitialized_Add(),v0);
	Convert_Vert(*Vertices.Uninitialized_Add(),v1);
	Convert_Vert(*Vertices.Uninitialized_Add(),v2);
	Convert_Vert(*Vertices.Uninitialized_Add(),v3);
}

void Render2DClass::Internal_Add_Quad_Colors(unsigned long color)
{
	*Colors.Uninitialized_Add() = color;
	*Colors.Uninitialized_Add() = color;
	*Colors.Uninitialized_Add() = color;
	*Colors.Uninitialized_Add() = color;
}

void Render2DClass::Internal_Add_Quad_Indicies(int start_vert_index,bool backfaced)
{
	bool b = backfaced;
	if (CoordinateScale.X * CoordinateScale.Y <= 0)
	{
		b = !b;
	}
	if (b)
	{
		*Indices.Uninitialized_Add() = (unsigned short)start_vert_index;
		*Indices.Uninitialized_Add() = (unsigned short)start_vert_index + 1;
		*Indices.Uninitialized_Add() = (unsigned short)start_vert_index + 2;
		*Indices.Uninitialized_Add() = (unsigned short)start_vert_index + 2;
		*Indices.Uninitialized_Add() = (unsigned short)start_vert_index + 1;
		*Indices.Uninitialized_Add() = (unsigned short)start_vert_index + 3;
	}
	else
	{
		*Indices.Uninitialized_Add() = (unsigned short)start_vert_index + 1;
		*Indices.Uninitialized_Add() = (unsigned short)start_vert_index;
		*Indices.Uninitialized_Add() = (unsigned short)start_vert_index + 2;
		*Indices.Uninitialized_Add() = (unsigned short)start_vert_index + 1;
		*Indices.Uninitialized_Add() = (unsigned short)start_vert_index + 2;
		*Indices.Uninitialized_Add() = (unsigned short)start_vert_index + 3;
	}
}

void Render2DClass::Internal_Add_Quad_VColors(unsigned long color1,unsigned long color2)
{
	*Colors.Uninitialized_Add() = color1;
	*Colors.Uninitialized_Add() = color2;
	*Colors.Uninitialized_Add() = color1;
	*Colors.Uninitialized_Add() = color2;
}

void Render2DClass::Internal_Add_Quad_HColors(unsigned long color1,unsigned long color2)
{
	*Colors.Uninitialized_Add() = color1;
	*Colors.Uninitialized_Add() = color1;
	*Colors.Uninitialized_Add() = color2;
	*Colors.Uninitialized_Add() = color2;
}

void Render2DClass::Internal_Add_Quad_UVs(const RectClass& uv)
{
	Vector2 *v = UVCoordinates.Uninitialized_Add();
	v->X = uv.Left;
	v->Y = uv.Top;
	v = UVCoordinates.Uninitialized_Add();
	v->X = uv.Left;
	v->Y = uv.Bottom;
	v = UVCoordinates.Uninitialized_Add();
	v->X = uv.Right;
	v->Y = uv.Top;
	v = UVCoordinates.Uninitialized_Add();
	v->X = uv.Right;
	v->Y = uv.Bottom;
}

void Render2DClass::Add_Quad(const RectClass& screen,unsigned long color)
{
	RectClass r = RectClass(0,0,1.0,1.0);
	Internal_Add_Quad_Indicies(Vertices.Count(),false);
	Internal_Add_Quad_Vertices(screen);
	Internal_Add_Quad_UVs(r);
	Internal_Add_Quad_Colors(color);
}

void Render2DClass::Add_Quad(const RectClass& screen,const RectClass& uv,unsigned long color)
{
	Internal_Add_Quad_Indicies(Vertices.Count(),false);
	Internal_Add_Quad_Vertices(screen);
	Internal_Add_Quad_UVs(uv);
	Internal_Add_Quad_Colors(color);
}

void Render2DClass::Add_Quad(const Vector2 & v0, const Vector2 & v1, const Vector2 & v2, const Vector2 & v3, const RectClass & uv, unsigned long color)
{
	Internal_Add_Quad_Indicies(Vertices.Count(),false);
	Internal_Add_Quad_Vertices(v0,v1,v2,v3);
	Internal_Add_Quad_UVs(uv);
	Internal_Add_Quad_Colors(color);
}

void Render2DClass::Add_Quad(const Vector2 & v0, const Vector2 & v1, const Vector2 & v2, const Vector2 & v3, unsigned long color)
{
	RectClass r = RectClass(0,0,1.0,1.0);
	Internal_Add_Quad_Indicies(Vertices.Count(),false);
	Internal_Add_Quad_Vertices(v0,v1,v2,v3);
	Internal_Add_Quad_UVs(r);
	Internal_Add_Quad_Colors(color);
}

void Render2DClass::Add_Quad_Backfaced(const Vector2& v0,const Vector2& v1,const Vector2& v2,const Vector2& v3,const RectClass& uv,unsigned long color)
{
	Internal_Add_Quad_Indicies(Vertices.Count(),true);
	Internal_Add_Quad_Vertices(v0,v1,v2,v3);
	Internal_Add_Quad_UVs(uv);
	Internal_Add_Quad_Colors(color);
}

void Render2DClass::Add_Line(const Vector2& a,const Vector2& b,float width,unsigned long color)
{
	RectClass r = RectClass(0,0,1.0,1.0);
	Add_Line(a,b,width,r,color);
}

void Render2DClass::Add_Quad_HGradient(const RectClass& screen,unsigned long left_color,unsigned long right_color)
{
	RectClass r = RectClass(0,0,1.0,1.0);
	Internal_Add_Quad_Indicies(Vertices.Count(),false);
	Internal_Add_Quad_Vertices(screen);
	Internal_Add_Quad_UVs(r);
	Internal_Add_Quad_HColors(left_color,right_color);
}

void Render2DClass::Add_Quad_VGradient(const RectClass& screen,unsigned long top_color,unsigned long bottom_color)
{
	RectClass r = RectClass(0,0,1.0,1.0);
	Internal_Add_Quad_Indicies(Vertices.Count(),false);
	Internal_Add_Quad_Vertices(screen);
	Internal_Add_Quad_UVs(r);
	Internal_Add_Quad_VColors(top_color,bottom_color);
}

unsigned char Font3DDataClass::Char_Width(wchar_t ch)
{
	return CharWidthTable[ch];
}

unsigned char Font3DDataClass::Char_Height(wchar_t ch)
{
	return CharHeight;
}

float Font3DDataClass::Char_U_Offset(wchar_t ch)
{
	return UOffsetTable[ch];
}

float Font3DDataClass::Char_V_Offset(wchar_t ch)
{
	return VOffsetTable[ch];
}

float Font3DDataClass::Char_U_Width(wchar_t ch)
{
	return UWidthTable[ch];
}

float Font3DDataClass::Char_V_Height(wchar_t ch)
{
	return VHeight;
}

Vector4 Font3DDataClass::Char_UV_Corners(wchar_t ch)
{
	return Vector4(UOffsetTable[ch],VOffsetTable[ch],UOffsetTable[ch] + UWidthTable[ch],VOffsetTable[ch] + VHeight);
}

TextureClass* Font3DDataClass::Peek_Texture()
{
	return Texture;
}

TextureClass* Font3DInstanceClass::Peek_Texture()
{
	return FontData->Peek_Texture();
}

void Font3DInstanceClass::Set_Mono_Spaced()
{
	MonoSpacing = FontData->Char_Width(L'W');
	Build_Cached_Tables();
}

RENEGADE_FUNCTION
void Font3DInstanceClass::Build_Cached_Tables()
AT1(0x005465E0);

float Font3DInstanceClass::String_Width(const char* test_str)
{
	float width = 0;
	while ((*test_str > 0) && (*test_str < 128))
	{
		width += ScaledSpacingTable[*test_str++];
	}
	return width;
}

float Font3DInstanceClass::String_Width(const wchar_t* test_str)
{
	float width = 0;
	while ((*test_str) && (*test_str < 256))
	{
		width += ScaledSpacingTable[*test_str++];
	}
	return width;
}

void Font3DInstanceClass::Set_Proportional()
{
	MonoSpacing = 0;
	Build_Cached_Tables();
}

void Font3DInstanceClass::Set_Scale(float scale)
{
	Scale = scale;
	Build_Cached_Tables();
}

float Font3DInstanceClass::Char_Width(wchar_t ch)
{
	return ScaledWidthTable[ch];
}

float Font3DInstanceClass::Char_Spacing(wchar_t ch)
{
	return ScaledSpacingTable[ch];
}

float Font3DInstanceClass::Char_Height()
{
	return ScaledHeight;
}

RectClass Font3DInstanceClass::Char_UV(wchar_t ch)
{
	Vector4 v;
	v = FontData->Char_UV_Corners(ch);
	return RectClass(v.X,v.Y,v.Z,v.W);
}

const int CHAR_TEXTURE_SIZE	= 256;
const int CHAR_BUFFER_LEN		= 32768;
#define IS_BREAK_CHAR(ch) ((ch == L' ') || ((ch >= 0x3000) && (ch <= 0xdfff)))
Render2DSentenceClass::Render2DSentenceClass (void) :
	Font (NULL),
	Location (0.0F,0.0F),
	Cursor (0.0F,0.0F),
	TextureOffset (0, 0),
	TextureStartX (0),
	CurSurface (NULL),
	CurrTextureSize (0),
	MonoSpaced (false),
	IsClippedEnabled (false),
	ClipRect (0, 0, 0, 0),
	BaseLocation (0, 0),
	LockedPtr (NULL),
	LockedStride (0),
	TextureSizeHint (0),
	WrapWidth (0),
	TabStop (5.0),
	DrawExtents (0, 0, 0, 0),
	Renderers(sizeof(PreAllocatedRenderers)/sizeof(RendererDataStruct),PreAllocatedRenderers)
{
	Shader = Render2DClass::Get_Default_Shader ();
	return ;
}

Render2DSentenceClass::~Render2DSentenceClass (void)
{
	REF_PTR_RELEASE (Font);
	Reset ();
	return ;
}

void
Render2DSentenceClass::Set_Font (FontCharsClass *font)
{
	Reset ();
	REF_PTR_SET (Font, font);
	return ;
}

void
Render2DSentenceClass::Reset_Polys (void)
{
	for (int index = 0; index < Renderers.Count (); index ++) {
		Renderers[index].Renderer->Reset ();
	}

	return ;
}

#ifdef SHADERS_EXPORTS
void
Render2DSentenceClass::Reset (void)
{
	if (LockedPtr != NULL) {
		CurSurface->Unlock ();
		LockedPtr = NULL;
	}
	REF_PTR_RELEASE (CurSurface);
	for (int i=0;i<Renderers.Count();++i) {
		delete Renderers[i].Renderer;
	}
	Renderers.Reset_Active();

	Cursor.Set (0, 0);
	MonoSpaced = false;

	Release_Pending_Surfaces ();
	Reset_Sentence_Data ();
	return ;
}
#endif

void
Render2DSentenceClass::Make_Additive (void)
{
	Shader.Set_Dst_Blend_Func (ShaderClass::DSTBLEND_ONE);
	Shader.Set_Src_Blend_Func (ShaderClass::SRCBLEND_ONE);
	Shader.Set_Primary_Gradient (ShaderClass::GRADIENT_MODULATE);
	Shader.Set_Secondary_Gradient (ShaderClass::SECONDARY_GRADIENT_DISABLE);

	Set_Shader (Shader);
	return ;
}

void
Render2DSentenceClass::Set_Shader (ShaderClass shader)
{
	Shader = shader;
	for (int i = 0; i < Renderers.Count (); i ++) {
		ShaderClass *curr_shader = Renderers[i].Renderer->Get_Shader ();
		(*curr_shader) = Shader;
	}
	return ;
}

#ifdef SHADERS_EXPORTS
void
Render2DSentenceClass::Render (void)
{
	if (DX8Wrapper::IsDeviceLost || !DX8Wrapper::IsInitted) return;
	Build_Textures ();
	for (int i = 0; i < Renderers.Count (); i ++) {
		Renderers[i].Renderer->Render ();
	}

	return ;
}
#endif

void
Render2DSentenceClass::Set_Base_Location (const Vector2 &loc)
{
	Vector2 dif		= loc - BaseLocation;
	BaseLocation	= loc;
	for (int i = 0; i < Renderers.Count (); i ++) {
		Renderers[i].Renderer->Move (dif);
	}

	return ;
}

void
Render2DSentenceClass::Set_Location (const Vector2 &loc)
{
	Location	= loc;
	return ;
}

void
Render2DSentenceClass::Set_Tabstop(float stop)
{
	if (stop > 0.0) {
		TabStop = stop;
	} else {
		TabStop = 1.0;
	}
}

Vector2
Render2DSentenceClass::Get_Text_Extents (const WCHAR *text)
{
	if (!DX8Wrapper::IsInitted) {
		Vector2 temp(0,0);
		return(temp);
	}

	Vector2 extent (0, (float)Font->Get_Char_Height());

	while (*text) {
		WCHAR ch = *text++;

		if ( ch != (WCHAR)'\n' ) {
			extent.X += Font->Get_Char_Spacing( ch );
		}
	}

	return extent;
}

const WCHAR *
Render2DSentenceClass::Find_Row_Start( const WCHAR * text, int row_index )
{
	if (row_index == 0) {
		return text;
	}

	if (!DX8Wrapper::IsInitted) {
		return text;
	}

	const WCHAR *retval = NULL;

	float max_x_pos	= 0;
	float x_pos			= 0;
	float y_pos			= (float)Font->Get_Char_Height ();

	int row_counter = 0;

	while (*text) {
		WCHAR ch = *text++;

		bool is_wrapped = false;
		if (IS_BREAK_CHAR (ch) && WrapWidth > 0) {
			const WCHAR *word	= text;
			float word_width = (float)Font->Get_Char_Spacing (ch);
			while ((*word != 0) && ((*word > L' ') && !IS_BREAK_CHAR (*word))) {
				word_width += Font->Get_Char_Spacing (*word++);
			}
			if ((x_pos + word_width) >= WrapWidth) {
				is_wrapped = true;
			}

		} else if (ch == L'\n') {
			is_wrapped = true;
		}
		if (is_wrapped) {
			max_x_pos = max (max_x_pos, x_pos);
			x_pos = 0;
			y_pos += Font->Get_Char_Height ();
			row_counter ++;
			if (row_counter == row_index) {
				retval = (ch == L' ' || ch == L'\n') ? text : text - 1;
				break;
			}
		}

		if (ch != (WCHAR)'\n') {
			x_pos += Font->Get_Char_Spacing (ch);
		}
	}

	return retval;
}

Vector2
Render2DSentenceClass::Get_Formatted_Text_Extents (const WCHAR *text, int *row_count)
{
	if (!DX8Wrapper::IsInitted) {
		Vector2 temp(0,0);
		return(temp);
	}
	
	float max_x_pos	= 0;
	float x_pos			= 0;
	float y_pos			= (float)Font->Get_Char_Height ();

	int row_counter = 0;

	while (*text) {
		WCHAR ch = *text++;

		bool is_wrapped = false;
		if (IS_BREAK_CHAR (ch) && WrapWidth > 0) {
			const WCHAR *word	= text;
			float word_width = (float)Font->Get_Char_Spacing (ch);
			while ((*word != 0) && ((*word > L' ') && !IS_BREAK_CHAR (*word))) {
				word_width += Font->Get_Char_Spacing (*word++);
			}
			if ((x_pos + word_width) >= WrapWidth) {
				is_wrapped = true;
			}

		} else if (ch == L'\n') {
			is_wrapped = true;
		}
		if (is_wrapped) {
			max_x_pos = max (max_x_pos, x_pos);
			x_pos = 0;
			y_pos += Font->Get_Char_Height ();
			row_counter ++;
		}

		if (ch != (WCHAR)'\n') {
			x_pos += Font->Get_Char_Spacing (ch);
		}
	}
	Vector2 extent;
	extent.X = max (max_x_pos, x_pos);
	extent.Y = y_pos;
	if (row_count != NULL) {
		(*row_count) = row_counter + 1;
	}

	return extent;
}

void
Render2DSentenceClass::Reset_Sentence_Data (void)
{
	for (int index = 0; index < SentenceData.Count (); index ++) {
		REF_PTR_RELEASE (SentenceData[index].Surface);
	}

	SentenceData.Reset_Active();
	return ;
}

void
Render2DSentenceClass::Release_Pending_Surfaces (void)
{
	for (int index = 0; index < PendingSurfaces.Count (); index ++) {
		SurfaceClass *curr_surface = PendingSurfaces[index].Surface;
		REF_PTR_RELEASE (curr_surface);
	}

	PendingSurfaces.Reset_Active();
	return;
}

#ifdef SHADERS_EXPORTS
void
Render2DSentenceClass::Build_Textures (void)
{
	if (LockedPtr != NULL) {
		CurSurface->Unlock ();
		LockedPtr = NULL;
	}
	REF_PTR_RELEASE (CurSurface);
	TextureOffset.Set (0, 0);
	TextureStartX = 0;
	for (int index = 0; index < PendingSurfaces.Count (); index ++) {
		PendingSurfaceStruct &surface_info = PendingSurfaces[index];
		SurfaceClass *curr_surface = surface_info.Surface;
		SurfaceClass::SurfaceDescription desc;
		curr_surface->Get_Description (desc);
		TextureClass *new_texture = new TextureClass (desc.Width, desc.Width, WW3D_FORMAT_A4R4G4B4, TextureClass::MIP_LEVELS_1,TextureClass::POOL_MANAGED,false);
		SurfaceClass *texture_surface = new_texture->Get_Surface_Level (0);
		D3DSURFACE_DESC desc2;
		HRESULT res = PendingSurfaces[index].Surface->Peek_D3D_Surface()->GetDesc(&desc2);
		D3DLOCKED_RECT src,dst;
		unsigned int formatsize = 4;
		if ((desc2.Format == D3DFMT_A4R4G4B4)|| (desc2.Format == D3DFMT_R5G6B5))
		{
			formatsize = 2;
		}
		res = texture_surface->Peek_D3D_Surface()->LockRect(&dst,NULL,0);
		res = PendingSurfaces[index].Surface->Peek_D3D_Surface()->LockRect(&src,NULL,D3DLOCK_READONLY);
		memcpy(dst.pBits,src.pBits,desc2.Width * desc2.Height * formatsize);
		texture_surface->Peek_D3D_Surface()->UnlockRect();
		PendingSurfaces[index].Surface->Peek_D3D_Surface()->UnlockRect();
		REF_PTR_RELEASE (texture_surface);
		for (int renderer_index = 0; renderer_index < surface_info.Renderers.Count (); renderer_index ++) {
			Render2DClass *renderer = surface_info.Renderers[renderer_index];
			renderer->Set_Texture (new_texture);
		}
		REF_PTR_RELEASE (new_texture);
		REF_PTR_RELEASE (curr_surface);
	}
	PendingSurfaces.Reset_Active();
	return ;
}
#endif


#ifdef SHADERS_EXPORTS
void
Render2DSentenceClass::Draw_Sentence (uint32 color)
{
	Render2DClass *curr_renderer	= NULL;
	SurfaceClass *curr_surface		= NULL;

	DrawExtents.Set (0, 0, 0, 0);
	for (int index = 0; index < SentenceData.Count (); index ++) {
		SentenceDataStruct &data = SentenceData[index];
		if (data.Surface != curr_surface) {
			curr_surface = data.Surface;
			bool found = false;
			for (int renderer_index = 0; renderer_index < Renderers.Count (); renderer_index ++) {
				if (Renderers[renderer_index].Surface == curr_surface) {
					found = true;
					curr_renderer = Renderers[renderer_index].Renderer;
					break;
				}
			}
			if (found == false) {
				curr_renderer = new Render2DClass;
				curr_renderer->Set_Coordinate_Range (Render2DClass::Get_Screen_Resolution ());
				ShaderClass *curr_shader = curr_renderer->Get_Shader ();
				(*curr_shader) = Shader;
				RendererDataStruct render_info;
				render_info.Renderer	= curr_renderer;
				render_info.Surface	= curr_surface;
				Renderers.Add (render_info);
				for (int surface_index = 0; surface_index < PendingSurfaces.Count (); surface_index ++) {
					PendingSurfaceStruct &surface_info = PendingSurfaces[surface_index];
					if (surface_info.Surface == curr_surface) {
						surface_info.Renderers.Add (curr_renderer);
					}
				}
			}
		}
		SurfaceClass::SurfaceDescription desc;
		curr_surface->Get_Description (desc);
		RectClass screen_rect	= data.ScreenRect;
		screen_rect					+= Location;
		RectClass uv_rect			= data.UVRect;
		bool add_quad = true;
		if (IsClippedEnabled) {
			if (	screen_rect.Right <= ClipRect.Left ||
					screen_rect.Bottom <= ClipRect.Top)
			{
				add_quad = false;
			} else {
				RectClass clipped_rect;
				clipped_rect.Left		= max (screen_rect.Left, ClipRect.Left);
				clipped_rect.Right	= min (screen_rect.Right, ClipRect.Right);
				clipped_rect.Top		= max (screen_rect.Top, ClipRect.Top);
				clipped_rect.Bottom	= min (screen_rect.Bottom, ClipRect.Bottom);
				RectClass clipped_uv_rect;
				float percent				= ((clipped_rect.Left - screen_rect.Left) / screen_rect.Width ());
				clipped_uv_rect.Left		= uv_rect.Left + (uv_rect.Width () * percent);

				percent						= ((clipped_rect.Right - screen_rect.Left) / screen_rect.Width ());
				clipped_uv_rect.Right	= uv_rect.Left + (uv_rect.Width () * percent);

				percent						= ((clipped_rect.Top - screen_rect.Top) / screen_rect.Height ());
				clipped_uv_rect.Top		= uv_rect.Top + (uv_rect.Height () * percent);

				percent						= ((clipped_rect.Bottom - screen_rect.Top) / screen_rect.Height ());
				clipped_uv_rect.Bottom	= uv_rect.Top + (uv_rect.Height () * percent);
				screen_rect = clipped_rect;
				uv_rect		= clipped_uv_rect;
			}
		}

		if (add_quad) {
			uv_rect *=  1.0F / ((float)desc.Width);
			curr_renderer->Add_Quad (screen_rect, uv_rect, color);
			if (DrawExtents.Width () == 0) {
				DrawExtents = screen_rect;
			} else {
				DrawExtents += screen_rect;
			}
		}
	}

	return ;
}
#endif

void
Render2DSentenceClass::Record_Sentence_Chunk (void)
{
	int width = TextureOffset.I - TextureStartX;
	if (width > 0) {
		float char_height = (float)Font->Get_Char_Height ();
		SentenceDataStruct sentence_data;
		sentence_data.Surface = CurSurface;
		sentence_data.Surface->Add_Ref ();
		sentence_data.ScreenRect.Left		= Cursor.X;
		sentence_data.ScreenRect.Right	= Cursor.X + width;
		sentence_data.ScreenRect.Top		= Cursor.Y;
		sentence_data.ScreenRect.Bottom	= Cursor.Y + char_height;
		sentence_data.UVRect.Left			= (float)TextureStartX;
		sentence_data.UVRect.Top			= (float)TextureOffset.J;
		sentence_data.UVRect.Right			= (float)TextureOffset.I;
		sentence_data.UVRect.Bottom		= (float)(TextureOffset.J + char_height);
		SentenceData.Add (sentence_data);
	}

	return ;
}

#ifdef SHADERS_EXPORTS
void
Render2DSentenceClass::Allocate_New_Surface (const WCHAR *text)
{
	if (LockedPtr != NULL) {
		CurSurface->Unlock ();
		LockedPtr = NULL;
	}
	int text_width = 0;
	for (int index = 0; text[index] != 0; index ++) {
		text_width += Font->Get_Char_Spacing (text[index]);
	}

	int char_height = Font->Get_Char_Height ();
	CurrTextureSize = 256;
	int best_tex_mem_usage = 999999999;
	for (int pow2 = 6; pow2 <= 8; pow2 ++) {

		int size					= 1 << pow2;
		int row_count			= (text_width / size) + 1;
		int rows_per_texture	= size / (char_height + 1);
		if (rows_per_texture > 0) {
			int texture_count	= row_count / rows_per_texture;
			texture_count		= max (texture_count, 1);
			int texture_mem_usage = (texture_count * size * size);
			if (texture_mem_usage < best_tex_mem_usage) {
				CurrTextureSize		= size;
				best_tex_mem_usage	= texture_mem_usage;
			}
		}
	}
	CurrTextureSize = max (TextureSizeHint, CurrTextureSize);
	REF_PTR_RELEASE (CurSurface);
	CurSurface = new SurfaceClass(CurrTextureSize, CurrTextureSize, WW3D_FORMAT_A4R4G4B4);
	CurSurface->Add_Ref ();
	PendingSurfaceStruct surface_info;
	surface_info.Surface = CurSurface;
	PendingSurfaces.Add (surface_info);
	TextureOffset.Set (0, 0);
	TextureStartX = 0;
	return ;
}
#endif

#ifdef SHADERS_EXPORTS
void
Render2DSentenceClass::Build_Sentence (const WCHAR *text)
{
	if (text == NULL) {
		return ;
	}

	if (!DX8Wrapper::IsInitted) {
		return;
	}
	Reset_Sentence_Data ();
	Cursor.Set (0, 0);
	if (CurSurface == NULL) {
		Allocate_New_Surface (text);
	}

	float char_height = (float)Font->Get_Char_Height ();
	while (text != NULL) {
		WCHAR ch = *text++;
		float char_spacing = (float)Font->Get_Char_Spacing (ch);

		bool exceeded_texture_width	= ((TextureOffset.I + char_spacing) >= CurrTextureSize);
		bool encountered_break_char	= (IS_BREAK_CHAR (ch) || ch == L'\n' || ch == 0 || ch == L'\t');
		if (exceeded_texture_width || encountered_break_char) {
			Record_Sentence_Chunk ();
			Cursor.X			+= (TextureOffset.I - TextureStartX);
			TextureStartX	= TextureOffset.I;
			if (IS_BREAK_CHAR (ch)) {

				if (ch == L' ') {
					Cursor.X += char_spacing;
				}
				if (WrapWidth > 0) {
					const WCHAR *word	= text;
					float word_width	= (ch == L' ') ? 0 : char_spacing;
					while ((*word != 0) && ((*word > L' ') && !IS_BREAK_CHAR (*word))) {
						word_width += Font->Get_Char_Spacing (*word++);
					}
					if ((Cursor.X + word_width) >= WrapWidth) {
						Cursor.X = 0;
						Cursor.Y += char_height;
					}
				}

			} else if (ch == L'\n') {
				Cursor.X = 0;
				Cursor.Y += char_height;
			} else if (ch == 0) {
				break;
			} else if (ch == L'\t') {
				float tab_spacing = (char_spacing * TabStop);
				float tab_pos = (floor(Cursor.X / tab_spacing) * tab_spacing);
				Cursor.X = (tab_pos + tab_spacing);
			}
			if (exceeded_texture_width) {
				TextureStartX		= 0;
				TextureOffset.I	= TextureStartX;
				TextureOffset.J	+= (int)char_height;
				if ((TextureOffset.J + char_height) >= CurrTextureSize) {
					Allocate_New_Surface (text);
				}
			}
		}

		if (ch != L'\n' && ch != L' ' && ch != L'\t') {
			if (LockedPtr == NULL) {
				LockedPtr = (uint16 *)CurSurface->Lock (&LockedStride);
			}
			Font->Blit_Char (ch, LockedPtr, LockedStride, TextureOffset.I, TextureOffset.J);
			TextureOffset.I += (int)char_spacing;
		}
	}

	return ;
}
#endif

void	Render2DSentenceClass::Force_Alpha( float alpha )
{
	for (int i = 0; i < Renderers.Count (); i ++) {
		Renderers[i].Renderer->Force_Alpha( alpha );
	}
}

FontCharsClass::FontCharsClass (void) :
	OldGDIFont(	NULL ),
	OldGDIBitmap( NULL ),
	GDIFont( NULL ),
	GDIBitmap( NULL ),
	GDIBitmapBits ( NULL ),
	MemDC( NULL ),
	CurrPixelOffset( 0 ),
	PointSize( 0 ),
	CharHeight( 0 ),
	UnicodeCharArray( NULL ),
	FirstUnicodeChar( 0xFFFF ),
	LastUnicodeChar( 0 ),
	IsBold (false),
	BufferList(sizeof(PreAllocatedBufferList)/sizeof(uint16*),PreAllocatedBufferList)
{
	::memset( ASCIICharArray, 0, sizeof (ASCIICharArray) );
	return ;
}

FontCharsClass::~FontCharsClass (void)
{
	for (int i=0;i<BufferList.Count(); ++i) {
		delete [] BufferList[i];
	}
	BufferList.Reset_Active();

	Free_GDI_Font();
	Free_Character_Arrays();
	return ;
}

const FontCharsClass::CharDataStruct *
FontCharsClass::Get_Char_Data (WCHAR ch)
{
	const CharDataStruct *retval = NULL;

	if ( ch < 256 ) {
		retval = ASCIICharArray[ch];
	} else {
		Grow_Unicode_Array( ch );
		retval = UnicodeCharArray[ch - FirstUnicodeChar];
	}
	if ( retval == NULL ) {
		retval = Store_GDI_Char( ch );
	}
	return retval;
}

int
FontCharsClass::Get_Char_Width (WCHAR ch)
{
	const CharDataStruct	* data = Get_Char_Data( ch );
	if ( data != NULL ) {
		return data->Width;
	}

	return 0;
}

int
FontCharsClass::Get_Char_Spacing (WCHAR ch)
{
	const CharDataStruct	* data = Get_Char_Data( ch );
	if ( data != NULL ) {
		if ( data->Width != 0 ) {
			return data->Width + 1;
		}
	}

	return 0;
}

void
FontCharsClass::Blit_Char (WCHAR ch, uint16 *dest_ptr, int dest_stride, int x, int y)
{
	const CharDataStruct	* data = Get_Char_Data( ch );
	if ( data != NULL && data->Width != 0 ) {
		int dest_inc		= (dest_stride >> 1);
		uint16 *src_ptr	= data->Buffer;
		dest_ptr				+= (dest_inc * y) + x;
		for ( int row = 0; row < CharHeight; row ++ ) {
			for ( int col = 0; col < data->Width; col ++ ) {
				dest_ptr[col] = *src_ptr++;
			}
			dest_ptr	+= dest_inc;
		}
	}

	return ;
}

#ifdef SHADERS_EXPORTS
const FontCharsClass::CharDataStruct *
FontCharsClass::Store_GDI_Char (WCHAR ch)
{
	int width	= PointSize * 0x20;
	int height	= PointSize * 0x20;
	SIZE char_size = { 0 };
	::GetTextExtentPoint32W( MemDC, &ch, 1, &char_size );
	int x_pos = 0;
	if ( (ch == 'W' || ch == 'V') && (GDIFontName.Compare_No_Case ("Arial MT") == 0) ) {
		x_pos				= 1;
		char_size.cx	+= 1;
	}
	RECT rect = { 0, 0, width, height };
	::ExtTextOutW( MemDC, x_pos, 0, ETO_OPAQUE, &rect, &ch, 1, NULL);
	Update_Current_Buffer( char_size.cx );
	uint16 *curr_buffer = BufferList[BufferList.Count () - 1];
	curr_buffer += CurrPixelOffset;
	int stride = (((width * 3) + 3) & ~3);
	for (int row = 0; row < char_size.cy; row ++) {
		int index = (row * stride);
		for (int col = 0; col < char_size.cx; col ++) {
			uint8 pixel_value = GDIBitmapBits[index];
			index += 3;

			uint16 pixel_color = 0;
			if (pixel_value != 0) {
				pixel_color = 0x0FFF;
			}
			uint8 alpha_value	= ((pixel_value >> 4) & 0xF);
			*curr_buffer ++	= pixel_color | (alpha_value << 12);
		}
	}
	CharDataStruct *char_data	= new CharDataStruct;
	char_data->Value				= ch;
	char_data->Width				= (short)char_size.cx;
	char_data->Buffer				= BufferList[BufferList.Count () - 1] + CurrPixelOffset;
	if ( ch < 256 ) {
		ASCIICharArray[ch] = char_data;
	} else {
		UnicodeCharArray[ch - FirstUnicodeChar] = char_data;
	}
	CurrPixelOffset += (char_size.cx * CharHeight);
	return char_data;
}
#endif

void
FontCharsClass::Update_Current_Buffer (int char_width)
{
	bool needs_new_buffer = (BufferList.Count () == 0);
	if (needs_new_buffer == false) {

		if ( (CurrPixelOffset + (char_width * CharHeight)) > CHAR_BUFFER_LEN ) {
			needs_new_buffer = true;
		}
	}
	if (needs_new_buffer) {
		uint16 *new_buffer = new uint16[CHAR_BUFFER_LEN];
		BufferList.Add( new_buffer );
		CurrPixelOffset = 0;
	}

	return ;
}

#ifdef SHADERS_EXPORTS
void
FontCharsClass::Create_GDI_Font (const char *font_name)
{
	HDC screen_dc = ::GetDC (NULL);
	int font_height = -MulDiv (PointSize, ::GetDeviceCaps (screen_dc, LOGPIXELSY), 72);
	DWORD bold		= IsBold ? FW_BOLD : FW_NORMAL;
	DWORD italic	= 0;
	DWORD	charset;
	switch (GetACP()) {
		case 936:
		case 950:
			charset = CHINESEBIG5_CHARSET;
			break;
		case 932:
			charset = SHIFTJIS_CHARSET;
			break;
		case 949:
			charset = HANGUL_CHARSET;
			break;
		default:
			charset = DEFAULT_CHARSET;
			break;
	}

	GDIFont = ::CreateFont (font_height, 0, 0, 0, bold, italic,
									FALSE, FALSE, charset, OUT_DEFAULT_PRECIS,
									CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
									VARIABLE_PITCH, font_name);
	BITMAPINFOHEADER bitmap_info = { 0 };
	bitmap_info.biSize				= sizeof (BITMAPINFOHEADER);
	bitmap_info.biWidth				= PointSize * 0x20;
	bitmap_info.biHeight				= -(PointSize * 0x20);
	bitmap_info.biPlanes				= 1;
	bitmap_info.biBitCount			= 24;
	bitmap_info.biCompression		= BI_RGB;
	bitmap_info.biSizeImage			= 0;
	bitmap_info.biXPelsPerMeter	= 0;
	bitmap_info.biYPelsPerMeter	= 0;
	bitmap_info.biClrUsed			= 0;
	bitmap_info.biClrImportant		= 0;
	GDIBitmap	= ::CreateDIBSection (	screen_dc,
													(const BITMAPINFO *)&bitmap_info,
													DIB_RGB_COLORS,
													(void **)&GDIBitmapBits,
													NULL,
													0L);
	MemDC = ::CreateCompatibleDC (NULL);
	OldGDIBitmap	= (HBITMAP)::SelectObject (MemDC, GDIBitmap);
	OldGDIFont		= (HFONT)::SelectObject (MemDC, GDIFont);
	::SetBkColor (MemDC, RGB (0, 0, 0));
	::SetTextColor (MemDC, RGB (255, 255, 255));
	TEXTMETRIC text_metric = { 0 };
	::GetTextMetrics (MemDC, &text_metric);
	CharHeight = text_metric.tmHeight;
	::ReleaseDC (NULL, screen_dc);
	return ;
}
#endif

void
FontCharsClass::Free_GDI_Font (void)
{
	if ( GDIFont != NULL ) {
		::SelectObject( MemDC, OldGDIFont );
		::DeleteObject( GDIFont );
		GDIFont = NULL;
	}
	if ( GDIBitmap != NULL ) {
		::SelectObject( MemDC, OldGDIBitmap );
		::DeleteObject( GDIBitmap );
		GDIBitmap = NULL;
	}
	if ( MemDC != NULL ) {
		::DeleteDC( MemDC );
		MemDC = NULL;
	}

	return ;
}

void
FontCharsClass::Initialize_GDI_Font (const char *font_name, int point_size, bool is_bold)
{
	Name.Format ("%s%d", font_name, point_size);

	GDIFontName	= font_name;
	PointSize	= point_size;
	IsBold		= is_bold;
	Create_GDI_Font (font_name);
	return ;
}

bool
FontCharsClass::Is_Font (const char *font_name, int point_size, bool is_bold)
{
	bool retval = false;
	if (	(GDIFontName.Compare_No_Case (font_name) == 0) &&
			(point_size == PointSize) &&
			(is_bold == IsBold))
	{
		retval = true;
	}

	return retval;
}

void
FontCharsClass::Grow_Unicode_Array (WCHAR ch)
{
	if ( ch < 256 ) {
		return ;
	}
	if ( ch >= FirstUnicodeChar && ch <= LastUnicodeChar ) {
		return ;
	}

	uint16 first_index	= min( FirstUnicodeChar, ch );
	uint16 last_index		= max( LastUnicodeChar, ch );
	uint16 count			= (last_index - first_index) + 1;
	CharDataStruct **new_array = new CharDataStruct *[count];
	::memset (new_array, 0, sizeof (CharDataStruct *) * count);
	if ( UnicodeCharArray != NULL ) {
		int start_offset	= (FirstUnicodeChar - first_index);
		int old_count		= (LastUnicodeChar - FirstUnicodeChar) + 1;
		::memcpy (&new_array[start_offset], UnicodeCharArray, sizeof (CharDataStruct *) * old_count);
		delete [] UnicodeCharArray;
		UnicodeCharArray = NULL;
	}

	FirstUnicodeChar	= first_index;
	LastUnicodeChar	= last_index;
	UnicodeCharArray	= new_array;
	return ;
}

void
FontCharsClass::Free_Character_Arrays (void)
{
	if ( UnicodeCharArray != NULL ) {

		int count = (LastUnicodeChar - FirstUnicodeChar) + 1;
		for (int index = 0; index < count; index ++) {
			if ( UnicodeCharArray[index] != NULL ) {
				delete UnicodeCharArray[index];
				UnicodeCharArray[index] = NULL;
			}
		}
		delete [] UnicodeCharArray;
		UnicodeCharArray = NULL;
	}
	for (int index = 0; index < 256; index ++) {
		if ( ASCIICharArray[index] != NULL ) {
			delete ASCIICharArray[index];
			ASCIICharArray[index] = NULL;
		}
	}
	return ;
}
