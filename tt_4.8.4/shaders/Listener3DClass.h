#ifndef TT_INCLUDE__LISTENER3DCLASS_H
#define TT_INCLUDE__LISTENER3DCLASS_H



#include "Sound3DClass.h"



class Listener3DClass :
	public Sound3DClass
{

public:
	Listener3DClass();
	virtual ~Listener3DClass();
	virtual Listener3DClass*    As_Listener3DClass     ();
	virtual void                Set_DropOff_Radius      (float dropOffRadius);
	virtual float               Get_DropOff_Radius      ();
	virtual unsigned long       Get_Class_ID            () const;
	virtual bool                Pause                   ();
	virtual bool                Resume                  ();
	virtual bool                Stop                    (bool removeFromPlaylist);
	virtual void                Seek                    (uint32 playPosition);
	virtual uint32              Get_State               () const;
	virtual void                Free_Miles_Handle       ();
	virtual void                Initialize_Miles_Handle ();
	virtual void                Allocate_Miles_Handle   ();
	virtual void                Set_Velocity            (Vector3 const&);
	virtual void                Set_Max_Vol_Radius      (float);
	virtual float               Get_Max_Vol_Radius      ();
	virtual void                On_Added_To_Scene       ();
	virtual void                Start_Sample            ();
	virtual void                Stop_Sample             ();
	virtual void                Resume_Sample           ();
	virtual void                End_Sample              ();
	virtual void                Set_Sample_Volume       (long);
	virtual long                Get_Sample_Volume       ();
	virtual void                Set_Sample_Pan          (long);
	virtual long                Get_Sample_Pan          ();
	virtual void                Set_Sample_Loop_Count   (unsigned long);
	virtual unsigned long       Get_Sample_Loop_Count   ();
	virtual void                Set_Sample_MS_Position  (unsigned long);
	virtual void                Get_Sample_MS_Position  (long *,long *);
	virtual long                Get_Sample_Playback_Rate();
	virtual void                Set_Sample_Playback_Rate(long);
};

#endif