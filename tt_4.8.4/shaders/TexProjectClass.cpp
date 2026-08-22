#include "General.h"
#include "TexProjectClass.h"
#include "WW3DAssetManager.h"
#include "RenderObjClass.h"
#include "DX8Wrapper.h"
#include "RenderInfoClass.h"
#include "WW3D.h"
#include "CameraClass.h"
const float INTENSITY_RATE_OF_CHANGE			= 1.0f;
TexProjectClass::TexProjectClass(void) :
	Flags(DEFAULT_FLAGS),
	DesiredIntensity(1.0f),
	Intensity(1.0f),
	Attenuation(1.0f),
	MaterialPass(NULL),
	Mapper1(NULL),
	RenderTarget(NULL),
	HFov(90.0f),
	VFov(90.0f),
	XMin(-10.0f),
	XMax(10.0f),
	YMin(-10.0f),
	YMax(10.0f),
	ZNear(1.0f),
	ZFar(1000.0f)
{
	MaterialPass = new MaterialPassClass(); 
	MaterialPass->CullVolume = &WorldBoundingVolume;
	VertexMaterialClass * vmtl = new VertexMaterialClass();
	vmtl->Set_Mapper(Mapper);
	MaterialPass->Set_Material(vmtl);
	vmtl->Release_Ref();
	vmtl = NULL;
	Init_Multiplicative();
}

TexProjectClass::~TexProjectClass(void)
{
	REF_PTR_RELEASE(Mapper1);
	REF_PTR_RELEASE(MaterialPass);
	REF_PTR_RELEASE(RenderTarget);
}

void TexProjectClass::Set_Texture_Size(int size)
{
	Flags &= ~SIZE_MASK;
	Flags |= (size << SIZE_SHIFT);
}

int TexProjectClass::Get_Texture_Size(void)
{
	return (Flags & SIZE_MASK) >> SIZE_SHIFT;
}

void TexProjectClass::Set_Flag(uint32 flag,bool onoff)	
{ 
	if (onoff) { 
		Flags |= flag; 
	} else { 
		Flags &= ~flag; 
	} 
}

bool TexProjectClass::Get_Flag(uint32 flag) const
{ 
	return (Flags & flag) == flag; 
}

void TexProjectClass::Set_Intensity(float intensity,bool immediate)
{
	DesiredIntensity = intensity;
	if (immediate) {
		Intensity = DesiredIntensity;
	}
}

float TexProjectClass::Get_Intensity(void)
{
	return DesiredIntensity;
}

bool TexProjectClass::Is_Intensity_Zero(void)
{
	return ((Intensity == 0.0f) && (DesiredIntensity == 0.0f));
}

void TexProjectClass::Set_Attenuation(float attenuation)
{
	attenuation = attenuation;
}

float TexProjectClass::Get_Attenuation(void)
{
	return Attenuation;
}

void TexProjectClass::Enable_Attenuation(bool onoff)
{
	Set_Flag(ATTENUATE,onoff);
}

bool TexProjectClass::Is_Attenuation_Enabled(void)
{
	return Get_Flag(ATTENUATE);
}

void TexProjectClass::Enable_Depth_Gradient(bool onoff)
{
	Set_Flag(USE_DEPTH_GRADIENT,onoff);
	if (Get_Flag(ADDITIVE)) {
		Init_Additive();
	} else {
		Init_Multiplicative();
	}
}

bool TexProjectClass::Is_Depth_Gradient_Enabled(bool onoff)
{
	return Get_Flag(USE_DEPTH_GRADIENT);
}

void TexProjectClass::Init_Multiplicative(void)
{	
	Set_Flag(ADDITIVE,false);
	static ShaderClass mult_shader(		SHADE_CNST(	ShaderClass::PASS_LEQUAL,
																	ShaderClass::DEPTH_WRITE_DISABLE,
																	ShaderClass::COLOR_WRITE_ENABLE,
																	ShaderClass::SRCBLEND_ZERO,
																	ShaderClass::DSTBLEND_SRC_COLOR,
																	ShaderClass::FOG_DISABLE,
																	ShaderClass::GRADIENT_ADD,
																	ShaderClass::SECONDARY_GRADIENT_DISABLE,
																	ShaderClass::TEXTURING_ENABLE,
																	ShaderClass::ALPHATEST_DISABLE,
																	ShaderClass::CULL_MODE_ENABLE,
																	0,
																	0) );
	if (WW3DAssetManager::TheInstance->Get_Activate_Fog_On_Load()) {
		mult_shader.Enable_Fog ("TexProjectClass");
	}
	if (Get_Flag(USE_DEPTH_GRADIENT)) {
		mult_shader.Set_Post_Detail_Color_Func(ShaderClass::DETAILCOLOR_ADD);
		TextureClass * grad_tex = WW3DAssetManager::TheInstance->Get_Texture("MultProjectorGradient.tga",TextureClass::MIP_LEVELS_ALL,WW3D_FORMAT_UNKNOWN,true);
		if (grad_tex) {
			grad_tex->UAddressMode = TextureClass::TEXTURE_ADDRESS_CLAMP;
			grad_tex->VAddressMode = TextureClass::TEXTURE_ADDRESS_CLAMP;
			MaterialPass->Set_Texture(grad_tex,1);
			grad_tex->Release_Ref();
		}
	} else {
		mult_shader.Set_Post_Detail_Color_Func(ShaderClass::DETAILCOLOR_DISABLE);
		MaterialPass->Set_Texture(NULL,1);
	}
	MaterialPass->Set_Shader(mult_shader);
	VertexMaterialClass * vmtl = MaterialPass->Material;
	vmtl->Set_Ambient(0,0,0);
	vmtl->Set_Diffuse(0,0,0);
	vmtl->Set_Specular(0,0,0);
	vmtl->Set_Emissive(0.0f,0.0f,0.0f);
	vmtl->Set_Opacity(1.0f);
	vmtl->Set_Lighting(true);
	if (Get_Flag(USE_DEPTH_GRADIENT)) {
		if (Mapper1 == NULL) {
			Mapper1 = new MatrixMapperClass(1);
		}
		Mapper1->Set_Type(MatrixMapperClass::DEPTH_GRADIENT);
		vmtl->Set_Mapper(Mapper1,1);
	} else {
		vmtl->Set_Mapper(NULL,1);
	}
}

void TexProjectClass::Init_Additive(void)
{
	Set_Flag(ADDITIVE,true);
	static ShaderClass add_shader(		SHADE_CNST(	ShaderClass::PASS_LEQUAL,
																	ShaderClass::DEPTH_WRITE_DISABLE,
																	ShaderClass::COLOR_WRITE_ENABLE,
																	ShaderClass::SRCBLEND_ONE,
																	ShaderClass::DSTBLEND_ONE,
																	ShaderClass::FOG_DISABLE,
																	ShaderClass::GRADIENT_MODULATE,
																	ShaderClass::SECONDARY_GRADIENT_DISABLE,
																	ShaderClass::TEXTURING_ENABLE,
																	ShaderClass::ALPHATEST_DISABLE,
																	ShaderClass::CULL_MODE_ENABLE,
																	ShaderClass::DETAILCOLOR_DISABLE,
																	ShaderClass::DETAILALPHA_DISABLE) );
	if (WW3DAssetManager::TheInstance->Get_Activate_Fog_On_Load()) {
		add_shader.Enable_Fog ("TexProjectClass");
	}
	add_shader.Set_Post_Detail_Color_Func(ShaderClass::DETAILCOLOR_SCALE);
	TextureClass * grad_tex = WW3DAssetManager::TheInstance->Get_Texture("AddProjectorGradient.tga",TextureClass::MIP_LEVELS_ALL,WW3D_FORMAT_UNKNOWN,true);
	if (grad_tex) {
		grad_tex->UAddressMode = TextureClass::TEXTURE_ADDRESS_CLAMP;
		grad_tex->VAddressMode = TextureClass::TEXTURE_ADDRESS_CLAMP;
		MaterialPass->Set_Texture(grad_tex,1);
		grad_tex->Release_Ref();
	}
	MaterialPass->Set_Shader(add_shader);
	VertexMaterialClass * vmtl = MaterialPass->Material;
	vmtl->Set_Ambient(0,0,0);
	vmtl->Set_Diffuse(0,0,0);
	vmtl->Set_Specular(0,0,0);
	vmtl->Set_Emissive(1,1,1);
	vmtl->Set_Opacity(1.0f);
	vmtl->Set_Lighting(true);
	if (Mapper1 == NULL) {
		Mapper1 = new MatrixMapperClass(1);
	}
	Mapper1->Set_Type(MatrixMapperClass::NORMAL_GRADIENT);
	vmtl->Set_Mapper(Mapper1,1);
}

void TexProjectClass::Set_Texture(TextureClass * texture)
{
	if (texture != NULL) {
		texture->UAddressMode = TextureClass::TEXTURE_ADDRESS_CLAMP;
		texture->VAddressMode = TextureClass::TEXTURE_ADDRESS_CLAMP;	
		MaterialPass->Set_Texture(texture,0);
	}
}

TextureClass * TexProjectClass::Get_Texture(void) const
{
	return MaterialPass->Get_Texture();
}

TextureClass * TexProjectClass::Peek_Texture(void) const
{
	return MaterialPass->Peek_Texture();
}

MaterialPassClass * TexProjectClass::Peek_Material_Pass(void) 
{
	return MaterialPass;
}

void TexProjectClass::Set_Perspective_Projection(float hfov,float vfov,float znear,float zfar)
{
	HFov = hfov;
	VFov = vfov;
	ZNear = znear;
	ZFar = zfar;
	ProjectorClass::Set_Perspective_Projection(hfov,vfov,znear,zfar);
	Set_Flag(PERSPECTIVE,true);
}

void TexProjectClass::Set_Ortho_Projection(float xmin,float xmax,float ymin,float ymax,float znear,float zfar)
{
	XMin = xmin;
	XMax = xmax;
	YMin = ymin;
	YMax = ymax;
	ZNear = znear;
	ZFar = zfar;
	ProjectorClass::Set_Ortho_Projection(xmin,xmax,ymin,ymax,znear,zfar);
	Set_Flag(PERSPECTIVE,false);
}

bool TexProjectClass::Compute_Perspective_Projection
(
	RenderObjClass *	model,
	const Vector3 &	lightpos,
	float					znear,
	float					zfar
)
{
	if (model == NULL) {
		return false;
	}
	AABoxClass box;
	model->Get_Obj_Space_Bounding_Box(box);
	const Matrix3D & tm = model->Get_Transform();
	return Compute_Perspective_Projection(box,tm,lightpos,znear,zfar);
}

bool TexProjectClass::Compute_Ortho_Projection
(
	RenderObjClass *	model,
	const Vector3 &	lightdir,
	float					znear,
	float					zfar
)
{
	if (model == NULL) {
		return false;
	}
	AABoxClass box;
	model->Get_Obj_Space_Bounding_Box(box);
	const Matrix3D & tm = model->Get_Transform();
	return Compute_Ortho_Projection(box,tm,lightdir,znear,zfar);
}

bool TexProjectClass::Compute_Texture(RenderObjClass * model,SpecialRenderInfoClass * context)
{
	if ((model == NULL) || (context == NULL)) {
		return false;
	}
	TextureClass * rtarget = Peek_Render_Target();
	if (rtarget != NULL && rtarget->D3DTexture != NULL) {
		DX8Wrapper::Set_Render_Target(rtarget);
		Configure_Camera(context->Camera);
		Vector3 color(0.0f,0.0f,0.0f);
		if (Get_Flag(ADDITIVE) == false) {
			color.Set(1.0f,1.0f,1.0f);
		}
		WW3D::Begin_Render(true,false,color,0);
		WW3D::Render(*model,*context);
		WW3D::End_Render(false);
		DX8Wrapper::Set_Render_Target(NULL,false);
	}
	return true;
}

bool TexProjectClass::Needs_Render_Target(void)
{
	return Get_Flag(TEXTURE_DIRTY);
}

void TexProjectClass::Set_Render_Target(TextureClass * render_target)
{
	REF_PTR_SET(RenderTarget,render_target);
	Set_Texture(RenderTarget);
}

TextureClass * TexProjectClass::Peek_Render_Target(void)
{
	return RenderTarget;
}

void TexProjectClass::Configure_Camera(CameraClass & camera)
{
	camera.Set_Transform(Transform);
	camera.Set_Clip_Planes(0.01f,ZFar);
	if (Get_Flag(PERSPECTIVE)) {
		camera.Set_Projection_Type(CameraClass::PERSPECTIVE);
		camera.Set_View_Plane(HFov,VFov);
	} else {
		camera.Set_Projection_Type(CameraClass::ORTHO);
		camera.Set_View_Plane(Vector2(XMin,YMin),Vector2(XMax,YMax));
	}
	float size=(float)Get_Texture_Size();
	float inv_size=1.0f/size;
	Vector2 vmin(1.0f*inv_size,1.0f*inv_size);
	Vector2 vmax((size-1.0f)*inv_size,(size-1.0f)*inv_size);
	camera.Set_Viewport(vmin,vmax);
}

void TexProjectClass::Pre_Render_Update(const Matrix3D & camera)
{
	Matrix3D world_to_texture;
	Matrix3D tmp;
	Matrix4	view_to_texture;
	Transform.Get_Orthogonal_Inverse(world_to_texture);
	Matrix3D::Multiply(world_to_texture,camera,&tmp);
	Matrix4::Multiply(Projection,tmp,&view_to_texture);
	float frame_time = (float)WW3D::Get_Frame_Time() / 1000.0f;
	float intensity_delta = DesiredIntensity - Intensity;
	float max_intensity_delta = INTENSITY_RATE_OF_CHANGE * frame_time;
	if (intensity_delta > max_intensity_delta) {
		Intensity += max_intensity_delta;
	} else if (intensity_delta < -max_intensity_delta) {
		Intensity -= max_intensity_delta;
	} else {
		Intensity = DesiredIntensity;
	}
	float actual_intensity = Intensity * Attenuation;
	VertexMaterialClass * vmat = MaterialPass->Material;
	if (Get_Flag(ADDITIVE)) {
		vmat->Set_Emissive(actual_intensity,actual_intensity,actual_intensity);
	} else {
		vmat->Set_Emissive(1.0f - actual_intensity,1.0f - actual_intensity,1.0f - actual_intensity);
	}
	if (Get_Flag(PERSPECTIVE)) {
		Mapper->Set_Type(MatrixMapperClass::PERSPECTIVE_PROJECTION);
	} else {
		Mapper->Set_Type(MatrixMapperClass::ORTHO_PROJECTION);
	}
	if (Get_Texture_Size() == 0) {
		Set_Texture_Size(MaterialPass->Peek_Texture()->Width);
	}
	Mapper->Set_Texture_Transform(view_to_texture,(float)Get_Texture_Size());
	if (Mapper1) {
		Mapper1->Set_Texture_Transform(view_to_texture,(float)Get_Texture_Size());
	}
}

RENEGADE_FUNCTION
void TexProjectClass::Update_WS_Bounding_Volume()
AT1(0x007868D0);

RENEGADE_FUNCTION
bool TexProjectClass::Compute_Perspective_Projection(const AABoxClass & obj_box, const Matrix3D & tm, const Vector3 & lightpos, float user_znear, float user_zfar)
AT1(0x00785DC0);

RENEGADE_FUNCTION
bool TexProjectClass::Compute_Ortho_Projection(const AABoxClass & obj_box, const Matrix3D & tm, const Vector3 & lightdir, float user_znear, float user_zfar)
AT1(0x007861B0);
