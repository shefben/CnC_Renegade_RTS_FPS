#ifndef TT_INCLUDE__CAPPPACKETSTATS_H
#define TT_INCLUDE__CAPPPACKETSTATS_H
#include "engine_string.h"


enum PACKET_TIER_ENUM
{
	PACKET_TIER_CREATION,
	PACKET_TIER_RARE,
	PACKET_TIER_OCCASIONAL,
	PACKET_TIER_FREQUENT,
};



class cAppPacketStats
{

public:

	static void Increment_Packets_Sent(uint8);
	static void Increment_Bits_Sent(uint8, uint32);
	static void Reset();
	static void Increment_Bits_Sent_Tier(uint8, PACKET_TIER_ENUM, uint32);
	static void Dump_Diagnostics();
	static int Get_Packets_Sent(uint8);
	static int Get_Bits_Sent(uint8);
	static int Get_Bits_Sent_Tier(uint8, PACKET_TIER_ENUM);
	static char *Interpret_Type(uint8);
	static void Update_Object_Tally();
	static int Get_Object_Tally(uint8);
	static StringClass &Get_Heading();
	static StringClass &Get_Description(uint8);

};



#endif