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

/******************************************************************************
*
* FILE
*     TT_Crates.cpp
*
* DESCRIPTION
*     The crate: a box lying in the world that gives whoever walks into it
*     one of ten things at random, some of which nobody wants.  The level
*     author turns each of the ten on or off, and the ones left in are drawn
*     from according to fixed weights.
*
*     Native port of the crate half of the 4.8.4 library's dan.cpp.  Its two
*     wreckage scripts belong with the other vehicle behaviour and live in
*     TT_Vehicles.cpp.
*
*     The donor drew its prize by building a table of repeated indices and
*     picking a slot, and got that wrong three ways: the loop filling the
*     table stopped one kind short, so the thief crate could never come up at
*     all; the draw ran from one to the total, so it never picked the first
*     slot and read one past the last; and each kind was written into the
*     table one more time than its weight said.  The draw here walks the
*     weights directly, which has no table to run off the end of.
*
*     The donor talked to players by writing SSGM console command lines
*     ("MESSAGE ...", "PPAGE <id> ...") and letting a server plugin parse
*     them back apart.  There is no such plugin here and no need for one:
*     those are `Send_Message` and `Send_Message_Player`, which take the
*     player rather than a number naming them.
*
******************************************************************************/

#include "scripts.h"
#include "Toolkit.h"

#include "gameobjobserver.h"
#include "playertype.h"

#include <stdio.h>
#include <stdarg.h>


/*
**	Crate announcements are the same colour as the rest of the server's.
*/
static const int	CRATE_RED	= 104;
static const int	CRATE_GREEN	= 234;
static const int	CRATE_BLUE	= 40;


static void Announce (const char * format, ...)
{
	char		text[512];
	va_list	args;

	va_start (args, format);
	::_vsnprintf (text, sizeof (text), format, args);
	va_end (args);

	text[sizeof (text) - 1] = 0;

	ScriptEngine::Send_Message (CRATE_RED, CRATE_GREEN, CRATE_BLUE, text);
}


static void Tell (GameObject * player, const char * format, ...)
{
	char		text[512];
	va_list	args;

	va_start (args, format);
	::_vsnprintf (text, sizeof (text), format, args);
	va_end (args);

	text[sizeof (text) - 1] = 0;

	ScriptEngine::Send_Message_Player (player, CRATE_RED, CRATE_GREEN, CRATE_BLUE, text);
}


/*
**	The announcements name the side the finder is on.  The donor left this
**	null for anyone who was on neither, and then printed it.
*/
static const char * Side_Name (GameObject * obj)
{
	switch (ScriptEngine::Get_Player_Type (obj))
	{
		case PLAYERTYPE_NOD:	return "Nod";
		case PLAYERTYPE_GDI:	return "GDI";
		default:					return "unaligned";
	}
}


static const char * Other_Side_Name (GameObject * obj)
{
	switch (ScriptEngine::Get_Player_Type (obj))
	{
		case PLAYERTYPE_NOD:	return "GDI";
		case PLAYERTYPE_GDI:	return "Nod";
		default:					return "everyone";
	}
}


/*
**	The ten things a crate can hold, in the order the switch below expects,
**	each with the parameter that turns it on and how often it comes up
**	relative to the others.
*/
struct JFW_Crate_Kind
{
	const char *	Parameter;
	int				Weight;
};


static const JFW_Crate_Kind	CRATE_KIND[] =
{
	{	"Points",			4	},	//	0	points, given or taken
	{	"Death_Crate",		4	},	//	1	kills the finder
	{	"Stealth_Suit",	2	},	//	2	turns the finder invisible
	{	"DeHealth",			5	},	//	3	health down to one
	{	"DeArmour",			5	},	//	4	armour down to one
	{	"Full_Health",		4	},	//	5	health and armour restored
	{	"Money",				4	},	//	6	credits, given or taken
	{	"Spy_Crate",		1	},	//	7	hides the finder from the map
	{	"Random_Weapon",	5	},	//	8	one weapon from the locker
	{	"Theif",				4	}	//	9	takes every credit the finder has
};

static const int	CRATE_KIND_COUNT = sizeof (CRATE_KIND) / sizeof (CRATE_KIND[0]);


/*
**	The weapons locker the random weapon comes out of.
*/
struct JFW_Crate_Weapon
{
	const char *	Preset;
	const char *	Name;
};


static const JFW_Crate_Weapon	CRATE_WEAPON[] =
{
	{	"CnC_POW_AutoRifle_Player_GDI",		"AutoRifle"					},
	{	"POW_Chaingun_Player",					"ChainGun"					},
	{	"POW_ChemSprayer_Player",				"Chem Sprayer"				},
	{	"POW_Flamethrower_Player",				"FlameThrower"				},
	{	"POW_GrenadeLauncher_Player",			"Grenade Launcher"		},
	{	"POW_LaserChaingun_Player",			"Laser Chaingun"			},
	{	"POW_LaserRifle_Player",				"Laser Rifle"				},
	{	"POW_MineProximity_Player",			"Proximity Mine"			},
	{	"POW_MineRemote_Player",				"Remote Mine"				},
	{	"POW_MineTimed_Player",					"Timed C4"					},
	{	"POW_PersonalIonCannon_Player",		"Personal Ion Cannon"	},
	{	"POW_Railgun_Player",					"RailGun"					},
	{	"POW_RamjetRifle_Player",				"Ramjet Rifle"				},
	{	"POW_RocketLauncher_Player",			"RocketLauncher"			},
	{	"POW_Shotgun_Player",					"Shotgun"					},
	{	"POW_SniperRifle_Player",				"Sniper Rifle"				},
	{	"POW_TiberiumAutoRifle_Player",		"Tiberium AutoRifle"		},
	{	"POW_TiberiumFlechetteGun_Player",	"Tiberium Flechette Gun"}
};

static const int	CRATE_WEAPON_COUNT = sizeof (CRATE_WEAPON) / sizeof (CRATE_WEAPON[0]);


/*DAN_CnC_Crate

  A crate.  It blocks vehicles while it sits there, so nobody can park on top
  of one, and when a player walks into it the blocker goes and one of the ten
  prizes lands on them.

  Every parameter is a switch: set it to one or more to leave that prize in
  the draw, zero to take it out.
*/

DECLARE_SCRIPT_TT (DAN_CnC_Crate,
		"Death_Crate:int,Stealth_Suit:int,DeHealth:int,DeArmour:int,Full_Health:int,"
		"Money:int,Spy_Crate:int,Random_Weapon:int,Points:int,Theif:int")
{
	int	BlockerID;

	REGISTER_VARIABLES()
	{
		SAVE_VARIABLE (BlockerID, 1);
	}

	void Created (GameObject * obj) override
	{
		ScriptEngine::Set_Model (obj, "vehcol2m");

		BlockerID = ScriptEngine::Get_ID (ScriptEngine::Create_Object ("Vehicle_Blocker",
				ScriptEngine::Get_Position (obj)));
	}

	void Custom (GameObject * obj, int type, intptr_t /*param*/, GameObject * sender) override
	{
		if (type != CUSTOM_EVENT_POWERUP_GRANTED || sender == nullptr)
		{
			return;
		}

		ScriptEngine::Destroy_Object (ScriptEngine::Find_Object (BlockerID));
		BlockerID = 0;

		int	total = 0;

		for (int kind = 0; kind < CRATE_KIND_COUNT; kind ++)
		{
			if (Get_Int_Parameter (CRATE_KIND[kind].Parameter) >= 1)
			{
				total += CRATE_KIND[kind].Weight;
			}
		}

		if (total <= 0)
		{
			return;
		}

		//
		//	Walk the weights rather than a table of repeated indices.  The
		//	donor's table was filled by a loop that stopped one kind short and
		//	written one entry too many per kind, and then indexed from one to
		//	the total, so it skipped its own first slot and read past its last.
		//
		int	roll	= ScriptEngine::Get_Random_Int (0, total);
		int	drawn	= CRATE_KIND_COUNT - 1;

		for (int kind = 0; kind < CRATE_KIND_COUNT; kind ++)
		{
			if (Get_Int_Parameter (CRATE_KIND[kind].Parameter) < 1)
			{
				continue;
			}

			roll -= CRATE_KIND[kind].Weight;

			if (roll < 0)
			{
				drawn = kind;
				break;
			}
		}

		//	Half the prizes come in a giving and a taking flavour.
		bool	generous = (ScriptEngine::Get_Random_Int (1, 3) == 1);

		switch (drawn)
		{
			case 0:	Points			(sender, generous);	break;
			case 1:	Death_Crate		(obj, sender);			break;
			case 2:	Stealth_Suit	(sender);				break;
			case 3:	De_Health		(sender);				break;
			case 4:	De_Armour		(sender);				break;
			case 5:	Full_Health		(sender);				break;
			case 6:	Money				(sender, generous);	break;
			case 7:	Spy_Crate		(sender);				break;
			case 8:	Random_Weapon	(sender);				break;
			case 9:	Thief				(sender);				break;
		}
	}

	//
	//	One in six of the prizes is worth ten times the smallest, so the
	//	amount is drawn separately from the kind.
	//
	int Amount (void)
	{
		int	roll = ScriptEngine::Get_Random_Int (1, 7);

		if (roll < 4)	return 100;
		if (roll < 6)	return 500;

		return 1000;
	}

	void Points (GameObject * finder, bool generous)
	{
		int	amount = Amount ();

		if (generous)
		{
			ScriptEngine::Give_Points (finder, (float)amount, true);
			Announce ("[Crate] Enjoy those %d points, %s", amount, Side_Name (finder));
		}
		else
		{
			ScriptEngine::Give_Points (finder, (float)-amount, true);
			Announce ("[Crate] Hope you don't miss those %d points, %s",
					amount, Side_Name (finder));
		}
	}

	void Money (GameObject * finder, bool generous)
	{
		int	amount = Amount ();

		if (generous)
		{
			ScriptEngine::Give_Money (finder, (float)amount, false);
			Tell (finder, "[Crate] You have got %d credits from the Money Crate!", amount);
		}
		else
		{
			ScriptEngine::Give_Money (finder, (float)-amount, false);
			Tell (finder, "[Crate] You have lost %d credits from the Money Crate!", amount);
		}
	}

	void Death_Crate (GameObject * obj, GameObject * finder)
	{
		ScriptEngine::Create_Object ("Beacon_Ion_Cannon_Anim_Post",
				ScriptEngine::Get_Position (obj));

		ScriptEngine::Apply_Damage (finder, 1000.0f, "Death", nullptr);

		Announce ("[Crate] Some poor %s guy just got owned by the Death Crate!",
				Side_Name (finder));
	}

	void Stealth_Suit (GameObject * finder)
	{
		ScriptEngine::Enable_Stealth (finder, true);
		Announce ("[Crate] Some %s guy found the Stealth Suit!", Side_Name (finder));
	}

	void De_Health (GameObject * finder)
	{
		ScriptEngine::Set_Health (finder, 1.0f);
		Tell (finder, "[Crate] You have got the DeHealth Crate -- your health is down to 1!");
	}

	void De_Armour (GameObject * finder)
	{
		//	Nothing to strip means the health crate instead.
		if (ScriptEngine::Get_Shield_Strength (finder) <= 1.0f)
		{
			De_Health (finder);
			return;
		}

		ScriptEngine::Set_Shield_Strength (finder, 1.0f);
		Tell (finder, "[Crate] You have got the DeArmour Crate -- your armour is down to 1!");
	}

	void Full_Health (GameObject * finder)
	{
		ScriptEngine::Set_Health (finder, ScriptEngine::Get_Max_Health (finder));
		ScriptEngine::Set_Shield_Strength (finder,
				ScriptEngine::Get_Max_Shield_Strength (finder));

		Tell (finder, "[Crate] You have got the Full Health Crate -- health and armour replenished!");
	}

	void Spy_Crate (GameObject * finder)
	{
		ScriptEngine::Set_Is_Visible (finder, false);

		Announce ("[Crate] Some %s guy got the Spy Crate! Better watch your base, %s",
				Side_Name (finder), Other_Side_Name (finder));
	}

	void Random_Weapon (GameObject * finder)
	{
		const JFW_Crate_Weapon & weapon =
				CRATE_WEAPON[ScriptEngine::Get_Random_Int (0, CRATE_WEAPON_COUNT)];

		ScriptEngine::Give_PowerUp (finder, weapon.Preset, true);

		Tell (finder, "[Crate] You have been given a %s from the Weapons Locker!", weapon.Name);
	}

	void Thief (GameObject * finder)
	{
		ScriptEngine::Give_Money (finder, -ScriptEngine::Get_Money (finder), false);

		Announce ("[Crate] Awwww, some poor %s guy lost all his money! Pwnt!",
				Side_Name (finder));
	}
};
