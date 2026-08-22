#ifndef TT_INCLUDE__BANDWIDTHBALANCERCLASS_H
#define TT_INCLUDE__BANDWIDTHBALANCERCLASS_H



class cConnection;



class BandwidthBalancerClass
{

public:

	bool IsEnabled; // 0000
	// ...


	BandwidthBalancerClass();
	~BandwidthBalancerClass();

	void Allocate_Client_Structs(int);
	void Adjust(cConnection* connection, bool isDedicated);
	int Allocate_Bandwidth(float, int, uint32);
	void Adjust_Connection_Budget(cConnection*);

};



extern REF_DECL2(BandwidthBalancer, BandwidthBalancerClass);



#endif