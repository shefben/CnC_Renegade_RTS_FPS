#ifndef TT_INCLUDE_SOUNDPSEUDO3DCLASS_H
#define TT_INCLUDE_SOUNDPSEUDO3DCLASS_H
#include "Sound3DClass.h"

class SoundPseudo3DClass : public Sound3DClass {
public:
	virtual ~SoundPseudo3DClass();
	virtual SoundPseudo3DClass* As_SoundPseudo3DClass  ();
	virtual bool                On_Frame_Update        (uint32);
	virtual void                Set_Position            (const Vector3& position);
	virtual void                Set_Listener_Transform  (const Matrix3D& listenerTransform);
	virtual void                Set_Transform           (const Matrix3D& transform);
	virtual void                Set_DropOff_Radius      (float dropOffRadius);
	virtual float               Get_DropOff_Radius      () const;
	virtual unsigned long       Get_Class_ID            () const;
	virtual void                Update_Volume           ();
	virtual const PersistFactoryClass &Get_Factory             () const;
	virtual void                Set_Miles_Handle        (uint32 milesHandle);
	virtual void                Free_Miles_Handle       ();
	virtual void                Initialize_Miles_Handle ();
	virtual void                Allocate_Miles_Handle   ();
	virtual void                On_Loop_End             ();
	virtual void                Set_Velocity            (Vector3 const&);
	virtual void                Set_Max_Vol_Radius      (float);
	virtual float               Get_Max_Vol_Radius      ();
	virtual void                Update_Pseudo_Volume    ();
	virtual void                Update_Pseudo_Volume    (float);
	virtual void                Update_Pseudo_Pan       ();
};

#endif