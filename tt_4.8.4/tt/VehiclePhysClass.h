#ifndef TT_INCLUDE__VEHICLEPHYSCLASS_H
#define TT_INCLUDE__VEHICLEPHYSCLASS_H



#include "RigidBodyClass.h"
#include "Matrix3D.h"



class SuspensionElementClass;
class VehicleDazzleClass;
class VehiclePhysDefClass;



class VehiclePhysClass :
	public RigidBodyClass
{
public:
	VehiclePhysClass(void);
	virtual ~VehiclePhysClass(void);
	virtual VehiclePhysClass *		As_VehiclePhysClass(void)		{ return this; }
	const VehiclePhysDefClass *	Get_VehiclePhysDef(void)		{ return (VehiclePhysDefClass*)Definition; }
	void									Init(const VehiclePhysDefClass & def);
	virtual void						Definition_Changed(void);
	virtual void						Render(RenderInfoClass & rinfo);
	virtual void						Set_Model(RenderObjClass * model);
	float									Compute_Approximate_Ride_Height(void);
	virtual void						Timestep(float dt); 
	void									Enable_Engine(bool onoff)		{ IsEngineOn = onoff; }
	bool									Is_Engine_Enabled(void)			{ return IsEngineOn; }
	int									Get_Wheel_Count(void)			{ return Wheels.Length(); }
	int									Get_Drive_Wheel_Count(void)	{ return DriveWheelCount; }
	int									Get_Real_Wheel_Count(void)		{ return RealWheelCount; }
	SuspensionElementClass *		Peek_Wheel(int wheel_index);
	bool									Is_In_Contact(void);
	void									Update_Wheels (void);
	void									Non_Physical_Wheel_Update(float suspension_fraction,float rotation);
	void									Teleport_To_Last_Good_Position(void);
	virtual bool						Is_Simulation_Disabled(void)				{ return _DisableVehicleSimulation || RigidBodyClass::Is_Simulation_Disabled(); }
	virtual bool						Is_Rendering_Disabled(void)				{ return _DisableVehicleRendering || RigidBodyClass::Is_Simulation_Disabled(); }
	static void							Disable_All_Simulation(bool onoff)		{ _DisableVehicleSimulation = onoff; }
	static void							Disable_All_Rendering(bool onoff)		{ _DisableVehicleRendering = onoff; }
	static bool							Is_All_Simulation_Disabled(void)			{ return _DisableVehicleSimulation; }
	static bool							Is_All_Rendering_Disabled(void)			{ return _DisableVehicleRendering; }
	virtual bool						Save(ChunkSaveClass &csave);
	virtual bool						Load(ChunkLoadClass &cload);
protected:
	virtual void						Compute_Force_And_Torque(Vector3 * force,Vector3 * torque);
	virtual bool						Can_Go_To_Sleep(float dt);
	void									Update_Cached_Model_Parameters(void);
	void									Release_Wheels(void);
	void									Create_Wheels(void);
	int									Find_Fork_Bone(RenderObjClass * model,const char * wpname);
	int									Find_Rotation_Bone(RenderObjClass * model,const char * wpname);
	int									Find_Translation_Bone(RenderObjClass * model,const char * wpname);
	void									Release_Auxiliary_Bones(void);
	void									Capture_Auxiliary_Bones(void);
	void									Release_Dazzles(void);
	void									Capture_Dazzles(void);
	VehicleDazzleClass *				Create_Dazzle_Controller(RenderObjClass * obj);
	virtual SuspensionElementClass *	Alloc_Suspension_Element(void) = 0;
	virtual float						Get_Normalized_Engine_Flame(void) { return 0.0f; }
	bool									IsEngineOn;				// is the engine on? vehicles will stop if the engine is off
	int									RealWheelCount;		// active wheel count
	int									DriveWheelCount;		// drive wheel count
	SimpleVecClass<SuspensionElementClass *>	Wheels;	// array of wheel objects
	SimpleVecClass<VehicleDazzleClass *>		Dazzles;	// array of dazzle controllers (for headlights, taillights, blinky lights, etc)	
	SimpleVecClass<int>				EngineFlameBones;		// indices to "engine-flame" bones
	Matrix3D								LastGoodPosition;		// last state where all wheels were on the ground
	float									ExpireTimer;			// time left before blowing up when rolled over	
	static REF_DECL2(_DisableVehicleSimulation, bool);
	static REF_DECL2(_DisableVehicleRendering, bool);
}; // 0314  0314  0330  032C
#endif
