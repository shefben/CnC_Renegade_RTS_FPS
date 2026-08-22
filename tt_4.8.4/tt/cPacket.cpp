#include "general.h"
#include "cPacket.h"
#include "Quaternion.h"
#include "Vector3.h"
#include "SysTimeClass.h"



bool cPacket::EncoderInit = true;
sint32 cPacket::RefCount;



cPacket::cPacket()
   : type(15), id(0), senderId(-1), lastSendTime(-1), firstSendTime(-1),
     resendCount(-1), sendCount(1)
{
	++RefCount;

	if (EncoderInit)
		this->Init_Encoder();
}



cPacket::cPacket(Type _type)
   : type((uint8)_type), id(0), senderId(-1), lastSendTime(-1), firstSendTime(-1),
     resendCount(-1), sendCount(1)
{
	++RefCount;

	if (EncoderInit)
		this->Init_Encoder();
}



cPacket::~cPacket()
{
   --RefCount;
}



void cPacket::Add_Quaternion
   (const Quaternion& quaternion)
{
   this->Add(quaternion.X);
   this->Add(quaternion.Y);
   this->Add(quaternion.Z);
   this->Add(quaternion.W);
}



void cPacket::Add_Vector3
   (const Vector3& vector)
{
   this->Add(vector.X);
   this->Add(vector.Y);
   this->Add(vector.Z);
}



void cPacket::Clear_Resend_Count()
{
   this->resendCount = 0;
}



void cPacket::Construct_App_Packet
   (cPacket& target, cPacket& source)
{
	uint8 temp1;
	uint32 temp2;
	char temp3;
	uint16 temp4;
	source.Get(temp1, BITPACK_PACKET_TYPE);
	source.Get(temp2, BITPACK_PACKET_ID);
	source.Get(temp3);
	source.Get(temp4);
	target.Set_Type(temp1);
	target.Set_Id(temp2);
	target.Set_Sender_Id(temp3);
	target.Set_Bit_Write_Position(temp4);

	::memcpy (&target.address, &source.address, sizeof(sockaddr_in));
	::memcpy (target.Get_Data(), &source.Get_Data()[7], target.Get_Compressed_Size_Bytes());
}



void cPacket::Construct_Full_Packet
   (cPacket& target, cPacket& source)
{
	target.Add(source.Get_Type(),BITPACK_PACKET_TYPE);
	target.Add(source.Get_Id(),BITPACK_PACKET_ID);

	target.Add((char)source.Get_Sender_Id());
	target.Add((uint16)source.Get_Bit_Write_Position());

	::memcpy (&target.Get_Data()[7], source.Get_Data(), source.Get_Compressed_Size_Bytes());
	target.Set_Bit_Write_Position (target.Get_Bit_Write_Position() + source.Get_Bit_Write_Position());
}



const Address& cPacket::getAddress() const
{
	return address;
}



uint32 cPacket::Get_Bit_Length() const
{
   return this->Get_Bit_Write_Position();
}



int cPacket::Get_Default_Send_Time()
{
	return DefSendTime;
}



sint32 cPacket::Get_First_Send_Time() const
{
   return this->firstSendTime;
}



const sockaddr_in& cPacket::Get_From_Address_Wrapper() const
{
   return this->address;
}



int cPacket::Get_Id() const
{
	return id;
}



uint32 cPacket::Get_Max_Size() const
{
   return MAX_BUFFER_SIZE;
}



sint32 cPacket::Get_Num_Sends() const
{
   return this->sendCount;
}



uint16 cPacket::Get_Packet_Header_Size()
{
   return PACKET_HEADER_SIZE;
}



void cPacket::Get_Quaternation
   (Quaternion& target)
{
   this->Get(target.X);
   this->Get(target.Y);
   this->Get(target.Z);
   this->Get(target.W);
}



sint32 cPacket::Get_Ref_Count()
{
   return RefCount;
}



int cPacket::Get_Resend_Count() const
{
   return this->resendCount;
}



sint32 cPacket::Get_Sender_Id() const
{
   return this->senderId;
}



int cPacket::Get_Send_Time() const
{
   return this->lastSendTime;
}



uint8 cPacket::Get_Type() const
{
   return this->type;
}



void cPacket::Get_Vector3
   (Vector3& target)
{
   this->Get(target.X);
   this->Get(target.Y);
   this->Get(target.Z);
}



void cPacket::Increment_Resend_Count()
{
   ++this->resendCount;
}



void cPacket::Init_Encoder()
{
   EncoderInit = false;
   cEncoderList::Set_Precision(BITPACK_PACKET_TYPE,4);
   cEncoderList::Set_Precision(BITPACK_PACKET_ID,28);
}



void cPacket::setAddress
   (const sockaddr_in& address)
{
   ::memcpy (&this->address, &address, sizeof(address));
}



void cPacket::Set_Bit_Length
   (uint32 bitLength)
{
   this->Set_Bit_Write_Position (bitLength);
}



void cPacket::Set_Id
   (uint32 id)
{
   this->id = id;
}



void cPacket::Set_Num_Sends
   (uint32 sendCount)
{
   this->sendCount = sendCount;
}



void cPacket::Set_Sender_Id
   (sint32 senderId)
{
   this->senderId = senderId;
}



void cPacket::Set_Send_Time()
{
   sint32 time = SysTimeClass::SystemTime.Get();

   if (this->lastSendTime == -1)
      this->firstSendTime = time;

   this->lastSendTime = time;
}



void cPacket::Set_Type
   (uint8 type)
{
   this->type = type;
}



cPacket& cPacket::operator =
   (const cPacket& packet)
{
   if (&packet == this)
      return *this;

   ::memcpy (&this->address, &packet.address, sizeof(sockaddr_in));

   this->type          = packet.type;
   this->id            = packet.id;
   this->senderId      = packet.senderId;
   this->lastSendTime  = packet.lastSendTime;
   this->firstSendTime = packet.firstSendTime;
   this->resendCount   = packet.resendCount;
   this->sendCount     = packet.sendCount;

   BitStreamClass::operator = (packet);

   return *this;
}



const char* cPacket::getTypeName(int type)
{
	switch (type)
	{
		case TypeUnreliable: return "PACKETTYPE_UNRELIABLE";
		case TypeReliable: return "PACKETTYPE_RELIABLE";
		case TypeAck: return "PACKETTYPE_ACK";
		case TypeKeepalive: return "PACKETTYPE_KEEPALIVE";
		case TypeConnect: return "PACKETTYPE_CONNECT_CS";
		case TypeAccept: return "PACKETTYPE_ACCEPT_SC";
		case TypeRefusal: return "PACKETTYPE_REFUSAL_SC";
		case TypeFirewallProbe: return "PACKETTYPE_FIREWALL_PROBE";
		case TypeResourceManager: return "PACKETTYPE_RESOURCE_MANAGER";
		default: TT_UNREACHABLE;
	}
}