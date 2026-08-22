#ifndef TT_INCLUDE__LOADINGSCREENCLASS_H
#define TT_INCLUDE__LOADINGSCREENCLASS_H



#include "menubackdrop.h"
#include "render2d.h"



class LoadingScreenClass
{

	MenuBackDropClass unk0000; // 0000
	Render2DSentenceClass unk001C; // 001C
	Render2DSentenceClass unk015C; // 015C
	float elapsedTime; // 029C
	float currentTime; // 02A0
	float progress; // 02A4
	float phaseEndTime; // 02A8
	float timeMultiplier; // 02AC

public:

	static float computePhaseEndTime(uint stage);

	LoadingScreenClass();
	void Render(bool);

};



#endif
