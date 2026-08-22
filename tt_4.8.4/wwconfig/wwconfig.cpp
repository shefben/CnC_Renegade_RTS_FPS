#include "General.h"
#include "resource.h"
#include "mss32.h"
#include "engine_vector.h"
#include "engine_string.h"
extern SimpleDynVecClass<char *> SoundDriverNames;
void Initialize_Audio();
void Shutdown_Audio();
HWND g_MainHwnd = 0;
HWND g_Tab = 0;
HWND g_TabHwnd[3] = { 0, 0, 0 };
HWND g_Tab0_Drivers = 0;
HWND g_Tab0_Res = 0;
HWND g_Tab0_ResLabel = 0;
HWND g_Tab0_Depth = 0;
HWND g_Tab0_Window = 0;
HWND g_Tab0_VSync = 0;
HWND g_Tab0_ChatLog = 0;
HWND g_Tab0_Shadow = 0;
HWND g_Tab0_Shader = 0;
HWND g_Tab0_SceneShader = 0;

HWND g_Tab1_Detail = 0;
HWND g_Tab1_Geo = 0;
HWND g_Tab1_Shadow = 0;
HWND g_Tab1_Texture = 0;
HWND g_Tab1_Particle = 0;
HWND g_Tab1_Surface = 0;
HWND g_Tab1_Lightmode = 0;
HWND g_Tab1_Texturefilt = 0;
HWND g_Tab1_Terrain = 0;

HWND g_Tab2_Drivers = 0;
HWND g_Tab2_SEEnable = 0;
HWND g_Tab2_SESlider = 0;
HWND g_Tab2_MEnable = 0;
HWND g_Tab2_MSlider = 0;
HWND g_Tab2_DEnable = 0;
HWND g_Tab2_DSlider = 0;
HWND g_Tab2_CEnable = 0;
HWND g_Tab2_CSlider = 0;
HWND g_Tab2_Quality = 0;
HWND g_Tab2_Playback = 0;
HWND g_Tab2_Speaker = 0;
HWND g_Tab2_Stereo = 0;

long g_Adapter;
char g_RenderDeviceName[513];
char g_RenderDeviceIdentifier[513];
unsigned long g_RenderDeviceWidth = 800;
unsigned long g_RenderDeviceHeight = 600;
long g_RenderDeviceDepth = 32;
long g_RenderDeviceWindowed = 0;
long g_RenderDeviceTextureDepth = 16; //TODO: Unhardcode this so its no longer always 16?
long g_DriverVersionCheckDisabled = 87;
long g_SysLOD = 5000; //TODO: make this unhardcoded so that mods like APB can make it bigger
long g_SysTextureResolution = 0;
long g_SysParticleDetail = 2;
long g_SysShadowMode = 2;
long g_SysSurfaceEffect = 2;
long g_SysPrelitMode = 2;
long g_SysTextureFiltering = 1;
long g_SysStaticProjectors = 1;
long g_SysDynamicProjectors = 1;
char g_SoundDeviceName[513];
long g_SoundQuality = 16;
long g_SoundHertz = 44100;
long g_SoundSpeakerType = 0;
long g_SoundSEnable = 1;
long g_SoundSVolume = 40;
long g_SoundDEnable = 1;
long g_SoundDVolume = 50;
long g_SoundMEnable = 1;
long g_SoundMVolume = 30;
long g_SoundCEnable = 1;
long g_SoundCVolume = 100;
long g_SoundStereo = 1;
long g_VSync = 1;
long g_Shadows = 1;
long g_Shader = 0;
long g_SceneShader = 0;
long g_ChatLog = 0;
IDirect3D9* D3DInterface;
DynamicVectorClass<unsigned int> Widths;
DynamicVectorClass<unsigned int> Heights;
int mode;
void EnumModes(int m_NewAdapter)
{
	Widths.Clear();
	Heights.Clear();
	long m_ModeCount = D3DInterface->GetAdapterModeCount(m_NewAdapter,D3DFMT_X8R8G8B8);
	long curmode = 0;
	D3DDISPLAYMODE m_Mode;
	for (int i = 0; i < m_ModeCount; i++)
	{
		D3DInterface->EnumAdapterModes(m_NewAdapter,D3DFMT_X8R8G8B8,i, &m_Mode);
		if ((m_Mode.Width < 640) || (m_Mode.Height < 480))
		{
			continue;
		}
		bool found = false;
		{
			for (int j = 0;j < Widths.Count();j++)
			{
				if ((Widths[j] == m_Mode.Width) && (Heights[j] == m_Mode.Height))
				{
					found = true;
				}
			}
		}
		if (!found)
		{
			Widths.Add(m_Mode.Width);
			Heights.Add(m_Mode.Height);
			if ((m_Mode.Width == g_RenderDeviceWidth) && (m_Mode.Height == g_RenderDeviceHeight))
			{
				mode = curmode;
			}
			curmode++;
		}
	}
	m_ModeCount = D3DInterface->GetAdapterModeCount(m_NewAdapter,D3DFMT_R5G6B5);
	for (int i = 0; i < m_ModeCount; i++)
	{
		D3DInterface->EnumAdapterModes(m_NewAdapter,D3DFMT_R5G6B5,i, &m_Mode);
		if ((m_Mode.Width < 640) || (m_Mode.Height < 480))
		{
			continue;
		}
		bool found = false;
		{
			for (int j = 0;j < Widths.Count();j++)
			{
				if ((Widths[j] == m_Mode.Width) && (Heights[j] == m_Mode.Height))
				{
					found = true;
				}
			}
		}
		if (!found)
		{
			Widths.Add(m_Mode.Width);
			Heights.Add(m_Mode.Height);
			if ((m_Mode.Width == g_RenderDeviceWidth) && (m_Mode.Height == g_RenderDeviceHeight))
			{
				mode = curmode;
			}
			curmode++;
		}
	}
}

inline char* GUIDToString(const GUID* guid, char* str, size_t sz)
{
    _snprintf(str, sz, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             guid->Data1, guid->Data2, guid->Data3,
             guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
             guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
    return str;
};

DynamicVectorClass<char *> _RenderDeviceNameTable;
DynamicVectorClass<char *> _RenderDeviceGUIDTable;
DynamicVectorClass<int> _RenderDeviceIdentifierTable;
void InitD3D()
{
	D3DInterface = Direct3DCreate9(D3D_SDK_VERSION);
	if (!D3DInterface)
	{
		return;
	}
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
		_RenderDeviceNameTable.Add(_strdup(devname));
		_RenderDeviceIdentifierTable.Add(adapter_index);
		char _guid[40];
		_RenderDeviceGUIDTable.Add(_strdup(GUIDToString(&id.DeviceIdentifier, _guid, sizeof(_guid))));
	}
}

void ShutdownD3D()
{
	SafeRelease(D3DInterface);
	_RenderDeviceNameTable.Clear();
	_RenderDeviceGUIDTable.Clear();
	_RenderDeviceIdentifierTable.Clear();
}

void LoadSettings()
{
	HKEY m_RegKey;
	DWORD m_Size = 4;
	memset(&g_RenderDeviceName, 0, sizeof(g_RenderDeviceName));
	memset(&g_SoundDeviceName, 0, sizeof(g_SoundDeviceName));
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Westwood\\Renegade\\Render", 0, KEY_READ, &m_RegKey) == ERROR_SUCCESS) //TODO: Make this registry key somehow not-hard-coded so it can pick up values for mods
	{
		RegQueryValueEx(m_RegKey, "RenderDeviceForceVSync", 0, 0, (LPBYTE)&g_VSync, &m_Size);
		RegQueryValueEx(m_RegKey, "RenderDeviceDepth", 0, 0, (LPBYTE)&g_RenderDeviceDepth, &m_Size);
		RegQueryValueEx(m_RegKey, "RenderDeviceWidth", 0, 0, (LPBYTE)&g_RenderDeviceWidth, &m_Size);
		RegQueryValueEx(m_RegKey, "RenderDeviceHeight", 0, 0, (LPBYTE)&g_RenderDeviceHeight, &m_Size);
		RegQueryValueEx(m_RegKey, "RenderDeviceWindowed", 0, 0, (LPBYTE)&g_RenderDeviceWindowed, &m_Size);
		RegQueryValueEx(m_RegKey, "RenderDeviceTextureDepth", 0, 0, (LPBYTE)&g_RenderDeviceTextureDepth, &m_Size);
		RegQueryValueEx(m_RegKey, "DriverVersionCheckDisabled", 0, 0, (LPBYTE)&g_DriverVersionCheckDisabled, &m_Size);
		m_Size = 512;
		RegQueryValueEx(m_RegKey, "RenderDeviceName", 0, 0, (LPBYTE)&g_RenderDeviceName, &m_Size);
		RegQueryValueEx(m_RegKey, "RenderDeviceIdentifier", 0, 0, (LPBYTE)&g_RenderDeviceIdentifier, &m_Size);
		m_Size = 4;
		//TODO: Read MSAA value
	}
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Westwood\\Renegade", 0, KEY_READ, &m_RegKey) == ERROR_SUCCESS) //TODO: Make this registry key somehow not-hard-coded so it can pick up values for mods
	{
		RegQueryValueEx(m_RegKey, "HighQualityShadows", 0, 0, (LPBYTE)&g_Shadows, &m_Size);
		RegQueryValueEx(m_RegKey, "ShaderDetail", 0, 0, (LPBYTE)&g_Shader, &m_Size);
		RegQueryValueEx(m_RegKey, "SceneShaderDetail", 0, 0, (LPBYTE)&g_SceneShader, &m_Size);
		RegQueryValueEx(m_RegKey, "ClientChatLog", 0, 0, (LPBYTE)&g_ChatLog, &m_Size);
	}
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Westwood\\Renegade\\Sound", 0, KEY_READ, &m_RegKey) == ERROR_SUCCESS) //TODO: Make this registry key somehow not-hard-coded so it can pick up values for mods
	{
		RegQueryValueEx(m_RegKey, "bits", 0, 0, (LPBYTE)&g_SoundQuality, &m_Size);
		RegQueryValueEx(m_RegKey, "sound enabled", 0, 0, (LPBYTE)&g_SoundSEnable, &m_Size);
		RegQueryValueEx(m_RegKey, "sound volume", 0, 0, (LPBYTE)&g_SoundSVolume, &m_Size);
		RegQueryValueEx(m_RegKey, "dialog enabled", 0, 0, (LPBYTE)&g_SoundDEnable, &m_Size);
		RegQueryValueEx(m_RegKey, "dialog volume", 0, 0, (LPBYTE)&g_SoundDVolume, &m_Size);
		RegQueryValueEx(m_RegKey, "music enabled", 0, 0, (LPBYTE)&g_SoundMEnable, &m_Size);
		RegQueryValueEx(m_RegKey, "music volume", 0, 0, (LPBYTE)&g_SoundMVolume, &m_Size);
		RegQueryValueEx(m_RegKey, "cinematic enabled", 0, 0, (LPBYTE)&g_SoundCEnable, &m_Size);
		RegQueryValueEx(m_RegKey, "cinematic volume", 0, 0, (LPBYTE)&g_SoundCVolume, &m_Size);
		RegQueryValueEx(m_RegKey, "hertz", 0, 0, (LPBYTE)&g_SoundHertz, &m_Size);
		RegQueryValueEx(m_RegKey, "speaker type", 0, 0, (LPBYTE)&g_SoundSpeakerType, &m_Size);
		RegQueryValueEx(m_RegKey, "stereo", 0, 0, (LPBYTE)&g_SoundStereo, &m_Size);
		m_Size = 512;
		RegQueryValueEx(m_RegKey, "device name", 0, 0, (LPBYTE)&g_SoundDeviceName, &m_Size);
		m_Size = 4;
	}
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Westwood\\Renegade\\System Settings", 0, KEY_READ, &m_RegKey) == ERROR_SUCCESS) //TODO: Make this registry key somehow not-hard-coded so it can pick up values for mods
	{
		RegQueryValueEx(m_RegKey, "Dynamic_LOD_Budget", 0, 0, (LPBYTE)&g_SysLOD, &m_Size);
		RegQueryValueEx(m_RegKey, "Texture_Resolution", 0, 0, (LPBYTE)&g_SysTextureResolution, &m_Size);
		RegQueryValueEx(m_RegKey, "Particle_Detail", 0, 0, (LPBYTE)&g_SysParticleDetail, &m_Size);
		RegQueryValueEx(m_RegKey, "Shadow_Mode", 0, 0, (LPBYTE)&g_SysShadowMode, &m_Size);
		RegQueryValueEx(m_RegKey, "Surface_Effect_Detail", 0, 0, (LPBYTE)&g_SysSurfaceEffect, &m_Size);
		RegQueryValueEx(m_RegKey, "Prelit_Mode", 0, 0, (LPBYTE)&g_SysPrelitMode, &m_Size);
		RegQueryValueEx(m_RegKey, "Texture_Filter_Mode", 0, 0, (LPBYTE)&g_SysTextureFiltering, &m_Size);
		RegQueryValueEx(m_RegKey, "Static_Projectors", 0, 0, (LPBYTE)&g_SysStaticProjectors, &m_Size);
		g_SysDynamicProjectors = g_SysShadowMode ? 1 : 0;
	}
}

void SaveSettings()
{
	ShutdownD3D();
	Shutdown_Audio();
	HKEY m_RegKey;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Westwood\\Renegade\\Debug", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &m_RegKey, NULL) == ERROR_SUCCESS) //TODO: Make this registry key somehow not-hard-coded so it can pick up values for mods
	{
		long m_Temp = 0;
		RegSetValueEx(m_RegKey, "GameInitInProgress", 0, REG_DWORD, (LPBYTE)&m_Temp, 4);
	}
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Westwood\\Renegade", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &m_RegKey, NULL) == ERROR_SUCCESS) //TODO: Make this registry key somehow not-hard-coded so it can pick up values for mods
	{
		RegSetValueEx(m_RegKey, "HighQualityShadows", 0, REG_DWORD, (LPBYTE)&g_Shadows, 4);
		RegSetValueEx(m_RegKey, "ShaderDetail", 0, REG_DWORD, (LPBYTE)&g_Shader, 4);
		RegSetValueEx(m_RegKey, "SceneShaderDetail", 0, REG_DWORD, (LPBYTE)&g_SceneShader, 4);
		RegSetValueEx(m_RegKey, "ClientChatLog", 0, REG_DWORD, (LPBYTE)&g_ChatLog, 4);
	}
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Westwood\\Renegade\\Render", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &m_RegKey, NULL) == ERROR_SUCCESS) //TODO: Make this registry key somehow not-hard-coded so it can pick up values for mods
	{
		RegSetValueEx(m_RegKey, "RenderDeviceForceVSync", 0, REG_DWORD, (LPBYTE)&g_VSync, 4);
		RegSetValueEx(m_RegKey, "RenderDeviceDepth", 0, REG_DWORD, (LPBYTE)&g_RenderDeviceDepth, 4);
		RegSetValueEx(m_RegKey, "RenderDeviceWidth", 0, REG_DWORD, (LPBYTE)&g_RenderDeviceWidth, 4);
		RegSetValueEx(m_RegKey, "RenderDeviceHeight", 0, REG_DWORD, (LPBYTE)&g_RenderDeviceHeight, 4);
		RegSetValueEx(m_RegKey, "RenderDeviceWindowed", 0, REG_DWORD, (LPBYTE)&g_RenderDeviceWindowed, 4);
		RegSetValueEx(m_RegKey, "RenderDeviceTextureDepth", 0, REG_DWORD, (LPBYTE)&g_RenderDeviceTextureDepth, 4);
		RegSetValueEx(m_RegKey, "DriverVersionCheckDisabled", 0, REG_DWORD, (LPBYTE)&g_DriverVersionCheckDisabled, 4);
		RegSetValueEx(m_RegKey, "RenderDeviceName", 0, REG_SZ, (LPBYTE)&g_RenderDeviceName, (DWORD)strlen(g_RenderDeviceName));
		RegSetValueEx(m_RegKey, "RenderDeviceIdentifier", 0, REG_SZ, (LPBYTE)&g_RenderDeviceIdentifier, (DWORD)strlen(g_RenderDeviceName));
		//TODO: Set MSAA value
	}
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Westwood\\Renegade\\Sound", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &m_RegKey, NULL) == ERROR_SUCCESS) //TODO: Make this registry key somehow not-hard-coded so it can pick up values for mods
	{
		g_SoundSVolume = (long)SendMessage(g_Tab2_SESlider, TBM_GETPOS, 0, 0);
		g_SoundDVolume = (long)SendMessage(g_Tab2_DSlider, TBM_GETPOS, 0, 0);
		g_SoundMVolume = (long)SendMessage(g_Tab2_MSlider, TBM_GETPOS, 0, 0);
		g_SoundCVolume = (long)SendMessage(g_Tab2_CSlider, TBM_GETPOS, 0, 0);
		RegSetValueEx(m_RegKey, "bits", 0, REG_DWORD, (LPBYTE)&g_SoundQuality, 4);
		RegSetValueEx(m_RegKey, "sound enabled", 0, REG_DWORD, (LPBYTE)&g_SoundSEnable, 4);
		RegSetValueEx(m_RegKey, "sound volume", 0, REG_DWORD, (LPBYTE)&g_SoundSVolume, 4);
		RegSetValueEx(m_RegKey, "dialog enabled", 0, REG_DWORD, (LPBYTE)&g_SoundDEnable, 4);
		RegSetValueEx(m_RegKey, "dialog volume", 0, REG_DWORD, (LPBYTE)&g_SoundDVolume, 4);
		RegSetValueEx(m_RegKey, "music enabled", 0, REG_DWORD, (LPBYTE)&g_SoundMEnable, 4);
		RegSetValueEx(m_RegKey, "music volume", 0, REG_DWORD, (LPBYTE)&g_SoundMVolume, 4);
		RegSetValueEx(m_RegKey, "cinematic enabled", 0, REG_DWORD, (LPBYTE)&g_SoundCEnable, 4);
		RegSetValueEx(m_RegKey, "cinematic volume", 0, REG_DWORD, (LPBYTE)&g_SoundCVolume, 4);
		RegSetValueEx(m_RegKey, "hertz", 0, REG_DWORD, (LPBYTE)&g_SoundHertz, 4);
		RegSetValueEx(m_RegKey, "speaker type", 0, REG_DWORD, (LPBYTE)&g_SoundSpeakerType, 4);
		RegSetValueEx(m_RegKey, "stereo", 0, REG_DWORD, (LPBYTE)&g_SoundStereo, 4);
		RegSetValueEx(m_RegKey, "device name", 0, REG_SZ, (LPBYTE)&g_SoundDeviceName, (DWORD)strlen(g_SoundDeviceName));
	}
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Westwood\\Renegade\\System Settings", 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &m_RegKey, NULL) == ERROR_SUCCESS) //TODO: Make this registry key somehow not-hard-coded so it can pick up values for mods
	{
		g_SysLOD = (long)SendMessage(g_Tab1_Geo, TBM_GETPOS, 0, 0) * 5000; //TODO: make this unhardcoded so that mods like APB can make it bigger
		g_SysTextureResolution = 2 - (long)SendMessage(g_Tab1_Texture, TBM_GETPOS, 0, 0);
		g_SysParticleDetail = (long)SendMessage(g_Tab1_Particle, TBM_GETPOS, 0, 0);
		g_SysShadowMode = (long)SendMessage(g_Tab1_Shadow, TBM_GETPOS, 0, 0);
		g_SysSurfaceEffect = (long)SendMessage(g_Tab1_Surface, TBM_GETPOS, 0, 0);
		g_SysDynamicProjectors = g_SysShadowMode ? 1 : 0;
		RegSetValueEx(m_RegKey, "Dynamic_LOD_Budget", 0, REG_DWORD, (LPBYTE)&g_SysLOD, 4);
		RegSetValueEx(m_RegKey, "Static_LOD_Budget", 0, REG_DWORD, (LPBYTE)&g_SysLOD, 4);
		RegSetValueEx(m_RegKey, "Texture_Resolution", 0, REG_DWORD, (LPBYTE)&g_SysTextureResolution, 4);
		RegSetValueEx(m_RegKey, "Particle_Detail", 0, REG_DWORD, (LPBYTE)&g_SysParticleDetail, 4);
		RegSetValueEx(m_RegKey, "Shadow_Mode", 0, REG_DWORD, (LPBYTE)&g_SysShadowMode, 4);
		RegSetValueEx(m_RegKey, "Surface_Effect_Detail", 0, REG_DWORD, (LPBYTE)&g_SysSurfaceEffect, 4);
		RegSetValueEx(m_RegKey, "Prelit_Mode", 0, REG_DWORD, (LPBYTE)&g_SysPrelitMode, 4);
		RegSetValueEx(m_RegKey, "Texture_Filter_Mode", 0, REG_DWORD, (LPBYTE)&g_SysTextureFiltering, 4);
		RegSetValueEx(m_RegKey, "Static_Projectors", 0, REG_DWORD, (LPBYTE)&g_SysStaticProjectors, 4);
		RegSetValueEx(m_RegKey, "Dynamic_Projectors", 0, REG_DWORD, (LPBYTE)&g_SysDynamicProjectors, 4);
	}
}

void UpdatePerform(long m_Pos)
{
	switch (m_Pos)
	{
		default:
		case 0:
			SendMessage(g_Tab1_Geo, TBM_SETPOS, TRUE, 0);
			SendMessage(g_Tab1_Shadow, TBM_SETPOS, TRUE, 0);
			SendMessage(g_Tab1_Texture, TBM_SETPOS, TRUE, 0);
			SendMessage(g_Tab1_Particle, TBM_SETPOS, TRUE, 0);
			SendMessage(g_Tab1_Surface, TBM_SETPOS, TRUE, 0);
			SendMessage(g_Tab1_Lightmode, CB_SETCURSEL, 0, 0);
			SendMessage(g_Tab1_Texturefilt, CB_SETCURSEL, 0, 0);
			SendMessage(g_Tab1_Terrain, BM_SETCHECK, BST_UNCHECKED, 0);
			break;
		case 1:
			SendMessage(g_Tab1_Geo, TBM_SETPOS, TRUE, 0);
			SendMessage(g_Tab1_Shadow, TBM_SETPOS, TRUE, 1);
			SendMessage(g_Tab1_Texture, TBM_SETPOS, TRUE, 1);
			SendMessage(g_Tab1_Particle, TBM_SETPOS, TRUE, 0);
			SendMessage(g_Tab1_Surface, TBM_SETPOS, TRUE, 0);
			SendMessage(g_Tab1_Lightmode, CB_SETCURSEL, 2, 0);
			SendMessage(g_Tab1_Texturefilt, CB_SETCURSEL, 0, 0);
			SendMessage(g_Tab1_Terrain, BM_SETCHECK, BST_UNCHECKED, 0);
			break;
		case 2:
			SendMessage(g_Tab1_Geo, TBM_SETPOS, TRUE, 1);
			SendMessage(g_Tab1_Shadow, TBM_SETPOS, TRUE, 2);
			SendMessage(g_Tab1_Texture, TBM_SETPOS, TRUE, 2);
			SendMessage(g_Tab1_Particle, TBM_SETPOS, TRUE, 1);
			SendMessage(g_Tab1_Surface, TBM_SETPOS, TRUE, 1);
			SendMessage(g_Tab1_Lightmode, CB_SETCURSEL, 2, 0);
			SendMessage(g_Tab1_Texturefilt, CB_SETCURSEL, 1, 0);
			SendMessage(g_Tab1_Terrain, BM_SETCHECK, BST_CHECKED, 0);
			break;
		case 3:
			SendMessage(g_Tab1_Geo, TBM_SETPOS, TRUE, 2);
			SendMessage(g_Tab1_Shadow, TBM_SETPOS, TRUE, 2);
			SendMessage(g_Tab1_Texture, TBM_SETPOS, TRUE, 2);
			SendMessage(g_Tab1_Particle, TBM_SETPOS, TRUE, 2);
			SendMessage(g_Tab1_Surface, TBM_SETPOS, TRUE, 2);
			SendMessage(g_Tab1_Lightmode, CB_SETCURSEL, 2, 0);
			SendMessage(g_Tab1_Texturefilt, CB_SETCURSEL, 2, 0);
			SendMessage(g_Tab1_Terrain, BM_SETCHECK, BST_CHECKED, 0);
			break;
		case 4:
			SendMessage(g_Tab1_Geo, TBM_SETPOS, TRUE, 2);
			SendMessage(g_Tab1_Shadow, TBM_SETPOS, TRUE, 3);
			SendMessage(g_Tab1_Texture, TBM_SETPOS, TRUE, 2);
			SendMessage(g_Tab1_Particle, TBM_SETPOS, TRUE, 2);
			SendMessage(g_Tab1_Surface, TBM_SETPOS, TRUE, 2);
			SendMessage(g_Tab1_Lightmode, CB_SETCURSEL, 2, 0);
			SendMessage(g_Tab1_Texturefilt, CB_SETCURSEL, 4, 0);
			SendMessage(g_Tab1_Terrain, BM_SETCHECK, BST_CHECKED, 0);
			break;
	}
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TCITEM m_Tab;
	NMHDR *lphdr;
	long i;
	char m_Buf[32];
	m_Buf[0] = 0;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		if (hwnd != g_MainHwnd)
			break;
		g_Tab = GetDlgItem(g_MainHwnd, IDC_TAB);
		m_Tab.mask = TCIF_TEXT;
		m_Tab.pszText = "Video";
		SendMessage(g_Tab, TCM_INSERTITEM, 0, (LPARAM)&m_Tab);
		m_Tab.pszText = "Audio";
		SendMessage(g_Tab, TCM_INSERTITEM, 1, (LPARAM)&m_Tab);
		m_Tab.pszText = "Performance";
		SendMessage(g_Tab, TCM_INSERTITEM, 2, (LPARAM)&m_Tab);
		InvalidateRect(g_Tab,NULL,true);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			case IDCANCEL:
				PostQuitMessage(0);
				break;
			case IDOK:
				SaveSettings();
				PostQuitMessage(0);
				break;
			case IDC_EXPERT:
				if (HIWORD(wParam) == BN_CLICKED)
				{
					int m_expert = SendMessage(GetDlgItem(g_TabHwnd[2],IDC_EXPERT), BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0;
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_EXPERTSETTINGS),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_GD),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_CS),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_TD),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_PD),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_SED),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_LM),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_TF),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_LOW1),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_LOW2),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_LOW3),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_LOW4),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_LOW5),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_HIGH1),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_HIGH2),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_HIGH3),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_HIGH4),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_HIGH5),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_LIGHTMODE),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_TEXTUREFILT),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_TERRAIN),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_GEO),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_SHADOW),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_TEXTURE),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_PARTICLE),m_expert);
					ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_SURFACE),m_expert);
				}
				break;
			case IDC_DEPTH:
				if (HIWORD(wParam) == CBN_SELCHANGE)
				{
					SendMessage(g_Tab0_Depth, CB_GETLBTEXT, (long)SendMessage(g_Tab0_Depth, CB_GETCURSEL, 0, 0), (LPARAM)m_Buf);
					g_RenderDeviceDepth = atoi(m_Buf);
				}
				break;
			case IDC_VSYNC:
				if (HIWORD(wParam) == BN_CLICKED)
					g_VSync = SendMessage(g_Tab0_VSync, BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0;
				break;
			case IDC_SHADOWS:
				if (HIWORD(wParam) == BN_CLICKED)
					g_Shadows = SendMessage(g_Tab0_Shadow, BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0;
				break;
			case IDC_CHATLOG:
				if (HIWORD(wParam) == BN_CLICKED)
					g_ChatLog = SendMessage(g_Tab0_ChatLog, BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0;
				break;
			case IDC_WINDOW:
				if (HIWORD(wParam) == BN_CLICKED)
					g_RenderDeviceWindowed = SendMessage(g_Tab0_Window, BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0;
				break;
			case IDC_LIGHTMODE:
				if (HIWORD(wParam) == CBN_SELCHANGE)
				{
					SendMessage(g_Tab1_Lightmode, CB_GETLBTEXT, (long)SendMessage(g_Tab1_Lightmode, CB_GETCURSEL, 0, 0), (LPARAM)m_Buf);
						if (!_stricmp(m_Buf, "Vertex"))
						g_SysPrelitMode = 0;
					else if (!_stricmp(m_Buf, "Multi-Pass Lightmaps"))
						g_SysPrelitMode = 1;
					else
						g_SysPrelitMode = 2;
				}
				break;
			case IDC_TEXTUREFILT:
				if (HIWORD(wParam) == CBN_SELCHANGE)
				{
					g_SysTextureFiltering = (long)SendMessage(g_Tab1_Texturefilt, CB_GETCURSEL, 0, 0);
				}
				break;
			case IDC_TERRAIN:
				if (HIWORD(wParam) == BN_CLICKED)
					g_SysStaticProjectors = SendMessage(g_Tab1_Terrain, BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0;
				break;
			case IDC_SEENABLE:
				if (HIWORD(wParam) == BN_CLICKED)
					g_SoundSEnable = SendMessage(g_Tab2_SEEnable, BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0;
				break;
			case IDC_DENABLE:
				if (HIWORD(wParam) == BN_CLICKED)
					g_SoundDEnable = SendMessage(g_Tab2_DEnable, BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0;
				break;
			case IDC_MENABLE:
				if (HIWORD(wParam) == BN_CLICKED)
					g_SoundMEnable = SendMessage(g_Tab2_MEnable, BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0;
				break;
			case IDC_CENABLE:
				if (HIWORD(wParam) == BN_CLICKED)
					g_SoundCEnable = SendMessage(g_Tab2_CEnable, BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0;
				break;
			case IDC_QUALITY:
				if (HIWORD(wParam) == CBN_SELCHANGE)
				{
					SendMessage(g_Tab2_Quality, CB_GETLBTEXT, (long)SendMessage(g_Tab2_Quality, CB_GETCURSEL, 0, 0), (LPARAM)m_Buf);
					g_SoundQuality = atoi(m_Buf);
				}
				break;
			case IDC_PLAYBACK:
				if (HIWORD(wParam) == CBN_SELCHANGE)
				{
					SendMessage(g_Tab2_Playback, CB_GETLBTEXT, (long)SendMessage(g_Tab2_Playback, CB_GETCURSEL, 0, 0), (LPARAM)m_Buf);
					i = atoi(m_Buf);
					if (i == 11)
						g_SoundHertz = 11025;
					else if (i == 22)
						g_SoundHertz = 11050;
					else
						g_SoundHertz = 44100;
				}
				break;
			case IDC_SPEAKER:
				if (HIWORD(wParam) == CBN_SELCHANGE)
				{
					SendMessage(g_Tab2_Speaker, CB_GETLBTEXT, (long)SendMessage(g_Tab2_Speaker, CB_GETCURSEL, 0, 0), (LPARAM)m_Buf);
					if (!_stricmp(m_Buf, "2 Speakers"))
						g_SoundSpeakerType = 0;
					else if (!_stricmp(m_Buf, "Headphone"))
						g_SoundSpeakerType = 1;
					else if (!_stricmp(m_Buf, "Surround Sound"))
						g_SoundSpeakerType = 2;
					else
						g_SoundSpeakerType = 3;
				}
				break;
			case IDC_STEREO:
				if (HIWORD(wParam) == BN_CLICKED)
					g_SoundStereo = SendMessage(g_Tab2_Stereo, BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0;
				break;
			//case IDC_AA: //TODO: MSAA
				//break;
		}
		break;
	case WM_CLOSE:
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_HSCROLL:
		if ((HWND)lParam == g_Tab1_Detail)
		{
			UpdatePerform((long)SendMessage(g_Tab1_Detail, TBM_GETPOS, 0, 0));
		}
		if ((HWND)lParam == g_Tab0_SceneShader)
		{
			g_SceneShader = (long)SendMessage(g_Tab0_SceneShader, TBM_GETPOS, 0, 0);
		}
		if ((HWND)lParam == g_Tab0_Shader)
		{
			g_Shader = (long)SendMessage(g_Tab0_Shader, TBM_GETPOS, 0, 0);
		}
		if ((HWND)lParam == g_Tab0_Res)
		{
			int res = (long)SendMessage(g_Tab0_Res, TBM_GETPOS, 0, 0);
			g_RenderDeviceWidth = Widths[res];
			g_RenderDeviceHeight = Heights[res];
			char restext[10];
			sprintf(restext,"%d x %d",g_RenderDeviceWidth,g_RenderDeviceHeight);
			SetWindowText(g_Tab0_ResLabel,restext);
		}
		break;
	case WM_NOTIFY:
		lphdr = (LPNMHDR)lParam;
		if (lphdr->code == LVN_ITEMCHANGED)
		{
			NMLISTVIEW *lv = (LPNMLISTVIEW)lParam;
			if (lv->uChanged == LVIF_STATE)
			{
				if (lv->hdr.hwndFrom == g_Tab2_Drivers)
				{
					int sel = SendMessage(g_Tab2_Drivers,LVM_GETNEXTITEM,(WPARAM)-1,LVNI_SELECTED);
					if (sel != -1)
					{
						strcpy(g_SoundDeviceName,SoundDriverNames[sel]);
					}
				}
				if (lv->hdr.hwndFrom == g_Tab0_Drivers)
				{
					int sel = SendMessage(g_Tab0_Drivers,LVM_GETNEXTITEM,(WPARAM)-1,LVNI_SELECTED);
					if (sel != -1)
					{
						strcpy(g_RenderDeviceName,_RenderDeviceNameTable[sel]);
						strcpy(g_RenderDeviceIdentifier,_RenderDeviceGUIDTable[sel]);
						EnumModes(_RenderDeviceIdentifierTable[sel]);
						SendMessage(g_Tab0_Res,TBM_SETRANGEMAX, TRUE, Widths.Count()-1);
						SendMessage(g_Tab0_Res,TBM_SETPOS, TRUE, mode);
						char restext[10];
						sprintf(restext,"%d x %d",g_RenderDeviceWidth,g_RenderDeviceHeight);
						SetWindowText(g_Tab0_ResLabel,restext);
					}
				}
			}
		}
		if ((lphdr->hwndFrom == g_Tab) && (lphdr->code == TCN_SELCHANGE))
		{
			int m_CurIndex = TabCtrl_GetCurSel(g_Tab);
			for (int j = 0; j < 3; j++)
			{
				ShowWindow(g_TabHwnd[j], j == m_CurIndex ? SW_SHOW : SW_HIDE);
			}
		}
		break;
	}
	return FALSE;
}

void InitTabs()
{
	long m_Id, m_ResultId = 0;
	g_Tab0_Drivers = GetDlgItem(g_TabHwnd[0], IDC_GFXDRIVERS);
	g_Tab0_Res = GetDlgItem(g_TabHwnd[0], IDC_RES);
	g_Tab0_ResLabel = GetDlgItem(g_TabHwnd[0], IDC_RESLABEL);
	g_Tab0_Depth = GetDlgItem(g_TabHwnd[0], IDC_DEPTH);
	g_Tab0_VSync = GetDlgItem(g_TabHwnd[0], IDC_VSYNC);
	g_Tab0_Window = GetDlgItem(g_TabHwnd[0], IDC_WINDOW);
	g_Tab0_ChatLog = GetDlgItem(g_TabHwnd[0], IDC_CHATLOG);
	g_Tab0_Shadow = GetDlgItem(g_TabHwnd[0], IDC_SHADOWS);
	g_Tab0_Shader = GetDlgItem(g_TabHwnd[0], IDC_SHADERS);
	g_Tab0_SceneShader = GetDlgItem(g_TabHwnd[0], IDC_SCENE);
	g_Tab1_Detail = GetDlgItem(g_TabHwnd[2], IDC_DETAIL);
	g_Tab1_Geo = GetDlgItem(g_TabHwnd[2], IDC_GEO);
	g_Tab1_Shadow = GetDlgItem(g_TabHwnd[2], IDC_SHADOW);
	g_Tab1_Texture = GetDlgItem(g_TabHwnd[2], IDC_TEXTURE);
	g_Tab1_Particle = GetDlgItem(g_TabHwnd[2], IDC_PARTICLE);
	g_Tab1_Surface = GetDlgItem(g_TabHwnd[2], IDC_SURFACE);
	g_Tab1_Lightmode = GetDlgItem(g_TabHwnd[2], IDC_LIGHTMODE);
	g_Tab1_Texturefilt = GetDlgItem(g_TabHwnd[2], IDC_TEXTUREFILT);
	g_Tab1_Terrain = GetDlgItem(g_TabHwnd[2], IDC_TERRAIN);
	g_Tab2_Drivers = GetDlgItem(g_TabHwnd[1], IDC_SNDDRIVERS);
	g_Tab2_SEEnable = GetDlgItem(g_TabHwnd[1], IDC_SEENABLE);
	g_Tab2_SESlider = GetDlgItem(g_TabHwnd[1], IDC_SESLIDER);
	g_Tab2_MEnable = GetDlgItem(g_TabHwnd[1], IDC_MENABLE);
	g_Tab2_MSlider = GetDlgItem(g_TabHwnd[1], IDC_MSLIDER);
	g_Tab2_DEnable = GetDlgItem(g_TabHwnd[1], IDC_DENABLE);
	g_Tab2_DSlider = GetDlgItem(g_TabHwnd[1], IDC_DSLIDER);
	g_Tab2_CEnable = GetDlgItem(g_TabHwnd[1], IDC_CENABLE);
	g_Tab2_CSlider = GetDlgItem(g_TabHwnd[1], IDC_CSLIDER);
	g_Tab2_Quality = GetDlgItem(g_TabHwnd[1], IDC_QUALITY);
	g_Tab2_Playback = GetDlgItem(g_TabHwnd[1], IDC_PLAYBACK);
	g_Tab2_Speaker = GetDlgItem(g_TabHwnd[1], IDC_SPEAKER);
	g_Tab2_Stereo = GetDlgItem(g_TabHwnd[1], IDC_STEREO);
	SendMessage(g_Tab1_Detail, TBM_SETRANGEMAX, TRUE, 4);
	SendMessage(g_Tab1_Geo, TBM_SETRANGEMAX, TRUE, 2);
	SendMessage(g_Tab1_Shadow, TBM_SETRANGEMAX, TRUE, 3);
	SendMessage(g_Tab1_Texture, TBM_SETRANGEMAX, TRUE, 2);
	SendMessage(g_Tab1_Particle, TBM_SETRANGEMAX, TRUE, 2);
	SendMessage(g_Tab1_Surface, TBM_SETRANGEMAX, TRUE, 2);
	SendMessage(g_Tab1_Detail, TBM_SETPOS, TRUE, 2);
	SendMessage(g_Tab1_Geo, TBM_SETPOS, TRUE, (g_SysLOD / 5000));
	SendMessage(g_Tab1_Shadow, TBM_SETPOS, TRUE, g_SysShadowMode);
	SendMessage(g_Tab1_Texture, TBM_SETPOS, TRUE, 2 - g_SysTextureResolution);
	SendMessage(g_Tab1_Particle, TBM_SETPOS, TRUE, g_SysParticleDetail);
	SendMessage(g_Tab1_Surface, TBM_SETPOS, TRUE, g_SysSurfaceEffect);
	SendMessage(g_Tab1_Terrain, BM_SETCHECK, g_SysStaticProjectors == 1 ? BST_CHECKED : BST_UNCHECKED, 0);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_EXPERTSETTINGS),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_GD),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_CS),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_TD),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_PD),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_SED),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_LM),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_TF),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_LOW1),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_LOW2),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_LOW3),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_LOW4),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_LOW5),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_HIGH1),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_HIGH2),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_HIGH3),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_HIGH4),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_HIGH5),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_LIGHTMODE),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_TEXTUREFILT),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_TERRAIN),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_GEO),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_SHADOW),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_TEXTURE),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_PARTICLE),SW_HIDE);
	ShowWindow(GetDlgItem(g_TabHwnd[2],IDC_SURFACE),SW_HIDE);
	SendMessage(g_Tab0_VSync, BM_SETCHECK, g_VSync == 1 ? BST_CHECKED : BST_UNCHECKED, 0);
	SendMessage(g_Tab0_Window, BM_SETCHECK, g_RenderDeviceWindowed == 1 ? BST_CHECKED : BST_UNCHECKED, 0);
	SendMessage(g_Tab0_ChatLog, BM_SETCHECK, g_ChatLog == 1 ? BST_CHECKED : BST_UNCHECKED, 0);
	SendMessage(g_Tab0_Shadow, BM_SETCHECK, g_Shadows == 1 ? BST_CHECKED : BST_UNCHECKED, 0);
	SendMessage(g_Tab0_Window, BM_SETCHECK, g_RenderDeviceWindowed == 1 ? BST_CHECKED : BST_UNCHECKED, 0);
	SendMessage(g_Tab0_Shader, TBM_SETRANGEMAX, TRUE, 4);
	SendMessage(g_Tab0_SceneShader, TBM_SETRANGEMAX, TRUE, 3);
	SendMessage(g_Tab0_Shader, TBM_SETPOS, TRUE, g_Shader);
	SendMessage(g_Tab0_SceneShader, TBM_SETPOS, TRUE, g_SceneShader);
	LVCOLUMN column;
	column.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	column.pszText = "Driver Name";
	column.fmt = 0;
	column.cx = 200;
	SendMessage(g_Tab2_Drivers, LVM_DELETEALLITEMS, 0, 0);
	SendMessage(g_Tab2_Drivers, LVM_INSERTCOLUMN, 0, (LPARAM)&column);
	Initialize_Audio();
	InitD3D();
	LVCOLUMN column2;
	column2.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	column2.pszText = "Driver Name";
	column2.fmt = 0;
	column2.cx = 200;
	SendMessage(g_Tab0_Drivers, LVM_DELETEALLITEMS, 0, 0);
	SendMessage(g_Tab0_Drivers, LVM_INSERTCOLUMN, 0, (LPARAM)&column2);
	for (int i = 0;i < _RenderDeviceNameTable.Count();i++)
	{
		LVITEM item;
		memset(&item,0,sizeof(item));
		item.mask = LVIF_TEXT;
		item.pszText = _strdup(_RenderDeviceNameTable[i]);
		SendMessage(g_Tab0_Drivers, LVM_INSERTITEM, 0, (LPARAM)&item);
		if (!_stricmp(_RenderDeviceNameTable[i],g_RenderDeviceName))
		{
			LVITEM item2;
			memset(&item2,0,sizeof(item2));
			item2.state = LVIS_SELECTED;
			item2.stateMask = LVIS_SELECTED;
			SendMessage(g_Tab0_Drivers, LVM_SETITEMSTATE, i, (LPARAM)&item2);
			EnumModes(_RenderDeviceIdentifierTable[i]);
			SendMessage(g_Tab0_Res,TBM_CLEARTICS, TRUE, 0);
			SendMessage(g_Tab0_Res,TBM_SETRANGEMAX, TRUE, Widths.Count()-1);
			SendMessage(g_Tab0_Res,TBM_SETPOS, TRUE, mode);
			for (int j = 0;j < Widths.Count()-1;j++)
			{
				SendMessage(g_Tab0_Res,TBM_SETTIC, 0, j);
			}
			char restext[10];
			sprintf(restext,"%d x %d",g_RenderDeviceWidth,g_RenderDeviceHeight);
			SetWindowText(g_Tab0_ResLabel,restext);
		}
	}
	//TODO: populate MSAA dropdown and select proper value
	m_ResultId = 0;
	m_Id = (long)SendMessage(g_Tab0_Depth, CB_ADDSTRING, 0, (LPARAM)"16 Bit");
	if (g_RenderDeviceDepth == 16)
		m_ResultId = m_Id;
	m_Id = (long)SendMessage(g_Tab0_Depth, CB_ADDSTRING, 0, (LPARAM)"32 Bit");
	if (g_RenderDeviceDepth == 32)
		m_ResultId = m_Id;
	SendMessage(g_Tab0_Depth, CB_SETCURSEL, m_ResultId, 0);
	for (int i = 0;i < SoundDriverNames.Count();i++)
	{
		LVITEM item;
		memset(&item,0,sizeof(item));
		item.mask = LVIF_TEXT;
		item.pszText = SoundDriverNames[i];
		SendMessage(g_Tab2_Drivers, LVM_INSERTITEM, 0, (LPARAM)&item);
		if (!g_SoundDeviceName[0] && !_stricmp(SoundDriverNames[i], "Miles Fast 2D Positional Audio"))
		{
			strcpy(g_SoundDeviceName, "Miles Fast 2D Positional Audio");
		}
		if (!_stricmp(SoundDriverNames[i],g_SoundDeviceName))
		{
			LVITEM item2;
			memset(&item2,0,sizeof(item2));
			item2.state = LVIS_SELECTED;
			item2.stateMask = LVIS_SELECTED;
			SendMessage(g_Tab2_Drivers, LVM_SETITEMSTATE, i, (LPARAM)&item2);
		}
	}
	SendMessage(g_Tab2_SEEnable, BM_SETCHECK, g_SoundSEnable == 1 ? BST_CHECKED : BST_UNCHECKED, 0);
	SendMessage(g_Tab2_DEnable, BM_SETCHECK, g_SoundDEnable == 1 ? BST_CHECKED : BST_UNCHECKED, 0);
	SendMessage(g_Tab2_MEnable, BM_SETCHECK, g_SoundMEnable == 1 ? BST_CHECKED : BST_UNCHECKED, 0);
	SendMessage(g_Tab2_CEnable, BM_SETCHECK, g_SoundCEnable == 1 ? BST_CHECKED : BST_UNCHECKED, 0);
	SendMessage(g_Tab2_SESlider, TBM_SETPOS, TRUE, g_SoundSVolume);
	SendMessage(g_Tab2_DSlider, TBM_SETPOS, TRUE, g_SoundDVolume);
	SendMessage(g_Tab2_MSlider, TBM_SETPOS, TRUE, g_SoundMVolume);
	SendMessage(g_Tab2_CSlider, TBM_SETPOS, TRUE, g_SoundCVolume);
	SendMessage(g_Tab2_Quality, CB_RESETCONTENT, 0, 0);
	SendMessage(g_Tab2_Playback, CB_RESETCONTENT, 0, 0);
	SendMessage(g_Tab2_Speaker, CB_RESETCONTENT, 0, 0);
	m_ResultId = 0;
	m_Id = (long)SendMessage(g_Tab2_Quality, CB_ADDSTRING, 0, (LPARAM)"8 Bit");
	if (g_SoundQuality == 8)
		m_ResultId = m_Id;
	m_Id = (long)SendMessage(g_Tab2_Quality, CB_ADDSTRING, 0, (LPARAM)"16 Bit");
	if (g_SoundQuality == 16)
		m_ResultId = m_Id;
	SendMessage(g_Tab2_Quality, CB_SETCURSEL, m_ResultId, 0);
	m_ResultId = 0;
	m_Id = (long)SendMessage(g_Tab2_Playback, CB_ADDSTRING, 0, (LPARAM)"11 kHz");
	if (g_SoundHertz == 11025)
		m_ResultId = m_Id;
	m_Id = (long)SendMessage(g_Tab2_Playback, CB_ADDSTRING, 0, (LPARAM)"22 kHz");
	if (g_SoundHertz == 22050)
		m_ResultId = m_Id;
	m_Id = (long)SendMessage(g_Tab2_Playback, CB_ADDSTRING, 0, (LPARAM)"44 kHz");
	if (g_SoundHertz == 44100)
		m_ResultId = m_Id;
	SendMessage(g_Tab2_Playback, CB_SETCURSEL, m_ResultId, 0);
	m_ResultId = 0;
	m_Id = (long)SendMessage(g_Tab2_Speaker, CB_ADDSTRING, 0, (LPARAM)"Headphone");
	if (g_SoundSpeakerType == 1)
		m_ResultId = m_Id;
	m_Id = (long)SendMessage(g_Tab2_Speaker, CB_ADDSTRING, 0, (LPARAM)"2 Speakers");
	if (g_SoundSpeakerType == 0)
		m_ResultId = m_Id;
	m_Id = (long)SendMessage(g_Tab2_Speaker, CB_ADDSTRING, 0, (LPARAM)"4 Speakers");
	if (g_SoundSpeakerType == 3)
		m_ResultId = m_Id;
	m_Id = (long)SendMessage(g_Tab2_Speaker, CB_ADDSTRING, 0, (LPARAM)"Surround Sound");
	if (g_SoundSpeakerType == 2)
		m_ResultId = m_Id;
	SendMessage(g_Tab2_Speaker, CB_SETCURSEL, m_ResultId, 0);
	SendMessage(g_Tab2_Stereo, BM_SETCHECK, g_SoundStereo == 1 ? BST_CHECKED : BST_UNCHECKED, 0);
	m_ResultId = 0;
	m_Id = (long)SendMessage(g_Tab1_Lightmode, CB_ADDSTRING, 0, (LPARAM)"Vertex");
	if (g_SysPrelitMode == 0)
		m_ResultId = m_Id;
	m_Id = (long)SendMessage(g_Tab1_Lightmode, CB_ADDSTRING, 0, (LPARAM)"Multi-Pass Lightmaps");
	if (g_SysPrelitMode == 1)
		m_ResultId = m_Id;
	m_Id = (long)SendMessage(g_Tab1_Lightmode, CB_ADDSTRING, 0, (LPARAM)"Multi-Texture Lightmaps");
	if (g_SysPrelitMode == 2)
		m_ResultId = m_Id;
	SendMessage(g_Tab1_Lightmode, CB_SETCURSEL, m_ResultId, 0);
	SendMessage(g_Tab1_Texturefilt, CB_RESETCONTENT, 0, 0);
	m_ResultId = 0;
	m_Id = (long)SendMessage(g_Tab1_Texturefilt, CB_ADDSTRING, 0, (LPARAM)"Bilinear");
	if (g_SysTextureFiltering == 0)
		m_ResultId = m_Id;
	m_Id = (long)SendMessage(g_Tab1_Texturefilt, CB_ADDSTRING, 0, (LPARAM)"Trilinear");
	if (g_SysTextureFiltering == 1)
		m_ResultId = m_Id;
	m_Id = (long)SendMessage(g_Tab1_Texturefilt, CB_ADDSTRING, 0, (LPARAM)"Anisotropic 2X");
	if (g_SysTextureFiltering == 2)
		m_ResultId = m_Id;
	m_Id = (long)SendMessage(g_Tab1_Texturefilt, CB_ADDSTRING, 0, (LPARAM)"Anisotropic 4X");
	if (g_SysTextureFiltering == 3)
		m_ResultId = m_Id;
	m_Id = (long)SendMessage(g_Tab1_Texturefilt, CB_ADDSTRING, 0, (LPARAM)"Anisotropic 8X");
	if (g_SysTextureFiltering == 4)
		m_ResultId = m_Id;
	m_Id = (long)SendMessage(g_Tab1_Texturefilt, CB_ADDSTRING, 0, (LPARAM)"Anisotropic 16X");
	if (g_SysTextureFiltering == 5)
		m_ResultId = m_Id;
	SendMessage(g_Tab1_Texturefilt, CB_SETCURSEL, m_ResultId, 0);
}

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,int nCmdShow)
{
	MSG msg;
	INITCOMMONCONTROLSEX ctrl;
	ctrl.dwICC =  0x00004000 | ICC_TAB_CLASSES;
	ctrl.dwSize = sizeof(INITCOMMONCONTROLSEX);
	InitCommonControlsEx(&ctrl);
	g_MainHwnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, (DLGPROC)WndProc);
	g_TabHwnd[0] = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_TAB_VIDEO), g_MainHwnd, (DLGPROC)WndProc);
	g_TabHwnd[1] = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_TAB_AUDIO), g_MainHwnd, (DLGPROC)WndProc);
	g_TabHwnd[2] = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_TAB_PERFORMANCE), g_MainHwnd, (DLGPROC)WndProc);
	HANDLE m_hIcon = LoadImage(hInstance, MAKEINTRESOURCE(IDI_MAIN), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	if (m_hIcon)
		SendMessage(g_MainHwnd, WM_SETICON, ICON_SMALL, (LPARAM)m_hIcon);
	SendMessage(g_MainHwnd, WM_INITDIALOG, 0, 0);
	LoadSettings();
	InitTabs();
	UpdateWindow(g_MainHwnd);
	ShowWindow(g_MainHwnd, SW_SHOW);
	ShowWindow(g_TabHwnd[0], SW_SHOW);
	while (GetMessage(&msg, NULL, 0U, 0U))
	{
		TranslateMessage(&msg);
		if ((msg.message == WM_KEYDOWN) && (msg.wParam == VK_ESCAPE))
			PostQuitMessage(0);
		DispatchMessage(&msg);
	}
	return 0;
}
