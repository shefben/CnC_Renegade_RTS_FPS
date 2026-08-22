#include "General.h"
#include "SlaveMasterClass.h"



REF_DEF2(SlaveMaster, SlaveMasterClass, 0x0081F5F8, 0x0081E7E0);



RENEGADE_FUNCTION
void SlaveMasterClass::Wait_For_Slave_Shutdown()
AT2(0x00453490, 0x004535A0);



RENEGADE_FUNCTION
SlaveServerClass &SlaveMasterClass::Get_Slave(int)
AT2(0x004536C0, 0x004537D0);



RENEGADE_FUNCTION
bool SlaveMasterClass::Save()
AT2(0x004536F0, 0x00453800);



RENEGADE_FUNCTION
bool SlaveMasterClass::Load()
AT2(0x004539D0, 0x00453AE0);



RENEGADE_FUNCTION
void SlaveMasterClass::Reset()
AT2(0x00453DA0, 0x00453EB0);



RENEGADE_FUNCTION
void SlaveMasterClass::Add_Slave(bool, char*, char*, uint16, char*, int, char*)
AT2(0x00453E40, 0x00453F50);



RENEGADE_FUNCTION
bool SlaveMasterClass::Aquire_Slave(int)
AT2(0x00453EE0, 0x00453FF0);



RENEGADE_FUNCTION
void SlaveMasterClass::Startup_Slaves()
AT2(0x00454130, 0x00454240);



RENEGADE_FUNCTION
void SlaveMasterClass::Shutdown_Slaves()
AT2(0x004545B0, 0x004546C0);



RENEGADE_FUNCTION
bool SlaveMasterClass::Shutdown_Slave(char*)
AT2(0x004547B0, 0x004548C0);



RENEGADE_FUNCTION
void SlaveMasterClass::Get_Slave_Info(char*, int)
AT2(0x004549F0, 0x00454B00);



RENEGADE_FUNCTION
void SlaveMasterClass::Create_Registry_Copies()
AT2(0x00454B00, 0x00454C10);



RENEGADE_FUNCTION
void SlaveMasterClass::Delete_Registry_Copies()
AT2(0x004553F0, 0x00455500);



RENEGADE_FUNCTION
int SlaveMasterClass::Get_Num_Enabled_Slaves()
AT2(0x004554D0, 0x004555E0);
