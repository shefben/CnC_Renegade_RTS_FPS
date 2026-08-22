#include "General.h"
#include "ConsoleModeClass.h"
#include "varargs.h"
#include "SysTimeClass.h"
#include "ServerSettingsClass.h"
REF_DEF2(ConsoleBox, ConsoleModeClass, 0x0081E228, 0x0081D410);
void ConsoleModeClass::Print(char const *format,...)
{
	va_list arguments;
	va_start(arguments, format);
	if (StdOutputHandle != INVALID_HANDLE_VALUE)
	{
		char temp[0x2000];
		vsprintf(temp,format,arguments);
		ConsoleModeClass::cPrintf("%s",temp);
		ConsoleModeClass::Log_To_Disk(temp);
		Apply_Attributes();
	}
}
void ConsoleModeClass::Print_Maybe(char const *format,...)
{
	va_list arguments;
	va_start(arguments, format);
	if (StdOutputHandle != INVALID_HANDLE_VALUE)
	{
		char temp[0x2000];
		vsprintf(temp,format,arguments);
		ConsoleModeClass::cPrintf("%s",temp);
		ConsoleModeClass::Log_To_Disk(temp);
		Apply_Attributes();
	}
}
char *ConsoleModeClass::Get_Log_File_Name()
{
	static char _log_file_name[0x100];
	SYSTEMTIME time;
	GetLocalTime(&time);
	sprintf(_log_file_name,"renlog_%d-%d-%02d.txt",time.wMonth,time.wDay,time.wYear);
	static unsigned int _last_day = 0xFFFFFFFF;
	if (_last_day != time.wDay)
	{
		if (ServerSettingsClass::DiskLogSize != -1)
		{
			_last_day = time.wDay;
			FILETIME ftime;
			if (SystemTimeToFileTime(&time,&ftime))
			{
				__int64 size = 864000000000i64 * ServerSettingsClass::DiskLogSize;
				FILETIME *ftime3 = (FILETIME *)&size;
				FILETIME ftime2;
				ftime2.dwLowDateTime = ftime.dwLowDateTime - ftime3->dwLowDateTime;
				ftime2.dwHighDateTime = ftime.dwHighDateTime - ((ftime.dwLowDateTime < ftime3->dwLowDateTime) + 711573504 * (ServerSettingsClass::DiskLogSize >> 31) + 201 * ServerSettingsClass::DiskLogSize + ftime3->dwHighDateTime);
				WIN32_FIND_DATAA data;
				HANDLE handle = ::FindFirstFileA("renlog_*.txt", &data);
				if (handle != INVALID_HANDLE_VALUE)
				{
					do
					{
						if (CompareFileTime(&data.ftLastWriteTime,&ftime2) == -1)
						{
							DeleteFile(data.cFileName);
						}
					}
					while (::FindNextFileA (handle, &data));
					::FindClose (handle);
				}
			}
		}
	}
	return _log_file_name;
}
RENEGADE_FUNCTION
void ConsoleModeClass::cPrintf(char const *,...)
AT2(0x00435E60,0x00435F00);
void ConsoleModeClass::Log_To_Disk(char const* msg)
{
	if (StdOutputHandle != INVALID_HANDLE_VALUE)
	{
		if (ServerSettingsClass::DiskLogSize > 0)
		{
			FILE *f = fopen(Get_Log_File_Name(),"at");
			if (f)
			{
				char time[256];
				memset(time,0,sizeof(time));
				time[0] = 0x3F;
				GetTimeFormat(LOCALE_SYSTEM_DEFAULT,TIME_FORCE24HOURFORMAT,0,"'['HH':'mm':'ss'] '",time,0xFF);
				fwrite(time,1,strlen(time),f);
				fwrite(msg,1,strlen(msg),f);
				fclose(f);
			}
		}
	}
}
void ConsoleModeClass::Add_Message(WideStringClass* str, Vector3* color, bool b)
{
	if (!unk115)
	{
		if (str)
		{
			if (color)
			{
				bool print = false;
				if (b)
				{
					print = true;
				}
				if (unk10 == 1)
				{
					uint32 time = TIMEGETTIME();
					time = time - unkC;
					if (time > 3000)
					{
						print = true;
					}
				}
				if (print)
				{
					StringClass str2(128,true);
					str2.Copy_Wide(*str);
					cPrintf("%s",(const char *)str2);
					Log_To_Disk(str2);
					Apply_Attributes();
				}
			}
		}
	}
}
RENEGADE_FUNCTION
void ConsoleModeClass::Apply_Attributes()
AT2(0x004367E0,0x00436880);
