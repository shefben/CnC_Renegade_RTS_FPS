/***********************************************************************************************
 ***                            Confidential - Westwood Studios                              ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Commando                                                     *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/constructionyardgameobj.h             $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*/

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef	__CONSTRUCTIONYARDGAMEOBJ_H
#define	__CONSTRUCTIONYARDGAMEOBJ_H

#include "always.h"
#include "building.h"


////////////////////////////////////////////////////////////////
//	Forward delcarations
////////////////////////////////////////////////////////////////
class BaseControllerClass;


////////////////////////////////////////////////////////////////
//
//	ConstructionYardGameObjDef
//
////////////////////////////////////////////////////////////////
class ConstructionYardGameObjDef : public BuildingGameObjDef
{
public:

	////////////////////////////////////////////////////////////////
	//	Friends
	////////////////////////////////////////////////////////////////
	friend class ConstructionYardGameObj;

	////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	////////////////////////////////////////////////////////////////
	ConstructionYardGameObjDef (void);
	~ConstructionYardGameObjDef (void);

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
	DECLARE_EDITABLE (ConstructionYardGameObjDef, BuildingGameObjDef);

protected:

	////////////////////////////////////////////////////////////////
	//	Protected methods
	////////////////////////////////////////////////////////////////
	void					Load_Variables (ChunkLoadClass &cload);

	////////////////////////////////////////////////////////////////
	//	Protected member data
	////////////////////////////////////////////////////////////////
	float					RepairRate;					// Seconds between building repair ticks
	float					VehicleRepairRate;		// Seconds between vehicle repair ticks
	float					RepairAmount;				// Health restored per tick
	float					LowPowerRepairAmount;	// ... and per tick while the base has no power
	int					RepairWarhead;				// Warhead the repair is credited to
};


////////////////////////////////////////////////////////////////
//
//	ConstructionYardGameObj
//
//	Keeps the rest of its team's base standing: every RepairRate seconds it
//	puts health back into each of the team's buildings, and every
//	VehicleRepairRate seconds into each of the team's vehicles.  Losing base
//	power does not stop it, it only slows it to LowPowerRepairAmount.
//
////////////////////////////////////////////////////////////////
class ConstructionYardGameObj : public BuildingGameObj
{
public:

	////////////////////////////////////////////////////////////////
	//	Public constructors/destructors
	////////////////////////////////////////////////////////////////
	ConstructionYardGameObj (void);
	~ConstructionYardGameObj (void);

	////////////////////////////////////////////////////////////////
	//	Public methods
	////////////////////////////////////////////////////////////////

	//
	// Definition support
	//
	virtual	void									Init( void ) override;
	void												Init (const ConstructionYardGameObjDef & definition);
	const ConstructionYardGameObjDef &		Get_Definition (void) const;

	//
	// RTTI
	//
	ConstructionYardGameObj *		As_ConstructionYardGameObj (void) override	{ return this; }

	//
	// Persist support
	//
	bool									Save (ChunkSaveClass &csave) override;
	bool									Load (ChunkLoadClass &cload) override;
	const	PersistFactoryClass &	Get_Factory (void) const override;

	//
	//	GameObj methods
	//
	void					Think (void) override;

	//
	//	A spy in the yard shuts the repairs off for as long as he is in there.
	//	Nothing else about the building changes, which is why this is not
	//	Set_Is_Destroyed or a disable on the base.
	//
	void					Set_Spy (bool onoff)			{ IsSpy = onoff; }
	bool					Is_Spied (void) const		{ return IsSpy; }

protected:

	////////////////////////////////////////////////////////////////
	//	Protected methods
	////////////////////////////////////////////////////////////////
	void					Load_Variables (ChunkLoadClass &cload);
	void					Repair_Buildings (float amount);
	void					Repair_Vehicles (float amount);

	////////////////////////////////////////////////////////////////
	//	Protected member data
	////////////////////////////////////////////////////////////////
	float					RepairTimer;
	float					VehicleRepairTimer;
	bool					IsSpy;
};


#endif	// __CONSTRUCTIONYARDGAMEOBJ_H
