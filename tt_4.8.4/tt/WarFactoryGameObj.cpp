#include "general.h"

#include "WarFactoryGameObj.h"



RENEGADE_FUNCTION
void WarFactoryGameObj::Play_Creation_Animation
   (bool bPlay)
   AT2(0x0073F2B0,0x0073EB50);

const WarFactoryGameObjDef & WarFactoryGameObj::Get_Definition( void ) const
{
	return (const WarFactoryGameObjDef &)*definition;
}
