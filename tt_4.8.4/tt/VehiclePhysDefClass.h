#ifndef TT_INCLUDE__VEHICLEPHYSDEFCLASS_H
#define TT_INCLUDE__VEHICLEPHYSDEFCLASS_H



#include "RigidBodyDefClass.h"



class VehiclePhysDefClass :
	public RigidBodyDefClass
{
public:
	VehiclePhysDefClass(void);
	virtual ~VehiclePhysDefClass(void);
	virtual uint32								Get_Class_ID (void) const;
	virtual const char *						Get_Type_Name(void)						{ return "VehiclePhysDef"; }
	virtual bool								Is_Type(const char *);
	float											Get_Spring_Constant(void) const		{ return SpringConstant; }
	float											Get_Damping_Constant(void) const		{ return DampingConstant; }
	float											Get_Spring_Length(void) const			{ return SpringLength; }
	float											Get_Traction_Multiplier(void) const	{ return TractionMultiplier; }
	float											Get_Lateral_Moment_Arm(void) const	{ return LateralMomentArm; }
	float											Get_Tractive_Moment_Arm(void) const { return TractiveMomentArm; }
	bool											Is_Fake(void) const						{ return IsFake; }
	void											Set_Spring_Constant(float ks)			{ SpringConstant = ks; }
	void											Set_Damping_Constant(float kd)		{ DampingConstant = kd; }
	void											Set_Spring_Length(float l)				{ SpringLength = l; }
	void											Set_Traction_Multiplier(float k)		{ TractionMultiplier = k; }
	void											Set_Lateral_Moment_Arm(float r)		{ LateralMomentArm = r; }
	void											Set_Tractive_Moment_Arm(float r)		{ TractiveMomentArm = r; }
	virtual const PersistFactoryClass &	Get_Factory (void) const;
	virtual bool								Save(ChunkSaveClass &csave);
	virtual bool								Load(ChunkLoadClass &cload);
	DECLARE_EDITABLE(VehiclePhysDefClass,RigidBodyDefClass);
protected:
	float											SpringConstant;			// suspension spring constant
	float											DampingConstant;			// suspension damping constant (shock absorber)
	float											SpringLength;				// suspension spring length
	float											TractionMultiplier;		// scales the downforce for more traction
	float											LateralMomentArm;			// arbitrary dist from x-y plane to apply lateral tire forces
	float											TractiveMomentArm;		// arbitrary dist from x-y plane to apply tractive tire forces
	float											EngineFlameLength;		// max length of the engine flame (how far to translate at flame=1.0)
	bool											IsFake;						// short-circuit all possible physics calculations!	
	friend class VehiclePhysClass;
};

#endif
