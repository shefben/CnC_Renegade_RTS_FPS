/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 ***                            Confidential - Westwood Studios                              ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/damageablegameobj.cpp                 $*
 *                                                                                             *
 *                      $Author:: Tom_s                                                       $*
 *                                                                                             *
 *                     $Modtime:: 12/18/01 3:02p                                              $*
 *                                                                                             *
 *                    $Revision:: 19                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "damageablegameobj.h"
#include "surfacetinteffect.h"
#include "pscene.h"
#include "physcoltest.h"
#include "lineseg.h"
#include "coltype.h"
#include "timemgr.h"
#include "WeatherMgr.h"
#include "physicalgameobj.h"
#include "phys.h"
#include "damagecontext.h"
#include "gameeventbus.h"
#include "debug.h"
#include "armedgameobj.h"
#include "playertype.h"
#include "colors.h"

/*
** DamageableGameObjDef - Defintion class for a DamageableGameObj
*/
DamageableGameObjDef::DamageableGameObjDef( void ) :
	TranslatedNameID( 0 ),
	EncyclopediaType( EncyclopediaMgrClass::TYPE_UNKNOWN ),
	EncyclopediaID( 0 ),
	NotTargetable( false ),
	DefaultPlayerType( PLAYERTYPE_NEUTRAL )
{
	DEFENSEOBJECTDEF_EDITABLE_PARAMS( DamageableGameObjDef, DefenseObjectDef );
	EDITABLE_PARAM( DamageableGameObjDef, ParameterClass::TYPE_STRINGSDB_ID,	TranslatedNameID );
	FILENAME_PARAM( DamageableGameObjDef, InfoIconTextureFilename, "InfoIconTextureFilename", ".TGA" );

#ifdef	PARAM_EDITING_ON
	EnumParameterClass *param = new EnumParameterClass( (int *)&EncyclopediaType );
	param->Set_Name( "Encyclopedia Type" );
	param->Add_Value( "<NA>",			0 );
	param->Add_Value( "Character",	EncyclopediaMgrClass::TYPE_CHARACTER );
	param->Add_Value( "Weapon",		EncyclopediaMgrClass::TYPE_WEAPON );
	param->Add_Value( "Vehicle",		EncyclopediaMgrClass::TYPE_VEHICLE );
	param->Add_Value( "Building",		EncyclopediaMgrClass::TYPE_BUILDING );
	GENERIC_EDITABLE_PARAM( DamageableGameObjDef, param )

	param = new EnumParameterClass( &DefaultPlayerType );
	param->Set_Name ("PlayerType");
	param->Add_Value ( "Mutant",		PLAYERTYPE_MUTANT );
	param->Add_Value ( "Unteamed",	PLAYERTYPE_NEUTRAL );
	param->Add_Value ( "Renegade",	PLAYERTYPE_RENEGADE );
	param->Add_Value ( "Nod",			PLAYERTYPE_NOD );
	param->Add_Value ( "GDI",			PLAYERTYPE_GDI );
	GENERIC_EDITABLE_PARAM(DamageableGameObjDef,param)
#endif

	EDITABLE_PARAM( DamageableGameObjDef, ParameterClass::TYPE_INT,	EncyclopediaID );
	EDITABLE_PARAM( DamageableGameObjDef, ParameterClass::TYPE_BOOL,	NotTargetable );
	return ;
}

enum	{
	CHUNKID_DEF_PARENT								=	207011205,
	CHUNKID_DEF_VARIABLES,
	CHUNKID_DEF_DEFENSEOBJECTDEF,

	MICROCHUNKID_DEF_TRANSLATED_NAME_ID			=	1,
	MICROCHUNKID_DEF_INFO_ICON_TEXTURE_FILENAME,
	MICROCHUNKID_DEF_ENCY_TYPE,
	MICROCHUNKID_DEF_ENCY_ID,
	MICROCHUNKID_DEF_NOT_TARGETABLE,
	MICROCHUNKID_DEF_DEFAULT_PLAYER_TYPE,
};

bool	DamageableGameObjDef::Save( ChunkSaveClass & csave )
{
	csave.Begin_Chunk( CHUNKID_DEF_PARENT );
		ScriptableGameObjDef::Save( csave );
	csave.End_Chunk();

	csave.Begin_Chunk( CHUNKID_DEF_VARIABLES );
		WRITE_MICRO_CHUNK( csave, MICROCHUNKID_DEF_TRANSLATED_NAME_ID, TranslatedNameID );
		WRITE_MICRO_CHUNK_WWSTRING( csave, MICROCHUNKID_DEF_INFO_ICON_TEXTURE_FILENAME, InfoIconTextureFilename );
		WRITE_MICRO_CHUNK( csave, MICROCHUNKID_DEF_ENCY_TYPE, EncyclopediaType );
		WRITE_MICRO_CHUNK( csave, MICROCHUNKID_DEF_ENCY_ID,	EncyclopediaID );
		WRITE_MICRO_CHUNK( csave, MICROCHUNKID_DEF_NOT_TARGETABLE,	NotTargetable );
		WRITE_MICRO_CHUNK( csave, MICROCHUNKID_DEF_DEFAULT_PLAYER_TYPE, DefaultPlayerType );
	csave.End_Chunk();

	csave.Begin_Chunk( CHUNKID_DEF_DEFENSEOBJECTDEF );
		DefenseObjectDef.Save(csave);
	csave.End_Chunk();

	return true;
}

bool	DamageableGameObjDef::Load( ChunkLoadClass &cload )
{
	while (cload.Open_Chunk()) {
		switch(cload.Cur_Chunk_ID()) {

			case CHUNKID_DEF_PARENT:
				ScriptableGameObjDef::Load( cload );
				break;

			case CHUNKID_DEF_VARIABLES:
				while (cload.Open_Micro_Chunk()) {
					switch(cload.Cur_Micro_Chunk_ID()) {
						READ_MICRO_CHUNK( cload, MICROCHUNKID_DEF_TRANSLATED_NAME_ID, TranslatedNameID );
						READ_MICRO_CHUNK_WWSTRING( cload, MICROCHUNKID_DEF_INFO_ICON_TEXTURE_FILENAME, InfoIconTextureFilename );
						READ_MICRO_CHUNK( cload, MICROCHUNKID_DEF_ENCY_TYPE,	EncyclopediaType );
						READ_MICRO_CHUNK( cload, MICROCHUNKID_DEF_ENCY_ID,		EncyclopediaID );
						READ_MICRO_CHUNK( cload, MICROCHUNKID_DEF_NOT_TARGETABLE,	NotTargetable );
						READ_MICRO_CHUNK( cload, MICROCHUNKID_DEF_DEFAULT_PLAYER_TYPE, DefaultPlayerType );

						default:
							Debug_Say(( "Unhandled MicroChunk:%d File:%s Line:%d\r\n",cload.Cur_Micro_Chunk_ID(),__FILE__,__LINE__));
							break;

					}
					cload.Close_Micro_Chunk();
				}
				break;

			case CHUNKID_DEF_DEFENSEOBJECTDEF:
				DefenseObjectDef.Load(cload);
				break;

			default:
				Debug_Say(( "Unhandled Chunk:%d File:%s Line:%d\r\n",cload.Cur_Chunk_ID(),__FILE__,__LINE__));
				break;

		}
		cload.Close_Chunk();
	}
	return true;
}

/*
** DamageableGameObj
*/
DamageableGameObj::DamageableGameObj( void ) :
	IsHealthBarDisplayed( true ),
	DamageTint( nullptr ),
	SkyView( -1.0f ),
	SkyViewAge( 0.0f )
{
	Set_Player_Type(PLAYERTYPE_NEUTRAL);
}

DamageableGameObj::~DamageableGameObj( void )
{
	Remove_All_Observers();
}



/*
**
*/
void	DamageableGameObj::Init( const DamageableGameObjDef & definition )
{
	ScriptableGameObj::Init( definition );
	Copy_Settings( definition );
	return ;
}

/*
**
*/
void	DamageableGameObj::Copy_Settings( const DamageableGameObjDef & definition )
{
	Set_Player_Type(definition.DefaultPlayerType);
	DefenseObject.Init(definition.DefenseObjectDef, this );
	return ;
}

/*
**
*/
void	DamageableGameObj::Re_Init( const DamageableGameObjDef & definition )
{
	int old_player_type	= PlayerType;

	//
	//	Record the health and shield percent so we can restore the
	// appropriate amount of health and shield after we've re-initialized
	//
	//float health_percent	= DefenseObject.Get_Health() / DefenseObject.Get_Health_Max();
	//float shield_percent	= DefenseObject.Get_Shield_Strength() / DefenseObject.Get_Shield_Strength_Max();

	//
	//	Re-initialize the base class
	//
	ScriptableGameObj::Re_Init( definition );

	//
	//	Copy any internal settings from the definition
	//
	Copy_Settings( definition );

	//
	//	Reset the health and shield to appropriate values
	//
	//DefenseObject.Set_Health (DefenseObject.Get_Health_Max () * health_percent);
	//DefenseObject.Set_Shield_Strength (DefenseObject.Get_Shield_Strength_Max () * shield_percent);
	Set_Player_Type( old_player_type );
	return ;
}

const DamageableGameObjDef & DamageableGameObj::Get_Definition( void ) const
{
	return (const DamageableGameObjDef &)BaseGameObj::Get_Definition();
}

/*
** DamageableGameObj Save and Load
*/
enum	{
	CHUNKID_PARENT							=	207011212,
	CHUNKID_DEFENSEOBJECT,
	CHUNKID_VARIABLES,

	MICROCHUNKID_PLAYER_TYPE				=	1,
	MICROCHUNKID_IS_HEALTH_BAR_DISPLAYED,
};

bool	DamageableGameObj::Save( ChunkSaveClass & csave )
{
	csave.Begin_Chunk( CHUNKID_PARENT );
		ScriptableGameObj::Save( csave );
	csave.End_Chunk();

	csave.Begin_Chunk( CHUNKID_VARIABLES );
		WRITE_MICRO_CHUNK( csave, MICROCHUNKID_PLAYER_TYPE, PlayerType );
		WRITE_MICRO_CHUNK( csave, MICROCHUNKID_IS_HEALTH_BAR_DISPLAYED, IsHealthBarDisplayed );
	csave.End_Chunk();

	csave.Begin_Chunk( CHUNKID_DEFENSEOBJECT );
		DefenseObject.Save(csave);
	csave.End_Chunk();

	return true;
}

bool	DamageableGameObj::Load( ChunkLoadClass &cload )
{
	while (cload.Open_Chunk()) {
		switch(cload.Cur_Chunk_ID()) {

			case CHUNKID_PARENT:
				ScriptableGameObj::Load( cload );
				break;

			case CHUNKID_VARIABLES:
				while (cload.Open_Micro_Chunk()) {
					switch(cload.Cur_Micro_Chunk_ID()) {
						READ_MICRO_CHUNK( cload, MICROCHUNKID_PLAYER_TYPE, PlayerType );
						READ_MICRO_CHUNK( cload, MICROCHUNKID_IS_HEALTH_BAR_DISPLAYED, IsHealthBarDisplayed );
						default:
							Debug_Say(( "Unhandled MicroChunk:%d File:%s Line:%d\r\n",cload.Cur_Micro_Chunk_ID(),__FILE__,__LINE__));
							break;

					}
					cload.Close_Micro_Chunk();
				}
				break;

			case CHUNKID_DEFENSEOBJECT:
				DefenseObject.Load( cload );
				break;

			default:
				Debug_Say(( "Unhandled Chunk:%d File:%s Line:%d\r\n",cload.Cur_Chunk_ID(),__FILE__,__LINE__));
				break;

		}
		cload.Close_Chunk();
	}

	return true;
}

/***********************************************************************************************
 * DamageableGameObj::Update_Damage_Tint -- roadmap Section 52A, Phase 61                      *
 *                                                                                             *
 *	The soot ramp follows the health fraction this object already replicates.  The effect is    *
 *	attached lazily, on the first damage that matters, so an untouched world carries no extra   *
 *	objects at all; and it is never attached while the feature is off, so turning it off after  *
 *	a fight leaves the effect in place but drawing nothing (Render_Push returns immediately at  *
 *	a zero global scale).                                                                       *
 *                                                                                             *
 *	An object with no physics object -- a script zone, anything not in the world -- has nothing *
 *	to tint and is skipped rather than special-cased later.                                     *
 *=============================================================================================*/
void DamageableGameObj::Update_Damage_Tint( void )
{
	//	Phase 59 shares this effect, so the attach fires for either reason: an object that has
	//	taken no damage at all still needs the effect once it starts raining on it.
	if ( ( SurfaceTintEffectClass::Get_Global_Scale() <= 0.0f ) &&
		  ( SurfaceTintEffectClass::Get_Wetness_Scale() <= 0.0f ) ) {
		return;
	}

	PhysicalGameObj * physical = As_PhysicalGameObj();
	if ( physical == nullptr ) {
		return;
	}

	PhysClass * physobj = physical->Peek_Physical_Object();
	if ( physobj == nullptr ) {
		return;
	}

	float maximum = DefenseObject.Get_Health_Max();
	if ( maximum <= 0.0f ) {
		return;
	}
	float fraction = DefenseObject.Get_Health() / maximum;

	if ( DamageTint == nullptr ) {
		DamageTint = new SurfaceTintEffectClass;
		physobj->Add_Effect_To_Me( DamageTint );
		//	Add_Effect_To_Me took its own reference; this one was the constructor's.
		DamageTint->Release_Ref();
	}

	DamageTint->Set_Health_Fraction( fraction );

	/*
	**	Roadmap Section 52A, Phase 59.  How wet this object is, which is not the world's
	**	wetness but the world's wetness times how much sky this object can see -- a truck
	**	parked under a bridge stays dry, and a soldier indoors stays dry, which is the
	**	acceptance line's "sheltered surfaces stay dry" and the reason the value is per object
	**	rather than global.
	**
	**	The sky view is cast rarely rather than every frame: an object does not move between
	**	shelter and open sky often, and a raycast per object per frame would be the single
	**	most expensive thing this phase does.
	*/
	if ( SurfaceTintEffectClass::Get_Wetness_Scale() > 0.0f ) {
		DamageTint->Set_Wetness( WeatherMgrClass::Get_Sheltered_Wetness( Compute_Sky_View() ) );
	}
}


/***********************************************************************************************
 * DamageableGameObj::Compute_Sky_View -- can the rain reach this object                       *
 *                                                                                             *
 *	One ray, straight up, against static geometry only -- the same cast Phase 52's terrain      *
 *	occlusion and Phase 56's camera sky view both use, for the same reason: a soldier walking   *
 *	over a puddle must not shelter it.                                                           *
 *                                                                                             *
 *	Cached, and refreshed on a timer rather than every frame.  Objects move between shelter and *
 *	open sky rarely, and paying a raycast per object per frame to notice it sooner would cost   *
 *	far more than the effect is worth.                                                           *
 *=============================================================================================*/
float DamageableGameObj::Compute_Sky_View( void )
{
	static const float SKY_TEST_DISTANCE	= 40.0f;
	static const float MOVE_AMOUNT			= 0.5f;
	static const float REFRESH_SECONDS		= 2.0f;

	SkyViewAge += TimeManager::Get_Frame_Seconds();
	if ( ( SkyViewAge < REFRESH_SECONDS ) && ( SkyView >= 0.0f ) ) {
		return SkyView;
	}
	SkyViewAge = 0.0f;

	PhysicsSceneClass * scene = PhysicsSceneClass::Get_Instance();
	PhysicalGameObj * physical = As_PhysicalGameObj();
	if ( ( scene == nullptr ) || ( physical == nullptr ) ) {
		SkyView = 1.0f;
		return SkyView;
	}

	Vector3 position;
	physical->Get_Position( &position );

	Vector3 p0 = position + Vector3( 0.0f, 0.0f, MOVE_AMOUNT );
	Vector3 p1 = position + Vector3( 0.0f, 0.0f, SKY_TEST_DISTANCE );

	CastResultStruct res;
	LineSegClass ray( p0, p1 );
	PhysRayCollisionTestClass raytest( ray, &res, 0, COLLISION_TYPE_PROJECTILE );
	raytest.CheckDynamicObjs = false;
	scene->Cast_Ray( raytest );

	SkyView = ( res.Fraction >= 1.0f ) ? 1.0f : 0.0f;
	return SkyView;
}


void	DamageableGameObj::Apply_Damage( const OffenseObjectClass & damager, float scale, int alternate_skin )
{
	if ( DefenseObject.Get_Health() <= 0 ) {
		return;
	}

	if (Is_Delete_Pending()) {
		return;
	}

	//
	//	Raised before armor and warhead scaling, so the amount reported is what
	//	the weapon asked for rather than what this object is about to lose.  A
	//	subscriber that refuses leaves the object untouched: no health change,
	//	no Damaged notification and no kill.
	//
	//	The ammo definition and impact bone are not carried on this path yet;
	//	they arrive with the rest of the 4.8.4 damage merge.
	if ( !GameEventBus::Raise_Damage( damager.Get_Owner(), As_PhysicalGameObj(),
			damager.Get_Damage() * scale, (unsigned int)damager.Get_Warhead(),
			nullptr, nullptr ) ) {
		return;
	}

	//
	//	Record what is hitting this object, so a Damaged or Killed handler can
	//	ask.  Cleared again when this returns; see damagecontext.h.
	//
	DamageContextClass::WarheadScopeClass warhead_scope( damager.Get_Warhead() );

	float old_health = DefenseObject.Get_Health();
	float old_shield = DefenseObject.Get_Shield_Strength();
	DefenseObject.Apply_Damage( damager, scale, alternate_skin );
	float new_health = DefenseObject.Get_Health();
	float new_shield = DefenseObject.Get_Shield_Strength();

	float diff = old_health + old_shield - new_health - new_shield;

	//	Roadmap Section 52A, Phase 61.  One site, because health changes here for buildings,
	//	vehicles and soldiers alike -- hooking each of them separately would be three places
	//	to forget.  Repair comes through the same path with a negative damage, so this is also
	//	where the tint reverses.
	Update_Damage_Tint();

	// Notify the observers
	const GameObjObserverList & observer_list = Get_Observers();

	for( int index = 0; index < observer_list.Count(); index++ ) {
		observer_list[ index ]->Damaged( this, damager.Get_Owner(), diff );
	}

	if ( DefenseObject.Get_Health() <= 0 ) {
		// notify the observers
		for( int index = 0; index < observer_list.Count(); index++ ) {
			observer_list[ index ]->Killed( this, damager.Get_Owner() );
		}

		GameEventBus::Raise_Kill( As_PhysicalGameObj(), damager.Get_Owner() );

		Completely_Damaged( damager );
	}
}

//------------------------------------------------------------------------------------
/*void	DamageableGameObj::Get_Information( StringClass & string )
{
	// If we just came from the editor, call created on all out observers
	const GameObjObserverList & observer_list = Get_Observers();
	for( int index = 0; index < observer_list.Count(); index++ ) {
		StringClass temp;
		temp.Format( "%s\n", observer_list[ index ]->Get_Name() );
		string += temp;
	}
} */


/*
**
*/
//
//	The maxima.  A script or a server may change either, and until they crossed
//	the wire every other machine drew the health bar against the value baked
//	into the definition.
//
void	DamageableGameObj::Export_Rare( BitStreamClass &packet )
{
	ScriptableGameObj::Export_Rare( packet );

	packet.Add( DefenseObject.Get_Health_Max() );
	packet.Add( DefenseObject.Get_Shield_Strength_Max() );
}

void	DamageableGameObj::Import_Rare( BitStreamClass &packet )
{
	ScriptableGameObj::Import_Rare( packet );

	float health_max = packet.Get( health_max );
	float shield_max = packet.Get( shield_max );

	DefenseObject.Set_Health_Max( health_max );
	DefenseObject.Set_Shield_Strength_Max( shield_max );
}

void	DamageableGameObj::Export_Occasional( BitStreamClass &packet )
{
	ScriptableGameObj::Export_Occasional( packet );

	//
	//	Export the defense object's state
	//
	DefenseObject.Export (packet);
}


/*
**
*/
void	DamageableGameObj::Import_Occasional( BitStreamClass &packet )
{
	ScriptableGameObj::Import_Occasional( packet );

	//
	//	Update the defense object's state
	//
	float old_health = DefenseObject.Get_Health();
	DefenseObject.Import (packet);
	float new_health = DefenseObject.Get_Health();

	/*
	//
	// Hack !
	//
	if ( Is_Delete_Pending() ) {
		if ( new_health != 0 ) {
			DefenseObject.Set_Health( 0 );
			new_health = DefenseObject.Get_Health();
		}
	} else {
		if ( new_health == 0 ) {
			DefenseObject.Set_Health( 0.01f );
			new_health = DefenseObject.Get_Health();
		}
	}
	*/

	if (old_health > 0 && old_health > new_health) {
		// Notify the observers that we are damaged
		const GameObjObserverList& observer_list = Get_Observers();
		int count = observer_list.Count();

		for (int index = 0; index < count; ++index) {
			observer_list[index]->Damaged(this, nullptr, old_health - new_health);
		}
	}

	//
	//	Check to see if the object is completely damaged
	//
	if ( old_health > 0 && new_health <= 0 ) {
		// Notify the observers that the building has been destroyed
		const GameObjObserverList& observer_list = Get_Observers();
		int count = observer_list.Count();

		for (int index = 0; index < count; ++index) {
			observer_list[index]->Killed(this, nullptr);
		}

		OffenseObjectClass dummy_offense_obj;
		Completely_Damaged( dummy_offense_obj );
	}
}


//-----------------------------------------------------------------------------
bool DamageableGameObj::Is_Team_Player(void)
{
	return PlayerType == PLAYERTYPE_NOD || PlayerType == PLAYERTYPE_GDI;
}

//-----------------------------------------------------------------------------
Vector3 DamageableGameObj::Get_Team_Color(void)
{
	return Get_Color_For_Team(PlayerType);
}

//-----------------------------------------------------------------------------
void DamageableGameObj::Set_Player_Type(int id)
{
	PlayerType = id;

	Set_Object_Dirty_Bit( NetworkObjectClass::BIT_RARE, true );
}

//-----------------------------------------------------------------------------
bool DamageableGameObj::Is_Teammate(DamageableGameObj * p_obj)
{
	WWASSERT(p_obj != nullptr);

   return ((p_obj == this) ||
			  (Is_Team_Player() && Get_Player_Type() == p_obj->Get_Player_Type()));
}

bool DamageableGameObj::Is_Enemy(DamageableGameObj * p_obj)
{
	WWASSERT(p_obj != nullptr);
   return ( (p_obj != this) && Player_Types_Are_Enemies( Get_Player_Type(), p_obj->Get_Player_Type() ) );
}


