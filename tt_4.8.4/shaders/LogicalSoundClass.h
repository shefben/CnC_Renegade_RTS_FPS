#ifndef TT_INCLUDE__LOGICALSOUNDCLASS_H
#define TT_INCLUDE__LOGICALSOUNDCLASS_H



#include "SoundSceneObjClass.h"



class LogicalSoundClass :
	public SoundSceneObjClass
{

public:
	
	virtual ~LogicalSoundClass();
	virtual bool On_Frame_Update(uint);
	virtual void Set_Position(const Vector3&);
	virtual Vector3 Get_Position();
	virtual void Set_Transform(const Matrix3D&);
	virtual Matrix3D Get_Transform();
	virtual void Cull_Sound(bool);
	virtual bool Is_Sound_Culled();
	virtual void Add_To_Scene(bool);
	virtual void Remove_From_Scene();
	virtual void Set_DropOff_Radius(float);
	virtual float Get_DropOff_Radius();
	virtual bool Save(ChunkSaveClass&);
	virtual bool Load(ChunkLoadClass&);
	virtual bool Is_Single_Shot();
	virtual UNK Set_Single_Shot(bool);
	virtual UNK Set_Type_Mask(uint32);
	virtual UNK Get_Type_Mask();
	virtual UNK Get_Notify_Delay();
	virtual UNK Set_Notify_Delay(float);
	virtual UNK Allow_Notify(uint32);
	virtual UNK Get_Listener_Timestamp();
	virtual UNK Set_Listener_Timestamp(int);
	virtual UNK Get_Factory();

	LogicalSoundClass();

};



#endif