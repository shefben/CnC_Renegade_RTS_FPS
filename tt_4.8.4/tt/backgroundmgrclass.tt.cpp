#include "General.h"
#include "BackgroundMgrClass.h"
#include "CombatManager.h"
#include "BitStream.h"
#include "cNetwork.h"
#include "cRemoteHost.h"
BackgroundMgrClass *_TheBackgroundMgr;
BackgroundMgrClass::~BackgroundMgrClass()
{
}
BackgroundMgrClass::BackgroundMgrClass()
{
	Set_Network_ID(0x7D2B7508);
	PacketType = 0x21;
}
uint32 BackgroundMgrClass::Chunk_ID() const
{
	return 0x40126;
}
const char *BackgroundMgrClass::Name() const
{
	return "BackgroundMgrClass";
}
void BackgroundMgrClass::Delete(void)
{
}
void BackgroundMgrClass::Set_Delete_Pending(void)
{
}
void BackgroundMgrClass::Init(SimpleSceneClass *renderscene, SoundEnvironmentClass *soundenvironment, bool render_available)
{
	_TheBackgroundMgr = new BackgroundMgrClass();
	if (render_available)
	{
		_Sky = new SkyClass(soundenvironment);
		renderscene->Add_Render_Object(_Sky);
		_Dazzle = new DazzleRenderObjClass("SUN");
		renderscene->Add_Render_Object(_Dazzle);
	}
	Reset();
}
void BackgroundMgrClass::Reset()
{
	for (unsigned p = 0; p < PARAMETER_COUNT; p++)
	{
		_Parameters[p].Initialize();
	}
	Set_Time_Of_Day(15, 0);
	_LightSourceType = LIGHT_SOURCE_TYPE_SUN;
	Set_Moon_Type(SkyClass::MOON_TYPE_FULL);
	MoonIsEarth = false;
	Set_Clouds(0.0f, 0.0f, 0.0f, false);
	Set_Sky_Tint(0.0f, 0.0f, false);
	Set_Lightning(0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.0f, false);
	Set_War_Blitz(0.0f, 0.0f, 1.0f, 0.0f, 0.5f);
	_LightVector.Set(1.0f, 0.0f, 0.0f);
	_UnitLightVector.Set(1.0f, 0.0f, 0.0f);
	_CloudOverrideCount = 0;
	_LightningOverrideCount = 0;
	_SkyTintOverrideCount = 0;
	Set_Dirty();
}
void BackgroundMgrClass::Shutdown()
{
	if (_Dazzle)
	{
		_Dazzle->Remove();
		REF_PTR_RELEASE(_Dazzle);
	}
	if (_Sky)
	{
		_Sky->Remove();
		REF_PTR_RELEASE(_Sky);
	}
	delete _TheBackgroundMgr;
	_TheBackgroundMgr = 0;
}
void BackgroundMgrClass::Set_Moon_Is_Earth(bool earth)
{
	if (CombatManager::I_Am_Server())
	{
		_TheBackgroundMgr->Set_Object_Dirty_Bit(DB_RARE, true);
		MoonIsEarth = earth;
	}
}
bool BackgroundMgrClass::Set_Clouds(float cloudcover, float cloudgloominess, float ramptime)
{
	if (CombatManager::I_Am_Server())
	{
		if (Set_Clouds(cloudcover, cloudgloominess, ramptime, false))
		{
			_TheBackgroundMgr->Set_Object_Dirty_Bit(DB_RARE, true);
			return true;
		}
	}
	return false;
}
bool BackgroundMgrClass::Override_Clouds(float cloudcover, float cloudgloominess, float ramptime)
{
	if (CombatManager::I_Am_Server())
	{
		_CloudOverrideCount++;
		if (Set_Clouds(cloudcover, cloudgloominess, ramptime, true))
		{
			_TheBackgroundMgr->Set_Object_Dirty_Bit(DB_RARE, true);
			return true;
		}
	}
	return false;
}
bool BackgroundMgrClass::Set_Clouds(float cloudcover, float cloudgloominess, float ramptime, bool override)
{
	if ((cloudcover >= 0.0f) && (cloudcover <= 1.0f) && (cloudgloominess >= 0.0f) && (cloudgloominess <= 1.0f) && (ramptime >= 0.0f))
	{
		const bool o = override && (_CloudOverrideCount > 0);
		_Parameters[PARAMETER_CLOUD_COVER].Set(cloudcover, ramptime, o);
		_Parameters[PARAMETER_CLOUD_GLOOMINESS].Set(cloudgloominess, ramptime, o);
		return true;
	}
	return false;
}
void BackgroundMgrClass::Restore_Clouds(float ramptime)
{
	if (CombatManager::I_Am_Server())
	{
		_Parameters[PARAMETER_CLOUD_COVER].Set(ramptime);
		_Parameters[PARAMETER_CLOUD_GLOOMINESS].Set(ramptime);
		_CloudOverrideCount--;
		_TheBackgroundMgr->Set_Object_Dirty_Bit(DB_RARE, true);
	}
}
bool BackgroundMgrClass::Set_Sky_Tint(float skytintfactor, float ramptime)
{
	if (CombatManager::I_Am_Server())
	{
		if (Set_Sky_Tint(skytintfactor, ramptime, false))
		{
			_TheBackgroundMgr->Set_Object_Dirty_Bit(DB_RARE, true);
			return true;
		}
	}
	return false;
}
bool BackgroundMgrClass::Override_Sky_Tint(float skytintfactor, float ramptime)
{
	if (CombatManager::I_Am_Server())
	{
		_SkyTintOverrideCount++;
		if (Set_Sky_Tint(skytintfactor, ramptime, true))
		{
			_TheBackgroundMgr->Set_Object_Dirty_Bit(DB_RARE, true);
			return true;
		}
	}
	return false;
}
void BackgroundMgrClass::Restore_Sky_Tint(float ramptime)
{
	if (CombatManager::I_Am_Server())
	{
		_Parameters[PARAMETER_SKY_TINT_FACTOR].Set(ramptime);
		_SkyTintOverrideCount--;
		_TheBackgroundMgr->Set_Object_Dirty_Bit(DB_RARE, true);
	}
}
bool BackgroundMgrClass::Set_Sky_Tint(float skytintfactor, float ramptime, bool override)
{
	if ((skytintfactor >= 0.0f) && (skytintfactor <= 1.0f) && (ramptime >= 0.0f))
	{
		const bool o = override && (_SkyTintOverrideCount > 0);
		_Parameters[PARAMETER_SKY_TINT_FACTOR].Set(skytintfactor, ramptime, o);
		return true;
	}
	return false;
}
bool BackgroundMgrClass::Set_Lightning_Intensity(float intensity, float ramptime)
{
	if (CombatManager::I_Am_Server())
	{
		if (Set_Lightning_Intensity(intensity, ramptime, false))
		{
			_TheBackgroundMgr->Set_Object_Dirty_Bit(DB_RARE, true);
			return true;
		}
	}
	return false;
}
bool BackgroundMgrClass::Set_Lightning(float intensity, float startdistance, float enddistance, float heading, float distribution, float ramptime)
{
	if (CombatManager::I_Am_Server())
	{
		if (Set_Lightning(intensity, startdistance, enddistance, heading, distribution, ramptime, false))
		{
			_TheBackgroundMgr->Set_Object_Dirty_Bit(DB_RARE, true);
			return true;
		}
	}
	return false;
}
bool BackgroundMgrClass::Override_Lightning(float intensity, float startdistance, float enddistance, float heading, float distribution, float ramptime)
{
	if (CombatManager::I_Am_Server())
	{
		_LightningOverrideCount++;
		if (Set_Lightning(intensity, startdistance, enddistance, heading, distribution, ramptime, true))
		{
			_TheBackgroundMgr->Set_Object_Dirty_Bit(DB_RARE, true);
			return true;
		}
	}
	return false;
}
bool BackgroundMgrClass::Set_Lightning_Intensity(float intensity, float ramptime, bool override)
{
	if ((intensity >= 0.0f) && (intensity <= 1.0f) && (ramptime >= 0.0f))
	{
		const bool o = (_LightningOverrideCount > 0) && override;
		_Parameters[PARAMETER_LIGHTNING_INTENSITY].Set(intensity, ramptime, o);
		return true;
	}
	return false;
}
bool BackgroundMgrClass::Set_Lightning(float intensity, float startdistance, float enddistance, float heading, float distribution, float ramptime, bool override)
{
	if ((intensity >= 0.0f) && (intensity <= 1.0f) && (startdistance >= 0.0f) && (startdistance <= 1.0f) && (enddistance >= 0.0f) && (enddistance <= 1.0f) && (heading >= 0.0f) && (heading <= 360.0f) && (distribution	>= 0.0f) && (distribution <= 1.0f) && (ramptime >= 0.0f))
	{
		const bool o = (_LightningOverrideCount > 0) && override;
		_Parameters[PARAMETER_LIGHTNING_INTENSITY].Set(intensity, ramptime, o);
		_Parameters[PARAMETER_LIGHTNING_START_DISTANCE].Set(startdistance, ramptime, o);
		_Parameters[PARAMETER_LIGHTNING_END_DISTANCE].Set(enddistance, ramptime, o);
		_Parameters[PARAMETER_LIGHTNING_HEADING].Set(heading, ramptime, o);
		_Parameters[PARAMETER_LIGHTNING_DISTRIBUTION].Set(distribution, ramptime, o);
		return true;
	}
	return false;
}
void BackgroundMgrClass::Get_Lightning(float &intensity, float &startdistance, float &enddistance, float &heading, float &distribution)
{
	intensity = _Parameters[PARAMETER_LIGHTNING_INTENSITY].Value();
	startdistance = _Parameters[PARAMETER_LIGHTNING_START_DISTANCE].Value();
	enddistance = _Parameters[PARAMETER_LIGHTNING_END_DISTANCE].Value();
	heading	= _Parameters[PARAMETER_LIGHTNING_HEADING].Value();
	distribution = _Parameters[PARAMETER_LIGHTNING_DISTRIBUTION].Value();
}
void BackgroundMgrClass::Restore_Lightning(float ramptime)
{
	if (CombatManager::I_Am_Server())
	{
		_Parameters[PARAMETER_LIGHTNING_INTENSITY].Set(ramptime);
		_Parameters[PARAMETER_LIGHTNING_START_DISTANCE].Set(ramptime);
		_Parameters[PARAMETER_LIGHTNING_END_DISTANCE].Set(ramptime);
		_Parameters[PARAMETER_LIGHTNING_HEADING].Set(ramptime);
		_Parameters[PARAMETER_LIGHTNING_DISTRIBUTION].Set(ramptime);
		_LightningOverrideCount--;
		_TheBackgroundMgr->Set_Object_Dirty_Bit(DB_RARE, true);
	}
}
bool BackgroundMgrClass::Set_War_Blitz(float intensity, float ramptime)
{
	if ((intensity >= 0.0f) && (intensity <= 1.0f) && (ramptime >= 0.0f))
	{
		_Parameters[PARAMETER_WAR_BLITZ_INTENSITY].Set(intensity, ramptime, false);
		return true;
	}
	return false;
}
bool BackgroundMgrClass::Set_War_Blitz(float intensity, float startdistance, float enddistance, float heading, float distribution, float ramptime, bool override)
{
	if ((intensity >= 0.0f) && (intensity <= 1.0f) && (startdistance >= 0.0f) && (startdistance <= 1.0f) && (enddistance >= 0.0f) && (enddistance <= 1.0f) && (heading >= 0.0f) && (heading <= 360.0f) && (distribution	>= 0.0f) && (distribution <= 1.0f) && (ramptime >= 0.0f))
	{
		_Parameters[PARAMETER_WAR_BLITZ_INTENSITY].Set(intensity, ramptime, false);
		_Parameters[PARAMETER_WAR_BLITZ_START_DISTANCE].Set(startdistance, ramptime, false);
		_Parameters[PARAMETER_WAR_BLITZ_END_DISTANCE].Set(enddistance, ramptime, false);
		_Parameters[PARAMETER_WAR_BLITZ_HEADING].Set(heading, ramptime, false);
		_Parameters[PARAMETER_WAR_BLITZ_DISTRIBUTION].Set(distribution, ramptime, false);
		return true;
	}
	return false;
}
void BackgroundMgrClass::Get_War_Blitz(float &intensity, float &startdistance, float &enddistance, float &heading, float &distribution)
{
	intensity = _Parameters[PARAMETER_WAR_BLITZ_INTENSITY].Value();
	startdistance = _Parameters[PARAMETER_WAR_BLITZ_START_DISTANCE].Value();
	enddistance = _Parameters[PARAMETER_WAR_BLITZ_END_DISTANCE].Value();
	heading = _Parameters[PARAMETER_WAR_BLITZ_HEADING].Value();
	distribution = _Parameters[PARAMETER_WAR_BLITZ_DISTRIBUTION].Value();
}
#define WRITE_PARAMETER(varname) \
WRITE_MICRO_CHUNK(csave, VARID_ ## varname ## _CURRENT_VALUE, _Parameters[PARAMETER_ ## varname ##].CurrentValue); \
WRITE_MICRO_CHUNK(csave, VARID_ ## varname ## _NORMAL_VALUE, _Parameters[PARAMETER_ ## varname ##].NormalValue);	\
WRITE_MICRO_CHUNK(csave, VARID_ ## varname ## _NORMAL_TARGET, _Parameters[PARAMETER_ ## varname ##].NormalTarget);	\
WRITE_MICRO_CHUNK(csave, VARID_ ## varname ## _NORMAL_DURATION, _Parameters[PARAMETER_ ## varname ##].NormalDuration);	\
WRITE_MICRO_CHUNK(csave, VARID_ ## varname ## _OVERRIDE_TARGET, _Parameters[PARAMETER_ ## varname ##].OverrideTarget);	\
WRITE_MICRO_CHUNK(csave, VARID_ ## varname ## _OVERRIDE_DURATION, _Parameters[PARAMETER_ ## varname ##].OverrideDuration)
bool BackgroundMgrClass::Save(ChunkSaveClass &csave)
{
	csave.Begin_Chunk(CHUNKID_MICRO_CHUNKS);
	WRITE_MICRO_CHUNK(csave, VARID_TIME_HOURS, _Hours);
	WRITE_MICRO_CHUNK(csave, VARID_TIME_MINUTES, _Minutes);
	WRITE_MICRO_CHUNK(csave, VARID_LIGHT_SOURCE_TYPE, _LightSourceType);
	WRITE_MICRO_CHUNK(csave, VARID_MOON_TYPE, _MoonType);
	csave.End_Chunk();
	Save_Dynamic(csave);
	return true;
}
bool BackgroundMgrClass::Save_Dynamic(ChunkSaveClass &csave)
{
	csave.Begin_Chunk(CHUNKID_DYNAMIC_MICRO_CHUNKS);
	WRITE_PARAMETER(SKY_TINT_FACTOR);
	WRITE_PARAMETER(CLOUD_COVER);
	WRITE_PARAMETER(CLOUD_GLOOMINESS);
	WRITE_PARAMETER(LIGHTNING_INTENSITY);
	WRITE_PARAMETER(LIGHTNING_START_DISTANCE);
	WRITE_PARAMETER(LIGHTNING_END_DISTANCE);
	WRITE_PARAMETER(LIGHTNING_HEADING);
	WRITE_PARAMETER(LIGHTNING_DISTRIBUTION);
	WRITE_MICRO_CHUNK(csave, VARID_CLOUD_OVERRIDE_COUNT, _CloudOverrideCount);
	WRITE_MICRO_CHUNK(csave, VARID_LIGHTNING_OVERRIDE_COUNT, _LightningOverrideCount);
	WRITE_PARAMETER(WAR_BLITZ_INTENSITY);
	WRITE_PARAMETER(WAR_BLITZ_START_DISTANCE);
	WRITE_PARAMETER(WAR_BLITZ_END_DISTANCE);
	WRITE_PARAMETER(WAR_BLITZ_HEADING);
	WRITE_PARAMETER(WAR_BLITZ_DISTRIBUTION);
	WRITE_MICRO_CHUNK(csave, VARID_SKY_TINT_OVERRIDE_COUNT, _SkyTintOverrideCount);
	csave.End_Chunk();
	return true;
}
bool BackgroundMgrClass::Load(ChunkLoadClass &cload)
{
	bool retval = true;
	while (cload.Open_Chunk())
	{
		switch (cload.Cur_Chunk_ID())
		{
			case CHUNKID_MICRO_CHUNKS:
				retval &= Load_Micro_Chunks(cload);
				break;
			case CHUNKID_DYNAMIC_MICRO_CHUNKS:
				retval &= Load_Dynamic_Micro_Chunks(cload);
				break;
		}
		cload.Close_Chunk();
	}
	return retval;
}
#define READ_PARAMETER(varname) \
READ_MICRO_CHUNK(cload, VARID_ ## varname ## _CURRENT_VALUE, _Parameters[PARAMETER_ ## varname ##].CurrentValue); \
READ_MICRO_CHUNK(cload, VARID_ ## varname ## _NORMAL_VALUE, _Parameters[PARAMETER_ ## varname ##].NormalValue);	\
READ_MICRO_CHUNK(cload, VARID_ ## varname ## _NORMAL_TARGET, _Parameters[PARAMETER_ ## varname ##].NormalTarget);	\
READ_MICRO_CHUNK(cload, VARID_ ## varname ## _NORMAL_DURATION, _Parameters[PARAMETER_ ## varname ##].NormalDuration);	\
READ_MICRO_CHUNK(cload, VARID_ ## varname ## _OVERRIDE_TARGET, _Parameters[PARAMETER_ ## varname ##].OverrideTarget);	\
READ_MICRO_CHUNK(cload, VARID_ ## varname ## _OVERRIDE_DURATION, _Parameters[PARAMETER_ ## varname ##].OverrideDuration)
bool BackgroundMgrClass::Load_Micro_Chunks(ChunkLoadClass &cload)
{
	while (cload.Open_Micro_Chunk())
	{
		switch (cload.Cur_Micro_Chunk_ID())
		{
			READ_MICRO_CHUNK(cload, VARID_TIME_HOURS, _Hours);
			READ_MICRO_CHUNK(cload, VARID_TIME_MINUTES, _Minutes);
			READ_MICRO_CHUNK(cload, VARID_LIGHT_SOURCE_TYPE, _LightSourceType);
			READ_MICRO_CHUNK(cload, VARID_MOON_TYPE, _MoonType);
		}
		cload.Close_Micro_Chunk();
	}
	return true;
}
bool BackgroundMgrClass::Load_Dynamic(ChunkLoadClass &cload)
{
	bool retval = true;
	while (cload.Open_Chunk())
	{
		switch (cload.Cur_Chunk_ID())
		{
			case CHUNKID_DYNAMIC_MICRO_CHUNKS:
				retval &= Load_Dynamic_Micro_Chunks(cload);
				break;
		}
		cload.Close_Chunk();
	}
	return retval;
}
bool BackgroundMgrClass::Load_Dynamic_Micro_Chunks(ChunkLoadClass &cload)
{
	while (cload.Open_Micro_Chunk())
	{
		switch (cload.Cur_Micro_Chunk_ID())
		{
			READ_PARAMETER(SKY_TINT_FACTOR);
			READ_PARAMETER(CLOUD_COVER);
			READ_PARAMETER(CLOUD_GLOOMINESS);
			READ_PARAMETER(LIGHTNING_INTENSITY);
			READ_PARAMETER(LIGHTNING_START_DISTANCE);
			READ_PARAMETER(LIGHTNING_END_DISTANCE);
			READ_PARAMETER(LIGHTNING_HEADING);
			READ_PARAMETER(LIGHTNING_DISTRIBUTION);
			READ_MICRO_CHUNK(cload, VARID_CLOUD_OVERRIDE_COUNT, _CloudOverrideCount);
			READ_MICRO_CHUNK(cload, VARID_LIGHTNING_OVERRIDE_COUNT, _LightningOverrideCount);
			READ_PARAMETER(WAR_BLITZ_INTENSITY);
			READ_PARAMETER(WAR_BLITZ_START_DISTANCE);
			READ_PARAMETER(WAR_BLITZ_END_DISTANCE);
			READ_PARAMETER(WAR_BLITZ_HEADING);
			READ_PARAMETER(WAR_BLITZ_DISTRIBUTION);
			READ_MICRO_CHUNK(cload, VARID_SKY_TINT_OVERRIDE_COUNT, _SkyTintOverrideCount);
		}
		cload.Close_Micro_Chunk();
	}
	return true;
}
#define EXPORT_PARAMETER(object, varname) \
object.Add(_Parameters[PARAMETER_ ## varname ##].NormalTarget); \
object.Add(_Parameters[PARAMETER_ ## varname ##].NormalDuration); \
object.Add(_Parameters[PARAMETER_ ## varname ##].OverrideTarget); \
object.Add(_Parameters[PARAMETER_ ## varname ##].OverrideDuration)
void BackgroundMgrClass::Export_Rare(BitStreamClass &packet)
{
	EXPORT_PARAMETER(packet, SKY_TINT_FACTOR);
	EXPORT_PARAMETER(packet, CLOUD_COVER);
	EXPORT_PARAMETER(packet, CLOUD_GLOOMINESS);
	EXPORT_PARAMETER(packet, LIGHTNING_INTENSITY);
	EXPORT_PARAMETER(packet, LIGHTNING_START_DISTANCE);
	EXPORT_PARAMETER(packet, LIGHTNING_END_DISTANCE);
	EXPORT_PARAMETER(packet, LIGHTNING_HEADING);
	EXPORT_PARAMETER(packet, LIGHTNING_DISTRIBUTION);
	packet.Add(_CloudOverrideCount);
	packet.Add(_LightningOverrideCount);
	packet.Add(_SkyTintOverrideCount);
	if (cNetwork::Get_Server_Rhost(cNetwork::lastUpdatedClientId)->getVersion() >= 4.0f)
	{
		packet.Add(MoonIsEarth);
	}
}
#define IMPORT_PARAMETER(object, varname) \
object.Get(_Parameters[PARAMETER_ ## varname ##].NormalTarget); \
object.Get(_Parameters[PARAMETER_ ## varname ##].NormalDuration); \
object.Get(_Parameters[PARAMETER_ ## varname ##].OverrideTarget); \
object.Get(_Parameters[PARAMETER_ ## varname ##].OverrideDuration)
void BackgroundMgrClass::Import_Rare(BitStreamClass &packet)
{
	IMPORT_PARAMETER(packet, SKY_TINT_FACTOR);
	IMPORT_PARAMETER(packet, CLOUD_COVER);
	IMPORT_PARAMETER(packet, CLOUD_GLOOMINESS);
	IMPORT_PARAMETER(packet, LIGHTNING_INTENSITY);
	IMPORT_PARAMETER(packet, LIGHTNING_START_DISTANCE);
	IMPORT_PARAMETER(packet, LIGHTNING_END_DISTANCE);
	IMPORT_PARAMETER(packet, LIGHTNING_HEADING);
	IMPORT_PARAMETER(packet, LIGHTNING_DISTRIBUTION);
	packet.Get(_CloudOverrideCount);
	packet.Get(_LightningOverrideCount);
	packet.Get(_SkyTintOverrideCount);
	if (cNetwork::Get_Client_Rhost()->getVersion() >= 4.0f)
	{
		bool b;
		packet.Get(b);
		if (MoonIsEarth != b)
		{
			Set_Dirty();
			MoonIsEarth = b;
		}
	}
}
#undef EXPORT_PARAMETER
#undef IMPORT_PARAMETER
#undef READ_PARAMETER
#undef WRITE_PARAMETER
