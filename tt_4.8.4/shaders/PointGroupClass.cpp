#include "General.h"
#include "PointGroupClass.h"



#include "engine_3d.h"
#include "texture.h"
#include "vertexbuffer.h"
#include "indexbuffer.h"
#include "SortingRenderer.h"
#include "VectorProcessorClass.h"
#include "DX8Wrapper.h"
#include "WW3D.h"

REF_DEF1(Quads, IndexBufferClass*, 0x0083AB50);
REF_DEF1(SortingQuads, IndexBufferClass*, 0x00837B28);
REF_DEF1(Tris, IndexBufferClass*, 0x0083CFB0);
REF_DEF1(SortingTris, IndexBufferClass*, 0x0083AB54);
REF_DEF1(VertexDiffuse, Vector4*, 0x0083CF6C);
REF_DEF1(VertexLoc, Vector3*, 0x0083CF7C);
REF_DEF1(VertexUV, Vector2*, 0x0083AB44);



REF_DEF1(PointGroupClass::PointMaterial, VertexMaterialClass*, 0x0083D048);

RENEGADE_FUNCTION
void PointGroupClass::Update_Arrays(Vector3* , Vector4* , float* , uint8* , uint8* , int , int , int& , int&)
AT1(0x005AB4B0);



void PointGroupClass::Set_Arrays(ShareBufferClass<Vector3>* a2, ShareBufferClass<Vector4>* a3, ShareBufferClass<uint>* a4, ShareBufferClass<float>* a5, ShareBufferClass<unsigned char>* a6, ShareBufferClass<unsigned char>* a7, int a8, float a9, float a10, float a11, float a12)
{
	REF_PTR_SET(PointLoc, a2);
	REF_PTR_SET(PointDiffuse, a3);
	REF_PTR_SET(APT, a4);
	REF_PTR_SET(PointSize, a5);
	REF_PTR_SET(PointOrientation, a6);
	REF_PTR_SET(PointFrame, a7);

	if (APT || a8 >= 0)
		PointCount = a8;
	else
		PointCount = PointLoc->Get_Count();
	
	VPXMin = a9;
	VPYMin = a10;
	VPXMax = a11;
	VPYMax = a12;
}


RENEGADE_FUNCTION
void PointGroupClass::_Shutdown()
AT1(0x005ACEC0);

RENEGADE_FUNCTION
void PointGroupClass::_Init()
AT1(0x005AC840);

PointGroupClass::PointGroupClass() : PointLoc(0), PointDiffuse(0), APT(0), PointSize(0), PointOrientation(0), PointFrame(0), PointCount(0), FrameRowColumnCountLog2(0), Texture(0), PointMode(TRIS), Flags(0), DefaultPointSize(0), DefaultPointColor(1,1,1), DefaultPointAlpha(1), DefaultPointOrientation(0), DefaultPointFrame(0), VPXMin(0), VPYMin(0), VPXMax(0), VPYMax(0), Shader(ShaderClass::_PresetAdditiveSpriteShader)
{
}
PointGroupClass::~PointGroupClass()
{
	REF_PTR_RELEASE(PointLoc);
	REF_PTR_RELEASE(PointDiffuse);
	REF_PTR_RELEASE(APT);
	REF_PTR_RELEASE(PointSize);
	REF_PTR_RELEASE(PointOrientation);
	REF_PTR_RELEASE(PointFrame);
	REF_PTR_RELEASE(Texture);
}
int PointGroupClass::Get_Flag(FlagsType flagsType)
{
	return (Flags >> flagsType) & 1;
}

void PointGroupClass::Set_Flag(FlagsType flag,bool onoff)
{
	if (onoff)
	{
		Flags |= 1 << flag;
	}
	else
	{
		Flags &= ~(1 << flag);
	}
}
void PointGroupClass::Set_Texture(TextureClass* texture)
{
	REF_PTR_SET(Texture,texture);
}
void PointGroupClass::Set_Shader(ShaderClass shader)
{
	Shader = shader;
}
void PointGroupClass::Set_Point_Mode(PointModeEnum mode)
{
	PointMode = mode;
}
void PointGroupClass::Set_Frame_Row_Column_Count_Log2(unsigned char frccl2)
{
	if (frccl2 > 4)
	{
		FrameRowColumnCountLog2 = 4;
	}
	else
	{
		FrameRowColumnCountLog2 = frccl2;
	}
}
void PointGroupClass::Render(RenderInfoClass& renderInfo)
{
	Shader.Set_Cull_Mode(ShaderClass::CULL_MODE_DISABLE);
	
	if (PointCount == 0)
		return;

	static VectorClass<Vector3> compressed_loc;
	static VectorClass<Vector4> compressed_diffuse;
	static VectorClass<float> compressed_size;
	static VectorClass<uint8> compressed_orient;
	static VectorClass<uint8> compressed_frame;
	static VectorClass<Vector3> transformed_loc;
	
	Vector3* unk_loc;
	Vector4* unk_diffuse = 0;
	float* unk_size = 0;
	uint8* unk_orient = 0;
	uint8* unk_frame = 0;
	
	bool isBrightWhite = DefaultPointColor.X > 0.9961f && DefaultPointColor.Y > 0.9961f && DefaultPointColor.Z > 0.9961f && DefaultPointAlpha > 0.9961f; // 254/255?

	Shader.Set_Texturing(ShaderClass::TEXTURING_DISABLE);
	Shader.Set_Primary_Gradient(ShaderClass::GRADIENT_DISABLE);
	
	if (PointDiffuse || !isBrightWhite || !Texture)
		Shader.Set_Primary_Gradient(ShaderClass::GRADIENT_MODULATE); // part of MASK_PRIGRADIENT

	if (Texture)
		Shader.Set_Texturing(ShaderClass::TEXTURING_ENABLE);
	
	if (APT)
	{
		if (compressed_loc.Length() < PointCount)
			compressed_loc.Resize(PointCount * 2, NULL);
		
		VectorProcessorClass::CopyIndexed(&compressed_loc[0], PointLoc->Get_Array(), APT->Get_Array(), PointCount);
		
		unk_loc = &compressed_loc[0];

		if (PointDiffuse)
		{
			if (compressed_diffuse.Length() < PointCount)
				compressed_diffuse.Resize(PointCount * 2, NULL);
			
			VectorProcessorClass::CopyIndexed(&compressed_diffuse[0], PointDiffuse->Get_Array(), APT->Get_Array(), PointCount);
			unk_diffuse = &compressed_diffuse[0];
		}
		
		if (PointSize)
		{
			if (compressed_size.Length() < PointCount)
				compressed_size.Resize(PointCount * 2, NULL);
			
			VectorProcessorClass::CopyIndexed(&compressed_size[0], PointSize->Get_Array(), APT->Get_Array(), PointCount);
			unk_size = &compressed_size[0];
		}
		
		if (PointOrientation)
		{
			if (compressed_orient.Length() < PointCount)
				compressed_orient.Resize(PointCount * 2, NULL);
			
			VectorProcessorClass::CopyIndexed(&compressed_orient[0], PointOrientation->Get_Array(), APT->Get_Array(), PointCount);
			unk_orient = &compressed_orient[0];
		}
		
		if (PointFrame)
		{
			if (compressed_frame.Length() < PointCount)
				compressed_frame.Resize(PointCount * 2, NULL);
			
			VectorProcessorClass::CopyIndexed(&compressed_frame[0], PointFrame->Get_Array(), APT->Get_Array(), PointCount);
			unk_frame = &compressed_frame[0];
		}
	}
	else
	{
		unk_loc = PointLoc->Get_Array();
		
		if (PointDiffuse)
			unk_diffuse = PointDiffuse->Get_Array();
		
		if (PointSize)
			unk_size = PointSize->Get_Array();
		
		if (PointOrientation)
			unk_orient = PointOrientation->Get_Array();
		
		if (PointFrame)
			unk_frame = PointFrame->Get_Array();
	}
	
	Matrix4 transform;
	DX8Wrapper::Get_Transform(D3DTS_VIEW, transform);

	if (Get_Flag(TRANSFORM))
	{
		if (transformed_loc.Length() < PointCount)
			transformed_loc.Resize(PointCount * 2, NULL);
		
		for (int i = 0; i < PointCount; i++)
		{
			Vector4 v = transform * unk_loc[i];
			transformed_loc[i] = Vector3(v.X,v.Y,v.Z);
		}
		
		unk_loc = &transformed_loc[0];
	}
	
	int vertexCount;
	int polygonCount;
	Update_Arrays(unk_loc, unk_diffuse, unk_size, unk_orient, unk_frame, PointCount, PointLoc->Get_Count(), vertexCount, polygonCount);
	
	DX8Wrapper::Set_Transform(D3DTS_WORLD, Matrix4::IDENTITY);
	DX8Wrapper::Set_Transform(D3DTS_VIEW, Matrix4::IDENTITY);
	
	if (PointMaterial != DX8Wrapper::render_state.material)
	{
		if (PointMaterial)
			PointMaterial->Add_Ref();
		
		SafeRelease(DX8Wrapper::render_state.material);
		
		DX8Wrapper::render_state.material = PointMaterial;
		DX8Wrapper::render_state_changed |= MATERIAL_CHANGED;
	}
	
	if (ShaderClass::ShaderDirty || Shader != DX8Wrapper::render_state.shader)
	{
		DX8Wrapper::render_state.shader = Shader;
		DX8Wrapper::render_state_changed |= SHADER_CHANGED;
	}
	
	// This seems to be some inlined function.
	if (Texture != DX8Wrapper::render_state.Textures[0])
	{
		if (Texture)
			Texture->Add_Ref();
		
		SafeRelease(DX8Wrapper::render_state.Textures[0]);
		
		DX8Wrapper::render_state.Textures[0] = Texture;
		DX8Wrapper::render_state_changed |= TEXTURE0_CHANGED;
	}
	
	bool isSorted = Shader.Get_Bits() & ShaderClass::MASK_DSTBLEND && !(Shader.Get_Bits() & ShaderClass::MASK_ALPHATEST) && WW3D::IsSortingEnabled;
	
	uint polygonVertexCount;
	IndexBufferClass* indexBuffer;
	if (PointMode == QUADS)
	{
		polygonVertexCount = 2;
		
		if (isSorted)
			indexBuffer = SortingQuads;
		else
			indexBuffer = Quads;
	}
	else
	{
		polygonVertexCount = 3;
		
		if (isSorted)
			indexBuffer = SortingTris;
		else
			indexBuffer = Tris;
	}
	
	for (int vertexIndex = 0; vertexIndex < vertexCount;)
	{
		int vbVertexCount = min(vertexCount - vertexIndex, 1200);
		
		DynamicVBAccessClass vertexBuffer(isSorted ? 3 : 2, D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX2, (uint16)vbVertexCount);
		
		{
			DynamicVBAccessClass::WriteLockClass vertexBufferLock(&vertexBuffer);
			
			uint8* vertices = (uint8*)vertexBufferLock.Get_Formatted_Vertex_Array();
			FVFInfoClass& fvfInfo = vertexBuffer.FVF_Info();
			
			for (int maxVertexIndex = vertexIndex + vbVertexCount; vertexIndex < maxVertexIndex; vertexIndex++)
			{
				*(Vector3*)(vertices + fvfInfo.Get_Location_Offset()) = VertexLoc[vertexIndex];
				
				Vector4 diffuseColor;
				if (unk_diffuse)
				{
					diffuseColor = VertexDiffuse[vertexIndex];
				}
				else
				{
					diffuseColor.X = DefaultPointColor.X;
					diffuseColor.Y = DefaultPointColor.Y;
					diffuseColor.Z = DefaultPointColor.Z;
					diffuseColor.W = DefaultPointAlpha;
				}
				DX8Wrapper::Clamp_Color(diffuseColor);
				*(D3DCOLOR*)(vertices + fvfInfo.Get_Diffuse_Offset()) = GetD3dColor(diffuseColor);
				
				*(Vector2*)(vertices + fvfInfo.Get_Tex_Offset(0)) = VertexUV[vertexIndex];
				vertices += fvfInfo.Get_FVF_Size();
			}
		}
		
		DX8Wrapper::Set_Index_Buffer(indexBuffer, 0);
		DX8Wrapper::Set_Vertex_Buffer(vertexBuffer);
		
		if (isSorted)
			SortingRendererClass::Insert_Triangles(0, (uint16)(vbVertexCount / polygonVertexCount), 0, (uint16)vbVertexCount);
		else
			DX8Wrapper::Draw_Triangles(0, (uint16)(vbVertexCount / polygonVertexCount), 0, (uint16)vbVertexCount);

		DX8Wrapper::Set_Transform(D3DTS_VIEW, transform);
	}
}
