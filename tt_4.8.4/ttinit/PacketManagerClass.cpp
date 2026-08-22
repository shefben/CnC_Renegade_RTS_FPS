#include "General.h"
#include "hooksupport.h"
#include "PacketManagerClass.h"
#include "engine_io.h"
#include "SysTimeClass.h"
#include "cNetUtil.h"

#pragma warning(disable:6255) // _alloca indicates failure by raising a stack overflow exception. Consider using _malloca instead
#pragma warning(disable:6263) // Using _alloca in a loop



RENEGADE_FUNCTION
int PacketManagerClass::Build_Delta_Packet_Patch(const byte* basePacket, const byte* targetPacket, byte* deltaPacket, int basePacketSize, int targetPacketSize)
AT2(0x0061BB30, 0x0061B3D0);


/*
// Doesn't seem to work correctly
int PacketManagerClass::Build_Delta_Packet_Patch(const byte* basePacket, const byte* targetPacket, byte* deltaPacket, int basePacketSize, int targetPacketSize)
{
	TT_ASSERT(basePacket);
	TT_ASSERT(targetPacket);
	TT_ASSERT(deltaPacket);
	TT_ASSERT(basePacketSize == targetPacketSize);

	DeltaHeader& deltaHeader = *(DeltaHeader*)deltaPacket;
	*deltaPacket = 0; // Clear the header

	BitWriter bitWriter(deltaPacket + 1);
	BitReader bitReader(deltaPacket + 1);

	int bigChunkCount = divideRoundUp(basePacketSize, 8);
	int smallChunkCount = 0;

	{
		// Check all full big chunks
		int i = 0;
		for (; i < basePacketSize - 7; i += 8)
		{
			bool areChunksEqual = memcmp(basePacket + i, targetPacket + i, 8) == 0;
			bitWriter.write(areChunksEqual);
			
			if (!areChunksEqual)
				smallChunkCount += 8;
		}

		// If needed, check the last partial big chunk
		if (i != basePacketSize)
		{
			bool areChunksEqual = memcmp(basePacket + i, targetPacket + i, basePacketSize - i) == 0;
			bitWriter.write(areChunksEqual);

			if (!areChunksEqual)
				smallChunkCount += basePacketSize - i;
		}
	}
	
	// If all big chunks are different, send only the small chunks
	if (smallChunkCount == basePacketSize)
	{
		bitWriter.reset(deltaPacket + 1);
		bigChunkCount = 0;
	}

	// Some of the chunks are different and some are not, we have to send the big chunks.
	else
		deltaHeader.useBigChunks = true;

	// If the packets are equal there is no need to send any small chunks
	if (smallChunkCount == 0)
		return 1 + divideRoundUp(bigChunkCount, 8);

	// The packets are not equal, we have to send the 
	else
	{
		deltaHeader.useSmallChunks = true;

		// Calculate the position at which the data bytes will be stored. That is right after the packet bits.
		byte* deltaPacketDataIter = deltaPacket + 1 + divideRoundUp(bigChunkCount + smallChunkCount, 8);

		{
			for (int i = 0; i < basePacketSize;)
			{
				// Only send the small chunks if we are not sending the big chunk bits or if this big chunk is not equal.
				if (!deltaHeader.useBigChunks || !bitReader.read())
				{
					// For every byte, write the equality bit and if unequal, write the data.
					do
					{
						bool areChunksEqual = basePacket[i] == targetPacket[i];
						bitWriter.write(areChunksEqual);
						
						if (!areChunksEqual)
							*deltaPacketDataIter++ = targetPacket[i];
					}
					while (++i % 8u == 0 && i < basePacketSize);
				}
				else
					i += 8;
			}
		}
		
		// deltaPacketDataIter points to the end of the packet here.
		return deltaPacketDataIter - deltaPacket;
	}
}
*/



RENEGADE_FUNCTION
int PacketManagerClass::Reconstruct_From_Delta(const byte* basePacket, byte* targetPacket, const byte* deltaPacket, int basePacketSize, int& deltaPacketSize)
AT2(0x0061BD90, 0x0061B630);




PacketManagerClass::PacketManagerClass()
{
	bandwidthStats.Set_Growth_Step(128);
	lastUsedBufferIndex = 0;
	usedBufferCount = 0;
	usedPacketCount = 0;
	lastUsedPacketIndex = 0;
	uFlushTime = 0;
	uFlushInterval = 100;
	statsUpdateDelay = 10000;
	canSendDelta = true;
	canCollapse = true;
	lastStatsUpdateTime = TIMEGETTIME();
	errorState = 0;
	isPacketCacheInDirty = true;
	isPacketCacheOutDirty = true;

	maxBufferCount = 256;
	buffers = new Buffer[maxBufferCount];

	maxPacketCount = 128;
	packets = new CPacket[maxPacketCount];

}



PacketManagerClass::~PacketManagerClass()
{
	delete[] buffers;
	delete[] packets;
	
}



void PacketManagerClass::Set_Is_Server(bool isServer)
{
	CriticalSectionClass::LockClass lock(mutex);
	
	if (isServer)
	{
		if (maxBufferCount == 1024)
			return;
		
		maxBufferCount = 1024;
		maxPacketCount = 2048;
	}
	else
	{
		if (maxBufferCount == 256)
			return;
		
		maxBufferCount = 256;
		maxPacketCount = 128;
	}
	
	usedBufferCount = 0;
	lastUsedBufferIndex = 0;

	usedPacketCount = 0;
	lastUsedPacketIndex = 0;
	
	delete[] buffers;
	buffers = new Buffer[maxBufferCount];
	
	delete[] packets;
	packets = new CPacket[maxPacketCount];
	
	Reset_Stats();
}



int PacketManagerClass::Get_Next_Free_Buffer_Index()
{
	if (maxBufferCount <= 0 || usedBufferCount == maxBufferCount)
		return -1;

	do
	{
		if (++lastUsedBufferIndex == maxBufferCount)
			lastUsedBufferIndex = 0;
	}
	while (buffers[lastUsedBufferIndex].dataSize != 0);
	
	return lastUsedBufferIndex;
}



bool PacketManagerClass::Take_Packet(const byte* data, uint dataSize, const Ip& ip, Port port, Socket& socket)
{
	CriticalSectionClass::LockClass lock(mutex);

	if (usedBufferCount >= maxBufferCount || dataSize > 535)
		return false;

	int bufferIndex = Get_Next_Free_Buffer_Index();
	if (bufferIndex < 0)
		return false;

	Buffer& buffer = buffers[bufferIndex];
	
	memcpy(buffer.data, data, dataSize);
	buffer.dataSize = dataSize;
	buffer.address  = ip;
	buffer.port     = port;
	buffer.socket   = &socket;

	++usedBufferCount;

	if (usedBufferCount + 4 >= maxBufferCount)
		Flush(true);

	Register_Packet_Out(ip, port, 0, dataSize + 28);
	return true;
}



void PacketManagerClass::Flush(bool urgent)
{
	CriticalSectionClass::LockClass lock(mutex);

	uint32 currentTime = TIMEGETTIME();
	if (urgent || !uFlushInterval || currentTime - uFlushTime >= uFlushInterval)
	{
		uFlushTime = currentTime;
		
		for (int i = 0; i < maxBufferCount; ++i)
			buffers[i].isUsed = false;
		
		int baseBufferId = lastUsedBufferIndex;
		
		for (; usedBufferCount != 0; usedBufferCount--)
		{
			// Find the next used buffer
			do
			{
				if (++baseBufferId >= maxBufferCount)
					baseBufferId = 0;
			} while (buffers[baseBufferId].dataSize == 0);

			((PacketHeader&)fullPacket).isPiggybacking = false;
			((PacketHeader&)fullPacket).dataSize = buffers[baseBufferId].dataSize;
			((PacketHeader&)fullPacket).deltaCount = 1;
			
			byte* fullPacketIter = fullPacket + 2;
			uint fullPacketSize = 2;
			
			memcpy(fullPacketIter, buffers[baseBufferId].data, buffers[baseBufferId].dataSize);
			fullPacketIter += buffers[baseBufferId].dataSize;
			fullPacketSize += buffers[baseBufferId].dataSize;
			
			int subBufferId = baseBufferId;
			
			for (uint remainingSubBufferCount = usedBufferCount - 1; remainingSubBufferCount; remainingSubBufferCount--)
			{
				// Find the next used buffer
				do
				{
					if (++subBufferId >= maxBufferCount)
						subBufferId = 0;
				} while (buffers[subBufferId].dataSize == 0);
				
				// Try to delta the packet if it is of the same size and sent to the same address
				if (buffers[subBufferId].dataSize == buffers[baseBufferId].dataSize &&
					buffers[subBufferId].socket == buffers[baseBufferId].socket &&
					buffers[subBufferId].port == buffers[baseBufferId].port &&
					buffers[subBufferId].address == buffers[baseBufferId].address)
				{
					uint deltaPacketSize = Build_Delta_Packet_Patch(buffers[baseBufferId].data, buffers[subBufferId].data, deltaPacket, buffers[baseBufferId].dataSize, buffers[subBufferId].dataSize);

					// If the delta was bigger than the original, send without delta.								
					if (deltaPacketSize > buffers[baseBufferId].dataSize || !canSendDelta)
					{
						// If the packet will become to big, try to find another packet instead.
						if (fullPacketSize + buffers[baseBufferId].dataSize + 1 > 540)
							continue;

						// Mark this packet as not delta'ed
						*fullPacketIter &= 0xFC;
						fullPacketIter++;
						fullPacketSize++;
						
						// Send the packet
						memcpy(fullPacketIter, buffers[subBufferId].data, buffers[baseBufferId].dataSize);
						fullPacketIter += buffers[baseBufferId].dataSize;
						fullPacketSize += buffers[baseBufferId].dataSize;
					}
					else
					{
						// If the packet will become to big, try to find another packet instead.
						if (fullPacketSize + deltaPacketSize + 1 > 540)
							continue;

						// Send the delta'ed packet.
						memcpy(fullPacketIter, &deltaPacket, deltaPacketSize);
						fullPacketIter += deltaPacketSize;
						fullPacketSize += deltaPacketSize;
					}
					
					buffers[subBufferId].dataSize = 0;
					--usedBufferCount;
					((PacketHeader&)fullPacket).deltaCount++;
					if (((PacketHeader&)fullPacket).deltaCount == 15)
						break;
				}
			}
			
			memcpy(buffers[baseBufferId].data, fullPacket, fullPacketSize);
			buffers[baseBufferId].dataSize = 0;
			buffers[baseBufferId].isUsed = true;
			buffers[baseBufferId].fullPacketSize = fullPacketSize;
		}

		if (canCollapse)
		{
			for (int i = 0; i < maxBufferCount; i++)
			{
				if (buffers[i].isUsed)
				{
					byte* previousBufferDataIter = buffers[i].data;
					
					for (int j = i + 1; j < maxBufferCount; j++)
					{
						if (buffers[j].isUsed)
						{
							if (buffers[j].socket == buffers[i].socket &&
								buffers[j].port == buffers[i].port &&
								buffers[j].address == buffers[i].address &&
								buffers[j].fullPacketSize + buffers[i].fullPacketSize < 540)
							{
								((PacketHeader*)previousBufferDataIter)->isPiggybacking = true;
								previousBufferDataIter = buffers[i].data + buffers[i].fullPacketSize;
								buffers[i].fullPacketSize += buffers[j].fullPacketSize;
								memcpy(previousBufferDataIter, buffers[j].data, buffers[j].fullPacketSize);
								buffers[j].isUsed = false;
							}
						}
					}
				}
			}
		}

		for (int i = 0; i < maxBufferCount; i++)
		{
			if (buffers[i].isUsed)
			{
				byte* vBuffer = (byte*)alloca(buffers[i].fullPacketSize + 4);
				
				*(uint32*)vBuffer = htonl(CRC_Memory(buffers[i].data, buffers[i].fullPacketSize, 0));
				memcpy(vBuffer + 4, buffers[i].data, buffers[i].fullPacketSize);
				Register_Packet_Out(buffers[i].address, buffers[i].port, buffers[i].fullPacketSize + 32, 0);
				
				Address address;
				address.sin_family = AF_INET;
				address.sin_port = buffers[i].port;
				address.sin_addr.s_addr = buffers[i].address;

				TT_ASSERT(buffers[i].socket)
				if (buffers[i].socket->sendTo(vBuffer, buffers[i].fullPacketSize + 4, address))
				{
					if (WSAGetLastError() == WSAEWOULDBLOCK)
					{
						Sleep(0);
						errorState = 1;
					}
					else
						buffers[i].socket->clearError();;
				}
			}
		}
		
		Update_Stats(0);
	}
}



void PacketManagerClass::Disable_Optimizations()
{
	uFlushInterval = 0;
	canSendDelta = false;
	canCollapse = false;
}



void PacketManagerClass::Break_Packet(const byte* data, uint dataSize, const Ip& ip, Port port)
{
	bool isPiggybacking;

	do
	{
		PacketHeader& packetHeader = *(PacketHeader*)data;

		isPiggybacking = packetHeader.isPiggybacking;
		int packetSize = packetHeader.dataSize;
		int subPacketCount = packetHeader.deltaCount;

		if (subPacketCount == 0 || packetSize > 540)
			return;

		bufferedPacketIp = ip;
		bufferedPacketPort = port;

		data += 2;
		dataSize -= 2;

		CPacket& firstPacket = packets[usedPacketCount];

		memcpy(firstPacket.data, data, packetSize);
		firstPacket.length = packetSize;

		Register_Packet_In(ip, port, 0, packetSize + 28);

		if (++usedPacketCount >= maxPacketCount)
			return; // We lose the rest of the packet by design.

		data += packetSize;
		dataSize -= packetSize;

		for (int i = 1; i < subPacketCount; ++i)
		{
			if (*data & 0x03)
			{
				int deltaPacketSize = 0;

				if (packetSize != Reconstruct_From_Delta(firstPacket.data, packets[usedPacketCount].data, data, packetSize, deltaPacketSize))
					return; // Error, don't try to decode the other parts of the packet

				if (deltaPacketSize <= 0)
					break;

				data += deltaPacketSize;
				dataSize -= deltaPacketSize;
			}
			else
			{
				memcpy(packets[usedPacketCount].data, data + 1, packetSize);

				data += packetSize + 1;
				dataSize -= packetSize + 1;
			}


			packets[usedPacketCount].length = packetSize;
			Register_Packet_In(ip, port, 0, packetSize + 28);

			if (++usedPacketCount >= maxPacketCount)
				return; // We lose the rest of the packet by design.
		}
	}
	while (isPiggybacking);
	TT_ASSERT(dataSize == 0);
}



int PacketManagerClass::Get_Packet(Socket& socket, byte* data, int maxDataSize, Ip& ip, Port& port)
{
	if (maxDataSize > 600)
	{
		return 0;
	}
	CriticalSectionClass::LockClass lock(mutex);

	while (usedPacketCount == 0)
	{
		sockaddr_in address;
		int dataSize = socket.receiveFrom(data, maxDataSize, address);

		if (dataSize <= 0)
		{
			int error = WSAGetLastError();

			if (error == WSAEWOULDBLOCK)
			{
				// No new packets available.
				socket.clearError();
				return 0;
			}
			else
			{
				// A previously sent packet was actively refused or some other error occurred.
				ip = address.sin_addr.s_addr;
				port = address.sin_port;
				return -1;
			}
		}
		else
		{
			if (dataSize > 4 &&
				dataSize <= 548 &&
				*(uint32*)data != 0x003A5252 && // RR packets start with 0x003A5252
				ntohl(*(uint32*)data) == CRC_Memory(data + 4, dataSize - 4, 0))
			{
				Register_Packet_In(address.sin_addr.s_addr, address.sin_port, dataSize + 28, 0);
				dataSize -= 4;
				memmove(data, data + 4, dataSize);
				bufferedPacketSocket = &socket;

				Break_Packet(data, dataSize, address.sin_addr.s_addr, address.sin_port);

				lastUsedPacketIndex = 0;
			}
		}
	}
	
	while (usedPacketCount != 0)
	{
		TT_ASSERT(usedPacketCount == 0 || &socket == bufferedPacketSocket); // If not, the original code skipped it. However this is inefficient. Assume that all packets for one socket are received at once so that this assert can never trigger.

		CPacket& packet = packets[lastUsedPacketIndex];

		if (++lastUsedPacketIndex >= usedPacketCount)
		{
			usedPacketCount = 0;
			lastUsedPacketIndex = 0;
		}

		TT_ASSERT(packet.length <= maxDataSize);
		if (packet.length < maxDataSize)
		{
			memcpy(data, packet.data, packet.length);

			ip = bufferedPacketIp;
			port = bufferedPacketPort;

			return packet.length;
		}
	}

	return 0;
}



void PacketManagerClass::Reset_Stats()
{
	CriticalSectionClass::LockClass lock(mutex);
	bandwidthStats.Delete_All();
	
	lastStatsUpdateTime = TIMEGETTIME();
	isPacketCacheInDirty = true;
	isPacketCacheOutDirty = true;
}



int PacketManagerClass::Get_Stats_Index(Ip ip, Port port, bool canCreate)
{
	int i;
	for (i = 0; i < bandwidthStats.Count(); i++)
		if (bandwidthStats[i].ip == ip && bandwidthStats[i].port == port)
			return i;
	
	if (canCreate)
	{
		bandwidthStats.Add(BandwidthStatsStruct(ip, port));
		return i;
	}
	else
		return -1;
}



void PacketManagerClass::Register_Packet_In(const Ip& ip, Port port, uint compressedSize, uint rawSize)
{
	static int cachedHostStatsId;
	static Ip cachedIp;
	static Port cachedPort;
	
	if (isPacketCacheInDirty)
	{
		cachedIp = 0;
		cachedPort = 0;
		cachedHostStatsId = -1;
		isPacketCacheInDirty = false;
	}
	
	if (cachedHostStatsId == -1 ||
		cachedIp != ip ||
		cachedPort != port)
	{
		cachedIp = ip;
		cachedPort = port;
		cachedHostStatsId = PacketManagerClass::Get_Stats_Index(ip, port, true);
	}
	
	bandwidthStats[cachedHostStatsId].rawBytesIn += rawSize;
	bandwidthStats[cachedHostStatsId].compressedBytesIn += compressedSize;
}



void PacketManagerClass::Register_Packet_Out(const Ip& ip, Port port, uint compressedSize, uint rawSize)
{
	static int cachedHostStatsId;
	static Ip cachedIp;
	static Port cachedPort;
	
	if (isPacketCacheOutDirty)
	{
		cachedIp = 0;
		cachedPort = 0;
		cachedHostStatsId = -1;
		isPacketCacheOutDirty = false;
	}
	
	if (cachedHostStatsId == -1 ||
		cachedIp != ip ||
		cachedPort != port)
	{
		cachedIp = ip;
		cachedPort = port;
		cachedHostStatsId = PacketManagerClass::Get_Stats_Index(ip, port, true);
	}
	
	bandwidthStats[cachedHostStatsId].rawBytesOut += rawSize;
	bandwidthStats[cachedHostStatsId].compressedBytesOut += compressedSize;
}



void PacketManagerClass::Update_Stats(bool urgent)
{
	CriticalSectionClass::LockClass lock(mutex);
	
	int currentTime = TIMEGETTIME();
	uint sampleTime = currentTime - lastStatsUpdateTime;
	if (urgent || sampleTime > statsUpdateDelay)
	{
		lastStatsUpdateTime = currentTime;
		
		compressedBandwidthIn = 0;
		compressedBandwidthOut = 0;
		rawBandwidthIn = 0;
		rawBandwidthOut = 0;
		
		for (int i = 0; i < bandwidthStats.Count(); i++)
		{
			BandwidthStatsStruct& hostStats = bandwidthStats[i];

			if (hostStats.compressedBytesOut)
			{
				hostStats.compressedBandwidthOut = hostStats.compressedBytesOut * 8 * 1000 / sampleTime;
				hostStats.compressedBytesOut = 0;
				compressedBandwidthOut += hostStats.compressedBandwidthOut;
			}
			
			if (hostStats.compressedBytesIn)
			{
				hostStats.compressedBandwidthIn = hostStats.compressedBytesIn * 8 * 1000 / sampleTime;
				hostStats.compressedBytesIn = 0;
				compressedBandwidthIn += hostStats.compressedBandwidthIn;
			}

			if (hostStats.rawBytesOut)
			{
				hostStats.rawBandwidthOut = hostStats.rawBytesOut * 8 * 1000 / sampleTime;
				hostStats.rawBytesOut = 0;
				rawBandwidthOut += hostStats.rawBandwidthOut;
			}

			if (hostStats.rawBytesIn)
			{
				hostStats.rawBandwidthIn = hostStats.rawBytesIn * 8 * 1000 / sampleTime;
				hostStats.rawBytesIn = 0;
				rawBandwidthIn += hostStats.rawBandwidthIn;
			}
		}
	}
}



uint PacketManagerClass::Get_Raw_Bandwidth_In(const Address& address)
{
	CriticalSectionClass::LockClass lock(mutex);

	uint statsIndex = PacketManagerClass::Get_Stats_Index(address.sin_addr.s_addr, address.sin_port, false);
	if (statsIndex != -1)
		return bandwidthStats[statsIndex].rawBandwidthIn;
	else
		return 0;
}



uint PacketManagerClass::Get_Raw_Bandwidth_Out(const Address& address)
{
	CriticalSectionClass::LockClass lock(mutex);

	uint statsIndex = PacketManagerClass::Get_Stats_Index(address.sin_addr.s_addr, address.sin_port, false);
	if (statsIndex != -1)
		return bandwidthStats[statsIndex].rawBandwidthOut;
	else
		return 0;
}



uint PacketManagerClass::Get_Raw_Bytes_Out(const Address& address)
{
	CriticalSectionClass::LockClass lock(mutex);

	uint statsIndex = PacketManagerClass::Get_Stats_Index(address.sin_addr.s_addr, address.sin_port, false);
	if (statsIndex != -1)
		return bandwidthStats[statsIndex].rawBytesOut;
	else
		return 0;
}



uint PacketManagerClass::Get_Compressed_Bandwidth_In(const Address& address)
{
	CriticalSectionClass::LockClass lock(mutex);

	uint statsIndex = PacketManagerClass::Get_Stats_Index(address.sin_addr.s_addr, address.sin_port, false);
	if (statsIndex != -1)
		return bandwidthStats[statsIndex].compressedBandwidthIn;
	else
		return 0;
}



uint PacketManagerClass::Get_Compressed_Bandwidth_Out(const Address& address)
{
	CriticalSectionClass::LockClass lock(mutex);

	uint statsIndex = PacketManagerClass::Get_Stats_Index(address.sin_addr.s_addr, address.sin_port, false);
	if (statsIndex != -1)
		return bandwidthStats[statsIndex].compressedBandwidthOut;
	else
		return 0;
}



void PacketManagerClass::Set_Stats_Sampling_Frequency_Delay(uint32 _statsUpdateDelay)
{
	statsUpdateDelay = _statsUpdateDelay;
	Reset_Stats();
}



int PacketManagerClass::Get_Error_State()
{
	int result = errorState;
	errorState = 0;
	return result;
}



void PacketManagerClass::_placeHooks()
{
	void* address = &PacketManager();
	uint32 null = NULL;

	switch (Exe)
	{

	case 0:

		// Change "mov ecx, PacketManager"
		WriteMemory((void*)(0x0040345A+1), &address, sizeof(address));
		WriteMemory((void*)(0x00404927+1), &address, sizeof(address));
		WriteMemory((void*)(0x00404F7C+1), &address, sizeof(address));
		WriteMemory((void*)(0x00404FF0+1), &address, sizeof(address));
		WriteMemory((void*)(0x004050B6+1), &address, sizeof(address));
		WriteMemory((void*)(0x00426E8C+1), &address, sizeof(address));
		WriteMemory((void*)(0x00426FEC+1), &address, sizeof(address));
		WriteMemory((void*)(0x00456CEF+1), &address, sizeof(address));
		WriteMemory((void*)(0x00457FC9+1), &address, sizeof(address));
		WriteMemory((void*)(0x004586CD+1), &address, sizeof(address));
		WriteMemory((void*)(0x00458EB3+1), &address, sizeof(address));
		WriteMemory((void*)(0x004602CE+1), &address, sizeof(address));
		WriteMemory((void*)(0x004602ED+1), &address, sizeof(address));
		WriteMemory((void*)(0x0046CF28+1), &address, sizeof(address));
		WriteMemory((void*)(0x0046CF34+1), &address, sizeof(address));
		WriteMemory((void*)(0x0046D0CD+1), &address, sizeof(address));
		WriteMemory((void*)(0x006194E0+1), &address, sizeof(address));
		WriteMemory((void*)(0x006194F0+1), &address, sizeof(address));
		WriteMemory((void*)(0x0061969D+1), &address, sizeof(address));
		WriteMemory((void*)(0x006196B1+1), &address, sizeof(address));
		WriteMemory((void*)(0x0061988E+1), &address, sizeof(address));
		WriteMemory((void*)(0x006198A2+1), &address, sizeof(address));
		WriteMemory((void*)(0x00619929+1), &address, sizeof(address));
		WriteMemory((void*)(0x0061ACCD+1), &address, sizeof(address));
		WriteMemory((void*)(0x0061B5C0+1), &address, sizeof(address));
		WriteMemory((void*)(0x0061DECB+1), &address, sizeof(address));
		WriteMemory((void*)(0x00620DFC+1), &address, sizeof(address));
		WriteMemory((void*)(0x006211EA+1), &address, sizeof(address));

		// Disable static constructor
		WriteMemory((void*)(0x007F4704), &null, sizeof(null));

		break;

	case 1:

		// Change "mov ecx, PacketManager"
		WriteMemory((void*)(0x0040345A+1), &address, sizeof(address));
		WriteMemory((void*)(0x00404927+1), &address, sizeof(address));
		WriteMemory((void*)(0x00404F7C+1), &address, sizeof(address));
		WriteMemory((void*)(0x00404FF0+1), &address, sizeof(address));
		WriteMemory((void*)(0x004050B6+1), &address, sizeof(address));
		WriteMemory((void*)(0x00426E8C+1), &address, sizeof(address));
		WriteMemory((void*)(0x00426FEC+1), &address, sizeof(address));
		WriteMemory((void*)(0x00457A79+1), &address, sizeof(address));
		WriteMemory((void*)(0x0045815D+1), &address, sizeof(address));
		WriteMemory((void*)(0x00458943+1), &address, sizeof(address));
		WriteMemory((void*)(0x0045FDC8+1), &address, sizeof(address));
		WriteMemory((void*)(0x0045FDE7+1), &address, sizeof(address));
		WriteMemory((void*)(0x0046C858+1), &address, sizeof(address));
		WriteMemory((void*)(0x0046C864+1), &address, sizeof(address));
		WriteMemory((void*)(0x0046C9FD+1), &address, sizeof(address));
		WriteMemory((void*)(0x00618D80+1), &address, sizeof(address));
		WriteMemory((void*)(0x00618D90+1), &address, sizeof(address));
		WriteMemory((void*)(0x00618F3D+1), &address, sizeof(address));
		WriteMemory((void*)(0x00618F51+1), &address, sizeof(address));
		WriteMemory((void*)(0x0061912E+1), &address, sizeof(address));
		WriteMemory((void*)(0x00619142+1), &address, sizeof(address));
		WriteMemory((void*)(0x006191C9+1), &address, sizeof(address));
		WriteMemory((void*)(0x0061A56D+1), &address, sizeof(address));
		WriteMemory((void*)(0x0061AE60+1), &address, sizeof(address));
		WriteMemory((void*)(0x0061D76B+1), &address, sizeof(address));
		WriteMemory((void*)(0x0062069C+1), &address, sizeof(address));
		WriteMemory((void*)(0x00620A8A+1), &address, sizeof(address));

		// Disable static constructor
		WriteMemory((void*)(0x007F3704), &null, sizeof(null)); // Disable static constructor
		break;
	default:
		TT_UNREACHABLE;
	}
}



PacketManagerClass& PacketManager()
{
	static PacketManagerClass packetManager;
	return packetManager;
}
