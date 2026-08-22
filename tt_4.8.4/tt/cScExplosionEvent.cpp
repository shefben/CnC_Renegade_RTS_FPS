#include "General.h"
#include "cScExplosionEvent.h"
#include "bitstream.h"

void cScExplosionEvent::Export_Creation(BitStreamClass& oStream)
{
	oStream.Add(Explosion_Preset_ID);
	oStream.Add(position.X,BITPACK_WORLD_POSITION_X);
	oStream.Add(position.Y,BITPACK_WORLD_POSITION_Y);
	oStream.Add(position.Z,BITPACK_WORLD_POSITION_Z);
	oStream.Add(Damager_ID);
	Set_Delete_Pending();
}
void cScExplosionEvent::Import_Creation(BitStreamClass& oStream)
{
	oStream.Get(Explosion_Preset_ID);
	oStream.Get(position.X,BITPACK_WORLD_POSITION_X);
	oStream.Get(position.Y,BITPACK_WORLD_POSITION_Y);
	oStream.Get(position.Z,BITPACK_WORLD_POSITION_Z);
	oStream.Get(Damager_ID);
	Act();
}

