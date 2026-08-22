#ifndef TT_INCLUDE__CONSOLEMODECLASS_H
#define TT_INCLUDE__CONSOLEMODECLASS_H
#include "engine_string.h"
#include "Vector3.h"
class ConsoleModeClass
{
public:
	HANDLE StdInputHandle; //0
	HANDLE StdOutputHandle; //4
	HWND ConsoleWindowHandle; //8
	int unkC; //C
	int unk10; //10
	char Title[0x100]; //14
	bool IsExclusive; //114
	bool unk115; //115
	int unk118; //118
	int unk11C; //11C
	char *Get_Log_File_Name();
	void Log_To_Disk(char const*);
	void Add_Message(WideStringClass*, Vector3*, bool);
	void Print(char const *,...);
	void Print_Maybe(char const *,...);
	void cPrintf(char const *,...);
	bool Is_Exclusive()
	{
		return IsExclusive;
	}
	void Apply_Attributes();
}; //0120
extern REF_DECL2(ConsoleBox, ConsoleModeClass);

#endif
