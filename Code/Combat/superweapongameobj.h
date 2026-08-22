/***********************************************************************************************
 ***                            Confidential - Westwood Studios                              ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/superweapongameobj.h                  $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef	__SUPERWEAPONGAMEOBJ_H
#define	__SUPERWEAPONGAMEOBJ_H

#include "always.h"
#include "building.h"


////////////////////////////////////////////////////////////////
//	Forward delcarations
////////////////////////////////////////////////////////////////
class BaseControllerClass;


////////////////////////////////////////////////////////////////
//
//	SuperweaponGameObjDef
//
////////////////////////////////////////////////////////////////
class SuperweaponGameObjDef : public BuildingGameObjDef
{
public:

	////////////////////////////////////////////////////////////////
	//	Friends
	////////////////////////////////////////////////////////////////
	friend class SuperweaponGameObj;

	////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	////////////////////////////////////////////////////////////////
	SuperweaponGameObjDef (void);
	~SuperweaponGameObjDef (void);

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////
	uint32								Get_Class_ID (void) const override;
	PersistClass *						Create (void) const override;
	bool									Save (ChunkSaveClass &csave) override;
	bool									Load (ChunkLoadClass &cload) override;
	const PersistFactoryClass &	Get_Factory (void) const override;

	////////////////////////////////////////////////////////////////
	//	Editable support
	////////////////////////////////////////////////////////////////
	DECLARE_EDITABLE (SuperweaponGameObjDef, BuildingGameObjDef);

protected:

	////////////////////////////////////////////////////////////////
	//	Protected methods
	////////////////////////////////////////////////////////////////
	void					Load_Variables (ChunkLoadClass &cload);
};


////////////////////////////////////////////////////////////////
//
//	SuperweaponGameObj
//
//	A building whose only behavior of its own is a launch: something asks it to
//	fire, it runs its launch animation and then goes quiet again.  What is
//	actually fired belongs to whatever asked -- the building is the visible
//	half of it, which is why the launch has to reach the clients and the rest
//	does not.
//
//	The launch animation is a static anim phys placed in the level next to the
//	building and recognised by its model name, the same arrangement the war
//	factory uses for its construction animation.
//
////////////////////////////////////////////////////////////////
class SuperweaponGameObj : public BuildingGameObj
{
public:

	////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	////////////////////////////////////////////////////////////////
	SuperweaponGameObj (void);
	~SuperweaponGameObj (void);

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	//
	// Definition support
	//
	virtual	void								Init( void ) override;
	void											Init (const SuperweaponGameObjDef & definition);
	const SuperweaponGameObjDef &		Get_Definition (void) const;

	//
	// RTTI
	//
	SuperweaponGameObj *				As_SuperweaponGameObj (void) override	{ return this; }

	//
	// Persist support
	//
	bool									Save (ChunkSaveClass &csave) override;
	bool									Load (ChunkLoadClass &cload) override;
	const	PersistFactoryClass &	Get_Factory (void) const override;

	//
	//	From BuildingGameObj
	//
	void					CnC_Initialize (BaseControllerClass *base) override;
	void					On_Destroyed (void) override;

	//
	//	GameObj methods
	//
	void					Think (void) override;

	//
	//	Launch support.  A destroyed weapon cannot launch, and a launch already
	//	under way is not restarted.
	//
	void					Launch (void);
	bool					Is_Launching (void) const	{ return Launching; }

	//
	//	Network object support.  The launch is presentation the clients have to
	//	see; it changes too seldom for the frequent tier and too often to wait
	//	for the rare one.
	//
	void					Import_Occasional (BitStreamClass &packet) override;
	void					Export_Occasional (BitStreamClass &packet) override;

protected:

	////////////////////////////////////////////////////////////////
	//	Protected methods
	////////////////////////////////////////////////////////////////
	void					Load_Variables (ChunkLoadClass &cload);
	void					Play_Launch_Animation (bool onoff);

	////////////////////////////////////////////////////////////////
	//	Protected member data
	////////////////////////////////////////////////////////////////
	int					LaunchAnimationID;	// Static anim phys that plays the launch
	float					LaunchTimer;			// Seconds left in the current launch
	bool					Launching;
};


#endif	// __SUPERWEAPONGAMEOBJ_H
