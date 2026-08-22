#ifndef TT_INCLUDE_SOUNDBUFFERCLASS_H
#define TT_INCLUDE_SOUNDBUFFERCLASS_H
#include "engine_vector.h"

class SoundBufferClass : public RefCountClass {
public:
	char*  cBuffer;    // 32
	uint32 uLength;    // 36
	char*  cFile;      // 40
	uint32 uDuration;  // 44
	uint32 uRate;      // 48
	uint32 uBits;      // 52
	uint32 uChannels;  // 56
	uint32 uType;      // 60
	virtual bool        Load_From_File   (const char* cFile);
	virtual bool        Load_From_File   (FileClass& xFile);
	virtual bool        Load_From_Memory (const char* cBuffer, uint32 uLength);
	virtual const char* Get_Raw_Buffer   () const;
	virtual uint32      Get_Raw_Length   () const;
	virtual const char* Get_Filename     () const;
	virtual void        Set_Filename     (const char* cFile);
	virtual uint32      Get_Duration     () const;
	virtual uint32      Get_Rate         () const;
	virtual uint32      Get_Bits         () const;
	virtual uint32      Get_Channels     () const;
	virtual uint32      Get_Type         () const;
	virtual bool        Is_Streaming     () const;
	virtual void        Free_Buffer      ();
	virtual void        Determine_Stats  (char*);
};

#endif