#include "general.h"
#include "vertexbuffer.h"
#include "texture.h"
#include "DX8Wrapper.h"
#include "WW3D.h"
extern unsigned short _DynamicDeclarationVertexBufferSize;
extern DeclarationVertexBufferClass *_DynamicDeclarationVertexBuffer;
extern unsigned short _DynamicDeclarationVertexBufferOffset;
void DynamicVBAccessClass::_Reset(bool frame_changed)
{
	_DynamicSortingVertexArrayOffset = 0;
	if (frame_changed)
	{
		_DynamicDX8VertexBufferOffset = 0;
	}
}

void DynamicDeclarationAccessClass::_Reset(bool frame_changed)
{
	if (frame_changed)
	{
		_DynamicDeclarationVertexBufferOffset = 0;
	}
}

VertexBufferLockClass::VertexBufferLockClass(VertexBufferClass* buffer)
{
	VertexBuffer = buffer;
	buffer->Add_Ref();
}

VertexBufferClass::WriteLockClass::WriteLockClass(VertexBufferClass* VertexBuffer) : VertexBufferLockClass(VertexBuffer)
{
	if (!VertexBuffer->type)
	{
		((DX8VertexBufferClass *)VertexBuffer)->Get_DX8_Vertex_Buffer()->Lock(0,0,&Vertices,0);
	}
	else if (VertexBuffer->type == 1)
	{
		Vertices = ((SortingVertexBufferClass *)VertexBuffer)->VertexBuffer;
	}
}
VertexBufferClass::WriteLockClass::~WriteLockClass()
{
	if (!VertexBuffer->type)
	{
		((DX8VertexBufferClass *)VertexBuffer)->Get_DX8_Vertex_Buffer()->Unlock();
	}
	VertexBuffer->Release_Ref();
}

VertexBufferClass::AppendLockClass::AppendLockClass(VertexBufferClass* VertexBuffer,unsigned int start_index,unsigned int index_range) : VertexBufferLockClass(VertexBuffer)
{
	TT_ASSERT(VertexBuffer->type != 5);
	if (!VertexBuffer->type)
	{
		int x = ((DeclarationVertexBufferClass *)VertexBuffer)->DeclarationSize;
		((DX8VertexBufferClass *)VertexBuffer)->Get_DX8_Vertex_Buffer()->Lock(start_index*x,index_range*x,&Vertices,0);
	}
	else if (VertexBuffer->type == 1)
	{
		Vertices = &((SortingVertexBufferClass *)VertexBuffer)->VertexBuffer[start_index];
	}
}

VertexBufferClass::AppendLockClass::~AppendLockClass()
{
	if (!VertexBuffer->type)
	{
		((DX8VertexBufferClass *)VertexBuffer)->Get_DX8_Vertex_Buffer()->Unlock();
	}
	VertexBuffer->Release_Ref();
}

void DX8Wrapper::Set_Vertex_Buffer(const VertexBufferClass *vb)
{
	render_state.vba_offset = 0;
	render_state.vba_count = 0; 
	REF_PTR_SET(render_state.vertex_buffer, const_cast<VertexBufferClass*>(vb));
	if (vb) 
	{
		render_state.vertex_buffer_type = vb->Type();
	}
	else 
	{
		render_state.index_buffer_type = 4; //BUFFER_TYPE_INVALID;
	}
	render_state_changed |= VERTEX_BUFFER_CHANGED;
}

void DX8Wrapper::Set_Vertex_Buffer(const DynamicVBAccessClass& vba_)
{
	
	DynamicVBAccessClass& vba = const_cast<DynamicVBAccessClass&>(vba_);
	render_state.vertex_buffer_type = vba.Get_Type();
	render_state.vba_offset = vba.VertexBufferOffset;
	render_state.vba_count = vba.Get_Vertex_Count();
	REF_PTR_SET(render_state.vertex_buffer, vba.VertexBuffer);
	render_state_changed |= VERTEX_BUFFER_CHANGED | INDEX_BUFFER_CHANGED;		// vba_offset changes so index buffer needs to be reset as well.
};

void Set_Vertex_Buffer(DynamicDeclarationAccessClass& vba_)
{
	DX8Wrapper::render_state.vertex_buffer_type = vba_.Get_Type();
	DX8Wrapper::render_state.vba_offset = vba_.VertexBufferOffset;
	DX8Wrapper::render_state.vba_count = vba_.Get_Vertex_Count();
	if (vba_.VertexBuffer)
	{
		vba_.VertexBuffer->Add_Ref();
	}
	if (DX8Wrapper::render_state.vertex_buffer)
	{
		DX8Wrapper::render_state.vertex_buffer->Release_Ref();
	}
	DX8Wrapper::render_state.vertex_buffer = vba_.VertexBuffer;
	DX8Wrapper::render_state_changed |= VERTEX_BUFFER_CHANGED;
}

void DynamicVBAccessClass::Allocate_Sorting_Dynamic_Buffer()
{
	_DynamicSortingVertexArrayInUse = true;
	int count = VertexCount + _DynamicSortingVertexArrayOffset;
	if (count > _DynamicSortingVertexArraySize)
	{
		if (_DynamicSortingVertexArray)
		{
			_DynamicSortingVertexArray->Release_Ref();
		}
		_DynamicSortingVertexArray = 0;
		_DynamicSortingVertexArraySize = VertexCount;
		if (VertexCount < 0x1388)
		{
			_DynamicSortingVertexArraySize = 0x1388;
		}
	}
	if (!_DynamicSortingVertexArray)
	{
		_DynamicSortingVertexArray = new SortingVertexBufferClass(_DynamicSortingVertexArraySize);
		_DynamicSortingVertexArrayOffset = 0;
	}
	_DynamicSortingVertexArray->Add_Ref();
	if (VertexBuffer)
	{
		VertexBuffer->Release_Ref();
	}
	VertexBuffer = _DynamicSortingVertexArray;
	VertexBufferOffset = _DynamicSortingVertexArrayOffset;
}

void DynamicVBAccessClass::Allocate_DX8_Dynamic_Buffer()
{
	_DynamicDX8VertexBufferInUse = true;
	if (VertexCount > _DynamicDX8VertexBufferSize)
	{
		if (_DynamicDX8VertexBuffer)
		{
			_DynamicDX8VertexBuffer->Release_Ref();
		}
		_DynamicDX8VertexBuffer = 0;
		_DynamicDX8VertexBufferSize = VertexCount;
		if (VertexCount < 0x1388)
		{
			_DynamicDX8VertexBufferSize = 0x1388;
		}
	}
	if (!_DynamicDX8VertexBuffer)
	{
		unsigned int u = DX8VertexBufferClass::USAGE_DYNAMIC;
		if (ShaderCaps::NPatchesSupported)
		{
			u |= DX8VertexBufferClass::USAGE_NPATCHES;
		}
		_DynamicDX8VertexBuffer = new DX8VertexBufferClass(0x252,_DynamicDX8VertexBufferSize,(DX8VertexBufferClass::UsageType)u);
		_DynamicDX8VertexBufferOffset = 0;
	}
	unsigned short offset = _DynamicDX8VertexBufferOffset + VertexCount;
	if (offset > _DynamicDX8VertexBufferSize)
	{
		_DynamicDX8VertexBufferOffset = 0;
	}
	_DynamicDX8VertexBuffer->Add_Ref();
	if (VertexBuffer)
	{
		VertexBuffer->Release_Ref();
	}
	VertexBuffer = _DynamicDX8VertexBuffer;
	VertexBufferOffset = _DynamicDX8VertexBufferOffset;
}

void DynamicVBAccessClass::Allocate_Declaration_Dynamic_Buffer()
{
	_DynamicDX8VertexBufferInUse = true;
	if (VertexCount > _DynamicDX8VertexBufferSize)
	{
		if (_DynamicDX8VertexBuffer)
		{
			_DynamicDX8VertexBuffer->Release_Ref();
		}
		_DynamicDX8VertexBuffer = 0;
		_DynamicDX8VertexBufferSize = VertexCount;
		if (VertexCount < 0x1388)
		{
			_DynamicDX8VertexBufferSize = 0x1388;
		}
	}
	if (!_DynamicDX8VertexBuffer)
	{
		unsigned int u = DX8VertexBufferClass::USAGE_DYNAMIC;
		if (ShaderCaps::NPatchesSupported)
		{
			u |= DX8VertexBufferClass::USAGE_NPATCHES;
		}
		_DynamicDX8VertexBuffer = new DeclarationVertexBufferClass(0x252,_DynamicDX8VertexBufferSize,(DX8VertexBufferClass::UsageType)u,false);
		_DynamicDX8VertexBufferOffset = 0;
	}
	unsigned short offset = _DynamicDX8VertexBufferOffset + VertexCount;
	if (offset > _DynamicDX8VertexBufferSize)
	{
		_DynamicDX8VertexBufferOffset = 0;
	}
	_DynamicDX8VertexBuffer->Add_Ref();
	if (VertexBuffer)
	{
		VertexBuffer->Release_Ref();
	}
	VertexBuffer = _DynamicDX8VertexBuffer;
	VertexBufferOffset = _DynamicDX8VertexBufferOffset;
}

DynamicVBAccessClass::DynamicVBAccessClass(unsigned int t,unsigned int fvf,unsigned short vertex_count_)
{
	FVFInfo = &_DynamicFVFInfo;
	Type = t;
	VertexCount = vertex_count_;
	VertexBuffer = NULL;
	if (t == 2)
	{
		Allocate_Declaration_Dynamic_Buffer();
	}
	else
	{
		Allocate_Sorting_Dynamic_Buffer();
	}
}
DynamicVBAccessClass::~DynamicVBAccessClass()
{
	if (Type == 2)
	{
		_DynamicDX8VertexBufferInUse = false;
		_DynamicDX8VertexBufferOffset += VertexCount;
	}
	else
	{
		_DynamicSortingVertexArrayInUse = false;
		_DynamicSortingVertexArrayOffset += VertexCount;
	}
	VertexBuffer->Release_Ref();
}

DynamicVBAccessClass::WriteLockClass::WriteLockClass(DynamicVBAccessClass* dynamic_vb_access_)
{
	DynamicVBAccess = dynamic_vb_access_;
	if (DynamicVBAccess->Get_Type() == 2)
	{
		unsigned int lock = D3DLOCK_NOSYSLOCK;
		if (!DynamicVBAccess->VertexBufferOffset)
		{
			lock |= D3DLOCK_DISCARD;
		}
		else
		{
			lock |= D3DLOCK_NOOVERWRITE;
		}
		((DX8VertexBufferClass *)DynamicVBAccess->VertexBuffer)->Get_DX8_Vertex_Buffer()->Lock(DynamicVBAccess->VertexBufferOffset * (((DeclarationVertexBufferClass *)_DynamicDX8VertexBuffer)->DeclarationSize),(DynamicVBAccess->Get_Vertex_Count() * (((DeclarationVertexBufferClass *)(DynamicVBAccess->VertexBuffer))->DeclarationSize)),(void **)&Vertices,lock);
	}
	else if (DynamicVBAccess->Get_Type() == 3)
	{
		Vertices = ((SortingVertexBufferClass *)DynamicVBAccess->VertexBuffer)->VertexBuffer + DynamicVBAccess->VertexBufferOffset;
	}
}

void DynamicVBAccessClass::WriteLockClass::Allocate(DynamicVBAccessClass* dynamic_vb_access_)
{
	DynamicVBAccess = dynamic_vb_access_;
	if (DynamicVBAccess->Get_Type() == 2)
	{
		unsigned int lock = D3DLOCK_NOSYSLOCK;
		if (!DynamicVBAccess->VertexBufferOffset)
		{
			lock |= D3DLOCK_DISCARD;
		}
		else
		{
			lock |= D3DLOCK_NOOVERWRITE;
		}
		((DX8VertexBufferClass *)DynamicVBAccess->VertexBuffer)->Get_DX8_Vertex_Buffer()->Lock(DynamicVBAccess->VertexBufferOffset * (((DeclarationVertexBufferClass *)_DynamicDX8VertexBuffer)->DeclarationSize),(DynamicVBAccess->Get_Vertex_Count() * (((DeclarationVertexBufferClass *)(DynamicVBAccess->VertexBuffer))->DeclarationSize)),(void **)&Vertices,lock);
	}
	else if (DynamicVBAccess->Get_Type() == 3)
	{
		Vertices = ((SortingVertexBufferClass *)DynamicVBAccess->VertexBuffer)->VertexBuffer + DynamicVBAccess->VertexBufferOffset;
	}
}
DynamicVBAccessClass::WriteLockClass::~WriteLockClass()
{
	if (DynamicVBAccess->Get_Type() == 2)
	{
		((DX8VertexBufferClass *)DynamicVBAccess->VertexBuffer)->Get_DX8_Vertex_Buffer()->Unlock();
	}
}

void DynamicDeclarationAccessClass::Allocate_Sorting_Dynamic_Buffer()
{
	_DynamicSortingVertexArrayInUse = true;
	int count = VertexCount + _DynamicSortingVertexArrayOffset;
	if (count > _DynamicSortingVertexArraySize)
	{
		if (_DynamicSortingVertexArray)
		{
			_DynamicSortingVertexArray->Release_Ref();
		}
		_DynamicSortingVertexArray = 0;
		_DynamicSortingVertexArraySize = VertexCount;
		if (VertexCount < 0x1388)
		{
			_DynamicSortingVertexArraySize = 0x1388;
		}
	}
	if (!_DynamicSortingVertexArray)
	{
		_DynamicSortingVertexArray = new SortingVertexBufferClass(_DynamicSortingVertexArraySize);
		_DynamicSortingVertexArrayOffset = 0;
	}
	_DynamicSortingVertexArray->Add_Ref();
	if (VertexBuffer)
	{
		VertexBuffer->Release_Ref();
	}
	VertexBuffer = _DynamicSortingVertexArray;
	VertexBufferOffset = _DynamicSortingVertexArrayOffset;
}

void DynamicDeclarationAccessClass::Allocate_Declaration_Dynamic_Buffer()
{
	if (VertexCount > _DynamicDeclarationVertexBufferSize)
	{
		if (_DynamicDeclarationVertexBuffer)
		{
			_DynamicDeclarationVertexBuffer->Release_Ref();
		}
		_DynamicDeclarationVertexBuffer = 0;
		_DynamicDeclarationVertexBufferSize = VertexCount;
		if (VertexCount < 0x1388)
		{
			_DynamicDeclarationVertexBufferSize = 0x1388;
		}
	}
	if (!_DynamicDeclarationVertexBuffer)
	{
		unsigned int u = DX8VertexBufferClass::USAGE_DYNAMIC;
		if (ShaderCaps::NPatchesSupported)
		{
			u |= DX8VertexBufferClass::USAGE_NPATCHES;
		}
		_DynamicDeclarationVertexBuffer = new DeclarationVertexBufferClass(0x252,_DynamicDeclarationVertexBufferSize,(DX8VertexBufferClass::UsageType)u,true);
		_DynamicDeclarationVertexBufferOffset = 0;
	}
	unsigned short offset = _DynamicDeclarationVertexBufferOffset + VertexCount;
	if (offset > _DynamicDeclarationVertexBufferSize)
	{
		_DynamicDeclarationVertexBufferOffset = 0;
	}
	_DynamicDeclarationVertexBuffer->Add_Ref();
	if (VertexBuffer)
	{
		VertexBuffer->Release_Ref();
	}
	VertexBuffer = _DynamicDeclarationVertexBuffer;
	VertexBufferOffset = _DynamicDeclarationVertexBufferOffset;
}

DynamicDeclarationAccessClass::DynamicDeclarationAccessClass(unsigned int t,unsigned int fvf,unsigned short vertex_count_)
{
	FVFInfo = &_DynamicFVFInfo;
	Type = t;
	VertexCount = vertex_count_;
	VertexBuffer = 0;
	if (t == 2)
	{
		Allocate_Declaration_Dynamic_Buffer();
	}
	else
	{
		Allocate_Sorting_Dynamic_Buffer();
	}
}
DynamicDeclarationAccessClass::~DynamicDeclarationAccessClass()
{
	if (Type == 2)
	{
		_DynamicDeclarationVertexBufferOffset = _DynamicDeclarationVertexBufferOffset + (unsigned short)VertexCount;
	}
	else
	{
		_DynamicSortingVertexArrayInUse = false;
		_DynamicSortingVertexArrayOffset += VertexCount;
	}
	SafeRelease(VertexBuffer);
}

DynamicDeclarationAccessClass::WriteLockClass::WriteLockClass(DynamicDeclarationAccessClass* dynamic_Declaration_access_)
{
	DynamicDeclarationAccess = dynamic_Declaration_access_;
	if (DynamicDeclarationAccess->Get_Type() == 2)
	{
		unsigned int lock = D3DLOCK_NOSYSLOCK;
		if (!DynamicDeclarationAccess->VertexBufferOffset)
		{
			lock |= D3DLOCK_DISCARD;
		}
		else
		{
			lock |= D3DLOCK_NOOVERWRITE;
		}
		((DeclarationVertexBufferClass *)DynamicDeclarationAccess->VertexBuffer)->Get_DX8_Vertex_Buffer()->Lock(DynamicDeclarationAccess->VertexBufferOffset * (_DynamicDeclarationVertexBuffer->DeclarationSize),(DynamicDeclarationAccess->Get_Vertex_Count() * ((DeclarationVertexBufferClass *)DynamicDeclarationAccess->VertexBuffer)->DeclarationSize),(void **)&Vertices,lock);
	}
	else if (DynamicDeclarationAccess->Get_Type() == 3)
	{
		Vertices = ((SortingVertexBufferClass *)DynamicDeclarationAccess->VertexBuffer)->VertexBuffer + DynamicDeclarationAccess->VertexBufferOffset;
	}
}
DynamicDeclarationAccessClass::WriteLockClass::~WriteLockClass()
{
	if (DynamicDeclarationAccess->Get_Type() == 2)
	{
		((DeclarationVertexBufferClass *)DynamicDeclarationAccess->VertexBuffer)->Get_DX8_Vertex_Buffer()->Unlock();
	}
}

VertexBufferClass::VertexBufferClass(unsigned int type_,unsigned int FVF,unsigned short vertex_count_): RefCountClass()
{
	type = type_;
	VertexCount = vertex_count_;
	engine_refs = 0;
	fvf_info = new FVFInfoClass(FVF);
	_VertexBufferCount++;
	_VertexBufferTotalVertices += VertexCount;
	_VertexBufferTotalSize += VertexCount * fvf_info->Get_FVF_Size();
}

VertexBufferClass::VertexBufferClass(unsigned int type_,unsigned short vertex_count_): RefCountClass()
{
	type = type_;
	VertexCount = vertex_count_;
	engine_refs = 0;
	fvf_info = NULL;
	_VertexBufferCount++;
	_VertexBufferTotalVertices += VertexCount;
}

VertexBufferClass::~VertexBufferClass()
{
	_VertexBufferCount--;
	_VertexBufferTotalVertices -= VertexCount;
	if (fvf_info)
	{
		_VertexBufferTotalSize -= VertexCount * fvf_info->Get_FVF_Size();
		delete fvf_info;
	}
}

DX8VertexBufferClass::DX8VertexBufferClass(unsigned int FVF,unsigned short vertex_count_,UsageType usage) : VertexBufferClass(0,FVF,vertex_count_)
{
	VertexBuffer = 0;
	Create_Vertex_Buffer(usage);
}

DX8VertexBufferClass::DX8VertexBufferClass(unsigned short vertex_count_,UsageType usage) : VertexBufferClass(0,vertex_count_)
{
	VertexBuffer = 0;
}

DX8VertexBufferClass::~DX8VertexBufferClass()
{
	SafeRelease(VertexBuffer);
}

extern int ShaderDetail;
DeclarationVertexBufferClass::DeclarationVertexBufferClass(unsigned int FVF,unsigned short vertex_count_,UsageType usage,bool TangentsWanted) : DX8VertexBufferClass(vertex_count_,usage)
{
	location_offset = 0;
	blend_offset = 0;
	ElementCount = 0;
	this->fvf_info = NULL;
	if ((FVF & D3DFVF_XYZ) == D3DFVF_XYZ)
	{
		ElementCount++;
		blend_offset = 0x0C;
	}
	normal_offset = blend_offset;
	if (((FVF & D3DFVF_XYZB4) == D3DFVF_XYZB4) && ((FVF & D3DFVF_LASTBETA_UBYTE4) == D3DFVF_LASTBETA_UBYTE4))
	{
		ElementCount++;
		normal_offset = blend_offset + 0x10;
	}
	diffuse_offset = normal_offset;
	HasNormals = false;
	if ((FVF & D3DFVF_NORMAL) == D3DFVF_NORMAL)
	{
		ElementCount++;
		diffuse_offset = normal_offset + 0x0C;
		HasNormals = true;
	}
	specular_offset = diffuse_offset;
	HasDiffuse = false;
	if ((FVF & D3DFVF_DIFFUSE) == D3DFVF_DIFFUSE)
	{
		ElementCount++;
		specular_offset = diffuse_offset + 4;
		HasDiffuse = true;
	}
	texcoord_offset[0] = specular_offset;
	if ((FVF & D3DFVF_SPECULAR) == D3DFVF_SPECULAR)
	{
		ElementCount++;
		texcoord_offset[0] = specular_offset + 4;
	}
	texcount = (FVF / 0x100);
	HasTexCoords = false;
	if (texcount)
	{
		HasTexCoords = true;
		ElementCount++;
	}
	if (texcount > 8) 
	{
		texcount = 8;
	}
	for (unsigned int i = 1;i < texcount;i++)
	{
		ElementCount++;
		texcoord_offset[i] = texcoord_offset[i-1] + 8;
	}
	if (HasTexCoords && HasNormals && ShaderDetail && TangentsWanted)
	{
		ElementCount++;
		if (texcount)
		{
			tangent_offset = texcoord_offset[texcount-1] + 8;
		}
		else
		{
			tangent_offset = texcoord_offset[0];
		}
		ElementCount++;
		HasTangents = true;
	}
	else
	{
		ElementCount++;
		tangent_offset = 0;
		HasTangents = false;
	}
	Elements = new D3DVERTEXELEMENT9[ElementCount+1];
	int element = 0;
	if ((FVF & D3DFVF_XYZ) == D3DFVF_XYZ)
	{
		Elements[element].Stream = 0;
		Elements[element].Offset = location_offset;
		Elements[element].Type = D3DDECLTYPE_FLOAT3;
		Elements[element].Method = D3DDECLMETHOD_DEFAULT;
		Elements[element].Usage = D3DDECLUSAGE_POSITION;
		Elements[element].UsageIndex = 0;
		element++;
	}
	if ((FVF & D3DFVF_NORMAL) == D3DFVF_NORMAL)
	{
		Elements[element].Stream = 0;
		Elements[element].Offset = normal_offset;
		Elements[element].Type = D3DDECLTYPE_FLOAT3;
		Elements[element].Method = D3DDECLMETHOD_DEFAULT;
		Elements[element].Usage = D3DDECLUSAGE_NORMAL;
		Elements[element].UsageIndex = 0;
		element++;
	}
	if ((FVF & D3DFVF_DIFFUSE) == D3DFVF_DIFFUSE)
	{
		Elements[element].Stream = 0;
		Elements[element].Offset = diffuse_offset;
		Elements[element].Type = D3DDECLTYPE_D3DCOLOR;
		Elements[element].Method = D3DDECLMETHOD_DEFAULT;
		Elements[element].Usage = D3DDECLUSAGE_COLOR;
		Elements[element].UsageIndex = 0;
		element++;
	}
	if ((FVF & D3DFVF_SPECULAR) == D3DFVF_SPECULAR)
	{
		Elements[element].Stream = 0;
		Elements[element].Offset = specular_offset;
		Elements[element].Type = D3DDECLTYPE_D3DCOLOR;
		Elements[element].Method = D3DDECLMETHOD_DEFAULT;
		Elements[element].Usage = D3DDECLUSAGE_COLOR;
		Elements[element].UsageIndex = 1;
		element++;
	}
	for (unsigned char i = 0;i < texcount;i++)
	{
		Elements[element].Stream = 0;
		Elements[element].Offset = texcoord_offset[i];
		Elements[element].Type = D3DDECLTYPE_FLOAT2;
		Elements[element].Method = D3DDECLMETHOD_DEFAULT;
		Elements[element].Usage = D3DDECLUSAGE_TEXCOORD;
		Elements[element].UsageIndex = i;
		element++;
	}
	if (HasTangents)
	{
		Elements[element].Stream = 0;
		Elements[element].Offset = tangent_offset;
		Elements[element].Type = D3DDECLTYPE_FLOAT3;
		Elements[element].Method = D3DDECLMETHOD_DEFAULT;
		Elements[element].Usage = D3DDECLUSAGE_TANGENT;
		Elements[element].UsageIndex = 0;
		element++;
	}
	Elements[element].Stream = 0xFF;
	Elements[element].Offset = 0;
	Elements[element].Type = D3DDECLTYPE_UNUSED;
	Elements[element].Method = 0;
	Elements[element].Usage = 0;
	Elements[element].UsageIndex = 0;
	DeclarationSize = D3DXGetDeclVertexSize(Elements,0);
	_VertexBufferTotalSize += VertexCount * DeclarationSize;
	DX8Wrapper::D3DDevice->CreateVertexDeclaration(Elements,&VertexDecl);
	delete[] Elements;
	Create_Declaration_Vertex_Buffer(usage);
}

DeclarationVertexBufferClass::~DeclarationVertexBufferClass()
{
	_VertexBufferTotalSize -= VertexCount * DeclarationSize;
	SafeRelease(VertexDecl);
}

void DeclarationVertexBufferClass::Create_Declaration_Vertex_Buffer(UsageType Usage)
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
	unsigned int length = VertexCount * DeclarationSize;
	unsigned int pool = ((~Usage) & 1);
	HRESULT res = DX8Wrapper::D3DDevice->CreateVertexBuffer(length,usage,0,(D3DPOOL)pool,&VertexBuffer,NULL);
 	if (FAILED(res))
	{
		TextureClass::Invalidate_Old_Unused_Textures(0x1388);
		WW3D::_Invalidate_Mesh_Cache();
		res = DX8Wrapper::D3DDevice->CreateVertexBuffer(length,usage,0,(D3DPOOL)pool,&VertexBuffer,NULL);
 		TT_ASSERT(SUCCEEDED(res));
	}
}

void DX8VertexBufferClass::Create_Vertex_Buffer(UsageType Usage)
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
	unsigned int length = VertexCount * fvf_info->Get_FVF_Size();
	unsigned int pool = ((~Usage) & 1);
	if (FAILED(DX8Wrapper::D3DDevice->CreateVertexBuffer(length,usage,fvf_info->Get_FVF(),(D3DPOOL)pool,&VertexBuffer,NULL)))
	{
		TextureClass::Invalidate_Old_Unused_Textures(0x1388);
		WW3D::_Invalidate_Mesh_Cache();
		DX8Wrapper::D3DDevice->CreateVertexBuffer(length,usage,fvf_info->Get_FVF(),(D3DPOOL)pool,&VertexBuffer,NULL);
	}
}

SortingVertexBufferClass::SortingVertexBufferClass(unsigned short VertexCount) : VertexBufferClass(1,0x252,VertexCount)
{
	VertexBuffer = new VertexFormatXYZNDUV2[VertexCount];
}

SortingVertexBufferClass::~SortingVertexBufferClass()
{
	delete[] VertexBuffer;
}

DX8VertexBufferClass __declspec(dllexport) *VertexBufferAllocate(unsigned short vertex_count_)
{
	return new DeclarationVertexBufferClass(0x152,vertex_count_,DX8VertexBufferClass::USAGE_DEFAULT,false);
}

void DynamicVBAccessClass::_Deinit()
{
	if (_DynamicDX8VertexBuffer)
	{
		_DynamicDX8VertexBuffer->Release_Ref();
	}
	_DynamicDX8VertexBuffer = 0;
	_DynamicDX8VertexBufferInUse = false;
	_DynamicDX8VertexBufferSize = 0x1388;
	_DynamicDX8VertexBufferOffset = 0;
	if (_DynamicSortingVertexArray)
	{
		_DynamicSortingVertexArray->Release_Ref();
	}
	_DynamicSortingVertexArray = 0;
	_DynamicSortingVertexArrayInUse = false;
	_DynamicSortingVertexArraySize = 0;
	_DynamicSortingVertexArrayOffset = 0;
}

inline unsigned int GetDeclTypeSize(BYTE type)
{
	TT_ASSUME(type < 5);
	switch(type)
	{
		case D3DDECLTYPE_FLOAT1: return sizeof(float);
		case D3DDECLTYPE_FLOAT2: return sizeof(Vector2);
		case D3DDECLTYPE_FLOAT3: return sizeof(Vector3);
		case D3DDECLTYPE_FLOAT4: return sizeof(Vector4);
		case D3DDECLTYPE_D3DCOLOR: return sizeof(int);
		DEFAULT_UNREACHABLE;
	};
};

StreamVertexBufferClass::WriteLockClass::WriteLockClass(StreamVertexBufferClass* VertexBuffer, unsigned int ElementUsage, unsigned int ElementUsageIndex): VertexBufferLockClass(VertexBuffer)
{
	unsigned int stream_index = VertexBuffer->GetStreamIndexForElement(ElementUsage, ElementUsageIndex); 
	TT_ASSERT(stream_index < 9);
	this->Stream = VertexBuffer->PeekStream(stream_index);
	TT_ASSERT(Stream != NULL);
	Stream->Lock(0, 0, &this->Vertices, D3DLOCK_NOSYSLOCK);
};

StreamVertexBufferClass::WriteLockClass::~WriteLockClass()
{
	Stream->Unlock();
	VertexBuffer->Release();
};

StreamVertexBufferClass::AppendLockClass::AppendLockClass(StreamVertexBufferClass* VertexBuffer, unsigned int ElementUsage, unsigned int ElementUsageIndex, unsigned int StartIndex, unsigned int IndexRange): VertexBufferLockClass(VertexBuffer)
{
	unsigned int stream_index = VertexBuffer->GetStreamIndexForElement(ElementUsage, ElementUsageIndex); 
	TT_ASSERT(stream_index < 9);
	this->Stream = VertexBuffer->PeekStream(stream_index);
	TT_ASSERT(this->Stream != NULL);
	unsigned int stride = VertexBuffer->GetStreamStride(stream_index);
	Stream->Lock(StartIndex * stride, IndexRange * stride, &this->Vertices, D3DLOCK_NOSYSLOCK);
};

StreamVertexBufferClass::AppendLockClass::~AppendLockClass()
{
	Stream->Unlock();
	VertexBuffer->Release();
};	

StreamVertexBufferClass::StreamVertexBufferClass(unsigned int FVF, unsigned short vertex_count, DX8VertexBufferClass::UsageType usage, bool enable_tangents): VertexBufferClass(StreamVertexBufferClass::TYPE, FVF, vertex_count)
{
	this->fvf_info = NULL;

  	this->HasNormals =		false;
	this->HasTexCoord[0] =	false;
  	this->HasTexCoord[1] =	false;
	this->HasDiffuse =		false;
	this->HasSpecular =		false;
	this->HasTangents =		false;

	memset(this->Streams, 0x00, sizeof(this->Streams));
	memset(this->StreamStrides, 0x00, sizeof(this->StreamStrides));
	memset(this->StreamUsageIndexMapping, 0xFF, sizeof(this->StreamUsageIndexMapping));

	D3DVERTEXELEMENT9 elements[MAX_STREAM_COUNT + 1] = {0};

	unsigned int current_stream = 0;
	unsigned int current_element = 0;

	if (FVF & D3DFVF_XYZ)
	{
		D3DVERTEXELEMENT9& element = elements[current_element++];
		element.Stream =		(WORD)current_stream++;
		element.Offset =		0;
		element.Type =			D3DDECLTYPE_FLOAT3;
		element.Method =		D3DDECLMETHOD_DEFAULT;
		element.Usage =			D3DDECLUSAGE_POSITION;
		element.UsageIndex =	0;
	}

	if (FVF & D3DFVF_NORMAL)
	{
		D3DVERTEXELEMENT9& element = elements[current_element++];
		element.Stream =		(WORD)current_stream++;
		element.Offset =		0;
		element.Type =			D3DDECLTYPE_FLOAT3;
		element.Method =		D3DDECLMETHOD_DEFAULT;
		element.Usage =			D3DDECLUSAGE_NORMAL;
		element.UsageIndex =	0;
		this->HasNormals =		true;
	};

	if (FVF & D3DFVF_DIFFUSE)
	{
		D3DVERTEXELEMENT9& element = elements[current_element++];
		element.Stream =		(WORD)current_stream++;
		element.Offset =		0;
		element.Type =			D3DDECLTYPE_D3DCOLOR;
		element.Method =		D3DDECLMETHOD_DEFAULT;
		element.Usage =			D3DDECLUSAGE_COLOR;
		element.UsageIndex =	0;
		this->HasDiffuse =		true;
	};
	
	if (FVF & D3DFVF_SPECULAR)
	{
		D3DVERTEXELEMENT9& element = elements[current_element++];
		element.Stream =		(WORD)current_stream++;
		element.Offset =		0;
		element.Type =			D3DDECLTYPE_D3DCOLOR;
		element.Method =		D3DDECLMETHOD_DEFAULT;
		element.Usage =			D3DDECLUSAGE_COLOR;
		element.UsageIndex =	1;
		this->HasSpecular =		true;
	};

	unsigned int texcount = (FVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
	TT_ASSUME(texcount <= 2);
	for (unsigned int i = 0; i < texcount; ++i)
	{
		D3DVERTEXELEMENT9& element = elements[current_element++];
		element.Stream =		(WORD)current_stream++;
		element.Offset =		0;
		element.Type =			D3DDECLTYPE_FLOAT2;
		element.Method =		D3DDECLMETHOD_DEFAULT;
		element.Usage =			D3DDECLUSAGE_TEXCOORD;
		element.UsageIndex =	(BYTE)i;
		this->HasTexCoord[i] =	true;
	}

	if (enable_tangents)
	{
		D3DVERTEXELEMENT9& element = elements[current_element++];
		element.Stream =		(WORD)current_stream++;
		element.Offset =		0;
		element.Type =			D3DDECLTYPE_FLOAT3;
		element.Method =		D3DDECLMETHOD_DEFAULT;
		element.Usage =			D3DDECLUSAGE_TANGENT;
		element.UsageIndex =	0;
		this->HasTangents =		true;
	};

	{	//ie D3DDECL_END
	  	D3DVERTEXELEMENT9& element = elements[current_element++];
		element.Stream =		0xFF;
		element.Offset =		0;
		element.Type =			D3DDECLTYPE_UNUSED;
		element.Method =		0;
		element.Usage =			0;
		element.UsageIndex =	0;
	};

	DX8Wrapper::D3DDevice->CreateVertexDeclaration(elements, &this->VertexDecl);

	D3DPOOL d3dpool = D3DPOOL((~usage) & 1); // no idea why this is done this way 
	unsigned int d3dusage = D3DUSAGE_WRITEONLY;

	switch(usage)
	{
	case DX8VertexBufferClass::USAGE_DYNAMIC:				d3dusage |= D3DUSAGE_DYNAMIC; break; 
	case DX8VertexBufferClass::USAGE_SOFTWAREPROCESSING:	d3dusage |= D3DUSAGE_SOFTWAREPROCESSING; break;
	case DX8VertexBufferClass::USAGE_NPATCHES:				d3dusage |= D3DUSAGE_NPATCHES; break;
	DEFAULT_UNREACHABLE;
	}

	for (unsigned int i = 0; i < current_stream; ++i)
	{
		// StreamUsageIndexMapping is used as a lookup table to quickly locate the stream associated with
		// a certain type of vertex element. it's filled out here to avoid extra code complexity in other places
		D3DVERTEXELEMENT9& element = elements[i];
		this->StreamUsageIndexMapping[element.Usage][element.UsageIndex] = i;
	

		unsigned int stride = GetDeclTypeSize(element.Type);
		unsigned int length = stride * vertex_count;
		this->StreamStrides[i] = stride;

		HRESULT res = DX8Wrapper::D3DDevice->CreateVertexBuffer(length, usage, 0, d3dpool, &this->Streams[i], NULL);
		if (FAILED(res)) // uhoh, we ran out of video memory
		{
			TextureClass::Invalidate_Old_Unused_Textures(5000); // so invalidate textures last used more than 5000 ms ago
			WW3D::_Invalidate_Mesh_Cache();
			// FIXME following needs "LOG_D3D_ERROR" macro 
			res = DX8Wrapper::D3DDevice->CreateVertexBuffer(length, usage, 0, d3dpool, &this->Streams[i], NULL);
			TT_ASSERT(SUCCEEDED(res));
		};
	};

};

StreamVertexBufferClass::~StreamVertexBufferClass()
{
	SafeRelease(this->VertexDecl);
	for (unsigned int i = 0; i < MAX_STREAM_COUNT; ++i)
	{
		SafeRelease(this->Streams[i]);
	};
};

void StreamVertexBufferClass::Apply()
{
	for (unsigned int i = 0; i < MAX_STREAM_COUNT; ++i)
	{
		DX8Wrapper::D3DDevice->SetStreamSource(i, this->Streams[i], 0, this->StreamStrides[i]); 
	};
	DX8Wrapper::D3DDevice->SetVertexDeclaration(this->VertexDecl);
};

