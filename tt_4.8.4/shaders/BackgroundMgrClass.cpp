#include "General.h"
#include "BackgroundMgrClass.h"
#include "Random2Class.h"
#include "indexbuffer.h"
#include "vertexbuffer.h"
#include "engine_3d.h"
#include "DX8Wrapper.h"
#include "WW3DAssetManager.h"
#include "WW3D.h"
#include "RenderInfoClass.h"
#include "CameraClass.h"
#include "WWAudioClass.h"
#include "AudibleSoundClass.h"
#include "PhysicsSceneClass.h"
unsigned BackgroundMgrClass::_Hours;
unsigned BackgroundMgrClass::_Minutes;
BackgroundMgrClass::LightSourceTypeEnum BackgroundMgrClass::_LightSourceType;
SkyClass::MoonTypeEnum BackgroundMgrClass::_MoonType;
bool BackgroundMgrClass::MoonIsEarth;
BackgroundParameterClass BackgroundMgrClass::_Parameters[PARAMETER_COUNT];
Vector3 BackgroundMgrClass::_LightVector;
Vector3 BackgroundMgrClass::_UnitLightVector;
unsigned BackgroundMgrClass::_CloudOverrideCount;
unsigned BackgroundMgrClass::_LightningOverrideCount;
unsigned BackgroundMgrClass::_SkyTintOverrideCount;
bool BackgroundMgrClass::_Dirty;
SkyClass *BackgroundMgrClass::_Sky = NULL;
DazzleRenderObjClass *BackgroundMgrClass::_Dazzle = NULL;
bool BackgroundMgrClass::_LensFlareVisible = true;
static Random2Class _RandomNumber(0x1f855092);
HazeClass::HazeClass(float radius) : RowCount(ROW_COUNT)
{
	const Vector3 white(1.0f, 1.0f, 1.0f);
	const float longitude[ROW_COUNT + 1] = {1.22f, 1.55f, 1.57f, 1.92f};
	const unsigned segmentcount = 16;
	const float twopioosegmentcount = (1.0f / segmentcount) * 2.0f * WWMATH_PI;
	unsigned row, segment;
	unsigned v;
	float latitude;
	float x, y, z;
	unsigned short i;
	VertexCount	= (RowCount + 1) * segmentcount;
	TriangleCount = segmentcount * (RowCount * 2);
	VertexArray = new Vector3[VertexCount];
	v = 0;
	for (segment = 0; segment < segmentcount; segment++)
	{
		latitude = segment * twopioosegmentcount;
		for (row = 0; row < RowCount + 1; row++)
		{
			x = radius * sinf(longitude[row]) * cosf(latitude);
			y = radius * sinf(longitude[row]) * sinf(latitude);
			z = radius * cosf(longitude[row]);
			VertexArray[v].Set(x, y, z);
			v++;
		}
	}
	IndexBuffer = new DX8IndexBufferClass(((unsigned short)TriangleCount * VERTICES_PER_TRIANGLE),DX8IndexBufferClass::USAGE_DEFAULT);
	{
		DX8IndexBufferClass::WriteLockClass lock(IndexBuffer);
		unsigned short *indices = lock.Get_Index_Array();
		i = 0;
		for (row = 0; row < RowCount; row++)
		{
			v = row;
			for (segment = 0; segment < segmentcount; segment++)
			{
				if (segment < segmentcount - 1)
				{
					indices[i + 0] = (unsigned short)(v);
					indices[i + 1] = (unsigned short)(v + RowCount + 1);
					indices[i + 2] = (unsigned short)(v + RowCount + 2);
					indices[i + 3] = (unsigned short)(v + RowCount + 2);
					indices[i + 4] = (unsigned short)(v + 1);
					indices[i + 5] = (unsigned short)(v);
				}
				else
				{
					indices[i + 0] = (unsigned short)(v);
					indices[i + 1] = (unsigned short)(row);
					indices[i + 2] = (unsigned short)(row + 1);
					indices[i + 3] = (unsigned short)(row + 1);
					indices[i + 4] = (unsigned short)(v + 1);
					indices[i + 5] = (unsigned short)(v);
				}
				v += RowCount + 1;
				i += 6;
			}
		}
	}
	Material = VertexMaterialClass::Get_Preset(VertexMaterialClass::PRELIT_NODIFFUSE);
	Shader = ShaderClass::_PresetOpaque2DShader;
	Shader.Set_Primary_Gradient(ShaderClass::GRADIENT_MODULATE);
	Shader.Set_Texturing(ShaderClass::TEXTURING_DISABLE);
	Shader.Set_Cull_Mode(ShaderClass::CULL_MODE_DISABLE);
	DiffuseArray = new unsigned[VertexCount];
	Configure(white, white, 1.0f);
}
HazeClass::~HazeClass()
{
	delete[] DiffuseArray;
	REF_PTR_RELEASE(Material);
	REF_PTR_RELEASE(IndexBuffer);
	delete[] VertexArray;
}
void HazeClass::Configure(const Vector3 &blendcolor, const Vector3 &horizoncolor, float intensity)
{
	BlendColor = blendcolor;
	HorizonColor = horizoncolor;
	Intensity = intensity;
	Configure();
}
void HazeClass::Configure(const Vector3 &blendcolor, const Vector3 &horizoncolor)
{
	BlendColor = blendcolor;
	HorizonColor = horizoncolor;
	Configure();
}
void HazeClass::Configure(const Vector3 &blendcolor, float intensity)
{
	BlendColor = blendcolor;
	Intensity = intensity;
	Configure();
}
void HazeClass::Configure(const Vector3 &blendcolor)
{
	BlendColor = blendcolor;
	Configure();
}
void HazeClass::Configure()
{
	const unsigned d3dblendcolor = GetD3dColor(BlendColor, 1.0f);
	const unsigned d3dhorizoncolor = GetD3dColor(HorizonColor * Intensity, 1.0f);
	for (unsigned v = 0; v < VertexCount; v++)
	{
		switch (v % (RowCount + 1))
		{
			case 0:
				DiffuseArray[v] = d3dblendcolor;
				break;
			default:
				DiffuseArray[v] = d3dhorizoncolor;
				break;
		}
	}
	Set_Visibility(true);
}
void HazeClass::Render()
{
	if (Is_Visible())
	{
		DynamicVBAccessClass dynamicvb(2,0x252,(unsigned short)VertexCount);
		{
			DynamicVBAccessClass::WriteLockClass lock(&dynamicvb);
			VertexFormatXYZNDUV2 *vertex = lock.Get_Formatted_Vertex_Array();
			for (unsigned v = 0; v < VertexCount; v++)
			{
				vertex->x = VertexArray[v].X;
				vertex->y = VertexArray[v].Y;
				vertex->z = VertexArray[v].Z;
				vertex->diffuse = DiffuseArray[v];
				vertex++;
			}
		}
		DX8Wrapper::Set_Material(Material);
		DX8Wrapper::Set_Shader(Shader);
		DX8Wrapper::Set_Index_Buffer(IndexBuffer, 0);
		DX8Wrapper::Set_Vertex_Buffer(dynamicvb);
		DX8Wrapper::Draw_Triangles(0, (uint16)TriangleCount, 0, (uint16)VertexCount);
	}
}
StarfieldClass::StarfieldClass (float extent, unsigned starcount)
{
	const Vector3 white(1.0f, 1.0f, 1.0f);
	VertexCount = VERTICES_PER_TRIANGLE * starcount;
	TriangleCount = starcount;
	VertexArray = new Vector3[VertexCount];
	IndexBuffer = new DX8IndexBufferClass((unsigned short)VertexCount,DX8IndexBufferClass::USAGE_DEFAULT);
	{
		DX8IndexBufferClass::WriteLockClass lock(IndexBuffer);
		unsigned short *indices = lock.Get_Index_Array();
		for (unsigned short i = 0; i < VertexCount; i++)
		{
			indices[i] = i;
		}
	}
	Material = VertexMaterialClass::Get_Preset(VertexMaterialClass::PRELIT_NODIFFUSE);
	Shader = ShaderClass::_PresetAlpha2DShader;
	Shader.Set_Primary_Gradient(ShaderClass::GRADIENT_MODULATE);
	Shader.Set_Cull_Mode(ShaderClass::CULL_MODE_DISABLE);
	Texture = WW3DAssetManager::TheInstance->Get_Texture("Star.tga",TextureClass::MIP_LEVELS_ALL,WW3D_FORMAT_UNKNOWN,true);
	Texture->UAddressMode = TextureClass::TEXTURE_ADDRESS_CLAMP;
	Texture->VAddressMode = TextureClass::TEXTURE_ADDRESS_CLAMP;
	DiffuseArray = new unsigned[VertexCount];
	Configure(Vector3(1.0f, 0.0f, 0.0f), 1.0f, 1.0f, white, white, 0.0f);
}
StarfieldClass::~StarfieldClass()
{
	delete[] DiffuseArray;
	REF_PTR_RELEASE(Texture);
	REF_PTR_RELEASE(Material);
	REF_PTR_RELEASE(IndexBuffer);
	delete[] VertexArray;
}
void StarfieldClass::Configure(const Vector3 &orientation, float length, float radius, const Vector3 &color0, const Vector3 &color1, float alpha)
{
	Orientation = orientation;
	Length = length;
	Radius = radius;
	Color0 = color0;
	Color1 = color1;
	Alpha = alpha;
	Configure();
}
void StarfieldClass::Configure(const Vector3 &orientation, float length, float radius)
{
	Orientation = orientation;
	Length = length;
	Radius = radius;
	Configure();
}
void StarfieldClass::Configure(const Vector3 &color0, const Vector3 &color1, float alpha)
{
	Color0 = color0;
	Color1 = color1;
	Alpha = alpha;
	Configure();
}
void StarfieldClass::Configure()
{
	const unsigned positionrandomness = 8192;
	const float oopositionrandomnesstwo = 2.0f / positionrandomness;
	const unsigned radiusrandomness = 16;
	const float ooradiusrandomness = 1.0f / radiusrandomness;
	const float	minradius = 0.65f;
	const float maxradius = 0.90f;
	const unsigned intensityrandomness = 16;
	const float oointensityrandomness = 1.0f / intensityrandomness;
	const float theta = sinf(WWMATH_PI / 36);
	Matrix3D m0;
	unsigned t;
	Vector3 colordifference;
	float maxdp;
	colordifference = Color1 - Color0;
	m0.Look_At(Vector3 (0.0f, 0.0f, 0.0f), Orientation, 0.0f);
	maxdp = Length / sqrtf((Length * Length) + (Radius * Radius));
	ActiveVertexCount = 0;
	ActiveTriangleCount = 0;
	for (t = 0; t < TriangleCount; t++)
	{
		float x, y, z;
		Vector3 d;
		float r, intensity;
		x = (_RandomNumber(0, positionrandomness) * oopositionrandomnesstwo) - 1.0f;
		y = (_RandomNumber(0, positionrandomness) * oopositionrandomnesstwo) - 1.0f;
		z = (_RandomNumber(0, positionrandomness) * oopositionrandomnesstwo) - 1.0f;
		r = _RandomNumber(1, radiusrandomness) * ooradiusrandomness;
		intensity = _RandomNumber(1, intensityrandomness) * oointensityrandomness;
		d.Set(x, y, z);
		d.Normalize();
		d = m0 * d;
		if (d.Z >= theta)
		{
			if (Vector3::Dot_Product(d, Orientation) < maxdp)
			{
				Matrix3D m1;
				Vector3	color;
				unsigned d3dcolor;
				m1.Look_At(d * Length, d * Length * 2.0f, 0.0f);
				r = pow(r, 10) * maxradius;
				r = MAX(r, minradius);
				VertexArray[ActiveVertexCount] = m1 * Vector3(-r, -r, 0.0f);
				VertexArray[ActiveVertexCount + 1] = m1 * Vector3(-r, +r, 0.0f);
				VertexArray[ActiveVertexCount + 2] = m1 * Vector3(+r, -r, 0.0f);
				color = Color0 + (colordifference * intensity);
				d3dcolor = GetD3dColor(color, Alpha);
				DiffuseArray[ActiveVertexCount + 0] = d3dcolor;
				DiffuseArray[ActiveVertexCount + 1] = d3dcolor;
				DiffuseArray[ActiveVertexCount + 2] = d3dcolor;
				ActiveTriangleCount++;
				ActiveVertexCount += VERTICES_PER_TRIANGLE;
			}
		}
	}
	Set_Visibility (Alpha > 0.0f);
}
void StarfieldClass::Render()
{
	if (Is_Visible())
	{
		const unsigned flickercount = 5;
		const float flickeralpha = 0.6f;
		unsigned activeflickercount;
		float alpha, f, frac;
		unsigned triangleindices[flickercount];
		unsigned i, v;
		if (WW3D::Get_Frame_Time() > 0)
		{
			alpha = Alpha * flickeralpha;
			f = flickercount * (((float)ActiveTriangleCount) / TriangleCount);
			frac = f - floorf(f);
			activeflickercount = (unsigned int)((frac < 0.5f) ? floorf(f) : floorf(f) + 1);
			for (i = 0; i < activeflickercount; i++)
			{
				triangleindices[i] = _RandomNumber(0, ActiveTriangleCount - 1);
				v = triangleindices[i] * VERTICES_PER_TRIANGLE;
				DX8Wrapper::Set_Alpha(alpha, DiffuseArray[v + 0]);
				DX8Wrapper::Set_Alpha(alpha, DiffuseArray[v + 1]);
				DX8Wrapper::Set_Alpha(alpha, DiffuseArray[v + 2]);
			}
		}
		else
		{
			activeflickercount = 0;
		}
		DynamicVBAccessClass dynamicvb(2,0x252,(unsigned short)ActiveVertexCount);
		{
			const float	texcoordarray[VERTICES_PER_TRIANGLE][2] = {{0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}};
			DynamicVBAccessClass::WriteLockClass lock(&dynamicvb);
			VertexFormatXYZNDUV2 *vertex = lock.Get_Formatted_Vertex_Array();
			for (unsigned v2 = 0; v2 < ActiveVertexCount; v2 += VERTICES_PER_TRIANGLE)
			{
				for (unsigned t = 0; t < VERTICES_PER_TRIANGLE; t++)
				{
					unsigned i2 = v2 + t;
					vertex->x = VertexArray[i2].X;
					vertex->y = VertexArray[i2].Y;
					vertex->z = VertexArray[i2].Z;
					vertex->diffuse = DiffuseArray[i2];
					vertex->u1 = texcoordarray[t][0];
					vertex->v1 = texcoordarray[t][1];
					vertex++;
				}
			}
		}
		DX8Wrapper::Set_Texture(0, Texture);
		DX8Wrapper::Set_Material(Material);
		DX8Wrapper::Set_Shader(Shader);
		DX8Wrapper::Set_Index_Buffer(IndexBuffer, 0);
		DX8Wrapper::Set_Vertex_Buffer(dynamicvb);
		DX8Wrapper::Draw_Triangles(0,(uint16)ActiveTriangleCount,0,(uint16)ActiveVertexCount);
		for (i = 0; i < activeflickercount; i++)
		{
			v = triangleindices[i] * VERTICES_PER_TRIANGLE;
			DX8Wrapper::Set_Alpha(Alpha, DiffuseArray[v + 0]);
			DX8Wrapper::Set_Alpha(Alpha, DiffuseArray[v + 1]);
			DX8Wrapper::Set_Alpha(Alpha, DiffuseArray[v + 2]);
		}
	}
}
SkyObjectClass::SkyObjectClass(ShaderClass shader) : VertexCount(8),TriangleCount(6)
{
	const Vector3 white(1.0f, 1.0f, 1.0f);
	VertexArray = new Vector3[VertexCount];
	IndexBuffer = new DX8IndexBufferClass(((unsigned short)TriangleCount * VERTICES_PER_TRIANGLE),DX8IndexBufferClass::USAGE_DEFAULT);
	{
		static const unsigned short _indices[] = {0, 4, 5, 5, 1, 0, 1, 5, 6, 6, 2, 1, 2, 6, 7, 7, 3, 2};
		DX8IndexBufferClass::WriteLockClass lock(IndexBuffer);
		unsigned short *indices = lock.Get_Index_Array();
		for (unsigned short i = 0; i < TriangleCount * VERTICES_PER_TRIANGLE; i++)
		{
			indices[i] = _indices[i];
		}
	}
	Material = VertexMaterialClass::Get_Preset(VertexMaterialClass::PRELIT_NODIFFUSE);
	Shader = shader;
	Shader.Set_Primary_Gradient(ShaderClass::GRADIENT_MODULATE);
	Shader.Set_Src_Blend_Func(ShaderClass::SRCBLEND_SRC_ALPHA);
	Shader.Set_Cull_Mode(ShaderClass::CULL_MODE_DISABLE);
	Texture = NULL;
	TexCoordArray = new Vector2[VertexCount];
	DiffuseArray = new unsigned[VertexCount];
	Configure(Vector3(1.0f, 0.0f, 0.0f), 1.0f, 1.0f, white);
}
SkyObjectClass::~SkyObjectClass()
{
	delete[] DiffuseArray;
	delete[] TexCoordArray;
	REF_PTR_RELEASE(Texture);
	REF_PTR_RELEASE(Material);
	REF_PTR_RELEASE(IndexBuffer);
	delete[] VertexArray;
}
void SkyObjectClass::Configure(const Vector3 &direction, float length, float radius, const Vector3 &color)
{
	Direction = direction;
	Length = length;
	Width = radius;
	Height = radius;
	Color = color;
	Configure();
}
void SkyObjectClass::Configure(const Vector3 &direction, float length, float width, float height)
{
	Direction = direction;
	Length = length;
	Width = width;
	Height = height;
	Configure();
}
void SkyObjectClass::Configure(const Vector3 &direction, float length, float radius)
{
	Direction = direction;
	Length = length;
	Width = radius;
	Height = radius;
	Configure();
}
void SkyObjectClass::Configure(const Vector3 &color)
{
	Color = color;
	Configure();
}
void SkyObjectClass::Configure()
{
	const Vector3 black = Vector3(0.0f, 0.0f, 0.0f);
	const float defaultlambda[2] = {0.0f, 1.0f};
	const float zblend[2] = {0.0f, 20.0f};
	Matrix3D m;
	float z0, z1;
	bool visible;
	float lambda[2];
	m.Look_At(Direction * Length, Direction * (Length * 2.0f), 0.0f);
	VertexArray[0] = m * Vector3(-Width, -Height, 0.0f);
	TexCoordArray[0].Set(0.0f, 0.0f);
	VertexArray[4] = m * Vector3(+Width, -Height, 0.0f);
	TexCoordArray[4].Set(1.0f, 0.0f);
	VertexArray[7] = m * Vector3(+Width, +Height, 0.0f);
	TexCoordArray[7].Set(1.0f, 1.0f);
	VertexArray[3] = m * Vector3(-Width, +Height, 0.0f);
	TexCoordArray[3].Set(0.0f, 1.0f);
	z0 = VertexArray[0].Z;
	z1 = VertexArray[3].Z;
	for (unsigned r = 0; r < 2; r++)
	{
		if (((z0 >= zblend[r]) && (z1 >= zblend[r])) || ((z0 <= zblend[r]) && (z1 <= zblend[r])))
		{
			lambda[r] = defaultlambda[r];
		}
		else
		{
			lambda[r] = (zblend[r] - z0) / (z1 - z0);
		}
		VertexArray[1 + r] = VertexArray[0] + lambda[r] * (VertexArray[3] - VertexArray[0]);
		TexCoordArray[1 + r] = TexCoordArray[0] + lambda[r] * (TexCoordArray[3] - TexCoordArray[0]);
		VertexArray[5 + r] = VertexArray[4] + lambda[r] * (VertexArray[7] - VertexArray[4]);
		TexCoordArray[5 + r] = TexCoordArray[4] + lambda[r] * (TexCoordArray[7] - TexCoordArray[4]);
	}
	visible = false;
	for (unsigned v = 0; v < VertexCount; v++)
	{
		float alpha;
		if (VertexArray[v].Z <= zblend[0])
		{
			alpha = 0.0f;
		}
		else
		{
			if (VertexArray[v].Z >= zblend[1])
			{
				alpha = 1.0f;
			}
			else
			{
				alpha = (VertexArray[v].Z - zblend[0]) / (zblend[1] - zblend[0]);
			}
		}
		visible |= (alpha > 0.0f);
		DiffuseArray[v] = GetD3dColor(Color, alpha);
	}
	Set_Visibility(visible);
}
void SkyObjectClass::Set_Texture (const char *texturename)
{
	REF_PTR_RELEASE(Texture);
	Texture = WW3DAssetManager::TheInstance->Get_Texture(texturename,TextureClass::MIP_LEVELS_ALL,WW3D_FORMAT_UNKNOWN,true);
}
void SkyObjectClass::Render()
{
	if (Is_Visible())
	{
		DynamicVBAccessClass dynamicvb(2,0x252,(unsigned short)VertexCount);
		{
			DynamicVBAccessClass::WriteLockClass lock(&dynamicvb);
			VertexFormatXYZNDUV2 *vertex = lock.Get_Formatted_Vertex_Array();
			for (unsigned v = 0; v < VertexCount; v++)
			{
				vertex->x = VertexArray[v].X;
				vertex->y = VertexArray[v].Y;
				vertex->z = VertexArray[v].Z;
				vertex->diffuse = DiffuseArray[v];
				vertex->u1 = TexCoordArray[v].X;
				vertex->v1 = TexCoordArray[v].Y;
				vertex++;
			}
		}
		DX8Wrapper::Set_Texture(0, Texture);
		DX8Wrapper::Set_Material(Material);
		DX8Wrapper::Set_Shader(Shader);
		DX8Wrapper::Set_Index_Buffer(IndexBuffer, 0);
		DX8Wrapper::Set_Vertex_Buffer(dynamicvb);
		DX8Wrapper::Draw_Triangles(0, (uint16)TriangleCount, 0, (uint16)VertexCount);
	}
}
CloudLayerClass::CloudLayerClass(float maxdistance, const char *texturename, const Vector2 &velocity, float tilefactor, bool rotate) : RowCount(4)
{
	const Vector3 white(1.0f, 1.0f, 1.0f);
	const Vector3 black(0.0f, 0.0f, 0.0f);
	const float radius = maxdistance;
	const float maxlongitude = WWMATH_PI / 15;
	const float oosinmaxlongitude = 1.0f / sinf(maxlongitude);
	const float oorowcount = 1.0f / RowCount;
	const unsigned segmentcount = 16;
	const float twopioosegmentcount = (1.0f / segmentcount) * 2.0f * WWMATH_PI;
	unsigned row, segment;
	unsigned v;
	unsigned short i;
	float longitude, latitude;
	float x, y, z;
	float scale;
	VertexCount = (RowCount + 1) * segmentcount;
	TriangleCount = segmentcount * (RowCount * 2);
	VertexArray = new Vector3[VertexCount];
	v = 0;
	for (segment = 0; segment < segmentcount; segment++)
	{
		latitude = segment * twopioosegmentcount;
		for (row = 0; row < RowCount + 1; row++)
		{
			longitude = (sinf(row * oorowcount * maxlongitude) * oosinmaxlongitude) * row * oorowcount * maxlongitude;
			x = radius * sinf(longitude) * cosf(latitude);
			y = radius * sinf(longitude) * sinf(latitude);
			z = radius * (cosf(longitude) - cosf(maxlongitude));
			VertexArray[v].Set(x, y, z);
			v++;
		}
	}
	IndexBuffer = new DX8IndexBufferClass((unsigned short)(TriangleCount * VERTICES_PER_TRIANGLE),DX8IndexBufferClass::USAGE_DEFAULT);
	{
		DX8IndexBufferClass::WriteLockClass lock(IndexBuffer);
		unsigned short *indices = lock.Get_Index_Array();
		i = 0;
		for (row = 0; row < RowCount; row++)
		{
			v = row;
			for (segment = 0; segment < segmentcount; segment++)
			{
				if (segment < segmentcount - 1)
				{
					indices[i + 0] = (unsigned short)(v);
					indices[i + 1] = (unsigned short)(v + RowCount + 1);
					indices[i + 2] = (unsigned short)(v + RowCount + 2);
					indices[i + 3] = (unsigned short)(v + RowCount + 2);
					indices[i + 4] = (unsigned short)(v + 1);
					indices[i + 5] = (unsigned short)(v);
				}
				else
				{
					indices[i + 0] = (unsigned short)(v);
					indices[i + 1] = (unsigned short)(row);
					indices[i + 2] = (unsigned short)(row + 1);
					indices[i + 3] = (unsigned short)(row + 1);
					indices[i + 4] = (unsigned short)(v + 1);
					indices[i + 5] = (unsigned short)(v);
				}
				v += RowCount + 1;
				i += 6;
			}
		}
	}
	Material = VertexMaterialClass::Get_Preset(VertexMaterialClass::PRELIT_NODIFFUSE);
	Shader = ShaderClass::_PresetAlpha2DShader;
	Shader.Set_Primary_Gradient(ShaderClass::GRADIENT_MODULATE);
	Shader.Set_Cull_Mode(ShaderClass::CULL_MODE_DISABLE);
	Texture = WW3DAssetManager::TheInstance->Get_Texture(texturename,TextureClass::MIP_LEVELS_ALL,WW3D_FORMAT_UNKNOWN,true);
	Texture->UAddressMode = TextureClass::TEXTURE_ADDRESS_REPEAT;
	Texture->VAddressMode = TextureClass::TEXTURE_ADDRESS_REPEAT;
	TexCoordArray = new Vector2[VertexCount];
	scale = tilefactor / (radius * sinf(maxlongitude));
	for (v = 0; v < VertexCount; v++)
	{
		if (rotate)
		{
			TexCoordArray[v] = Vector2(VertexArray[v].Y, VertexArray[v].X) * scale;
		}
		else
		{
			TexCoordArray[v] = Vector2(VertexArray[v].X, VertexArray[v].Y) * scale;
		}
	}
	if (rotate)
	{
		Velocity.X = velocity.Y;
		Velocity.Y = velocity.X;
  	}
	else
	{
		Velocity = velocity;
	}
	Velocity *= tilefactor;
	DiffuseArray = new unsigned[VertexCount];
	WarmColor = white;
	ColdColor = white;
	WarmDirection = Vector3(1.0f, 0.0f, 0.0f);
	Alpha = 1.0f;
	CloudIntensity = 1.0f;
	HorizonIntensity = 1.0f;
	Configure();
}
CloudLayerClass::~CloudLayerClass()
{
	delete[] DiffuseArray;
	delete[] TexCoordArray;
	REF_PTR_RELEASE(Texture);
	REF_PTR_RELEASE(Material);
	REF_PTR_RELEASE(IndexBuffer);
	delete[] VertexArray;
}
void CloudLayerClass::Configure(const Vector3 &warmcolor, const Vector3 &coldcolor)
{
	WarmColor = warmcolor;
	ColdColor = coldcolor;
	Configure();
}
void CloudLayerClass::Configure(const Vector3 &warmdirection)
{
	WarmDirection = warmdirection;
	Configure();
}
void CloudLayerClass::Configure(const Vector3 &warmcolor, const Vector3 &coldcolor, const Vector3 &warmdirection)
{
	WarmColor = warmcolor;
	ColdColor = coldcolor;
	WarmDirection = warmdirection;
	Configure();
}
void CloudLayerClass::Configure(float alpha, float cloudintensity, float horizonintensity)
{
	Alpha = alpha;
	CloudIntensity = cloudintensity;
	HorizonIntensity = horizonintensity;
	Configure();
}
void CloudLayerClass::Configure()
{
	const Vector3 cap(1.0f, 1.0f, 1.0f);
	Vector3 warmcloudcolor, coldcloudcolor, warmhorizoncolor, coldhorizoncolor;
	warmcloudcolor = WarmColor * CloudIntensity;
	coldcloudcolor = ColdColor * CloudIntensity;
	warmhorizoncolor = WarmColor * HorizonIntensity;
	coldhorizoncolor = ColdColor * HorizonIntensity;
	for (unsigned v = 0; v < VertexCount; v++)
	{
		float interpolant, alpha;
		Vector3 n, color;
		n = Normalize(VertexArray[v]);
		interpolant = Vector3::Dot_Product(WarmDirection, n);
		if (interpolant < 0.0f) interpolant = 0.0f;
		if (v % (RowCount + 1) < RowCount)
		{
			Vector3::Lerp(coldcloudcolor, warmcloudcolor, interpolant, &color);
			alpha = Alpha;
		}
		else
		{
			Vector3::Lerp(coldhorizoncolor, warmhorizoncolor, interpolant, &color);
			alpha = 0.0f;
		}
		DiffuseArray[v] = GetD3dColor(color, alpha);
	}
	Set_Visibility(Alpha > 0.0f);
}
void CloudLayerClass::Render()
{
	const float wraplimit = 8.0f;
	Vector2 offset;
	offset = (WW3D::Get_Frame_Time() * 0.001f) * Velocity;
	if (TexCoordArray[0].X < -wraplimit)
	{
		offset.X += wraplimit;
	}
	else
	{
		if (TexCoordArray[0].X > wraplimit)
		{
			offset.X -= wraplimit;
		}
	}
	if (TexCoordArray[0].Y < -wraplimit)
	{
		offset.Y += wraplimit;
	}
	else
	{
		if (TexCoordArray[0].Y > wraplimit)
		{
			offset.Y -= wraplimit;
		}
	}
	for (unsigned v = 0; v < VertexCount; v++)
	{
		TexCoordArray[v] += offset;
	}
	if (Is_Visible())
	{
		DynamicVBAccessClass dynamicvb(2,0x252,(unsigned short)VertexCount);
		{
			DynamicVBAccessClass::WriteLockClass lock(&dynamicvb);
			VertexFormatXYZNDUV2 *vertex = lock.Get_Formatted_Vertex_Array();
			for (unsigned v = 0; v < VertexCount; v++)
			{
				vertex->x = VertexArray[v].X;
				vertex->y = VertexArray[v].Y;
				vertex->z = VertexArray[v].Z;
				vertex->diffuse = DiffuseArray[v];
				vertex->u1 = TexCoordArray[v].X;
				vertex->v1 = TexCoordArray[v].Y;
				vertex++;
			}
		}
		DX8Wrapper::Set_Texture(0,Texture);
		DX8Wrapper::Set_Material(Material);
		DX8Wrapper::Set_Shader(Shader);
		DX8Wrapper::Set_Index_Buffer(IndexBuffer, 0);
		DX8Wrapper::Set_Vertex_Buffer(dynamicvb);
		DX8Wrapper::Draw_Triangles(0,(uint16)TriangleCount,0,(uint16)VertexCount);
	}
}
SkyGlowClass::SkyGlowClass(float radius) : RowCount(ROW_COUNT), Radius(radius)
{
	const Vector3 white(1.0f, 1.0f, 1.0f);
	const float longitude[ROW_COUNT + 1] = {1.37f, 1.42f, 1.47f, 1.52f, 1.57f, 1.92f};
	const unsigned segmentcount = 32;
	const float twopioosegmentcount = (1.0f / segmentcount) * 2.0f * WWMATH_PI;
	unsigned row, segment;
	unsigned v;
	float latitude;
	float x, y, z;
	unsigned short i;
	VertexCount = (RowCount + 1) * segmentcount;
	TriangleCount = segmentcount * (RowCount * 2);
	MinZ = 0.0f;
	MaxZ = radius * cosf (longitude[0]);
	VertexArray = new Vector3[VertexCount];
	v = 0;
	for (segment = 0; segment < segmentcount; segment++)
	{
		latitude = segment * twopioosegmentcount;
		for (row = 0; row < RowCount + 1; row++)
		{
			x = radius * sinf(longitude[row]) * cosf(latitude);
			y = radius * sinf(longitude[row]) * sinf(latitude);
			z = radius * cosf(longitude[row]);
			VertexArray[v].Set(x, y, z);
			v++;
		}
	}
	IndexBuffer = new DX8IndexBufferClass((unsigned short)(TriangleCount * VERTICES_PER_TRIANGLE),DX8IndexBufferClass::USAGE_DEFAULT);
	{
		DX8IndexBufferClass::WriteLockClass lock(IndexBuffer);
		unsigned short *indices = lock.Get_Index_Array();
		i = 0;
		for (row = 0; row < RowCount; row++)
		{
			v = row;
			for (segment = 0; segment < segmentcount; segment++)
			{
				if (segment < segmentcount - 1)
				{
					indices[i + 0] = (unsigned short)(v);
					indices[i + 1] = (unsigned short)(v + RowCount + 1);
					indices[i + 2] = (unsigned short)(v + RowCount + 2);
					indices[i + 3] = (unsigned short)(v + RowCount + 2);
					indices[i + 4] = (unsigned short)(v + 1);
					indices[i + 5] = (unsigned short)(v);
				} else {
					indices[i + 0] = (unsigned short)(v);
					indices[i + 1] = (unsigned short)(row);
					indices[i + 2] = (unsigned short)(row + 1);
					indices[i + 3] = (unsigned short)(row + 1);
					indices[i + 4] = (unsigned short)(v + 1);
					indices[i + 5] = (unsigned short)(v);
				}
				v += RowCount + 1;
				i += 6;
			}
		}
	}
	Material = VertexMaterialClass::Get_Preset(VertexMaterialClass::PRELIT_NODIFFUSE);
	Shader = ShaderClass::_PresetAdditive2DShader;
	Shader.Set_Primary_Gradient(ShaderClass::GRADIENT_MODULATE);
	Shader.Set_Texturing(ShaderClass::TEXTURING_DISABLE);
	Shader.Set_Cull_Mode(ShaderClass::CULL_MODE_DISABLE);
	DiffuseArray = new unsigned[VertexCount];
	Configure(Vector2(1.0f, 0.0f), white, 1.0f);
}
SkyGlowClass::~SkyGlowClass()
{
	delete[] DiffuseArray;
	REF_PTR_RELEASE(Material);
	REF_PTR_RELEASE(IndexBuffer);
	delete[] VertexArray;
}
void SkyGlowClass::Configure(const Vector2 &hotdirection, const Vector3 &horizoncolor, float coldintensity)
{
	HotDirection = Normalize(hotdirection);
	HorizonColor = horizoncolor;
	ColdIntensity = coldintensity;
	Configure();
}
void SkyGlowClass::Configure()
{
	const float ooradius = 1.0f / Radius;
	const float exponent = 7.0f;
	const float oomaxz = 1.0f / MaxZ;
	const Vector3 hotdirection(HotDirection.X, HotDirection.Y, 0.0f);
	for (unsigned v = 0; v < VertexCount; v++)
	{
		float dotproduct, gradient, z;
		dotproduct = Vector3::Dot_Product(hotdirection,VertexArray[v] * ooradius);
		dotproduct = pow(dotproduct,exponent);
		if (dotproduct < ColdIntensity) dotproduct = ColdIntensity;
		z = VertexArray[v].Z;
		if (z  < MinZ)
		{
			gradient = 1.0f;
		}
		else
		{
			gradient = 1.0f - (z * oomaxz);
		}
		DiffuseArray[v] = GetD3dColor(HorizonColor * (dotproduct * gradient), 1.0f);
	}
	Set_Visibility(true);
}
void SkyGlowClass::Render()
{
	if (Is_Visible())
	{
		DynamicVBAccessClass dynamicvb(2,0x252,(unsigned short)VertexCount);
		{
			DynamicVBAccessClass::WriteLockClass lock(&dynamicvb);
			VertexFormatXYZNDUV2 *vertex = lock.Get_Formatted_Vertex_Array();
			for (unsigned v = 0; v < VertexCount; v++)
			{
				vertex->x = VertexArray[v].X;
				vertex->y = VertexArray[v].Y;
				vertex->z = VertexArray[v].Z;
				vertex->diffuse = DiffuseArray[v];
				vertex++;
			}
		}
		DX8Wrapper::Set_Material(Material);
		DX8Wrapper::Set_Shader(Shader);
		DX8Wrapper::Set_Index_Buffer(IndexBuffer, 0);
		DX8Wrapper::Set_Vertex_Buffer(dynamicvb);
		DX8Wrapper::Draw_Triangles(0, (uint16)TriangleCount, 0, (uint16)VertexCount);
	}
}
LightningBoltClass::LightningBoltClass(int branchcount, Matrix3D &m, float length, float childlength, float width, float amplitude) : Branches(NULL)
{
	const int randomness = 100;
	const float oorandomness = 1.0f / randomness;
	const unsigned maxvertexcount = (unsigned int)MAX(2, length * 0.5f);
	const float oomaxvertexcountminusone = 1.0f / (maxvertexcount - 1);
	const char *texturename = "LightningBolt.tga";
	Vector3 white(1.0f, 1.0f, 1.0f);
	unsigned vertexcount;
	float x;
	Vector3 *localvertex, *worldvertex;
	int branchrandomness;
	TextureClass *texture;
 	localvertex = new Vector3[maxvertexcount];
	worldvertex = new Vector3[maxvertexcount];
	vertexcount = 0;
	x = 0;
	for (unsigned v = 0; v < maxvertexcount; v++)
	{
		vertexcount++;
		localvertex[v] = Vector3(x, -oomaxvertexcountminusone * v * length, 0.0f);
		worldvertex[v] = m * localvertex[v];
		x = x + _RandomNumber(-randomness, +randomness) * 0.5f * oorandomness * amplitude;
	}
	Set_Points(vertexcount, worldvertex);
	texture = WW3DAssetManager::TheInstance->Get_Texture(texturename,TextureClass::MIP_LEVELS_3,WW3D_FORMAT_UNKNOWN,true);
	Set_Texture(texture);
	REF_PTR_RELEASE(texture);
	Set_Shader(ShaderClass::_PresetAdditive2DShader);
	Set_Width(width);
	Set_Texture_Mapping_Mode(SegLineRendererClass::UNIFORM_WIDTH_TEXTURE_MAP);
	Set_Merge_Intersections(false);
	Set_Disable_Sorting(true);
	BranchCount = (vertexcount == maxvertexcount) ? branchcount : 0;
	if (BranchCount > 0)
	{
		const float	minbranchangle = WWMATH_PI * 0.167f;
		const float	maxbranchangle = WWMATH_PI * 0.223f;
		const float	branchfactor = 0.50f;
		const float minlengthfactor = 0.20f;
		const float maxlengthfactor = 0.45f;
		const float widthfactor = 0.30f;
		const float	minwidth = 0.13f;
		const float amplitudefactor = 0.85f;
		float oobranchcount;
		int	branchcount2;
		float minlength, maxlength, w, a;
		Branches = new BranchStruct[BranchCount];
		oobranchcount = 1.0f / BranchCount;
		branchrandomness = vertexcount / (BranchCount * 2);
		branchcount2 = (int)(BranchCount * branchfactor);
		minlength = childlength * minlengthfactor;
		maxlength = childlength * maxlengthfactor;
		w = MAX(minwidth, width * widthfactor);
		a = amplitude * amplitudefactor;
		for (int b = 0; b < BranchCount; b++)
		{
			float angle, l;
			unsigned v;
			Matrix3D m0(m);
			angle = WWMath::Lerp(minbranchangle, maxbranchangle, _RandomNumber(0, randomness) * oorandomness);
			if ((b & 0x1) == 0) angle = -angle;
			l = WWMath::Lerp(minlength, maxlength, _RandomNumber(0, randomness) * oorandomness);
			v = (unsigned int)(MIN(((int)vertexcount) - 1, b * oobranchcount * (((int)vertexcount) - 1) + _RandomNumber(0, +branchrandomness)));
			m0.Translate(localvertex[v] - localvertex[0]);
			m0.Rotate_Z(angle);
			if (m0.Get_Translation().Z > 0.0f)
			{
				Branches[b].LightningBolt = new LightningBoltClass(branchcount2, m0, l, l, w, a);
			}
			else
			{
				Branches[b].LightningBolt = NULL;
			}
		}
	}
	Configure(white);
	delete[] localvertex;
	delete[] worldvertex;
}
LightningBoltClass::~LightningBoltClass()
{
	for (int b = 0; b < BranchCount; b++)
	{
		REF_PTR_RELEASE(Branches[b].LightningBolt);
	}
	if (Branches != NULL)
	{
		delete[] Branches;
	}
}
void LightningBoltClass::Set_Visibility(bool visible, bool recurse)
{
	VisibilityClass::Set_Visibility(visible);
	if (recurse)
	{
		for (int b = 0; b < BranchCount; b++)
		{
			if (Branches[b].LightningBolt != NULL)
			{
				Branches[b].LightningBolt->Set_Visibility(visible, recurse);
			}
		}
	}
}
void LightningBoltClass::Configure(Vector3 &color)
{
	Set_Color(color);
	for (int b = 0; b < BranchCount; b++)
	{
		if (Branches[b].LightningBolt != NULL)
		{
			Branches[b].LightningBolt->Configure(color);
		}
	}
}
void LightningBoltClass::Set_Transform(Matrix3D &t)
{
	((SegmentedLineClass*)this)->Set_Transform(t);
	for (int b = 0; b < BranchCount; b++)
	{
		if (Branches[b].LightningBolt != NULL)
		{
			Branches[b].LightningBolt->Set_Transform(t);
		}
	}
}
void LightningBoltClass::Render(RenderInfoClass &rinfo)
{
	if (VisibilityClass::Is_Visible())
	{
		if (!rinfo.Camera.Cull_Sphere(Get_Bounding_Sphere()))
		{
			Matrix4 t;
			DX8Wrapper::Get_Transform(D3DTS_WORLD, t);
			SegmentedLineClass::Render_Seg_Line(rinfo);
			DX8Wrapper::Set_Transform(D3DTS_WORLD, t);
		}
		for (int b = 0; b < BranchCount; b++)
		{
			if (Branches[b].LightningBolt != NULL)
			{
				Branches[b].LightningBolt->Render(rinfo);
			}
		}
	}
}
LightningClass::LightningClass(float extent, float startdistance, float enddistance, float heading, float distribution) : Time(0), PlayedThunder(false)
{
	const Vector3 white(1.0f, 1.0f, 1.0f);
	const unsigned randomness = 100;
	const float oorandomness = 1.0f / randomness;
	const unsigned minthunderdelaytime = 0;
	const unsigned maxthunderdelaytime = 1000;
	const float minlongitude = WWMATH_PI * 0.340f;
	const float	maxlongitude = WWMATH_PI * 0.472f;
	const float bufferangle = 0.35f;
	const int minbranchcount = 5;
	const int maxbranchcount = 8;
	const float	minwidth = 0.5f;
	const float	maxwidth = 1.1f;
	const float amplitude = 1.5f;
	const char *lightningsourcetexturename = "LightningSource.tga";
	const float	minlightningsourcewidth = 1.00f;
	const float	maxlightningsourcewidth = 3.00f;
	const float	minlightningsourceheight = 0.35f;
	const float maxlightningsourceheight = 1.05f;
	const unsigned majorsamplecount	= 2;
	const unsigned minorsamplecount = 3;
	static const char *_thundersamplename[majorsamplecount][minorsamplecount] = {
		{"Thunder01", "Thunder02", "Thunder03"},
		{"Thunder04", "Thunder05", "Thunder06"}
	};
	unsigned branchcount;
	float latitude, longitude, theta, x, y, z, length, childlength, width;
	Vector3	d, o;
	Matrix3D m;
	float lightningsourcewidth, lightningsourceheight;
	unsigned majorsampleindex, minorsampleindex;
	Distance = WWMath::Lerp(startdistance, enddistance, _RandomNumber(0, randomness) * oorandomness);
	ThunderDelayTime = (unsigned int)WWMath::Lerp((float) minthunderdelaytime, (float) maxthunderdelaytime, Distance);
	LightningGlow = new SkyGlowClass(extent);
	branchcount = (unsigned int)WWMath::Lerp((float) maxbranchcount, (float) minbranchcount, Distance);
	latitude = heading + (0.5f * WWMATH_PI) + (_RandomNumber(- ((int) randomness), ((int) randomness)) * oorandomness * WWMATH_PI * distribution);
	x = cosf(latitude);
	y = sinf(latitude);
	Direction.Set(x, y);
	d.Set(x, y, 0.0f);
	longitude = WWMath::Lerp(minlongitude, maxlongitude, Distance);
	x = sinf(longitude) * cosf(latitude);
	y = sinf(longitude) * sinf(latitude);
	z = cosf(longitude);
	o.Set(x, y, z);
	m.Look_At(o * extent, (o + d) * extent, 0.0f);
	theta = (WWMATH_PI * 0.5f) - longitude;
	childlength = extent * sinf(theta);
	length = childlength + extent * cosf(theta) * tanf(bufferangle);
	width = WWMath::Lerp(maxwidth, minwidth, Distance);
	LightningBolt = new LightningBoltClass(branchcount, m, length, childlength, width, amplitude);
	LightningSource = new SkyObjectClass(ShaderClass::_PresetAdditive2DShader);
	lightningsourcewidth = WWMath::Lerp(maxlightningsourcewidth,  minlightningsourcewidth,  Distance);
	lightningsourceheight = WWMath::Lerp(maxlightningsourceheight, minlightningsourceheight, Distance);
	LightningSource->Configure(o, extent, lightningsourcewidth, lightningsourceheight);
	LightningSource->Set_Texture(lightningsourcetexturename);
	ThunderPosition	= o * (extent * Distance);
	majorsampleindex = MIN((unsigned)(Distance * majorsamplecount), majorsamplecount - 1);
	minorsampleindex = _RandomNumber(0, minorsamplecount - 1);
	ThunderSampleName = _thundersamplename[majorsampleindex][minorsampleindex];
}
LightningClass::~LightningClass()
{
	delete LightningSource;
	REF_PTR_RELEASE(LightningBolt);
	delete LightningGlow;
}
bool LightningClass::Update(Matrix3D &t, Vector3 &additivecolor, SoundEnvironmentClass *soundenvironment)
{
	const Vector3 black(0.000f, 0.000f, 0.000f);
	const Vector3 blue(0.663f, 0.750f, 0.969f);
	const Vector3 gray(0.350f, 0.350f, 0.350f);
	const Vector3 white(1.000f, 1.000f, 1.000f);
	const unsigned lightningtime = 550;
	const float minglowintensity = 0.10f;
	const float	coldintensity = 0.25f;
	bool notfinished = true;
	LightningBolt->Set_Transform(t);
	Time += WW3D::Get_Frame_Time();
	if (Time < lightningtime)
	{
  		const unsigned phasecount = 7;
  		const static float _intensities[phasecount] = {1.00f, 0.75f, 0.50f, 0.50f, 1.00f, 0.50f, 0.10f};
  		const static bool _renderbranches[phasecount] = {true,  true,  true,  true,  false, false, false};
  		unsigned phase;
  		Vector3	color;
  		phase = (unsigned int)((((float) Time) / lightningtime) * (phasecount + 1));
  		phase = MIN(phase, phasecount);
  		additivecolor = blue * _intensities[phase] * MAX(1.0f - Distance, minglowintensity);
		LightningGlow->Configure(Direction, additivecolor, coldintensity);
		additivecolor *= coldintensity;
  		color = gray * _intensities[phase];
  		LightningBolt->Configure(color);
  		if (_renderbranches[phase])
		{
  			LightningBolt->Set_Visibility(true, true);
  		}
		else
		{
  			LightningBolt->Set_Visibility(false, true);
  			LightningBolt->Set_Visibility(true, false);
  		}
  		color = white * _intensities[phase];
  		LightningSource->Configure(color);
  		LightningSource->Set_Visibility(true);
  	}
	else
	{
  		additivecolor = black;
  		if (Time < lightningtime + ThunderDelayTime)
		{
  			LightningGlow->Set_Visibility(false);
  			LightningBolt->Set_Visibility(false, false);
  			LightningSource->Set_Visibility(false);
  		}
		else
		{
  			LightningGlow->Set_Visibility(false);
  			LightningBolt->Set_Visibility(false, false);
  			LightningSource->Set_Visibility(false);
  			if (!PlayedThunder)
			{
  				AudibleSoundClass *sound;
  				sound = WWAudioClass::Get_Instance()->Create_Sound(ThunderSampleName, NULL, 0, 4);
  				if (sound != NULL)
				{
  					Matrix3D m(t);
  					m.Adjust_Translation(ThunderPosition);
  					sound->Set_Transform(m);
  					sound->Add_To_Scene(true);
  					sound->Set_Volume(soundenvironment->Get_Amplitude() * 0.5f);
  					sound->Play(true);
  					sound->Release_Ref();
  				}
  				PlayedThunder = true;
  			}
  			notfinished = false;
  		}
  	}
	return notfinished;
}
void LightningClass::Render(RenderInfoClass &rinfo)
{
	LightningGlow->Render();
	LightningBolt->Render(rinfo);
	LightningSource->Render();
}
WarBlitzClass::WarBlitzClass (float extent, float startdistance, float enddistance, float heading, float distribution) : Time(0), PlayedSample(false)
{
	const unsigned randomness = 100;
	const float oorandomness = 1.0f / randomness;
	const unsigned minsampledelaytime = 25;
	const unsigned maxsampledelaytime = 400;
	float latitude, x, y;
	Vector2 position;
	Distance = WWMath::Lerp(startdistance, enddistance, _RandomNumber(0, randomness) * oorandomness);
	SampleDelayTime = (unsigned int)(WWMath::Lerp((float) minsampledelaytime, (float) maxsampledelaytime, Distance));
	latitude = heading + (0.5f * WWMATH_PI) + (_RandomNumber(- ((int) randomness), ((int) randomness)) * oorandomness * WWMATH_PI * distribution);
	x = cosf(latitude);
	y = sinf(latitude);
	Direction.Set(x, y);
	position = Direction * (extent * Distance);
	SamplePosition.Set(position.X, position.Y, 0.0f);
	WarBlitzGlow = new SkyGlowClass(extent);
}
WarBlitzClass::~WarBlitzClass()
{
	delete WarBlitzGlow;
}
bool WarBlitzClass::Update(Matrix3D &t, Vector3 &additivecolor)
{
	const Vector3 red(0.950f, 0.250f, 0.250f);
	const Vector3 black(0.000f, 0.000f, 0.000f);
	const unsigned warblitztime = 550;
	const float	minglowintensity = 0.10f;
	const float coldintensity = 0.25f;
	bool notfinished = true;
	Time += WW3D::Get_Frame_Time();
	if (Time < warblitztime)
	{
  		const unsigned phasecount = 7;
		const static float _intensities[phasecount] = {0.50f, 1.00f, 0.80f, 0.60f, 0.40f, 0.20f, 0.10f};
  		unsigned phase;
  		phase = (unsigned int)((((float) Time) / warblitztime) * (phasecount + 1));
  		phase = MIN(phase, phasecount);
  		additivecolor = red * _intensities[phase] * MAX(1.0f - Distance, minglowintensity);
		WarBlitzGlow->Configure(Direction, additivecolor, coldintensity);
  	}
	else
	{
		additivecolor = black;
		WarBlitzGlow->Set_Visibility(false);
		if (Time >= warblitztime + SampleDelayTime)
		{
			if (!PlayedSample)
			{
				const char *samplename = "SFX.Ambient_Explosion_01";
			 	AudibleSoundClass *sound;
  				sound = WWAudioClass::Get_Instance()->Create_Sound(samplename, NULL, 0, 4);
  				if (sound != NULL)
				{
  					Matrix3D m(t);
  					m.Adjust_Translation(SamplePosition);
  					sound->Set_Transform(m);
  					sound->Add_To_Scene(true);
  					sound->Play(true);
  					sound->Release_Ref();
  				}
  				PlayedSample = true;
			}
	  		notfinished = false;
		}
	}
	additivecolor *= coldintensity;
	return notfinished;
}
void WarBlitzClass::Render(RenderInfoClass &rinfo)
{
	WarBlitzGlow->Render();
}
SkyClass::SkyClass(SoundEnvironmentClass *soundenvironment) : Extent(100.0f),Color(0.0f, 0.0f, 0.0f),Hours(0),Minutes(0),Gloominess(0.0f),TintFactor(0.0f),LightningIntensity(0.0f),LightningStartDistance(0.0f),LightningEndDistance(1.0f),LightningHeading(0.0f),LightningDistribution(0.5f),SoundEnvironment(NULL),WarBlitzIntensity(0.0f),WarBlitzStartDistance(0.0f),WarBlitzEndDistance(1.0f),WarBlitzHeading(0.0f),WarBlitzDistribution(0.5f)
{
	const unsigned starcount = 200;
	REF_PTR_SET(SoundEnvironment, soundenvironment);
	SoundEnvironment->Add_User();
	Haze = new HazeClass(Extent);
	Starfield = new StarfieldClass(Extent, starcount);
	Sun = new SkyObjectClass(ShaderClass::_PresetAdditive2DShader);
	Sun->Set_Texture("Sun.tga");
	SunHalo = new SkyObjectClass(ShaderClass::_PresetAdditive2DShader);
	SunHalo->Set_Texture("SunHalo.tga");
	Moon = new SkyObjectClass(ShaderClass::_PresetAlpha2DShader);
	Set_Moon_Type(MOON_TYPE_FULL,false);
	MoonHalo = new SkyObjectClass(ShaderClass::_PresetAdditive2DShader);
	MoonHalo->Set_Texture("MoonHalo.tga");
	CloudLayer0 = new CloudLayerClass(Extent, "CloudLayer.tga", Vector2(0.0030f, 0.0006f), 1.2f, false);
	CloudLayer1 = new CloudLayerClass(Extent, "CloudLayer.tga", Vector2(0.0050f, 0.0010f), 0.8f, true);
	for (unsigned l = 0; l < LIGHTNING_COUNT; l++)
	{
		Lightning[l] = NULL;
		LightningCountdown[l] = Lightning_Delay();
	}
	WarBlitz = NULL;
	WarBlitzCountdown = War_Blitz_Delay();
}
SkyClass::~SkyClass()
{
	if (WarBlitz != NULL) delete WarBlitz;
	for (unsigned l = 0; l < LIGHTNING_COUNT; l++)
	{
		if (Lightning[l] != NULL) delete Lightning[l];
	}
	delete CloudLayer1;
	delete CloudLayer0;
	delete MoonHalo;
	delete Moon;
	delete SunHalo;
	delete Sun;
	delete Starfield;
	delete Haze;
	SoundEnvironment->Remove_User();
	REF_PTR_RELEASE(SoundEnvironment);
}
void SkyClass::Set_Light_Direction(const Vector3 &sundirection, const Vector3 &moondirection)
{
	const float	sunradius = 30.00f;
	const float	sunhaloradius = 85.00f;
	const float	moonradius = 5.00f;
	const float	moonhaloradius = 20.00f;
	Starfield->Configure(moondirection, Extent, moonradius);
	Sun->Configure(sundirection, Extent, sunradius);
	SunHalo->Configure(sundirection, Extent, sunhaloradius);
	Moon->Configure(moondirection, Extent, moonradius);
	MoonHalo->Configure(moondirection, Extent, moonhaloradius);
	CloudLayer0->Configure(sundirection);
	CloudLayer1->Configure(sundirection);
}
void SkyClass::Set_Time_Of_Day(unsigned hours, unsigned minutes)
{
	const Vector3 white(1.00f, 1.00f, 1.00f);
	const Vector3 starfieldcolor(0.27f, 0.31f, 0.37f);
	const unsigned colorcount = 24;
	const unsigned alphacount = 24;
	static const unsigned char _warmskycolors[colorcount][3] = {
	{ 36,  36,  40},
	{ 36,  36,  40},
	{ 36,  36,  40},
	{ 36,  36,  40},
	{ 51,  83, 100},
	{168, 160, 216},
	{240, 212, 216},
	{255, 255, 192},
	{255, 255, 248},
	{255, 255, 248},
	{255, 255, 248},
	{255, 255, 248},
	{255, 255, 248},
	{255, 255, 248},
	{255, 255, 248},
	{255, 255, 248},
	{245, 243, 200},
	{245, 209, 121},
	{229, 188,  88},
	{201, 103,  60},
	{ 72,  72,  80},
	{ 36,  36,  40},
	{ 36,  36,  40},
	{ 36,  36,  40}};
	static const unsigned char _coldskycolors[colorcount][3] = {
	{ 36,  36,  40},
	{ 36,  36,  40},
	{ 36,  36,  40},
	{ 36,  36,  40},
	{ 51,  83, 100},
	{170, 164, 205},
	{200, 179, 182},
	{215, 215, 181},
	{215, 215, 209},
	{215, 215, 209},
	{215, 215, 209},
	{215, 215, 209},
	{215, 215, 209},
	{215, 215, 209},
	{215, 215, 209},
	{215, 215, 209},
	{210, 209, 176},
	{213, 190, 135},
	{214, 187, 123},
	{179, 123,  98},
	{ 72,  72,  80},
	{ 36,  36,  40},
	{ 36,  36,  40},
	{ 36,  36,  40}};
	static const unsigned char _suncolors[colorcount][3] = {
	{  0,   0,   0},
	{  0,   0,   0},
	{  0,   0,   0},
	{  0,   0,   0},
	{  0,   0,   0},
	{127, 127,   0},
	{127, 127,   0},
	{110, 110,   0},
	{ 96,  96,  32},
	{ 76,  76,  76},
	{ 76,  76,  76},
	{ 76,  76,  76},
	{ 76,  76,  76},
	{ 76,  76,  76},
	{ 76,  76,  76},
	{ 76,  76,  76},
	{ 96,  81,  48},
	{123,  81,  21},
	{127,  32,  32},
	{127,  32,  32},
	{  0,   0,   0},
	{  0,   0,   0},
	{  0,   0,   0},
	{  0,   0,   0}};
	static const unsigned char _sunhalocolors[colorcount][3] = {
	{  0,   0,   0},
	{  0,   0,   0},
	{  0,   0,   0},
	{  0,   0,   0},
	{  0,   0,   0},
	{192, 192, 144},
	{192, 192, 144},
	{192, 192, 144},
	{ 96,  96,  96},
	{ 96,  96,  96},
	{ 96,  96,  96},
	{ 96,  96,  96},
	{ 96,  96,  96},
	{ 96,  96,  96},
	{ 96,  96,  96},
	{ 96,  96,  96},
	{ 96,  96,  96},
	{245, 223, 185},
	{239, 179, 107},
	{239, 179, 107},
	{  0,   0,   0},
	{  0,   0,   0},
	{  0,   0,   0},
	{  0,   0,   0}};
	static const unsigned char _moonhalocolors[colorcount][3] = {
	{192, 192, 192},
	{192, 192, 192},
	{192, 192, 192},
	{192, 192, 192},
	{160, 160, 160},
	{128, 128, 128},
	{ 64,  64,  64},
	{  0,   0,   0},
	{  0,   0,   0},
	{  0,   0,   0},
	{  0,   0,   0},
	{  0,   0,   0},
	{  0,   0,   0},
	{  0,   0,   0},
	{  0,   0,   0},
	{  0,   0,   0},
	{  0,   0,   0},
	{  0,   0,   0},
	{ 64,  64,  64},
	{128, 128, 128},
	{160, 160, 160},
	{192, 192, 192},
	{192, 192, 192},
	{192, 192, 192}};
	static const unsigned char _starfieldalphas[alphacount] = {
	255,
	255,
	255,
	255,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	255,
	255,
	255,
	255};
	float timeofday;
	Vector3 warmskycolor, coldskycolor, sunhalocolor, moonhalocolor, cloudcolor, suncolor;
	float starfieldalpha;
	Hours = hours;
	Minutes = minutes;
	Set_Color();
	timeofday = hours + (minutes / 60.0f);
	starfieldalpha = Interpolate_Scalar(_starfieldalphas, alphacount, timeofday);
	warmskycolor = Interpolate_Color(_warmskycolors, colorcount, timeofday);
	coldskycolor = Interpolate_Color(_coldskycolors, colorcount, timeofday);
	sunhalocolor = Interpolate_Color(_sunhalocolors, colorcount, timeofday);
	moonhalocolor = Interpolate_Color(_moonhalocolors, colorcount, timeofday);
	suncolor = Interpolate_Color(_suncolors, colorcount, timeofday);
	Haze->Configure(Get_Color(), coldskycolor);
	Starfield->Configure(starfieldcolor, white, starfieldalpha);
	Sun->Configure(suncolor);
	SunHalo->Configure(sunhalocolor);
	Moon->Configure(white);
	MoonHalo->Configure(moonhalocolor);
	CloudLayer0->Configure(warmskycolor, coldskycolor);
	CloudLayer1->Configure(warmskycolor, coldskycolor);
}
void SkyClass::Set_Clouds(float cloudcover, float gloominess)
{
	const float layer0minintensity = 0.60f;
	const float layer0maxintensity = 0.95f;
	const float layer1minintensity = 0.20f;
	const float layer1maxintensity = 0.90f;
	float layer0alpha, layer1alpha, layer0intensity, layer1intensity;
	Gloominess = gloominess;
	Set_Color();
	if (cloudcover <= 0.5f)
	{
		layer0alpha = cloudcover * 2.0f;
		layer1alpha = 0.0f;
	}
	else
	{
		layer0alpha = 1.0f;
		layer1alpha = (cloudcover - 0.5f) * 2.0f;
	}
	layer0alpha = log10 (cloudcover * 9.0f + 1.0f);
	layer1alpha = cloudcover * cloudcover * cloudcover;
	layer0intensity = layer0maxintensity - gloominess * (layer0maxintensity - layer0minintensity);
	layer1intensity = layer1maxintensity - gloominess * (layer1maxintensity - layer1minintensity);
	Haze->Configure(Get_Color(), layer0intensity);
	CloudLayer0->Configure(layer0alpha, layer0intensity, layer0intensity);
	CloudLayer1->Configure(layer1alpha, layer1intensity, layer0intensity);
}
void SkyClass::Set_Tint_Factor(float tintfactor)
{
	TintFactor = tintfactor;
	Set_Color();
	Haze->Configure(Get_Color());
}
void SkyClass::Set_Color()
{
	const Vector3 tintcolor(1.0f, 0.0f, 0.0f);
	const unsigned colorcount = 24;
	static const unsigned char _skycolors[colorcount][3] = {
	{  4,  12,  18},
	{	4,  12,  18},
	{	4,  12,  18},
	{	4,  12,  18},
	{  8,  24,  40},
	{ 72,  92, 136},
	{120, 120, 128},
	{128, 152, 168},
	{112, 148, 168},
	{112, 148, 176},
	{112, 148, 176},
	{112, 148, 176},
	{112, 148, 176},
	{112, 148, 176},
	{112, 148, 176},
	{112, 146, 176},
	{128, 160, 176},
	{120, 148, 152},
	{160, 132, 112},
	{ 72,  88,  88},
	{  8,  24,  36},
	{	4,  12,  18},
	{	4,  12,  18},
	{	4,  12,  18}};
	static const unsigned char _gloominesscolors[colorcount][3] = {
	{  4,  12,  18},
	{	4,  12,  18},
	{	4,  12,  18},
	{	4,  12,  18},
	{  8,  24,  40},
	{ 72,  92, 136},
	{192, 192, 192},
	{192, 192, 192},
	{192, 192, 192},
	{192, 192, 192},
	{192, 192, 192},
	{192, 192, 192},
	{192, 192, 192},
	{192, 192, 192},
	{192, 192, 192},
	{192, 192, 192},
	{192, 192, 192},
	{192, 192, 192},
	{192, 192, 192},
	{ 72,  88,  88},
	{  8,  24,  36},
	{	4,  12,  18},
	{	4,  12,  18},
	{	4,  12,  18}};
	float timeofday, interpolant;
	Vector3 color;
	timeofday = Hours + (Minutes / 60.0f);
	interpolant = log10(Gloominess * 9.0f + 1.0f);
	Vector3::Lerp(Interpolate_Color(_skycolors, colorcount, timeofday), Interpolate_Color(_gloominesscolors, colorcount, timeofday), interpolant, &color);
	Vector3::Lerp(color, tintcolor, TintFactor, &Color);
}
void SkyClass::Set_Moon_Type(MoonTypeEnum moontype,bool earth)
{
	const char *moontextures[MOON_TYPE_COUNT] = {"FullMoon.tga", "PartMoon.tga"};
	const char *earthtextures[MOON_TYPE_COUNT] = {"FullEarth.tga", "PartEarth.tga"};
	if (!earth)
	{
		Moon->Set_Texture(moontextures[moontype]);
	}
	else
	{
		Moon->Set_Texture(earthtextures[moontype]);
	}
}
Vector3 SkyClass::Interpolate_Color(const unsigned char colortable[][3], unsigned colorcount, float interpolant)
{
	const float ooucharmax = 1.0f / UCHAR_MAX;
	unsigned lowerindex, upperindex;
	Vector3 lowercolor, uppercolor;
	float alpha;
	Vector3 color;
	interpolant = MIN(MAX(0.0f, interpolant), colorcount);
	if (interpolant == colorcount) interpolant = 0.0f;
	lowerindex = (unsigned int)floorf(interpolant);
	upperindex = (lowerindex + 1) % colorcount;
	lowercolor = Vector3(colortable[lowerindex][0] * ooucharmax, colortable[lowerindex][1] * ooucharmax, colortable[lowerindex][2] * ooucharmax);
	uppercolor = Vector3(colortable[upperindex][0] * ooucharmax, colortable[upperindex][1] * ooucharmax, colortable[upperindex][2] * ooucharmax);
	alpha = interpolant - lowerindex;
	Vector3::Lerp(lowercolor, uppercolor, alpha, &color);
	return color;
}
float SkyClass::Interpolate_Scalar(const unsigned char scalartable[], unsigned scalarcount, float interpolant)
{
	const float ooucharmax = 1.0f / UCHAR_MAX;
	unsigned lowerindex, upperindex;
	float lowervalue, uppervalue, alpha;
	interpolant = MIN(MAX(0.0f, interpolant), scalarcount);
	if (interpolant == scalarcount) interpolant = 0.0f;
	lowerindex = (unsigned int)floorf (interpolant);
	upperindex = (lowerindex + 1) % scalarcount;
	lowervalue = scalartable[lowerindex] * ooucharmax;
	uppervalue = scalartable[upperindex] * ooucharmax;
	alpha = interpolant - lowerindex;
	return lowervalue + ((uppervalue - lowervalue) * alpha);
}
unsigned SkyClass::Lightning_Delay()
{
	const unsigned maxdelay = 240000;
	return _RandomNumber(1, maxdelay);
}
unsigned SkyClass::War_Blitz_Delay()
{
	const unsigned maxdelay = 240000;
	return _RandomNumber(1, maxdelay);
}
void SkyClass::Update(SceneClass *mainscene, const Vector3 &cameraposition)
{
	const unsigned timescale = 40;
	const Vector3 white(1.0f, 1.0f, 1.0f);
	unsigned time;
	Vector3	colorsum, fogcolor;
	int ticks;
	Matrix3D t = Matrix3D(cameraposition);
	Set_Transform(t);
	time = WW3D::Get_Frame_Time();
	colorsum.Set(0.0f, 0.0f, 0.0f);
	for (unsigned l = 0; l < LIGHTNING_COUNT; l++)
	{
		ticks = ((int)(LightningIntensity * timescale)) * time;
		LightningCountdown[l] -= ticks;
		if (LightningCountdown[l] <= 0)
		{
			LightningCountdown[l] = Lightning_Delay();
			if (Lightning[l] == NULL)
			{
				if (LightningIntensity > 0.0f)
				{
					Lightning[l] = new LightningClass(Extent, LightningStartDistance, LightningEndDistance, LightningHeading, LightningDistribution);
				}
			}
		}
		if (Lightning [l] != NULL)
		{
			Vector3 lightningcolor;
			if (!Lightning [l]->Update (t, lightningcolor, SoundEnvironment))
			{
				delete Lightning[l];
				Lightning [l] = NULL;
			}
			else
			{
				colorsum += lightningcolor;
			}
		}
	}
	ticks = ((int)(WarBlitzIntensity * timescale)) * time;
	WarBlitzCountdown -= ticks;
	if (WarBlitzCountdown <= 0)
	{
		WarBlitzCountdown = War_Blitz_Delay();
		if (WarBlitz == NULL)
		{
			if (WarBlitzIntensity > 0.0f)
			{
				WarBlitz = new WarBlitzClass(Extent, WarBlitzStartDistance, WarBlitzEndDistance, WarBlitzHeading, WarBlitzDistribution);
			}
		}
	}
	if (WarBlitz != NULL)
	{
		Vector3 warblitzcolor;
		if (!WarBlitz->Update(t, warblitzcolor))
		{
			delete WarBlitz;
			WarBlitz = NULL;
		}
		else
		{
			colorsum += warblitzcolor;
		}
	}
	fogcolor = Haze->Get_Horizon_Color() + colorsum;
	fogcolor.Update_Min(white);
	mainscene->Set_Fog_Color(fogcolor);
}
void SkyClass::Render(RenderInfoClass &rinfo)
{
	Matrix4 m2(Transform);
	DX8Wrapper::Set_Transform(D3DTS_WORLD,m2);
	Haze->Render();
	Starfield->Render();
	CloudLayer0->Render();
	MoonHalo->Render();
	Moon->Render();
	SunHalo->Render();
	CloudLayer1->Render();
	Sun->Render();
	for (unsigned l = 0; l < LIGHTNING_COUNT; l++)
	{
		if (Lightning [l] != NULL) Lightning [l]->Render(rinfo);
	}
	if (WarBlitz != NULL) WarBlitz->Render(rinfo);
}
void SkyClass::Get_Obj_Space_Bounding_Sphere(SphereClass &sphere) const
{
	const Vector3 position(0.0f, 0.0f, 0.0f);
	const Vector3 extent(Extent, Extent, Extent);
	sphere.Init(position, extent.Length());
}
void SkyClass::Get_Obj_Space_Bounding_Box(AABoxClass &box) const
{
	const Vector3 position(0.0f, 0.0f, 0.0f);
	const Vector3 extent(Extent, Extent, Extent);
	box.Init (position, extent);
}
void BackgroundParameterClass::Initialize()
{
	CurrentValue = 0.0f;
	NormalTarget = 0.0f;
	NormalDuration = 0.0f;
	OverrideTarget = 0.0f;
	OverrideDuration = 0.0f;
}
void BackgroundParameterClass::Set(float target, float ramptime, bool override)
{
	if (!override)
	{
		NormalTarget = target;
		NormalDuration = ramptime;
	}
	else
	{
		OverrideTarget = target;
		OverrideDuration = ramptime;
	}
}
bool BackgroundParameterClass::Update(float time, bool override)
{
	const float previouscurrentvalue = CurrentValue;
	Update(NormalValue, NormalTarget, NormalDuration, time);
	if (override)
	{
		Update(CurrentValue, OverrideTarget, OverrideDuration, time);
	}
	else
	{
		if (OverrideDuration > 0.0f)
		{
			Update(CurrentValue, NormalValue, OverrideDuration, time);
		}
		else
		{
			CurrentValue = NormalValue;
		}
	}
	return CurrentValue != previouscurrentvalue;
}
void BackgroundParameterClass::Update(float &value, float &target, float &duration, float time)
{
	if (value == target)
	{
		duration = 0.0f;
	}
	else
	{
		duration -= time;
		if (duration > 0.0f)
		{
			bool sign0, sign1;
			sign0 = value < target;
			value += ((target - value) * (time / duration));
			if (value == target)
			{
				duration = 0.0f;
			}
			else
			{
				sign1 = value < target;
				// If the value has 'blown past' the target value...
				if (sign0 ^ sign1)
				{
					duration = 0.0f;
					value = target;
				}
			}
		}
		else
		{
			duration = 0.0f;
			value = target;
		}
	}
}
bool BackgroundMgrClass::Is_Dirty()
{
	return _Dirty;
}
void BackgroundMgrClass::Set_Dirty(bool dirty)
{
	_Dirty = dirty;
}
bool BackgroundMgrClass::Set_Time_Of_Day(unsigned hours, unsigned minutes)
{
	if ((hours >= 0) && (hours <= 23) && (minutes >= 0) && (minutes <= 59))
	{
		_Hours	= hours;
		_Minutes = minutes;
		Set_Dirty();
		return true;
	}
	else
	{
		return false;
	}
}
void BackgroundMgrClass::Get_Time_Of_Day(unsigned &hours, unsigned &minutes)
{
	hours = _Hours;
	minutes = _Minutes;
}
void BackgroundMgrClass::Set_Light_Source_Type(LightSourceTypeEnum lightsourcetype)
{
	_LightSourceType = lightsourcetype;
	Set_Dirty();
}
BackgroundMgrClass::LightSourceTypeEnum BackgroundMgrClass::Get_Light_Source_Type()
{
	return _LightSourceType;
}
void BackgroundMgrClass::Set_Moon_Type(SkyClass::MoonTypeEnum moontype)
{
	_MoonType = moontype;
	Set_Dirty();
}
SkyClass::MoonTypeEnum BackgroundMgrClass::Get_Moon_Type()
{
	return _MoonType;
}
Vector3 BackgroundMgrClass::Get_Clear_Color()
{
	if (_Sky != NULL)
	{
			return _Sky->Get_Color();
	}
	else
	{
		return Vector3(0.0f, 0.0f, 0.0f);
	}
}
void BackgroundMgrClass::Get_Clouds(float &cloudcover, float &gloominess)
{
	cloudcover = _Parameters[PARAMETER_CLOUD_COVER].Value();
	gloominess = _Parameters[PARAMETER_CLOUD_GLOOMINESS].Value();
}
void BackgroundMgrClass::Update(PhysicsSceneClass *mainscene, CameraClass *camera)
{
	const float minlensflareintensity = 0.10f;
	const float maxlensflareintensity = 0.50f;
	float time;
	bool cloudsmodified, skytintmodified, lightningmodified, warblitzmodified;
	Vector3 lightvector;
	float znear, zfar;
	Vector3 dazzlecolor;
	float cloudcover, gloominess;
	float intensity, dazzleintensity, lensflareintensity;
	if (_Sky == NULL) return;
	if (Is_Dirty())
	{
		_Sky->Set_Time_Of_Day(_Hours, _Minutes);
		_Sky->Set_Moon_Type(_MoonType,MoonIsEarth);
	}
	time = WW3D::Get_Frame_Time() * 0.001f;
	cloudsmodified = _Parameters[PARAMETER_CLOUD_COVER].Update(time, _CloudOverrideCount > 0);
	cloudsmodified |= _Parameters[PARAMETER_CLOUD_GLOOMINESS].Update(time, _CloudOverrideCount > 0);
  	if (Is_Dirty() || cloudsmodified)
	{
  		_Sky->Set_Clouds(_Parameters[PARAMETER_CLOUD_COVER].Value(), _Parameters[PARAMETER_CLOUD_GLOOMINESS].Value());
	}
	skytintmodified = _Parameters[PARAMETER_SKY_TINT_FACTOR].Update(time, _SkyTintOverrideCount > 0);
	if (Is_Dirty() || skytintmodified)
	{
		_Sky->Set_Tint_Factor(_Parameters[PARAMETER_SKY_TINT_FACTOR].Value());
	}
  	lightningmodified = _Parameters[PARAMETER_LIGHTNING_INTENSITY].Update(time, _LightningOverrideCount > 0);
	lightningmodified |= _Parameters[PARAMETER_LIGHTNING_START_DISTANCE].Update(time, _LightningOverrideCount > 0);
	lightningmodified |= _Parameters[PARAMETER_LIGHTNING_END_DISTANCE].Update(time, _LightningOverrideCount > 0);
	lightningmodified |= _Parameters[PARAMETER_LIGHTNING_HEADING].Update(time, _LightningOverrideCount > 0);
	lightningmodified |= _Parameters[PARAMETER_LIGHTNING_DISTRIBUTION].Update(time, _LightningOverrideCount > 0);
	if (Is_Dirty() || lightningmodified)
	{
 		_Sky->Set_Lightning(_Parameters[PARAMETER_LIGHTNING_INTENSITY].Value(), _Parameters[PARAMETER_LIGHTNING_START_DISTANCE].Value(), _Parameters[PARAMETER_LIGHTNING_END_DISTANCE].Value(), _Parameters[PARAMETER_LIGHTNING_HEADING].Value(), _Parameters[PARAMETER_LIGHTNING_DISTRIBUTION].Value());
  	}
	warblitzmodified = _Parameters[PARAMETER_WAR_BLITZ_INTENSITY].Update(time, false);
	warblitzmodified |= _Parameters[PARAMETER_WAR_BLITZ_START_DISTANCE].Update(time, false);
	warblitzmodified |= _Parameters[PARAMETER_WAR_BLITZ_END_DISTANCE].Update(time, false);
	warblitzmodified |= _Parameters[PARAMETER_WAR_BLITZ_HEADING].Update(time, false);
	warblitzmodified |= _Parameters[PARAMETER_WAR_BLITZ_DISTRIBUTION].Update(time, false);
	if (Is_Dirty() || warblitzmodified)
	{
 		_Sky->Set_War_Blitz(_Parameters[PARAMETER_WAR_BLITZ_INTENSITY].Value(), _Parameters[PARAMETER_WAR_BLITZ_START_DISTANCE].Value(), _Parameters[PARAMETER_WAR_BLITZ_END_DISTANCE].Value(), _Parameters[PARAMETER_WAR_BLITZ_HEADING].Value(), _Parameters[PARAMETER_WAR_BLITZ_DISTRIBUTION].Value());
	}
  	mainscene->Get_Sun_Light_Vector(&lightvector);
  	if (Is_Dirty() || (_LightVector != lightvector))
	{
  		_LightVector = lightvector;
  		_UnitLightVector = Normalize(-lightvector);
  		if (_LightSourceType == LIGHT_SOURCE_TYPE_MOON)
		{
  			_UnitLightVector = -_UnitLightVector;
  		}
  		_Sky->Set_Light_Direction(_UnitLightVector, -_UnitLightVector);
  	}
	// FIXME camera->Get_Position produces incorrect results for the two custom camera types used by water
  	// The result right now is that the sky surface moves as you do instead of staying stationary
	_Sky->Update(mainscene, camera->Get_Position());
  	camera->Get_Clip_Planes(znear, zfar);
	if (_LensFlareVisible)
	{
  		_Dazzle->Set_Transform(Matrix3D(camera->Get_Position() + _UnitLightVector * (zfar - 1.0f)));
		Get_Clouds(cloudcover, gloominess);
		intensity = 1.0f - (0.5f * (cloudcover + gloominess));
		dazzleintensity = (0.75f * intensity) + 0.25f;
		dazzlecolor = _Sky->Get_Sun()->Get_Color() * dazzleintensity;
		if (intensity < minlensflareintensity)
		{
			lensflareintensity = 0.0f;
		}
		else
		{
	  		lensflareintensity = MIN(maxlensflareintensity, intensity);
		}
		_Dazzle->Set_Dazzle_Color(dazzlecolor);
		_Dazzle->Set_Lensflare_Intensity(lensflareintensity);
	}
	else
	{
		_Dazzle->Set_Lensflare_Intensity(0);
		// Note: DazzleRenderObjectClass does not respect Set_Visible...
	}
	Set_Dirty(false);
}
