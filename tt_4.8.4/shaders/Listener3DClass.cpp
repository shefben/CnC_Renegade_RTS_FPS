#include "General.h"
#include "Listener3DClass.h"

Listener3DClass::Listener3DClass()
{
}

Listener3DClass::~Listener3DClass()
{
}

Listener3DClass* Listener3DClass::As_Listener3DClass()
{
	return this;
}

void Listener3DClass::Set_DropOff_Radius(float dropOffRadius)
{
}

float Listener3DClass::Get_DropOff_Radius()
{
	return 0;
}

unsigned long Listener3DClass::Get_Class_ID() const
{
	return 3;
}

bool Listener3DClass::Pause()
{
	return false;
}

bool Listener3DClass::Resume()
{
	return false;
}

bool Listener3DClass::Stop(bool removeFromPlaylist)
{
	return false;
}

void Listener3DClass::Seek(uint32 playPosition)
{
}

uint32 Listener3DClass::Get_State() const
{
	return 0;
}

void Listener3DClass::Free_Miles_Handle()
{
}

RENEGADE_FUNCTION
void Listener3DClass::Initialize_Miles_Handle()
AT1(0x00524330);

void Listener3DClass::Allocate_Miles_Handle()
{
}

void Listener3DClass::Set_Velocity(Vector3 const&)
{
}

void Listener3DClass::Set_Max_Vol_Radius(float)
{
}

float Listener3DClass::Get_Max_Vol_Radius()
{
	return 0;
}

void Listener3DClass::On_Added_To_Scene()
{
	Allocate_Miles_Handle();
}

void Listener3DClass::Start_Sample()
{
	Free_Miles_Handle();
}

void Listener3DClass::Stop_Sample()
{
}

void Listener3DClass::Resume_Sample()
{
}

void Listener3DClass::End_Sample()
{
}

void Listener3DClass::Set_Sample_Volume(long)
{
}

long Listener3DClass::Get_Sample_Volume()
{
	return 0;
}

void Listener3DClass::Set_Sample_Pan(long)
{
}

long Listener3DClass::Get_Sample_Pan()
{
	return 64;
}

void Listener3DClass::Set_Sample_Loop_Count(unsigned long)
{
}

unsigned long Listener3DClass::Get_Sample_Loop_Count()
{
	return 0;
}

void Listener3DClass::Set_Sample_MS_Position(unsigned long)
{
}

void Listener3DClass::Get_Sample_MS_Position(long *,long *)
{
}

long Listener3DClass::Get_Sample_Playback_Rate()
{
	return 0;
}

void Listener3DClass::Set_Sample_Playback_Rate(long)
{
}
