#include "General.h"
#include "WWAudioClass.h"



#include "AudibleSoundClass.h"



REF_DEF2(WWAudioClass::_theInstance, WWAudioClass*, 0x00830294, 0x0082F47C);



RENEGADE_FUNCTION
UNK WWAudioClass::Flush_Cache()
AT2(0x00511E30, 0x005116D0);



RENEGADE_FUNCTION
UNK WWAudioClass::Open_2D_Device(WaveFormat*)
AT2(0x00511EC0, 0x00511EC0);



RENEGADE_FUNCTION
UNK WWAudioClass::Open_2D_Device(bool, int, int)
AT2(0x00512160, 0x00511A00);



RENEGADE_FUNCTION
UNK WWAudioClass::Close_2D_Device()
AT2(0x00512230, 0x00511AD0);



RENEGADE_FUNCTION
UNK WWAudioClass::Close_3D_Device()
AT2(0x00512360, 0x00511C00);



RENEGADE_FUNCTION
UNK WWAudioClass::Get_Sound_Buffer(const char*, bool)
AT2(0x00512440, 0x00511CE0);



RENEGADE_FUNCTION
UNK WWAudioClass::Get_Sound_Buffer(FileClass&, const char*, bool)
AT2(0x00512530, 0x00511DD0);



RENEGADE_FUNCTION
UNK WWAudioClass::Find_Cached_Buffer(const char*)
AT2(0x005125C0, 0x00511E60);



RENEGADE_FUNCTION
UNK WWAudioClass::Free_Cache_Space(int)
AT2(0x00512640, 0x00511EE0);



RENEGADE_FUNCTION
UNK WWAudioClass::Cache_Buffer(SoundBufferClass*, const char*)
AT2(0x00512790, 0x00512030);



RENEGADE_FUNCTION
UNK WWAudioClass::Create_Sound_Buffer(FileClass&, const char*, bool)
AT2(0x00512860, 0x00512100);



RENEGADE_FUNCTION
UNK WWAudioClass::Create_Sound_Buffer(uint8*, uint32, const char*, bool)
AT2(0x005129B0, 0x005122504);



RENEGADE_FUNCTION
UNK WWAudioClass::Create_Sound_Effect(FileClass&, const char*)
AT2(0x00512AD0, 0x00512370);



RENEGADE_FUNCTION
UNK WWAudioClass::Create_Sound_Effect(const char*)
AT2(0x00512C40, 0x005124E0);



RENEGADE_FUNCTION
UNK WWAudioClass::Create_Sound_Effect(const char*, uint8*, uint32)
AT2(0x00512E40, 0x005126E0);



RENEGADE_FUNCTION
UNK WWAudioClass::Create_3D_Sound(FileClass&, const char*, int)
AT2(0x00513040, 0x005128E0);



RENEGADE_FUNCTION
UNK WWAudioClass::Create_3D_Sound(const char*, int)
AT2(0x00513210, 0x00512AB0);



RENEGADE_FUNCTION
UNK WWAudioClass::Create_3D_Sound(const char*, uint8*, uint32, int)
AT2(0x00513410, 0x00512CB0);



RENEGADE_FUNCTION
AudibleSoundClass* WWAudioClass::Create_Sound(int, RefCountClass*, uint32, int)
AT2(0x00513670, 0x00512F10);



RENEGADE_FUNCTION
AudibleSoundClass* WWAudioClass::Create_Sound(const char*, RefCountClass*, uint32, int)
AT2(0x005136D0, 0x00512F70);



RENEGADE_FUNCTION
AudibleSoundClass *WWAudioClass::Create_Continuous_Sound(int, RefCountClass*, uint32, int)
AT2(0x00513730, 0x00512FD0);



RENEGADE_FUNCTION
uint32 WWAudioClass::Create_Instant_Sound(int, const Matrix3D&, RefCountClass*, uint32, int)
AT2(0x005137A0, 0x00513040);



RENEGADE_FUNCTION
AudibleSoundClass *WWAudioClass::Create_Continuous_Sound(const char*, RefCountClass*, uint32, int)
AT2(0x00513830, 0x005130D0);



RENEGADE_FUNCTION
uint32 WWAudioClass::Create_Instant_Sound(const char*, const Matrix3D&, RefCountClass*, uint32, int)
AT2(0x005138A0, 0x00513140);



RENEGADE_FUNCTION
void WWAudioClass::Flush_Playlist(WWAudioClass::SOUND_PAGE)
AT2(0x00513930, 0x005131D0);



RENEGADE_FUNCTION
void WWAudioClass::Flush_Playlist()
AT2(0x005139B0, 0x00513250);



RENEGADE_FUNCTION
UNK WWAudioClass::Free_Completed_Sounds()
AT2(0x00513A80, 0x00513320);



RENEGADE_FUNCTION
UNK WWAudioClass::Get_Playlist_Entry(int)
AT2(0x00513B80, 0x00513420);



void WWAudioClass::Add_To_Playlist(AudibleSoundClass* sound)
{
	if (sound && !Is_Sound_In_Playlist(sound))
	{
		sound->Add_Ref();
		soundPages[soundPage].Add(sound);
	}
}



bool WWAudioClass::Remove_From_Playlist(AudibleSoundClass* sound)
{
	if (sound)
	{
		bool found = false;
		for (int pageIndex = 0; pageIndex < 3 && !found; ++pageIndex)
		{
			for (int j = 0; j < soundPages[pageIndex].Count() && !found; ++j)
			{
				if (soundPages[pageIndex][j] == sound)
				{
					unk013C.Add(sound);
					found = true;
				}
			}
		}
		
		if (sound->Get_Loop_Count() != 0)
		{
			for (int index = 0; index < callbacks.Count(); ++index)
			{
				uint32 userData;
				Callback callback = callbacks.Get_Callback(index, &userData);
				if (callback)
					callback(sound, userData);
			}
		}

		return found;
	}
	else
		return false;
}



bool WWAudioClass::Is_Sound_In_Playlist(AudibleSoundClass* sound)
{
	for (int index = 0; index < soundPages[soundPage].Count(); index++)
		if (sound == soundPages[soundPage][index])
			return true;

	return false;
}



RENEGADE_FUNCTION
UNK WWAudioClass::Reprioritize_Playlist()
AT2(0x00513DB0, 0x00513650);



RENEGADE_FUNCTION
void WWAudioClass::On_Frame_Update(uint)
AT2(0x00513E50, 0x005136F0);



RENEGADE_FUNCTION
UNK WWAudioClass::Release_2D_Handles()
AT2(0x00513ED0, 0x00513770);



RENEGADE_FUNCTION
UNK WWAudioClass::Allocate_2D_Handles()
AT2(0x00513F40, 0x005137E0);



RENEGADE_FUNCTION
UNK WWAudioClass::Get_2D_Sample(const AudibleSoundClass&)
AT2(0x00514050, 0x005138F0);



RENEGADE_FUNCTION
UNK WWAudioClass::Get_3D_Sample(const Sound3DClass&)
AT2(0x005141E0, 0x00513A80);



RENEGADE_FUNCTION
UNK WWAudioClass::Get_Listener_Handle()
AT2(0x00514370, 0x00513C10);



RENEGADE_FUNCTION
UNK WWAudioClass::Build_3D_Driver_List()
AT2(0x005143A0, 0x00513C40);



RENEGADE_FUNCTION
UNK WWAudioClass::Free_3D_Driver_List()
AT2(0x00514560, 0x00513E00);



RENEGADE_FUNCTION
UNK WWAudioClass::Select_3D_Device(const char*)
AT2(0x00514690, 0x00513F30);



RENEGADE_FUNCTION
UNK WWAudioClass::Select_3D_Device(const char*, uint32)
AT2(0x005146F0, 0x00513F90);



RENEGADE_FUNCTION
UNK WWAudioClass::Select_3D_Device(int)
AT2(0x00514940, 0x005141E0);



RENEGADE_FUNCTION
UNK WWAudioClass::Select_3D_Device(WWAudioClass::DRIVER_TYPE_3D)
AT2(0x00514970, 0x00514210);



RENEGADE_FUNCTION
UNK WWAudioClass::Find_3D_Device(WWAudioClass::DRIVER_TYPE_3D)
AT2(0x005149B0, 0x00514250);



RENEGADE_FUNCTION
UNK WWAudioClass::Allocate_3D_Handles()
AT2(0x00514A60, 0x00514300);



RENEGADE_FUNCTION
UNK WWAudioClass::Release_3D_Handles()
AT2(0x00514B60, 0x00514400);



RENEGADE_FUNCTION
UNK WWAudioClass::Validate_3D_Sound_Buffer(SoundBufferClass*)
AT2(0x00514BD0, 0x00514470);



RENEGADE_FUNCTION
UNK WWAudioClass::ReAssign_2D_Handles()
AT2(0x00514C10, 0x005144B0);



RENEGADE_FUNCTION
UNK WWAudioClass::ReAssign_3D_Handles()
AT2(0x00514C90, 0x00514530);



RENEGADE_FUNCTION
UNK WWAudioClass::Remove_2D_Sound_Handles()
AT2(0x00514CF0, 0x00514590);



RENEGADE_FUNCTION
UNK WWAudioClass::Remove_3D_Sound_Handles()
AT2(0x00514D40, 0x005145E0);



RENEGADE_FUNCTION
UNK WWAudioClass::Set_Dialog_Volume(float)
AT2(0x00514D90, 0x00514630);



RENEGADE_FUNCTION
UNK WWAudioClass::Set_Cinematic_Volume(float)
AT2(0x00514E30, 0x005146D0);



RENEGADE_FUNCTION
UNK WWAudioClass::Set_Sound_Effects_Volume(float)
AT2(0x00514ED0, 0x00514770);



RENEGADE_FUNCTION
UNK WWAudioClass::Set_Music_Volume(float)
AT2(0x00514FA0, 0x00514840);



RENEGADE_FUNCTION
UNK WWAudioClass::Internal_Set_Sound_Effects_Volume(float)
AT2(0x00515070, 0x00514910);



RENEGADE_FUNCTION
UNK WWAudioClass::Internal_Set_Music_Volume(float)
AT2(0x00515110, 0x005149B0);



RENEGADE_FUNCTION
bool WWAudioClass::Is_Disabled()
AT2(0x005151B0, 0x00514A50);



RENEGADE_FUNCTION
UNK WWAudioClass::Initialize(const char*)
AT2(0x00515250, 0x00514AF0);



RENEGADE_FUNCTION
UNK WWAudioClass::Initialize(bool, int, int)
AT2(0x00515350, 0x00514BF0);



RENEGADE_FUNCTION
UNK WWAudioClass::Shutdown()
AT2(0x00515450, 0x00514CF0);



RENEGADE_FUNCTION
UNK WWAudioClass::Register_EOS_Callback(void (*)(SoundSceneObjClass*, uint32), uint32)
AT2(0x005156C0, 0x00514F60);



RENEGADE_FUNCTION
UNK WWAudioClass::UnRegister_EOS_Callback(void (*)(SoundSceneObjClass*, uint32))
AT2(0x00515720, 0x00514FC0);



RENEGADE_FUNCTION
UNK WWAudioClass::Register_Text_Callback(void (*)(AudibleSoundClass*, const StringClass&, uint32), uint32)
AT2(0x005157A0, 0x00515040);



RENEGADE_FUNCTION
UNK WWAudioClass::UnRegister_Text_Callback(void (*)(AudibleSoundClass*, const StringClass&, uint32))
AT2(0x00515800, 0x005150A0);



RENEGADE_FUNCTION
void WWAudioClass::Fire_Text_Callback(AudibleSoundClass*, const StringClass&)
AT2(0x00515880, 0x00515120);



RENEGADE_FUNCTION
UNK WWAudioClass::Allow_Sound_Effects(bool)
AT2(0x005158F0, 0x00515190);



RENEGADE_FUNCTION
UNK WWAudioClass::Allow_Music(bool)
AT2(0x00515A50, 0x005152F0);



RENEGADE_FUNCTION
UNK WWAudioClass::Allow_Dialog(bool)
AT2(0x00515BC0, 0x00515460);



RENEGADE_FUNCTION
UNK WWAudioClass::Allow_Cinematic_Sound(bool)
AT2(0x00515D30, 0x005155D0);



RENEGADE_FUNCTION
void WWAudioClass::Simple_Play_2D_Sound_Effect(const char*, float, float)
AT2(0x00515EA0, 0x00515740);



RENEGADE_FUNCTION
void WWAudioClass::Simple_Play_2D_Sound_Effect(FileClass&, float, float)
AT2(0x00516030, 0x005158D0);



RENEGADE_FUNCTION
UNK WWAudioClass::Get_File(const char*)
AT2(0x005161D0, 0x00515A70);



RENEGADE_FUNCTION
UNK WWAudioClass::Return_File(FileClass*)
AT2(0x005161F0, 0x00515A90);



RENEGADE_FUNCTION
UNK WWAudioClass::Create_Logical_Sound()
AT2(0x00516220, 0x00515AC0);



RENEGADE_FUNCTION
UNK WWAudioClass::Create_Logical_Listener()
AT2(0x00516240, 0x00515AE0);



RENEGADE_FUNCTION
UNK WWAudioClass::Add_Logical_Type(int, const char*)
AT2(0x00516260, 0x00515B00);



RENEGADE_FUNCTION
UNK WWAudioClass::Reset_Logical_Types()
AT2(0x005163A0, 0x00515C40);



RENEGADE_FUNCTION
UNK WWAudioClass::Get_Logical_Type(int, StringClass&)
AT2(0x005163D0, 0x00515C70);



RENEGADE_FUNCTION
UNK WWAudioClass::Find_Sound_Object(uint32)
AT2(0x00516460, 0x00515D00);



RENEGADE_FUNCTION
bool WWAudioClass::Load_From_Registry(const char*)
AT2(0x005164A0, 0x00515D40);



RENEGADE_FUNCTION
bool WWAudioClass::Load_From_Registry(const char*, StringClass&, bool&, int&, int&, bool&, bool&, bool&, bool&, float&, float&, float&, float&, int&)
AT2(0x00516630, 0x00515ED0);



RENEGADE_FUNCTION
bool WWAudioClass::Save_To_Registry(const char*)
AT2(0x00516980, 0x00516220);



RENEGADE_FUNCTION
bool WWAudioClass::Save_To_Registry(const char*, const StringClass&, bool, int, int, bool, bool, bool, bool, float, float, float, float, int)
AT2(0x00516A80, 0x00516320);



RENEGADE_FUNCTION
UNK WWAudioClass::File_Open_Callback(const char*, uint32*)
AT2(0x00516C10, 0x005164B0);



RENEGADE_FUNCTION
UNK WWAudioClass::File_Close_Callback(uint32)
AT2(0x00516C60, 0x00516500);



RENEGADE_FUNCTION
UNK WWAudioClass::File_Seek_Callback(uint32, sint32, uint32)
AT2(0x00516C90, 0x00516530);



RENEGADE_FUNCTION
UNK WWAudioClass::File_Read_Callback(uint32, void*, uint32)
AT2(0x00516CF0, 0x00516590);



RENEGADE_FUNCTION
UNK WWAudioClass::Fade_Background_Music(const char*, int, int)
AT2(0x00516D10, 0x005165B0);



RENEGADE_FUNCTION
UNK WWAudioClass::Set_Background_Music(const char*)
AT2(0x00516FB0, 0x00516850);



RENEGADE_FUNCTION
UNK WWAudioClass::Set_Active_Sound_Page(WWAudioClass::SOUND_PAGE)
AT2(0x00517250, 0x00516AF0);



RENEGADE_FUNCTION
UNK WWAudioClass::Fade_Non_Dialog_In()
AT2(0x005172E0, 0x00516B80);



RENEGADE_FUNCTION
UNK WWAudioClass::Fade_Non_Dialog_Out()
AT2(0x00517310, 0x00516BB0);



RENEGADE_FUNCTION
UNK WWAudioClass::Update_Fade()
AT2(0x00517340, 0x00516BE0);



RENEGADE_FUNCTION
UNK WWAudioClass::Peek_2D_Sample(int)
AT2(0x00517540, 0x00516DE0);



RENEGADE_FUNCTION
UNK WWAudioClass::Peek_3D_Sample(int)
AT2(0x00517590, 0x00516E30);



RENEGADE_FUNCTION
bool WWAudioClass::Acquire_Virtual_Channel(AudibleSoundClass*, int)
AT2(0x005175E0, 0x00516E80);



RENEGADE_FUNCTION
UNK WWAudioClass::Release_Virtual_Channel(AudibleSoundClass*, int)
AT2(0x005176A0, 0x00516F40);



RENEGADE_FUNCTION
UNK WWAudioClass::Set_Speaker_Type(int)
AT2(0x005176E0, 0x00516F80);



RENEGADE_FUNCTION
UNK WWAudioClass::Get_Speaker_Type()
AT2(0x00517700, 0x00516FA0);



RENEGADE_FUNCTION
UNK WWAudioClass::Push_Active_Sound_Page(WWAudioClass::SOUND_PAGE)
AT2(0x00517710, 0x00516FB0);



RENEGADE_FUNCTION
UNK WWAudioClass::Pop_Active_Sound_Page()
AT2(0x00517770, 0x00517010);



RENEGADE_FUNCTION
UNK WWAudioClass::Temp_Disable_Audio(bool)
AT2(0x005177C0, 0x00517060);



RENEGADE_FUNCTION
bool WWAudioClass::Load_Default_Volume(int&, int&, int&, int&)
AT2(0x00517840, 0x005170E0);
