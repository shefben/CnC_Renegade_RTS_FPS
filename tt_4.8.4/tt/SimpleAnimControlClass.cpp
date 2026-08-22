#include "general.h"

#include "SimpleAnimControlClass.h"
#include "WW3DAssetManager.h"
#include "RenderObjClass.h"

RENEGADE_FUNCTION
bool AnimChannelClass::Load
   (ChunkLoadClass& chunkLoader)
   AT2(0x00724C60,0x00724220);


RENEGADE_FUNCTION
bool AnimChannelClass::Save
   (ChunkSaveClass& chunkSaver)
   AT2(0x00724BB0,0x00724170);

AnimChannelClass::AnimChannelClass() :
	Animation(NULL),
	Frame(0.0f),
	NumFrames(1),
	TargetFrame(0.0f),
	Mode(ANIM_MODE_ONCE)
{
}

AnimChannelClass::~AnimChannelClass()
{
	if (Animation)
	{
		Animation->Release_Ref();
		Animation = NULL;
	}
}

AnimChannelClass & AnimChannelClass::operator = (const AnimChannelClass & src)
{
	if (Animation != NULL)
	{
		Animation->Release_Ref();
	}
	Animation = src.Animation;
	if (Animation != NULL)
	{
		Animation->Add_Ref();
	}
	Frame = src.Frame;
	NumFrames = src.NumFrames;
	Mode = src.Mode;
	TargetFrame	= src.TargetFrame;
	return *this;
}

void AnimChannelClass::Set_Animation(const char *name)
{
	if ((Animation != NULL) && (name != NULL))
	{
		if (_stricmp(Animation->Get_Name(), name) == 0)
		{
			return;
		}
	}
	if ((Animation == NULL) && (name == NULL))
	{
		return;
	}
	if (Animation)
	{
		Animation->Release_Ref();
		Animation = NULL;
	}
	if ((name != NULL) && (name[0] != 0))
	{
		Animation = WW3DAssetManager::TheInstance->Get_HAnim(name);
	}
	if (Animation)
	{
		NumFrames = (float)Animation->Get_Num_Frames();
		Mode = ANIM_MODE_ONCE;
		Frame = 0;
		TargetFrame = 0;
	}
}

void AnimChannelClass::Set_Animation(const HAnimClass *anim)
{
	if (Animation == anim )
	{
		return;
	}
	if (Animation)
	{
		Animation->Release_Ref();
		Animation = NULL;
	}
	Animation = (HAnimClass *)anim;
	if (Animation)
	{
		Animation->Add_Ref();
		NumFrames = (float)Animation->Get_Num_Frames();
		Mode = ANIM_MODE_ONCE;
		Frame = 0;
		TargetFrame = 0;
	}
}

void AnimChannelClass::Set_Mode(AnimMode mode, float frame)
{ 
	Mode = mode; 
	if (frame >= 0)
	{
		Frame = frame; 
	}
}

void AnimChannelClass::Update(float dtime)
{
	if (Mode == ANIM_MODE_STOP)
	{
		return;
	}
	if (Animation != NULL)
	{
		switch (Mode)
		{		
			case ANIM_MODE_LOOP:
				Frame += dtime * Animation->Get_Frame_Rate();
				if (Frame >= NumFrames-1)
				{
					Frame -= NumFrames-1;
				}
				if (Frame >= NumFrames)
				{
					Frame = 0;
				}
				break;
			case ANIM_MODE_TARGET:
				if (Frame < TargetFrame)
				{
					Frame += dtime * Animation->Get_Frame_Rate();
					if (Frame >= TargetFrame)
					{
						Frame = TargetFrame;
					}
				}
				else if (Frame > TargetFrame)
				{
					Frame -= dtime * Animation->Get_Frame_Rate();
					if (Frame <= TargetFrame)
					{
						Frame = TargetFrame;
					}
				}
				break;
			case ANIM_MODE_ONCE:
				Frame += dtime * Animation->Get_Frame_Rate();
				if (Frame > NumFrames-1)
				{
					Frame = NumFrames-1;
				}
				break;
		}
	}
}

void AnimChannelClass::Get_Animation_Data(AnimationDataList & list, float weight)
{
	if (Animation != NULL && weight > 0)
	{
		AnimationDataRecord * record = list.Uninitialized_Add();
		record->Animation = Animation;
		record->Frame = Frame;
		record->Weight = weight;
	}
}

void AnimChannelClass::Update_Model(RenderObjClass	*anim_model)
{
	if (Animation)
	{
		anim_model->Set_Animation(Animation, Frame, RenderObjClass::ANIM_MODE_MANUAL);
	}
	else
	{
		anim_model->Set_Animation();
	}
}

BlendableAnimChannelClass::BlendableAnimChannelClass(void) :
	BlendTimer(0),
	BlendTotal(0)
{
}

void BlendableAnimChannelClass::Set_Animation(const char *name, float blendtime, float start_frame)
{
	if ((NewChannel.Peek_Animation() == NULL) && (name == NULL))
	{
		return;
	}

	if (( NewChannel.Peek_Animation() != NULL) && (name != NULL))
	{
		if (_stricmp(NewChannel.Peek_Animation()->Get_Name(), name) == 0)
		{
			return;
		}
	}
	if ((NewChannel.Peek_Animation() == NULL) || (blendtime == 0) || (name == NULL))
	{
		BlendTotal = 0.0f;
		BlendTimer = 0.0f;
	}
	else if (BlendTotal == 0.0)
	{
		OldChannel = NewChannel;	
		BlendTimer = 0.0f;
		BlendTotal = blendtime;
	}
	else if ((BlendTimer / BlendTotal) > 0.5)
	{
		OldChannel = NewChannel;
		BlendTimer = (1.0f - (BlendTimer / BlendTotal)) * blendtime;
		BlendTotal = blendtime;
	}
	else
	{
		BlendTimer = (BlendTimer / BlendTotal) * blendtime;
		BlendTotal = blendtime;
	}
	NewChannel.Set_Animation(name);
	if (NewChannel.Peek_Animation() != NULL)
	{
		NewChannel.Set_Frame(start_frame);
	}
	if (name == NULL)
	{
		OldChannel.Set_Animation((const char *)NULL);
	}
}

void BlendableAnimChannelClass::Set_Animation(const HAnimClass *anim, float blendtime, float start_frame)
{
	if ((NewChannel.Peek_Animation() == NULL) && (anim == NULL))
	{
		return;
	}
	if ((NewChannel.Peek_Animation() != NULL) && (anim != NULL))
	{
		if (NewChannel.Peek_Animation() == anim)
		{
			return;
		}
	}
	if ((NewChannel.Peek_Animation() == NULL) || (blendtime == 0) || (anim == NULL))
	{
		BlendTotal = 0.0f;
		BlendTimer = 0.0f;
	}
	else if (BlendTotal == 0.0)
	{
		OldChannel = NewChannel;	
		BlendTimer = 0.0f;
		BlendTotal = blendtime;
	}
	else if ((BlendTimer / BlendTotal) > 0.5)
	{
		OldChannel = NewChannel;
		BlendTimer = (1.0f - (BlendTimer / BlendTotal)) * blendtime;
		BlendTotal = blendtime;
	}
	else
	{
		BlendTimer = (BlendTimer / BlendTotal) * blendtime;
		BlendTotal = blendtime;
	}
	NewChannel.Set_Animation(anim);
	if (NewChannel.Peek_Animation() != NULL)
	{
		NewChannel.Set_Frame(start_frame);
	}
	if (anim == NULL)
	{
		OldChannel.Set_Animation((const HAnimClass *)NULL);
	}
}

void BlendableAnimChannelClass::Update(float dtime)
{
	if (BlendTotal != 0.0f)
	{
		BlendTimer += dtime;
		if (BlendTimer >= BlendTotal)
		{
			BlendTotal = 0.0f;
			BlendTimer = 0.0f;
			OldChannel.Set_Animation((const char *)NULL);
		}
	}
	NewChannel.Update(dtime);
	OldChannel.Update(dtime);
}

float Clamp(float val,float min,float max);
void BlendableAnimChannelClass::Get_Animation_Data(AnimationDataList & list, float weight)
{
	float blend_ratio = 1.0f;
	if (BlendTotal != 0.0f)
	{
		blend_ratio = Clamp(BlendTimer / BlendTotal, 0, 1);
	}
	NewChannel.Get_Animation_Data(list, weight * blend_ratio);
	OldChannel.Get_Animation_Data(list, weight * (1 - blend_ratio));
}


void BlendableAnimChannelClass::Update_Model(RenderObjClass *anim_model)
{
	float blend_ratio = 1.0f;
	if (BlendTotal != 0.0f)
	{
		blend_ratio = Clamp(BlendTimer / BlendTotal, 0, 1);
	}
	if (OldChannel.Peek_Animation())
	{
		anim_model->Set_Animation(OldChannel.Peek_Animation(),OldChannel.Get_Frame(),
										NewChannel.Peek_Animation(),NewChannel.Get_Frame(),
										blend_ratio);
	}
	else if (NewChannel.Peek_Animation())
	{
		anim_model->Set_Animation(NewChannel.Peek_Animation(),NewChannel.Get_Frame(),RenderObjClass::ANIM_MODE_MANUAL);
	}
	else
	{
		anim_model->Set_Animation();
	}
}

RENEGADE_FUNCTION
bool BlendableAnimChannelClass::Load
   (ChunkLoadClass& chunkLoader)
   AT2(0x00725120,0x007246E0);


RENEGADE_FUNCTION
bool BlendableAnimChannelClass::Save
   (ChunkSaveClass& chunkSaver)
   AT2(0x00725080,0x00724640);

RENEGADE_FUNCTION
bool SimpleAnimControlClass::Load
   (ChunkLoadClass& chunkLoader)
   AT2(0x00725A80,0x00725040);


RENEGADE_FUNCTION
bool SimpleAnimControlClass::Save
   (ChunkSaveClass& chunkSaver)
   AT2(0x00725A10,0x00724FD8);

SimpleAnimControlClass::SimpleAnimControlClass( void ) 
{
}

SimpleAnimControlClass::~SimpleAnimControlClass( void )
{
}

void SimpleAnimControlClass::Set_Animation(const char *name, float blendtime, float start_frame)
{
	Channel.Set_Animation(name, blendtime, start_frame);
}

void SimpleAnimControlClass::Set_Animation(const HAnimClass * anim, float blendtime, float start_frame)
{
	Channel.Set_Animation(anim, blendtime, start_frame);
}

void SimpleAnimControlClass::Update(float dtime)
{
	Channel.Update(dtime);
	Channel.Update_Model(Model);
}
