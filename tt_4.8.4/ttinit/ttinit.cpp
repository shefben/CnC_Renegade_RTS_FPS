/*	Renegade tt.dll
	base d3d9 implementation
	Copyright 2009 Jonathan Wilson

	This file is part of the renegade tt.dll.
	CONFIDENTIAL: DO NOT USE OR DISTRIBUTE WITHOUT PERMISSION
*/
#include "general.h"
#include "hooksupport.h"
#include "scripts.h"
#include "_globals.h"
#include "RegistryClass.h"
#include "cNetwork.h"
#include "render2d.h"
#include "DX8MeshRendererClass.h"
#include "PacketManagerClass.h"
#include "HashMixFileFactoryClass.h"
#include "FileFactoryListClass.h"
#include "DX8Caps.h"
#include "texture.h"
#include "ConsoleModeClass.h"
#include "DirectInput.h"
#include "CombatManager.h"
#include "input.h"
#include "SimpleFileFactoryClass.h"
#include "ScriptManager.h"
void placeMemoryHooks();
#pragma warning(disable:6322) // Empty _except block

class TextureClass;

bool g_ExtendedMinidumps = false;

unsigned int Get_Registry_Int(const char *entry,int defaultvalue);

void *osaddr;

RENEGADE_FUNCTION
void PATCH_Get_OS_Info()
{
   __asm
   {
      sub esp, 0xCC
      mov dword ptr [esp+0x0C], 0
      mov eax, osaddr
      jmp eax
   };
}

void Set_Working_Directory(HINSTANCE instance)
{
	char path_to_exe[256];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char path[_MAX_PATH];
	GetModuleFileName(instance, path_to_exe, sizeof(path_to_exe));
	_splitpath(path_to_exe, drive, dir, NULL, NULL);
	_makepath(path, drive, dir, NULL, NULL);
	SetCurrentDirectory(path);
}
bool Get_Restart_Flag()
{
	bool restart = false;
	RegistryClass registry(Build_Registry_Location_String((char *)0x007F5274,0,"WOLSettings"),true);
	if (registry.bIsValid)
	{
		restart = (registry.Get_Int("AutoRestartFlag",0) != 0);
	}
	return restart;
}

#define VALUE_NAME_RENDER_DEVICE_NAME "RenderDeviceName"
#define VALUE_NAME_GAME_INITIALIZATION_IN_PROGRESS "GameInitInProgress"
static bool Graphics_Settings_Trouble_Shooting()
{
	RegistryClass registry( APPLICATION_SUB_KEY_NAME_DEBUG );
	if (!registry.Is_Valid()) return true;

	int progress=registry.Get_Int( VALUE_NAME_GAME_INITIALIZATION_IN_PROGRESS, 0 );
	if (progress) {
		StringClass options="wwconfig.exe ";
		char* opts=options.Peek_Buffer();
		STARTUPINFO startup_info;
		ZeroMemory(&startup_info,sizeof(STARTUPINFO));
		startup_info.cb=sizeof(STARTUPINFO);
		PROCESS_INFORMATION process_info;
		CreateProcess(
			NULL,
			opts,
			NULL,
			NULL,
			FALSE,
			0,
			NULL,
			NULL,
			&startup_info,
			&process_info);
		CloseHandle(process_info.hThread);
		unsigned long exit_code=STILL_ACTIVE;
		unsigned res=0;
		do {
			res=GetExitCodeProcess(process_info.hProcess,&exit_code);
			if (!res) {
				return true;
			}
			Sleep(100);
		}
		while (exit_code==STILL_ACTIVE);
		return !exit_code;
	}
	return true;
}

bool IsDebuggerAttached()
{
    DWORD dw;

    __asm
    {
        push eax    // Preserve the registers
        push ecx
        mov eax, fs:[0x18]  // Get the TIB's linear address
        mov eax, dword ptr [eax + 0x30]
        mov ecx, dword ptr [eax]    // Get the whole DWORD
        mov dw, ecx // Save it
        pop ecx // Restore the registers
        pop eax
    }


    // The 3rd byte is the byte we really need to check for the
    // presence of a debugger.
    // Check the 3rd byte

    return (dw & 0x00010000) != false;
};

FastCriticalSectionClass ExceptionHandlerLock;
typedef void (*exceptcallback) (void);
REF_DEF2(application_exception_callback,exceptcallback,0x0084F5C4,0x0084E7AC);
LONG WINAPI ExceptionHandler(int code, LPEXCEPTION_POINTERS pExs)
{
	FastCriticalSectionClass::LockClass kung_foo_death_grip(ExceptionHandlerLock);
	application_exception_callback();
	if (IsDebuggerAttached()) return EXCEPTION_CONTINUE_SEARCH;
	if (pExs->ExceptionRecord->ExceptionCode == ERROR_MOD_NOT_FOUND ||
		pExs->ExceptionRecord->ExceptionCode == ERROR_PROC_NOT_FOUND)
	{
		//tell user
		ExitProcess(pExs->ExceptionRecord->ExceptionCode);
	}

	switch(pExs->ExceptionRecord->ExceptionCode)
	{
		case EXCEPTION_ACCESS_VIOLATION:
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		case EXCEPTION_BREAKPOINT:
		case EXCEPTION_DATATYPE_MISALIGNMENT:
		case EXCEPTION_FLT_DENORMAL_OPERAND:
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		case EXCEPTION_FLT_INEXACT_RESULT:
		case EXCEPTION_FLT_INVALID_OPERATION:
		case EXCEPTION_FLT_OVERFLOW:
		case EXCEPTION_FLT_STACK_CHECK:
		case EXCEPTION_FLT_UNDERFLOW:
		case EXCEPTION_ILLEGAL_INSTRUCTION:
		case EXCEPTION_IN_PAGE_ERROR:
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
		case EXCEPTION_INT_OVERFLOW:
		case EXCEPTION_INVALID_DISPOSITION:
		case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		case EXCEPTION_PRIV_INSTRUCTION:
		case EXCEPTION_SINGLE_STEP:
		case EXCEPTION_STACK_OVERFLOW:
		{
			wchar_t filename[MAX_PATH];
			wchar_t path[MAX_PATH];
		
			HANDLE dumpFile;
			SYSTEMTIME time;
			MINIDUMP_EXCEPTION_INFORMATION expParam;
			
			GetLocalTime(&time);
			GetCurrentDirectoryW(MAX_PATH, path);

			swprintf(filename, MAX_PATH, L"%s\\debug", path);
			CreateDirectoryW(filename, NULL);

			swprintf(filename, MAX_PATH, g_ExtendedMinidumps ? L"%s\\debug\\extcrashdump.%04u%02u%02u-%02u%02u%02u.dmp" : L"%s\\debug\\crashdump.%04u%02u%02u-%02u%02u%02u.dmp", 
							path, 
							time.wYear, time.wMonth, time.wDay, 
							time.wHour, time.wMinute, time.wSecond);

			dumpFile = CreateFileW(filename, GENERIC_READ | GENERIC_WRITE, 
							FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH, NULL);

			expParam.ThreadId = GetCurrentThreadId();
			expParam.ExceptionPointers = pExs;
			expParam.ClientPointers = FALSE;

			MINIDUMP_TYPE type = (MINIDUMP_TYPE) ((g_ExtendedMinidumps ? MiniDumpWithFullMemory : (MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory)));
				
			MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dumpFile, type, &expParam, NULL, NULL);
			CloseHandle(dumpFile);

			// FIXME FIXME FIXME
			// Shiny dialog explaining that something went horribly wrong and 
			// that we need the crashdumps to fix it
			// Following message is placeholder
			MessageBoxW(NULL, L"Renegade has encountered an internal error and is unable to continue normally.", L"Command and Conquer: Renegade - Internal Error", MB_ICONERROR | MB_OK | MB_SETFOREGROUND);

			ExitProcess(pExs->ExceptionRecord->ExceptionCode); // Exit Renegade.
			break;
		}
		default:
			return EXCEPTION_CONTINUE_SEARCH;
			break;
	}
}


BOOL Create_Main_Window(HANDLE hInstance, int nCmdShow);
void Register_Thread_ID(int,char *,bool);
void Unregister_Thread_ID(int,char *);
int Game_Main_Loop();


RENEGADE_FUNCTION
BOOL Create_Main_Window(HANDLE hInstance, int nCmdShow)
AT1(0x0043D920);
RENEGADE_FUNCTION
void Register_Thread_ID(int,char *,bool)
AT1(0x005E5DD0);
RENEGADE_FUNCTION
void Unregister_Thread_ID(int,char *)
AT1(0x005E5F00);
RENEGADE_FUNCTION
int Game_Main_Loop()
AT1(0x0043BA30);

BOOL TryLoadD3DX()
{
	__try
	{
		BOOL test = D3DXCheckVersion(D3D_SDK_VERSION, D3DX_SDK_VERSION); // will cause an exception if the dll is missing
		test;
		return TRUE;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) 
	{
		return FALSE;
	}
}
HINSTANCE ProgramInstance;
int Start_Application( HINSTANCE hInstance, HINSTANCE , LPSTR , int nCmdShow )
{
	ProgramInstance = hInstance;
	if(!TryLoadD3DX())
	{
		FILE *f2 = fopen("dllload.txt","at");
		fprintf(f2,"[shaders.dll] D3DX not found, unable to continue. Please update your version of DirectX using the web updater.\n");
		MessageBox(NULL,"You need to update your version of DirectX in order to use this copy of scripts.dll.","Error",MB_OK|MB_ICONEXCLAMATION|MB_TOPMOST);
		fclose(f2);
		return ERROR_MOD_NOT_FOUND;
	}

	{
		Set_Working_Directory(hInstance);
		char tmp_buffer[256];
		char* tmp_ptr;
		if (!SearchPath(
			"data",
			"always.dat",
			NULL,
			sizeof(tmp_buffer),
			tmp_buffer,
			&tmp_ptr)) {
			MessageBox(NULL,"Set working folder and try again...","Invalid working folder",MB_OK);
			return 0;
		}
#ifndef DEBUG
		if (Get_Restart_Flag() == false && !ConsoleBox.Is_Exclusive()){
			if (!Graphics_Settings_Trouble_Shooting()) return 0;
		}
#endif
		if (!Create_Main_Window(hInstance, nCmdShow)) return 0;
		Register_Thread_ID(GetCurrentThreadId(), "Main Thread", true);
	}

	{
		char tmp_buffer[256];
		char* tmp_ptr;
		if (SearchPath("debug","extended_crashdumps", NULL, sizeof(tmp_buffer), tmp_buffer, &tmp_ptr))
		{
			g_ExtendedMinidumps = true;
		};
	}
	
	int exitCode = EXIT_SUCCESS;
		*((unsigned int *)0x0084F5C4) = 0x00438B30;
		*((unsigned int *)0x0084F5C8) = 0x00435030;
		__try {
			exitCode = Game_Main_Loop();
		} __except(ExceptionHandler(GetExceptionCode(), GetExceptionInformation())) {};
	Unregister_Thread_ID(GetCurrentThreadId(), "Main Thread");
	return exitCode;
}

const unsigned char Code2[12] = {0x56,0x57,0x8B,0x7C,0x24,0x0C,0x57,0xE8,0x8E,0xB1,0xFE,0xFF};
ScriptCommands *Commands;

void *input;
HMODULE tt = 0;

REF_DEF2(RenegadeFileFactory, FileFactoryListClass, 0x0081ED28, 0x0081DF10);
REF_DEF2(RenegadeBaseFileFactory, SimpleFileFactoryClass, 0x0081ED60, 0x0081DF48);
void Load_Mix_Files()
{
	RenegadeFileFactory.Add_FileFactory(new HashMixFileFactoryClass("Always2.dat",RenegadeBaseFileFactory),"Always2.dat");
	RenegadeFileFactory.Add_FileFactory(new HashMixFileFactoryClass("Always.dbs",RenegadeBaseFileFactory),"Always.dbs");
	RenegadeFileFactory.Add_FileFactory(new HashMixFileFactoryClass("Always.dat",RenegadeBaseFileFactory),"Always.dat");
}


DECLSPEC_RESTRICT void* RenegadeAllocate(size_t size)
{
#ifndef NDEBUG
	char functionName[128];
	sprintf(functionName, "0x%08X", _ReturnAddress());
	return AllocateMemory(size, AllocType_Unvalidated, "Renegade.exe", 0, functionName);
#else
	return AllocateMemory(size);
#endif
};

void RenegadeFree(void* memory)
{
#ifndef NDEBUG
	char functionName[128];
	sprintf(functionName, "0x%08X", _ReturnAddress());
	FreeMemory(memory, DeallocType_Unvalidated, "Renegade.exe", 0, functionName);
#else
	FreeMemory(memory);
#endif
};

BOOL __declspec(dllexport) APIENTRY DllMain(HINSTANCE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			{
				TT_ASSERT(Exe != 6);

				CheckMemoryManagerConfiguration(MEMORYMANAGER_CONFIGURATION);
				hookAsJump(0x00672B70, 0x00672410, RenegadeAllocate); //patch for memory manager
				hookAsJump(0x00672C80, 0x00672520, RenegadeFree);  //patch for memory manager
				hookAsJump(0x0043A1D0, 0x0043A270, Build_Registry_Location_String); // Registry location fixup
				hookAsCall(0x0043907C, 0x0043911C, Load_Mix_Files); //patch to load mix files at startup for filehashing
				void InitShaderHooks();
				InitShaderHooks();

				if (Exe == 0)
				{	
					osaddr = (void *)0x005ECE36;
					WriteJump(0x0043907C+5, (void*)0x00439192); // Do not load all .mix files at startup
					hookAsJump(0x0043D070,0x00000000,Start_Application); //hooks for multimon stuff
					hookAsJump(0x006ECA50,0x0,DirectInput::Init); //DirectInput hooks
					hookAsJump(0x006ECD00,0x0,DirectInput::Shutdown); //DirectInput hooks
					hookAsJump(0x006ECDD0,0x0,DirectInput::Acquire); //DirectInput hooks
					hookAsJump(0x006ECE80,0x0,DirectInput::Unacquire); //DirectInput hooks
					hookAsJump(0x006ED300,0x0,DirectInput::Eat_Mouse_Held_States); //DirectInput hooks
					hookAsJump(0x00676070,0x0,Input::Is_Button_Down); //Input hooks
					hookAsJump(0x006767F0,0x0,Input::Load_Configuration); //Input hooks
					hookAsJump(0x00676460,0x0,Input::Save_Configuration); //Input hooks
					hookAsJump(0x00677040,0x0,Input::Get_Translated_Key_Name); //Input hooks
					hookAsJump(0x00675950,0x0,Input::Flush); //Input hooks
					hookAsJump(0x00675C10,0x0,Input::Update); //Input hooks
					WriteNop((void *)0x006E28E0,73); //weather manager hooks
					WriteNop((void *)0x00710890,73); //weather manager hooks
					WriteNop((void *)0x004EC4B3,10); //remove calls to ToolTipMgrClass::Initialize and MenuDialogClass::Initialize
					WriteNop((void *)0x004399A6,5); //remove call to cGameData::Onetime_Init
					WriteNop((void *)0x004EC4A6,5); //remove call to StyleMgrClass::Initialize_From_INI
				}
				if (Exe == 1)
				{
					osaddr = (void *)0x005EC6D6;
					WriteNop((void *)0x006E2180,73); //weather manager hooks
					WriteNop((void *)0x0070FE50,73); //weather manager hooks
					WriteJump(0x0043911C+5, (void*)0x00439232); // Do not load all .mix files at startup
				}
				hookAsJump(0x005E5CC0, 0x005E5560, ExceptionHandler); //exception handler hook
				hookAsJump(0x00457040, 0x00456AF0, cNetwork::Get_Data_Files_CRC); //Fix memory leak in cNetwork::Get_Data_Files_CRC
				hookAsJump(0x005DCFA0, 0x005DC840, StringClass::Get_String); //StringClass patches
				hookAsJump(0x005DD230, 0x005DCAD0, StringClass::Resize); //StringClass patches
				hookAsJump(0x005DD330, 0x005DCBD0, StringClass::Uninitialised_Grow); //StringClass patches
				hookAsJump(0x005DD410, 0x005DCCB0, StringClass::Free_String); //StringClass patches
				hookAsJump(0x005DD7A0, 0x005DD040, WideStringClass::Get_String); //WideStringClass patches
				hookAsJump(0x005DD990, 0x005DD230, WideStringClass::Free_String); //WideStringClass patches
				hookAsJump(0x005DDC50, 0x005DD4F0, WideStringClass::Convert_From); //WideStringClass patches
				hookAsJump(0x005ECE30, 0x005EC6D0, PATCH_Get_OS_Info); //patch Get_OS_Info to solve a crash on newer platforms
				hookAsJump(0x006703F5, 0x0066FC95, CombatManager::postSceneInit); //disable collision group for ladder fix
				hookAsJump(0x005E4220, 0x005E3AC0, SimpleFileFactoryClass::Get_File); //hooks for filehashing checks
				hookAsJump(0x005E4480, 0x005E3D20, SimpleFileFactoryClass::Return_File); //hooks for filehashing checks
				hookAsJump(0x006EC6F0, 0x006EBF90, FileFactoryListClass::Get_File); //hooks for filehashing checks
				hookAsJump(0x006EC7D0, 0x006EC070, FileFactoryListClass::Return_File); //hooks for filehashing checks
				hookAsJump(0x005EE320, 0x005EDBC0, GenericMultiListClass::Internal_Add); //MultiListClass hacks
				hookAsJump(0x005EE440, 0x005EDCE0, GenericMultiListClass::Internal_Add_Tail); //MultiListClass hacks
				hookAsJump(0x005EE550, 0x005EDDF0, GenericMultiListClass::Internal_Add_After); //MultiListClass hacks
				hookAsJump(0x005EE680, 0x005EDF20, GenericMultiListClass::Internal_Remove); //MultiListClass hacks
				hookAsJump(0x005EE710, 0x005EDFB0, GenericMultiListClass::Internal_Remove_List_Head); //MultiListClass hacks
				hookAsJump(0x006A8310, 0x006A7BB0, ScriptManager::Init); //hook ScriptManager
				hookAsJump(0x006A8330, 0x006A7BD0, ScriptManager::Shutdown); //hook ScriptManager
				hookAsJump(0x006A83D0, 0x006A7C70, ScriptManager::Destroy_Pending); //hook ScriptManager
				hookAsJump(0x006A8640, 0x006A7EE0, ScriptManager::Create_Script); //hook ScriptManager
				hookAsJump(0x006A87C0, 0x006A8060, ScriptManager::Save); //hook ScriptManager
				hookAsJump(0x006A89E0, 0x006A8280, ScriptManager::Load); //hook ScriptManager
				PacketManagerClass::_placeHooks(); //hook packetmgrclass
			}
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}
