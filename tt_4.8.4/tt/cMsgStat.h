#ifndef TT_INCLUDE__CMSGSTAT_H
#define TT_INCLUDE__CMSGSTAT_H



class cMsgStat
{

	int Num_Msg_Sent;
	int Num_Byte_Sent;
	int Num_Msg_Recd;
	int Num_Byte_Recd;
	char Name[32];

public:

	cMsgStat();
	~cMsgStat();

}; // 0030



#endif