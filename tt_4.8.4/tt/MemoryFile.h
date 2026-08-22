#ifndef TT_INCLUDE__MEMORYFILE_H
#define TT_INCLUDE__MEMORYFILE_H



#include "engine_io.h"
#include "engine_math.h"



class MemoryFile :
	public FileClass
{

public:

	byte* data;
	int dataOffset;
	int dataSize;
	int dataCapacity;

	MemoryFile()
	{
		data = NULL;
		dataOffset = 0;
		dataSize = 0;
	}

	void reserve(int _dataCapacity)
	{
		byte* _data = new byte[_dataCapacity];
		memcpy(_data, data, dataSize);
		delete[] data;
		data = _data;
		dataCapacity = _dataCapacity;
	}

	void resize(int _dataSize)
	{
		if (_dataSize > dataCapacity)
			reserve(_dataSize * 2);
		dataSize = _dataSize;
	}

	virtual ~MemoryFile() {}
	virtual const char* File_Name() { TT_UNREACHABLE; }
	virtual const char* Set_Name(const char* name) { TT_UNREACHABLE; }
	virtual bool Create() { TT_UNREACHABLE; }
	virtual bool Delete() { TT_UNREACHABLE; }
	virtual bool Is_Available(int handle = 0) { return true; }
	virtual bool Is_Open() { return true; }
	virtual int Open(const char* name, int mode = 1) { TT_UNIMPLEMENTED; }
	virtual int Open(int mode = 1) { TT_UNIMPLEMENTED; }
	
	virtual int Read(void* buffer, int size)
	{
		size = min(size, dataSize - dataOffset);
		memcpy(buffer, data + dataOffset, size);
		dataOffset += size;
		return size;
	}

	virtual int Seek(int offset, int origin)
	{
		if (origin == SEEK_SET)
			dataOffset = offset;
		else if (origin == SEEK_CUR)
			dataOffset += offset;
		else if (origin == SEEK_END)
			dataOffset = dataSize + offset;
		else
			TT_UNREACHABLE;
		
		dataOffset = clamp(dataOffset, 0, dataSize);
		return dataOffset;
	}
	
	virtual int Size() { return dataSize; }
	
	virtual int Write(void* buffer, int size)
	{
		int _dataOffset = dataOffset + size;

		if (_dataOffset > dataSize)
			resize(_dataOffset);

		memcpy(data + dataOffset, buffer, size);
		dataOffset = _dataOffset;

		return size;
	}

	virtual void Close() { TT_UNIMPLEMENTED; }
	virtual void Error(int a, int b, const char* c) { TT_UNIMPLEMENTED; }
	virtual void Bias(int start, int length) { TT_UNIMPLEMENTED; }

};



#endif