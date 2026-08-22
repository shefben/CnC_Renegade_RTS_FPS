#include "General.h"
#include "dx8fvf.h"

#include "MeshClass.h"
#include "vertexbuffer.h"
#include "indexbuffer.h"
#include "shader.h"
#include "DX8Wrapper.h"
#include "DX8PolygonRendererClass.h"
#include "DX8TextureCategoryClass.h"
#include "MaterialPassClass.h"
#include "WW3D.h"
#include "Textures_Material_And_Shader_Booking_Struct.h"



extern int ShaderDetail;
extern EnlargeableBufferClass<Vector3> TempTangentBuffer;
EnlargeableBufferClass<Vector2> FakeUVSource;
extern REF_DECL1(fvf_category_container_delete_list, MultiListClass<DX8FVFCategoryContainer>);



Vertex_Split_Table::Vertex_Split_Table(MeshClass* Mesh)
{
	mc = Mesh;
	mmc = Mesh->Peek_Model();
	npatch_enable = false;
	allocated_polygon_array = false;
	if ((ShaderCaps::NPatchesSupported) && (mmc->Needs_Vertex_Normals()) && (mmc->Flags & MeshGeometryClass::ALLOW_NPATCHES))
	{
		npatch_enable = true;
	}
	polygon_count = mmc->PolyCount;
	if (mmc->GapFiller)
	{
		polygon_count += mmc->GapFiller->PolygonCount;
	}
	allocated_polygon_array = true;
	polygon_array = new Vector3i16[polygon_count];
	memcpy(polygon_array,mmc->Poly->Get_Array(),mmc->PolyCount*sizeof(Vector3i16));
	if (mmc->GapFiller)
	{
		memcpy(polygon_array+mmc->PolyCount,mmc->GapFiller->PolygonArray,mmc->GapFiller->PolygonCount*sizeof(Vector3i16));
	}
}



MatPassTaskClass::MatPassTaskClass(MaterialPassClass* pass,MeshClass* mesh)
{
	MaterialPass = pass;
	Mesh = mesh;
	NextVisible = 0;
	pass->Add_Ref();
	mesh->Add_Ref();
}



MatPassTaskClass::~MatPassTaskClass()
{
	MaterialPass->Release_Ref();
	Mesh->Release_Ref();
}



inline void CalculateTangentArray(long vertexCount, const Vector3 *vertex, const Vector3 *normal, 
							const Vector2 *vertex_UV, long triangleCount, const Vector3i16 *triangle, 
							Vector3* tangentArray)
{
	ZeroMemory(tangentArray, vertexCount * sizeof(Vector3));
	for (long f = 0; f < triangleCount; ++f)
	{
		long i0 = triangle[f].I;
		long i1 = triangle[f].J;
		long i2 = triangle[f].K;

		Vector3 side_0 = vertex[i1] - vertex[i0];
		Vector3 side_1 = vertex[i2] - vertex[i0];

		float delta_U_0 = vertex_UV[i1].U - vertex_UV[i0].U;
		float delta_U_1 = vertex_UV[i2].U - vertex_UV[i0].U;
		float delta_V_0 = vertex_UV[i1].V - vertex_UV[i0].V;
		float delta_V_1 = vertex_UV[i2].V - vertex_UV[i0].V;

        float r = 1.0F / (delta_U_0 * delta_V_1 - delta_U_1 * delta_V_0);

		Vector3 tangent(	(delta_V_1 * side_0.X - delta_V_0 * side_1.X) * r,
							(delta_V_1 * side_0.Y - delta_V_0 * side_1.Y) * r,
							(delta_V_1 * side_0.Z - delta_V_0 * side_1.Z) * r);
		
		/*
		// calculate the bitangent
		Vector3 bitangent(	(delta_U_0 * side_1.X - delta_U_1 * side_0.X) * r,
								(delta_U_0 * side_1.Y - delta_U_1 * side_0.Y) * r,
								(delta_U_0 * side_1.Z - delta_U_1 * side_0.Z) * r);
		*/


		tangentArray[i0] += tangent;
		tangentArray[i1] += tangent;
		tangentArray[i2] += tangent;
	};
	
	for (long v = 0; v < vertexCount; ++v)
	{
		const Vector3& n = normal[v];
		Vector3 t = tangentArray[v];
 
		// Gram-Schmidt orthogonalize
		t -= n * t * n;
		tangentArray[v] = Normalize(t);
	};
}



void DX8RigidFVFCategoryContainer::Add_Delayed_Visible_Material_Pass(MaterialPassClass* pass,MeshClass* mesh)
{
	MatPassTaskClass *m = new MatPassTaskClass(pass,mesh);
	if (delayed_visible_matpass_head)
	{
		delayed_visible_matpass_tail->Set_Next_Visible(m);
	}
	else
	{
		delayed_visible_matpass_head = m;
	}
	delayed_visible_matpass_tail = m;
	DelayedAnythingToRender = true;
}



void DX8RigidFVFCategoryContainer::Render_Delayed_Procedural_Material_Passes()
{
	if (DelayedAnythingToRender)
	{
		DelayedAnythingToRender = false;
		DX8Wrapper::Set_Vertex_Buffer(vertex_buffer);
		DX8Wrapper::Set_Index_Buffer(index_buffer.Peek(),0);
		for (MatPassTaskClass* node = delayed_visible_matpass_head, *nextNode; node; node = nextNode)
		{
			nextNode = node->Get_Next_Visible();
			node->Peek_Mesh()->Render_Material_Pass(node->Peek_Material_Pass(), index_buffer.Peek());
			delete node;
		}
		delayed_visible_matpass_head = NULL;
		delayed_visible_matpass_tail = NULL;
	}
}



void DX8RigidFVFCategoryContainer::Add_Mesh(MeshClass *mesh)
{
	Vertex_Split_Table split = Vertex_Split_Table(mesh);	
	if (!this->vertex_buffer)
	{
		unsigned short vertexcount = (unsigned short) split.mmc->VertexCount > 4000 ? (unsigned short) split.mmc->VertexCount : 4000;
		if (this->sorting)
		{
			this->vertex_buffer = new SortingVertexBufferClass(vertexcount); 
		}
		else
		{
			/*
			// disabled because "everything drawn via shader" isn't done yet
			if (ShaderCaps::MaxStreams >= 8)
			{
				// yay, we get to use the new system
				this->vertex_buffer = new StreamVertexBufferClass(this->FVF, vertexcount, DX8VertexBufferClass::USAGE_DEFAULT, ShaderDetail != 0);
			}
			else
			*/
			{
				this->vertex_buffer = new DeclarationVertexBufferClass(this->FVF, vertexcount, DX8VertexBufferClass::USAGE_DEFAULT, true);
			};
		};
	}; 

	if (this->vertex_buffer->Type() == StreamVertexBufferClass::TYPE)
	{
		StreamVertexBufferClass* vb = (StreamVertexBufferClass*)this->vertex_buffer;

		unsigned int vertex_count = split.mmc->VertexCount;

		Vector3* position_array = split.mmc->Vertex->Get_Array();
		Vector3* normal_array = split.mmc->VertexNorm->Get_Array();
		unsigned int* diffuse_array = split.mc->Get_User_Lighting_Array(false);
		if (!diffuse_array) diffuse_array = split.mmc->CurMatDesc->Get_Color_Array(0, false);
		unsigned int* specular_array = split.mmc->CurMatDesc->Get_Color_Array(1, false);
		Vector2* uv0_array = split.mmc->CurMatDesc->Get_UV_Array_By_Index(0, false);
		Vector2* uv1_array = split.mmc->CurMatDesc->Get_UV_Array_By_Index(1, false);

		{
			StreamVertexBufferClass::AppendLockClass position_lock = StreamVertexBufferClass::AppendLockClass(vb, D3DDECLUSAGE_POSITION, 0, this->used_vertices, vertex_count);
			memcpy(position_lock.Get_Vertex_Array(), position_array, sizeof(Vector3) * vertex_count);
		}

		if (vb->HasNormals && normal_array)
		{
			StreamVertexBufferClass::AppendLockClass normal_lock = StreamVertexBufferClass::AppendLockClass(vb, D3DDECLUSAGE_NORMAL, 0, this->used_vertices, vertex_count);
			memcpy(normal_lock.Get_Vertex_Array(), normal_array, sizeof(Vector3) * vertex_count);	
		}

		if (vb->HasDiffuse && diffuse_array)
		{
			StreamVertexBufferClass::AppendLockClass diffuse_lock = StreamVertexBufferClass::AppendLockClass(vb, D3DDECLUSAGE_COLOR, 0, this->used_vertices, vertex_count);
			memcpy(diffuse_lock.Get_Vertex_Array(), diffuse_array, sizeof(unsigned int) * vertex_count);	
		}

		if (vb->HasSpecular && specular_array)
		{
			StreamVertexBufferClass::AppendLockClass specular_lock = StreamVertexBufferClass::AppendLockClass(vb, D3DDECLUSAGE_COLOR, 1, this->used_vertices, vertex_count);
			memcpy(specular_lock.Get_Vertex_Array(), specular_array, sizeof(unsigned int) * vertex_count);	
		}

		if(vb->HasTexCoord[0] && uv0_array)
		{
			StreamVertexBufferClass::AppendLockClass uv0_lock = StreamVertexBufferClass::AppendLockClass(vb, D3DDECLUSAGE_TEXCOORD, 0, this->used_vertices, vertex_count);
			memcpy(uv0_lock.Get_Vertex_Array(), uv0_array, sizeof(unsigned int) * vertex_count);	
		}

		if(vb->HasTexCoord[1] && uv1_array)
		{
			StreamVertexBufferClass::AppendLockClass uv1_lock = StreamVertexBufferClass::AppendLockClass(vb, D3DDECLUSAGE_TEXCOORD, 1, this->used_vertices, vertex_count);
			memcpy(uv1_lock.Get_Vertex_Array(), uv1_array, sizeof(unsigned int) * vertex_count);	
		}	

		bool tangents_needed = false;

		if (CalculateTangents && ShaderDetail && vb->HasTangents)
		{
			if (split.mmc->CurMatDesc->Material[0] && split.mmc->CurMatDesc->Material[0]->Mapper[0])
			{
				ProgrammableShader *shader = split.mmc->CurMatDesc->Material[0]->Mapper[0]->Peek_Shader();
				if (shader)
				{
					if (shader->Validate() == true)
					{
						if (shader->NeedsTangents())
						{
							tangents_needed = true;
						};
					};
				};
			}
			else if (split.mmc->CurMatDesc->Texture[0][0] && split.mmc->CurMatDesc->Texture[0][0]->Name)
			{
		 		ProgrammableShader *shader = ShaderController.PeekShader(split.mmc->CurMatDesc->Texture[0][0]->Name);
				if (shader)
				{
					if (shader->Validate() == true)
					{
						if (shader->NeedsTangents())
						{
							tangents_needed = true;
						};
					};
				}; 
			};
		};

		Vector3* tangent_array = NULL;

		if (tangents_needed)
		{
			if (uv0_array)
			{
				TempTangentBuffer.Enlarge(vertex_count);
				CalculateTangentArray(vertex_count, position_array, normal_array, uv0_array, split.polygon_count, split.polygon_array, TempTangentBuffer.GetBuffer());
				tangent_array = TempTangentBuffer.GetBuffer();
			};
		};

		if(vb->HasTangents && tangent_array)
		{
			StreamVertexBufferClass::AppendLockClass tangent_lock = StreamVertexBufferClass::AppendLockClass(vb, D3DDECLUSAGE_TANGENT, 0, this->used_vertices, vertex_count);
			memcpy(tangent_lock.Get_Vertex_Array(), tangent_array, sizeof(Vector3) * vertex_count);	
		};
	}
	else
	{
		//FIXME FIXME FIXME
		VertexBufferClass::AppendLockClass Lock = VertexBufferClass::AppendLockClass(this->vertex_buffer,this->used_vertices,split.mmc->VertexCount);
		char *vbarray = (char *)Lock.Get_Vertex_Array();
		Vector3 *varray = split.mmc->Vertex->Get_Array();
		Vector3 *narray = split.mmc->Get_Vertex_Normal_Array();
		unsigned int *ularray;
		if (split.mc->Get_User_Lighting_Array(false))
		{
			ularray = split.mc->Get_User_Lighting_Array(false);
		}
		else
		{
			ularray = split.mmc->CurMatDesc->Get_Color_Array(0, false);
		}
		unsigned int *carray = split.mmc->CurMatDesc->Get_Color_Array(1,false);
		unsigned int texcount = (this->FVF / 0x100);
		bool hasnormals = false;
		bool hasdiffuse = false;
		bool hasspecular = false;
		if (this->FVF & D3DFVF_NORMAL)
		{
			hasnormals = true;
		}
		if (this->FVF & D3DFVF_DIFFUSE)
		{
			hasdiffuse = true;
		}
		if (this->FVF & D3DFVF_SPECULAR)
		{
			hasspecular = true;
		}
		int locationoffset;
		int normaloffset;
		int diffuseoffset;
		int specularoffset;
		int tangentoffset;
		int texoffset[8];
		int fvfsize;
		if (this->vertex_buffer->fvf_info)
		{
			FVFInfoClass fvfd = this->vertex_buffer->FVF_Info();
			locationoffset = fvfd.Get_Location_Offset();
			normaloffset = fvfd.Get_Normal_Offset();
			diffuseoffset = fvfd.Get_Diffuse_Offset();
			specularoffset = fvfd.Get_Specular_Offset();
			tangentoffset = 0;
			for (unsigned int t = 0;t < 8;t++)
			{
				texoffset[t] = fvfd.Get_Tex_Offset(t);
			}
			fvfsize = fvfd.Get_FVF_Size();
		}
		else
		{
			DeclarationVertexBufferClass *d = (DeclarationVertexBufferClass *)this->vertex_buffer;
			locationoffset = d->location_offset;
			normaloffset = d->normal_offset;
			diffuseoffset = d->diffuse_offset;
			specularoffset = d->specular_offset;
			tangentoffset = 0;
			if ((CalculateTangents) && (ShaderDetail))
			{
				if ((split.mmc->CurMatDesc->Material[0]) && (split.mmc->CurMatDesc->Material[0]->Mapper[0]))
				{
					ProgrammableShader *shader = split.mmc->CurMatDesc->Material[0]->Mapper[0]->Peek_Shader();
					if (shader)
					{
						if (shader->Validate() == true)
						{
							if (shader->NeedsTangents())
							{
								tangentoffset = d->tangent_offset;
							}
						}
					}
				}
				if ((split.mmc->CurMatDesc->Texture[0][0]) && (split.mmc->CurMatDesc->Texture[0][0]->Name))
				{
					ProgrammableShader *shader = (ProgrammableShader*) ShaderController.PeekShader(split.mmc->CurMatDesc->Texture[0][0]->Name);
					if (shader)
					{
						if (shader->Validate() == true)
						{
							if (shader->NeedsTangents())
							{
								tangentoffset = d->tangent_offset;
							}
						}
					}
				}
			}
			for (unsigned int t = 0;t < 8;t++)
			{
				texoffset[t] = d->texcoord_offset[t];
			}
			fvfsize = d->DeclarationSize;
		}

		if (tangentoffset)
		{
			int vertexCount = split.mmc->Vertex->Get_Count();
			TempTangentBuffer.Enlarge(vertexCount);
			Vector2 *uv = split.mmc->DefMatDesc->UV[0]->Get_Array();
			if (uv)
			{
				CalculateTangentArray(vertexCount,split.mmc->Vertex->Get_Array(),split.mmc->Get_Vertex_Normal_Array(),
					uv,split.polygon_count,split.polygon_array,TempTangentBuffer.GetBuffer());
			}
		}

		for (int vertexcount = 0;vertexcount < split.mmc->VertexCount;vertexcount++)
		{
			((Vector3 *)(vbarray + locationoffset))->X = varray->X;
			((Vector3 *)(vbarray + locationoffset))->Y = varray->Y;
			((Vector3 *)(vbarray + locationoffset))->Z = varray->Z;
			if (hasnormals && narray)
			{
				((Vector3 *)(vbarray + normaloffset))->X = narray->X;
				((Vector3 *)(vbarray + normaloffset))->Y = narray->Y;
				((Vector3 *)(vbarray + normaloffset))->Z = narray->Z;
			}
			if (tangentoffset)
			{
				*((Vector3 *)(vbarray + tangentoffset)) = TempTangentBuffer[vertexcount];
			}
			if (hasdiffuse)
			{
				if (ularray)
				{
					*((unsigned int *)(vbarray + diffuseoffset)) = *ularray;
				}
				else
				{
					*((unsigned int *)(vbarray + diffuseoffset)) = 0xFFFFFFFF;
				}
			}
			if (hasspecular)
			{
				if (carray)
				{
					*((unsigned int *)(vbarray + specularoffset)) = *carray;
				}
				else
				{
					*((unsigned int *)(vbarray + specularoffset)) = 0xFFFFFFFF;
				}
			}
			for (unsigned int tc = 0;tc < texcount;tc++)
			{
				UVBufferClass *uv = split.mmc->CurMatDesc->UV[tc];
				if (uv)
				{
					Vector2 *iv = uv->Get_Array();
					if (iv)
					{
						Vector2 *ov = ((Vector2 *)(vbarray + texoffset[tc]));
						ov->X = iv[vertexcount].X;
						ov->Y = iv[vertexcount].Y;
					}
				}
			}
			vbarray += fvfsize;
			varray++;
			narray++;
			if (ularray)
			{
				ularray++;
			}
			if (carray)
			{
				carray++;
			}
		}
	}

	this->Generate_Texture_Categories(&split,this->used_vertices);
	this->used_vertices += split.mmc->VertexCount;
}



void DX8RigidFVFCategoryContainer::Render()
{
	if (!this->AnythingToRender) return;

	int zbias = 0;

	DX8Wrapper::Set_Vertex_Buffer(this->vertex_buffer);
	DX8Wrapper::Set_Index_Buffer(this->index_buffer.Peek(), 0);

	DX8Wrapper::Set_DX8_ZBias(zbias);

	for (unsigned int i = 0;i < this->passes;i++)
	{
		bool anythingRendered = false;

		for (DX8TextureCategoryClass* t = visible_texture_category_list[i].Remove_Head(); t; t = visible_texture_category_list[i].Remove_Head())
		{
			t->Render();
			anythingRendered = true;
		}
		
		if (anythingRendered)
		{
			zbias++;
			if (zbias > 0x0F)
			{
				zbias = 0x0F;
			}
			DX8Wrapper::Set_DX8_ZBias(zbias);
		}
	}
	this->Render_Procedural_Material_Passes();
	DX8Wrapper::Set_DX8_ZBias(0);
}



void DX8SkinFVFCategoryContainer::Render()
{
	if (!AnythingToRender)
	{
		return;
	}
	AnythingToRender = false;
	DX8Wrapper::Set_Vertex_Buffer((VertexBufferClass *)0);
	DynamicDeclarationAccessClass vb = DynamicDeclarationAccessClass(sorting + 2,0x252,(unsigned short)VisibleVertexCount);
	unsigned int renderedVertexCount = 0;
	if (VisibleSkinHead)
	{
		DynamicDeclarationAccessClass::WriteLockClass l = DynamicDeclarationAccessClass::WriteLockClass::WriteLockClass(&vb);
		VertexFormatXYZNDUV2* dest_verts = 0;
		VertexFormatXYZNDUV2Extra* dest_verts_Declaration = 0;
		if ((sorting) || !(((DeclarationVertexBufferClass *)(vb.VertexBuffer))->HasTangents))
		{
			dest_verts = l.Get_Formatted_Vertex_Array();
		}
		else
		{
			dest_verts_Declaration = l.Get_Formatted_Declaration_Vertex_Array();
		}
		MeshClass *mesh = VisibleSkinHead;
		do
		{
			MeshModelClass* mmc = mesh->Peek_Model();
			int mesh_vertex_count = mmc->VertexCount;
			if (TempVertexBuffer.Length() < mesh_vertex_count)
			{
				TempVertexBuffer.Resize(mesh_vertex_count,0);
			}
			if (TempNormalBuffer.Length() < mesh_vertex_count)
			{
				TempNormalBuffer.Resize(mesh_vertex_count,0);
			}
			Vector2* uv0 = mmc->CurMatDesc->Get_UV_Array_By_Index(0,false);
			Vector2* uv1 = mmc->CurMatDesc->Get_UV_Array_By_Index(1,false);
			unsigned int *color = mmc->CurMatDesc->Get_Color_Array(0,false);

			
			if (!vb.VertexBuffer->fvf_info)
			{
				if (((DeclarationVertexBufferClass *)(vb.VertexBuffer))->HasTangents)
				{
					// This skin mesh is declaration based, and it wants tangents
					TempTangentBuffer.Enlarge(mmc->VertexCount);
					mesh->Get_Deformed_Vertexes(&(TempVertexBuffer[0]),&(TempNormalBuffer[0]),TempTangentBuffer.GetBuffer());
				}
				else
				{
					// This skin mesh is declaration based, but it doesn't want tangents
					mesh->Get_Deformed_Vertexes(&(TempVertexBuffer[0]),&(TempNormalBuffer[0]));
				}
			}
			else
			{
				// This skin mesh is FVF based, and it definately doesn't want tangents
				mesh->Get_Deformed_Vertexes(&(TempVertexBuffer[0]),&(TempNormalBuffer[0]));
			};

			if (mesh_vertex_count > 0)
			{
				if ((sorting) || !(((DeclarationVertexBufferClass *)(vb.VertexBuffer))->HasTangents))
				{
					for (int count = 0;count < mesh_vertex_count;count++)
					{
						dest_verts[renderedVertexCount+count].x = TempVertexBuffer[count].X;
						dest_verts[renderedVertexCount+count].y = TempVertexBuffer[count].Y;
						dest_verts[renderedVertexCount+count].z = TempVertexBuffer[count].Z;
						dest_verts[renderedVertexCount+count].nx = TempNormalBuffer[count].X;
						dest_verts[renderedVertexCount+count].ny = TempNormalBuffer[count].Y;
						dest_verts[renderedVertexCount+count].nz = TempNormalBuffer[count].Z;
						if (color)
						{
							dest_verts[renderedVertexCount+count].diffuse = color[count];
						}
						else
						{
							dest_verts[renderedVertexCount+count].diffuse = 0;
						}
						if (uv0)
						{
							dest_verts[renderedVertexCount+count].u1 = uv0[count].X;
							dest_verts[renderedVertexCount+count].v1 = uv0[count].Y;
						}
						else
						{
							dest_verts[renderedVertexCount+count].u1 = 0;
							dest_verts[renderedVertexCount+count].v1 = 0;
						}
						if (uv1)
						{
							dest_verts[renderedVertexCount+count].u2 = uv1[count].X;
							dest_verts[renderedVertexCount+count].v2 = uv1[count].Y;
						}
						else
						{
							dest_verts[renderedVertexCount+count].u2 = 0;
							dest_verts[renderedVertexCount+count].v2 = 0;
						}
					}
				}
				else
				{
					for (int count = 0;count < mesh_vertex_count;count++)
					{
						dest_verts_Declaration[renderedVertexCount+count].x = TempVertexBuffer[count].X;
						dest_verts_Declaration[renderedVertexCount+count].y = TempVertexBuffer[count].Y;
						dest_verts_Declaration[renderedVertexCount+count].z = TempVertexBuffer[count].Z;
						dest_verts_Declaration[renderedVertexCount+count].nx = TempNormalBuffer[count].X;
						dest_verts_Declaration[renderedVertexCount+count].ny = TempNormalBuffer[count].Y;
						dest_verts_Declaration[renderedVertexCount+count].nz = TempNormalBuffer[count].Z;
						if (color)
						{
							dest_verts_Declaration[renderedVertexCount+count].diffuse = color[count];
						}
						else
						{
							dest_verts_Declaration[renderedVertexCount+count].diffuse = 0;
						}
						if (uv0)
						{
							dest_verts_Declaration[renderedVertexCount+count].u1 = uv0[count].X;
							dest_verts_Declaration[renderedVertexCount+count].v1 = uv0[count].Y;
						}
						else
						{
							dest_verts_Declaration[renderedVertexCount+count].u1 = 0;
							dest_verts_Declaration[renderedVertexCount+count].v1 = 0;
						}
						if (uv1)
						{
							dest_verts_Declaration[renderedVertexCount+count].u2 = uv1[count].X;
							dest_verts_Declaration[renderedVertexCount+count].v2 = uv1[count].Y;
						}
						else
						{
							dest_verts_Declaration[renderedVertexCount+count].u2 = 0;
							dest_verts_Declaration[renderedVertexCount+count].v2 = 0;
						}
						if(TempTangentBuffer.GetCurrentBufferSize() > (unsigned int)count)
						{
							dest_verts_Declaration[renderedVertexCount+count].Tangent = TempTangentBuffer[count];
						}
						else 
						{
							dest_verts_Declaration[renderedVertexCount+count].Tangent = Vector3();
						}
					}				
				}
			}
			mesh->Set_Base_Vertex_Offset(renderedVertexCount);
			renderedVertexCount += mesh_vertex_count;
			mesh = mesh->Peek_Next_Visible_Skin();
		} while (mesh);
	}
	
	TT_ASSERT(VisibleSkinHead);
	Set_Vertex_Buffer(vb);
	DX8Wrapper::Set_Index_Buffer(index_buffer.Peek(),0);
	for (unsigned int i = 0;i < passes; ++i)
	{
		for (DX8TextureCategoryClass* t = visible_texture_category_list[i].Remove_Head(); t; t = visible_texture_category_list[i].Remove_Head())
		{
			t->Render();
		};
	};
	Render_Procedural_Material_Passes();
	VisibleSkinHead = 0;
	VisibleVertexCount = 0;
}



void DX8SkinFVFCategoryContainer::Add_Mesh(MeshClass *mesh)
{
	Vertex_Split_Table split = Vertex_Split_Table(mesh);
	MeshModelClass* mmc = mesh->Peek_Model();

	if (!TangentHashtable[mmc])
	{
		// FIXME FIXME FIXME
		// Single player crash in here is because a skin mesh appears that has no texcoords. 
		// There is nothing to support this situation, so we're going to fake texcoords.
		// I don't like this...
		Vector2* uv_source = mmc->DefMatDesc->Get_UV_Array_By_Index(0,false);
		if (!uv_source) 
		{
			FakeUVSource.Enlarge(mmc->VertexCount);
			uv_source = FakeUVSource.GetBuffer();
		};
		Vector3* position = mmc->Get_Vertex_Array();
		Vector3* normal = mmc->Get_Vertex_Normal_Array();
		Vector3i16* triangles = mmc->Poly->Get_Array();

		Vector3* tangent = new Vector3[mmc->VertexCount];
		
		CalculateTangentArray(mmc->VertexCount,position,normal,uv_source,mmc->PolyCount,triangles,tangent);
		TangentHashtable.Add(mmc,tangent);
	};

	this->Generate_Texture_Categories(&split,0);
}



void DX8SkinFVFCategoryContainer::Add_Visible_Skin(MeshClass *mesh)
{
	mesh->Set_Next_Visible_Skin(VisibleSkinHead);
	VisibleSkinHead = mesh;
	VisibleVertexCount += mesh->Peek_Model()->VertexCount;
}



void DX8SkinFVFCategoryContainer::Add_Delayed_Visible_Material_Pass(MaterialPassClass* pass,MeshClass* mesh)
{
	Add_Visible_Material_Pass(pass,mesh);
}



DX8SkinFVFCategoryContainer::DX8SkinFVFCategoryContainer(bool sorting) : DX8FVFCategoryContainer(67,sorting)
{
	VisibleVertexCount = 0;
	VisibleSkinHead = 0;
}



DX8RigidFVFCategoryContainer::DX8RigidFVFCategoryContainer(unsigned int FVF,bool sorting_) : DX8FVFCategoryContainer(FVF,sorting_), vertex_buffer(0), used_vertices(0), delayed_visible_matpass_head(0), delayed_visible_matpass_tail(0)
{
}



DX8SkinFVFCategoryContainer::~DX8SkinFVFCategoryContainer()
{
}



DX8RigidFVFCategoryContainer::~DX8RigidFVFCategoryContainer()
{
	REF_PTR_RELEASE(vertex_buffer);
}



void DX8SkinFVFCategoryContainer::Log(bool only_visible)
{
}



bool DX8SkinFVFCategoryContainer::Check_If_Mesh_Fits(MeshModelClass *mmc)
{
	if (index_buffer)
	{
		int polycount = mmc->PolyCount;
		if (mmc->GapFiller)
		{
			polycount += mmc->GapFiller->PolygonCount;
		}
		return ((3 * mmc->CurMatDesc->PassCount * polycount) <= (index_buffer->Get_Index_Count() - used_indices));
	}
	else
	{
		return true;
	}
}



void DX8SkinFVFCategoryContainer::Render_Delayed_Procedural_Material_Passes()
{
}



bool DX8RigidFVFCategoryContainer::Check_If_Mesh_Fits(MeshModelClass *mmc)
{
	if (vertex_buffer)
	{
		int polycount = mmc->PolyCount;
		if (mmc->GapFiller)
		{
			polycount += mmc->GapFiller->PolygonCount;
		}
		return ((mmc->VertexCount <= (vertex_buffer->Get_Vertex_Count() - used_vertices)) && ((3 * mmc->CurMatDesc->PassCount * polycount) <= (index_buffer->Get_Index_Count() - used_indices)));
	}
	else
	{
		return true;
	}
}



void DX8RigidFVFCategoryContainer::Log(bool only_visible)
{
}



DX8FVFCategoryContainer::DX8FVFCategoryContainer(unsigned int FVF_, bool sorting_) :
	visible_matpass_head(0),
	visible_matpass_tail(0),
	index_buffer(0),
	used_indices(0),
	FVF(FVF_),
	passes(4),
	uv_coordinate_channels(0),
	sorting(sorting_),
	AnythingToRender(false),
	DelayedAnythingToRender(false)
{
	uv_coordinate_channels = (FVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
}



DX8FVFCategoryContainer::~DX8FVFCategoryContainer()
{
	for (uint pass = 0; pass < passes; ++pass)
		while (texture_category_list[pass].Get_Head())
			delete texture_category_list[pass].Remove_Head();
}



void DX8FVFCategoryContainer::Render_Procedural_Material_Passes()
{
	for (MatPassTaskClass* node = visible_matpass_head, *nextNode; node; node = nextNode)
	{
		nextNode = node->Get_Next_Visible();
		node->Peek_Mesh()->Render_Material_Pass(node->Peek_Material_Pass(), index_buffer.Peek());
		delete node;
	}
	visible_matpass_head = NULL;
	visible_matpass_tail = NULL;
}



void DX8FVFCategoryContainer::Add_Visible_Material_Pass(MaterialPassClass* pass,MeshClass* mesh)
{
	MatPassTaskClass *m = new MatPassTaskClass(pass,mesh);
	if (visible_matpass_head)
	{
		visible_matpass_tail->Set_Next_Visible(m);
	}
	else
	{
		visible_matpass_head = m;
	}
	visible_matpass_tail = m;
	AnythingToRender = true;
}



uint DX8FVFCategoryContainer::Define_FVF(MeshModelClass* model, uint* userLightingArray, bool enableLighting)
{
	if (model->Flags & MeshGeometryClass::SORT && WW3D::IsSortingEnabled)
		return D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX2;
	
	MeshMatDescClass* matDesc = model->CurMatDesc;
	uint result = D3DFVF_XYZ;

	if (matDesc->ColorArray[0] || userLightingArray)
		result |= D3DFVF_DIFFUSE;

	if (matDesc->ColorArray[1])
		result |= D3DFVF_SPECULAR;

	uint uvCount = 0;
	for (; uvCount < 8 && matDesc->UV[uvCount]; ++uvCount);
	result |= uvCount << D3DFVF_TEXCOUNT_SHIFT;
	
	if (model->Needs_Vertex_Normals())
		result |= D3DFVF_NORMAL;

	return result;
}



void DX8FVFCategoryContainer::Remove_Texture_Category(DX8TextureCategoryClass* textureCategory)
{
	for (uint i = 0; i < passes; ++i)
		texture_category_list[i].Remove(textureCategory);
	
	for (uint i = 0; i < passes; ++i)
		if (texture_category_list[i].Get_Head())
			return;
	
	fvf_category_container_delete_list.Add_Tail(this);
}



void DX8FVFCategoryContainer::Insert_To_Texture_Category(Vertex_Split_Table& vertexSplitTable, TextureClass** textures, VertexMaterialClass* material, ShaderClass shader, int pass, uint vertexOffset)
{
	for (MultiListIterator<DX8TextureCategoryClass> textureCategory(&texture_category_list[pass]); textureCategory; ++textureCategory)
	{
		bool texturesMatch = true;
		for (uint i = 0; i < 2; ++i)
			texturesMatch = texturesMatch && textureCategory->Peek_Texture(i) == textures[i];
		
		if (texturesMatch)
		{
			uint32 materialCrc1 = textureCategory->Peek_Material() ? textureCategory->Peek_Material()->Get_CRC() : 0;
			uint32 materialCrc2 = material ? material->Get_CRC() : 0;
			
			if (materialCrc1 == materialCrc2 &&
				textureCategory->Get_Shader() == shader)
			{
				used_indices += textureCategory->Add_Mesh(vertexSplitTable, vertexOffset, used_indices, index_buffer.Peek(), pass);
				return;
			}
		}
	}
	
	DX8TextureCategoryClass* newTextureCategory = new DX8TextureCategoryClass(this, textures, shader, material, pass);
	used_indices += newTextureCategory->Add_Mesh(vertexSplitTable, vertexOffset, used_indices, index_buffer.Peek(), pass);
	
	for (MultiListIterator<DX8TextureCategoryClass> textureCategory(&texture_category_list[pass]); textureCategory; ++textureCategory)
	{
		if (textureCategory->Peek_Texture(0) == textures[0])
		{
			texture_category_list[pass].Add_After(newTextureCategory, *textureCategory);
			return;
		}
	}
	
	texture_category_list[pass].Add_Tail(newTextureCategory);
}



void DX8FVFCategoryContainer::Generate_Texture_Categories(Vertex_Split_Table* vertexSplitTable, uint vertexOffset)
{
	if (!index_buffer)
	{
		uint16 indexCount = (uint16)max(12000, 3 * vertexSplitTable->polygon_count * vertexSplitTable->mmc->CurMatDesc->PassCount);
		if (sorting)
			index_buffer = Create_NEW(new SortingIndexBufferClass(indexCount));
		else
			index_buffer = Create_NEW(new DX8IndexBufferClass(indexCount, ShaderCaps::NPatchesSupported && (uint&)WW3D::NPatchesLevel > 1u ? DX8IndexBufferClass::USAGE_NPATCHES : DX8IndexBufferClass::USAGE_DEFAULT)); // Usage is not used
	}
	
	Textures_Material_And_Shader_Booking_Struct bookings;
	for (int pass = 0; pass < vertexSplitTable->mmc->CurMatDesc->PassCount; ++pass)
	{
		for (int polygonIndex = 0; polygonIndex < vertexSplitTable->polygon_count; ++polygonIndex)
		{
			Textures_Material_And_Shader_Booking_Struct::Booking booking;
			booking.textures[0] = vertexSplitTable->Peek_Texture(polygonIndex, pass, 0);
			booking.textures[1] = vertexSplitTable->Peek_Texture(polygonIndex, pass, 1);
			booking.material = vertexSplitTable->Peek_Material(polygonIndex, pass);
			booking.shader = vertexSplitTable->Peek_Shader(polygonIndex, pass);

			if (bookings.Add_Textures_Material_And_Shader(booking))
				Insert_To_Texture_Category(*vertexSplitTable, booking.textures, booking.material, booking.shader, pass, vertexOffset);
		}
	}
}
