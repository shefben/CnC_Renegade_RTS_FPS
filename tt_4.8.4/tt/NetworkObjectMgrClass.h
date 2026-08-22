#ifndef TT_INCLUDE__NETWORKOBJECTMGRCLASS_H
#define TT_INCLUDE__NETWORKOBJECTMGRCLASS_H



#include "engine_vector.h"
#include "NetworkObjectClass.h"
class NetworkObjectMgrClass
{

public:

	static REF_DECL2(_NewDynamicID, uint);
	static REF_DECL2(_NewClientID, uint);
	static REF_DECL2(_IsLevelLoading, bool);
	static REF_DECL2(_ObjectList, DynamicVectorClass<NetworkObjectClass*>);
	static REF_DECL2(_DeletePendingList, DynamicVectorClass<NetworkObjectClass*>);


public:

	static void Register_Object(NetworkObjectClass* object);
	static void Unregister_Object(NetworkObjectClass* object);
	static NetworkObjectClass* Find_Object(uint networkId);
	static void Set_New_Dynamic_ID(uint networkId);
	static int Get_New_Dynamic_ID();
	static int Get_Current_Dynamic_ID();
	static void Init_New_Client_ID(int arg);
	static int Get_New_Client_ID();
	static bool Find_Object(uint networkId, uint& maxIndex);
	static void Think();
	static void Set_All_Delete_Pending();
	static void Delete_Pending();
	static void Delete_Client_Objects(int);
	static void Restore_Dirty_Bits(int);
	static void Register_Object_For_Deletion(NetworkObjectClass* object);
	static void Reset_Import_State_Counts();
	static NetworkObjectClass* Get_Object(int objectIndex) { return _ObjectList[objectIndex]; }
	static int Get_Object_Count() { return _ObjectList.Count(); }

};



#endif
