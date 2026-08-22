#include "General.h"
#include "CCameraClass.h"

#include "ww3dassetmanager.h"
#include "engine_io.h"
#include "HashTemplateIterator.h"
#include "DX8Wrapper.h"
#include "WWAudioClass.h"
#include "Listener3DClass.h"
#include "FileHash.h"
#include "CombatManager.h"
#include "SoldierGameObj.h"
#include "physcoltest.h"
#include "linesegclass.h"
#include "PhysicsSceneClass.h"
#include "PersistFactoryClass.h"
#include "BuildingAggregateClass.h"
#include "HUDInfo.h"
#include "TimeManager.h"
#include "cGameType.h"
#include "WeaponClass.h"
#include "GameObjManager.h"
#pragma warning(disable: 6053) //Call to 'strncpy' might not zero-terminate string 'tmp'


REF_DEF1(ProfileHash, TT_NOOP(HashTemplateClass<StringClass, CCameraProfileClass*>), 0x0085CB20);
class CCameraProfileClass 
{
public:
	static void Init();
	static void	Shutdown();
protected:
	void	Lerp( const CCameraProfileClass & a, const CCameraProfileClass & b, float lerp );
	CCameraProfileClass();
	~CCameraProfileClass();
	const CCameraProfileClass &operator =(const CCameraProfileClass &);
	static CCameraProfileClass *Find(const char *name);
	float FOV;
	float Height;
	float ViewTilt;
	float TiltTweak;
	float TranslationTilt;
	float Distance;
	Vector3 Lag;
	static bool _ProfilesInitted;
	friend CCameraClass;
};
REF_DEF2(DrawDistance, float, 0x007FAE3C, 0x007F9FCC);
CCameraClass::CCameraClass() : 
	CameraClass(),
	HostModel( NULL ),
	AnchorPosition(0,0,0),
	IsValid( false ),
	Tilt( 0 ),
	Heading( 0 ),
	DistanceFraction(1.0f),
	Enable2DTargeting( false ),
	EnableWeaponHelp( false ),
	CameraTarget2DOffset( 0.5f, 0.5f ),
	LerpTimeTotal( 0.0f ),
	LerpTimeRemaining( 0.0f ),
	LastAnchorPosition(0,0,0),
	LastHeading( 0 ),
	CurrentProfile( NULL ),
	LastProfile( NULL ),
	DefaultProfile( NULL ),
	NearClipPlane( 0.26f ),
	IsStarSniping( false ),
	WasStarSniping( false ),
	CinematicSnipingEnabled( false ),
	CinematicSnipingDesiredZoom( 0 ),
	SniperZoom( 0 ),
	SniperDistance( 0 ),
	SniperListener( NULL ),
	SnapShotMode( SNAPSHOT_OFF ),
	WeaponHelpTimer( 0 ),
	WeaponHelpTargetID( 0 ),
	LagPersistTimer( 0 ),
	DisableLag( false )
{
	FarClipPlane = DrawDistance;
	Set_Clip_Planes( NearClipPlane, FarClipPlane );
	Set_View_Plane( DEG2RAD( 90.0f ) );

	SniperListener = new Listener3DClass;

	DefaultProfile = CCameraProfileClass::Find( "default" );
	DefaultProfileName="default";
	Use_Default_Profile(); 
	Set_Aspect_Ratio((float)DX8Wrapper::ResolutionWidth / (float)DX8Wrapper::ResolutionHeight);
}

void	CCameraClass::Use_Default_Profile()
{
	CurrentProfile = DefaultProfile;
	CurrentProfileName = DefaultProfileName;
}

bool CCameraProfileClass::_ProfilesInitted = false;
CCameraProfileClass::CCameraProfileClass( void ) : FOV( DEG2RAD( 65.0f ) ),Height( 1.95f ),ViewTilt( DEG2RAD(20.0f) ),TiltTweak( 0.6f ),TranslationTilt( DEG2RAD( 19.9f ) ),Distance( 3.1f ),Lag( 0,0,0 )
{
}
CCameraProfileClass::~CCameraProfileClass()
{
}
#define	Get_Camera_Profile_Radians( v, e )			\
	profile->v = (float)DEG2RAD(camerasINI->Get_Float(	section_name, e, (float)RAD2DEG( profile->v ) ) )
#define	Get_Camera_Profile_Float( v, e )			\
	profile->v = camerasINI->Get_Float(	section_name, e, profile->v )
void CCameraClass::Init()
{
	CCameraProfileClass::Init();
}

void CCameraClass::Shutdown()
{
	CCameraProfileClass::Shutdown();
}

void CCameraProfileClass::Init()
{
	if (_ProfilesInitted)
	{
		CCameraProfileClass::Shutdown();
	}
	FileClass *f = Get_Data_File("CAMERAS.INI");
	if ((f) && (f->Is_Available()))
	{
		unsigned int size = f->Size();
		char *data = new char[size];
		f->Open(1);
		f->Read(data,size);
		CheckLoad(data,size,"CAMERAS.INI",HashCamerasIni);
		delete[] data;
		f->Close();
		Close_Data_File(f);
	}
	INIClass *camerasINI = Get_INI("CAMERAS.INI");
	if (camerasINI != NULL)
	{
		int count,entry;
		count = camerasINI->Entry_Count("Profile_List");
		for ( entry = 0; entry < count; entry++ )
		{
			StringClass	section_name;
			camerasINI->Get_String(section_name,"Profile_List",camerasINI->Get_Entry("Profile_List",entry));
			CCameraProfileClass *profile = new CCameraProfileClass();
			StringClass name(true);
			camerasINI->Get_String(name,section_name,"Name");
			Get_Camera_Profile_Radians(	FOV,					"FOV"					);
			Get_Camera_Profile_Float(		Height,				"Height"				);
			Get_Camera_Profile_Radians(	ViewTilt,			"ViewTilt"				);
			Get_Camera_Profile_Float(		TiltTweak,			"TiltTweak"			);
			Get_Camera_Profile_Radians(	TranslationTilt,	"TranslationTilt"	);
			Get_Camera_Profile_Float(		Distance,			"Distance"				);

			Get_Camera_Profile_Float(		Lag.Y,				"LagUp"				);
			Get_Camera_Profile_Float(		Lag.X,				"LagLeft"				);
			Get_Camera_Profile_Float(		Lag.Z,				"LagForward"			);

			_strlwr(name.Peek_Buffer());
			ProfileHash.Insert(name,profile);
		}
		Release_INI(camerasINI);
	}	
	_ProfilesInitted = true;
}

void CCameraProfileClass::Shutdown()
{
	for (HashTemplateIterator<StringClass, CCameraProfileClass*> iter(ProfileHash); iter; ++iter)
		delete iter.getValue();
	
	ProfileHash.Remove_All();
	_ProfilesInitted = false;
}

const CCameraProfileClass & CCameraProfileClass::operator = ( const CCameraProfileClass & src )
{
	FOV = src.FOV;
	Height = src.Height;
	ViewTilt = src.ViewTilt;
	TiltTweak = src.TiltTweak;
	TranslationTilt	= src.TranslationTilt;
	Distance = src.Distance;
	Lag = src.Lag;
	return *this;
}

RENEGADE_FUNCTION
bool	CCameraClass::Save( ChunkSaveClass & csave )
AT1(0x006DC030);

RENEGADE_FUNCTION
bool	CCameraClass::Load( ChunkLoadClass &cload )
AT1(0x006DC430);

CCameraClass::~CCameraClass(void)
{
	Set_Host_Model(NULL);

	if (SniperListener &&
		WWAudioClass::Get_Instance() &&
		WWAudioClass::Get_Instance()->Get_Sound_Scene() &&
		WWAudioClass::Get_Instance()->Get_Sound_Scene()->Peek_2nd_Listener())
	{
		WWAudioClass::Get_Instance()->Get_Sound_Scene()->Set_2nd_Listener( NULL );
	}
	REF_PTR_RELEASE(SniperListener);
}

RENEGADE_FUNCTION
void	CCameraClass::Set_Host_Model( RenderObjClass * host )
AT1(0x006DCA30);

CCameraProfileClass	*	CCameraProfileClass::Find( const char * name )
{
	char tmp[256];
	memset(tmp,0,sizeof(tmp));
	strncpy(tmp,name,sizeof(tmp));
	_strlwr(tmp);
	StringClass tmp_string(tmp,true);
	CCameraProfileClass* profile = *ProfileHash.Get(tmp_string);
	return profile;
}

void Ignore_Star_And_Vehicle( void ) 
{
	if (COMBAT_STAR != NULL)
	{
		COMBAT_STAR->Peek_Physical_Object()->Inc_Ignore_Counter();
		VehicleGameObj * vehicle = COMBAT_STAR->Get_Profile_Vehicle();
		if ( vehicle )
		{
			vehicle->Peek_Physical_Object()->Inc_Ignore_Counter();
		}
	}
}

void Unignore_Star_And_Vehicle( void ) 
{
	if (COMBAT_STAR != NULL)
	{
		COMBAT_STAR->Peek_Physical_Object()->Dec_Ignore_Counter();
		VehicleGameObj * vehicle = COMBAT_STAR->Get_Profile_Vehicle();
		if ( vehicle )
		{
			vehicle->Peek_Physical_Object()->Dec_Ignore_Counter();
		}
	}
}

bool	CCameraClass::Determine_Targeting_Position( void )
{
	bool	looking_at_object = false;
	Matrix3D tm = Get_Transform();
	SoldierGameObj * star = COMBAT_STAR;
	bool is_star_determining_target = CombatManager::Is_Star_Determining_Target();
	if ( star && is_star_determining_target )
	{
		Vector3 cast_start = tm.Get_Translation();
		float range = 100;
		if ( star && star->Get_Weapon() )
		{
			range = star->Get_Weapon()->Get_Range();
		}
		if ( star && star->Get_Vehicle() && star->Get_Vehicle()->Get_Weapon() )
		{
			range = star->Get_Vehicle()->Get_Weapon()->Get_Range();
		}
		range += CurrentProfile->Distance;
		Vector2 view_plane;
		view_plane.X = CameraTarget2DOffset.X;
		view_plane.Y = CameraTarget2DOffset.Y;
		Vector3 world_pos;
		COMBAT_CAMERA->Un_Project( world_pos, view_plane );
		Vector3 cast_end = world_pos - cast_start;
		cast_end.Normalize();
		cast_end *= range;
		cast_end += cast_start;
		LineSegClass ray;
		if ( !Enable2DTargeting )
		{
			float move_forward_dist = DistanceFraction * CurrentProfile->Distance + 0.5f;
			cast_start += move_forward_dist * -(tm.Get_Z_Vector());
			ray.Set( cast_start, cast_end );			
		}
		else
		{
			ray.Set( cast_start, cast_end );
		}
		float start_fraction = NearClipPlane / ray.Get_Length();
		ray.Compute_Point( start_fraction, &cast_start );
		ray.Set( cast_start, cast_end );
		CastResultStruct result;
		PhysRayCollisionTestClass raytest(ray, &result, BULLET_COLLISION_GROUP, COLLISION_TYPE_PROJECTILE);
		Ignore_Star_And_Vehicle();
		COMBAT_SCENE->Cast_Ray( raytest );
		Unignore_Star_And_Vehicle();
		ray.Compute_Point( raytest.Result->Fraction, &StarTargetingPosition );
		if (COMBAT_STAR != NULL)
		{
			Vector3 player_pos(0,0,0);
			if (COMBAT_STAR->Get_Profile_Vehicle() != NULL)
			{
				COMBAT_STAR->Get_Profile_Vehicle()->Get_Position(&player_pos);
			}
			else
			{
				COMBAT_STAR->Get_Position(&player_pos);
			}
			float player_dx = player_pos.X - cast_start.X;
			float player_dy = player_pos.Y - cast_start.Y;
			float target_dx = StarTargetingPosition.X - cast_start.X;
			float target_dy = StarTargetingPosition.Y - cast_start.Y;
			float xy_dist_to_player2 = player_dx * player_dx + player_dy * player_dy;
			float xy_dist_to_target2 = target_dx * target_dx + target_dy * target_dy;
			if (xy_dist_to_target2 < xy_dist_to_player2)
			{
				float ray_parameter = 1.1f * WWMath::Sqrt(xy_dist_to_player2) / WWMath::Sqrt(xy_dist_to_target2);
				StarTargetingPosition = ray.Get_P0() + ray.Get_Dir() * ray_parameter;
			}
		}
		if ( raytest.CollidedPhysObj != NULL && raytest.CollidedPhysObj->Get_Observer() != NULL )
		{
			DamageableGameObj * obj = ((CombatPhysObserverClass *)raytest.CollidedPhysObj->Get_Observer())->As_DamageableGameObj();
			bool MCT = false;
			if ( obj->As_BuildingGameObj() )
			{
				if (raytest.CollidedPhysObj->Get_Factory().Chunk_ID() == 0x20A03)
				{
					if (((BuildingAggregateClass *)raytest.CollidedPhysObj)->Is_MCT())
					{
						MCT = true;
					}
				}
			}
			if ( obj->Is_Targetable() == false )
			{
				obj = NULL;
			}
			if ( obj && obj->As_SmartGameObj() && obj->As_SmartGameObj()->Is_Stealthed() && COMBAT_STAR && obj->Is_Enemy( COMBAT_STAR ) )
			{
				obj = NULL;
			}
			looking_at_object = ( obj != NULL );
			HUDInfo::Set_Info_Object( obj, MCT );
			HUDInfo::Set_Weapon_Target_Object( obj );
		}
		if ( raytest.CollidedPhysObj )
		{
			Set_Sniper_Distance( raytest.Result->Fraction * ray.Get_Length() );
		}
		else
		{
			Set_Sniper_Distance( 0 );
		}
	}
	return looking_at_object;
}
void CCameraClass::Set_Sniper_Distance( float dist )
{
	if ( SniperDistance != dist )
	{
		SniperDistance = dist;
		Update_Sniper_Listener_Pos();
	}	
}
void CCameraClass::Update_Sniper_Listener_Pos( void )
{
	if ( SniperListener != NULL )
	{
		Matrix3D tm = Get_Transform();
		float dist = ::tan (1.45F + ((1.5672F - 1.45F) * SniperZoom));	
		dist = min (dist, SniperDistance);
		Vector3 pos = tm.Get_Translation() - (tm.Get_Z_Vector() * dist);
		SniperListener->Set_Transform( Matrix3D( pos ) );
	}
}

void	CCameraClass::Apply_Weapon_Help( void )
{
	WeaponHelpTimer -= TimeManager::FrameSeconds;
	if ( WeaponHelpTimer <= 0 )
	{
		WeaponHelpTimer = 0.5f;
		WeaponHelpTargetID = 0;
		SmartGameObj * star = COMBAT_STAR;
		if ( star && CombatManager::Get_Difficulty_Level() == 0 && IS_MISSION )
		{
			if ( CombatManager::Is_Hit_Reticle_Enabled() )
			{
				CombatManager::Toggle_Hit_Reticle_Enabled();
			}
			float weapon_range = 100;
			if ( star->Get_Weapon() )
			{
				weapon_range = star->Get_Weapon()->Get_Range();
			}		
			Vector3	star_pos;
			star->Get_Position( &star_pos );
			SmartGameObj * best_obj = NULL;
			float best_distance = 0.1f;
			SLNode<GameObject> * smart_objnode;
			for (smart_objnode = GameObjManager::SmartGameObjList.Head(); smart_objnode; smart_objnode = smart_objnode->Next())
			{
				SmartGameObj * obj = (SmartGameObj *)smart_objnode->Data();
				if ( obj == COMBAT_STAR )
				{
					continue;
				}
				Vector3 pos = obj->Get_Bullseye_Position();
				float range = (pos - star_pos).Length();
				if ( range > weapon_range )
				{
					continue;
				}
				Vector3 twoDpos;
				if ( Project( twoDpos, pos ) == CameraClass::INSIDE_FRUSTUM )
				{
					twoDpos.X -= CameraTarget2DOffset.X;
					twoDpos.Y -= CameraTarget2DOffset.Y;
					twoDpos.Z = 0;
					float distance = twoDpos.Length();
					if ( distance < best_distance )
					{
						Vector3 cast_start = Get_Transform().Get_Translation();
						Vector3 cast_end = pos;
						LineSegClass ray;
						ray.Set( cast_start, cast_end );			
						CastResultStruct result;
						PhysRayCollisionTestClass raytest(ray, &result, BULLET_COLLISION_GROUP, COLLISION_TYPE_PROJECTILE);
						Ignore_Star_And_Vehicle();
						COMBAT_SCENE->Cast_Ray( raytest );
						Unignore_Star_And_Vehicle();
						if ( raytest.CollidedPhysObj != NULL && raytest.CollidedPhysObj->Get_Observer() != NULL && ((CombatPhysObserverClass *)raytest.CollidedPhysObj->Get_Observer())->As_PhysicalGameObj() )
						{
							best_distance = distance;
							best_obj = obj;
						}
						else
						{
							if ( obj == HUDInfo::Get_Info_Object() )
							{
								HUDInfo::Set_Info_Object( NULL ,false); 
							}
						}
					}
				}
  			}
			if ( best_obj != NULL )
			{
				WeaponHelpTargetID = best_obj->Get_Network_ID();
				StarTargetingPosition = best_obj->Get_Bullseye_Position();
				HUDInfo::Set_Info_Object( best_obj, false);
			}
		}
	}
	else
	{
		if ( WeaponHelpTargetID != 0 )
		{
			SmartGameObj * target = GameObjManager::Find_SmartGameObj( WeaponHelpTargetID );
			if ( target != NULL )
			{
				StarTargetingPosition = target->Get_Bullseye_Position();
			}
			else
			{
				WeaponHelpTargetID = 0;
			}
		}
	}
}

float	RadianLerp( float a, float b, float lerp ) 
{
	float diff = b - a;
	diff = WWMath::Wrap( diff, DEG_TO_RADF( -180 ), DEG_TO_RADF( 180 ) );
	return a + diff * lerp;
}

void	CCameraProfileClass::Lerp( const CCameraProfileClass & a, const CCameraProfileClass & b, float lerp )
{
	*this = a;
	FOV				= WWMath::Lerp( a.FOV,					b.FOV,					lerp );
	Height			= WWMath::Lerp( a.Height, 				b.Height,				lerp );
	ViewTilt			= WWMath::Lerp( a.ViewTilt,			b.ViewTilt,				lerp );
	TiltTweak		= WWMath::Lerp( a.TiltTweak,			b.TiltTweak, 			lerp );
	TranslationTilt= WWMath::Lerp( a.TranslationTilt,	b.TranslationTilt,	lerp );
	Distance			= WWMath::Lerp( a.Distance,			b.Distance,				lerp );
}

void	Convert_World_To_Camera( Matrix3D * tm ) 
{
	tm->Rotate_Y( DEG_TO_RADF(-90.0) );
	tm->Rotate_Z( DEG_TO_RADF(-90.0) );
}

const float CAMERA_UNWIND_SPEED = 1.0f;
#define CCAMERA_MIN_NEARZ					0.2f				// how small nearz gets when we're up against a wall
void CCameraClass::Update()
{
	Handle_Input();
	if ( SnapShotMode != SNAPSHOT_OFF )
	{
		Handle_Snap_Shot_Mode();
		return;
	}
	if ( Is_Using_Host_Model()	)
	{
		Use_Host_Model();
		return;
	}
	Vector3	anchor_position = AnchorPosition;
	float		camera_heading  =	Heading;
	CCameraProfileClass	profile;
	bool interpolating = false;
	if ( LerpTimeTotal )
	{
		interpolating = true;
		float frame_time = TimeManager::FrameSeconds;
		float lerp = WWMath::Clamp( LerpTimeRemaining / LerpTimeTotal, 0, 1 );
		LerpTimeRemaining -= frame_time;
		if ( LerpTimeRemaining <= 0 )
		{
			LerpTimeRemaining = 0.0f;
			LerpTimeTotal = 0.0f;
			LastAnchorPosition = anchor_position;
			LastHeading	= Heading;	
		}
		profile = *CurrentProfile;
		if ( LastProfile )
		{
			profile.Lerp( *CurrentProfile, *LastProfile, lerp );
		}
		anchor_position = ::Lerp( anchor_position, LastAnchorPosition, lerp );
		camera_heading	 =	RadianLerp( camera_heading, LastHeading, lerp );
	}
	else
	{
		profile = *CurrentProfile;
		LastProfile = CurrentProfile;
		LastProfileName=CurrentProfileName;
		if ( profile.Lag.Length() > 0 )
		{
			bool lag_ok = false;
			if ( COMBAT_STAR == NULL || COMBAT_STAR->Is_Airborne() )
			{
				lag_ok = true;
				LagPersistTimer = 1;
			}
			else
			{
				if ( LagPersistTimer > 0 )
				{
					lag_ok = true;
					LagPersistTimer -= TimeManager::FrameSeconds;
				}
			}
			if ( DisableLag )
			{
				lag_ok = false;
				DisableLag = false;
			}
			if ( lag_ok )
			{
				Vector3 local_last;
				Vector3 local_current;
				Matrix3D tm = Get_Transform();
				Matrix3D::Inverse_Transform_Vector( tm, LastAnchorPosition, &local_last );
				Matrix3D::Inverse_Transform_Vector( tm, anchor_position, &local_current );
				Vector3	lerp = profile.Lag * WWMath::Clamp( LagPersistTimer, 0, 1);
				lerp.X = ::pow( lerp.X, 10 * TimeManager::FrameSeconds );
				lerp.Y = ::pow( lerp.Y, 10 * TimeManager::FrameSeconds );
				lerp.Z = ::pow( lerp.Z, 10 * TimeManager::FrameSeconds );
				lerp = Vector3( 1,1,1 ) - lerp;
				local_current.X = local_last.X + ( local_current.X - local_last.X ) * lerp.X;
				local_current.Y = local_last.Y + ( local_current.Y - local_last.Y ) * lerp.Y;
				local_current.Z = local_last.Z + ( local_current.Z - local_last.Z ) * lerp.Z;
				Matrix3D::Transform_Vector( tm, local_current, &anchor_position );
				Vector3 diff = anchor_position - LastAnchorPosition;
				if ( diff.Length() < 5.0f )
				{
					anchor_position = ::Lerp( anchor_position, LastAnchorPosition, 0.25f );
				}
				else
				{
					LagPersistTimer = 0;
					DisableLag = true;
				}
			}
		}
		LastAnchorPosition = anchor_position;
		LastHeading	= Heading;
	}
	Set_View_Plane( profile.FOV );
	Matrix3D	tm(1);
	tm.Translate( anchor_position );
	tm.Translate( Vector3( 0, 0, profile.Height ) );
	Convert_World_To_Camera( &tm );
	tm.Rotate_Y( camera_heading );
	tm.Rotate_X( -profile.ViewTilt - Tilt);
	if ( profile.Distance != 0 )
	{
		float nearz,farz;
		const float HEAD_RADIUS = 0.2f;
		Get_Clip_Planes(nearz,farz);
		tm.Translate_Z(nearz + HEAD_RADIUS);						
	}
	Vector3 intermediate_pos = tm.Get_Translation();
	Matrix3D intermediate_tm = tm;
	Vector3 camera_move(0,0,profile.Distance);
	camera_move.Rotate_X(-profile.TranslationTilt);
	camera_move.Rotate_X( -(WWMath::Max( -Tilt * profile.TiltTweak, 0 )) );
	tm.Translate(camera_move);
	Vector3 end_pos = tm.Get_Translation();
	if ( profile.Distance != 0 )
	{
		Ignore_Star_And_Vehicle();
		CastResultStruct res;				
		Set_Transform(intermediate_tm);
		Set_Clip_Planes( NearClipPlane,FarClipPlane );
		OBBoxClass box = Get_Near_Clip_Bounding_Box();
		PhysOBBoxCollisionTestClass boxtest(	box,
															end_pos - intermediate_pos,
															&res,
															DEFAULT_COLLISION_GROUP,
															COLLISION_TYPE_CAMERA);
		PhysicsSceneClass::Get_Instance()->Cast_OBBox(boxtest);
		if ( res.StartBad && interpolating && boxtest.CollidedPhysObj )
		{
			PhysClass * hit = boxtest.CollidedPhysObj;
			hit->Inc_Ignore_Counter();
			res.Reset();
			PhysicsSceneClass::Get_Instance()->Cast_OBBox(boxtest);
			hit->Dec_Ignore_Counter();
		}
		if (res.Fraction < DistanceFraction)
		{
			DistanceFraction = res.Fraction;
		}
		if (res.Fraction > DistanceFraction)
		{
			float dt = TimeManager::FrameSeconds;
			DistanceFraction += WWMath::Min(res.Fraction-DistanceFraction,CAMERA_UNWIND_SPEED * dt);
		}
		if (DistanceFraction < 1.0f)
		{
			tm.Set_Translation(intermediate_pos + DistanceFraction * (end_pos - intermediate_pos));
			end_pos = tm.Get_Translation();
		}
		Unignore_Star_And_Vehicle();
	}
	else 
	{
		Ignore_Star_And_Vehicle();
		CastResultStruct res;				
		Set_Transform(tm);
		Set_Clip_Planes(NearClipPlane,FarClipPlane);
		OBBoxClass box = Get_Near_Clip_Bounding_Box();
		Vector3 null_vec(0.0f,0.0f,0.0f);
		PhysOBBoxCollisionTestClass boxtest(	box,
															null_vec,
															&res,
															DEFAULT_COLLISION_GROUP,
															COLLISION_TYPE_CAMERA);
		PhysicsSceneClass::Get_Instance()->Cast_OBBox(boxtest);
		if (res.StartBad)
		{
			Set_Clip_Planes(CCAMERA_MIN_NEARZ,FarClipPlane);
		}
		else
		{
			Set_Clip_Planes(NearClipPlane,FarClipPlane);
		}
		Unignore_Star_And_Vehicle();
	}
	Set_Transform( tm );
	if ( Determine_Targeting_Position() == false )
	{
		Apply_Weapon_Help();
	}
	bool is_star_determining_target = CombatManager::Is_Star_Determining_Target();
	if ( COMBAT_STAR && is_star_determining_target )
	{
		COMBAT_STAR->Set_Targeting( StarTargetingPosition ); 
	}
}

RENEGADE_FUNCTION
void CCameraClass::Handle_Snap_Shot_Mode( void )
AT1(0x006DFD30);

RENEGADE_FUNCTION
void CCameraClass::Handle_Input()
AT1(0x006DF110);

void	CCameraClass::Use_Host_Model( void ) 
{
	Matrix3D tm = HostModel->Get_Bone_Transform( "CAMERA" );
	Set_Transform( tm );
	if ( !CinematicSnipingEnabled )
	{
		CurrentProfile->FOV = DEG_TO_RADF( 75.0f );
	}
	Set_View_Plane( CurrentProfile->FOV );
}

void CCameraClass::Force_Look( const Vector3 & target ) 
{
	Vector3 diff = Get_Transform().Get_Translation();
	diff -= target;

	Heading = WWMath::Atan2( -diff.Y, -diff.X );

	float drop = diff.Z;
	float dist = diff.Length();
	Tilt = WWMath::Asin( drop/dist );
	Tilt -= CurrentProfile->ViewTilt;
}
