#include "general.h"
#include "cAppPacketStats.h"



RENEGADE_FUNCTION
void cAppPacketStats::Reset()
AT2(0x00455560, 0x00455670);



RENEGADE_FUNCTION
void cAppPacketStats::Dump_Diagnostics()
AT2(0x004555A0, 0x004556B0);



RENEGADE_FUNCTION
void cAppPacketStats::Increment_Packets_Sent(uint8)
AT2(0x004555B0, 0x004556C0);



RENEGADE_FUNCTION
void cAppPacketStats::Increment_Bits_Sent(uint8, uint32)
AT2(0x004555E0, 0x004556F0);



RENEGADE_FUNCTION
void cAppPacketStats::Increment_Bits_Sent_Tier(uint8, PACKET_TIER_ENUM, uint32)
AT2(0x00455610, 0x00455720);



RENEGADE_FUNCTION
int cAppPacketStats::Get_Packets_Sent(uint8)
AT2(0x00455650, 0x00455760);



RENEGADE_FUNCTION
int cAppPacketStats::Get_Bits_Sent(uint8)
AT2(0x00455670, 0x00455780);



RENEGADE_FUNCTION
int cAppPacketStats::Get_Bits_Sent_Tier(uint8, PACKET_TIER_ENUM)
AT2(0x00455690, 0x004557A0);



RENEGADE_FUNCTION
char *cAppPacketStats::Interpret_Type(uint8)
AT2(0x004556B0, 0x004557C0);



RENEGADE_FUNCTION
void cAppPacketStats::Update_Object_Tally()
AT2(0x00455920, 0x00455A30);



RENEGADE_FUNCTION
int cAppPacketStats::Get_Object_Tally(uint8)
AT2(0x00455980, 0x00455A90);



RENEGADE_FUNCTION
StringClass &cAppPacketStats::Get_Heading()
AT2(0x004559A0, 0x00455AB0);



RENEGADE_FUNCTION
StringClass &cAppPacketStats::Get_Description(uint8)
AT2(0x004559F0, 0x00455B00);
