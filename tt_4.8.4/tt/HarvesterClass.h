#ifndef TT_INCLUDE__HARVESTERCLASS_H
#define TT_INCLUDE__HARVESTERCLASS_H

#include "scripts.h"
#include "engine_string.h"
#include "OBBoxClass.h"
class RefineryGameObj;
class VehicleGameObj;
class HarvesterClass : public GameObjObserverClass
{

public:

   enum State
   {
      NoState             = 0,
      StateGoingToHarvest = 1,
      StateHarvesting     = 2,
      StateGoingToUnload  = 3,
      StateUnloading      = 4,
   };


private:

   RefineryGameObj* refinery;            //   8
   Vector3          dockLocation;        //  12
   Vector3          dockEntrance;        //  24
   State            state;               //  36
   OBBoxClass       tiberiumRegion;      //  40
   VehicleGameObj*  harvester;           // 100
   StringClass      harvestAnimation;    // 104
   float            harvestRelocateTime; // 108
   float            harvestTime;         // 112
   bool             harvesting;          // 116


public:

            HarvesterClass();
   virtual ~HarvesterClass();

   virtual const char*       Get_Name           ();
   virtual void        Attach             (GameObject* object);
   virtual void        Detach             (GameObject* object);
   virtual void        Created            (GameObject* object);
   virtual void        Destroyed          (GameObject* object);
   virtual void        Killed             (GameObject* object, GameObject*);
   virtual void        Damaged            (GameObject* object, GameObject*, float);
   virtual void        Custom             (GameObject* object, int, int, GameObject*);
   virtual void        Sound_Heard        (GameObject* object, const CombatSound&);
   virtual void        Enemy_Seen         (GameObject* object, GameObject*);
   virtual void        Action_Complete    (GameObject* object, int, ActionCompleteReason);
   virtual void        Timer_Expired      (GameObject* object, int);
   virtual void        Animation_Complete (GameObject* object, const char*);
   virtual void        Poked              (GameObject* object, GameObject*);
   virtual void        Entered            (GameObject* object, GameObject*);
   virtual void        Exited             (GameObject* object, GameObject*);

   void            Go_Harvest             ();
   void            Go_Unload_Tiberium     ();
   VehicleGameObj* Get_Vehicle            ();
   void            Harvest_Tiberium       ();
   void            Play_Harvest_Animation (bool play);
   void            Think                  ();
   void            Unload_Tiberium        ();

};

#endif
