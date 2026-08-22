#ifndef TT_INCLUDE__LANGAMEMODECLASS_H
#define TT_INCLUDE__LANGAMEMODECLASS_H



#include "GameModeClass.h"



class cLanChat;



class LanGameModeClass :
	public GameModeClass
{

public:

	static REF_DECL2(PLanChat, cLanChat*);

	static cLanChat* Get_Lan_Interface();

	void Init();
	void Shutdown();
	void Think();
	~LanGameModeClass();
	const char* Name();
	void Render();

};



#define PLC (LanGameModeClass::Get_Lan_Interface())



#endif