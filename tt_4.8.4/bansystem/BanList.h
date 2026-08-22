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

#include "engine_string.h"
#include "engine_vector.h"
#include "IpAddress.h"
#include "HashTemplateClass.h"


class BanList
{

public:
	
	class Entry :
		public NoEqualsClass<Entry>
	{
		StringClass clientName;
		IpAddress clientIpAddress;
		StringClass clientSerialHash;
		StringClass banReason;

		friend DynamicVectorClass<Entry>;

	public:
		
		Entry();
		Entry(const StringClass& _clientName, const IpAddress _clientIpAddress, const StringClass& _clientSerialHash, const StringClass& _banReason);
		Entry(const Entry& that) { *this = that; }

		Entry& operator=(const Entry& that);
		
		const StringClass& getClientName() const { return clientName; }
		const IpAddress getClientIpAddress() const { return clientIpAddress; }
		const StringClass& getClientSerialHash() const { return clientSerialHash; }
		const StringClass& getBanReason() const { return banReason; }
	};

private:
	
	DynamicVectorClass<Entry> entries; // Ban records should never be removed unless the hashmaps below are updated too
	HashTemplateClass<StringClass, int> entryIdsByName;
	HashTemplateClass<uint32, int> entryIdsByIpAddress;
	HashTemplateClass<StringClass, int> entryIdsBySerialHash;

public:
	
	void addEntry(const Entry& entry);
	void clearEntries();
	const Entry* getEntryForPlayer(const StringClass& clientName, const IpAddress clientIpAddress, const StringClass& clientSerialHash);

};
