#include "General.h"
#include "Line3DClass.h"

#include "DX8Wrapper.h"
#include "vertexbuffer.h"
#include "indexbuffer.h"



void Line3DClass::Render(RenderInfoClass& renderInfo)
{
	static const uint16 indices[36] =
	{
		3, 5, 1,
		7, 5, 3,
		1, 5, 0,
		5, 4, 0,
		4, 2, 0,
		4, 6, 2,
		7, 3, 2,
		6, 7, 2,
		7, 6, 5,
		5, 6, 4,
		2, 3, 1,
		2, 1, 0,
	};

	if (!Is_Not_Hidden_At_All())
		return;

	if (ShaderClass::ShaderDirty || shader != DX8Wrapper::render_state.shader)
	{
		DX8Wrapper::render_state.shader = shader;
		DX8Wrapper::render_state_changed |= SHADER_CHANGED;
	}
	
	if (DX8Wrapper::render_state.Textures[0])
	{
		DX8Wrapper::render_state.Textures[0]->Release_Ref();
		DX8Wrapper::render_state.Textures[0] = 0;
		DX8Wrapper::render_state_changed |= TEXTURE0_CHANGED;
	}
	
	VertexMaterialClass* material = VertexMaterialClass::Get_Preset(VertexMaterialClass::PRELIT_DIFFUSE);
	if (material != DX8Wrapper::render_state.material)
	{
		DX8Wrapper::render_state.material->Release_Ref();
		DX8Wrapper::render_state.material = material;
		DX8Wrapper::render_state_changed |= MATERIAL_CHANGED;
	} else if (material)
		material->Release_Ref();
	
	
	DX8Wrapper::Set_Transform(D3DTS_WORLD, Matrix4(Transform));
	
	DynamicVBAccessClass vertexBuffer(2, 0x252, 8);
	{
		D3DCOLOR d3dColor = GetD3dColor(color, opacity);
		
		DynamicVBAccessClass::WriteLockClass vertexBufferLock(&vertexBuffer);
		VertexFormatXYZNDUV2* _vertices = vertexBufferLock.Get_Formatted_Vertex_Array();
		
		for (int i = 0; i < 8; ++i)
		{
			_vertices[i].x = vertices[i].X;
			_vertices[i].y = vertices[i].Y;
			_vertices[i].z = vertices[i].Z;
			_vertices[i].diffuse = d3dColor;
		}
	}
	
	DynamicIBAccessClass indexBuffer(2, 36);
	{
		DynamicIBAccessClass::WriteLockClass indexBufferLock(&indexBuffer);
		uint16* _indices = indexBufferLock.Get_Index_Array();
		for (int i = 0; i < 36; ++i)
			_indices[i] = indices[i];
	}
	
	DX8Wrapper::Set_Vertex_Buffer(vertexBuffer);
	DX8Wrapper::Set_Index_Buffer(indexBuffer, 0);
	DX8Wrapper::Draw_Triangles(0, 12, 0, 8);
}



void Line3DClass::Get_Obj_Space_Bounding_Sphere(SphereClass& sphere) const
{
	float halfLength = length * .5f;
	sphere = SphereClass(Vector3(halfLength, 0, 0), halfLength);
}



void Line3DClass::Get_Obj_Space_Bounding_Box(AABoxClass& box) const
{
	float halfLength = length * .5f;
	Vector3 point(halfLength, 0, 0);
	box = AABoxClass(point, point);
}




void Line3DClass::Scale(float scale)
{
	for (int i = 0; i < 8; ++i)
		vertices[i] *= scale;
	
	length *= scale;
	thickness *= scale;
	
	Bits &= ~RenderObjClass::BOUNDING_VOLUMES_VALID;
	RenderObjClass* container = Get_Container();
	if (container)
		container->Update_Obj_Space_Bounding_Volumes();
}



void Line3DClass::Scale(float scaleX, float scaleY, float scaleZ)
{
	for (int i = 0; i < 8; ++i)
	{
		vertices[i].X *= scaleX;
		vertices[i].Y *= scaleY;
		vertices[i].Z *= scaleZ;
	}
	
	length *= scaleX;
	thickness *= scaleY;
	
	Bits &= ~RenderObjClass::BOUNDING_VOLUMES_VALID;
	RenderObjClass* container = Get_Container();
	if (container)
		container->Update_Obj_Space_Bounding_Volumes();
}



Line3DClass::Line3DClass(const Vector3& from, const Vector3& to, float _thickness, float colorR, float colorG, float colorB, float _opacity)
{
	color = Vector3(colorR, colorG, colorB);
	Set_Opacity(_opacity);
	
	length = (to - from).Length();
	thickness = _thickness;

	float halfThickness = thickness * .5f;
	vertices[0] = Vector3(0,      -halfThickness, -halfThickness);
	vertices[1] = Vector3(0,       halfThickness, -halfThickness);
	vertices[2] = Vector3(0,      -halfThickness,  halfThickness);
	vertices[3] = Vector3(0,       halfThickness,  halfThickness);
	vertices[4] = Vector3(length, -halfThickness, -halfThickness);
	vertices[5] = Vector3(length,  halfThickness, -halfThickness);
	vertices[6] = Vector3(length, -halfThickness,  halfThickness);
	vertices[7] = Vector3(length,  halfThickness,  halfThickness);
	
	Matrix3D transform;
	transform.Obj_Look_At(from, to);
	Set_Transform(transform);
}



void Line3DClass::Re_Color(float r, float g, float b)
{
	color = Vector3(r, g, b);
}



void Line3DClass::Set_Opacity(float _opacity)
{
	if (_opacity >= 1.f)
		shader = ShaderClass::_PresetOpaqueSolidShader;
	else
		shader = ShaderClass::_PresetAlphaSolidShader;
	
	opacity = _opacity;
}
