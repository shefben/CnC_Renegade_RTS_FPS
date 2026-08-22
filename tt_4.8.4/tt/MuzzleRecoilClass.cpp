#include "General.h"
#include "MuzzleRecoilClass.h"



#include "RenderObjClass.h"
#include "TimeManager.h"


MuzzleRecoilClass::MuzzleRecoilClass()
{
	Init(0);
}



void MuzzleRecoilClass::Init(int _boneId)
{
	boneId = _boneId;
	scale = 1.f;
	remainingTime = 0.f;
	timeScale = 0.f;
}



void MuzzleRecoilClass::Start_Recoil(float _scale, float time)
{
	scale = _scale;
	remainingTime = time;

	if (time > 0)
		timeScale = 1.f / time;
}



void MuzzleRecoilClass::Update(RenderObjClass* renderObj)
{
	if (remainingTime > 0 && boneId)
	{
		renderObj->Capture_Bone(boneId);
		
		Matrix3D matrix = Matrix3D::Identity;
		matrix.setPosition(Vector3(-(scale * remainingTime * timeScale), 0, 0));
		
		renderObj->Control_Bone(boneId, matrix, false);
		remainingTime -= TimeManager::FrameSeconds;
		if (remainingTime <= 0)
		{
			remainingTime = 0;
			renderObj->Release_Bone(boneId);
		}
	}
}