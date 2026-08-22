#ifndef TT_INCLUDE__PACKETMANAGERCLASS_H
#define TT_INCLUDE__PACKETMANAGERCLASS_H



#include "engine_vector.h"
#include "engine_threading.h"
#include "socket.h"



#ifdef TTINIT_EXPORTS
#define TTINIT_EXPORT __declspec(dllexport)
#else
#define TTINIT_EXPORT __declspec(dllimport)
#endif


#pragma warning(push)
#pragma warning(disable: 4251)



class TTINIT_EXPORT PacketManagerClass
{

public:

	class BitReader
	{

		const byte* data;
		int bitPosition;

	public:

		BitReader(const byte* _data, uint _bitPosition = 0)
		{
			TT_ASSERT(_bitPosition < 8);
			reset(_data, _bitPosition);
		}

		void reset(const byte* _data, uint _bitPosition = 0)
		{
			TT_ASSERT(_bitPosition < 8);
			data = _data;
			bitPosition = _bitPosition;
		}

		bool read()
		{
			bool result = (*data >> bitPosition) & 1;
			if (++bitPosition == 8)
			{
				bitPosition = 0;
				++data;
			}
			return result;
		}

	};

	class BitWriter
	{

		byte* data;
		int bitPosition;

	public:

		BitWriter(byte* _data, uint _bitPosition = 0)
		{
			TT_ASSERT(_bitPosition < 8);
			reset(_data, _bitPosition);
		}

		void reset(byte* _data, uint _bitPosition = 0)
		{
			TT_ASSERT(_bitPosition < 8);
			data = _data;
			bitPosition = _bitPosition;
		}

		void write(bool bit)
		{
			*data |= bit << bitPosition;
			if (++bitPosition == 8)
			{
				bitPosition = 0;
				*++data = 0;
			}
		}

	};

	struct BandwidthStatsStruct
	{
		Ip ip; // 0000
		Port port; // 0004
		uint rawBytesIn; // 0008
		uint rawBytesOut; // 000C
		uint compressedBytesIn; // 0010
		uint compressedBytesOut; // 0014
		uint rawBandwidthIn; // 0018
		uint rawBandwidthOut; // 001C
		uint compressedBandwidthIn; // 0020
		uint compressedBandwidthOut; // 0024


		BandwidthStatsStruct()
		{
			memset(this, 0, sizeof(BandwidthStatsStruct));
		}

		BandwidthStatsStruct(Ip _ip, Port _port)
		{
			memset(this, 0, sizeof(BandwidthStatsStruct));
			ip = _ip;
			port = _port;
		}

		bool operator ==(const BandwidthStatsStruct& that) const
		{
			return memcmp(this, &that, sizeof(BandwidthStatsStruct)) == 0;
		}

		bool operator !=(const BandwidthStatsStruct& that) const
		{
			return memcmp(this, &that, sizeof(BandwidthStatsStruct)) != 0;
		}

	}; // 0028


	class Buffer
	{

	public:

		byte*   data;           // 0000
		Ip      address;        // 0004
		Port    port;           // 0008
		uint    dataSize;       // 000C
		bool    isUsed;         // 0010
		uint    fullPacketSize; // 0014
		Socket* socket;         // 0018

		Buffer()
		{
			data = new byte[600];
			address = 0;
			port = 0;
			dataSize = 0;
			isUsed = false;
			fullPacketSize = 0;
			socket = NULL;
		}

		~Buffer()
		{
			delete[] data;
		}

	}; // 001C


	class CPacket
	{

	public:

		byte data[600];
		int length;

	}; // size: 604

	struct PacketHeader
	{
		uint deltaCount : 5;
		uint dataSize : 10;
		uint isPiggybacking : 1;
	}; // 0002

	struct DeltaHeader
	{
		uint useBigChunks : 1;
		uint useSmallChunks : 1;
	}; // 0001

   
	int                  maxBufferCount;          // 0
	Buffer*              buffers;                 // 4
	sint32               lastUsedBufferIndex;     // 8
	int                  usedBufferCount;         // 12
	byte                 fullPacket[540];         // 16
	byte                 deltaPacket[668];        // 556; This is possibly shorter!
	uint32               maxPacketCount;          // 1224
	CPacket*             packets;                 // 1228
	uint32               bufferedPacketIp;        // 1232
	uint16               bufferedPacketPort;      // 1236
	uint32               usedPacketCount;         // 1240
	uint32               lastUsedPacketIndex;     // 1244
	Socket*              bufferedPacketSocket;    // 1248
	uint32               uFlushTime;              // 1252
	uint32               uFlushInterval;          // 1256
	DynamicVectorClass<BandwidthStatsStruct> bandwidthStats; // 1260
	uint32               compressedBandwidthIn;   // 1284
	uint32               compressedBandwidthOut;  // 1288
	uint32               rawBandwidthIn;          // 1292
	uint32               rawBandwidthOut;         // 1296
	uint32               statsUpdateDelay;        // 1300
	uint32               lastStatsUpdateTime;     // 1304
	bool                 isPacketCacheInDirty;    // 1308
	bool                 isPacketCacheOutDirty;   // 1309
	bool                 canSendDelta;            // 1310
	bool                 canCollapse;             // 1311
	uint32               errorState;              // 1312
	CriticalSectionClass mutex;                   // 1316

	static int Build_Delta_Packet_Patch(const byte* basePacket, const byte* targetPacket, byte* deltaPacket, int basePacketSize, int targetPacketSize);
	static int Reconstruct_From_Delta(const byte* basePacket, byte* targetPacket, const byte* deltaPacket, int basePacketSize, int& deltaPacketSize);


	PacketManagerClass();
	~PacketManagerClass();

	void Set_Is_Server(bool);
	int Get_Next_Free_Buffer_Index();
	bool Take_Packet(const byte* data, uint dataSize, const Ip& ip, Port port, Socket& socket);
	void Flush(bool urgent = false);
	void Disable_Optimizations();
	void Break_Packet(const byte* data, uint dataSize, const Ip& ip, Port port);
	int Get_Packet(Socket& socket, byte* data, int maxDataSize, Ip& ip, Port& port);
	void Reset_Stats();
	int Get_Stats_Index(Ip ip, Port port, bool canCreate);
	void Register_Packet_In(const Ip& ip, Port port, uint compressedSize, uint rawSize);
	void Register_Packet_Out(const Ip& ip, Port port, uint compressedSize, uint rawSize);
	void Update_Stats(bool urgent);

	uint Get_Total_Raw_Bandwidth_In() const { return rawBandwidthIn; }
	uint Get_Total_Raw_Bandwidth_Out() const { return rawBandwidthOut; }
	uint Get_Total_Compressed_Bandwidth_In() const { return compressedBandwidthIn; }
	uint Get_Total_Compressed_Bandwidth_Out() const { return compressedBandwidthIn; }
	uint Get_Raw_Bandwidth_In(const Address& address);
	uint Get_Raw_Bandwidth_Out(const Address& address);
	uint Get_Raw_Bytes_Out(const Address& address);
	uint Get_Compressed_Bandwidth_In(const Address& address);
	uint Get_Compressed_Bandwidth_Out(const Address& address);
	void Set_Stats_Sampling_Frequency_Delay(uint32 _statsSampleDelay);
	int Get_Error_State();

	static void _placeHooks();

	TT_DEPRECATED("Use Socket::clearError instead.") int Clear_Socket_Error(Socket& socket);

}; // 052C 0534



__declspec(dllexport) PacketManagerClass& PacketManager();



#pragma warning(disable: 4251)
#pragma warning(pop)



#endif
