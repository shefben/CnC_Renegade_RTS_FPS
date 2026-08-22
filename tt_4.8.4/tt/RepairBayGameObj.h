#ifndef TT_INCLUDE__REPAIRBAYGAMEOBJ_H
#define TT_INCLUDE__REPAIRBAYGAMEOBJ_H

#include "BuildingGameObj.h"
#include "Matrix3D.h"
#include "engine_common.h"
#include "SimplePersistFactoryClass.h"
#include "RepairBayGameObjDef.h"
#include "OBBoxClass.h"

class RepairBayGameObj : public BuildingGameObj
{
private:
   static const char* BoneNames[6];
   float                               repairWaitTime;    // 2112
   OBBoxClass                          repairZone;        // 2116
   uint32                              physId;            // 2176
   bool                                isRepairing;       // 2180
   DynamicVectorClass<ReferencerClass> vehicles;          // 2184 2164
   BuildingAggregateClass*             repairEffect;      // 2208
   PhysicalGameObj*                    arcEffects[4];     // 2212
   float                               arcDisplayTime[4]; // 2228
   Matrix3D                            lastBoneTransform; // 2244
   Matrix3D                            boneTransforms[6]; // 2292 2272
public:
   RepairBayGameObj();
   virtual ~RepairBayGameObj();
   virtual const PersistFactoryClass& Get_Factory  () const;
   virtual bool              Save                (ChunkSaveClass& chunkSaver);
   virtual bool              Load                (ChunkLoadClass& chunkLoader);
   virtual void              Init                ();
   virtual void              Think               ();
   virtual RepairBayGameObj* As_RepairBayGameObj () {return this;}
   virtual void              CnC_Initialize      (BaseControllerClass* base);
   virtual void              Import_Creation     (BitStreamClass& stream);
   virtual void              Export_Creation     (BitStreamClass& stream);

   void        Emit_Welding_Arc            (RenderObjClass* targetModel);
   void        Load_Variables              (ChunkLoadClass& chunkLoader);
   void        Play_Repairing_Animation    (bool play);
   bool        Repair_Vehicle              ();
   void        Update_Repairing_Animations ();
   void        Init(const RepairBayGameObjDef &);
   const RepairBayGameObjDef & Get_Definition( void ) const ;
};


#endif
