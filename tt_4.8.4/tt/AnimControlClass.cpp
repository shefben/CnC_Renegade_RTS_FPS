#include "general.h"

#include "AnimControlClass.h"
#include "RenderObjClass.h"

AnimControlClass::AnimControlClass() : Model(NULL)
{
}

AnimControlClass::~AnimControlClass()
{
	REF_PTR_RELEASE(Model);
}

RENEGADE_FUNCTION
bool AnimControlClass::Load
   (ChunkLoadClass& chunkLoader)
   AT2(0x00725800,0x00724220);

RENEGADE_FUNCTION
bool AnimControlClass::Save
   (ChunkSaveClass& chunkSaver)
   AT2(0x007257C0,0x00724170);

void AnimControlClass::Set_Model
   (RenderObjClass* anim_model)
{
	REF_PTR_SET(Model, anim_model);
}
