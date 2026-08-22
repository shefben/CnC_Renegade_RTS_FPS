#include "StdLib.h"


#define MAX_CHUNKS 0x100

typedef struct _ChunkStruct{
	size_32		m_ChunkID;
	size_32		m_ChunkSize;
} ChunkStruct;


class ChunkLoadClass {
private:
	file		m_hFile;
	int			m_CurrentChunk;
	size_32		m_ChunkOffsets[MAX_CHUNKS];
	ChunkStruct	m_Chunks[MAX_CHUNKS];
	bool		m_IsMicroChunkOpen;
	size_32		m_MicroChunkOffset;
	struct {
		size_16		m_ID;
		size_16		m_Length;
	} m_MicroChunk;
public:
	ChunkLoadClass();
	void SetFile(file f);
	bool OpenChunk();
	bool CloseChunk();
	size_32 CurChunkID();
	size_32 CurChunkLength();
	size_32 CurChunkDepth();
	bool ContainsChunks();
	bool OpenMicroChunk();
	bool CloseMicroChunk();
	size_16 CurrentMicroChunkID();
	size_16 CurrentMicroChunkLength();
	void Seek(size_32 size);
	size_32 Read(void *buffer, size_32 size);
};

class ChunkSaveClass {
private:
	file		m_hFile;
	int			m_CurrentChunk;
	size_32		m_ChunkOffsets[MAX_CHUNKS];
	ChunkStruct	m_Chunks[MAX_CHUNKS]; 
	bool		m_IsMicroChunkOpen;
	size_32		m_MicroChunkOffset;
	struct {
		size_16 m_ID;
		size_16 m_Length;
	} m_MicroChunk;
public:
	ChunkSaveClass();
	void SetFile(file f);
	bool BeginChunk(size_32 ChunkID);
	bool EndChunk();
	bool BeginMicroChunk(size_16 ChunkID);
	bool EndMicroChunk();
	size_32 Write(const void *buffer, size_32 size);
};