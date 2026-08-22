#ifndef TT_INCLUDE__SOUNDENVIRONMENTCLASS_H
#define TT_INCLUDE__SOUNDENVIRONMENTCLASS_H
#include "engine_vector.h"


class PhysicsSceneClass;
class CameraClass;



class SoundEnvironmentClass : public RefCountClass
{
public:
	int users;
	int count;
	float amplitude;
	float *values;
	SoundEnvironmentClass();
	~SoundEnvironmentClass();
	void Update(PhysicsSceneClass*, CameraClass*);
	void Reset();
	float Get_Amplitude()
	{
		return 0.125f * amplitude;
	}
	void Add_User()
	{
		users++;
	}
	void Remove_User()
	{
		users--;
	}

};



#endif