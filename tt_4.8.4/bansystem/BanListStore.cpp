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
#include "BanListStore.h"


BanListStore::BanListStore(const StringClass& _fileName) :
	fileName(_fileName)
{
}

bool BanListStore::loadEntries(BanList& banList)
{
	bool result;
		
	banList.clearEntries();

	FILE* file = fopen(fileName, "rt");
	if (!file)
		result = false;
	else
	{
		result = true;

		char line[1024];
		while (fgets(line, _countof(line), file))
		{
			int lineLength = strnlen(line, _countof(line));
			if (lineLength >= _countof(line))
				result = false;
			else
			{
				// Strip off unwanted newline character if present.
				if (lineLength > 0 && line[lineLength-1] == '\n')
				{
					--lineLength;
					line[lineLength] = '\0';
				}

				const char* clientName = strtok(line, "\t");
				const char* clientIpAddressString = strtok(NULL, "\t");
				const char* clientSerialHash = strtok(NULL, "\t");
				const char* banReason = strtok(NULL, "\t");
					
				// Handle valid lines.
				if (banReason)
				{
					const Entry entry(clientName, IpAddress(clientIpAddressString), clientSerialHash, banReason);
					banList.addEntry(entry);
				}

				// Handle invalid lines.
				else if (clientName)
					result = false;

				// Handle empty lines.
				else
					;
			}
		}

		fclose(file);
	}
	return result;
}

bool BanListStore::storeEntry(const Entry& entry)
{
	bool result;

	FILE* file = fopen(fileName, "at");
	if (!file)
		result = false;
	else
	{
		fprintf(file, "%s\t%s\t%s\t%s\n", entry.getClientName().Peek_Buffer(), (const char*)entry.getClientIpAddress(), entry.getClientSerialHash().Peek_Buffer(), entry.getBanReason().Peek_Buffer());
		fclose(file);

		result = true;
	}
	return result;
}
