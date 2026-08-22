#ifndef TT_INCLUDE__PHYS3CLASS_H
#define TT_INCLUDE__PHYS3CLASS_H



#include "MoveablePhysClass.h"



class Vector3;
class Phys3HistoryClass;


class Phys3Class : public MoveablePhysClass
{

public:

   struct GroundStateStruct
   {
      bool       invalid;        // 0
      bool       inContact;      // 1
      bool       b2;
      uint32     contactSurface; // 4
      sint32     s8;
      Vector3    o12;
      Vector3    o24;
      PhysClass* groundObject; // 36
      sint32     s40;//40
   };//44

private:
                                       // RH7   *
   Vector3            o220;            //     192
   Vector3            o232;            //     204
   bool               inContact;       // 244 216
   bool               b245;            //     217
   bool               b246;            //     218
   uint32             contactSurface;  // 248 220
   Vector3            position;        // 252 224
   Vector3            velocity;        // 264 236
   sint32             s276;            //     248
   float              normalizedSpeed; // 280 252
   float              f284;            //     256
   float              slideAngle;      // 288 260
   float              f292;            //     264
   float              f296;            //     268
   sint32             s300;            //     272
   sint32             s304;            //     276
public:
   GroundStateStruct  o308;            //     280
   Vector3            animationMove;    //     324
   Phys3HistoryClass* history;         // 364 336
   Vector3            o368;
   Vector3            serverPosition;  // 380
   Vector3            serverVelocity;  // 392

            Phys3Class();
   virtual ~Phys3Class();
   virtual void            Timestep                     (float);
   virtual const Matrix3D& Get_Transform                () const;
   virtual void            Set_Transform                (const Matrix3D&);
   virtual bool            Cast_Ray                     (PhysRayCollisionTestClass&);
   virtual bool            Cast_AABox                   (PhysAABoxCollisionTestClass&);
   virtual bool            Cast_OBBox                   (PhysOBBoxCollisionTestClass&);
   virtual bool            Intersection_Test            (PhysAABoxIntersectionTestClass&);
   virtual bool            Intersection_Test            (PhysOBBoxIntersectionTestClass&);
   virtual bool            Intersection_Test            (PhysMeshIntersectionTestClass&);
   virtual bool            Push                         (const Vector3&);
   virtual void            Set_Model                    (RenderObjClass*);
   virtual void            Get_Shadow_Blob_Box          (AABoxClass*);
   virtual Phys3Class*     As_Phys3Class                () {return this;}
   virtual bool            Can_Teleport(const Matrix3D &new_tm, bool check_dyn_only = false,MultiListClass<PhysClass> * result_list = NULL);
   virtual bool            Can_Teleport_And_Stand(const Matrix3D &new_tm, Matrix3D *out);
   virtual bool            Find_Teleport_Location       (const Vector3&, float, Vector3*);
   virtual bool            Can_Move_To                  (const Matrix3D&);
   virtual void            Get_Velocity(Vector3 * set_vel) const;
   virtual void            Set_Velocity                 (const Vector3&);
   virtual const AABoxClass&          Get_Bounding_Box             () const;
   virtual void            Get_Collision_Box            ();
   virtual void            Apply_Impulse                (const Vector3&);
   virtual void            Apply_Impulse                (const Vector3&, const Vector3&);
   virtual bool            Is_In_Contact                ();
   virtual void            Set_In_Contact               (bool);
   virtual uint32          Get_Contact_Surface_Type     ();
   virtual void            Invalidate_Ground_State      ();
   virtual void            Set_Position                 (const Vector3&);
   virtual void            Get_Position                 ();
   virtual void            Set_Heading                  (float heading);
   virtual float         Get_Heading                  () const;
   virtual void            Set_Slide_Angle              (float);
   virtual void            Get_Slide_Angle              ();
   virtual void            Set_Normalized_Speed         (float);
   virtual void            Get_Normalized_Speed         ();
   virtual void            Collide                      (const Vector3&);
   virtual const GroundStateStruct& Get_Ground_State             () const;
   virtual void            Check_Ground                 (const AABoxClass&, GroundStateStruct*, float);
   virtual bool            User_Move                    (float);
   virtual bool            Ballistic_Move               (float);
   virtual bool            Slide_Move                   (const GroundStateStruct&, float);
   virtual bool            Normal_Move                  (const GroundStateStruct&, float);
   virtual bool            Collide_Move                 (const Vector3&, float);
   bool Apply_Move                  (const Vector3& move, float, bool, bool, bool);
   void Network_Teleport_Correction ();
   void Update_Transform            (bool);
   void Network_State_Update(const Vector3&, const Vector3&);
   void Network_Latency_State_Update(const Vector3&, const Vector3&);
};



#endif