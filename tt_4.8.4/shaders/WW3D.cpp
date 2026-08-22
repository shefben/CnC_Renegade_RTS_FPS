#include "General.h"
#include "WW3D.h"
#include "DX8Wrapper.h"
#include "RenderObjClass.h"
#include "WW3DAssetManager.h"
#include "textureloader.h"
#include "HashTemplateIterator.h"
#include "DazzleRenderObjClass.h"
#include "dx8texman.h"
#include <mmsystem.h>

REF_DEF2(WW3D::IsSortingEnabled, bool, 0x007FD64C, 0x007FC824);
REF_DEF1(WW3D::AreStaticSortListsEnabled, bool, 0x00830488);
REF_DEF1(WW3D::CurrentStaticSortLists, RefMultiListClass<RenderObjClass>*, 0x008304A0);
REF_DEF1(WW3D::DefaultStaticSortLists, RefMultiListClass<RenderObjClass>*, 0x008304A4);
REF_DEF1(WW3D::MinStaticSortLevel, unsigned int, 0x007FD658);
REF_DEF1(WW3D::MaxStaticSortLevel, unsigned int, 0x007FD65C);
REF_DEF1(WW3D::SyncTime, unsigned int, 0x00830474);
REF_DEF1(WW3D::PreviousSyncTime, unsigned int, 0x00830478);
REF_DEF1(WW3D::FrameCount, unsigned int, 0x00830490);
REF_DEF1(WW3D::NPatchesLevel, float, 0x007FD66C);
REF_DEF1(WW3D::PrelitMode, unsigned int, 0x007FD660);
REF_DEF1(WW3D::MeshDrawMode, WW3D::MeshDrawModeEnum, 0x008304B0);
REF_DEF1(WW3D::DecalRejectionDistance, float, 0x007FD650);

bool WW3D::IsInitted;
bool WW3D::IsTexturingEnabled = true;
TextureClass::TextureFilterMode WW3D::TextureFilter = TextureClass::FILTER_MODE_BILINEAR;
bool WW3D::IsRendering;
bool WW3D::ThumbnailEnabled = true;
REF_DEF1(_TextureReduction, int, 0x008304B8);
REF_DEF1(WW3D::IsScreenUVBiased, bool, 0x00830487);

int WW3D::Get_Render_Device()
{
	return DX8Wrapper::CurRenderDevice;
};

void WW3D::Set_Texture_Filter(int mode)
{
	if ((mode > TextureClass::FILTER_MODE_ANISOTROPIC16X) || (mode < TextureClass::FILTER_MODE_BILINEAR)) mode = TextureClass::FILTER_MODE_BILINEAR;
	TextureFilter = (TextureClass::TextureFilterMode)mode;
	TextureClass::_Init_Filters(TextureFilter);
}

const wchar_t* WW3D::Get_Texture_Filter_Name()
{
	if ((TextureFilter > TextureClass::FILTER_MODE_ANISOTROPIC16X) || (TextureFilter < TextureClass::FILTER_MODE_BILINEAR)) TextureFilter = TextureClass::FILTER_MODE_BILINEAR;
	switch (TextureFilter)
	{
	case TextureClass::FILTER_MODE_BILINEAR:		return L"Bilinear";
	case TextureClass::FILTER_MODE_TRILINEAR:		return L"Trilinear";
	case TextureClass::FILTER_MODE_ANISOTROPIC2X:	return L"Anisotropic 2X";
	case TextureClass::FILTER_MODE_ANISOTROPIC4X:	return L"Anisotropic 4X";
	case TextureClass::FILTER_MODE_ANISOTROPIC8X:	return L"Anisotropic 8X";
	case TextureClass::FILTER_MODE_ANISOTROPIC16X:	return L"Anisotropic 16X";
	DEFAULT_UNREACHABLE;
	};
};

const char* WW3D::Get_Render_Device_Name(int device)
{
	return DX8Wrapper::Get_Render_Device_Name(device);
};

void WW3D::Get_Render_Target_Resolution(int& set_w,int& set_h,int& set_bits,bool& set_windowed)
{
	return DX8Wrapper::Get_Render_Target_Resolution(set_w, set_h, set_bits, set_windowed);
}

void WW3D::Get_Device_Resolution(int& width, int& height, int& bit, bool& windowed)
{
	return DX8Wrapper::Get_Device_Resolution(width, height, bit, windowed);
}

WW3DErrorType WW3D::Registry_Save_Render_Device(char const* sub_key)
{
	return DX8Wrapper::Registry_Save_Render_Device(sub_key) ? WW3D_ERROR_OK : WW3D_ERROR_INITIALIZATION_FAILED ;
};

WW3DErrorType WW3D::Registry_Load_Render_Device(const char* sub_key, bool resize_window)
{
	return DX8Wrapper::Registry_Load_Render_Device(sub_key, resize_window) ? WW3D_ERROR_OK : WW3D_ERROR_INITIALIZATION_FAILED;
};

void WW3D::Add_To_Static_Sort_List(RenderObjClass* robj, unsigned int sort_level)
{
	if ((sort_level >= 1) && (sort_level <= 0x20))
	{
		CurrentStaticSortLists[sort_level].Add_Tail(robj);
	}
}

void WW3D::Set_Texture_Reduction(int reduction)
{
	if (_TextureReduction != reduction)
	{
		_TextureReduction = reduction;
		WW3D::_Invalidate_Textures();
	}
}

void WW3D::_Invalidate_Textures()
{
	if (WW3DAssetManager::TheInstance)
	{
		TextureLoader::Flush_Pending_Load_Tasks();
		for (HashTemplateIterator<StringClass, TextureClass*> textureIter(WW3DAssetManager::TheInstance->TextureHash); textureIter; ++textureIter)
		{
			TextureClass* texture = textureIter.getValue();
			texture->Invalidate();
		}
	}
}

RENEGADE_FUNCTION
void WW3D::_Invalidate_Mesh_Cache()
AT1(0x00529AA0);

RENEGADE_FUNCTION
int WW3D::Get_Texture_Reduction()
AT1(0x0052ABA0);

RENEGADE_FUNCTION
void PredictiveLODOptimizerClass::Free()
AT1(0x0057BB90);

class AnimatedSoundMgrClass
{
public:
	static void Shutdown();
	static void Initialize(char const *);
};

RENEGADE_FUNCTION
void AnimatedSoundMgrClass::Shutdown()
AT1(0x0057A4E0);

RENEGADE_FUNCTION
void AnimatedSoundMgrClass::Initialize(char const *)
AT1(0x00579FA0);

REF_DEF1(_Hwnd_0, HWND, 0x008304B4);

bool IsConsoleExclusive;

const char *DAZZLE_INI_FILENAME = "DAZZLE.INI";

WW3DErrorType WW3D::Init(HWND hwnd, char *defaultpal, bool isconsoleexclusive)
{
	_Hwnd_0 = hwnd;
	IsConsoleExclusive = isconsoleexclusive;
	if (DX8Wrapper::Init(hwnd,isconsoleexclusive))
	{
		timeBeginPeriod(1);
		if (!isconsoleexclusive)
		{
			FileClass* file = _TheFileFactory->Get_File(DAZZLE_INI_FILENAME);
			INIClass ini(*file);
			DazzleRenderObjClass::Init_From_INI(&ini);
			_TheFileFactory->Return_File(file);
		}
		DefaultStaticSortLists = new RefMultiListClass<RenderObjClass>[33];
		Reset_Current_Static_Sort_Lists_To_Default();
		if (!isconsoleexclusive)
		{
			AnimatedSoundMgrClass::Initialize(0);
			WW3D::IsInitted = true;
		}
		return WW3D_ERROR_OK;
	}
	else
	{
		return WW3D_ERROR_INITIALIZATION_FAILED;
	}
}

WW3DErrorType WW3D::Shutdown()
{
	PredictiveLODOptimizerClass::Free();
	if (!IsConsoleExclusive)
	{
		DazzleRenderObjClass::Deinit();
	}
	if (WW3DAssetManager::TheInstance)
	{
		WW3DAssetManager::TheInstance->Free_Assets();
	}
	DX8TextureManagerClass::Shutdown();
	if (!IsConsoleExclusive)
	{
		DX8Wrapper::Shutdown();
	}
	if (DefaultStaticSortLists)
	{
		delete[] DefaultStaticSortLists;
	}
	AnimatedSoundMgrClass::Shutdown();
	IsInitted = false;
	return WW3D_ERROR_OK;
}

void WW3D::Reset_Current_Static_Sort_Lists_To_Default()
{
	CurrentStaticSortLists = DefaultStaticSortLists;
	MinStaticSortLevel = 1;
	MaxStaticSortLevel = 32;
}
