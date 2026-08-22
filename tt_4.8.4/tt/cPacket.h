#ifndef TT_INCLUDE__CPACKET_H
#define TT_INCLUDE__CPACKET_H



#include "bitstream.h"
class Vector3;
#include "cNetUtil.h"


class Quaternion;



class cPacket :
	public BitStreamClass
{

public:

	enum Type
	{
		TypeUnreliable    = 0,
		TypeReliable      = 1,
		TypeAck           = 2,
		TypeKeepalive     = 3,
		TypeConnect       = 4,
		TypeAccept        = 5,
		TypeRefusal       = 6,
		TypeFirewallProbe = 7,
		TypeResourceManager = 8,
		TypeCount
	};


	static bool EncoderInit;
	static sint32 RefCount;

	static const int DefSendTime = -1;
	static const uint16 PACKET_HEADER_SIZE = 7;


private:

#if defined(RR_RH7)
	byte      __fillup[4];
#endif                       //   * RH7
	sockaddr_in address;       // 564 568
	uint8       type;          // 580 584
	int         id;            // 584 588
	sint32      senderId;      // 588 592
	sint32      lastSendTime;  // 592 596
	sint32      firstSendTime; // 596 600
	sint32      resendCount;   // 600 604
	uint32      sendCount;     // 604 608


public:

	static void   Construct_App_Packet   (cPacket& target, cPacket& source);
	static void   Construct_Full_Packet  (cPacket& target, cPacket& source);
	static int Get_Default_Send_Time  ();
	static uint16 Get_Packet_Header_Size ();
	static sint32 Get_Ref_Count          ();
	static const char* getTypeName(int type);


	cPacket();
	cPacket(Type _type);
	cPacket(const cPacket& that) { *this = that; }

	virtual ~cPacket ();

	void               Add_Quaternion           (const Quaternion& quaternion);
	void               Add_Vector3              (const Vector3& vector);
	void               Clear_Resend_Count       ();
	uint32             Get_Bit_Length           () const;
	sint32             Get_First_Send_Time      () const;
	const Address&     Get_From_Address_Wrapper () const; // StealthEye: Based on connect.cpp source, the return type is incorrect.
	int                Get_Id                   () const;
	uint32             Get_Max_Size             () const;
	sint32             Get_Num_Sends            () const;
	void               Get_Quaternation         (Quaternion& target);
	int                Get_Resend_Count         () const;
	sint32             Get_Sender_Id            () const;
	int                Get_Send_Time            () const;
	uint8              Get_Type                 () const;
	void               Get_Vector3              (Vector3& target);
	void               Increment_Resend_Count   ();
	void               Init_Encoder             ();
	void               Set_Bit_Length           (uint32 bitLength);
	void               Set_Id                   (uint32 id);
	void               Set_Num_Sends            (uint32 sendCount);
	void               Set_Sender_Id            (sint32 senderId);
	void               Set_Send_Time            ();
	void               Set_Type                 (uint8 type);

	const sockaddr_in& getAddress () const;
	void               setAddress (const sockaddr_in& address);

	cPacket& operator = (const cPacket& packet);

}; // 604, RH7: 612



#endif
