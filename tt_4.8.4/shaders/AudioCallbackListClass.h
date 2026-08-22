#ifndef TT_INCLUDE__AUDIOCALLBACKLISTCLASS_H
#define TT_INCLUDE__AUDIOCALLBACKLISTCLASS_H



#include "engine_vector.h"



template<typename Callback>
struct AUDIO_CALLBACK_STRUCT
{
	Callback callback; // 0000
	uint32 userData; // 0004
}; // 0008



template<typename Callback>
class AudioCallbackListClass :
	public SimpleDynVecClass<AUDIO_CALLBACK_STRUCT<Callback>>
{

public:

	Callback Get_Callback(int index, uint32* userData)
	{
		if (userData)
			*userData = (*this)[index].userData;
		return (*this)[index].callback;
	}



	void Add_Callback(Callback callback, uint32 userData)
	{
		AUDIO_CALLBACK_STRUCT callbackStruct = {callback, userData};
		Add(callbackStruct);
	}



	void Remove_Callback(Callback callback)
	{
		for (int index = 0; index < Count(); ++index)
		{
			if ((*this)[index].callback == callback)
			{
				Remove(index);
				break;
			}
		}
	}

};



#endif