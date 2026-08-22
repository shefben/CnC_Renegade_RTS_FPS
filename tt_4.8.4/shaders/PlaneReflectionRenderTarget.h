#ifndef _PLANEREFLECTIONRENDERTARGET_H_
#define _PLANEREFLECTIONRENDERTARGET_H_

#include "TextureController.h"
#include "PlaneReflectionCamera.h"
#include "WaterSystemClass.h"

class PlaneReflectionRenderTarget: public RenderTargetTexture
{
protected:
	WaterSystemClass*			WaterSystem;
	bool						Initialized;
	char						MeshName[64];
	char*						ShaderToDisable;
	RenderObjClass*				Mesh;
	PlaneReflectionCamera*		Camera;
public:
	PlaneReflectionRenderTarget(RenderTargetDatabase*, WaterSystemClass* water_system);
	PlaneReflectionRenderTarget(const PlaneReflectionRenderTarget&);
	virtual ~PlaneReflectionRenderTarget();

	bool LoadFromINI(INIClass* ini, const char* key);

	void SetMesh(const char* mesh_name);


	void Initialize();
	virtual void Render();
};

#endif
