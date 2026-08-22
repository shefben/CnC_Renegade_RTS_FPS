#ifndef TT_INCLUDE__SLAVEMASTERCLASS_H
#define TT_INCLUDE__SLAVEMASTERCLASS_H



#include "SlaveServerClass.h"



class SlaveMasterClass
{

private:

	SlaveServerClass slaves[7];
	int slaveCount; // 0C40  0C08
	bool iAmSlave; // 0C44  0C0C

public:

	SlaveMasterClass();
	~SlaveMasterClass();
	void Wait_For_Slave_Shutdown();
	SlaveServerClass &Get_Slave(int);
	bool Save();
	bool Load();
	void Reset();
	void Add_Slave(bool, char*, char*, uint16, char*, int, char*);
	bool Aquire_Slave(int);
	void Startup_Slaves();
	void Shutdown_Slaves();
	bool Shutdown_Slave(char*);
	void Get_Slave_Info(char*, int);
	void Create_Registry_Copies();
	void Delete_Registry_Copies();
	int Get_Num_Enabled_Slaves();
	
	bool Am_I_Slave() const { return iAmSlave; }

}; // 0C48  0C10



extern REF_DECL2(SlaveMaster, SlaveMasterClass);



#endif