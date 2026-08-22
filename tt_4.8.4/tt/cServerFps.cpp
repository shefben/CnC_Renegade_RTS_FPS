#include "General.h"
#include "cServerFps.h"



#include "bitstream.h"



REF_DEF2(cServerFps::TheInstance, cServerFps*, 0x0082FD00, 0x0082EEE8);



void cServerFps::Create_Instance()
{
	TheInstance = new cServerFps;
}



void cServerFps::Destroy_Instance()
{
	delete TheInstance;
	TheInstance = NULL;
}



cServerFps::cServerFps()
{
	Set_Network_ID(2100000006);
	PacketType = 19;

	fps = 0;
}



void cServerFps::Export_Frequent(BitStreamClass& bitStream)
{
	bitStream.Add(fps);
}



void cServerFps::Import_Frequent(BitStreamClass& bitStream)
{
	bitStream.Get(fps);
}
