#ifndef TT_INCLUDE_LOGICALLISTENERCLASS_H
#define TT_INCLUDE_LOGICALLISTENERCLASS_H
#include "SoundSceneObjClass.h"

class LogicalListenerClass : public SoundSceneObjClass {
public:
  virtual void                Set_Position           (const Vector3& xPosition);
  virtual Vector3             Get_Position           () const;
  virtual void                Set_Transform          (const Matrix3D& xTransform);
  virtual Matrix3D            Get_Transform          () const;
  virtual void                Cull_Sound             (bool bCulled);
  virtual bool                Is_Sound_Culled        () const;
  virtual void                Add_To_Scene           (bool);
  virtual void                Remove_From_Scene      ();
  virtual void                Set_DropOff_Radius     (float fDropOffRadius);
  virtual float               Get_DropOff_Radius     () const;
  virtual void                Set_Type_Mask          (unsigned long mask);
  virtual unsigned long       Get_Type_Mask          () const;
  virtual void                Set_Scale              (float scale);
  virtual float               Get_Scale              () const;
  virtual float               Get_Effective_Scale    () const;
};

#endif