#ifndef TT_INCLUDE__WW3D_H
#define TT_INCLUDE__WW3D_H
#include "engine_3dre.h"
#include "texture.h"
#include "DX8Wrapper.h"

typedef void (*updfunc)(void);
class LayerClass;
class SceneClass;
class CameraClass;
class Vector3;
class RenderObjClass;
class RenderInfoClass;
class RenderDeviceDescClass;
template<typename T> class List;
template<typename T> class RefMultiListClass;

class SHADERS_API WW3D
{

public:
	enum MeshDrawModeEnum {
		MESH_DRAW_MODE_OLD = 0x0,
		MESH_DRAW_MODE_NEW = 0x1,
		MESH_DRAW_MODE_DEBUG_DRAW = 0x2,
		MESH_DRAW_MODE_DEBUG_CLIP = 0x3,
		MESH_DRAW_MODE_DEBUG_BOX = 0x4,
		MESH_DRAW_MODE_NONE = 0x5,
		MESH_DRAW_MODE_DX8_ONLY = 0x6,
	};
	enum NPatchesGapFillingModeEnum;
	static REF_DECL2(IsSortingEnabled, bool);
	static REF_DECL1(PreviousSyncTime, unsigned int);
	static REF_DECL1(AreStaticSortListsEnabled, bool);
	static REF_DECL1(CurrentStaticSortLists, RefMultiListClass<RenderObjClass>*);
	static REF_DECL1(DefaultStaticSortLists, RefMultiListClass<RenderObjClass>*);
	static REF_DECL1(MinStaticSortLevel, unsigned int);
	static REF_DECL1(MaxStaticSortLevel, unsigned int);
	static REF_DECL1(DecalRejectionDistance, float);
	static TextureClass::TextureFilterMode TextureFilter;
	static bool ThumbnailEnabled;
	static bool IsInitted;
	static REF_DECL1(IsScreenUVBiased, bool);
	static bool IsTexturingEnabled;
	static REF_DECL1(SyncTime, unsigned int);
	static REF_DECL1(FrameCount, unsigned int);
	static REF_DECL1(NPatchesLevel, float);
	static REF_DECL1(PrelitMode, unsigned int);
	static bool IsRendering;
	static REF_DECL1(MeshDrawMode, MeshDrawModeEnum);

	static WW3DErrorType Render(List<LayerClass*> const&);
	static WW3DErrorType Render(LayerClass const&);
	static WW3DErrorType Render(SceneClass*, CameraClass*, bool, bool, Vector3 const&);
	static WW3DErrorType Render(RenderObjClass&, RenderInfoClass&);
	static void Flush(RenderInfoClass&);
	static WW3DErrorType End_Render(bool);
	static void Sync(uint);
	static void Set_Ext_Swap_Interval(long);
	static long Get_Ext_Swap_Interval();
	static void Set_Collision_Box_Display_Mask(int);
	static int Get_Collision_Box_Display_Mask();
	static void Make_Screen_Shot(char const*);
	static void Add_To_Static_Sort_List(RenderObjClass*, uint);
	static void Set_NPatches_Gap_Filling_Mode(NPatchesGapFillingModeEnum);
	static void Set_NPatches_Level(uint);
	static WW3DErrorType Init(HWND, char*, bool);
	static WW3DErrorType Shutdown();
	static WW3DErrorType Set_Render_Device(char const*, int, int, int, int, bool);
	static WW3DErrorType Set_Any_Render_Device();
	static WW3DErrorType Set_Render_Device(int, int, int, int, int, bool);
	static WW3DErrorType Set_Next_Render_Device();
	static HWND Get_Window();
	static bool Is_Windowed();
	static WW3DErrorType Toggle_Windowed();
	static int Get_Render_Device();
	static RenderDeviceDescClass& Get_Render_Device_Desc(int);
	static int Get_Render_Device_Count();
	static const char* Get_Render_Device_Name(int);
	static WW3DErrorType Set_Device_Resolution(int, int, int, int, bool);
	static void Get_Render_Target_Resolution(int&, int&, int&, bool&);
	static void Get_Device_Resolution(int&, int&, int&, bool&);
	static WW3DErrorType Registry_Save_Render_Device(char const*);
	static WW3DErrorType Registry_Save_Render_Device(char const*, int, int, int, int, bool, int);
	static WW3DErrorType Registry_Load_Render_Device(char const*, bool);
	static bool Registry_Load_Render_Device(char const*, char*, int, int&, int&, int&, int&, int&);
	static void _Invalidate_Mesh_Cache();
	static void _Invalidate_Textures();
	static void Set_Texture_Filter(int mode);
	static const wchar_t* Get_Texture_Filter_Name();
	static void Flip_To_Primary();
	static unsigned int Get_Last_Frame_Poly_Count();
	static unsigned int Get_Last_Frame_Vertex_Count();
	static void Normalize_Coordinates(int, int, float&, float&);
	static void Start_Movie_Capture(char const*, float);
	static void Stop_Movie_Capture();
	static void Toggle_Movie_Capture(char const*, float);
	static void Start_Single_Frame_Movie_Capture(char const*);
	static void Capture_Next_Movie_Frame();
	static void Pause_Movie(bool);
	static bool Is_Movie_Paused();
	static bool Is_Recording_Next_Frame();
	static bool Is_Movie_Ready();
	static void Update_Movie_Capture();
	static float Get_Movie_Capture_Frame_Rate();
	static void Set_Texture_Reduction(int);
	static void Enable_Texturing(bool);
	static int Get_Texture_Reduction();
	static VertexMaterialClass* Peek_Default_Debug_Material();
	static ShaderClass Peek_Default_Debug_Shader();
	static ShaderClass Peek_Lightmap_Debug_Shader();
	static void Allocate_Debug_Resources();
	static void Release_Debug_Resources();
	static WW3DErrorType On_Deactivate_App();
	static WW3DErrorType On_Activate_App();
	static void Get_Pixel_Center(float&, float&);
	static void Update_Pixel_Center();
	static void Set_Texture_Bitdepth(int new_bitdepth)
	{
		RenderDeviceSettings::Set_Texture_Bit_Depth(new_bitdepth);
	};
	static int Get_Texture_Bitdepth()
	{
		return RenderDeviceSettings::Get_Texture_Bit_Depth();
	};
	static void Render_And_Clear_Static_Sort_Lists(RenderInfoClass&);
	static void Enable_Sorting(bool);
	static void Override_Current_Static_Sort_Lists(RefMultiListClass<RenderObjClass>*, uint, uint);
	static void Reset_Current_Static_Sort_Lists_To_Default();
	static WW3DErrorType Begin_Render(bool, bool, const Vector3&, updfunc);
	static bool Are_Static_Sort_Lists_Enabled()
	{
		return AreStaticSortListsEnabled;
	}
	static void Enable_Static_Sort_Lists(bool enabled)
	{
		AreStaticSortListsEnabled = enabled;
	}
	static unsigned int Get_Frame_Time()
	{
		return SyncTime - PreviousSyncTime;
	}
};

#endif
