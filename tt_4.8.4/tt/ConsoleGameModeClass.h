#ifndef TT_INCLUDE__CONSOLEGAMEMODECLASS_H
#define TT_INCLUDE__CONSOLEGAMEMODECLASS_H



#include "Vector3.h"
#include "GameModeClass.h"



class ConsoleGameModeClass :
	public GameModeClass
{
	
	static REF_DECL2(LeftMargin, UNK);
	static REF_DECL2(Instance, ConsoleGameModeClass*);

public:

	UNK unk0008[0x51];
	Vector3 unk014C; // 014C
	float unk0158; // 0158
	//...?

public:

	void Init();
	void Shutdown();
	bool Load_Registry_Keys();
	bool Save_Registry_Keys();
	void Think();
	UNK Parse_Input(char*);
	UNK Clear_Suggestion();
	UNK Accept_Suggestion(char*);
	UNK Update_Suggestion(char*, bool);
	UNK Profile_Command(const char*);
	UNK Update_Profile();
	UNK Begin_Profile_Log();
	UNK End_Profile_Log();
	UNK Process_Profile_Log();
	UNK Update_Memory_Log();
	~ConsoleGameModeClass();
	const char* Name();
	void Render();
	bool Is_FPS_Active();
	UNK Set_FPS_Active(bool);
	UNK Toggle_Player_Position();
	UNK Set_Player_Position(Vector3&, float);

	static ConsoleGameModeClass* Get_Instance() { return Instance; }

};



#endif