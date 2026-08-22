#ifndef TT_INCLUDE__CCAMERACLASS_H
#define TT_INCLUDE__CCAMERACLASS_H



#include "CameraClass.h"



class CCameraProfileClass;
class Listener3DClass;

class CCameraClass :
	public CameraClass
{

protected:
	RenderObjClass *HostModel; // 0380
	Vector3 AnchorPosition; // 0384
	bool IsValid; // 0390
	float Tilt; // 0394
	float Heading; // 0398
	float DistanceFraction; // 039C
	bool Enable2DTargeting; // 03A0
	float LagPersistTimer; // 03A4
	bool DisableLag; // 03A8
	bool EnableWeaponHelp; // 03A9
	float WeaponHelpTimer; // 03AC
	int	WeaponHelpTargetID; // 03B0
	Vector3 StarTargetingPosition; // 03B4
	Vector2 CameraTarget2DOffset; // 03C0
	float LerpTimeTotal; // 03C8
	float LerpTimeRemaining; // 03CC
	Vector3 LastAnchorPosition; // 03D0
	float LastHeading; // 03DC
	CCameraProfileClass *CurrentProfile; // 03E0
	CCameraProfileClass *LastProfile; // 03E4
	CCameraProfileClass *DefaultProfile; // 03E8
	StringClass	CurrentProfileName; // 03EC
	StringClass LastProfileName; // 03F0
	StringClass DefaultProfileName; // 03F4
	float NearClipPlane; // 03F8
	float FarClipPlane; // 03FC
	bool IsStarSniping; // 0400
	bool WasStarSniping; // 0401
	float SniperZoom; // 0404
	float SniperDistance; // 0408
	Listener3DClass* SniperListener; // 040C
	bool CinematicSnipingEnabled; // 0410
	float CinematicSnipingDesiredZoom; // 0414
	enum {
		SNAPSHOT_OFF,
		SNAPSHOT_ON,
		SNAPSHOT_PROGRESS,
	};
	int SnapShotMode; // 0418
public:
	SHADERS_API static void Init();
	SHADERS_API static void Shutdown();
	CCameraClass();
	void				Use_Default_Profile( void );
	virtual ~CCameraClass();
	virtual	bool Save(ChunkSaveClass &csave);
	virtual	bool Load(ChunkLoadClass &cload);
	virtual void Update();
	bool Is_Star_Sniping()
	{
		return IsStarSniping;
	}
	float Get_Sniper_Zoom()
	{
		return SniperZoom;
	}
	bool Is_Using_Host_Model()
	{
		return (HostModel != NULL);
	}
	bool Is_Valid()
	{
		return IsValid;
	}

	bool				Is_2D_Targeting( void )					{ return Enable2DTargeting; }
	const Vector2 &Get_Camera_Target_2D_Offset()			{ return CameraTarget2DOffset; }
	void				Set_Host_Model( RenderObjClass * host );
	bool Draw_Sniper()
	{
		if (Is_Using_Host_Model())
		{
			return CinematicSnipingEnabled;
		}
		else
		{
			return IsStarSniping;
		}
	}
	bool				Is_In_Cinematic( void )					{ return HostModel != NULL; }
	bool				Determine_Targeting_Position( void );	
	void				Set_Sniper_Distance( float dist );
	void				Update_Sniper_Listener_Pos( void );
	void				Apply_Weapon_Help( void );
	void				Handle_Snap_Shot_Mode( void );
	void				Handle_Input();
	void						Use_Host_Model( void );
	SHADERS_API void				Force_Look( const Vector3 & target );
}; // 0434


#endif
