#ifndef TT_INCLUDE__CMSGSTATLIST_H
#define TT_INCLUDE__CMSGSTATLIST_H


class cMsgStat;



class cMsgStatList
{

	cMsgStat* stats;
	int statCount;

public:

	cMsgStatList();
	~cMsgStatList();

	void Init(int packetTypeCount);
	void Increment_Num_Msg_Sent(int, int = 1);
	void Increment_Num_Byte_Sent(int, int);
	void Increment_Num_Msg_Recd(int, int = 1);
	void Increment_Num_Byte_Recd(int, int);
	int Get_Num_Msg_Sent(int);
	int Get_Num_Byte_Sent(int);
	int Get_Num_Msg_Recd(int);
	int Get_Num_Byte_Recd(int);
	void Compute_Avg_Num_Byte_Sent(int);
	void Compute_Avg_Num_Byte_Recd(int);
	cMsgStat* Get_Stat(int);
	void Set_Name(int packetType, const char* name);
	const char* Get_Name(int);

};



#endif