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

class StringClass;


struct IpAddress
{
	union
	{
		uint32 dword;
		in_addr address;
		uint8 bytes[4];
	};
	
	IpAddress() {}
	IpAddress(uint32 _address) : dword(_address) {}
	IpAddress(in_addr _address) : address(_address) {}
	IpAddress(const char* string) { dword = inet_addr(string); }

	operator const char* const () const { return inet_ntoa(address); }
	operator const uint32() const { return dword; }
	operator const in_addr() const { return address; }

};
