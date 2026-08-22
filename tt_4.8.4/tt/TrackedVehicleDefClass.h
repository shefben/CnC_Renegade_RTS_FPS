#ifndef TT_INCLUDE__TRACKEDVEHICLEDEFCLASS_H
#define TT_INCLUDE__TRACKEDVEHICLEDEFCLASS_H



#include "VehiclePhysDefClass.h"
#include "Vector2.h"



class TrackedVehicleDefClass :
	public VehiclePhysDefClass
{
public:
	TrackedVehicleDefClass(void);
	virtual uint32								Get_Class_ID (void) const;
	virtual PersistClass *					Create(void) const;
	virtual const char *						Get_Type_Name(void)				{ return "TrackedVehicleDef"; }
	virtual bool								Is_Type(const char *);
	virtual const PersistFactoryClass &	Get_Factory (void) const;
	virtual bool								Save(ChunkSaveClass &csave);
	virtual bool								Load(ChunkLoadClass &cload);
	float											Get_Max_Engine_Torque(void) const { return MaxEngineTorque; }
	float											Get_Track_U_Scale_Factor(void) const { return TrackUScaleFactor; }
	float											Get_Track_V_Scale_Factor(void) const { return TrackVScaleFactor; }
	float											Get_Turn_Torque_Scale_Factor(void) const { return TurnTorqueScaleFactor; }
	void											Set_Max_Engine_Torque(float t) { MaxEngineTorque = t; }
	void											Set_Track_U_Scale_Factor(float k) { TrackUScaleFactor = k; }
	void											Set_Track_V_Scale_Factor(float k) { TrackVScaleFactor = k; }
	void											Set_Turn_Torque_Scale_Factor(float k) { TurnTorqueScaleFactor = k; }
	DECLARE_EDITABLE(TrackedVehicleDefClass,VehiclePhysDefClass);
protected:
	float											MaxEngineTorque;
	float											TrackUScaleFactor;
	float											TrackVScaleFactor;
	float											TurnTorqueScaleFactor;
	friend class TrackedVehicleClass;
};

#endif
