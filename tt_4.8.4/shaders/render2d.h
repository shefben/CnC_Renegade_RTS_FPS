#ifndef TT_INCLUDE__RENDER2D_H
#define TT_INCLUDE__RENDER2D_H



#include "engine_3dre.h"
#include "Rect.h"
#define VRGB_TO_INT32(rgb)		(unsigned(rgb[0]*255.0f)<<16)|(unsigned(rgb[1]*255.0f)<<8)|(unsigned(rgb[2]*255.0f))|0xFF000000
#define VRGBA_TO_INT32(rgb)	(unsigned(rgb[3]*255.0f)<<24)|(unsigned(rgb[0]*255.0f)<<16)|(unsigned(rgb[1]*255.0f)<<8)|(unsigned(rgb[2]*255.0f))
#define RGB_TO_INT32(r,g,b)		(unsigned(r)<<16)|(unsigned(g)<<8)|(unsigned(b))|0xFF000000
#define RGBA_TO_INT32(r,g,b,a)	(unsigned(a)<<24)|(unsigned(r)<<16)|(unsigned(g)<<8)|(unsigned(b))
#define FRGB_TO_INT32(r,g,b)		(unsigned(r*255.0f)<<16)|(unsigned(g*255.0f)<<8)|(unsigned(b*255.0f))|0xFF000000
#define FRGBA_TO_INT32(r,g,b,a)	(unsigned(a*255.0f)<<24)|(unsigned(r*255.0f)<<16)|(unsigned(g*255.0f)<<8)|(unsigned(b*255.0f))
#define INT32_TO_VRGB(color, vrgb)							\
	vrgb[0] = ((color & 0x00FF0000) >> 16) / 256.0F;	\
	vrgb[1] = ((color & 0x0000FF00) >> 8) / 256.0F;		\
	vrgb[2] = ((color & 0x000000FF)) / 256.0F;
#define INT32_TO_VRGBA(color, vrgba)						\
	vrgba[0] = ((color & 0x00FF0000) >> 16) / 256.0F;	\
	vrgba[1] = ((color & 0x0000FF00) >> 8) / 256.0F;	\
	vrgba[2] = ((color & 0x000000FF)) / 256.0F;			\
	vrgba[3] = ((color & 0xFF000000) >> 24) / 256.0F;



class TextureClass;
class Render2DSentenceClass;



class Render2DClass
{
public:
	Render2DClass( TextureClass* tex = NULL );
	virtual ~Render2DClass(void);
	virtual void Reset();
	void SHADERS_API Render();
	void	Set_Coordinate_Range( const RectClass & range );
	void	Set_Texture( const char * filename );
	void	Set_Texture_Init( const char * filename );
	void	Set_Texture(TextureClass* tex);
	
	static ShaderClass Get_Default_Shader();
	static void Set_Screen_Resolution(const RectClass& rect);
	static RectClass &Get_Screen_Resolution();
	TextureClass * Peek_Texture( void )			{ return Texture; }
	void	Enable_Additive(bool b);
	void	Enable_Alpha(bool b);
	void  Enable_Texturing(bool b);
	ShaderClass *			Get_Shader( void ) { return &Shader; }
	void	Add_Quad( const Vector2 & v0, const Vector2 & v1, const Vector2 & v2, const Vector2 & v3, const RectClass & uv, unsigned long color = 0xFFFFFFFF  );
	void	Add_Quad_Backfaced( const Vector2 & v0, const Vector2 & v1, const Vector2 & v2, const Vector2 & v3, const RectClass & uv, unsigned long color = 0xFFFFFFFF  );
	void	Add_Quad( const RectClass & screen, const RectClass & uv, unsigned long color = 0xFFFFFFFF  );
	void	Add_Quad( const Vector2 & v0, const Vector2 & v1, const Vector2 & v2, const Vector2 & v3, unsigned long color = 0xFFFFFFFF  );
	void	Add_Quad( const RectClass & screen, unsigned long color = 0xFFFFFFFF );
	void	Add_Quad_VGradient( const RectClass & screen, unsigned long top_color, unsigned long bottom_color );
	void	Add_Quad_HGradient( const RectClass & screen, unsigned long left_color, unsigned long right_color );
	void	Add_Tri( const Vector2 & v0, const Vector2 & v1, const Vector2 & v2, const Vector2 & uv0, const Vector2 & uv1, const Vector2 & uv2, unsigned long color = 0xFFFFFFFF  );
	void	Add_Line( const Vector2 & a, const Vector2 & b, float width, unsigned long color = 0xFFFFFFFF );
	void	Add_Line( const Vector2 & a, const Vector2 & b, float width, const RectClass & uv, unsigned long color = 0xFFFFFFFF );
	void	Add_Outline( const RectClass & rect, float width = 1.0F, unsigned long color = 0xFFFFFFFF );
	void	Add_Outline( const RectClass & rect, float width, const RectClass & uv, unsigned long color = 0xFFFFFFFF );
	void	Add_Rect( const RectClass & rect, float border_width = 1.0F, uint32 border_color = 0xFF000000, uint32 fill_color = 0xFFFFFFFF);
	void Set_Hidden( bool hide )			{ IsHidden = hide; }
	void	Set_Z_Value (float z_value)	{ ZValue = z_value; }
	void	Move( const Vector2 & a );
	DynamicVectorClass<unsigned long> &	Get_Color_Array (void)	{ return Colors; }
	void	Force_Alpha( float alpha );
	void	Force_Color( int color );
protected:
	Vector2										CoordinateScale;
	Vector2										CoordinateOffset;
	Vector2										BiasedCoordinateOffset;
	TextureClass *								Texture;
	ShaderClass									Shader;
	DynamicVectorClass<unsigned short>	Indices;
	unsigned short								PreAllocatedIndices[60];
	DynamicVectorClass<Vector2>				Vertices;
	Vector2										PreAllocatedVertices[60];
	DynamicVectorClass<Vector2>				UVCoordinates;
	Vector2										PreAllocatedUVCoordinates[60];
	DynamicVectorClass<unsigned long>		Colors;
	unsigned long								PreAllocatedColors[60];
	bool											IsHidden;
	float											ZValue;
	static REF_DECL1(ScreenResolution, RectClass);
	Vector2 Convert_Vert( const Vector2 & v );
	void	  Convert_Vert( Vector2 & vert_out, const Vector2 & vert_in );
	void	  Convert_Vert( Vector2 & vert_out, float x_in, float y_in );
	void	  Update_Bias( void );
	void	Internal_Add_Quad_Vertices( const Vector2 & v0, const Vector2 & v1, const Vector2 & v2, const Vector2 & v3 );
	void	Internal_Add_Quad_Vertices( const RectClass & screen );
	void	Internal_Add_Quad_UVs( const RectClass & uv );
	void	Internal_Add_Quad_Colors( unsigned long color );
	void	Internal_Add_Quad_VColors( unsigned long color1, unsigned long color2 );
	void	Internal_Add_Quad_HColors( unsigned long color1, unsigned long color2 );
	void	Internal_Add_Quad_Indicies( int start_vert_index, bool backfaced = false );
}; // 05B4



class Font3DDataClass : public RefCountClass {
private:
	char *Name; // 0008  0020
	TextureClass *Texture; // 000C  0024
	float UOffsetTable[256]; // 0010  0028
	float VOffsetTable[256]; // 0410  0428
	float UWidthTable[256]; // 0810  0828
	float VHeight; // 0C10  0C28
	unsigned char CharWidthTable[256]; // 0C14  0C2C
	unsigned char CharHeight; // 0D14  0D2C
public:
	SurfaceClass SHADERS_API *Minimize_Font_Image(SurfaceClass *surface);
	~Font3DDataClass();

	Font3DDataClass(char* filename);
	unsigned char Char_Width(wchar_t ch);
	unsigned char Char_Height(wchar_t ch);
	float Char_U_Offset(wchar_t ch);
	float Char_V_Offset(wchar_t ch);
	float Char_U_Width(wchar_t ch);
	float Char_V_Height(wchar_t ch);
	Vector4 Char_UV_Corners(wchar_t ch);
	TextureClass* Peek_Texture();
	SHADERS_API bool Load_Font_Image(char* filename);
	SurfaceClass* Make_Proportional(SurfaceClass* surface);
}; // 0D18  0D30

class Font3DInstanceClass : public RefCountClass {
private:
	Font3DDataClass *FontData;
	float Scale;
	float SpaceSpacing;
	float InterCharSpacing;
	float MonoSpacing;
	float ScaledWidthTable[256];
	float ScaledSpacingTable[256];
	float ScaledHeight;
public:
	~Font3DInstanceClass();
	Font3DInstanceClass(char* filename);
	TextureClass* Peek_Texture();
	void Set_Mono_Spaced();
	void Set_Proportional();
	void Set_Scale(float scale);
	float Char_Width(wchar_t ch);
	float Char_Spacing(wchar_t ch);
	float Char_Height();
	float String_Width(const char* test_str);
	float String_Width(const wchar_t* test_str);
	RectClass Char_UV(wchar_t ch);
	void Build_Cached_Tables();
};

class Render2DTextClass : public Render2DClass {
public:
	Render2DTextClass(Font3DInstanceClass *font=NULL);
	~Render2DTextClass();
	Font3DInstanceClass *	Peek_Font( void )				{ return Font; }
	void	Set_Font( Font3DInstanceClass *font );
	virtual	void	Reset(void);
	void	Set_Location( const Vector2 & loc )				{ Location = loc; Cursor = loc; }
	void	Set_Wrapping_Width (float width)					{ WrapWidth = width; }
	void	Set_Clipping_Rect( const RectClass &rect )	{ ClipRect = rect; IsClippedEnabled = true; }
	bool	Is_Clipping_Enabled( void ) const				{ return IsClippedEnabled; }
	void	Enable_Clipping( bool onoff )						{ IsClippedEnabled = onoff; }
	void	Draw_Text( const char * text, unsigned long color = 0xFFFFFFFF );
	void	Draw_Text( const WCHAR * text, unsigned long color = 0xFFFFFFFF );
	void	Draw_Block( const RectClass & screen, unsigned long color = 0xFFFFFFFF );
	const RectClass & Get_Draw_Extents( void )			{ return DrawExtents; }
	const RectClass & Get_Total_Extents( void )			{ return TotalExtents; }
	const Vector2 & Get_Cursor( void )						{ return Cursor; }
	Vector2	Get_Text_Extents( const WCHAR * text );
private:
	Font3DInstanceClass* Font; // 05B4
	Vector2					Location; // 05B8
	Vector2					Cursor; // 05C0
	float						WrapWidth; // 05C8
	RectClass				DrawExtents; // 05CC
	RectClass				TotalExtents; // 05DC
	RectClass				BlockUV; // 05EC
	RectClass				ClipRect; // 05FC
	bool						IsClippedEnabled; // 060C
	void	Draw_Char( WCHAR ch, unsigned long color );
}; // 0610

class FontCharsClass : public RefCountClass
{

public:
	FontCharsClass( void );
	~FontCharsClass();
	void	Initialize_GDI_Font( const char *font_name, int point_size, bool is_bold );
	bool	Is_Font( const char *font_name, int point_size, bool is_bold );
	const char * Get_Name( void )			{ return Name; }	
	int	Get_Char_Height( void )			{ return CharHeight; }
	int	Get_Char_Width( WCHAR ch );
	int	Get_Char_Spacing( WCHAR ch );
	void	Blit_Char( WCHAR ch, uint16 *dest_ptr, int dest_stride, int x, int y );
private:
	struct CharDataStruct {
		WCHAR				Value;
		short				Width;
		uint16 *			Buffer;
	};
	void SHADERS_API Create_GDI_Font(const char *font_name);
	void							Free_GDI_Font( void );
	const CharDataStruct SHADERS_API *Store_GDI_Char(wchar_t ch);
	void							Update_Current_Buffer( int char_width );
	const CharDataStruct	*	Get_Char_Data( WCHAR ch );
	void							Grow_Unicode_Array( WCHAR ch );
	void							Free_Character_Arrays( void );
	StringClass							Name;
	DynamicVectorClass<uint16 *>	BufferList;
	uint16*								PreAllocatedBufferList[16];	// We'll use this with BufferList first
	int									CurrPixelOffset;
	int									CharHeight;
	int									PointSize;
	StringClass							GDIFontName;
	HFONT									OldGDIFont;
	HBITMAP								OldGDIBitmap;
	HBITMAP								GDIBitmap;	
	HFONT									GDIFont;
	uint8 *								GDIBitmapBits;
	HDC									MemDC;
	CharDataStruct *					ASCIICharArray[256];
	CharDataStruct **					UnicodeCharArray;
	uint16								FirstUnicodeChar;
	uint16								LastUnicodeChar;
	bool									IsBold;
}; // 0498

class Render2DSentenceClass {
public:
	Render2DSentenceClass( void );
	~Render2DSentenceClass();
	void SHADERS_API Render();
	SHADERS_API virtual void Reset();
	void				Reset_Polys (void);
	FontCharsClass *	Peek_Font( void )						{ return Font; }
	void	Set_Font( FontCharsClass *font );
	void	Set_Location( const Vector2 & loc );
	void	Set_Base_Location( const Vector2 & loc );
	void	Set_Wrapping_Width (float width)					{ WrapWidth = width; }
	void	Set_Tabstop(float stop);
	void	Set_Clipping_Rect( const RectClass &rect )	{ ClipRect = rect; IsClippedEnabled = true; }
	bool	Is_Clipping_Enabled( void ) const				{ return IsClippedEnabled; }
	void	Enable_Clipping( bool onoff )						{ IsClippedEnabled = onoff; }
	void			Make_Additive (void);
	ShaderClass	Get_Shader (void) const						{ return Shader; }
	void			Set_Shader (ShaderClass shader);
	const RectClass & Get_Draw_Extents( void )			{ return DrawExtents; }
	Vector2			Get_Text_Extents( const WCHAR * text );
	Vector2			Get_Formatted_Text_Extents( const WCHAR * text, int *row_count = NULL );
	const WCHAR *	Find_Row_Start( const WCHAR * text, int row_index );
	SHADERS_API void	Build_Sentence (const WCHAR *text);
	SHADERS_API void	Draw_Sentence (uint32 color = 0xFFFFFFFF);
	void	Set_Texture_Size_Hint( int hint )				{ TextureSizeHint = hint; }
	int	Get_Texture_Size_Hint( void ) const				{ return TextureSizeHint; }
	void	Set_Mono_Spaced( bool onoff )						{ MonoSpaced = onoff; }
	void	Force_Alpha( float alpha );
private:
	struct SentenceDataStruct {
		SurfaceClass *		Surface;
		RectClass			ScreenRect;
		RectClass			UVRect;

		bool operator== (const SentenceDataStruct &src)	{ return false; }
		bool operator!= (const SentenceDataStruct &src)	{ return true; }
	};
	struct PendingSurfaceStruct {
		SurfaceClass *								Surface;
		DynamicVectorClass<Render2DClass *>	Renderers;
		Render2DClass *							PreAllocatedRenderers[16];	// Use this with Renderers at first

		PendingSurfaceStruct() : Renderers(sizeof(PreAllocatedRenderers)/sizeof(Render2DClass*),PreAllocatedRenderers) {}

		bool operator== (const PendingSurfaceStruct &src)	{ return false; }
		bool operator!= (const PendingSurfaceStruct &src)	{ return true; }
	};
	struct RendererDataStruct {
		Render2DClass *	Renderer;
		SurfaceClass *		Surface;

		bool operator== (const RendererDataStruct &src)	{ return false; }
		bool operator!= (const RendererDataStruct &src)	{ return true; }
	};
	void	Reset_Sentence_Data (void);
	void	Build_Textures (void);
	void	Record_Sentence_Chunk (void);
	SHADERS_API void	Allocate_New_Surface (const WCHAR *text);
	void	Release_Pending_Surfaces (void);
	DynamicVectorClass<SentenceDataStruct>		SentenceData;
	DynamicVectorClass<PendingSurfaceStruct>	PendingSurfaces;
	DynamicVectorClass<RendererDataStruct>		Renderers;
	RendererDataStruct								PreAllocatedRenderers[16];	// Use this with Renderers at first
	FontCharsClass	*								Font;
	Vector2											BaseLocation;
	Vector2											Location;
	Vector2											Cursor;
	Vector2i											TextureOffset;
	int												TextureStartX;
	int												CurrTextureSize;
	int												TextureSizeHint;
	SurfaceClass *									CurSurface;
	bool												MonoSpaced;
	float												WrapWidth;
	float												TabStop;
	RectClass										ClipRect;
	RectClass										DrawExtents;
	bool												IsClippedEnabled;
	uint16 *											LockedPtr;
	int												LockedStride;
	TextureClass *									CurTexture;
	ShaderClass										Shader;
}; // 0140

Render2DSentenceClass *CreateRender2DSentenceClass(unsigned int font);
void Render2DSentenceDrawSentence(Render2DSentenceClass *r,const wchar_t *sentence,Vector2 *position,unsigned long color);
Render2DTextClass *CreateRender2DTextClass(const char *texture);
Render2DClass *CreateRender2DClass();
#endif
