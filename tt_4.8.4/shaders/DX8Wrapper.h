#ifndef TT_INCLUDE__DX8WRAPPER_H
#define TT_INCLUDE__DX8WRAPPER_H
#include "matrix4.h"
#include "engine_3d.h"
#include "texture.h"
class Vector4;
class Vector3;
class TextureClass;
class LightClass;
struct IDirect3DDevice9;
extern SHADERS_API D3DPRESENT_PARAMETERS _PresentParameters;


typedef void (*__CreateDX8Caps) (D3DCAPS9* caps, D3DADAPTER_IDENTIFIER9& identifier);

extern __CreateDX8Caps CreateDX8Caps;

SHADERS_API void SetDX8CapsCreateFunction(__CreateDX8Caps func);

//-------------------------------------------------------------------------------------

#define RDS_SETSTATE(a, b) if ((a) != (b)) { a = b; SettingsDirty = true; }

class SHADERS_API RenderDeviceSettings
{
private:
	static bool		SettingsDirty;

	static char		Name[512];			// only used for ingame display as it is non-unique
	static char		Identifier[40];		// guid
	
	static int		ResolutionWidth;
	static int		ResolutionHeight;
 	static int		BitDepth;
	static int		TextureBitDepth;
	static bool		IsWindowed;

	static bool		VSyncEnabled;
	static int		MultisampleMode;	// index into an array of MultisampleModeDefinitions

public:
	static bool Registry_Load(const char* sub_key);
	static bool Registry_Save(const char* sub_key);	

	TT_INLINE static bool Are_Settings_Dirty() { return SettingsDirty; };
	TT_INLINE static void Set_Settings_Dirty(bool state = true) { SettingsDirty = state; };

	TT_INLINE static const char* Peek_Name() { return Name; };
	TT_INLINE static const char* Peek_Identifier() { return Identifier;	};

	TT_INLINE static int Get_Resolution_Width() { return ResolutionWidth; };
	TT_INLINE static int Get_Resolution_Height() { return ResolutionHeight; };
	TT_INLINE static int Get_Bit_Depth() { return BitDepth; };
 	TT_INLINE static int Get_Texture_Bit_Depth() { return TextureBitDepth; };
	TT_INLINE static bool Is_Windowed() { return IsWindowed; };

	TT_INLINE static bool VSync_Enabled() { return VSyncEnabled; };
	TT_INLINE static int Get_Multisample_Mode() { return MultisampleMode; };
									
	TT_INLINE static void Set_Resolution_Width(int width) { RDS_SETSTATE(ResolutionWidth, width); };
	TT_INLINE static void Set_Resolution_Height(int height) { RDS_SETSTATE(ResolutionHeight, height); };
	TT_INLINE static void Set_Bit_Depth(int bitdepth) { RDS_SETSTATE(BitDepth, bitdepth); };
	TT_INLINE static void Set_Texture_Bit_Depth(int bitdepth) { RDS_SETSTATE(TextureBitDepth, bitdepth); };
	TT_INLINE static void Set_Windowed(bool windowed) { RDS_SETSTATE(IsWindowed, windowed); };

	TT_INLINE static void Set_VSync_Enabled(bool enable) { RDS_SETSTATE(VSyncEnabled, enable); };
	TT_INLINE static void Set_Multisample_Mode(int mode) { RDS_SETSTATE(MultisampleMode, mode); };
};

#undef RDS_SETSTATE

class DynamicVBAccessClass;
class DynamicIBAccessClass;

class SHADERS_API DX8Wrapper
{
private:
	static bool _EnableTriangleDraw;
public:

	static UINT ShadowWidth;
	static UINT ShadowHeight;
	static UINT vsync;
	static UINT AA;
	static IDirect3D9* D3DInterface;

	static int CurRenderDevice;
	static IDirect3DDevice9* D3DDevice;

	static IDirect3DSurface9*	DefaultRenderTarget;
	static IDirect3DSurface9*	DefaultDepthBuffer;

	static int					CurrentRenderTargetIndex;
	static IDirect3DSurface9*	CurrentRenderTargets[16]; 
	static IDirect3DSurface9*	CurrentDepthBuffers[16];

	static float ZFar;
	static float ZNear;
	static REF_DECL1(IsRenderToTexture, bool);
	static int ResolutionWidth;
	static int ResolutionHeight;
	static int BitDepth;
	static bool IsWindowed;
	static bool IsDeviceLost;
	static REF_DECL1(_MainThreadID, int);
	static int FrameCount;
	static REF_DECL1(IsInitted, bool);
	static REF_DECL1(render_state_changed, unsigned int);
	static REF_DECL1(render_state, RenderStateStruct);
	static unsigned long FogColor;
	static bool FogEnable;
	static int ZBias;
	static Matrix4 ProjectionMatrix;

	static void Flip_To_Primary();

	static bool Init(HWND hwnd, bool windowed);
	static void Shutdown();

	static void Do_Onetime_Device_Dependent_Inits();
	static void Do_Onetime_Device_Dependent_Shutdowns();

	static void Enumerate_Devices();

	static bool Set_Device_Resolution(int width,int height,int bits,int windowed,bool resize_window);
	static void Get_Device_Resolution(int& set_w, int& set_h, int& set_bits, bool& set_windowed);
	static void Get_Render_Target_Resolution(int& set_w, int& set_h, int& set_bits, bool& set_windowed);

	static bool Set_Render_Device(const char* dev_name, const char* guid, int width, int height, int bits, int windowed, bool resize_window);
	static bool Set_Render_Device(int dev, int width, int height, int bits, int windowed, bool resize_window);
	static bool Set_Any_Render_Device();

	static int Get_Render_Device()
	{
		return CurRenderDevice;
	};
	static const char* Get_Render_Device_Name(int device);

	static bool Registry_Save_Render_Device(const char* sub_key);
	static bool Registry_Save_Render_Device(const char* sub_key, int device, int width, int height, int depth, bool windowed, int texture_depth);
	static bool Registry_Load_Render_Device(const char* sub_key, bool resize_window);
	static bool Registry_Load_Render_Device(const char* sub_key, char* device, int device_len, char* guid, int &width, int &height, int &depth, int &windowed, int &texture_depth);

	static bool Test_Color_Mode(D3DFORMAT colorbuffer, unsigned int resx, unsigned int resy);
	static bool Test_Z_Mode(D3DFORMAT colorbuffer, D3DFORMAT backbuffer, D3DFORMAT zmode);
	static bool Find_Z_Mode(D3DFORMAT colorbuffer, D3DFORMAT backbuffer, D3DFORMAT* zmode);
	static bool Find_Color_And_Z_Mode(int resx, int resy, int bitdepth, D3DFORMAT* set_colorbuffer, D3DFORMAT* set_zmode);

	static bool Create_Device();

	static void Set_Render_Target(IDirect3DSurface9 *render_target, bool depthbuffer);
	static void Set_Render_Target(TextureClass* texture);
	static void Clamp_Color(Vector4& color);

	static void Get_Transform(D3DTRANSFORMSTATETYPE type, Matrix4& transform);
	static void Set_Transform(D3DTRANSFORMSTATETYPE type, const Matrix4& transform);
	static void Set_Transform(D3DTRANSFORMSTATETYPE type, const Matrix3D& transform);

	static void Set_Projection_Transform_With_Depth_Bias(const Matrix4& transform, float znear, float zfar);

	static void Draw(uint primitiveType, uint16 startIndex, uint16 polygonCount, uint16 minVertexIndex, uint16 vertexCount);
	static void Draw_Triangles(uint16 startIndex, uint16 polygonCount, uint16 minVertexIndex, uint16 vertexCount);
	static void Draw_Strip(uint16 startIndex, uint16 polygonCount, uint16 minVertexIndex, uint16 vertexCount);

	static void Clear(bool clearColor, bool clearZStencil,const Vector3& color, float z, unsigned int stencil);

	static void Set_Vertex_Buffer(const VertexBufferClass* vb);
	static void Set_Vertex_Buffer(const DynamicVBAccessClass& vba);
	static void Set_Index_Buffer(const IndexBufferClass* ib,unsigned short index_base_offset);
	static void Set_Index_Buffer(const DynamicIBAccessClass& iba,unsigned short index_base_offset);

	static void Set_DX8_ZBias(int zbias);
	static bool Reset_Device();

	static void _Enable_Triangle_Draw(bool enable);
	static bool _Is_Triangle_Draw_Enabled();

	static void Set_Light_Environment(LightEnvironmentClass *light_env);
	static void Set_Light(int pos, D3DLIGHT9 *light);
	static void Set_Light(unsigned index,const LightClass &light);

	static TextureClass *Create_Render_Target_NonPow2(int width, int height, WW3DFormat format);
	static TextureClass *Create_Render_Target(int width, int height, WW3DFormat format);
	static void ClearZ();
	static void Begin_Scene();
	static void End_Scene(bool);
	static IDirect3DTexture9 *Create_DX8_Texture(unsigned int width,unsigned int height,WW3DFormat format,TextureClass::MipCountType mip_level_count,D3DPOOL pool,bool rendertarget);
	static IDirect3DTexture9 *_Create_DX8_Texture(IDirect3DSurface9* surface,TextureClass::MipCountType mip_level_count);
	static void Set_Render_State(RenderStateStruct &state);
	static void Release_Render_State();
	static IDirect3DSurface9 *Create_DX8_Surface(unsigned int width,unsigned int height,WW3DFormat format);
	static IDirect3DSurface9 *_Create_DX8_Surface(const char *name);
	static void Set_Gamma(float gamma,float bright,float contrast,bool,bool);
	static void Set_Alpha(const float alpha, unsigned int &color)
	{
		unsigned char *component = (unsigned char*)&color;
		component[3] = (unsigned char)(255.0f * alpha);
	}
	static void Set_Material(const VertexMaterialClass* material)
	{
		if (material==render_state.material) return;
		REF_PTR_SET(render_state.material,const_cast<VertexMaterialClass*>(material));
		render_state_changed|=MATERIAL_CHANGED;
	}
	static void Set_Texture(unsigned stage,TextureClass* texture)
	{
		if (texture==render_state.Textures[stage]) return;
		REF_PTR_SET(render_state.Textures[stage],texture);
		render_state_changed|=(TEXTURE0_CHANGED<<stage);
	}
	static void Set_Shader(const ShaderClass& shader)
	{
		if (!ShaderClass::ShaderDirty && ((unsigned&)shader==(unsigned&)render_state.shader)) return;
		render_state.shader=shader;
		render_state_changed|=SHADER_CHANGED;
	}
	static void Set_World_Identity()
	{
		if (render_state_changed&(unsigned)WORLD_IDENTITY) return;
		render_state.world.Make_Identity();
		render_state_changed|=(unsigned)WORLD_CHANGED|(unsigned)WORLD_IDENTITY;
	}
};



#endif
