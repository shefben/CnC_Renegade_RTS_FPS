#ifndef _UNDERWATERRENDERTARGET_H_
#define _UNDERWATERRENDERTARGET_H_

#include "WaterSystemClass.h"
#include "TextureController.h"
#include "UserClipPlaneCamera.h"

class UnderwaterRenderTarget: public RenderTargetTexture
{
protected:
	bool					Initialized;
	WaterSystemClass*		WaterSystem;
	UserClipPlaneCamera*	UnderwaterCamera;
public:
	UnderwaterRenderTarget(RenderTargetDatabase* db, WaterSystemClass* water_system): RenderTargetTexture(db), Initialized(false), WaterSystem(NULL), UnderwaterCamera(NULL)
	{
		REF_PTR_SET(WaterSystem, water_system);
	}

	virtual ~UnderwaterRenderTarget()
	{
		REF_PTR_RELEASE(WaterSystem);
		REF_PTR_RELEASE(UnderwaterCamera);
	}

	void Initialize();
	virtual void Render();

};

#endif // _UNDERWATERRENDERTARGET_H_