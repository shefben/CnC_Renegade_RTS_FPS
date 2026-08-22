#include "general.h"
#include "LightEnvironmentClass.h"
#include "Matrix3D.h"
LightEnvironmentClass::LightEnvironmentClass() : LightCount(0), ObjectCenter(0,0,0), OutputAmbient(0,0,0)
{
}

void LightEnvironmentClass::OutputLightStruct::Init(InputLightStruct& input,const Matrix3D& camera_tm)
{
	Diffuse = input.Diffuse;
	Direction.X = (camera_tm[0].X * input.Direction.X) + (camera_tm[1].X * input.Direction.Y) + (camera_tm[2].X * input.Direction.Z);
	Direction.Y = (camera_tm[0].Y * input.Direction.X) + (camera_tm[1].Y * input.Direction.Y) + (camera_tm[2].Y * input.Direction.Z);
	Direction.Z = (camera_tm[0].Z * input.Direction.X) + (camera_tm[1].Z * input.Direction.Y) + (camera_tm[2].Z * input.Direction.Z);
}

float Clamp(float val,float min,float max)
{
	if (val < min)
	{
		return min;
	}
	if (val > max)
	{
		return max;
	}
	return val;
}

void LightEnvironmentClass::Pre_Render_Update(const Matrix3D &camera_tm)
{
	for (int i = 0;i < LightCount;i++)
	{
		OutputLights[i].Init(InputLights[i],camera_tm);
	}
	OutputAmbient.X = Clamp(OutputAmbient.X,0,1);
	OutputAmbient.Y = Clamp(OutputAmbient.Y,0,1);
	OutputAmbient.Z = Clamp(OutputAmbient.Z,0,1);
}

void LightEnvironmentClass::Reset(Vector3& object_center,Vector3& ambient)
{
	LightCount = 0;
	ObjectCenter = object_center;
	OutputAmbient = ambient;
}

RENEGADE_FUNCTION
void LightEnvironmentClass::Add_Light(LightClass& light)
AT1(0x007850D0);
