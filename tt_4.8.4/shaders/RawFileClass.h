#ifndef TT_INCLUDE_RAWFILECLASS_H
#define TT_INCLUDE_RAWFILECLASS_H



#include "engine_io.h"
#include "engine_math.h"


class RawFileClass : public FileClass {
private:
	int Rights; // 0004
	int BiasStart; // 0008
	int BiasLength; // 000C
	void* Handle; // 0010
	StringClass Filename; // 0014
	unsigned short Date; // 0018
	unsigned short Time; // 001C
public:
	RawFileClass()
	{
		BiasLength = -1;
		Handle = INVALID_HANDLE_VALUE;
		Rights = 0;
		BiasStart = 0;
		Date = 0;
		Time = 0;
		Filename = "";
	}
	RawFileClass(const char* filename)
	{
		BiasLength = -1;
		Handle = INVALID_HANDLE_VALUE;
		Rights = 0;
		BiasStart = 0;
		Date = 0;
		Time = 0;
		Filename = filename;
	}
	int Transfer_Block_Size()
	{
		return -11;
	}
	void Reset()
	{
		Close();
		Filename = "";
	}
	~RawFileClass()
	{
		Reset();
	}
	const char *File_Name()
	{
		return Filename;
	}
	const char *Set_Name(const char* name)
	{
		Reset();
		if (name)
		{
			Bias(0,-1);
			Filename = name;
		}
		return 0;
	}
	bool Create()
	{
		Close();
		if (!Open(2))
		{
			return false;
		}
		if (BiasLength != -1)
		{
			Seek(0,0);
		}
		Close();
		return true;
	}
	bool Delete()
	{
		Close();
		if (Filename.Is_Empty())
		{
			Error(2,0,0);
		}
		if (!Is_Available(0))
		{
			return false;
		}
		if (!DeleteFile(Filename))
		{
			Error(GetLastError(),0,Filename);
			return false;
		}
		return true;
	}
	bool Is_Available(int _handle)
	{
		if (Filename.Is_Empty())
			return false;
		
		if (Is_Open())
			return true;
		
		if (_handle)
		{
			Open(1);
			Close();
			return true;
		}
		else
		{
			Handle = CreateFile(Filename,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
			if (Handle == INVALID_HANDLE_VALUE)
				return false;
			
			if (!CloseHandle(Handle))
				Error(GetLastError(),0,Filename);
			
			Handle = INVALID_HANDLE_VALUE;
		}
		return true;
	}
	bool Is_Open()
	{
		if (Handle != INVALID_HANDLE_VALUE)
		{
			return true;
		}
		return false;
	}
	int Open(const char* name, int mode)
	{
		Set_Name(name);
		return Open(mode);
	}
	int Open(int mode)
	{
		Close();
		if (Filename.Is_Empty())
		{
			Error(2,0,0);
		}
		Rights = mode;
		switch (Rights)
		{
		case 2:
			Handle = CreateFile(Filename,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
			break;
		case 1:
			Handle = CreateFile(Filename,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
			break;
		case 0:
			Handle = CreateFile(Filename,GENERIC_READ|GENERIC_WRITE,0,0,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
			break;
		default:
			errno = EINVAL;
		}
		if ((BiasStart) && (BiasLength != -1))
		{
			Seek(0,0);
		}
		if (Handle != INVALID_HANDLE_VALUE)
		{
			return true;
		}
		return false;
	}


	int RawFileClass::Read(void* buffer, int bytesToRead)
	{
		DWORD bytesRead = 0;
		if (!Is_Open())
		{
			if (Open(1))
			{
				bytesRead = Read(buffer, bytesToRead);
				Close();
			}
		}
		else
		{
			if (BiasLength != -1)
			{
				int maxBytesToRead = BiasLength - Tell();
				if (bytesToRead > maxBytesToRead)
					bytesToRead = maxBytesToRead;
			}
			
			if (!ReadFile(Handle, buffer, bytesToRead, &bytesRead, NULL))
				Error(GetLastError(), 1, Filename);
		}
		
		return bytesRead;
	}


	int Seek(int pos, int dir)
	{
		if (BiasLength != -1)
		{
			switch (dir)
			{
			case 1: pos += Raw_Seek(0, 1) - BiasStart; break;
			case 2: pos += BiasLength; break;
			}

			int result = Raw_Seek(BiasStart + clamp(pos, 0, BiasLength), 0);
			if (result != -1)
				result -= BiasStart;

			TT_ASSERT(result <= BiasLength);
			return result;
		}
		else
			return Raw_Seek(pos, dir);
	}


	int Raw_Seek(int pos, int dir)
	{
		if (!Is_Open())
			Error(9, 0, Filename);
		
		int seek = SetFilePointer(Handle, pos, 0, dir);
		if (seek == -1)
			Error(GetLastError(), 0, Filename);
		
		return seek;
	}


	int Size()
	{
		if (BiasLength != -1)
			return BiasLength;
		else
		{
			int size = -1;
			if (!Is_Open())
			{
				if (Open(1))
				{
					size = Size();
					Close();
				}
			}
			else
			{
				size = GetFileSize(Handle,0);
				if (size == -1)
					Error(GetLastError(), 0, Filename);
			}
			return size;
		}
	}


	int Write(void* buffer, int size)
	{
		DWORD bytesWritten = 0;
		if (!Is_Open())
		{
			if (Open(2))
			{
				bytesWritten = Write(buffer, size);
				Close();
			}
		}
		else
		{
			if (!WriteFile(Handle, buffer, size, &bytesWritten, 0))
				Error(GetLastError(), 0, Filename);
			
			if (BiasLength != -1)
			{
				if (Raw_Seek(0, 1) > BiasStart + BiasLength)
				{
					BiasLength = Raw_Seek(0, 1) - BiasStart;
				}
			}
		}
		return bytesWritten;
	}


	void Close()
	{
		if (Is_Open())
		{
			if (!CloseHandle(Handle))
				Error(GetLastError(), 0, Filename);
			
			Handle = INVALID_HANDLE_VALUE;
		}
	}


	unsigned long Get_Date_Time()
	{
		BY_HANDLE_FILE_INFORMATION info;
		unsigned short dostime;
		unsigned short dosdate;
		if (GetFileInformationByHandle(Handle,&info))
		{
			FileTimeToDosDateTime(&info.ftLastWriteTime, &dosdate, &dostime);
			return dosdate << 0x10 | dostime;
		}
		return 0;
	}
	bool Set_Date_Time(unsigned long datetime)
	{
		BY_HANDLE_FILE_INFORMATION info;
		FILETIME filetime;
		if (Is_Open())
		{
			if (GetFileInformationByHandle(Handle,&info))
			{
				if (DosDateTimeToFileTime((WORD)(datetime >> 0x10),(WORD)datetime,&filetime))
				{
					if (SetFileTime(Handle,&info.ftCreationTime,&filetime,&filetime))
					{
						return true;
					}
				}
			}
		}
		return false;
	}
	void Error(int a, int b, const char *c)
	{
	}
	HANDLE Get_File_Handle()
	{
		return Handle;
	}
	void Bias(int start, int length)
	{
		if (start == 0)
		{
			TT_ASSERT(length == -1);
			BiasStart = 0;
			BiasLength = -1;
		}
		else
		{
			BiasLength = Size();
			BiasStart += start;
			if (length != -1)
			{
				if (length < BiasLength)
					BiasLength = length;
				
				if (BiasLength < 0)
					BiasLength = 0;
			}
		}
	}
	void Attach(HANDLE handle,int rights)
	{
		Reset();
		Handle = handle;
		Rights = rights;
		BiasStart = 0;
		BiasLength = -1;
		Date = 0;
		Time = 0;
	}
	void Detach()
	{
		Rights = 0;
		Handle = INVALID_HANDLE_VALUE;
		BiasStart = 0;
		BiasLength = -1;
		Date = 0;
		Time = 0;
	}
}; // 0020

#endif
