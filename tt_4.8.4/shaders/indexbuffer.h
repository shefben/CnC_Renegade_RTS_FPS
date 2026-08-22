#ifndef TT_INCLUDE__INDEXBUFFER_H
#define TT_INCLUDE__INDEXBUFFER_H



#include "engine_vector.h"
#include "RefCountPtr.h"



class IndexBufferClass : public RefCountClass {
public:
	class WriteLockClass {
		IndexBufferClass* index_buffer;
		unsigned short* indices;
	public:
		WriteLockClass(IndexBufferClass* index_buffer_);
		~WriteLockClass();
		unsigned short *Get_Index_Array()
		{
			return indices;
		}
	};
	class AppendLockClass {
		RefCountPtr<IndexBufferClass> indexBuffer;
		unsigned short* indices;
	public:
		AppendLockClass(IndexBufferClass* index_buffer_,unsigned int start_index,unsigned int index_range);
		~AppendLockClass();
		unsigned short *Get_Index_Array()
		{
			return indices;
		}
	};
protected:
	int engine_refs;
	unsigned short index_count;
	unsigned int type;
public:
	virtual ~IndexBufferClass();
	IndexBufferClass(unsigned int type_,unsigned short index_count_);
	void Copy(unsigned short* indices,unsigned int first_index,unsigned int count);
	void Copy(unsigned int* indices,unsigned int first_index,unsigned int count);
	unsigned short Get_Index_Count()
	{
		return index_count;
	}
	unsigned int Type() const
	{
		return type;
	}
	void Add_Engine_Ref()
	{
		engine_refs++;
	}
	void Release_Engine_Ref()
	{
		engine_refs--;
	}
	unsigned int Engine_Refs()
	{
		return engine_refs;
	}
};

class DynamicIBAccessClass {
private:
	unsigned int Type;
	unsigned short IndexCount;
public:
	class WriteLockClass {
		DynamicIBAccessClass* DynamicIBAccess;
		unsigned short* Indices;
	public:
		WriteLockClass(DynamicIBAccessClass* ib_access_);
		~WriteLockClass();
		unsigned short *Get_Index_Array()
		{
			return Indices;
		}
	};
	unsigned short IndexBufferOffset;
	IndexBufferClass* IndexBuffer;
	void Allocate_Sorting_Dynamic_Buffer();
	void Allocate_DX8_Dynamic_Buffer();
	DynamicIBAccessClass(unsigned short type_,unsigned short index_count_);
	~DynamicIBAccessClass();
	static void _Reset(bool frame_changed);
	static void _Deinit();
	unsigned int Get_Type()
	{
		return Type;
	}
	unsigned short Get_Index_Count()
	{
		return IndexCount;
	}
};

class DX8IndexBufferClass : public IndexBufferClass {
private:
	IDirect3DIndexBuffer9* index_buffer;
public:
	enum UsageType {
		USAGE_DEFAULT = 0x0,
		USAGE_DYNAMIC = 0x1,
		USAGE_SOFTWAREPROCESSING = 0x2,
		USAGE_NPATCHES = 0x4,
	};
	~DX8IndexBufferClass();
	DX8IndexBufferClass(unsigned short index_count_,UsageType usage);
	void Copy(unsigned short* indices,unsigned int first_index,unsigned int count);
	void Copy(unsigned int* indices,unsigned int first_index,unsigned int count);
	IDirect3DIndexBuffer9 *Get_DX8_Index_Buffer()
	{
		return index_buffer;
	}
};

class SortingIndexBufferClass : public IndexBufferClass {
public:
	unsigned short* index_buffer;
	SortingIndexBufferClass(unsigned short index_count_);
	~SortingIndexBufferClass();
};

#endif