#ifndef TT_INCLUDE__CPRIORITY_H
#define TT_INCLUDE__CPRIORITY_H



class Vector3;
class SoldierGameObj;
class NetworkObjectClass;



class cPriority
{

	static REF_DECL2(TURRET_FACTOR, UNK);
	static REF_DECL2(VEHICLE_FACTOR, UNK);
	static REF_DECL2(SOLDIER_FACTOR, UNK);
	static REF_DECL2(SOLDIER_IN_VEHICLE_FACTOR, UNK);
	static REF_DECL2(BUILDING_FACTOR, UNK);
	static REF_DECL2(MaxDistance, UNK);

public:
	
	static float Compute_Object_Priority(int, const Vector3&, NetworkObjectClass*, bool, SoldierGameObj*);
	static UNK Compute_Facing_Factor(int, const Vector3&, NetworkObjectClass*, SoldierGameObj*);
	static float Get_Object_Distance(const Vector3&, NetworkObjectClass*);
	static UNK Compute_Type_Factor(NetworkObjectClass*);
	static UNK Compute_Relevance_Factor(int, NetworkObjectClass*, SoldierGameObj*);
	static float Compute_Object_Priority_2(int, const Vector3&, NetworkObjectClass*, bool, SoldierGameObj*);
	static UNK Compute_Facing_Factor_2(int, const Vector3&, NetworkObjectClass*, SoldierGameObj*);
	static float Get_Object_Distance_2(const Vector3&, NetworkObjectClass*);
	static UNK Compute_Type_Factor_2(NetworkObjectClass*, float);
	static UNK Compute_Relevance_Factor_2(int, NetworkObjectClass*, SoldierGameObj*);

};



#endif