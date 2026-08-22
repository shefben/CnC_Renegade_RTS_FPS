#include "ChunkClasses.h"

ChunkLoadClass::ChunkLoadClass(){
	m_CurrentChunk = 0;
	memset(m_ChunkOffsets, 0, sizeof(m_ChunkOffsets));
	memset(m_Chunks, 0, sizeof(m_Chunks));
	m_IsMicroChunkOpen = false;
	m_MicroChunkOffset = 0;
	m_MicroChunk.m_ID = 0;
	m_MicroChunk.m_Length = 0;
}


void ChunkLoadClass::SetFile(file f){
    m_hFile = f;
}

bool ChunkLoadClass::OpenChunk(){
	if (m_CurrentChunk > 0){
		if (m_ChunkOffsets[m_CurrentChunk-1] == (m_Chunks[m_CurrentChunk-1].m_ChunkSize & 0x7FFFFFFF)){
			return false;
		}            
	}
	if (fread_(&m_Chunks[m_CurrentChunk], 8, m_hFile) != 8)
		return false;
	m_ChunkOffsets[m_CurrentChunk] = 0;
	m_CurrentChunk++;
	return true;
}

bool ChunkLoadClass::CloseChunk(){
	if (m_ChunkOffsets[m_CurrentChunk-1] < ((m_Chunks[m_CurrentChunk-1].m_ChunkSize & 0x7FFFFFFF))){
		fseek_(m_hFile, (m_Chunks[m_CurrentChunk-1].m_ChunkSize & 0x7FFFFFFF)-m_ChunkOffsets[m_CurrentChunk-1], SEEK_CUR);
	}    
	if ((--m_CurrentChunk) > 0){
		m_ChunkOffsets[m_CurrentChunk-1] += 8 + (m_Chunks[m_CurrentChunk].m_ChunkSize & 0x7FFFFFFF);
	}
	return true;
}

size_32 ChunkLoadClass::CurChunkID(){
	return m_Chunks[m_CurrentChunk-1].m_ChunkID;
}

size_32 ChunkLoadClass::CurChunkLength(){
	return m_Chunks[m_CurrentChunk-1].m_ChunkSize & 0x7FFFFFFF;
}
size_32 ChunkLoadClass::CurChunkDepth(){
	return m_CurrentChunk;
}

bool ChunkLoadClass::ContainsChunks(){
	return (m_Chunks[m_CurrentChunk-1].m_ChunkSize & 0x80000000) > 0;
}

bool ChunkLoadClass::OpenMicroChunk(){
	if (Read(&m_MicroChunk, sizeof(m_MicroChunk)) != sizeof(m_MicroChunk)){
		return 0;
	}
	m_IsMicroChunkOpen = true;
	m_MicroChunkOffset = 0;
	return true;
}

bool ChunkLoadClass::CloseMicroChunk(){
	m_IsMicroChunkOpen = false;
	if (m_MicroChunkOffset < m_MicroChunk.m_Length){
		fseek_(m_hFile, m_MicroChunk.m_Length - m_MicroChunkOffset, SEEK_CUR);
		if (m_CurrentChunk > 0)
			m_ChunkOffsets[m_CurrentChunk-1] += (m_MicroChunk.m_Length - m_MicroChunkOffset);
	}
	return true;
}

size_16 ChunkLoadClass::CurrentMicroChunkID(){
	return m_MicroChunk.m_ID;
}

size_16 ChunkLoadClass::CurrentMicroChunkLength(){
	return m_MicroChunk.m_Length;
}

void ChunkLoadClass::Seek(size_32 size){
	if (m_ChunkOffsets[m_CurrentChunk-1] + size > (m_Chunks[m_CurrentChunk-1].m_ChunkSize & 0x7FFFFFFF))
		return;
	if (m_IsMicroChunkOpen && m_MicroChunkOffset + size > m_MicroChunk.m_Length){
		return;
	}
	m_ChunkOffsets[m_CurrentChunk-1] += size;
	if (m_IsMicroChunkOpen)
		m_MicroChunkOffset += size;
	fseek_(m_hFile, size, FILE_CURRENT);
}

size_32 ChunkLoadClass::Read(void *buffer, size_32 size){
	if (m_ChunkOffsets[m_CurrentChunk-1] + size > (m_Chunks[m_CurrentChunk-1].m_ChunkSize & 0x7FFFFFFF))
		size = (m_Chunks[m_CurrentChunk-1].m_ChunkSize & 0x7FFFFFFF)-m_ChunkOffsets[m_CurrentChunk-1];
	if (m_IsMicroChunkOpen && m_MicroChunkOffset + size > m_MicroChunk.m_Length){
		return 0;
	}
	if (((m_Chunks[m_CurrentChunk-1].m_ChunkSize & 0x7FFFFFFF) - m_ChunkOffsets[m_CurrentChunk-1]) < size)
		size = ((m_Chunks[m_CurrentChunk-1].m_ChunkSize & 0x7FFFFFFF) - m_ChunkOffsets[m_CurrentChunk-1]);

	m_ChunkOffsets[m_CurrentChunk-1] += size;
	
	if (m_IsMicroChunkOpen)
		m_MicroChunkOffset += size;
	return fread_(buffer, size, m_hFile);
}

ChunkSaveClass::ChunkSaveClass(){
	m_CurrentChunk = 0;
	memset(m_ChunkOffsets, 0, sizeof(m_ChunkOffsets));
	memset(m_Chunks, 0, sizeof(m_Chunks));
	m_IsMicroChunkOpen = false;
	m_MicroChunkOffset = 0;
	m_MicroChunk.m_ID = 0;
	m_MicroChunk.m_Length = 0;
}

void ChunkSaveClass::SetFile(file f){
    m_hFile = f;
}

bool ChunkSaveClass::BeginChunk(size_32 ChunkID){
	ChunkStruct chunk = {0,0};
	if (m_CurrentChunk > 0){
		m_Chunks[m_CurrentChunk-1].m_ChunkSize |= 0x80000000;
	}
	chunk.m_ChunkID = ChunkID;
	chunk.m_ChunkSize &= 0x80000000;
	m_ChunkOffsets[m_CurrentChunk] = ftell_(m_hFile);
	m_Chunks[m_CurrentChunk] = chunk;
	m_CurrentChunk++;
	if (fwrite_(&chunk, 8, m_hFile) == 8)
		return true;
	else return false;
}

bool ChunkSaveClass::EndChunk(){
	size_32 cur_pos = ftell_(m_hFile);
	ChunkStruct chunk;
	m_CurrentChunk--;
	chunk = m_Chunks[m_CurrentChunk];
	fseek_(m_hFile, m_ChunkOffsets[m_CurrentChunk], SEEK_SET);
	if (fwrite_(&chunk, 8, m_hFile) != 8)
		return false;
	if (m_CurrentChunk > 0){
		m_Chunks[m_CurrentChunk-1].m_ChunkSize = ((chunk.m_ChunkSize & 0x7FFFFFFF) + (m_Chunks[m_CurrentChunk-1].m_ChunkSize & 0x7FFFFFFF) + 8) | (m_Chunks[m_CurrentChunk-1].m_ChunkSize & 0x80000000);
	}
	fseek_(m_hFile, cur_pos, SEEK_SET);
	return true;
}

bool ChunkSaveClass::BeginMicroChunk(size_16 ChunkID){
	m_MicroChunk.m_ID = ChunkID;
	m_MicroChunk.m_Length = 0;
	m_MicroChunkOffset = ftell_(m_hFile);
	if (fwrite_(&m_MicroChunk, sizeof(m_MicroChunk), m_hFile) == sizeof(m_MicroChunk)){
		m_Chunks[m_CurrentChunk-1].m_ChunkSize =  (sizeof(m_MicroChunk) + (m_Chunks[m_CurrentChunk-1].m_ChunkSize & 0x7FFFFFFF)) | ((m_Chunks[m_CurrentChunk-1].m_ChunkSize) & 0x80000000);
		if (m_IsMicroChunkOpen)
			m_MicroChunk.m_Length += sizeof(m_MicroChunk);
		m_IsMicroChunkOpen = true;
		return 1;
	}
	return false;
}

bool ChunkSaveClass::EndMicroChunk(){
	size_32 cur_pos = ftell_(m_hFile);
	fseek_(m_hFile, m_MicroChunkOffset, SEEK_SET);
	if (fwrite_(&m_MicroChunk, sizeof(m_MicroChunk), m_hFile) != sizeof(m_MicroChunk))
		return false;
	fseek_(m_hFile, cur_pos, SEEK_SET);
	m_IsMicroChunkOpen = false;
	return true;
}

size_32 ChunkSaveClass::Write(const void *buffer,size_32 size){
	if (m_IsMicroChunkOpen && (size + (size_32)m_MicroChunk.m_Length) > 0xFFFF)
		size = 0xFFFF-m_MicroChunk.m_Length;
	if (fwrite_((void*)buffer, size, m_hFile) != size){
		return 0;
	}
	m_Chunks[m_CurrentChunk-1].m_ChunkSize =  (size + (m_Chunks[m_CurrentChunk-1].m_ChunkSize & 0x7FFFFFFF)) | ((m_Chunks[m_CurrentChunk-1].m_ChunkSize) & 0x80000000);
	if (m_IsMicroChunkOpen)
		m_MicroChunk.m_Length += (size_16)size;
	return size;
}