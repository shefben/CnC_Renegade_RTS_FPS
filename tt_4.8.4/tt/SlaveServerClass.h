#ifndef TT_INCLUDE__SLAVESERVERCLASS_H
#define TT_INCLUDE__SLAVESERVERCLASS_H



class SlaveServerClass
{

private:
	
	char Name[0x20]; // 0000  0000;
	char Serial[0x40]; // 0020  0020;
	char Password[0x40]; // 0060  0060;
	unsigned short Port; // 00A0  00A0
	char Settings[0x104]; // 00A2  00A2;
	bool isEnabled; // 01A6  01A6
	int unk01A8; // 01A8  01A8
	int Bandwidth; // 01AC  01AC
	int unk01B0; // 01B0  01B0
	int unk01B4; // 01B4  01B4
	int ProcessID; // 01B8  ----
	int unk01BC; // 01BC  ----


public:

}; // 01C0  01B8



#endif