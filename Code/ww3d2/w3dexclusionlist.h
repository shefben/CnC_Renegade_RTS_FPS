/***********************************************************************************************
 ***                            Confidential - Westwood Studios                              ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando / G 3D Library                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/ww3d2/w3dexclusionlist.h                     $*
 *                                                                                             *
 *                       Author:: Greg_h                                                       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef W3DEXCLUSIONLIST_H
#define W3DEXCLUSIONLIST_H

#include "always.h"
#include "vector.h"
#include "wwstring.h"
#include "hashtemplate.h"

class PrototypeClass;
class HTreeClass;
class HAnimClass;

/*
** W3DExclusionListClass
** This class encapsulates an "exclusion list" which the asset manager and related classes use
** to filter what resources get released.  This is useful between level loads for example.
** The Is_Excluded function uses w3d naming convention assumptions to determine whether the given
** asset name is in the list or is a child of something in the list.
**
** The naming conventions it relies on are the ones stock Renegade assets already use:
** a sub-object is "<file>.<subobject>", a run-time munged object contains a '#', and an
** animation is "<skeleton>.<animname>" where the part after the '.' is the file it came from.
** No asset has to be re-authored or re-exported for this to work.
*/
class W3DExclusionListClass
{
public:
	W3DExclusionListClass(const DynamicVectorClass<StringClass> & names);
	~W3DExclusionListClass(void);

	bool	Is_Excluded(PrototypeClass * proto) const;
	bool	Is_Excluded(HTreeClass * htree) const;
	bool	Is_Excluded(HAnimClass * hanim) const;

	bool	Is_Excluded(const char * root_name) const;

	/*
	** The two naming rules the typed overloads above are made of, separated out so they
	** can be checked without building a prototype or an animation to hold the name.
	**
	**	Model name   "<file>"            kept when the file is named
	**	             "<file>.<subobj>"   kept when the file is named
	**	             anything with a '#' never kept -- it was munged at run time
	**	Anim name    "<skeleton>.<file>" kept when the file is named
	*/
	bool	Is_Excluded_Model_Name(const char * name) const;
	bool	Is_Excluded_Anim_Name(const char * name) const;

	int	Count(void) const { return Names.Count(); }

protected:

	const DynamicVectorClass<StringClass> &	Names;
	HashTemplateClass<StringClass,int>			NameHash;
};

#endif
