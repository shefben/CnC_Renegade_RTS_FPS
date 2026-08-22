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
#include "General.h"
#include "BanList.h"


BanList::Entry::Entry()
{
}

BanList::Entry::Entry(const StringClass& _clientName, const IpAddress _clientIpAddress, const StringClass& _clientSerialHash, const StringClass& _banReason) :
	clientName(_clientName),
	clientIpAddress(_clientIpAddress),
	clientSerialHash(_clientSerialHash),
	banReason(_banReason)
{
}

BanList::Entry& BanList::Entry::operator=(const BanList::Entry& that)
{
	clientName = that.clientName;
	clientIpAddress = that.clientIpAddress;
	clientSerialHash = that.clientSerialHash;
	banReason = that.banReason;
	return *this;
}


void BanList::addEntry(const BanList::Entry& entry)
{
	const uint entryId = entries.Count();
	entries.Add(entry);
	entryIdsByName.Insert(entry.getClientName(), entryId);
	entryIdsByIpAddress.Insert(entry.getClientIpAddress(), entryId);
	entryIdsBySerialHash.Insert(entry.getClientSerialHash(), entryId);
}

void BanList::clearEntries()
{
	entries.Clear();
	entryIdsByName.Remove_All();
	entryIdsByIpAddress.Remove_All();
	entryIdsBySerialHash.Remove_All();
}

const BanList::Entry* BanList::getEntryForPlayer(const StringClass& clientName, const IpAddress clientIpAddress, const StringClass& clientSerialHash)
{
	const Entry* entry = NULL;

	const int* entryId = entryIdsByName.Get(clientName);
	if (!entryId)
		entryId = entryIdsByIpAddress.Get(clientIpAddress);

    if (!entryId)
    {
        bool allow_serial_ban = false;
        for (const char* serial_buffer = clientSerialHash.Peek_Buffer(); *serial_buffer; ++serial_buffer)
        {
            if (*serial_buffer != '0') allow_serial_ban = true;
        }

        if (allow_serial_ban)
            entryId = entryIdsBySerialHash.Get(clientSerialHash);
    }

	if (entryId)
		entry = &entries[*entryId];

	return entry;
}
