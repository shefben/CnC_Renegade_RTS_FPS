#ifndef TT_INCLUDE__CMSGSTATLISTGROUP_H
#define TT_INCLUDE__CMSGSTATLISTGROUP_H



class cMsgStatList;



class cMsgStatListGroup
{

private:

	cMsgStatList* statLists;
	int statListCount;

public:

	cMsgStatListGroup();
	~cMsgStatListGroup();
	void Init(int, int);
	void Increment_Num_Msg_Sent(int, int, int);
	void Increment_Num_Byte_Sent(int, int, int);
	void Increment_Num_Msg_Recd(int, int, int);
	void Increment_Num_Byte_Recd(int, int, int);
	int Get_Num_Msg_Sent(int, int);
	int Get_Num_Byte_Sent(int, int);
	int Get_Num_Msg_Recd(int, int);
	int Get_Num_Byte_Recd(int, int);
	void Compute_Avg_Num_Byte_Sent(int, int);
	void Compute_Avg_Num_Byte_Recd(int, int);
	cMsgStatList* Get_Stat_List(int);
	void Set_Name(int, const char*);

};



#endif