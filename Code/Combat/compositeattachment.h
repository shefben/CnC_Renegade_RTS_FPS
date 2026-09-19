/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 OpenW3D contributors
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *	Roadmap Section 29, Zero Hour / SAGE feature 18 -- the "composite child-model attachment     *
 *	controller" named in the Initial components list, and the concrete answer to the section's    *
 *	"Zero Hour composite/dynamic attachment requirement": child model attached to a named W3D     *
 *	node/bone, runtime add/remove of that child visual, independent child animation state,        *
 *	turret/module attachment transforms, and visibility/damage-state propagation.  One            *
 *	generalized implementation, usable by vehicles and by the building semantic descriptor         *
 *	system alike, rather than a special class per donor unit -- exactly the instruction the       *
 *	section gives for Zero Hour's expanded Overlord-style attachment modules.                      *
 *                                                                                             *
 *	Two of the six requirement bullets need no new code at all, because RenderObjClass and        *
 *	HLodClass (Code/ww3d2/rendobj.h, hlod.h) already carry them for every bone-attached sub-       *
 *	object, stock Renegade included: parent-relative transform propagation is the HTree doing      *
 *	its job -- a sub-object attached with Add_Sub_Object_To_Bone rides its bone automatically,      *
 *	with no per-tick reposition call anywhere in the existing soldier weapon-attach code this      *
 *	class follows the convention of (Code/Combat/soldier.cpp, WeaponRenderModel) -- and             *
 *	LOD/culling propagation is HLodClass's own Prepare_LOD/Increment_LOD/Decrement_LOD chain,       *
 *	which already walks every sub-object.  This class exists for the four bullets that were         *
 *	genuinely missing: a named slot to add/remove a child by, and the animation and hidden-state    *
 *	control a caller needs per slot.                                                                *
 *                                                                                             *
 *	A slot's child model comes from WW3DAssetManager::Create_Render_Obj by name -- the missing-     *
 *	art rule applies the same way it did to P25's GAMEPLAY_DEBRIS fragments: a name that does       *
 *	not resolve fails the Attach call sanely (false, nothing added, WWDEBUG_SAY) rather than         *
 *	crashing or drawing a placeholder.  See docs/zerohour/ModularDrawComponents.md.                  *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef COMPOSITEATTACHMENT_H
#define COMPOSITEATTACHMENT_H

#include "always.h"
#include "vector.h"
#include "wwstring.h"

class RenderObjClass;


class CompositeAttachmentClass
{
	public:

		CompositeAttachmentClass(void);
		~CompositeAttachmentClass(void);

		/*
		**	Each slot remembers its own host, so one controller can hold slots on several
		**	different models at once -- which is what makes this usable by a building, whose
		**	visible geometry is a set of separate aggregates rather than one model, as well as
		**	by a vehicle, which has exactly one.  A slot holds a reference to its host and to
		**	its child, so neither can be freed out from under an attachment; Detach releases
		**	both, and the destructor detaches anything still attached.
		*/
		bool	Attach(RenderObjClass * host_model,const char * slot_name,const char * bone_name,
						  const char * child_model_name);
		void	Detach(const char * slot_name);
		void	Detach_All(void);

		//	False if the slot does not exist, or its child model has no such animation.
		bool	Set_Slot_Animation(const char * slot_name,const char * anim_name,bool looping);

		//	The damage-state hook: hide/show an attached module without detaching it.
		void	Set_Slot_Hidden(const char * slot_name,bool hidden);

		bool	Is_Slot_Attached(const char * slot_name) const;
		int		Get_Slot_Count(void) const;

	private:

		struct AttachmentSlotStruct
		{
			StringClass			Name;
			RenderObjClass *	Host;		// one held reference each, both released on Detach
			RenderObjClass *	Child;

			//	DynamicVectorClass instantiates these whether or not they are called; slots are
			//	only ever removed by index, never by value.  Same stubs AnimationDataRecord
			//	(Code/Combat/animcontrol.h) already uses for the same reason.
			bool operator == (AttachmentSlotStruct const &) const	{ return false; }
			bool operator != (AttachmentSlotStruct const &) const	{ return true; }
		};

		int		Find_Slot(const char * slot_name) const;

		DynamicVectorClass<AttachmentSlotStruct>	Slots;

		//	Not implemented -- a controller's slots are tied to one owner's bookkeeping.
		CompositeAttachmentClass(const CompositeAttachmentClass &);
		CompositeAttachmentClass & operator=(const CompositeAttachmentClass &);
};

#endif // COMPOSITEATTACHMENT_H
