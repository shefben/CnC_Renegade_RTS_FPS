#include "general.h"
#include "BuildingGameObj.h"
#include "BuildingGameObjDef.h"
#include "bitstream.h"



REF_DEF2(BuildingGameObj::CanRepairBuildings, bool, 0x0081129C, 0x00810474);



RENEGADE_FUNCTION
void BuildingGameObj::Add_Aggregate
   (BuildingAggregateClass* aggregate)
   AT2(0x00683F20,0x006837C0);

RENEGADE_FUNCTION
void BuildingGameObj::CnC_Initialize
   (BaseControllerClass* base)
   AT2(0x006840E0,0x00683980);

RENEGADE_FUNCTION
void BuildingGameObj::Initialize_Building()
   AT2(0x00683970,0x00683210);

void BuildingGameObj::Import_Creation
   (BitStreamClass& stream)
{
	ScriptableGameObj::Import_Creation(stream);
	stream.Get(position.X,BITPACK_WORLD_POSITION_X);
	stream.Get(position.Y,BITPACK_WORLD_POSITION_Y);
	stream.Get(position.Z,BITPACK_WORLD_POSITION_Z);
	stream.Get(collectionSphere.Center.X,BITPACK_WORLD_POSITION_X);
	stream.Get(collectionSphere.Center.Y,BITPACK_WORLD_POSITION_Y);
	stream.Get(collectionSphere.Center.Z,BITPACK_WORLD_POSITION_Z);
	stream.Get(collectionSphere.Radius,BITPACK_BUILDING_RADIUS);
	Collect_Building_Components();
	Update_State(false);
	Initialize_Building();
}

void BuildingGameObj::Export_Creation
   (BitStreamClass& stream)
{
	ScriptableGameObj::Export_Creation(stream);
	stream.Add(position.X,BITPACK_WORLD_POSITION_X);
	stream.Add(position.Y,BITPACK_WORLD_POSITION_Y);
	stream.Add(position.Z,BITPACK_WORLD_POSITION_Z);
	stream.Add(collectionSphere.Center.X,BITPACK_WORLD_POSITION_X);
	stream.Add(collectionSphere.Center.Y,BITPACK_WORLD_POSITION_Y);
	stream.Add(collectionSphere.Center.Z,BITPACK_WORLD_POSITION_Z);
	stream.Add(collectionSphere.Radius,BITPACK_BUILDING_RADIUS);
}

void BuildingGameObj::Import_Rare
   (BitStreamClass& stream)
{
   DamageableGameObj::Import_Rare (stream);
   bool Powered = powered;
   bool Destroyed = destroyed;
   int State = state;
   stream.Get(Destroyed);
   stream.Get(Powered);
   stream.Get(State,BITPACK_BUILDING_STATE);
   if (Powered != powered)
   {
	   Enable_Power(Powered);
   }
   if (Destroyed)
   {
	   Defense.Set_Health(0);
   }
   if (State != state)
   {
	   Update_State(false);
   }
   if (Destroyed)
   {
	   if (!destroyed)
	   {
		   On_Destroyed();
	   }
   }
}

void BuildingGameObj::Export_Rare
   (BitStreamClass& stream)
{
   DamageableGameObj::Export_Rare (stream);
   stream.Add(destroyed);
   stream.Add(powered);
   stream.Add(state,BITPACK_BUILDING_STATE);
}

RENEGADE_FUNCTION
void BuildingGameObj::Update_State
   (bool)
   AT2(0x00682FD0,0x00682870);

RENEGADE_FUNCTION
void BuildingGameObj::Enable_Power
   (bool enable)
   AT2(0x00682800,0x006820A0);



void BuildingGameObj::Set_Precision()
{
	cEncoderList::Set_Precision(BITPACK_BUILDING_STATE,-1.0f,10.0f,1.0f);
}

RENEGADE_FUNCTION
void BuildingGameObj::Apply_Damage_Building(const OffenseObjectClass&, StaticPhysClass*)
AT2(0x006829A0, 0x00682240);

RENEGADE_FUNCTION
void BuildingGameObj::Apply_Damage_Building(const OffenseObjectClass &offense,bool b)
AT2(0x00682C40,0x006824E0);

RENEGADE_FUNCTION
void BuildingGameObj::Find_Closest_Poly(const Vector3& oPosition, float* fTarget)
AT2(0x00685630,0x00684ED0);

RENEGADE_FUNCTION
void BuildingGameObj::Play_Announcement(int,bool)
AT2(0x00683670,0x00682F10);
