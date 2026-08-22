#ifndef TT_INCLUDE__LIGHTENVIRONMENTCLASS_H
#define TT_INCLUDE__LIGHTENVIRONMENTCLASS_H
#include "Vector3.h"
#include "Matrix3D.h"
class LightClass;
class LightEnvironmentClass
{
public:
	struct InputLightStruct
	{
		Vector3 Direction; //0
		Vector3 Ambient; //12
		Vector3 Diffuse; //24
		bool DiffuseRejected; //36
	};
	struct OutputLightStruct
	{
		Vector3 Direction; //0
		Vector3 Diffuse; //12
		void Init(InputLightStruct& input,const Matrix3D& camera_tm);
	};
	int LightCount; //0
	Vector3 ObjectCenter; //4
	InputLightStruct InputLights[4]; //16
	Vector3 OutputAmbient; //176
	OutputLightStruct OutputLights[4]; //188
	LightEnvironmentClass();
	void Pre_Render_Update(const Matrix3D &camera_tm);
	void Reset(Vector3& object_center,Vector3& ambient);
	void Add_Light(LightClass& light);
};

#endif
