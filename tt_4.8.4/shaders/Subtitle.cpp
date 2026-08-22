#include "General.h"
#include "Subtitle.h"



SubtitleUnk1::~SubtitleUnk1()
{
	delete[] unk0014;
}


RENEGADE_FUNCTION
bool Subtitle::Read(const char*, const char*)
AT1(0x007700B0);



Subtitle::Subtitle() :
	unk0000(0),
	unk0004(0),
	unk0008(0)
{
}



Subtitle::~Subtitle()
{
	if (unk0000)
	{
		for (int i = 0; i < unk0000->Count(); i++)
			delete (*unk0000)[i];
		
		delete unk0000;
	}
}



Subtitle* Subtitle::Create(const char* moviePath, const char* iniPath, FontCharsClass* font)
{
	if (!moviePath || strlen(moviePath) == 1 || iniPath && !font)
		return 0;
	
	Subtitle* subtitle = new Subtitle;

	if (font)
		subtitle->sentence.Set_Font(font);
	
	char movieName[_MAX_FNAME];
	_splitpath(moviePath, 0, 0, movieName, 0);

	if (subtitle->Read(movieName, iniPath))
		return subtitle;
	
	delete subtitle;
	return NULL;
}



bool Subtitle::Build(int arg)
{
	if (!unk0000)
		return 0;
	
	bool result = false;
	for (int i = unk0004; i < unk0000->Count(); i = unk0004)
	{
		if ((*unk0000)[i]->unk0000 > arg)
			break;
		
		unk0004++;
		unk0008 = (*unk0000)[unk0004];
		Build_Line(unk0008);
		result = true;
	}
	
	if (unk0008 && arg > unk0008->unk0000 + unk0008->unk0004)
	{
		unk0008 = NULL;
		sentence.Reset();
		return true;
	}
	
	return result;
}



void Subtitle::Render()
{
	sentence.Render();
}



RENEGADE_FUNCTION
void Subtitle::Build_Line(SubtitleUnk1*)
AT1(0x00770230);
