#ifndef _CHUNKCLASSES_H_
#define _CHUNKCLASSES_H_
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "FileClass.h"
//#pragma warning (disable: 4091)


#define MAX_CHUNKS 0x100

typedef struct _W3dChunkStruct{
	unsigned long chunk_id;
	unsigned long chunk_size;
} W3dChunkStruct;


class ChunkLoadClass {
private:
	FileClass *f;
	int cur_chunk;
	unsigned long chunk_offsets[MAX_CHUNKS];
	W3dChunkStruct chunks[MAX_CHUNKS]; 
	bool Is_Micro_Chunk_Open;
	unsigned long Micro_Chunk_Offset;
	struct {
		unsigned char ID;
		unsigned char LENGTH;
	} W3dMicroChunkStruct;
public:
	ChunkLoadClass(FileClass *file);
	bool Open_Chunk();
	bool Close_Chunk();
	unsigned long Cur_Chunk_ID();
	unsigned long Cur_Chunk_Length();
	unsigned long Cur_Chunk_Depth();
	bool Contains_Chunks();
	bool Open_Mircro_Chunk();
	bool Close_Mircro_Chunk();
	unsigned long Current_Micro_Chunk_ID();
	unsigned long Current_Micro_Chunk_Length();
	void Seek(unsigned long size);
	unsigned long Read(void *buffer, unsigned long size);
};

class ChunkSaveClass {
private:
	FileClass *f;
	int cur_chunk;
	unsigned long chunk_offsets[MAX_CHUNKS];
	W3dChunkStruct chunks[MAX_CHUNKS]; 
	bool Is_Micro_Chunk_Open;
	unsigned long Micro_Chunk_Offset;
	struct {
		unsigned char ID;
		unsigned char LENGTH;
	} W3dMicroChunkStruct;
public:
	ChunkSaveClass(FileClass *file);
	bool Begin_Chunk(unsigned long Chunk_ID);
	bool End_Chunk();
	bool Begin_Micro_Chunk(unsigned long Chunk_ID);
	bool End_Micro_Chunk();
	unsigned long Write(void *buffer,unsigned long size);
};

#endif //_CHUNKCLASSES_H_