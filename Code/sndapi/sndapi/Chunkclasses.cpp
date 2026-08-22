#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "chunkclasses.h"
#include "FileClass.h"


ChunkLoadClass::ChunkLoadClass(FileClass *file){
    f = file;
	cur_chunk = 0;
	memset(chunk_offsets, 0, sizeof(chunk_offsets));
	memset(chunks, 0, sizeof(chunks));
	Is_Micro_Chunk_Open = false;
	Micro_Chunk_Offset = 0;
	W3dMicroChunkStruct.ID = 0;
	W3dMicroChunkStruct.LENGTH = 0;
}
bool ChunkLoadClass::Open_Chunk(){
	if (cur_chunk > 0){
		if (chunk_offsets[cur_chunk-1] == (chunks[cur_chunk-1].chunk_size & 0x7FFFFFFF)){
			return false;
		}            
	}
	if (f->Read(&chunks[cur_chunk], 8) != 8)
		return false;
	chunk_offsets[cur_chunk] = 0;
	cur_chunk++;
	return true;
}
bool ChunkLoadClass::Close_Chunk(){
	if (chunk_offsets[cur_chunk-1] < ((chunks[cur_chunk-1].chunk_size & 0x7FFFFFFF))){
		f->Seek((chunks[cur_chunk-1].chunk_size & 0x7FFFFFFF)-chunk_offsets[cur_chunk-1], SEEK_CUR);
	}    
	if ((--cur_chunk) > 0){
		chunk_offsets[cur_chunk-1] += 8 + (chunks[cur_chunk].chunk_size & 0x7FFFFFFF);
	}
	return true;
}
unsigned long ChunkLoadClass::Cur_Chunk_ID(){
	return chunks[cur_chunk-1].chunk_id;
}
unsigned long ChunkLoadClass::Cur_Chunk_Length(){
	return chunks[cur_chunk-1].chunk_size & 0x7FFFFFFF;
}
unsigned long ChunkLoadClass::Cur_Chunk_Depth(){
	return cur_chunk;
}
bool ChunkLoadClass::Contains_Chunks(){
	return (chunks[cur_chunk-1].chunk_size & 0x80000000) > 0;
}
bool ChunkLoadClass::Open_Mircro_Chunk(){
	if (Read(&W3dMicroChunkStruct, 2) != 2){
		return 0;
	}
	Is_Micro_Chunk_Open = true;
	Micro_Chunk_Offset = 0;
	return true;
}
bool ChunkLoadClass::Close_Mircro_Chunk(){
	Is_Micro_Chunk_Open = false;
	if (Micro_Chunk_Offset < W3dMicroChunkStruct.LENGTH){
		f->Seek(W3dMicroChunkStruct.LENGTH - Micro_Chunk_Offset, 1);
		if (cur_chunk > 0)
			chunk_offsets[cur_chunk-1] += (W3dMicroChunkStruct.LENGTH - Micro_Chunk_Offset);
	}
	return true;
}
unsigned long ChunkLoadClass::Current_Micro_Chunk_ID(){
	return W3dMicroChunkStruct.ID;
}
unsigned long ChunkLoadClass::Current_Micro_Chunk_Length(){
	return W3dMicroChunkStruct.LENGTH;
}
void ChunkLoadClass::Seek(unsigned long size){
	if (chunk_offsets[cur_chunk-1] + size > (chunks[cur_chunk-1].chunk_size & 0x7FFFFFFF))
		return;
	if (Is_Micro_Chunk_Open && Micro_Chunk_Offset + size > W3dMicroChunkStruct.LENGTH){
		return;
	}
	chunk_offsets[cur_chunk-1] += size;
	if (Is_Micro_Chunk_Open)
		Micro_Chunk_Offset += size;
	f->Seek(size, FILE_CURRENT);
}
unsigned long ChunkLoadClass::Read(void *buffer, unsigned long size){
	if (chunk_offsets[cur_chunk-1] + size > (chunks[cur_chunk-1].chunk_size & 0x7FFFFFFF))
		return 0;
	if (Is_Micro_Chunk_Open && Micro_Chunk_Offset + size > W3dMicroChunkStruct.LENGTH){
		return 0;
	}
	chunk_offsets[cur_chunk-1] += size;
	if (Is_Micro_Chunk_Open)
		Micro_Chunk_Offset += size;
	return f->Read(buffer, size);
}

ChunkSaveClass::ChunkSaveClass(FileClass *file){
    f = file;
	cur_chunk = 0;
	memset(chunk_offsets, 0, sizeof(chunk_offsets));
	memset(chunks, 0, sizeof(chunks));
	Is_Micro_Chunk_Open = false;
	Micro_Chunk_Offset = 0;
	W3dMicroChunkStruct.ID = 0;
	W3dMicroChunkStruct.LENGTH = 0;
}
bool ChunkSaveClass::Begin_Chunk(unsigned long Chunk_ID){
	W3dChunkStruct chunk = {0,0};
	if (cur_chunk > 0){
		chunks[cur_chunk-1].chunk_size |= 0x80000000;
	}
	chunk.chunk_id = Chunk_ID;
	chunk.chunk_size &= 0x80000000;
	chunk_offsets[cur_chunk] = f->Seek(0, 1);
	chunks[cur_chunk] = chunk;
	cur_chunk++;
	if (f->Write(&chunk, 8) == 8)
		return true;
	else return false;
}
bool ChunkSaveClass::End_Chunk(){
	unsigned long cur_pos = f->Seek(0, 1);
	W3dChunkStruct chunk;
	cur_chunk--;
	chunk = chunks[cur_chunk];
	f->Seek(chunk_offsets[cur_chunk], FILE_BEGIN);
	if (f->Write(&chunk, 8) != 8)
		return false;
	if (cur_chunk > 0){
		chunks[cur_chunk-1].chunk_size = ((chunk.chunk_size & 0x7FFFFFFF) + (chunks[cur_chunk-1].chunk_size & 0x7FFFFFFF) + 8) | (chunks[cur_chunk-1].chunk_size & 0x80000000);
	}
	f->Seek(cur_pos, FILE_BEGIN);
	return true;
}
bool ChunkSaveClass::Begin_Micro_Chunk(unsigned long Chunk_ID){
	W3dMicroChunkStruct.ID = (unsigned char)Chunk_ID;
	W3dMicroChunkStruct.LENGTH = 0;
	Micro_Chunk_Offset = f->Seek(0, FILE_CURRENT);
	if (f->Write(&W3dMicroChunkStruct, 2) == 2){
		chunks[cur_chunk-1].chunk_size =  (2 + (chunks[cur_chunk-1].chunk_size & 0x7FFFFFFF)) | ((chunks[cur_chunk-1].chunk_size) & 0x80000000);
		if (Is_Micro_Chunk_Open)
			W3dMicroChunkStruct.LENGTH += 2;
		Is_Micro_Chunk_Open = true;
		return 1;
	}
	return false;
}
bool ChunkSaveClass::End_Micro_Chunk(){
	unsigned long cur_pos = f->Seek(0, FILE_CURRENT);
	f->Seek(Micro_Chunk_Offset, FILE_BEGIN);
	if (f->Write(&W3dMicroChunkStruct, 2) != 2)
		return false;
	f->Seek(cur_pos, FILE_BEGIN);
	Is_Micro_Chunk_Open = false;
	return true;
}
unsigned long ChunkSaveClass::Write(void *buffer,unsigned long size){
	if (f->Write(buffer, size) != size){
		return 0;
	}
	chunks[cur_chunk-1].chunk_size =  (size + (chunks[cur_chunk-1].chunk_size & 0x7FFFFFFF)) | ((chunks[cur_chunk-1].chunk_size) & 0x80000000);
	if (Is_Micro_Chunk_Open)
		W3dMicroChunkStruct.LENGTH += (unsigned char)size;
	return size;
}