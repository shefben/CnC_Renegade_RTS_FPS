#ifndef __SOUNDSCENEOBJCLASS_H__
#define __SOUNDSCENEOBJCLASS_H__

#include "scripts.h"
#include "engine_common.h"
#include "engine_threading.h"
#include "engine_string.h"
#include "engine_vector.h"
#include "ScriptableGameObj.h"
#include "Matrix3D.h"
class SoundSceneClass;
class SoundCullObjClass;
class Sound3DClass;
class SoundPseudo3DClass;
class FilteredSoundClass;
class Listener3DClass;
class AudibleSoundClass;
class RenderObjClass;
class SoundSceneObjClass : public MultiListObjectClass, public PersistClass, public RefCountClass
{
  static REF_DECL1(m_NextAvailableID, uint32);

public:
  
  SoundSceneClass*           xScene;         // 48 30
  SoundCullObjClass*         xCullObj;       // 52 34
  AudioCallbackClass*        xCallback;      // 56 38
  AudioCallbackClass::Events eCallbackEvent; // 60 3C
  uint32                     uID;            // 64 40
  RenderObjClass*            xParent;        // 68 44
  sint32                     iParentBone;    // 72 48
  uint32                     uUserData;      // 76 4C
  RefCountClass*             xUser;          // 80 50

  static bool Find_Sound_Object       (uint32 uID, uint32* uIndex);
  static void Register_Sound_Object   (SoundSceneObjClass* xSound);
  static void Unregister_Sound_Object (SoundSceneObjClass* xSound);

           SoundSceneObjClass ();
           SoundSceneObjClass (const SoundSceneObjClass& xSound);
  virtual ~SoundSceneObjClass ();

  virtual bool Save (ChunkSaveClass& xSave);
  virtual bool Load (ChunkLoadClass& xLoad);

  virtual Sound3DClass*       As_Sound3DClass        ();
  virtual SoundPseudo3DClass* As_SoundPseudo3DClass  ();
  virtual FilteredSoundClass* As_FilteredSoundClass  ();
  virtual Listener3DClass*    As_Listener3DClass     ();
  virtual AudibleSoundClass*  As_AudibleSoundClass   ();
  virtual unsigned long       Get_ID                 ();
  virtual void                Set_ID                 (uint32 uID);
  virtual bool                On_Frame_Update        (uint32);
  virtual void                On_Event               (AudioCallbackClass::Events eCallbackEvent,
                                                      LogicalListenerClass* xListener,
                                                      LogicalSoundClass* xSound);
  virtual void                Register_Callback      (AudioCallbackClass::Events eCallbackEvent,
                                                      AudioCallbackClass* xCallback);
  virtual void                Remove_Callback        ();
  virtual void                Set_Position           (const Vector3& xPosition) = 0;
  virtual Vector3             Get_Position           () const = 0;
  virtual void                Set_Listener_Transform (const Matrix3D& xTransform);
  virtual void                Set_Transform          (const Matrix3D& xTransform) = 0;
  virtual Matrix3D            Get_Transform          () const = 0;
  virtual void                Cull_Sound             (bool bCulled) = 0;
  virtual bool                Is_Sound_Culled        () const = 0;
  virtual void                Set_User_Data          (RefCountClass*, uint32);
  virtual uint32              Get_User_Data          () const;
  virtual RefCountClass*      Peek_User_Obj          () const;
  virtual void                Attach_To_Object       (RenderObjClass* xParent, sint32 iBone);
  virtual void                Attach_To_Object       (RenderObjClass* xParent, const char* cBone);
  virtual RenderObjClass*     Peek_Parent_Object     () const;
  virtual sint32              Get_Parent_Bone        () const;
  virtual void                Apply_Auto_Position    ();
  virtual void                Add_To_Scene           (bool) = 0;
  virtual void                Remove_From_Scene      () = 0;
  virtual bool                Is_In_Scene            () const;
  virtual void                Set_DropOff_Radius     (float fDropOffRadius) = 0;
  virtual float               Get_DropOff_Radius     () const = 0;
  virtual SoundCullObjClass*  Peek_Cullable_Wrapper  () const;
  virtual void                Set_Cullable_Wrapper   (SoundCullObjClass* xCullObj);

}; // size: 84 54


#endif
