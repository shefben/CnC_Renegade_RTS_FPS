#include "General.h"
#include "SoundSceneObjClass.h"
#include "RenderObjClass.h"
REF_DEF1(SoundSceneObjClass::m_NextAvailableID, uint32, 0x007FD5EC);
void AudioCallbackClass::On_Registered(SoundSceneObjClass* sound)
{
   if (Sounds.ID(sound) == -1)
   {
      Sounds.Add(sound);
   }
}
void AudioCallbackClass::On_UnRegistered(SoundSceneObjClass* sound)
{
	Sounds.Delete(Sounds.ID(sound));
}
SoundSceneObjClass::SoundSceneObjClass() : xScene(0), xCullObj(0), xCallback(0), eCallbackEvent(AudioCallbackClass::NoEvent), uID(0), xParent(0), uUserData(0), xUser(0)
{
	uID = m_NextAvailableID++;
	Register_Sound_Object(this);
}

SoundSceneObjClass::~SoundSceneObjClass()
{
	if (xCallback)
	{
		xCallback->On_UnRegistered(this);
	}
	eCallbackEvent = AudioCallbackClass::NoEvent;
	xCallback = 0;
	REF_PTR_RELEASE(xUser);
	REF_PTR_RELEASE(xParent);
	Unregister_Sound_Object(this);
}

RENEGADE_FUNCTION
bool SoundSceneObjClass::Find_Sound_Object(uint32 uID, uint32* uIndex)
AT1(0x00523D00);

RENEGADE_FUNCTION
void SoundSceneObjClass::Register_Sound_Object(SoundSceneObjClass* xSound)
AT1(0x00523B30);

RENEGADE_FUNCTION
void SoundSceneObjClass::Unregister_Sound_Object(SoundSceneObjClass* xSound)
AT1(0x00523C80);

RENEGADE_FUNCTION
bool SoundSceneObjClass::Save(ChunkSaveClass& xSave)
AT1(0x00523790);

RENEGADE_FUNCTION
bool SoundSceneObjClass::Load(ChunkLoadClass& xLoad)
AT1(0x00523860);

Sound3DClass* SoundSceneObjClass::As_Sound3DClass()
{
	return 0;
}

SoundPseudo3DClass* SoundSceneObjClass::As_SoundPseudo3DClass()
{
	return 0;
}

FilteredSoundClass* SoundSceneObjClass::As_FilteredSoundClass()
{
	return 0;
}

Listener3DClass* SoundSceneObjClass::As_Listener3DClass()
{
	return 0;
}

AudibleSoundClass* SoundSceneObjClass::As_AudibleSoundClass()
{
	return 0;
}

unsigned long SoundSceneObjClass::Get_ID()
{
	return uID;
}

void SoundSceneObjClass::Set_ID(uint32 ID)
{
	Unregister_Sound_Object(this);
	uID = ID;
	Register_Sound_Object(this);
}

bool SoundSceneObjClass::On_Frame_Update(uint32)
{
	Apply_Auto_Position();
	return true;
}

RENEGADE_FUNCTION
void SoundSceneObjClass::On_Event(AudioCallbackClass::Events eCallbackEvent,LogicalListenerClass* xListener,LogicalSoundClass* xSound)
AT1(0x0051D8D0);

void SoundSceneObjClass::Register_Callback(AudioCallbackClass::Events CallbackEvent,AudioCallbackClass* Callback)
{
	if (xCallback)
	{
		xCallback->On_UnRegistered(this);
	}
	eCallbackEvent = CallbackEvent;
	xCallback = Callback;
	if (xCallback)
	{
		xCallback->On_Registered(this);
	}
}

void SoundSceneObjClass::Remove_Callback()
{
	xCallback = 0;
	eCallbackEvent = AudioCallbackClass::NoEvent;
}

void SoundSceneObjClass::Set_Listener_Transform(const Matrix3D& xTransform)
{
}

RENEGADE_FUNCTION
void SoundSceneObjClass::Set_User_Data(RefCountClass*, uint32)
AT1(0x0051D810);

uint32 SoundSceneObjClass::Get_User_Data() const
{
	return uUserData;
}

RefCountClass* SoundSceneObjClass::Peek_User_Obj() const
{
	return xUser;
}

RENEGADE_FUNCTION
void SoundSceneObjClass::Attach_To_Object(RenderObjClass* xParent, sint32 iBone)
AT1(0x00523560);

RENEGADE_FUNCTION
void SoundSceneObjClass::Attach_To_Object(RenderObjClass* xParent, const char* cBone)
AT1(0x00523500);

RenderObjClass* SoundSceneObjClass::Peek_Parent_Object() const
{
	return xParent;
}

sint32 SoundSceneObjClass::Get_Parent_Bone() const
{
	return iParentBone;
}

RENEGADE_FUNCTION
void SoundSceneObjClass::Apply_Auto_Position()
AT1(0x005235B0);

bool SoundSceneObjClass::Is_In_Scene() const
{
	return xScene != 0;
}

SoundCullObjClass* SoundSceneObjClass::Peek_Cullable_Wrapper() const
{
	return xCullObj;
}

void SoundSceneObjClass::Set_Cullable_Wrapper(SoundCullObjClass* CullObj)
{
	xCullObj = CullObj;
}
