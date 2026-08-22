#ifndef TT_INCLUDE__SOUNDHANDLECLASS_H
#define TT_INCLUDE__SOUNDHANDLECLASS_H



class Sound3DHandleClass;
class Sound2DHandleClass;
class SoundStreamHandleClass;
class ListenerHandleClass;
class SoundBufferClass;



class SoundHandleClass
{

public:

	virtual ~SoundHandleClass();
	virtual Sound3DHandleClass* As_Sound3DHandleClass() { return NULL; }
	virtual Sound2DHandleClass* As_Sound2DHandleClass() { return NULL; }
	virtual SoundStreamHandleClass* As_SoundStreamHandleClass() { return NULL; }
	virtual ListenerHandleClass* As_ListenerHandleClass() { return NULL; }
	virtual UNK Get_H3DSAMPLE();
	virtual UNK Get_HSAMPLE();
	virtual UNK Get_HSTREAM();
	virtual UNK Set_Miles_Handle(uint32) = 0;
	virtual UNK Initialize(SoundBufferClass*);
	virtual void Start_Sample() = 0;
	virtual void Stop_Sample() = 0;
	virtual void Resume_Sample() = 0;
	virtual void End_Sample() = 0;
	virtual UNK Set_Sample_Pan(sint32) = 0;
	virtual UNK Get_Sample_Pan() = 0;
	virtual UNK Set_Sample_Volume(sint32) = 0;
	virtual UNK Get_Sample_Volume() = 0;
	virtual UNK Set_Sample_Loop_Count(uint32) = 0;
	virtual UNK Get_Sample_Loop_Count() = 0;
	virtual UNK Set_Sample_MS_Position(uint32) = 0;
	virtual UNK Get_Sample_MS_Position(sint32*, sint32*) = 0;
	virtual UNK Set_Sample_User_Data(sint32, uint32) = 0;
	virtual UNK Get_Sample_User_Data(sint32) = 0;
	virtual UNK Get_Sample_Playback_Rate() = 0;
	virtual UNK Set_Sample_Playback_Rate(sint32) = 0;

	SoundHandleClass();

};



#endif