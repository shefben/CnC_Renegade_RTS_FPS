#include "general.h"
#include "indexbuffer.h"
#include "engine_3d.h"
#include "texture.h"
#include "DX8Wrapper.h"
#include "WW3D.h"

void DynamicIBAccessClass::_Reset(bool frame_changed)
{
	_DynamicSortingIndexArrayOffset = 0;
	if (frame_changed)
	{
		_DynamicDX8IndexBufferOffset = 0;
	}
}

void DX8Wrapper::Set_Index_Buffer(const IndexBufferClass* ib,unsigned short index_base_offset)
{
	render_state.iba_offset = 0;
	REF_PTR_SET(render_state.index_buffer,const_cast<IndexBufferClass*>(ib));
	render_state.index_base_offset = index_base_offset;
	if (ib) 
	{
		render_state.index_buffer_type = ib->Type();
	}
	else 
	{
		render_state.index_buffer_type = 4; //BUFFER_TYPE_INVALID;
	}
	render_state_changed |= INDEX_BUFFER_CHANGED;
}

void DX8Wrapper::Set_Index_Buffer(const DynamicIBAccessClass& iba_,unsigned short index_base_offset)
{
	DynamicIBAccessClass& iba = const_cast<DynamicIBAccessClass&>(iba_);
	render_state.index_base_offset = index_base_offset;
	render_state.index_buffer_type = iba.Get_Type();
	render_state.iba_offset = iba.IndexBufferOffset;
	REF_PTR_SET(render_state.index_buffer, iba.IndexBuffer);
	render_state.index_buffer->Add_Engine_Ref();
	render_state_changed |= INDEX_BUFFER_CHANGED;
}

void DynamicIBAccessClass::Allocate_Sorting_Dynamic_Buffer()
{
	_DynamicSortingIndexArrayInUse = true;
	int count = IndexCount + _DynamicSortingIndexArrayOffset;
	if (count > _DynamicSortingIndexArraySize)
	{
		if (_DynamicSortingIndexArray)
		{
			_DynamicSortingIndexArray->Release_Ref();
		}
		_DynamicSortingIndexArray = 0;
		_DynamicSortingIndexArraySize = IndexCount;
		if (IndexCount < 0x1388)
		{
			_DynamicSortingIndexArraySize = 0x1388;
		}
	}
	if (!_DynamicSortingIndexArray)
	{
		_DynamicSortingIndexArray = new SortingIndexBufferClass(_DynamicSortingIndexArraySize);
		_DynamicSortingIndexArrayOffset = 0;
	}
	_DynamicSortingIndexArray->Add_Ref();
	if (IndexBuffer)
	{
		IndexBuffer->Release_Ref();
	}
	IndexBuffer = _DynamicSortingIndexArray;
	IndexBufferOffset = _DynamicSortingIndexArrayOffset;
}

void DynamicIBAccessClass::Allocate_DX8_Dynamic_Buffer()
{
	_DynamicDX8IndexBufferInUse = true;
	if (IndexCount > _DynamicDX8IndexBufferSize)
	{
		if (_DynamicDX8IndexBuffer)
		{
			_DynamicDX8IndexBuffer->Release_Ref();
		}
		_DynamicDX8IndexBuffer = 0;
		_DynamicDX8IndexBufferSize = IndexCount;
		if (IndexCount < 0x1388)
		{
			_DynamicDX8IndexBufferSize = 0x1388;
		}
	}
	if (!_DynamicDX8IndexBuffer)
	{
		unsigned int u = DX8IndexBufferClass::USAGE_DYNAMIC;
		if (ShaderCaps::NPatchesSupported)
		{
			u |= DX8IndexBufferClass::USAGE_NPATCHES;
		}
		_DynamicDX8IndexBuffer = new DX8IndexBufferClass(_DynamicDX8IndexBufferSize,(DX8IndexBufferClass::UsageType)u);
		_DynamicDX8IndexBufferOffset = 0;
	}
	unsigned short offset = _DynamicDX8IndexBufferOffset + IndexCount;
	if (offset > _DynamicDX8IndexBufferSize)
	{
		_DynamicDX8IndexBufferOffset = 0;
	}
	_DynamicDX8IndexBuffer->Add_Ref();
	if (IndexBuffer)
	{
		IndexBuffer->Release_Ref();
	}
	IndexBuffer = _DynamicDX8IndexBuffer;
	IndexBufferOffset = _DynamicDX8IndexBufferOffset;
}

DynamicIBAccessClass::DynamicIBAccessClass(unsigned short type_,unsigned short index_count_)
{
	Type = type_;
	IndexCount = index_count_;
	IndexBuffer = 0;
	if (type_ == 2)
	{
		Allocate_DX8_Dynamic_Buffer();
	}
	else
	{
		Allocate_Sorting_Dynamic_Buffer();
	}
}

DynamicIBAccessClass::~DynamicIBAccessClass()
{
	if (IndexBuffer)
	{
		IndexBuffer->Release_Ref();
		IndexBuffer = 0;
	}
	if (Type == 2)
	{
		_DynamicDX8IndexBufferInUse = false;
		_DynamicDX8IndexBufferOffset += IndexCount;
	}
	else
	{
		_DynamicSortingIndexArrayInUse = false;
		_DynamicSortingIndexArrayOffset += IndexCount;
	}
}



IndexBufferClass::AppendLockClass::AppendLockClass(IndexBufferClass* _indexBuffer, uint startIndex, uint indexRange) :
	indexBuffer(Create_Peek(_indexBuffer))
{
	if (indexBuffer->type == 0)
		((DX8IndexBufferClass*)indexBuffer.Peek())->Get_DX8_Index_Buffer()->Lock(startIndex*2, indexRange*2, (void**)&indices, 0);
	else if (indexBuffer->type == 1)
		indices = &((SortingIndexBufferClass*)indexBuffer.Peek())->index_buffer[startIndex];
}



IndexBufferClass::AppendLockClass::~AppendLockClass()
{
	if (indexBuffer->type == 0)
		((DX8IndexBufferClass*)indexBuffer.Peek())->Get_DX8_Index_Buffer()->Unlock();
}



IndexBufferClass::~IndexBufferClass()
{
	_IndexBufferCount--;
	_IndexBufferTotalIndices -= index_count;
	_IndexBufferTotalSize -= index_count * 2;
}

IndexBufferClass::IndexBufferClass(unsigned int type_,unsigned short index_count_)
{
	engine_refs = 0;
	index_count = index_count_;
	type = type_;
	_IndexBufferCount++;
	_IndexBufferTotalIndices += index_count;
	_IndexBufferTotalSize += index_count * 2;
}

DX8IndexBufferClass::DX8IndexBufferClass(unsigned short index_count_,UsageType Usage) : IndexBufferClass(0,index_count_)
{
	unsigned int usage = D3DUSAGE_WRITEONLY;
	if (Usage == USAGE_DYNAMIC)
	{
		usage |= D3DUSAGE_DYNAMIC;
	}
	if (Usage == USAGE_SOFTWAREPROCESSING)
	{
		usage |= D3DUSAGE_SOFTWAREPROCESSING;
	}
	if (Usage == USAGE_NPATCHES)
	{
		usage |= D3DUSAGE_NPATCHES;
	}
	unsigned int pool = ((~Usage) & 1);
	if (FAILED(DX8Wrapper::D3DDevice->CreateIndexBuffer(index_count * 2,usage,D3DFMT_INDEX16,(D3DPOOL)pool,&index_buffer,NULL)))
	{
		TextureClass::Invalidate_Old_Unused_Textures(0x1388);
		WW3D::_Invalidate_Mesh_Cache();
		DX8Wrapper::D3DDevice->CreateIndexBuffer(index_count * 2,usage,D3DFMT_INDEX16,(D3DPOOL)pool,&index_buffer,NULL);
	}
}

DX8IndexBufferClass::~DX8IndexBufferClass()
{
	SafeRelease(index_buffer);
}

SortingIndexBufferClass::SortingIndexBufferClass(unsigned short index_count_) : IndexBufferClass(1,index_count_)
{
	index_buffer = new unsigned short[index_count];
}

DynamicIBAccessClass::WriteLockClass::WriteLockClass(DynamicIBAccessClass* ib_access_)
{
	DynamicIBAccess = ib_access_;
	ib_access_->IndexBuffer->Add_Ref();
	if (DynamicIBAccess->Get_Type() == 2)
	{
		unsigned int lock = D3DLOCK_NOSYSLOCK;
		if (!DynamicIBAccess->IndexBufferOffset)
		{
			lock |= D3DLOCK_DISCARD;
		}
		else
		{
			lock |= D3DLOCK_NOOVERWRITE;
		}
		((DX8IndexBufferClass *)DynamicIBAccess->IndexBuffer)->Get_DX8_Index_Buffer()->Lock(DynamicIBAccess->IndexBufferOffset*2,DynamicIBAccess->IndexCount*2,(void **)&Indices,lock);
	}
	else if (DynamicIBAccess->Get_Type() == 3)
	{
		Indices = ((SortingIndexBufferClass *)DynamicIBAccess->IndexBuffer)->index_buffer + DynamicIBAccess->IndexBufferOffset;
	}
}

SortingIndexBufferClass::~SortingIndexBufferClass()
{
	delete[] index_buffer;
}

DynamicIBAccessClass::WriteLockClass::~WriteLockClass()
{
	if (DynamicIBAccess->Get_Type() == 2)
	{
		((DX8IndexBufferClass *)DynamicIBAccess->IndexBuffer)->Get_DX8_Index_Buffer()->Unlock();
	}
	DynamicIBAccess->IndexBuffer->Release_Ref();
}

void DynamicIBAccessClass::_Deinit()
{
	if (_DynamicDX8IndexBuffer)
	{
		_DynamicDX8IndexBuffer->Release_Ref();
	}
	_DynamicDX8IndexBuffer = 0;
	_DynamicDX8IndexBufferInUse = false;
	_DynamicDX8IndexBufferSize = 0x1388;
	_DynamicDX8IndexBufferOffset = 0;
	if (_DynamicSortingIndexArray)
	{
		_DynamicSortingIndexArray->Release_Ref();
	}
	_DynamicSortingIndexArray = 0;
	_DynamicSortingIndexArrayInUse = false;
	_DynamicSortingIndexArraySize = 0;
	_DynamicSortingIndexArrayOffset = 0;
}

IndexBufferClass::WriteLockClass::WriteLockClass(IndexBufferClass *index_buffer_)
{
	index_buffer = index_buffer_;
	index_buffer_->Add_Ref();
	if (index_buffer->Type())
	{
		if (index_buffer->Type() == 1)
		{
			indices = ((SortingIndexBufferClass *)index_buffer)->index_buffer;
		}
	}
	else
	{
		((DX8IndexBufferClass *)index_buffer)->Get_DX8_Index_Buffer()->Lock(0,index_buffer->Get_Index_Count() * 2,(void **)&indices,0);
	}
}

IndexBufferClass::WriteLockClass::~WriteLockClass()
{
	if (!index_buffer->Type())
	{
		((DX8IndexBufferClass *)index_buffer)->Get_DX8_Index_Buffer()->Unlock();
	}
	index_buffer->Release_Ref();
}
