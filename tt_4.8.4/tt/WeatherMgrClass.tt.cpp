#include "General.h"
#include "WeatherMgrClass.h"
#include "CombatManager.h"
#include "BitStream.h"
WeatherMgrClass *_TheWeatherMgr;
void WeatherMgrClass::Set_Delete_Pending()
{
}
uint32 WeatherMgrClass::Chunk_ID() const
{
	return 0x40800;
}
const char *WeatherMgrClass::Name() const
{
	return "WeatherMgrClass";
}
void WeatherMgrClass::Delete()
{
}
WeatherMgrClass::WeatherMgrClass()
{
	Set_Network_ID (0x7D2B7507);
	PacketType = 0x17;
}
bool WeatherMgrClass::Set_Wind(float heading, float speed, float variability, float ramptime)
{
	if (CombatManager::I_Am_Server())
	{
		if (Set_Wind (heading, speed, variability, ramptime, false))
		{
			_TheWeatherMgr->Set_Object_Dirty_Bit(DB_RARE, true);
			return true;
		}
	}
	return false;
}
bool WeatherMgrClass::Override_Wind(float heading, float speed, float variability, float ramptime)
{
	if (CombatManager::I_Am_Server())
	{
		_WindOverrideCount++;
		if (Set_Wind (heading, speed, variability, ramptime, true))
		{
			_TheWeatherMgr->Set_Object_Dirty_Bit (DB_RARE, true);
			return true;
		}
	}
	return false;
}
bool WeatherMgrClass::Set_Wind(float heading, float speed, float variability, float ramptime, bool override)
{
	if ((heading >= 0.0f) && (heading <= 360.0f) && (speed >= 0.0f) && (variability >= 0.0f) && (variability <= 1.0f) && (ramptime >= 0.0f))
	{
		bool o;
		o = (_WindOverrideCount > 0) && override;
		_Parameters[PARAMETER_WIND_HEADING].Set(heading, ramptime, o);
		_Parameters[PARAMETER_WIND_SPEED].Set(speed, ramptime, o);
		_Parameters[PARAMETER_WIND_VARIABILITY].Set(variability, ramptime, o);
		return true;
	}
	return false;
}
void WeatherMgrClass::Get_Wind(float &heading, float &speed, float &variability)
{
	heading = _Parameters[PARAMETER_WIND_HEADING].Value();
	speed = _Parameters[PARAMETER_WIND_SPEED].Value();
	variability = _Parameters[PARAMETER_WIND_VARIABILITY].Value();
}
void WeatherMgrClass::Restore_Wind(float ramptime)
{
	if (CombatManager::I_Am_Server())
	{
		_Parameters[PARAMETER_WIND_HEADING].Set(ramptime);
		_Parameters[PARAMETER_WIND_SPEED].Set(ramptime);
		_Parameters[PARAMETER_WIND_VARIABILITY].Set(ramptime);
		_WindOverrideCount--;
		_TheWeatherMgr->Set_Object_Dirty_Bit(DB_RARE, true);
	}
}
bool WeatherMgrClass::Set_Precipitation(PrecipitationEnum precipitation, float density, float ramptime)
{
	if (CombatManager::I_Am_Server())
	{
		if (Set_Precipitation(precipitation, density, ramptime, false))
		{
			_TheWeatherMgr->Set_Object_Dirty_Bit(DB_RARE, true);
			return true;
		}
	}
	return false;
}
bool WeatherMgrClass::Override_Precipitation(PrecipitationEnum precipitation, float density, float ramptime)
{
	if (CombatManager::I_Am_Server())
	{
		bool success;
		_PrecipitationOverrideCount++;
		success = true;
		for (int p = PRECIPITATION_FIRST; p < PRECIPITATION_COUNT; p++)
		{
			if (p != precipitation)
			{
				success &= Set_Precipitation((PrecipitationEnum) p, 0.0f, ramptime, true);
			}
			else
			{
				success &= Set_Precipitation((PrecipitationEnum) p, density, ramptime, true);
			}
		}
		_TheWeatherMgr->Set_Object_Dirty_Bit(DB_RARE, true);
		return success;
	}
	return false;
}
bool WeatherMgrClass::Set_Precipitation(PrecipitationEnum precipitation, float density, float ramptime, bool override)
{
	if ((density >= 0.0f) && (ramptime >= 0.0f))
	{
		const bool o = (_PrecipitationOverrideCount > 0) && override;
		switch (precipitation)
		{
			case PRECIPITATION_RAIN:
				_Parameters[PARAMETER_RAIN_DENSITY].Set(density, ramptime, o);
				break;
			case PRECIPITATION_SNOW:
				_Parameters[PARAMETER_SNOW_DENSITY].Set(density, ramptime, o);
				break;
			case PRECIPITATION_ASH:
				_Parameters[PARAMETER_ASH_DENSITY].Set(density, ramptime, o);
				break;
			default:
				break;
		}
		return true;
	}
	return false;
}
void WeatherMgrClass::Get_Precipitation(PrecipitationEnum precipitation, float &density)
{
	switch (precipitation)
	{
		case PRECIPITATION_RAIN:
			density = _Parameters[PARAMETER_RAIN_DENSITY].Value();
			break;
		case PRECIPITATION_SNOW:
			density = _Parameters[PARAMETER_SNOW_DENSITY].Value();
			break;
		case PRECIPITATION_ASH:
			density = _Parameters[PARAMETER_ASH_DENSITY].Value();
			break;
		default:
			break;
	}
}
void WeatherMgrClass::Restore_Precipitation(float ramptime)
{
	if (CombatManager::I_Am_Server())
	{
		_Parameters[PARAMETER_RAIN_DENSITY].Set(ramptime);
		_Parameters[PARAMETER_SNOW_DENSITY].Set(ramptime);
		_Parameters[PARAMETER_ASH_DENSITY].Set(ramptime);
		_PrecipitationOverrideCount--;
		_TheWeatherMgr->Set_Object_Dirty_Bit(DB_RARE, true);
	}
}
bool WeatherMgrClass::Set_Fog_Range(float startdistance, float enddistance, float ramptime)
{
	if ((startdistance >= 0.0f) && (enddistance >= startdistance))
	{
		_Parameters[PARAMETER_FOG_START_DISTANCE].Set(startdistance, ramptime, false);
		_Parameters[PARAMETER_FOG_END_DISTANCE].Set(enddistance, ramptime, false);
		return true;
	}
	return false;
}
void WeatherMgrClass::Get_Fog_Range(float &startdistance, float &enddistance)
{
	startdistance = _Parameters[PARAMETER_FOG_START_DISTANCE].Value();
	enddistance = _Parameters[PARAMETER_FOG_END_DISTANCE].Value();
}
void WeatherMgrClass::Set_Fog_Enable(bool enabled)
{
	_FogEnabled = enabled;
	Set_Dirty();
}
bool WeatherMgrClass::Get_Fog_Enable()
{
	return _FogEnabled;
}
void WeatherMgrClass::Init(SoundEnvironmentClass *soundenvironment)
{
	_TheWeatherMgr = new WeatherMgrClass();
	REF_PTR_SET(_SoundEnvironment, soundenvironment);
	_Wind = NULL;
	for (int p = PRECIPITATION_FIRST; p < PRECIPITATION_COUNT; p++)
	{
		_Precipitation[p] = NULL;
	}
	Reset();
}
void WeatherMgrClass::Reset()
{
	int p;
	for (p = 0; p < PARAMETER_COUNT; p++)
	{
		_Parameters[p].Initialize();
	}
	if (_Wind != NULL)
	{
		delete _Wind;
		_Wind = NULL;
	}
	Set_Wind(0.0f, 0.0f, 0.0f, 0.0f, false);
	for (p = PRECIPITATION_FIRST; p < PRECIPITATION_COUNT; p++)
	{
		if (_Precipitation[p] != NULL)
		{
			_Precipitation[p]->Remove();
  			REF_PTR_RELEASE(_Precipitation[p]);
			Set_Precipitation((PrecipitationEnum)p, 0.0f, 0.0f, false);
		}
	}
	Set_Fog_Enable(false);
	Set_Fog_Range(200.0f, 300.0f);
	_Prime = true;
	_Imported = false;
	_WindOverrideCount = 0;
	_PrecipitationOverrideCount = 0;
	Set_Dirty();
}
void WeatherMgrClass::Shutdown()
{
	Reset();
	REF_PTR_RELEASE(_SoundEnvironment);
	delete _TheWeatherMgr;
	_TheWeatherMgr = 0;
}
WeatherMgrClass::~WeatherMgrClass()
{
}
#define WRITE_PARAMETER(varname) \
WRITE_MICRO_CHUNK (csave, VARID_ ## varname ## _CURRENT_VALUE, _Parameters [PARAMETER_ ## varname ##].CurrentValue); \
WRITE_MICRO_CHUNK (csave, VARID_ ## varname ## _NORMAL_VALUE, _Parameters [PARAMETER_ ## varname ##].NormalValue);	\
WRITE_MICRO_CHUNK (csave, VARID_ ## varname ## _NORMAL_TARGET, _Parameters [PARAMETER_ ## varname ##].NormalTarget);	\
WRITE_MICRO_CHUNK (csave, VARID_ ## varname ## _NORMAL_DURATION, _Parameters [PARAMETER_ ## varname ##].NormalDuration);	\
WRITE_MICRO_CHUNK (csave, VARID_ ## varname ## _OVERRIDE_TARGET, _Parameters [PARAMETER_ ## varname ##].OverrideTarget);	\
WRITE_MICRO_CHUNK (csave, VARID_ ## varname ## _OVERRIDE_DURATION, _Parameters [PARAMETER_ ## varname ##].OverrideDuration)
bool WeatherMgrClass::Save(ChunkSaveClass &csave)
{
	csave.Begin_Chunk(CHUNKID_MICRO_CHUNKS);
	csave.End_Chunk();
	Save_Dynamic(csave);
	return true;
}
bool WeatherMgrClass::Save_Dynamic(ChunkSaveClass &csave)
{
	csave.Begin_Chunk(CHUNKID_DYNAMIC_MICRO_CHUNKS);
	WRITE_PARAMETER(WIND_HEADING);
	WRITE_PARAMETER(WIND_SPEED);
	WRITE_PARAMETER(WIND_VARIABILITY);
	WRITE_PARAMETER(RAIN_DENSITY);
	WRITE_PARAMETER(SNOW_DENSITY);
	WRITE_PARAMETER(ASH_DENSITY);
	WRITE_MICRO_CHUNK(csave, VARID_WIND_OVERRIDE_COUNT, _WindOverrideCount);
	WRITE_MICRO_CHUNK(csave, VARID_PRECIPITATION_OVERRIDE_COUNT, _PrecipitationOverrideCount);
	WRITE_MICRO_CHUNK(csave, VARID_FOG_ENABLED, _FogEnabled);
	WRITE_PARAMETER(FOG_START_DISTANCE);
	WRITE_PARAMETER(FOG_END_DISTANCE);
	csave.End_Chunk();
	return true;
}
bool WeatherMgrClass::Load(ChunkLoadClass &cload)
{
	bool retval = true;
	while (cload.Open_Chunk ())
	{
		switch (cload.Cur_Chunk_ID ())
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
READ_MICRO_CHUNK (cload, VARID_ ## varname ## _CURRENT_VALUE, _Parameters [PARAMETER_ ## varname ##].CurrentValue); \
READ_MICRO_CHUNK (cload, VARID_ ## varname ## _NORMAL_VALUE, _Parameters [PARAMETER_ ## varname ##].NormalValue);	\
READ_MICRO_CHUNK (cload, VARID_ ## varname ## _NORMAL_TARGET, _Parameters [PARAMETER_ ## varname ##].NormalTarget);	\
READ_MICRO_CHUNK (cload, VARID_ ## varname ## _NORMAL_DURATION, _Parameters [PARAMETER_ ## varname ##].NormalDuration);	\
READ_MICRO_CHUNK (cload, VARID_ ## varname ## _OVERRIDE_TARGET, _Parameters [PARAMETER_ ## varname ##].OverrideTarget);	\
READ_MICRO_CHUNK (cload, VARID_ ## varname ## _OVERRIDE_DURATION, _Parameters [PARAMETER_ ## varname ##].OverrideDuration)
bool WeatherMgrClass::Load_Micro_Chunks(ChunkLoadClass &cload)
{
	while (cload.Open_Micro_Chunk())
	{
		cload.Close_Micro_Chunk();
	}
	return true;
}
bool WeatherMgrClass::Load_Dynamic(ChunkLoadClass &cload)
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
bool WeatherMgrClass::Load_Dynamic_Micro_Chunks(ChunkLoadClass &cload)
{
	while (cload.Open_Micro_Chunk())
	{
		switch (cload.Cur_Micro_Chunk_ID())
		{
			READ_PARAMETER(WIND_HEADING);
			READ_PARAMETER(WIND_SPEED);
			READ_PARAMETER(WIND_VARIABILITY);
			READ_PARAMETER(RAIN_DENSITY);
			READ_PARAMETER(SNOW_DENSITY);
			READ_PARAMETER(ASH_DENSITY);
			READ_MICRO_CHUNK(cload, VARID_WIND_OVERRIDE_COUNT, _WindOverrideCount);
			READ_MICRO_CHUNK(cload, VARID_PRECIPITATION_OVERRIDE_COUNT, _PrecipitationOverrideCount);
			READ_MICRO_CHUNK(cload, VARID_FOG_ENABLED, _FogEnabled);
			READ_PARAMETER(FOG_START_DISTANCE);
			READ_PARAMETER(FOG_END_DISTANCE);
		}
		cload.Close_Micro_Chunk();
	}
	return true;
}
#define EXPORT_PARAMETER(object, varname) \
object.Add (_Parameters [PARAMETER_ ## varname ##].NormalTarget); \
object.Add (_Parameters [PARAMETER_ ## varname ##].NormalDuration); \
object.Add (_Parameters [PARAMETER_ ## varname ##].OverrideTarget); \
object.Add (_Parameters [PARAMETER_ ## varname ##].OverrideDuration)
void WeatherMgrClass::Export_Rare(BitStreamClass &packet)
{
	EXPORT_PARAMETER(packet, WIND_HEADING);
	EXPORT_PARAMETER(packet, WIND_SPEED);
	EXPORT_PARAMETER(packet, WIND_VARIABILITY);
	EXPORT_PARAMETER(packet, RAIN_DENSITY);
	EXPORT_PARAMETER(packet, SNOW_DENSITY);
	EXPORT_PARAMETER(packet, ASH_DENSITY);
	packet.Add(_WindOverrideCount);
	packet.Add(_PrecipitationOverrideCount);
}
#define IMPORT_PARAMETER(object, varname) \
object.Get(_Parameters [PARAMETER_ ## varname ##].NormalTarget); \
object.Get(_Parameters [PARAMETER_ ## varname ##].NormalDuration); \
object.Get(_Parameters [PARAMETER_ ## varname ##].OverrideTarget); \
object.Get(_Parameters [PARAMETER_ ## varname ##].OverrideDuration)
void WeatherMgrClass::Import_Rare (BitStreamClass &packet)
{
	IMPORT_PARAMETER(packet, WIND_HEADING);
	IMPORT_PARAMETER(packet, WIND_SPEED);
	IMPORT_PARAMETER(packet, WIND_VARIABILITY);
	IMPORT_PARAMETER(packet, RAIN_DENSITY);
	IMPORT_PARAMETER(packet, SNOW_DENSITY);
	IMPORT_PARAMETER(packet, ASH_DENSITY);
	packet.Get(_WindOverrideCount);
	packet.Get(_PrecipitationOverrideCount);
	_Imported = true;
}
#undef EXPORT_PARAMETER
#undef IMPORT_PARAMETER
#undef READ_PARAMETER
#undef WRITE_PARAMETER
