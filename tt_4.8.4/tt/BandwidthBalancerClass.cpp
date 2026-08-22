#include "General.h"
#include "BandwidthBalancerClass.h"



REF_DEF2(BandwidthBalancer, BandwidthBalancerClass, 0x00854FB8, 0x008541A0);



RENEGADE_FUNCTION
void BandwidthBalancerClass::Adjust(cConnection* connection, bool isDedicated)
AT2(0x00620CF0, 0x00620590);
