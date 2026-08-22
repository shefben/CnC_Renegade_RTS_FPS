#include "general.h"

#include "BuildingAggregateDefClass.h"
//#include "scripts.h"
//#include "engine_common.h"
//#include "engine_vector.h"
//#include "engine_io.h"

BuildingAggregateDefClass* BuildingAggregateDefClass::repairBayAnimation = 0;



bool BuildingAggregateDefClass::Load
   (ChunkLoadClass& chunkLoader)
{
   for (; chunkLoader.Open_Chunk(); chunkLoader.Close_Chunk())
      switch (chunkLoader.Cur_Chunk_ID())
      {
      case 8281441:
         StaticAnimPhysDefClass::Load (chunkLoader);
         break;

      case 8281443:
      case 8281444:
      case 8281445:
      case 8281446:
      case 8281447:
      case 8281448:
      case 8281449:
      case 8281450:
      case 8281451:
      case 8281452:
         this->Load_State_Animation_Data (chunkLoader, chunkLoader.Cur_Chunk_ID() - 8281443);
         break;

      case 8281442:
         for (; chunkLoader.Open_Micro_Chunk(); chunkLoader.Close_Micro_Chunk())
            switch (chunkLoader.Cur_Micro_Chunk_ID())
            {
            case 0:
               chunkLoader.Read (&this->mode, 4);
               break;

            case 4:
               chunkLoader.Read (&this->terminal, 1);
               break;
            }
      }

   if (::strstr (this->ModelName, "rep^nod_fx"))
      repairBayAnimation = this;

   return true;
}



RENEGADE_FUNCTION
void BuildingAggregateDefClass::Load_State_Animation_Data
   (ChunkLoadClass& chunkLoader, uint32 stateIndex)
   AT2(0x00723F80,0x00723540);
