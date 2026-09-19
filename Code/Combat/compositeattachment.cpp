#include "compositeattachment.h"

#include "assetmgr.h"
#include "hanim.h"
#include "rendobj.h"
#include "wwdebug.h"

CompositeAttachmentClass::CompositeAttachmentClass(void)
{
}

CompositeAttachmentClass::~CompositeAttachmentClass(void)
{
	Detach_All();
}

int CompositeAttachmentClass::Find_Slot(const char * slot_name) const
{
	for (int index = 0; index < Slots.Count(); index++) {
		if (Slots[index].Name == slot_name) {
			return (index);
		}
	}
	return (-1);
}

bool CompositeAttachmentClass::Attach(RenderObjClass * host_model,const char * slot_name,
	const char * bone_name,const char * child_model_name)
{
	WWASSERT(host_model != nullptr);
	WWASSERT(slot_name != nullptr);
	WWASSERT(bone_name != nullptr);

	if (Find_Slot(slot_name) >= 0) {
		//	Re-attaching an occupied slot is a caller bug, not a runtime case -- detach first.
		WWDEBUG_SAY(("CompositeAttachmentClass::Attach() : slot '%s' already attached\n",slot_name));
		return (false);
	}

	RenderObjClass * child = WW3DAssetManager::Get_Instance()->Create_Render_Obj(child_model_name);
	if (child == nullptr) {
		//	Missing art: fail sanely, draw nothing for this slot, do not crash or fake a shape.
		WWDEBUG_SAY(("CompositeAttachmentClass::Attach() : model '%s' not found for slot '%s'\n",
			child_model_name,slot_name));
		return (false);
	}

	host_model->Add_Sub_Object_To_Bone(child,bone_name);

	AttachmentSlotStruct slot;
	slot.Name = slot_name;
	slot.Host = host_model;
	slot.Host->Add_Ref();
	slot.Child = child;			// Create_Render_Obj's reference, kept rather than released
	Slots.Add(slot);
	return (true);
}

void CompositeAttachmentClass::Detach(const char * slot_name)
{
	int index = Find_Slot(slot_name);
	if (index < 0) {
		return;
	}

	Slots[index].Host->Remove_Sub_Object(Slots[index].Child);
	Slots[index].Child->Release_Ref();
	Slots[index].Host->Release_Ref();
	Slots.Delete(index);
}

void CompositeAttachmentClass::Detach_All(void)
{
	for (int index = 0; index < Slots.Count(); index++) {
		Slots[index].Host->Remove_Sub_Object(Slots[index].Child);
		Slots[index].Child->Release_Ref();
		Slots[index].Host->Release_Ref();
	}
	Slots.Delete_All();
}

bool CompositeAttachmentClass::Set_Slot_Animation(const char * slot_name,const char * anim_name,bool looping)
{
	int index = Find_Slot(slot_name);
	if (index < 0) {
		return (false);
	}

	HAnimClass * anim = WW3DAssetManager::Get_Instance()->Get_HAnim(anim_name);
	if (anim == nullptr) {
		WWDEBUG_SAY(("CompositeAttachmentClass::Set_Slot_Animation() : animation '%s' not found\n",
			anim_name));
		return (false);
	}

	//	The child's own animation state, independent of whatever the host is playing -- which is
	//	the point of the "independent child animation state" requirement.  A sub-object attached
	//	to a bone follows that bone's transform no matter what it is animating itself.
	Slots[index].Child->Set_Animation(anim,0.0f,
		looping ? RenderObjClass::ANIM_MODE_LOOP : RenderObjClass::ANIM_MODE_ONCE);
	anim->Release_Ref();
	return (true);
}

void CompositeAttachmentClass::Set_Slot_Hidden(const char * slot_name,bool hidden)
{
	int index = Find_Slot(slot_name);
	if (index >= 0) {
		Slots[index].Child->Set_Hidden(hidden ? 1 : 0);
	}
}

bool CompositeAttachmentClass::Is_Slot_Attached(const char * slot_name) const
{
	return (Find_Slot(slot_name) >= 0);
}

int CompositeAttachmentClass::Get_Slot_Count(void) const
{
	return (Slots.Count());
}
