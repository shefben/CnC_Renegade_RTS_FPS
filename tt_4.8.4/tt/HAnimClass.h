#ifndef TT_INCLUDE__HANIMCLASS_H
#define TT_INCLUDE__HANIMCLASS_H

#include "scripts.h"
#include "engine_common.h"
#include "engine_vector.h"
#include "engine_threading.h"
#include "engine_string.h"
#include "Quaternion.h"
#include "Matrix3D.h"
class HashableClass
{
	HashableClass* NextHash;
	virtual ~HashableClass();
	virtual char *Get_Key() = 0;
};
class HAnimClass : public RefCountClass, public HashableClass
{

   bool HasEmbeddedSounds; //40

public:

   virtual ~HAnimClass();

   virtual const char* Get_Name                () const = 0;
   virtual const char* Get_HName               () const = 0;
   virtual uint32      Get_Num_Frames          () const = 0;
   virtual float       Get_Frame_Rate          () = 0;
   virtual float       Get_Total_Time          () = 0;
   virtual void        Get_Translation         (sint32, float);
   virtual void        Get_Orientation         (sint32, float);
   virtual void        Get_Translation         (Vector3&, sint32, float) = 0;
   virtual void        Get_Orientation         (Quaternion&, sint32, float) = 0;
   virtual void        Get_Transform           (Matrix3D&, sint32, float) = 0;
   virtual bool        Get_Visibility          (sint32, float) = 0;
   virtual uint32      Get_Num_Pivots          () = 0;
   virtual bool        Is_Node_Motion_Present  () = 0;
   virtual bool        Has_X_Translation       (sint32) const;
   virtual bool        Has_Y_Translation       (sint32) const;
   virtual bool        Has_Z_Translation       (sint32) const;
   virtual bool        Has_Rotation            (sint32) const;
   virtual bool        Has_Visibility          (sint32) const;
   virtual bool        Has_Embedded_Sounds     () const;
   virtual void        Set_Has_Embedded_Sounds (bool);

};


#endif
