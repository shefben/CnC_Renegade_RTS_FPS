#include "General.h"
#include "Sound3DClass.h"
#include "vector3.h"
Sound3DClass::Sound3DClass() : IsAutoCalcVelocity(true), IsTransformValid(false), Velocity(0,0,0), MaxVolRadius(0), IsStatic(0), Time(0)
{

}
Sound3DClass::~Sound3DClass()
{
	Free_Miles_Handle();
}


Sound3DClass* Sound3DClass::As_Sound3DClass()
{
	return this;
}

RENEGADE_FUNCTION
bool Sound3DClass::On_Frame_Update(uint32 timePast)
AT1(0x005213C0);

RENEGADE_FUNCTION
void Sound3DClass::Set_Position(const Vector3& position)
AT1(0x00521A00);

RENEGADE_FUNCTION
Vector3 Sound3DClass::Get_Position() const
AT1(0x005207F0);

RENEGADE_FUNCTION
void Sound3DClass::Set_Listener_Transform(const Matrix3D& listenerTransform)
AT1(0x00521860);

RENEGADE_FUNCTION
void Sound3DClass::Set_Transform(const Matrix3D& transform)
AT1(0x005216A0);

Matrix3D Sound3DClass::Get_Transform() const
{
	return transform;
}

RENEGADE_FUNCTION
void Sound3DClass::Add_To_Scene(bool)
AT1(0x00521F50);

RENEGADE_FUNCTION
void Sound3DClass::Remove_From_Scene()
AT1(0x00521FA0);

RENEGADE_FUNCTION
void Sound3DClass::Set_DropOff_Radius(float dropOffRadius)
AT1(0x00521CE0);

RENEGADE_FUNCTION
bool Sound3DClass::Save(ChunkSaveClass& xSave)
AT1(0x00522010);

RENEGADE_FUNCTION
bool Sound3DClass::Load(ChunkLoadClass& xLoad)
AT1(0x005220D0);

SoundCullObjClass* Sound3DClass::Peek_Cullable_Wrapper() const
{
	return xCullObj;
}

void Sound3DClass::Set_Cullable_Wrapper(SoundCullObjClass* obj)
{
	xCullObj = obj;
}

RENEGADE_FUNCTION
bool Sound3DClass::Play(bool addToPlaylist)
AT1(0x00521360);

float Sound3DClass::Get_Priority() const
{
	if (culled)
	{
		return 0;
	}
	else
	{
		return priority;
	}
}

RENEGADE_FUNCTION
const PersistFactoryClass &Sound3DClass::Get_Factory() const
AT1(0x00522000);

RENEGADE_FUNCTION
void Sound3DClass::Set_Miles_Handle(uint32 milesHandle)
AT1(0x005221A0);

RENEGADE_FUNCTION
void Sound3DClass::Initialize_Miles_Handle()
AT1(0x00521DC0);

RENEGADE_FUNCTION
void Sound3DClass::Allocate_Miles_Handle()
AT1(0x00521F20);

void Sound3DClass::On_Loop_End()
{
	AudibleSoundClass::On_Loop_End();
}

void Sound3DClass::Make_Static(bool s)
{
	IsStatic = s;
}

bool Sound3DClass::Is_Static()
{
	return IsStatic;
}

RENEGADE_FUNCTION
void Sound3DClass::Set_Velocity(Vector3 const&)
AT1(0x00521C70);

Vector3 Sound3DClass::Get_Velocity()
{
	return Velocity;
}

void Sound3DClass::Get_Velocity(Vector3 &v)
{
	v = Velocity;
}

void Sound3DClass::Auto_Calc_Velocity(bool b)
{
	IsAutoCalcVelocity = b;
}

bool Sound3DClass::Is_Auto_Calc_Velocity_On()
{
	return IsAutoCalcVelocity;
}

RENEGADE_FUNCTION
void Sound3DClass::Set_Max_Vol_Radius(float)
AT1(0x00521D50);

float Sound3DClass::Get_Max_Vol_Radius()
{
	return MaxVolRadius;
}

float Sound3DClass::Get_DropOff_Radius()
{
	return dropOffRadius;
}
