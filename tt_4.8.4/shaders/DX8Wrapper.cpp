#include "General.h"
#include "DX8Wrapper.h"
#include "engine_3d.h"
#include "shaderstatemanager.h"
#include "texture.h"
#include "ww3d.h"
#include "vertexbuffer.h"
#include "indexbuffer.h"
#include "RegistryClass.h"
#include "DX8MeshRendererClass.h"
#include "shader.h"
#include "sortingrenderer.h"
#include "dx8texman.h"
#include "missingtexture.h"
#include "textureloader.h"
#include "LightClass.h"
#include "PointGroupClass.h"
#include "BoxRenderObjClass.h"
void ReleaseResources();
void ReloadResources();
void DestroyResources();

//-----------------------------------------------------
// Direct3D8 wrapper functions
//-----------------------------------------------------
__CreateDX8Caps CreateDX8Caps;
void SetDX8CapsCreateFunction(__CreateDX8Caps func)
{
	CreateDX8Caps = func;
}


//-----------------------------------------------------
// Render device settings
//-----------------------------------------------------
UINT DX8Wrapper::ShadowWidth;
UINT DX8Wrapper::ShadowHeight;
UINT DX8Wrapper::vsync;
UINT DX8Wrapper::AA;
bool RenderDeviceSettings::SettingsDirty =		false;

char RenderDeviceSettings::Name[512] =			{0};
char RenderDeviceSettings::Identifier[40] =		{0};

int RenderDeviceSettings::ResolutionWidth =		800;
int RenderDeviceSettings::ResolutionHeight =	600;
int RenderDeviceSettings::BitDepth =			32;
int RenderDeviceSettings::TextureBitDepth =		16;
bool RenderDeviceSettings::IsWindowed =			false;

bool RenderDeviceSettings::VSyncEnabled =		true;
int RenderDeviceSettings::MultisampleMode =		0;

bool RenderDeviceSettings::Registry_Load(const char* sub_key)
{
	RegistryClass registry(sub_key, false);
	if (registry.bIsValid)
	{
		registry.Get_String("RenderDeviceName",			Name, sizeof(Name), 0);
		registry.Get_String("RenderDeviceIdentifier",	Identifier, sizeof(Identifier), 0);

		ResolutionWidth =	registry.Get_Int("RenderDeviceWidth",			ResolutionWidth);
		ResolutionHeight =	registry.Get_Int("RenderDeviceHeight",			ResolutionHeight);
		BitDepth =			registry.Get_Int("RenderDeviceDepth",			BitDepth) == 16 ? 16 : 32;
		TextureBitDepth =	registry.Get_Int("RenderDeviceTextureDepth",	TextureBitDepth) == 16 ? 16 : 32;
		IsWindowed =		registry.Get_Bool("RenderDeviceWindowed",		IsWindowed);

		VSyncEnabled =		registry.Get_Bool("RenderDeviceForceVSync",		VSyncEnabled);
		MultisampleMode	=   registry.Get_Int("RenderDeviceMultisampleMode", MultisampleMode);
		return true;
	}
	else
	{
		return false;
	}
};

bool RenderDeviceSettings::Registry_Save(const char* sub_key)
{
	RegistryClass registry(sub_key, true);
	if (registry.bIsValid)
	{
		registry.Set_String("RenderDeviceName",			Name);
		registry.Set_String("RenderDeviceIdentifier",	Identifier);
		registry.Set_Int("RenderDeviceWidth",			ResolutionWidth);
		registry.Set_Int("RenderDeviceHeight",			ResolutionHeight);
		registry.Set_Int("RenderDeviceDepth",			BitDepth);
		registry.Set_Int("RenderDeviceTextureDepth",	TextureBitDepth);
		registry.Set_Bool("RenderDeviceWindowed",		IsWindowed);

		registry.Set_Bool("RenderDeviceForceVSync",		VSyncEnabled);
		registry.Set_Int("RenderDeviceMultisampleMode",	MultisampleMode);
		return true;
	}
	else
	{
		return false;
	}
};

//-----------------------------------------------------
D3DFORMAT WW3DFormat_To_D3DFormat(WW3DFormat Format);
DynamicVectorClass<StringClass> _RenderDeviceNameTable;
DynamicVectorClass<StringClass> _RenderDeviceGUIDTable;

IDirect3D9* DX8Wrapper::D3DInterface;
int DX8Wrapper::CurRenderDevice;
D3DDEVTYPE _CurrentDeviceType = D3DDEVTYPE_HAL; // May be REF if PerfHUD is active
IDirect3DDevice9 *DX8Wrapper::D3DDevice;

bool DX8Wrapper::IsDeviceLost = false;
bool DX8Wrapper::_EnableTriangleDraw = true;
float DX8Wrapper::ZFar;
float DX8Wrapper::ZNear;
Matrix4 DX8Wrapper::ProjectionMatrix = Matrix4::IDENTITY;
IDirect3DSurface9*	DX8Wrapper::DefaultRenderTarget;
IDirect3DSurface9*	DX8Wrapper::DefaultDepthBuffer;

int					DX8Wrapper::CurrentRenderTargetIndex = 0;
IDirect3DSurface9*	DX8Wrapper::CurrentRenderTargets[16] = {}; 
IDirect3DSurface9*	DX8Wrapper::CurrentDepthBuffers[16] = {};

REF_DEF1(DX8Wrapper::IsRenderToTexture, bool, 0x00831484);
int DX8Wrapper::ResolutionWidth;
int DX8Wrapper::ResolutionHeight;
int DX8Wrapper::BitDepth;
bool DX8Wrapper::IsWindowed;
REF_DEF1(DX8Wrapper::_MainThreadID, int, 0x008314A8);
D3DPRESENT_PARAMETERS _PresentParameters;
int DX8Wrapper::ZBias = 0;
int DX8Wrapper::FrameCount = 0;
REF_DEF1(DX8Wrapper::IsInitted, bool, 0x00831468);
REF_DEF1(DX8Wrapper::render_state_changed, unsigned int, 0x00830728);
REF_DEF1(DX8Wrapper::render_state, RenderStateStruct, 0x008310E0);

unsigned long DX8Wrapper::FogColor;
bool DX8Wrapper::FogEnable;

inline int GetFormatBitDepth(D3DFORMAT fmt)
{
	switch(fmt)
	{
		case D3DFMT_R8G8B8:
		case D3DFMT_A8R8G8B8:
		case D3DFMT_X8R8G8B8:
			return 32;
		case D3DFMT_R5G6B5:
		case D3DFMT_A1R5G5B5:
		case D3DFMT_A4R4G4B4:
			return 16;
		case D3DFMT_A8:
		case D3DFMT_P8:
		case D3DFMT_L8:
			return 8;
		default:
			return -1;
	}
};

inline char* GUIDToString(const GUID* guid, char* str, size_t sz)
{
    _snprintf(str, sz, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             guid->Data1, guid->Data2, guid->Data3,
             guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
             guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
    return str;
};

REF_DEF1(_Hwnd, HWND, 0x00831464);
bool DX8Wrapper::Init(HWND hwnd, bool IsConsoleExclusive)
{
	_Hwnd = hwnd;
	_MainThreadID = GetCurrentThreadId();
	CurRenderDevice = -1;
	ResolutionWidth = 800;
	ResolutionHeight = 600;
	Render2DClass::Set_Screen_Resolution(RectClass(0, 0, 800, 600));
	BitDepth = 32;
	IsWindowed = false;
	ShaderClass::ShaderDirty = true;
	D3DDevice = NULL;
	D3DInterface = Direct3DCreate9(D3D_SDK_VERSION);
	if (!D3DInterface)
	{
		return false;
	}
	IsInitted = true;
	Enumerate_Devices();
	return true;
};

class ShatterSystem
{
public:
	static void Init();
	static void Shutdown();
};

RENEGADE_FUNCTION
void ShatterSystem::Init()
AT1(0x005A7DE0);

RENEGADE_FUNCTION
void ShatterSystem::Shutdown()
AT1(0x005A7FA0);

RENEGADE_FUNCTION
void VertexMaterialClass::Shutdown()
AT1(0x0058C360);

RENEGADE_FUNCTION
void BoxRenderObjClass::Shutdown()
AT1(0x0057C5F0);

void DX8Wrapper::Do_Onetime_Device_Dependent_Shutdowns()
{
	REF_PTR_RELEASE(render_state.vertex_buffer);
	REF_PTR_RELEASE(render_state.index_buffer);
	REF_PTR_RELEASE(render_state.material);
	REF_PTR_RELEASE(render_state.Textures[0]);
	REF_PTR_RELEASE(render_state.Textures[1]);
	TextureLoader::Deinit();
	SortingRendererClass::Deinit();
	DynamicVBAccessClass::_Deinit();
	DynamicIBAccessClass::_Deinit();
	ShatterSystem::Shutdown();
	PointGroupClass::_Shutdown();
	VertexMaterialClass::Shutdown();
	BoxRenderObjClass::Shutdown();
	TheDX8MeshRenderer.Shutdown();
	MissingTexture::_Deinit();
}

void DX8Wrapper::Shutdown()
{
	if (D3DDevice)
	{
		DestroyResources();
		Set_Render_Target(NULL, false);
		Do_Onetime_Device_Dependent_Shutdowns();
		SafeRelease(D3DDevice);
		SafeRelease(D3DInterface);
		_RenderDeviceNameTable.Clear();
		_RenderDeviceGUIDTable.Clear();
		IsInitted = false;
	}
}


RENEGADE_FUNCTION
void BoxRenderObjClass::Init()
AT1(0x0057C550);
RENEGADE_FUNCTION
void VertexMaterialClass::Init()
AT1(0x0058C300);

void InitShaders();
void DX8Wrapper::Do_Onetime_Device_Dependent_Inits()
{
	InitShaders();	
	MissingTexture::_Init();
	TheDX8MeshRenderer.Init();
	BoxRenderObjClass::Init();
	VertexMaterialClass::Init();
	PointGroupClass::_Init();
	ShatterSystem::Init();
	TextureLoader::Init();
}

void DX8Wrapper::Clamp_Color(Vector4& color)
{
	// TODO: This used to use the cmov instruction if availible as optimisation

	color.X = clamp(color.X, 0.f, 1.f);
	color.Y = clamp(color.Y, 0.f, 1.f);
	color.Z = clamp(color.Z, 0.f, 1.f);
	color.W = clamp(color.W, 0.f, 1.f);
}

void DX8Wrapper::Enumerate_Devices()
{
	unsigned int adapter_count = D3DInterface->GetAdapterCount();
	for (unsigned int adapter_index = 0; adapter_index < adapter_count; ++adapter_index)
	{
		D3DADAPTER_IDENTIFIER9 id = {0};

		if (FAILED(D3DInterface->GetAdapterIdentifier(adapter_index, 0, &id))) continue;
	
		int display = 0;
		if (!sscanf(id.DeviceName,"\\\\.\\DISPLAY%d",&display))
		{
			display = 0;
		}

		char devname[MAX_DEVICE_IDENTIFIER_STRING];
		strrtrim(id.Description);
		if (display > 1)
		{
			sprintf_s(devname, 512, "%s - Display %d", id.Description, display);
		}
		else
		{
			memcpy(devname, id.Description, sizeof(devname));
		}

		StringClass Description(devname);
		_RenderDeviceNameTable.Add(Description);
		char _guid[40];
		StringClass guid(GUIDToString(&id.DeviceIdentifier, _guid, sizeof(_guid)));
		_RenderDeviceGUIDTable.Add(guid);
	}
}

void DX8Wrapper::Get_Device_Resolution(int& set_w, int& set_h, int& set_bits, bool& set_windowed)
{
	set_w =			ResolutionWidth;
	set_h =			ResolutionHeight;
	set_bits =		BitDepth;
	set_windowed =	IsWindowed;
};

bool DX8Wrapper::Set_Render_Device(const char* dev_name, const char* guid, int width, int height, int bits, int windowed, bool resize_window)
{
	for (int i = 0; i < _RenderDeviceNameTable.Count(); i++)
	{
		if(!_stricmp(guid, _RenderDeviceGUIDTable[i]))
		{
			return Set_Render_Device(i, width, height, bits, windowed, resize_window);
		};

		// backcompat for now, really should complain and launch (our) wwconfig
		if(!_stricmp(dev_name,_RenderDeviceNameTable[i]))
		{
			return Set_Render_Device(i, width, height, bits, windowed, resize_window);
		}

	}
	return false;
}

bool DX8Wrapper::Set_Render_Device(int dev, int width, int height, int bits, int windowed, bool resize_window)
{
	if (CurRenderDevice == -1 && dev == -1) CurRenderDevice = 0;
	if (dev != -1)		CurRenderDevice = dev;
	if (width != -1)	ResolutionWidth = width;
	if (height != -1)	ResolutionHeight = height;
	if (bits != -1)		BitDepth = bits;
	if (windowed != -1)	IsWindowed = windowed != 0;

	RectClass r(0, 0, (float)ResolutionWidth, (float)ResolutionHeight);
	Render2DClass::Set_Screen_Resolution(r);

	D3DDISPLAYMODE dmode;
	D3DInterface->GetAdapterDisplayMode(CurRenderDevice, &dmode);

	_PresentParameters.BackBufferWidth =	ResolutionWidth;
	_PresentParameters.BackBufferHeight =	ResolutionHeight;
	_PresentParameters.BackBufferFormat =	dmode.Format;
	_PresentParameters.BackBufferCount =	2 - (IsWindowed != 0); // Doesn't really make sense, but copying it anyways for now

	_PresentParameters.SwapEffect =			D3DSWAPEFFECT_DISCARD; // Causes problems with older cards and fullscreen blending, but we don't set those states
	_PresentParameters.hDeviceWindow =		_Hwnd; // Not exactly need, added just for completeness
	_PresentParameters.Windowed =			IsWindowed;
	_PresentParameters.EnableAutoDepthStencil = true;
	_PresentParameters.Flags =				D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL; 
	
	_PresentParameters.FullScreen_RefreshRateInHz = 0; // Updated by Direct3D anyways
	_PresentParameters.PresentationInterval = RenderDeviceSettings::VSync_Enabled() ? D3DPRESENT_INTERVAL_DEFAULT: D3DPRESENT_INTERVAL_IMMEDIATE;

	//TODO Multisampling support

	if (IsWindowed)
	{
		bool styleset = false;
		if (!(GetWindowLong(_Hwnd,GWL_STYLE) & WS_CHILD))
		{
			SetWindowLong(_Hwnd,GWL_STYLE,WS_MINIMIZEBOX|WS_SYSMENU|WS_CAPTION|WS_CLIPCHILDREN);
			styleset = true;
		}
		BitDepth = GetFormatBitDepth(_PresentParameters.BackBufferFormat);
		if (BitDepth == -1) return false;

		if (!Find_Z_Mode(_PresentParameters.BackBufferFormat,_PresentParameters.BackBufferFormat,&_PresentParameters.AutoDepthStencilFormat))
		{
			if (BitDepth == 32)
			{
				BitDepth = 16;
				_PresentParameters.BackBufferFormat = D3DFMT_R5G6B5;
				if (!Find_Z_Mode(D3DFMT_R5G6B5,D3DFMT_R5G6B5,&_PresentParameters.AutoDepthStencilFormat))
				{
					_PresentParameters.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
				}
			}
			else
			{
				_PresentParameters.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
			}
		}
		if (styleset)
		{
			RECT rect;
			rect.top = 0;
			rect.left = 0;
			rect.right = 0;
			rect.bottom = 0;
			GetClientRect(_Hwnd,&rect);
			int style = GetWindowLong(_Hwnd,GWL_STYLE);
			int exstyle = GetWindowLong(_Hwnd,GWL_EXSTYLE);
			rect.right  = ResolutionWidth;
			rect.bottom = ResolutionHeight;
			HMENU menu = GetMenu(_Hwnd);
			AdjustWindowRectEx(&rect,style,menu != 0,exstyle);
			SetWindowPos(_Hwnd,0,0,0,rect.right - rect.left,rect.bottom - rect.top,SWP_NOCOPYBITS|SWP_SHOWWINDOW);
		}
	}
	else
	{
		SetWindowLong(_Hwnd,GWL_STYLE,WS_POPUP);
		int xres = GetSystemMetrics(SM_CXSCREEN);
		int yres = GetSystemMetrics(SM_CYSCREEN);
		SetWindowPos(_Hwnd,HWND_TOPMOST,0,0,yres,xres,SWP_NOCOPYBITS|SWP_SHOWWINDOW);
		Find_Color_And_Z_Mode(ResolutionWidth,ResolutionHeight,BitDepth,&_PresentParameters.BackBufferFormat,&_PresentParameters.AutoDepthStencilFormat);
	}
	if (!_PresentParameters.AutoDepthStencilFormat)
	{
		_PresentParameters.AutoDepthStencilFormat = (D3DFORMAT)((-(BitDepth != 32) & 9) + 71);
	}
	return Create_Device();
}

bool DX8Wrapper::Set_Any_Render_Device()
{
	for (int i = 0;i < _RenderDeviceNameTable.Count();i++)
	{
		if (Set_Render_Device(i, -1, -1, -1, 0, false))
		{
			return true;
		}
	}
	return false;
}

const char* DX8Wrapper::Get_Render_Device_Name(int device)
{
	return (const char*) _RenderDeviceNameTable[device];
};

bool DX8Wrapper::Registry_Save_Render_Device(const char* sub_key)
{
	return RenderDeviceSettings::Registry_Save(sub_key);
};

bool DX8Wrapper::Registry_Save_Render_Device(const char* sub_key, int device, int width, int height, int depth, bool windowed, int texture_depth)
{
	TT_INTERRUPT;
}

bool DX8Wrapper::Registry_Load_Render_Device(const char* sub_key, bool resize_window)
{
	
	if (!RenderDeviceSettings::Registry_Load(sub_key) || !*RenderDeviceSettings::Peek_Name())
	{
		return Set_Any_Render_Device(); 
	};

	if (!*RenderDeviceSettings::Peek_Identifier())
	{
		//TODO Migrate old settings based on 'best guess', or complain and let the user decide
	}

	//if (ShaderDeveloperMode) //TODO config system for ShaderDeveloperMode
	{
		// PerfHUD mode
		for (int i = 0; i < _RenderDeviceNameTable.Count(); i++)
		{
			if(!_stricmp("NVIDIA PerfHUD", _RenderDeviceNameTable[i]))
			{
				_CurrentDeviceType = D3DDEVTYPE_REF;
				return Set_Render_Device(i, RenderDeviceSettings::Get_Resolution_Width(), RenderDeviceSettings::Get_Resolution_Height(), RenderDeviceSettings::Get_Bit_Depth(), RenderDeviceSettings::Is_Windowed(), resize_window);
			};
		}
	}

	if (Set_Render_Device(RenderDeviceSettings::Peek_Name(), RenderDeviceSettings::Peek_Identifier(), RenderDeviceSettings::Get_Resolution_Width(), RenderDeviceSettings::Get_Resolution_Height(), RenderDeviceSettings::Get_Bit_Depth(), RenderDeviceSettings::Is_Windowed(), resize_window))
	{
		return true;
	}
	return false;
}

bool DX8Wrapper::Registry_Load_Render_Device(const char* sub_key, char* device, int device_len, char* guid, int &width, int &height, int &depth, int &windowed, int &texture_depth)
{
	TT_INTERRUPT;
}

bool DX8Wrapper::Test_Color_Mode(D3DFORMAT colorbuffer, unsigned int resx, unsigned int resy)
{
	D3DDISPLAYMODE dmode;
	uint modes = D3DInterface->GetAdapterModeCount(CurRenderDevice, colorbuffer);
	for (uint mode = 0; mode < modes; mode++)
	{
		D3DInterface->EnumAdapterModes(CurRenderDevice, colorbuffer, mode, &dmode);
		if ((resx == dmode.Width) && (resy == dmode.Height))
		{
			return true;
		};
	};
	return false;
};

bool DX8Wrapper::Test_Z_Mode(D3DFORMAT colorbuffer, D3DFORMAT backbuffer, D3DFORMAT zmode)
{
	if (SUCCEEDED(D3DInterface->CheckDeviceFormat(CurRenderDevice, _CurrentDeviceType, colorbuffer, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, zmode)))
	{
		if (SUCCEEDED(D3DInterface->CheckDepthStencilMatch(CurRenderDevice, _CurrentDeviceType, colorbuffer, backbuffer, zmode)))
		{
			return true;
		}
	}
	return false;
};

bool DX8Wrapper::Find_Z_Mode(D3DFORMAT colorbuffer, D3DFORMAT backbuffer, D3DFORMAT* zmode)
{
	if (Test_Z_Mode(colorbuffer,backbuffer,D3DFMT_D32))
	{
		*zmode = D3DFMT_D32;
		return true;
	}
	if (Test_Z_Mode(colorbuffer,backbuffer,D3DFMT_D24S8))
	{
		*zmode = D3DFMT_D24S8;
		return true;
	}
	if (Test_Z_Mode(colorbuffer,backbuffer,D3DFMT_D24X8))
	{
		*zmode = D3DFMT_D24X8;
		return true;
	}
	if (Test_Z_Mode(colorbuffer,backbuffer,D3DFMT_D24X4S4))
	{
		*zmode = D3DFMT_D24X4S4;
		return true;
	}
	if (Test_Z_Mode(colorbuffer,backbuffer,D3DFMT_D16))
	{
		*zmode = D3DFMT_D16;
		return true;
	}
	if (Test_Z_Mode(colorbuffer,backbuffer,D3DFMT_D15S1))
	{
		*zmode = D3DFMT_D15S1;
		return true;
	}
	return false;
};

bool DX8Wrapper::Find_Color_And_Z_Mode(int resx, int resy, int bitdepth, D3DFORMAT* set_colorbuffer, D3DFORMAT* set_zmode)
{
	static D3DFORMAT _formats16[3] = {D3DFMT_R5G6B5,D3DFMT_X1R5G5B5,D3DFMT_A1R5G5B5};
	static D3DFORMAT _formats32[3] = {D3DFMT_A8R8G8B8,D3DFMT_X8R8G8B8,D3DFMT_R8G8B8};
	D3DFORMAT *format_table = _formats16;
	if (BitDepth != 16)
	{
		format_table = _formats32;
	}
	bool found = false;
	unsigned int mode;
	for (mode = 0;mode < 3;mode++)
	{
		found = Test_Color_Mode(format_table[mode], resx, resy);
		if (found)
		{
			break;
		}
	}
	if (!found)
	{
		return false;
	}
	*set_colorbuffer = format_table[mode];
	return Find_Z_Mode(format_table[mode], format_table[mode], set_zmode);
};


bool DX8Wrapper::Create_Device()
{
	// Stash away the adapter identifier for later (used to create CurrentCaps)
	D3DADAPTER_IDENTIFIER9 identifier;
	if (FAILED(D3DInterface->GetAdapterIdentifier(CurRenderDevice, 0, &identifier))) return false;

	// Get some caps so we know what we are doing
	D3DCAPS9 caps;
	if (FAILED(D3DInterface->GetDeviceCaps(CurRenderDevice, _CurrentDeviceType, &caps))) return false;

	// If the caps say we have hardware T&L, let's use it.
	DWORD flags = caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT ? D3DCREATE_HARDWARE_VERTEXPROCESSING : D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	// And you know what? Let's use a pure device also (if we can)
	if (caps.DevCaps & D3DDEVCAPS_PUREDEVICE) flags |= D3DCREATE_PUREDEVICE;

	// FIXME: Direct3D complains without this
	flags |= D3DCREATE_MULTITHREADED; 

	// If what was chosen earlier doesn't create properly, just return false. 
	// Westwood code did some other stuff if a 16 bit backbuffer was chosen and creation failed, but that's just silly...
	if (FAILED(D3DInterface->CreateDevice(CurRenderDevice, _CurrentDeviceType, _Hwnd, flags, &_PresentParameters, &D3DDevice))) return false;

	// We really need a better place to store these settings...
	vsync = _PresentParameters.PresentationInterval == D3DPRESENT_INTERVAL_IMMEDIATE;
	if (Get_Registry_Int("HighQualityShadows",1))
	{
		ShadowWidth = ShadowHeight = Find_POT_LT( _PresentParameters.BackBufferHeight);
	}
	else
	{
		ShadowWidth = ShadowHeight = 0x100;
	}
	AA = _PresentParameters.MultiSampleType == D3DMULTISAMPLE_NONMASKABLE ? _PresentParameters.MultiSampleQuality + 1 : 0;
	

	// Take care of the DX8Caps instance here
	CreateDX8Caps(&caps, identifier);

	// And do all that fancy "initialization" stuff
	Do_Onetime_Device_Dependent_Inits();

	return true;
}


void DX8Wrapper::Get_Transform(D3DTRANSFORMSTATETYPE type, Matrix4& transform)
{
	switch ((int)type)
	{

	case D3DTS_WORLD:

		if (render_state_changed & WORLD_IDENTITY)
			transform.Make_Identity();
		else
			transform = render_state.world.Transpose();

		break;

	case D3DTS_VIEW:

		if (render_state_changed & VIEW_IDENTITY)
			transform.Make_Identity();
		else
			transform = render_state.view.Transpose();

		break;
	
	case D3DTS_PROJECTION:
			
		transform = Transforms.projection.Transpose();
		break;

	default:

		D3DDevice->GetTransform(type, (D3DMATRIX*)&transform);
		transform = transform.Transpose();
		break;
	}
}



void DX8Wrapper::Set_Transform(D3DTRANSFORMSTATETYPE type, const Matrix4& transform)
{
	switch ((int)type)
	{

		case D3DTS_WORLD:

			render_state.world = transform.Transpose();
			render_state_changed = (render_state_changed | WORLD_CHANGED) & ~WORLD_IDENTITY;
			break;
		
		case D3DTS_VIEW:

			render_state.view = transform.Transpose();
			render_state_changed = (render_state_changed | VIEW_CHANGED) & ~VIEW_IDENTITY;
			break;
		
		case D3DTS_PROJECTION:
			
			Transforms.SetProjection(transform.Transpose());
			DX8Wrapper::ZFar = 0;
			DX8Wrapper::ZNear = 0;
			break;
		
		default:

			Matrix4 transposedTransform = transform.Transpose();
			D3DDevice->SetTransform(type, (D3DMATRIX*)&transposedTransform);
			break;

	}
}

void DX8Wrapper::Set_Transform(D3DTRANSFORMSTATETYPE type, const Matrix3D& transform)
{
	Matrix4 m(transform);
	Set_Transform(type, m);
}

void DX8Wrapper::Set_Projection_Transform_With_Depth_Bias(const Matrix4& transform, float znear, float zfar)
{
	ProjectionMatrix = transform.Transpose();	
	ZFar = zfar;
	ZNear = znear;

	if (ShaderCaps::SlopeScaleDepthBiasSupported || ZNear == ZFar)
	{
		Transforms.SetProjection(DX8Wrapper::ProjectionMatrix);
	}
	else
	{
		Matrix4 mat;
		mat = DX8Wrapper::ProjectionMatrix;
		mat[2].Z = (float)(ProjectionMatrix[2].Z - (((1.0 / (ZFar - ZNear)) * (ZBias * 0.0625)) * mat[3].Z));
		Transforms.SetProjection(mat);
	}
}


void DX8Wrapper::Draw_Triangles(uint16 startIndex, uint16 polygonCount, uint16 minVertexIndex, uint16 vertexCount)
{
	Draw(D3DPT_TRIANGLELIST, startIndex, polygonCount, minVertexIndex, vertexCount);
}



void DX8Wrapper::Draw_Strip(uint16 startIndex, uint16 polygonCount, uint16 minVertexIndex, uint16 vertexCount)
{
	Draw(D3DPT_TRIANGLESTRIP, startIndex, polygonCount, minVertexIndex, vertexCount);
}

void DX8Wrapper::Set_Render_Target(IDirect3DSurface9 *render_target, bool depthbuffer)
{
	if (render_target && render_target != DefaultRenderTarget) // if we want to set an RT and it's not the backbuffer
	{
		if (render_target != CurrentRenderTargets[CurrentRenderTargetIndex]) // and it's not the one that's currently active
		{
			if (!DefaultDepthBuffer) D3DDevice->GetDepthStencilSurface(&DefaultDepthBuffer);	// if we don't have the current depth buffer stored away, do so
			if (!DefaultRenderTarget) D3DDevice->GetRenderTarget(0, &DefaultRenderTarget);		// if we don't have the current backbuffer stored away, do so
			if (CurrentRenderTargets[CurrentRenderTargetIndex]) ++CurrentRenderTargetIndex;		// if we have something in this stack location, move up the stack
			IDirect3DSurface9*& current_render_target = CurrentRenderTargets[CurrentRenderTargetIndex];
			IDirect3DSurface9*& current_depth_buffer = CurrentDepthBuffers[CurrentRenderTargetIndex];
			current_render_target = render_target;
			current_render_target->AddRef();
			StateManager::SetRenderTarget(0, current_render_target);
			if (depthbuffer) // FIXME Needs to handle custom depth buffers if RT > Backbuffer
			{
				current_depth_buffer = DefaultDepthBuffer;
				current_depth_buffer->AddRef();
				D3DDevice->SetDepthStencilSurface(DefaultDepthBuffer);
			}
			else
			{
				current_depth_buffer = NULL;
				D3DDevice->SetDepthStencilSurface(0);
			};
		};
	}
	else
	{
		if ((CurrentRenderTargetIndex) && (CurrentRenderTargetIndex < 16)) // if we've got items in the stack!
		{
			SafeRelease(CurrentRenderTargets[CurrentRenderTargetIndex]);
			SafeRelease(CurrentDepthBuffers[CurrentRenderTargetIndex]);
			--CurrentRenderTargetIndex;
			StateManager::SetRenderTarget(0, CurrentRenderTargets[CurrentRenderTargetIndex]);
			D3DDevice->SetDepthStencilSurface(CurrentDepthBuffers[CurrentRenderTargetIndex]);
		}
		else
		{
			if (!DefaultRenderTarget)
			{
				IsRenderToTexture = false;
				return;
			}
			SafeRelease(CurrentRenderTargets[0]);
			SafeRelease(CurrentDepthBuffers[0]);
			D3DDevice->SetDepthStencilSurface(DefaultDepthBuffer);
			StateManager::SetRenderTarget(0, DefaultRenderTarget);
			SafeRelease(DefaultRenderTarget);
			SafeRelease(DefaultDepthBuffer);
		}
	};
	IsRenderToTexture = false;
}

void DX8Wrapper::Set_Render_Target(TextureClass* texture)
{
	IDirect3DSurface9 *surface = texture->Get_D3D_Surface_Level(0);
	Set_Render_Target(surface, false);
	surface->Release();
	IsRenderToTexture = true;
}

void DX8Wrapper::Get_Render_Target_Resolution(int& set_w,int& set_h,int& set_bits,bool& set_windowed)
{
	if (CurrentRenderTargets[CurrentRenderTargetIndex])
	{
		D3DSURFACE_DESC desc;
		CurrentRenderTargets[CurrentRenderTargetIndex]->GetDesc(&desc);
		set_w = desc.Width;
		set_h = desc.Height;
	}
	else
	{
		set_w = ResolutionWidth;
		set_h = ResolutionHeight;
	}
	set_bits = BitDepth;
	set_windowed = IsWindowed;
}

bool DX8Wrapper::Set_Device_Resolution(int width,int height,int bits,int windowed,bool resize_window)
{
	if (D3DDevice)
	{
		if (width != -1)
		{
			ResolutionWidth = _PresentParameters.BackBufferWidth =  width;
		}
		if (height != -1)
		{
			ResolutionHeight = _PresentParameters.BackBufferHeight = height;
		}
		return Reset_Device();
	}
	return false;
}

void DX8Wrapper::Flip_To_Primary()
{
	//FIXME FIXME FIXME
	if (!IsWindowed)
	{
		int count = 0;
		int bbc = (int)(_PresentParameters.BackBufferCount + 1 - DX8Wrapper::FrameCount % (_PresentParameters.BackBufferCount + 1)) % (int)(_PresentParameters.BackBufferCount + 1);
		if (bbc > 0)
		{
			while (count < 3)
			{
				HRESULT res;
				if (RenderDeviceSettings::Are_Settings_Dirty())
				{
					res = D3DERR_DEVICENOTRESET;
				}
				else
				{
					res = D3DDevice->TestCooperativeLevel();
				}
				if (SUCCEEDED(res))
				{
					if (SUCCEEDED(D3DDevice->Present(0,0,0,0)))
					{
						DX8Wrapper::FrameCount++;
					}
				}
				else
				{
					if (res == D3DERR_DEVICELOST)
					{
						return;
					}
					if (res == D3DERR_DEVICENOTRESET)
					{
						Reset_Device();
						count++;
					}
				}
				bbc--;
				if (bbc <= 0)
				{
					return;
				}
			}
		}
	}
}

void DX8Wrapper::Clear(bool clearColor, bool clearZStencil,const Vector3& color, float z, unsigned int stencil)
{
	uint clearFlags = 0;
	
	if (clearColor)
		clearFlags |= 1;
	
	if (clearZStencil)
	{
		clearFlags |= 2;
		
		if (_PresentParameters.AutoDepthStencilFormat == D3DFMT_D15S1 || _PresentParameters.AutoDepthStencilFormat == D3DFMT_D24S8 || _PresentParameters.AutoDepthStencilFormat == D3DFMT_D24X4S4)
			clearFlags |= 4;
	}
	
	if (clearFlags)
		D3DDevice->Clear(0, 0, clearFlags, GetD3dColor(color), z, stencil);
}

void DX8Wrapper::Set_DX8_ZBias(int zbias)
{
	if (zbias == ZBias)
	{
		return;
	}
	if (zbias > 0x0F)
	{
		zbias = 0x0F;
	}
	if (zbias < 0)
	{
		zbias = 0;
	}
	ZBias = zbias;

	ApplyDepthBias(ZBias);
}

bool DX8Wrapper::Reset_Device()
{
	if (IsInitted && D3DDevice)
	{
		WW3D::_Invalidate_Textures();
		Set_Vertex_Buffer(0);
		Set_Index_Buffer(0,0);
		Set_Render_Target(0, false);
		DynamicVBAccessClass::_Deinit();
		DynamicIBAccessClass::_Deinit();
		ResetDeclarationBuffer();
		DX8TextureManagerClass::Release_Textures();
		ReleaseResources();

		if (RenderDeviceSettings::Are_Settings_Dirty())
		{
			_PresentParameters.PresentationInterval = RenderDeviceSettings::VSync_Enabled() ? D3DPRESENT_INTERVAL_DEFAULT: D3DPRESENT_INTERVAL_IMMEDIATE;
			RenderDeviceSettings::Set_Settings_Dirty(false);
		}

		D3DDevice->Reset(&_PresentParameters);
		DX8TextureManagerClass::Recreate_Textures();
		ShaderClass::ShaderDirty = true;
		ReloadResources();
		return true;
	}
	return false;
}

void DX8Wrapper::_Enable_Triangle_Draw(bool enable)
{
	_EnableTriangleDraw = enable;
}

bool DX8Wrapper::_Is_Triangle_Draw_Enabled()
{
	return _EnableTriangleDraw;
}

RENEGADE_FUNCTION
int Find_POT(int num)
AT1(0x005FE990);

TextureClass *DX8Wrapper::Create_Render_Target_NonPow2(int width, int height, WW3DFormat format)
{
	WW3DFormat form = format;
	if (!form)
	{
		D3DDISPLAYMODE mode;
		D3DDevice->GetDisplayMode(0,&mode);
		form = D3DFormat_To_WW3DFormat(mode.Format);
	}
	if ((form >= WW3D_FORMAT_COUNT) || (!ShaderCaps::SupportRenderToTextureFormat[form]))
	{
		return 0;
	}
	/* FIXME
	if (width > (int)ShaderCaps::MaxTextureWidth)
	{
		width = (int)ShaderCaps::MaxTextureWidth;
	}
	if (height > (int)ShaderCaps::MaxTextureHeight)
	{
		height = (int)ShaderCaps::MaxTextureHeight;
	}
	*/
	TextureClass *tex = new TextureClass(width,height,form,TextureClass::MIP_LEVELS_1,TextureClass::POOL_DEFAULT,true);
	if (!tex->D3DTexture)
	{
		REF_PTR_RELEASE(tex);
	}
	return tex;
}

TextureClass *DX8Wrapper::Create_Render_Target(int width, int height, WW3DFormat format)
{
	WW3DFormat form = format;
	if (!form)
	{
		D3DDISPLAYMODE mode;
		D3DDevice->GetDisplayMode(0,&mode);
		form = D3DFormat_To_WW3DFormat(mode.Format);
	}
	if ((form >= WW3D_FORMAT_COUNT) || (!ShaderCaps::SupportRenderToTextureFormat[form]))
	{
		return 0;
	}
	unsigned int size = width;
	if (height > 0)
	{
		if (height < width)
		{
			size = height;
		}
	}
	size = Find_POT(size);
	if (size > ShaderCaps::MaxTextureWidth)
	{
		size = ShaderCaps::MaxTextureWidth;
	}
	if (size > ShaderCaps::MaxTextureHeight)
	{
		size = ShaderCaps::MaxTextureHeight;
	}
	TextureClass *tex = new TextureClass(size,size,form,TextureClass::MIP_LEVELS_1,TextureClass::POOL_DEFAULT,true);
	if (!tex->D3DTexture)
	{
		REF_PTR_RELEASE(tex);
	}
	return tex;
}

void DX8Wrapper::Set_Light(int pos, D3DLIGHT9 *light)
{
	if (light)
	{
		StateManager::SetLight(pos, light);
		StateManager::LightEnable(pos, true);
	}
	else
	{
		StateManager::LightEnable(pos, false);
	}
}

void DX8Wrapper::ClearZ()
{
	ZNear = 0;
	ZFar = 0;
}

void DX8Wrapper::Draw(unsigned int primitive_type, unsigned short start_index, unsigned short polygon_count, unsigned short min_vertex_index, unsigned short vertex_count)
{
	if (render_state_changed & WORLD_CHANGED)
	{
		Transforms.SetWorld(render_state.world);
	}
	if (render_state_changed & VIEW_CHANGED)
	{
		Transforms.SetView(render_state.view);
	}
	DebugEventStart(DEBUG_COLOR2,L"Draw<%S>",render_state.Textures[0] ? render_state.Textures[0]->Name : "NULL");
	ShaderController.Render(primitive_type,start_index,polygon_count,min_vertex_index,vertex_count);
	DebugEventEnd();
}

void DX8Wrapper::Begin_Scene()
{
	D3DDevice->BeginScene();
}

void DX8Wrapper::End_Scene(bool flip_frame)
{
	DX8Wrapper::D3DDevice->EndScene();
	if (flip_frame)
	{
		HRESULT res = DX8Wrapper::D3DDevice->Present(0,0,0,0);
		if (FAILED(res))
		{
			(DX8Wrapper::IsDeviceLost) = true;
		}
		else
		{
			(DX8Wrapper::IsDeviceLost) = false;
			DX8Wrapper::FrameCount++;
		}
		Set_Vertex_Buffer(0);
		Set_Index_Buffer(0,0);
		SafeRelease(render_state.Textures[0]);
		SafeRelease(render_state.Textures[1]);
		SafeRelease(render_state.material);
		render_state_changed |= TEXTURE0_CHANGED;
		render_state_changed |= TEXTURE1_CHANGED;
		render_state_changed |= MATERIAL_CHANGED;
	}
}

void Get_WW3D_Format_Name(WW3DFormat Format,StringClass &str)
{
	switch(Format)
	{
	case WW3D_FORMAT_R8G8B8:
		str = "R8G8B8";
		break;
	case WW3D_FORMAT_A8R8G8B8:
		str = "A8R8G8B8";
		break;
	case WW3D_FORMAT_X8R8G8B8:
		str = "X8R8G8B8";
		break;
	case WW3D_FORMAT_R5G6B5:
		str = "R5G6B5";
		break;
	case WW3D_FORMAT_X1R5G5B5:
		str = "X1R5G5B5";
		break;
	case WW3D_FORMAT_A1R5G5B5:
		str = "A1R5G5B5";
		break;
	case WW3D_FORMAT_A4R4G4B4:
		str = "A4R4G4B4";
		break;
	case WW3D_FORMAT_R3G3B2:
		str = "R3G3B2";
		break;
	case WW3D_FORMAT_A8:
		str = "A8";
		break;
	case WW3D_FORMAT_A8R3G3B2:
		str = "A8R3G3B2";
		break;
	case WW3D_FORMAT_X4R4G4B4:
		str = "X4R4G4B4";
		break;
	case WW3D_FORMAT_A8P8:
		str = "A8P8";
		break;
	case WW3D_FORMAT_P8:
		str = "P8";
		break;
	case WW3D_FORMAT_L8:
		str = "L8";
		break;
	case WW3D_FORMAT_A8L8:
		str = "A8L8";
		break;
	case WW3D_FORMAT_A4L4:
		str = "A4L4";
		break;
	case WW3D_FORMAT_U8V8:
		str = "U8V8";
		break;
	case WW3D_FORMAT_L6V5U5:
		str = "L6V5U5";
		break;
	case WW3D_FORMAT_X8L8V8U8:
		str = "X8L8V8U8";
		break;
	case WW3D_FORMAT_DXT1:
		str = "DXT1";
		break;
	case WW3D_FORMAT_DXT2:
		str = "DXT2";
		break;
	case WW3D_FORMAT_DXT3:
		str = "DXT3";
		break;
	case WW3D_FORMAT_DXT4:
		str = "DXT4";
		break;
	case WW3D_FORMAT_DXT5:
		str = "DXT5";
		break;
	default:
		str = "Unknown";
		break;
	}
}

IDirect3DTexture9 *DX8Wrapper::Create_DX8_Texture(unsigned int width,unsigned int height,WW3DFormat format,TextureClass::MipCountType mip_level_count,D3DPOOL pool,bool rendertarget)
{
	IDirect3DTexture9 *d3dtexture = 0;
	if (!rendertarget)
	{
		HRESULT res = D3DXCreateTexture(DX8Wrapper::D3DDevice,width,height,mip_level_count,0,WW3DFormat_To_D3DFormat(format),pool,&d3dtexture);
		if (res == D3DERR_OUTOFVIDEOMEMORY)
		{
			TextureClass::Invalidate_Old_Unused_Textures(5000);
			WW3D::_Invalidate_Mesh_Cache();
			res = D3DXCreateTexture(DX8Wrapper::D3DDevice,width,height,mip_level_count,0,WW3DFormat_To_D3DFormat(format),pool,&d3dtexture);
		}
		return d3dtexture;
	}
	HRESULT res = D3DXCreateTexture(DX8Wrapper::D3DDevice,width,height,mip_level_count,D3DUSAGE_RENDERTARGET,WW3DFormat_To_D3DFormat(format),pool,&d3dtexture);
	if (res == D3DERR_NOTAVAILABLE)
	{
		return 0;
	}
	if (res == D3DERR_OUTOFVIDEOMEMORY)
	{
		TextureClass::Invalidate_Old_Unused_Textures(5000);
		WW3D::_Invalidate_Mesh_Cache();
		res = D3DXCreateTexture(DX8Wrapper::D3DDevice,width,height,mip_level_count,0,WW3DFormat_To_D3DFormat(format),pool,&d3dtexture);
	}
	return d3dtexture;
}

void DX8Wrapper::Set_Render_State(RenderStateStruct &state)
{
	render_state = state;
	render_state_changed = 0xFFFFFFFF;
}

void DX8Wrapper::Release_Render_State()
{
	SafeRelease(render_state.index_buffer);
	SafeRelease(render_state.vertex_buffer);
	SafeRelease(render_state.material);
	SafeRelease(render_state.Textures[0]);
	SafeRelease(render_state.Textures[1]);
}

IDirect3DSurface9 *DX8Wrapper::Create_DX8_Surface(unsigned int width,unsigned int height,WW3DFormat format)
{
	IDirect3DSurface9 *surface;
	D3DDevice->CreateOffscreenPlainSurface(width,height,WW3DFormat_To_D3DFormat(format),D3DPOOL_SYSTEMMEM,&surface,0);
	return surface;
}

IDirect3DTexture9 *DX8Wrapper::_Create_DX8_Texture(IDirect3DSurface9* surface,TextureClass::MipCountType mip_level_count)
{
	D3DSURFACE_DESC desc;
	memset(&desc,0,sizeof(D3DSURFACE_DESC));
	surface->GetDesc(&desc);
	IDirect3DTexture9 *tex = DX8Wrapper::Create_DX8_Texture(desc.Width,desc.Height,D3DFormat_To_WW3DFormat(desc.Format),mip_level_count,D3DPOOL_MANAGED,false);
	IDirect3DSurface9 *surf;
	tex->GetSurfaceLevel(0,&surf);
	D3DXLoadSurfaceFromSurface(surf,0,0,surface,0,0,5,0);
	surf->Release();
	if (mip_level_count != 1)
	{
		D3DXFilterTexture(tex,0,0,5);
	}
	return tex;
}

template<class T> inline
T Bound(T original, T minval, T maxval)
{
	if (original < minval) return(minval);
	if (original > maxval) return(maxval);
	return(original);
};
void DX8Wrapper::Set_Gamma(float gamma,float bright,float contrast,bool calibrate,bool uselimit)
{
	gamma=Bound(gamma,0.6f,6.0f);
	bright=Bound(bright,-0.5f,0.5f);
	contrast=Bound(contrast,0.5f,2.0f);
	float oo_gamma=1.0f/gamma;
	DWORD flag=(calibrate?D3DSGR_CALIBRATE:D3DSGR_NO_CALIBRATION);
	D3DGAMMARAMP ramp;
	float			 limit;	
	if (uselimit) {
		limit=(contrast-1)/2*contrast;
	} else {
		limit = 0.0f;
	}
	for (int i=0; i<256; i++) {
		float in,out;
		in=i/256.0f;
		float x=in-limit;
		x=Bound(x,0.0f,1.0f);
		x=powf(x,oo_gamma);
		out=contrast*x+bright;
		out=Bound(out,0.0f,1.0f);
		ramp.red[i]=(WORD) (out*65535);
		ramp.green[i]=(WORD) (out*65535);
		ramp.blue[i]=(WORD) (out*65535);
	}
	if (ShaderCaps::SupportGamma)	{
		D3DDevice->SetGammaRamp(0,flag,&ramp);
	} else {
		HWND hwnd = GetDesktopWindow();
		HDC hdc = GetDC(hwnd);
		if (hdc)
		{
			SetDeviceGammaRamp (hdc, &ramp);
			ReleaseDC (hwnd, hdc);
		}
	}
}

IDirect3DSurface9* DX8Wrapper::_Create_DX8_Surface(const char *name)
{
	file_auto_ptr ptr(_TheFileFactory,name);
	if (!ptr->Is_Available())
	{
		return MissingTexture::_Create_Missing_Surface();
	}
	return TextureLoader::Load_Surface_Immediate(name,WW3D_FORMAT_UNKNOWN,true);
}

void DX8Wrapper::Set_Light(unsigned index,const LightClass &light)
{
	D3DLIGHT9 dlight;
	Vector3 temp;
	memset(&dlight,0,sizeof(D3DLIGHT9));
	switch (light.Type)
	{
	case LightClass::POINT:
		{
			dlight.Type=D3DLIGHT_POINT;
		}
		break;
	case LightClass::DIRECTIONAL:
		{
			dlight.Type=D3DLIGHT_DIRECTIONAL;
		}
		break;
	case LightClass::SPOT:
		{
			dlight.Type=D3DLIGHT_SPOT;
		}
		break;
	}
	temp = light.Diffuse;
	temp*=light.Intensity;
	dlight.Diffuse.r=temp.X;
	dlight.Diffuse.g=temp.Y;
	dlight.Diffuse.b=temp.Z;
	dlight.Diffuse.a=1.0f;
	temp = light.Specular;
	temp*=light.Intensity;
	dlight.Specular.r=temp.X;
	dlight.Specular.g=temp.Y;
	dlight.Specular.b=temp.Z;
	dlight.Specular.a=1.0f;
	temp = light.Ambient;
	temp*=light.Intensity;
	dlight.Ambient.r=temp.X;
	dlight.Ambient.g=temp.Y;
	dlight.Ambient.b=temp.Z;
	dlight.Ambient.a=1.0f;
	temp=light.Get_Position();
	dlight.Position=*(D3DVECTOR*) &temp;
	temp=light.SpotDirection;
	dlight.Direction=*(D3DVECTOR*) &temp;
	dlight.Range=light.FarAttenEnd;
	dlight.Falloff=light.SpotExponent;
	dlight.Theta=light.SpotAngle;
	dlight.Phi=light.SpotAngle;
	double a,b;
	a = light.FarAttenStart;
	b = light.FarAttenEnd;
	dlight.Attenuation0=1.0f;
	if (fabs(a-b)<1e-5)
	{
		dlight.Attenuation1=0.0f;
	}
	else
	{
		dlight.Attenuation1=(float)(1.0f/a);
	}
	dlight.Attenuation2=0.0f;
	Set_Light(index,&dlight);
}
