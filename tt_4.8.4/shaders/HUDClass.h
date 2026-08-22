#ifndef TT_INCLUDE__HUDCLASS_H
#define TT_INCLUDE__HUDCLASS_H



class ChunkLoadClass;
class ChunkSaveClass;



class SHADERS_API HUDClass
{

public:

	static	void	Init(bool render_available = true);
	static	void 	Shutdown();
	static	void 	Think();
	static	void 	Render();
	static	void 	Reset();
	static	bool	Save( ChunkSaveClass &csave );
	static	bool	Load( ChunkLoadClass &cload );
	static	void	Display_Points( float points );
	static	void	Toggle_Hide_Points( void );
	static	bool	Is_Enabled( void );
	static	void	Enable( bool );
	static	void	Force_Weapon_Chart_Update( void );
	static	void	Force_Weapon_Chart_Display( void );
	static	void	Add_Powerup_Weapon( int id, int rounds );
	static	void	Add_Powerup_Ammo( int id, int rounds );
	static	void	Add_Shield_Grant( float strength );
	static	void	Add_Health_Grant( float amount );
	static	void	Add_Shield_Upgrade_Grant( float strength );
	static	void	Add_Health_Upgrade_Grant( float amount );
	static	void	Add_Key_Grant( int key );
	static	void	Add_Objective( int type );
	static	void	Add_Data_Link( void );
	static	void	Add_Map_Reveal( void );
	static	void	Damage_Render( void );
};



#endif