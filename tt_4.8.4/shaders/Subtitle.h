#ifndef TT_INCLUDE__SUBTITLE_H
#define TT_INCLUDE__SUBTITLE_H



#include "engine_vector.h"
#include "render2d.h"



class SubtitleUnk1
{

public:

	int unk0000;
	int unk0004;
	int unk0008;
	int unk0010;
	wchar_t *unk0014;

	virtual ~SubtitleUnk1();
	
};



class Subtitle
{

private:

	DynamicVectorClass<SubtitleUnk1*>* unk0000;
	int unk0004;
	SubtitleUnk1* unk0008;
	Render2DSentenceClass sentence;
	//...?

public:

	SHADERS_API static Subtitle* Create(const char* moviePath, const char* iniPath, FontCharsClass* font);

	Subtitle();
	~Subtitle();
	SHADERS_API bool Build(int);
	void Render();
	bool Read(const char*, const char*);
	void Build_Line(SubtitleUnk1*);

};



#endif