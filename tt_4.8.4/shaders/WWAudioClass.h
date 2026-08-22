#ifndef __WWAUDIOCLASS_H__
#define __WWAUDIOCLASS_H__



#include "engine_common.h"
#include "engine_string.h"
#include "engine_vector.h"
#include "scripts.h"
#include "AudioCallbackListClass.h"
#include "Matrix3D.h"

class INIClass;
class FileFactoryClass;
class AudibleSoundClass;
class SoundSceneObjClass;
class DigDriver;
class ChunkSaveClass;
class ChunkLoadClass;
class RenderObjClass;
class Listener3DClass;
class SoundCullObjClass;
class AudibleInfoClass;
class LogicalSoundClass;
class LogicalListenerClass;
class WaveFormat;
class SoundBufferClass;
class Sound3DClass;
class FileClass;



class SoundSceneClass
{
public:
   virtual ~SoundSceneClass();

   virtual void Re_Partition              (const Vector3&, const Vector3&);
   virtual void Collect_Logical_Sounds    (sint32);
   virtual void Attach_Listener_To_Obj    (RenderObjClass*, sint32);
   virtual void Set_Listener_Position     (const Vector3&);
   virtual void Get_Listener_Position     ();
   virtual void Set_Listener_Transform    (const Matrix3D&);
   virtual void Get_Listener_Transform    ();
   virtual Listener3DClass* Peek_2nd_Listener();
   virtual void Set_2nd_Listener          (Listener3DClass*);
   virtual void Flush_Scene               ();
   virtual void Update_Sound              (SoundCullObjClass*);
   virtual void Add_Sound                 (AudibleSoundClass*, bool bPlay);
   virtual void Remove_Sound              (AudibleSoundClass*, bool bStopSound);
   virtual void Add_Static_Sound          (AudibleSoundClass*, bool);
   virtual void Remove_Static_Sound       (AudibleSoundClass*, bool bStopSound);
   virtual void Add_Logical_Sound         (LogicalSoundClass*, bool);
   virtual void Remove_Logical_Sound      (LogicalSoundClass*, bool bStopSound, bool);
   virtual void Add_Logical_Listener      (LogicalListenerClass*);
   virtual void Remove_Logical_Listener   (LogicalListenerClass*);
   virtual bool On_Frame_Update           (uint32);
   virtual void Initialize                ();
   virtual void Is_Logical_Sound_In_Scene (LogicalSoundClass*, bool);
   virtual void Save_Static_Sounds        (ChunkSaveClass&);
   virtual void Load_Static_Sounds        (ChunkLoadClass&);
   virtual void Collect_Audible_Sounds    (Listener3DClass*, MultiListClass<AudibleInfoClass>&);

};



class SHADERS_API WWAudioClass abstract
{

	// The following dummy functions are to make class exporting work:
	WWAudioClass(const WWAudioClass&);
	const WWAudioClass& operator =(const WWAudioClass&);

public:

	typedef void (*Callback) (SoundSceneObjClass* sound, uint32 userData);
	typedef void (*TextCallback) (AudibleSoundClass* sound, const StringClass& displayText, uint32 userData);
	
	enum SOUND_PAGE
	{
		SOUND_PAGE_0,
		SOUND_PAGE_MENU,
		SOUND_PAGE_2,
	};
	
	class _CACHE_ENTRY_STRUCT: public NoEqualsClass<_CACHE_ENTRY_STRUCT>
	{
	};
	
	class _LOGICAL_TYPE_STRUCT: public NoEqualsClass<_LOGICAL_TYPE_STRUCT>
	{
	};

	struct _DRIVER_INFO_STRUCT;
	enum DRIVER_TYPE_3D;

	uint playbackRate; // 0004
	uint playbackBits; // 0008
	bool playbackStereo; // 000C
	float musicVolume; // 0010
	float effectsVolume; // 0014
	float unk0018; // 0018
	float unk001C; // 001C
	float dialogVolume; // 0020
	float cinematicVolume; // 0024
	int unk0028; // 0028
	int unk002C; // 002C
	uint maxBuffer2D; // 0030
	uint maxBuffer3D; // 0034
	int unk0038; // 0038
	bool soundMusic; // 003C
	bool soundDialog; // 003D
	bool soundCinematic; // 003E
	bool soundEffects; // 003F
	bool newSounds; // 0040
	FileFactoryClass* fileFactory; // 0044
	AudibleSoundClass* backgroundSound; // 0048
	StringClass backgroundSoundFile; // 004C
	bool unk0050; // 0050
	bool unk0051; // 0051
	bool unk0052; // 0052
	bool unk0053; // 0053
	AudioCallbackListClass<Callback> callbacks; // 0054
	AudioCallbackListClass<TextCallback> textCallbacks; // 0064
	SoundSceneClass* soundScene; // 0074
	SOUND_PAGE soundPage; // 0078
	DynamicVectorClass<SOUND_PAGE> pages; // 007C
	DigDriver* driver2d; // 0094
	DigDriver* driver3d; // 0098
	int unk009C; // 009C
	uint reverbFilter; // 00A0
	DynamicVectorClass<_DRIVER_INFO_STRUCT *> drivers; // 00A4
	StringClass driverName; // 00BC
	uint speakerType; // 00C0
	DynamicVectorClass<long *> unk00C4; // 00C4
	DynamicVectorClass<long *> unk00DC; // 00DC
	DynamicVectorClass<AudibleSoundClass*> soundPages[3]; // 00F4
	DynamicVectorClass<AudibleSoundClass*> unk013C; // 013C
	DynamicVectorClass<AudibleSoundClass*> unk0154; // 0154
	DynamicVectorClass<_CACHE_ENTRY_STRUCT> cacheTable[256]; // 016C
	uint32 cacheSize; // 196C
	uint32 currentCacheSize; // 1970
	DynamicVectorClass<_LOGICAL_TYPE_STRUCT> logicalTypes; // 1974
	float effectsLevel; // 198C
	uint32 reverbRoomType; // 1990
	uint32 nonDialogFadeTime; // 1994
	int unk1998; // 1998
	float unk199C; // 199C
	INIClass *ini; // 19A0
	bool unk19A4; // 19A4

public:

	static REF_DECL2(_theInstance, WWAudioClass*);
	static REF_DECL2(_TimerSyncEvent, UNK);

	static WWAudioClass* Get_Instance() { return _theInstance; }

	virtual ~WWAudioClass();
	
	WWAudioClass(bool);
	UNK Flush_Cache();
	UNK Open_2D_Device(WaveFormat*);
	UNK Open_2D_Device(bool, int, int);
	UNK Close_2D_Device();
	UNK Close_3D_Device();
	UNK Get_Sound_Buffer(const char*, bool);
	UNK Get_Sound_Buffer(FileClass&, const char*, bool);
	UNK Find_Cached_Buffer(const char*);
	UNK Free_Cache_Space(int);
	UNK Cache_Buffer(SoundBufferClass*, const char*);
	UNK Create_Sound_Buffer(FileClass&, const char*, bool);
	UNK Create_Sound_Buffer(uint8*, uint32, const char*, bool);
	UNK Create_Sound_Effect(FileClass&, const char*);
	UNK Create_Sound_Effect(const char*);
	UNK Create_Sound_Effect(const char*, uint8*, uint32);
	UNK Create_3D_Sound(FileClass&, const char*, int);
	UNK Create_3D_Sound(const char*, int);
	UNK Create_3D_Sound(const char*, uint8*, uint32, int);
	AudibleSoundClass* Create_Sound(int, RefCountClass*, uint32, int);
	AudibleSoundClass* Create_Sound(const char*, RefCountClass*, uint32, int);
	AudibleSoundClass* Create_Continuous_Sound(int, RefCountClass*, uint32, int);
	uint32 Create_Instant_Sound(int, const Matrix3D&, RefCountClass*, uint32, int);
	AudibleSoundClass* Create_Continuous_Sound(const char*, RefCountClass*, uint32, int);
	uint32 Create_Instant_Sound(const char*, const Matrix3D&, RefCountClass*, uint32, int);
	void Flush_Playlist(SOUND_PAGE);
	void Flush_Playlist();
	UNK Free_Completed_Sounds();
	UNK Get_Playlist_Entry(int);
	void Add_To_Playlist(AudibleSoundClass*);
	bool Remove_From_Playlist(AudibleSoundClass*);
	bool Is_Sound_In_Playlist(AudibleSoundClass*);
	UNK Reprioritize_Playlist();
	void On_Frame_Update(uint);
	UNK Release_2D_Handles();
	UNK Allocate_2D_Handles();
	uint32 Get_2D_Sample(const AudibleSoundClass&);
	uint32 Get_3D_Sample(const Sound3DClass&);
	uint32 Get_Listener_Handle();
	UNK Build_3D_Driver_List();
	UNK Free_3D_Driver_List();
	UNK Select_3D_Device(const char*);
	UNK Select_3D_Device(const char*, uint32);
	UNK Select_3D_Device(int);
	UNK Select_3D_Device(DRIVER_TYPE_3D);
	UNK Find_3D_Device(DRIVER_TYPE_3D);
	UNK Allocate_3D_Handles();
	UNK Release_3D_Handles();
	UNK Validate_3D_Sound_Buffer(SoundBufferClass*);
	UNK ReAssign_2D_Handles();
	UNK ReAssign_3D_Handles();
	UNK Remove_2D_Sound_Handles();
	UNK Remove_3D_Sound_Handles();
	UNK Set_Dialog_Volume(float);
	UNK Set_Cinematic_Volume(float);
	UNK Set_Sound_Effects_Volume(float);
	UNK Set_Music_Volume(float);
	UNK Internal_Set_Sound_Effects_Volume(float);
	UNK Internal_Set_Music_Volume(float);
	bool Is_Disabled();
	UNK Initialize(const char*);
	UNK Initialize(bool, int, int);
	UNK Shutdown();
	UNK Register_EOS_Callback(void (*)(SoundSceneObjClass*, uint32), uint32);
	UNK UnRegister_EOS_Callback(void (*)(SoundSceneObjClass*, uint32));
	UNK Register_Text_Callback(void (*)(AudibleSoundClass*, const StringClass&, uint32), uint32);
	UNK UnRegister_Text_Callback(void (*)(AudibleSoundClass*, const StringClass&, uint32));
	void Fire_Text_Callback(AudibleSoundClass*, const StringClass&);
	UNK Allow_Sound_Effects(bool);
	UNK Allow_Music(bool);
	UNK Allow_Dialog(bool);
	UNK Allow_Cinematic_Sound(bool);
	void Simple_Play_2D_Sound_Effect(const char*, float, float);
	void Simple_Play_2D_Sound_Effect(FileClass&, float, float);
	UNK Get_File(const char*);
	UNK Return_File(FileClass*);
	UNK Create_Logical_Sound();
	UNK Create_Logical_Listener();
	UNK Add_Logical_Type(int, const char*);
	UNK Reset_Logical_Types();
	UNK Get_Logical_Type(int, StringClass&);
	UNK Find_Sound_Object(uint32);
	bool Load_From_Registry(const char*);
	bool Load_From_Registry(const char*, StringClass&, bool&, int&, int&, bool&, bool&, bool&, bool&, float&, float&, float&, float&, int&);
	bool Save_To_Registry(const char*);
	bool Save_To_Registry(const char*, const StringClass&, bool, int, int, bool, bool, bool, bool, float, float, float, float, int);
	UNK File_Open_Callback(const char*, uint32*);
	UNK File_Close_Callback(uint32);
	UNK File_Seek_Callback(uint32, sint32, uint32);
	UNK File_Read_Callback(uint32, void*, uint32);
	UNK Fade_Background_Music(const char*, int, int);
	UNK Set_Background_Music(const char*);
	UNK Set_Active_Sound_Page(WWAudioClass::SOUND_PAGE);
	UNK Fade_Non_Dialog_In();
	UNK Fade_Non_Dialog_Out();
	UNK Update_Fade();
	UNK Peek_2D_Sample(int);
	UNK Peek_3D_Sample(int);
	bool Acquire_Virtual_Channel(AudibleSoundClass*, int);
	UNK Release_Virtual_Channel(AudibleSoundClass*, int);
	UNK Set_Speaker_Type(int);
	UNK Get_Speaker_Type();
	UNK Push_Active_Sound_Page(WWAudioClass::SOUND_PAGE);
	UNK Pop_Active_Sound_Page();
	UNK Temp_Disable_Audio(bool);
	bool Load_Default_Volume(int&, int&, int&, int&);
	UNK Get_2D_Driver();
	UNK Get_3D_Driver();
	UNK Get_3D_Driver_Name();
	UNK Get_Reverb_Filter();
	UNK Get_Playback_Rate();
	UNK Get_Playback_Bits();
	UNK Get_Playback_Stereo();
	UNK Get_3D_Device_Count();
	UNK Get_3D_Device(int, _DRIVER_INFO_STRUCT**);
	bool Is_3D_Device_Available(DRIVER_TYPE_3D);
	UNK Set_File_Factory(FileFactoryClass*);
	UNK Get_Effects_Level();
	UNK Get_Reverb_Room_Type();
	UNK Get_Sound_Effects_Volume();
	UNK Get_Music_Volume();
	UNK Get_Dialog_Volume();
	UNK Get_Cinematic_Volume();
	UNK Are_Sound_Effects_On();
	bool Is_Music_On();
	bool Is_Dialog_On();
	bool Is_Cinematic_Sound_On();
	UNK Enable_New_Sounds(bool);
	UNK Are_New_Sounds_Enabled();
	UNK Get_Background_Music_Name();
	UNK Peek_Background_Music();
	UNK Get_Logical_Type_Count();
	SoundSceneClass* Get_Sound_Scene() { return soundScene; }
	UNK Set_Cache_Size(int);
	UNK Get_Cache_Size();
	UNK Get_Current_Cache_Size();
	UNK Set_Max_2D_Sound_Buffer(int);
	UNK Set_Max_3D_Sound_Buffer(int);
	UNK Get_Playlist_Count();
	UNK Peek_Playlist_Entry(int);
	UNK Get_Active_Sound_Page();
	UNK Set_Non_Dialog_Fade_Time(float);
	UNK Get_2D_Sample_Count();
	UNK Get_3D_Sample_Count();
	bool Is_OK_To_Give_Handle(const AudibleSoundClass&);

};



#endif
