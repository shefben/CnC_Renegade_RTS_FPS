#ifndef TT_INCLUDE__STRAW_H
#define TT_INCLUDE__STRAW_H

class Straw {
private:
	Straw* ChainTo;
	Straw* ChainFrom;
public:
	Straw();
	virtual ~Straw();
	virtual void Get_From(Straw* straw);
	virtual int Get(void* source,int slen);
};
class Buffer {
private:
	void* BufferPtr;
	long Size;
	bool IsAllocated;
public:
	Buffer(void* buffer,long size);
	void *Get_Buffer()
	{
		return BufferPtr;
	}
	long Get_Size()
	{
		return Size;
	}
	~Buffer();
};

class BufferStraw : public Straw  {
private:
	Buffer BufferPtr;
	int Index;
public:
	BufferStraw(void* buffer, int size);
	~BufferStraw();
	int Get(void* source,int slen);
};

class FileClass;
class FileStraw : public Straw {
private:
	FileClass* File;
	bool HasOpened;
public:
	FileStraw(class FileClass&);
	~FileStraw();
	int Get(void* source,int slen);
};

#endif;