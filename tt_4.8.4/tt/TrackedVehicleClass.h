#ifndef TT_INCLUDE__TRACKEDVEHICLECLASS_H
#define TT_INCLUDE__TRACKEDVEHICLECLASS_H



#include "VehiclePhysClass.h"



class TrackedVehicleDefClass;
class MeshClass;
class LinearOffsetTextureMapperClass;



class TrackedVehicleClass :
	public VehiclePhysClass
{
public:
	TrackedVehicleClass(void);
	virtual ~TrackedVehicleClass(void);
	virtual TrackedVehicleClass * As_TrackedVehicleClass(void) { return this; }
	const TrackedVehicleDefClass * Get_TrackedVehicleDef(void) { return (TrackedVehicleDefClass *)Definition; }
	void								Init(const TrackedVehicleDefClass & def);
	virtual void					Render(RenderInfoClass & rinfo);
	virtual void					Set_Model(RenderObjClass * model);
	float								Get_Left_Track_Movement(void)		{ return LeftTrackMovement; }
	float								Get_Right_Track_Movement(void)	{ return RightTrackMovement; }
	virtual const PersistFactoryClass &	Get_Factory (void) const;
	virtual bool								Save (ChunkSaveClass &csave);
	virtual bool								Load (ChunkLoadClass &cload);
	virtual void								On_Post_Load (void);
	enum { LEFT_TRACK = 0, RIGHT_TRACK = 1 };
#ifdef HOVER_TEST
	void Stabilizer_Compute_Force_And_Torque(const Matrix3D& transform, Vector3* force, Vector3* torque);
#endif
protected:
	virtual SuspensionElementClass *	Alloc_Suspension_Element(void);
	virtual void					Compute_Force_And_Torque(Vector3 * force,Vector3 * torque);
	void								Update_Cached_Model_Parameters(void);
	void								Grab_Track_Mappers(RenderObjClass * model);
	void								Add_Track_Mappers(MeshClass * mesh,int track_type);
	float								LeftTrackTorque;
	float								RightTrackTorque;
	float								LeftTrackMovement;
	float								RightTrackMovement;
	Vector3							LeftTrackLastPosition;
	Vector3							RightTrackLastPosition;
	struct TrackMapperStruct
	{
		TrackMapperStruct(void) : Mapper(NULL), TrackType(LEFT_TRACK) { }
		LinearOffsetTextureMapperClass *		Mapper;
		int											TrackType;
	};
	SimpleDynVecClass<TrackMapperStruct>	TrackMappers;
private:
	TrackedVehicleClass(const TrackedVehicleClass &);
	TrackedVehicleClass & operator = (const TrackedVehicleClass &);
}; // 034C  034C  0368  0364

#endif