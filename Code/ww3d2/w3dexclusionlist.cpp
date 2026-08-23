/***********************************************************************************************
 ***                            Confidential - Westwood Studios                              ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando / G 3D Library                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/ww3d2/w3dexclusionlist.cpp                   $*
 *                                                                                             *
 *                       Author:: Greg_h                                                       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   W3DExclusionListClass::Is_Excluded -- is this prototype named, or a child of one named    *
 *   W3DExclusionListClass::Is_Excluded -- is this hierarchy tree named                        *
 *   W3DExclusionListClass::Is_Excluded -- is the file this animation came from named          *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "w3dexclusionlist.h"
#include "proto.h"
#include "htree.h"
#include "hanim.h"

#include <string.h>


/*
** Names are matched without regard to case.  Stock Renegade content is inconsistent about
** it -- the hierarchy tree manager already lower-cases every name it hashes -- and a name
** that fails to match here silently frees an asset somebody asked to keep.
*/
static StringClass Normalized(const char * name)
{
	StringClass lower_case_name(name,true);
	if (lower_case_name.Get_Length() > 0) {
		_strlwr(lower_case_name.Peek_Buffer());
	}
	return lower_case_name;
}


W3DExclusionListClass::W3DExclusionListClass(const DynamicVectorClass<StringClass> & names) :
	Names(names)
{
	for (int i=0; i<Names.Count(); i++) {
		NameHash.Insert(Normalized(Names[i]),i);
	}
}


W3DExclusionListClass::~W3DExclusionListClass(void)
{
	NameHash.Remove_All();
}


/***********************************************************************************************
 * W3DExclusionListClass::Is_Excluded -- is this prototype named, or a child of one named      *
 *                                                                                             *
 * A run-time munged prototype (its name contains a '#') is never excluded; it belongs to       *
 * whatever built it.  A sub-object prototype "<file>.<subobject>" is excluded when its file    *
 * is named, which is how naming one model keeps the whole model.                               *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool	W3DExclusionListClass::Is_Excluded(PrototypeClass * proto) const
{
	return Is_Excluded_Model_Name(proto->Get_Name());
}


bool	W3DExclusionListClass::Is_Excluded_Model_Name(const char * name) const
{
	if (name == nullptr) {
		return false;
	}

	StringClass copy(name,true);
	char * root_name = copy.Peek_Buffer();

	if (strchr(root_name,'#') != nullptr) {
		return false;
	}

	char * tmp = strchr(root_name,'.');
	if (tmp != nullptr) {
		*tmp = 0;
	}

	return Is_Excluded(root_name);
}


/***********************************************************************************************
 * W3DExclusionListClass::Is_Excluded -- is this hierarchy tree named                          *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool	W3DExclusionListClass::Is_Excluded(HTreeClass * htree) const
{
	return Is_Excluded(htree->Get_Name());
}


/***********************************************************************************************
 * W3DExclusionListClass::Is_Excluded -- is the file this animation came from named            *
 *                                                                                             *
 * Animations are named "<skeleton>.<animname>", and the part after the '.' is the file they    *
 * were loaded from.  An animation with no '.' did not come from a file we can name.            *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool	W3DExclusionListClass::Is_Excluded(HAnimClass * hanim) const
{
	return Is_Excluded_Anim_Name(hanim->Get_Name());
}


bool	W3DExclusionListClass::Is_Excluded_Anim_Name(const char * name) const
{
	if (name == nullptr) {
		return false;
	}

	const char * tmp = strchr(name,'.');
	if (tmp != nullptr) {
		return Is_Excluded(tmp + 1);
	}

	//	No '.' means no file we can name, so it cannot be excluded by name.
	return false;
}


bool W3DExclusionListClass::Is_Excluded(const char * root_name) const
{
	if (root_name == nullptr) {
		return false;
	}
	return NameHash.Exists(Normalized(root_name));
}
