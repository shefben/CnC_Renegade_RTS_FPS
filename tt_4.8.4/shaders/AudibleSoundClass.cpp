#include "General.h"
#include "AudibleSoundClass.h"
#include "WWAudioClass.h"
#include "SysTimeClass.h"
#include "SoundHandleClass.h"
#include "AudibleSoundDefinitionClass.h"
#include "LogicalSoundClass.h"
#include "FilteredSoundClass.h"
#include "SoundBufferClass.h"
#pragma warning(disable:4201) // anonymous unions warning
#include <mmsystem.h>
#pragma warning(default:4201) // anonymous unions warning
#include "mss32.h"

void AudibleSoundClass::Allocate_Miles_Handle()
{
	if (!handle)
		Set_Miles_Handle(WWAudioClass::_theInstance->Get_2D_Sample(*this));
}



bool AudibleSoundClass::Play(bool addToPlaylist)
{
	AIL_lock();
	if (Verify_Playability())
	{
		if (addToPlaylist && !filteredSound)
			Allocate_Miles_Handle();
		
		if (state != 1)
		{
			WWAudioClass::_theInstance->Add_To_Playlist(this);
			state = 1;
			
			time = TIMEGETTIME();
			remainingLoops = loopCount;
			if (handle)
				handle->Start_Sample();
			
			playPosition = (uint32)(duration * startPosition);
			if (playPosition != 0)
				Seek(playPosition);
			
			On_Event(AudioCallbackClass::EventSoundStarted, 0, 0);
			
			if (!logicalSound && definition)
				logicalSound = definition->Create_Logical();
			
			if (logicalSound)
			{
				logicalSound->Set_User_Data(xUser, uUserData);
				logicalSound->Set_Transform(transform);
				logicalSound->Add_To_Scene(true);
			}
			
			if (!culled && definition)
				WWAudioClass::_theInstance->Fire_Text_Callback(this, definition->Get_Display_Text());
		}
		else
			// This is a workaround to make the shoot shotgun test work. Without this the shotgun fire sound would not always play.
			// StealthEye assumes there is a better way to fix this, it is possible that the sound is stopped too early somehow.
			if (handle)
				handle->Resume_Sample();
		AIL_unlock();
		return true;
	}
	else
	{
		AIL_unlock();
		return false;
	}
}



bool AudibleSoundClass::Verify_Playability()
{
	if (!channel)
		return true;

	return WWAudioClass::_theInstance->Acquire_Virtual_Channel(this, channel);
}

AudibleSoundClass::AudibleSoundClass() : handle(0), time(0), state(0), type(TypeEffect), fade(NoFade), fadeRemainingTime(0), fadeTime(0), channel(0), buffer(0), runtimePriority(0), priority(0.5), realVolume(1), volume(1), pan(0.5), loopCount(1), remainingLoops(0), duration(0), playPosition(0), startPosition(0), pitch(1), listenerTransform(true), transform(true), o260(true), filteredSound(0), definition(0), logicalSound(0), culled(true), dirty(true), dropOffRadius(1)
{
}

class WWAudioThreadsClass
{
public:
	static void Add_Delayed_Release_Object(RefCountClass *, unsigned long);
};

RENEGADE_FUNCTION
void WWAudioThreadsClass::Add_Delayed_Release_Object(RefCountClass *, unsigned long)
AT1(0x0051C5B0);

AudibleSoundClass::~AudibleSoundClass()
{
	if (filteredSound)
	{
		Stop(true);
		REF_PTR_RELEASE(filteredSound);
	}
	if (!culled)
	{
		if (state == 1)
		{
			filteredSound->Allocate_Miles_Handle();
		}
	}
	REF_PTR_RELEASE(logicalSound);
	if (buffer)
	{
		WWAudioThreadsClass::Add_Delayed_Release_Object(buffer,2000);
		buffer = 0;
	}
	Free_Miles_Handle();
}

RENEGADE_FUNCTION
const PersistFactoryClass &AudibleSoundClass::Get_Factory() const
AT1(0x0051F1E0);

RENEGADE_FUNCTION
bool AudibleSoundClass::Save(ChunkSaveClass& xSave)
AT1(0x0051FEC0);

RENEGADE_FUNCTION
bool AudibleSoundClass::Load(ChunkLoadClass& xLoad)
AT1(0x005201A0);

AudibleSoundClass* AudibleSoundClass::As_AudibleSoundClass()
{
	return this;
}

RENEGADE_FUNCTION
bool AudibleSoundClass::On_Frame_Update(uint32 timePast)
AT1(0x0051EBA0);

void AudibleSoundClass::Set_Position(const Vector3& position)
{
	transform[0][3] = position.X;
	transform[1][3] = position.Y;
	transform[2][3] = position.Z;
	Set_Dirty(true);
}

Vector3 AudibleSoundClass::Get_Position() const
{
	return Vector3(transform[0][3],transform[1][3],transform[2][3]);
}

void AudibleSoundClass::Set_Listener_Transform(const Matrix3D& Transform)
{
	listenerTransform = Transform;
}

void AudibleSoundClass::Set_Transform(const Matrix3D& Transform)
{
	transform = Transform;
	Set_Dirty(true);
}

Matrix3D AudibleSoundClass::Get_Transform() const
{
	return transform;
}

void AudibleSoundClass::Cull_Sound(bool cull)
{
	if (culled != cull)
	{
		culled = cull;
		if (cull || filteredSound)
		{
			Allocate_Miles_Handle();
		}
		else
		{
			Free_Miles_Handle();
		}
	}
}

bool AudibleSoundClass::Is_Sound_Culled() const
{
	return culled;
}

RENEGADE_FUNCTION
void AudibleSoundClass::Add_To_Scene(bool)
AT1(0x0051EF10);

RENEGADE_FUNCTION
void AudibleSoundClass::Remove_From_Scene()
AT1(0x0051EF40);

void AudibleSoundClass::Set_DropOff_Radius(float dropOff)
{
	dropOffRadius = dropOff;
	Set_Dirty(true);
}

float AudibleSoundClass::Get_DropOff_Radius() const
{
	return dropOffRadius;
}

unsigned long AudibleSoundClass::Get_Class_ID() const
{
	return 1;
}

AudibleSoundClass::SoundType AudibleSoundClass::Get_Type() const
{
	return type;
}

void AudibleSoundClass::Set_Type(SoundType t)
{
	type = t;
}

RENEGADE_FUNCTION
bool AudibleSoundClass::Pause()
AT1(0x0051E480);

RENEGADE_FUNCTION
bool AudibleSoundClass::Resume()
AT1(0x0051E500);

RENEGADE_FUNCTION
bool AudibleSoundClass::Stop(bool removeFromPlaylist)
AT1(0x0051E5A0);

RENEGADE_FUNCTION
void AudibleSoundClass::Seek(uint32 playPosition)
AT1(0x0051E640);

uint32 AudibleSoundClass::Get_State() const
{
	return state;
}

void AudibleSoundClass::Fade_Out(uint32 Time)
{
	fade = FadeOut;
	fadeTime = Time;
	fadeRemainingTime = Time;
}

void AudibleSoundClass::Fade_In(uint32 Time)
{
	fadeTime = Time;
	fadeRemainingTime = Time;
	fade = FadeIn;
	Internal_Set_Volume(0);
	Play(true);
}

uint32 AudibleSoundClass::Get_Timestamp() const
{
	return time;
}

uint32 AudibleSoundClass::Get_Virtual_Channel() const
{
	return channel;
}

void AudibleSoundClass::Set_Virtual_Channel(uint32 ch)
{
	channel = ch;
}

RENEGADE_FUNCTION
const char* AudibleSoundClass::Get_Filename() const
AT1(0x0051ED80);

bool AudibleSoundClass::Is_Playing() const
{
	return Get_State() == 1;
}

RENEGADE_FUNCTION
float AudibleSoundClass::Get_Pan() const
AT1(0x0051E870);

RENEGADE_FUNCTION
void AudibleSoundClass::Set_Pan(float pan)
AT1(0x0051E8B0);

RENEGADE_FUNCTION
float AudibleSoundClass::Get_Volume() const
AT1(0x0051E9F0);

RENEGADE_FUNCTION
void AudibleSoundClass::Set_Volume(float volume)
AT1(0x0051EAB0);

void AudibleSoundClass::Update_Volume()
{
	Set_Volume(realVolume);
}

uint32 AudibleSoundClass::Get_Loop_Count() const
{
	return loopCount;
}

uint32 AudibleSoundClass::Get_Loops_Left() const
{
	return remainingLoops;
}

void AudibleSoundClass::Set_Loop_Count(uint32 count)
{
	loopCount = count;
}

float AudibleSoundClass::Get_Priority() const
{
	return priority;
}

float AudibleSoundClass::Peek_Priority() const
{
	return priority;
}

void AudibleSoundClass::Set_Priority(float p)
{
	priority = p;
}

float AudibleSoundClass::Get_Runtime_Priority() const
{
	return runtimePriority;
}

void AudibleSoundClass::Set_Runtime_Priority(float Priority)
{
	runtimePriority = Priority;
}

RENEGADE_FUNCTION
uint32 AudibleSoundClass::Get_Playback_Rate() const
AT1(0x0051E990);

RENEGADE_FUNCTION
void AudibleSoundClass::Set_Playback_Rate(uint32 rate)
AT1(0x0051E9C0);

float AudibleSoundClass::Get_Pitch_Factor() const
{
	return pitch;
}

RENEGADE_FUNCTION
void AudibleSoundClass::Set_Pitch_Factor(float pitch)
AT1(0x0051E930);

uint32 AudibleSoundClass::Get_Duration() const
{
	return duration;
}

uint32 AudibleSoundClass::Get_Play_Position() const
{
	return playPosition;
}

RENEGADE_FUNCTION
void AudibleSoundClass::Set_Play_Position(float playPosition)
AT1(0x0051DA80);

void AudibleSoundClass::Set_Play_Position(uint32 playPosition)
{
	Seek(playPosition);
}

void AudibleSoundClass::Set_Start_Offset(float start)
{
	startPosition = start;
}

float AudibleSoundClass::Get_Start_Offset() const
{
	return startPosition;
}

void AudibleSoundClass::Set_Dirty(bool d)
{
	dirty = d;
}

bool AudibleSoundClass::Is_Dirty() const
{
	return dirty;
}

void AudibleSoundClass::Set_Definition(AudibleSoundDefinitionClass* def)
{
	definition = def;
}

AudibleSoundDefinitionClass *AudibleSoundClass::Get_Definition()
{
	return definition;
}

RENEGADE_FUNCTION
void AudibleSoundClass::Re_Sync(const AudibleSoundClass& sound)
AT1(0x0051EF90);

RENEGADE_FUNCTION
void AudibleSoundClass::Free_Conversion()
AT1(0x0051F100);

RENEGADE_FUNCTION
void AudibleSoundClass::Convert_To_Filtered()
AT1(0x0051F160);

FilteredSoundClass* AudibleSoundClass::As_Converted_Format()
{
	if (!filteredSound)
	{
		Convert_To_Filtered();
	}
	return filteredSound;
}

RENEGADE_FUNCTION
float AudibleSoundClass::Determine_Real_Volume() const
AT1(0x0051ED40);

RENEGADE_FUNCTION
void AudibleSoundClass::Internal_Set_Volume(float volume)
AT1(0x0051EA30);

RENEGADE_FUNCTION
void AudibleSoundClass::Update_Fade()
AT1(0x0051E1E0);

SoundHandleClass* AudibleSoundClass::Get_Miles_Handle() const
{
	return handle;
}

RENEGADE_FUNCTION
void AudibleSoundClass::Set_Miles_Handle(uint32 milesHandle)
AT1(0x0051E6C0);

RENEGADE_FUNCTION
void AudibleSoundClass::Free_Miles_Handle()
AT1(0x0051E830);

RENEGADE_FUNCTION
void AudibleSoundClass::Initialize_Miles_Handle()
AT1(0x0051E730);

RENEGADE_FUNCTION
SoundBufferClass* AudibleSoundClass::Get_Buffer() const
AT1(0x0051E1C0);

SoundBufferClass* AudibleSoundClass::Peek_Buffer() const
{
	return buffer;
}

RENEGADE_FUNCTION
void AudibleSoundClass::Set_Buffer(SoundBufferClass* buffer)
AT1(0x0051E130);

void AudibleSoundClass::Restart_Loop()
{
}

RENEGADE_FUNCTION
void AudibleSoundClass::Update_Play_Position()
AT1(0x0051EC00);

RENEGADE_FUNCTION
void AudibleSoundClass::On_Loop_End()
AT1(0x0051ECF0);
