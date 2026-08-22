/*	Renegade Scripts.dll
	Copyright 2013 Tiberian Technologies

	This file is part of the Renegade scripts.dll
	The Renegade scripts.dll is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2, or (at your option) any later
	version. See the file COPYING for more details.
	In addition, an exemption is given to allow Run Time Dynamic Linking of this code with any closed source module that does not contain code covered by this licence.
	Only the source code to the module(s) containing the licenced code has to be released.
*/
#pragma once

#include "BanList.h"


class BanListStore
{

public:

	typedef BanList::Entry Entry;

	const StringClass fileName;
	
public:

	BanListStore(const StringClass& _fileName);
	TT_DEPRECATED("This class can not be copied") BanListStore(const BanListStore& that);
	TT_DEPRECATED("This class can not be copied") BanListStore& operator=(const BanListStore& that);

	bool loadEntries(BanList& banList);
	bool storeEntry(const Entry& entry);

};