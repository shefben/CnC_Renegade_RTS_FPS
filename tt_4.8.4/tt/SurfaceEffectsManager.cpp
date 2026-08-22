#include "General.h"
#include "SurfaceEffectsManager.h"

#include "ArmorWarheadManager.h"
#include "SurfaceEffect.h"
#include "PhysicsConstants.h"
#include "RefCountedReferencerClass.h"
#include "WWAudioClass.h"
#include "CombatManager.h"
#include "CCameraClass.h"
#include "CollisionMath.h"
#include "PhysicsSceneClass.h"
#include "FileHashEvent.h"


RENEGADE_FUNCTION
void EffectRecyclerClass::Spawn_Effect(char  const*, Matrix3D  const&, float)
AT2(0x0072CA50,0x0072C2F0);
RENEGADE_FUNCTION
void EffectRecyclerClass::Object_Removed_From_Scene(PhysClass *)
AT2(0x0072CB60,0x0072C400);
RENEGADE_FUNCTION
void EffectRecyclerClass::Reset()
AT2(0x0072C9E0, 0x0072C280);

EffectRecyclerClass _EmitterRecycler;



REF_DEF2(SurfaceEffectsManager::Mode, int, 0x00812034, 0x0081120C);
REF_DEF2(SurfaceEffectsManager::OverrideSurfaceType, int, 0x00812038, 0x00811210);
REF_ARR_DEF2(SurfaceDamageRate, float, 32, 0x0085C038, 0x0085B220);
REF_ARR_DEF2(SurfaceDamageWarhead, int, 32, 0x0085BF98, 0x0085B180);
bool SurfaceStopsBullets[32];
bool _IsSurfaceEffectsInitted;
typedef SurfaceEffect* SurfaceEffectsDatabaseType[32][27];
REF_DEF2(SurfaceEffectsDatabase, SurfaceEffectsDatabaseType, 0x0085B218, 0x0085A400);



const char* SURFACE_TYPE_STRINGS[32] =
{
	"Light Metal",
	"Heavy Metal",
	"Water",
	"Sand",
	"Dirt",
	"Mud",
	"Grass",
	"Wood",
	"Concrete",
	"Flesh",
	"Rock",
	"Snow",
	"Ice",
	"Default",
	"Glass",
	"Cloth",
	"Tiberium Field",
	"Foliage Permeable",
	"Glass Permeable",
	"Ice Permeable",
	"Cloth Permeable",
	"Electrical",
	"Electrical Permeable",
	"Flammable",
	"Flammable Permeable",
	"Steam",
	"Steam Permeable",
	"Water Permeable",
	"Tiberium Water",
	"Tiberium Water Permeable",
	"Underwater Dirt",
	"Underwater Tiberium Dirt",
};



const char* HitterTypeName[27] =
{
	"None",
	"Generic Object",
	"Footstep Run",
	"Footstep Walk",
	"Footstep Crouched",
	"Footstep Jump",
	"Footstep Land",
	"Bullet",
	"Bullet Fire",
	"Bullet Grenade",
	"Bullet Chem",
	"Bullet Electric",
	"Bullet Laser",
	"Bullet Squish",
	"Bullet Tib Spray",
	"Bullet Tib",
	"Bullet Shotgun",
	"Bullet Silenced",
	"Bullet Sniper",
	"Bullet Water",
	"Eject Casing",
	"Shell Shotgun",
	"Tire Rolling",
	"Tire Sliding",
	"Track Rolling",
	"Track Sliding",
	"Footstep Ladder",
};


const int HitterPhysicsTypes[27] = { -1, -1, -1, -1, -1, 0, -1, 1, -1, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };



void SurfaceEffectsManager::Init()
{
	if (_IsSurfaceEffectsInitted)
		SurfaceEffectsManager::Shutdown();

	for (uint surfaceId = 0; surfaceId < 32; ++surfaceId)
	{
		SurfaceStopsBullets[surfaceId] = true;
		SurfaceDamageWarhead[surfaceId] = NULL;
		SurfaceDamageRate[surfaceId] = NULL;
	}

	FileClass *f = Get_Data_File("surfaceeffects.INI");
	if ((f) && (f->Is_Available()))
	{
		unsigned int size = f->Size();
		char *data = new char[size];
		f->Open(1);
		f->Read(data,size);
		CheckGlobal(data,size,"surfaceeffects.ini",HashSurfaceEffectsIni);
		delete[] data;
		f->Close();
		Close_Data_File(f);
	}
	INIClass* configuration = Get_INI("surfaceeffects.ini");
	if (configuration)
	{
		for (uint surfaceId = 0; surfaceId < 32; ++surfaceId)
			SurfaceStopsBullets[surfaceId] = configuration->Get_Bool("Bullet_Stopping", SURFACE_TYPE_STRINGS[surfaceId], true);

		for (uint surfaceId = 0; surfaceId < 32; ++surfaceId)
		{
			StringClass warheadType;
			configuration->Get_String(warheadType, "Damage_Warhead", SURFACE_TYPE_STRINGS[surfaceId], "");
			if (warheadType)
				SurfaceDamageWarhead[surfaceId] = ArmorWarheadManager::Get_Warhead_Type(warheadType);

			SurfaceDamageRate[surfaceId] = configuration->Get_Float("Damage_Rate", SURFACE_TYPE_STRINGS[surfaceId], 0.f);
		}

		for (uint surfaceId = 0; surfaceId < 32; ++surfaceId)
		{
			for (uint hitterId = 0; hitterId < 27; ++hitterId)
			{
				StringClass section;
				section.Format("%s_%s", SURFACE_TYPE_STRINGS[surfaceId], HitterTypeName[hitterId]);
				if (configuration->Entry_Count(section) != 0)
				{
					SurfaceEffect& surfaceEffect = *new SurfaceEffect;
					for (uint soundIndex = 0; ; ++soundIndex)
					{
						StringClass key, value;
						key.Format("Sound%d", soundIndex);
						configuration->Get_String(value, section, key, "");
						if (value.Is_Empty())
							break;

						surfaceEffect.sounds.Add_String(value);
					}

					for (uint emitterIndex = 0; ; ++emitterIndex)
					{
						StringClass key, value;
						key.Format("Emitter%d", emitterIndex);
						configuration->Get_String(value, section, key, "");
						if (value.Is_Empty())
							break;

						surfaceEffect.emitters.Add_String(value);
					}

					for (uint decalIndex = 0; ; ++decalIndex)
					{
						StringClass key, value;
						key.Format("Decal%d", decalIndex);
						configuration->Get_String(value, section, key, "");
						if (value.Is_Empty())
							break;

						surfaceEffect.decals.Add_String(value);
					}

					surfaceEffect.decalSize = configuration->Get_Float(section, "DecalSize", 1.f);
					surfaceEffect.decalSizeRandom = configuration->Get_Float(section, "DecalSizeRandom", 0);

					if (HitterPhysicsTypes[hitterId] != -1)
					{
						PhysicsConstants::Set_Contact_Friction_Coefficient(HitterPhysicsTypes[hitterId], surfaceId, configuration->Get_Float(section, "Friction", PhysicsConstants::DefaultContactFriction));
						PhysicsConstants::Set_Contact_Drag_Coefficient(HitterPhysicsTypes[hitterId], surfaceId, configuration->Get_Float(section, "Drag", PhysicsConstants::DefaultContactDrag));
					}

					SurfaceEffectsDatabase[surfaceId][hitterId] = &surfaceEffect;
				}
				else
					SurfaceEffectsDatabase[surfaceId][hitterId] = NULL;
			}
		}

		Release_INI(configuration);
	}

	_IsSurfaceEffectsInitted = true;
}



void SurfaceEffectsManager::Shutdown()
{
	_EmitterRecycler.Reset();
	for (unsigned int i = 0;i < 32;i++)
	{
		for (unsigned int j = 0;j < 27;j++)
		{
			SAFE_DELETE(SurfaceEffectsDatabase[i][j]);
		}
	}
	_IsSurfaceEffectsInitted = false;
}

CollisionReactionType PhysObserverClass::Collision_Occurred(const CollisionEventClass& collide)
{
	return COLLISION_REACTION_DEFAULT;
}

int PhysObserverClass::Object_Expired(PhysClass *obj)
{
	return 1;
}

void PhysObserverClass::Object_Removed_From_Scene(PhysClass *obj)
{
}

void PhysObserverClass::Object_Shattered_Something(PhysClass *obj1,PhysClass *obj2,int i)
{
}

DamageableGameObj* CombatPhysObserverClass::As_DamageableGameObj()
{
	return 0;
}

PhysicalGameObj* CombatPhysObserverClass::As_PhysicalGameObj()
{
	return 0;
}

BuildingGameObj* CombatPhysObserverClass::As_BuildingGameObj()
{
	return 0;
}

void SurfaceEffectsManager::Apply_Effect(int surfaceType, int hitterType, const Matrix3D& transform, PhysClass* phys, PhysicalGameObj* obj, bool applyDecal, bool applyEmitter)
{
	if ((uint)surfaceType < 32 && (uint)hitterType < 27 &&
		SurfaceEffectsManager::Mode != 0)
	{
		if (SurfaceEffectsManager::OverrideSurfaceType != -1)
			surfaceType = SurfaceEffectsManager::OverrideSurfaceType;
		
		SurfaceEffect* surfaceEffect = SurfaceEffectsDatabase[surfaceType][hitterType];
		if (surfaceEffect)
		{
			const StringClass* sound = surfaceEffect->sounds.Get_String();
			const StringClass* emitter = surfaceEffect->emitters.Get_String();
			const StringClass* decal = surfaceEffect->decals.Get_String();
			
			if (sound)
			{
				RefCountedReferencerClass* referencer = new RefCountedReferencerClass();
				referencer->Set_Ptr(obj);
				WWAudioClass::_theInstance->Create_Instant_Sound(sound->Peek_Buffer(), transform, referencer, 0, 4);
				referencer->Release_Ref();
			}
			
			Vector3 position = transform.getPosition();
			float sqrDistance = (position - COMBAT_CAMERA->Get_Position()).Length2();
			
			if (sqrDistance <= 100.f ||
				sqrDistance <= 2500.f && CollisionMath::Overlap_Test(COMBAT_CAMERA->Get_Frustum(), position) != 1)
			{
				if (applyEmitter && emitter && SurfaceEffectsManager::Mode != 1)
					_EmitterRecycler.Spawn_Effect(*emitter, transform, -1.f);
				
				if (applyDecal && decal)
				{
					float maxRand = fabs(surfaceEffect->decalSizeRandom);
					float decalSize = surfaceEffect->decalSize - maxRand + ((FreeRandom() & 0x7FFFFFFF) % 4097)/4096.f * 2 * maxRand;
					
					StringClass decalFilename(_MAX_FNAME, true);
					Strip_Path_From_Filename(decalFilename, *decal);

					bool hasPhys = surfaceType == 14 || surfaceType == 18;
					if (!hasPhys)
						phys = NULL;

					PhysicsSceneClass::Get_Instance()->Create_Decal(transform, decalFilename, decalSize, 0, hasPhys, phys);
				}
			}
		}
	}
}



RENEGADE_FUNCTION
PersistantSurfaceSoundClass *SurfaceEffectsManager::Create_Persistant_Sound()
AT2(0x006A99A0, 0x006A9240);



RENEGADE_FUNCTION
void SurfaceEffectsManager::Destroy_Persistant_Sound(PersistantSurfaceSoundClass*)
AT2(0x006A99D0, 0x006A9270);



RENEGADE_FUNCTION
void SurfaceEffectsManager::Update_Persistant_Sound(PersistantSurfaceSoundClass*, int, int, const Matrix3D&)
AT2(0x006A9A20, 0x006A92C0);



RENEGADE_FUNCTION
PersistantSurfaceEmitterClass *SurfaceEffectsManager::Create_Persistant_Emitter()
AT2(0x006A9B20, 0x006A93C0);



RENEGADE_FUNCTION
void SurfaceEffectsManager::Destroy_Persistant_Emitter(PersistantSurfaceEmitterClass*)
AT2(0x006A9B50, 0x006A93F0);



RENEGADE_FUNCTION
void SurfaceEffectsManager::Update_Persistant_Emitter(PersistantSurfaceEmitterClass*, int, int, const Matrix3D&)
AT2(0x006A9B70, 0x006A9410);



bool SurfaceEffectsManager::Does_Surface_Stop_Bullets(int surface)
{
	return SurfaceStopsBullets[surface];
}


RENEGADE_FUNCTION
void SurfaceEffectsManager::Apply_Damage(int, PhysicalGameObj*)
AT2(0x006A9BF0, 0x006A9490);



RENEGADE_FUNCTION
bool SurfaceEffectsManager::Is_Surface_Permeable(int)
AT2(0x006A9CB0, 0x006A9550);



RENEGADE_FUNCTION
void SurfaceEffectsManager::Set_Override_Surface_Type(int)
AT2(0x006A9CF0, 0x006A9590);
