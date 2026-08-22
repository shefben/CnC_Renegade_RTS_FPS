#ifndef TT_INCLUDE__VERTEXBUFFER_H
#define TT_INCLUDE__VERTEXBUFFER_H



#include "engine_3d.h"



class VertexBufferClass;



class VertexBufferLockClass {
protected:
	VertexBufferClass* VertexBuffer;
	void* Vertices;
public:
	VertexBufferLockClass(VertexBufferClass* buffer);
	void* Get_Vertex_Array()
	{
		return Vertices;
	}
};

class VertexBufferClass : public RefCountClass {
protected:
	unsigned int type;
	unsigned short VertexCount;
	int engine_refs;
public:
	FVFInfoClass* fvf_info;
	class WriteLockClass : public VertexBufferLockClass {
	public:
		WriteLockClass(VertexBufferClass* VertexBuffer);
		~WriteLockClass();
	};
	class AppendLockClass : public VertexBufferLockClass {
	public:
		AppendLockClass(VertexBufferClass* VertexBuffer,unsigned int start_index,unsigned int index_range);
		~AppendLockClass();
	};
	~VertexBufferClass();
	void Add_Engine_Ref()
	{
		engine_refs++;
	}
	void Release_Engine_Ref()
	{
		engine_refs--;
	}
	FVFInfoClass& FVF_Info()
	{
		return *fvf_info;
	}
	unsigned short Get_Vertex_Count()
	{
		return VertexCount;
	}
	unsigned int Type() const
	{
		return type;
	}
	unsigned int Engine_Refs()
	{
		return engine_refs;
	}
	VertexBufferClass(unsigned int type_,unsigned int FVF,unsigned short vertex_count_);
	VertexBufferClass(unsigned int type_,unsigned short vertex_count_);
};

class DX8VertexBufferClass : public VertexBufferClass {
protected:
	IDirect3DVertexBuffer9* VertexBuffer;
public:
	enum UsageType {
		USAGE_DEFAULT = 0x0,
		USAGE_DYNAMIC = 0x1,
		USAGE_SOFTWAREPROCESSING = 0x2,
		USAGE_NPATCHES = 0x4,
	};
	IDirect3DVertexBuffer9* Get_DX8_Vertex_Buffer()
	{
		return VertexBuffer;
	}
	~DX8VertexBufferClass();
	DX8VertexBufferClass(unsigned int FVF,unsigned short vertex_count_,UsageType usage);
	DX8VertexBufferClass(unsigned short vertex_count_,UsageType usage);
	void Create_Vertex_Buffer(UsageType Usage);
};

class DeclarationVertexBufferClass : public DX8VertexBufferClass {
public:
	IDirect3DVertexDeclaration9 *VertexDecl;
	D3DVERTEXELEMENT9 *Elements;
	unsigned int ElementCount;
	unsigned int DeclarationSize;
	unsigned short location_offset;
	unsigned short normal_offset;
	unsigned short blend_offset;
	unsigned short texcoord_offset[8];
	unsigned short diffuse_offset;
	unsigned short specular_offset;
	unsigned short tangent_offset;
	bool HasNormals;
	bool HasTexCoords;
	bool HasDiffuse;
	bool HasTangents;
	unsigned int texcount;
	DeclarationVertexBufferClass(unsigned int FVF,unsigned short vertex_count_,UsageType usage,bool TangentsWanted);
	~DeclarationVertexBufferClass();
	void Create_Declaration_Vertex_Buffer(UsageType Usage);
};

struct VertexFormatXYZNDUV2 {
	float x;
	float y;
	float z;
	float nx;
	float ny;
	float nz;
	unsigned int diffuse;
	float u1;
	float v1;
	float u2;
	float v2;
};

struct VertexFormatXYZNDUV2Extra {
	float x;
	float y;
	float z;
	float nx;
	float ny;
	float nz;
	unsigned int diffuse;
	float u1;
	float v1;
	float u2;
	float v2;
	Vector3 Tangent;
};



class DynamicVBAccessClass
{

public:

	class WriteLockClass
	{
		DynamicVBAccessClass* DynamicVBAccess;
		VertexFormatXYZNDUV2* Vertices;

	public:

		WriteLockClass(DynamicVBAccessClass* dynamic_vb_access_);
		~WriteLockClass();
		void SHADERS_API Allocate(DynamicVBAccessClass* dynamic_vb_access_);


		VertexFormatXYZNDUV2 *Get_Formatted_Vertex_Array()
		{
			return Vertices;
		}
	};

private:

	FVFInfoClass *FVFInfo; // 0000
	unsigned int Type; // 0004
	unsigned short VertexCount; // 0008

public:

	unsigned short VertexBufferOffset; // 000A
	VertexBufferClass* VertexBuffer; // 000C


	DynamicVBAccessClass(unsigned int t,unsigned int fvf,unsigned short vertex_count_);
	~DynamicVBAccessClass();
	void Allocate_Sorting_Dynamic_Buffer();
	void Allocate_DX8_Dynamic_Buffer();
	void SHADERS_API Allocate_Declaration_Dynamic_Buffer();
	static void _Reset(bool frame_changed);
	static void _Deinit();


	FVFInfoClass& FVF_Info()
	{
		return *FVFInfo;
	}

	unsigned int Get_Type()
	{
		return Type;
	}

	unsigned short Get_Vertex_Count()
	{
		return VertexCount;
	}

}; // 0010



class DynamicDeclarationAccessClass {
private:
	FVFInfoClass *FVFInfo;
	unsigned int Type;
	unsigned short VertexCount;
public:
	class WriteLockClass {
		DynamicDeclarationAccessClass* DynamicDeclarationAccess;
		VertexFormatXYZNDUV2* Vertices;
	public:
		WriteLockClass(DynamicDeclarationAccessClass* dynamic_declaration_access_);
		~WriteLockClass();
		VertexFormatXYZNDUV2 *Get_Formatted_Vertex_Array()
		{
			return Vertices;
		}
		VertexFormatXYZNDUV2Extra *Get_Formatted_Declaration_Vertex_Array()
		{
			return (VertexFormatXYZNDUV2Extra *)Vertices;
		}
	};
	unsigned short VertexBufferOffset;
	VertexBufferClass* VertexBuffer;
	void Allocate_Sorting_Dynamic_Buffer();
	void Allocate_Declaration_Dynamic_Buffer();
	DynamicDeclarationAccessClass(unsigned int t,unsigned int fvf,unsigned short vertex_count_);
	~DynamicDeclarationAccessClass();
	static void _Reset(bool frame_changed);
	FVFInfoClass& FVF_Info()
	{
		return *FVFInfo;
	}
	unsigned int Get_Type()
	{
		return Type;
	}
	unsigned short Get_Vertex_Count()
	{
		return VertexCount;
	}
};

class SortingVertexBufferClass : public VertexBufferClass {
public:
	struct VertexFormatXYZNDUV2* VertexBuffer;
	~SortingVertexBufferClass();
	SortingVertexBufferClass(unsigned short VertexCount);
};

class StreamVertexBufferClass: public VertexBufferClass
{
protected:
	enum
	{
		MAX_STREAM_COUNT =	8,
		MAX_USAGE =			13,
		MAX_USAGE_INDEX =	2,
	};

	IDirect3DVertexBuffer9*		Streams[MAX_STREAM_COUNT];
	unsigned int				StreamStrides[MAX_STREAM_COUNT];
	unsigned int				StreamUsageIndexMapping[MAX_USAGE][MAX_USAGE_INDEX];

	IDirect3DVertexDeclaration9* VertexDecl;
public:
	enum
	{
		TYPE = 5,
	};

 	class WriteLockClass: public VertexBufferLockClass 
	{
	protected:
		IDirect3DVertexBuffer9* Stream;
	public:
		WriteLockClass(StreamVertexBufferClass* VertexBuffer, unsigned int ElementUsage, unsigned int ElementUsageIndex);
		~WriteLockClass();
	};

	class AppendLockClass: public VertexBufferLockClass 
	{
	protected:
		IDirect3DVertexBuffer9* Stream;
	public:
		AppendLockClass(StreamVertexBufferClass* VertexBuffer, unsigned int ElementUsage, unsigned int ElementUsageIndex, unsigned int StartIndex, unsigned int IndexRange);
		~AppendLockClass();
	};

	bool HasNormals;
	bool HasTexCoord[2];
	bool HasDiffuse;
	bool HasSpecular;
	bool HasTangents;

	StreamVertexBufferClass(unsigned int FVF, unsigned short vertex_count, DX8VertexBufferClass::UsageType usage, bool enable_tangents);
	~StreamVertexBufferClass();
	void Apply();

	TT_INLINE IDirect3DVertexBuffer9* PeekStream(unsigned int Index)
	{
		return Streams[Index];	
	};

	TT_INLINE unsigned int GetStreamStride(unsigned int Index)
	{
		return StreamStrides[Index];
	};

	TT_INLINE unsigned int GetStreamIndexForElement(unsigned int ElementUsage, unsigned int ElementUsageIndex)
	{
		return StreamUsageIndexMapping[ElementUsage][ElementUsageIndex];
	};

};

void Set_Vertex_Buffer(DynamicDeclarationAccessClass& vba_); //TODO



#endif
