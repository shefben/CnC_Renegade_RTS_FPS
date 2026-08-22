#ifndef TT_INCLUDE__RADIOCOMMANDDISPLAYCLASS_H
#define TT_INCLUDE__RADIOCOMMANDDISPLAYCLASS_H



class RadioCommandDisplayClass
{

public:

	enum DISPLAY_TYPE;

	static void Shutdown();
	static void Initialize();
	static void Update(DISPLAY_TYPE);
	static void Display(bool, DISPLAY_TYPE);
	static void Render();
	static void Check_Keys();
};



#endif