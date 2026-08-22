#include "general.h"

#include "SaveLoadSystemClass.h"
REF_DEF2(SaveLoadSystemClass::PointerRemapper, PointerRemapClass, 0x00830398, 0x0082F580);

RENEGADE_FUNCTION
SaveLoadSubSystemClass* SaveLoadSystemClass::Find_Sub_System
   (uint32 id)
   AT2(0x00525B40,0x005253E0);

RENEGADE_FUNCTION
void SaveLoadSystemClass::Post_Load_Processing
   (void(*)())
   AT2(0x00525980,0x00525220);

bool SaveLoadSystemClass::Load
   (ChunkLoadClass& chunkLoader, bool auto_post_load)
{
	SaveLoadSystemClass::PointerRemapper.Reset();

	bool allSucceeded = true;

	for (; chunkLoader.Open_Chunk(); chunkLoader.Close_Chunk())
	{
		// DEPRECATED: SaveLoadStatus::Inc_Status_Count();
		SaveLoadSubSystemClass* subSystem = SaveLoadSystemClass::Find_Sub_System (chunkLoader.Cur_Chunk_ID());
		if (subSystem)
		{
		   // SaveLoadStatus::Set_Status_Text (subSystem->Name(), 1);
		   allSucceeded = allSucceeded && subSystem->Load (chunkLoader);
		}
	}

   SaveLoadSystemClass::PointerRemapper.Process();
   SaveLoadSystemClass::PointerRemapper.Reset();

   if (auto_post_load)
      SaveLoadSystemClass::Post_Load_Processing (0);

   return allSucceeded;
}

RENEGADE_FUNCTION
void SaveLoadSystemClass::Register_Pointer
   (void*, void*)
   AT2(0x00525D20,0x005255C0);



RENEGADE_FUNCTION
void SaveLoadSystemClass::Register_Post_Load_Callback
   (PostLoadableClass*)
   AT2(0x00525C20,0x005254C0);

RENEGADE_FUNCTION
void SaveLoadSystemClass::Request_Pointer_Remap
   (void**)
   AT2(0x00525D40,0x005255E0);

bool SaveLoadSubSystemClass::Contains_Data() const
{
	return true;
}

SaveLoadSubSystemClass::SaveLoadSubSystemClass()
{
	s8 = 0;
	SaveLoadSystemClass::Register_Sub_System(this);
}

SaveLoadSubSystemClass::~SaveLoadSubSystemClass()
{
	SaveLoadSystemClass::Unregister_Sub_System(this);
}

RENEGADE_FUNCTION
void SaveLoadSystemClass::Register_Sub_System(SaveLoadSubSystemClass *)
AT2(0x00525AE0,0x00525380);

RENEGADE_FUNCTION
void SaveLoadSystemClass::Unregister_Sub_System(SaveLoadSubSystemClass *)
AT2(0x00525B00,0x005253A0);
