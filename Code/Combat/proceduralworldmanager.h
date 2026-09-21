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
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Combat/proceduralworldmanager.h              $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *	The procedural world owner.  V2 roadmap Section 34, from Phase 3 onward.                     *
 *                                                                                             *
 *	Phases 0 to 2 produced services -- a profile parser, a seed deriver, a manifest, a boundary  *
 *	-- and no one thing that decides a world exists and drives them in order.  Phase 3 step 1    *
 *	asks for a cliff component "under the procedural-world/terrain owner", which is the first    *
 *	time that owner is actually needed.  This is it, and every later phase's generation stage    *
 *	hangs off Generate_World rather than finding its own way into the level load.                *
 *                                                                                             *
 *	Directive 1.10 is the whole shape of this class.  A hand-authored Renegade map must keep     *
 *	working untouched, so nothing here runs unless something explicitly asked for a procedural   *
 *	world: Request_World is that explicit ask, and with no request outstanding Generate_World    *
 *	returns immediately having configured nothing.  The boundary stays unconfigured, the cliff   *
 *	ring stays unbuilt, and the level loads exactly as it did before v2 existed.                 *
 *                                                                                             *
 *	Where the request comes from.  The main menu's procedural button, which sets the request     *
 *	before starting the game, and the `procedural_world` console command.  Both are client-side  *
 *	entry points into a server-side generation: the generation itself runs inside the level      *
 *	load, on whichever side is loading.                                                          *
 *                                                                                             *
 *	Why generation is not replicated.  Section 3B exists precisely so it does not have to be.    *
 *	Every client deriving the same world from the same seed and profile is the determinism       *
 *	contract working; sending a generated world over the wire would be sending something both    *
 *	ends can compute.  What must agree is the seed and the profile, and the manifest is what     *
 *	makes disagreement detectable rather than silent.                                            *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef PROCEDURALWORLDMANAGER_H
#define PROCEDURALWORLDMANAGER_H

#ifndef ALWAYS_H
#include "always.h"
#endif

#include "proceduralworldprofile.h"
#include "worldgenerationmanifest.h"


class ProceduralWorldManager
{
	public:

		//	The profile shipped with the game, used when a request names no other.
		static const char * const DEFAULT_PROFILE_PATH;

		static void		Init(void);
		static void		Shutdown(void);

		/*
		**	Ask for the next level load to build a procedural world over itself.
		**
		**	`seed` of 0 means "choose one", which is what the menu button does -- the chosen
		**	value is normalized, recorded in the manifest and logged, so a world a player liked
		**	can be asked for again by number rather than being unrepeatable.
		**
		**	`profile_path` of nullptr means DEFAULT_PROFILE_PATH.
		**
		**	Returns false and requests nothing if the profile will not load or will not
		**	validate.  Failing here, before a level starts loading, is deliberate: the
		**	alternative is a player watching a level load and then finding an ordinary map,
		**	with the reason only in a log.
		*/
		static bool		Request_World(unsigned int seed,const char * profile_path,StringClass * error_out);

		//	Drop any outstanding request.  The next level load is an ordinary one.
		static void		Clear_Request(void);

		static bool		Is_Requested(void)			{ return Requested; }
		static bool		Is_World_Active(void)		{ return WorldActive; }

		/*
		**	Run the generation for the level that has just finished loading.  Called from
		**	CombatManager::Post_Load_Level and from nowhere else; returns immediately with no
		**	request outstanding, which is every hand-authored map.
		**
		**	It is deliberately called after the level is complete rather than before it starts,
		**	because a world fitted to its host level cannot be sized until that level's own
		**	extents exist, and because the cliff ring needs a physics scene to put itself in.
		*/
		static bool		Generate_World(StringClass * error_out);

		//	Tear the generated world down.  Called from the level unload path.
		static void		Destroy_World(void);

		/*
		**	What was generated, for the console, the manifest file and the self check.  The
		**	profile returned is the one actually used, including any fit-to-level override, not
		**	the one as authored -- otherwise a manifest would fail to reproduce its own world.
		*/
		static const ProceduralWorldProfile &		Get_Active_Profile(void)	{ return ActiveProfile; }
		static const WorldGenerationManifest &		Get_Manifest(void)			{ return Manifest; }

	private:

		static bool		Fit_Profile_To_Level(ProceduralWorldProfile * profile);
		static bool		Build_Cliff_Ring(const ProceduralWorldProfile & profile,StringClass * error_out);

		//	Roadmap Section 34 Phase 4.  `built_out` is set to whether generation actually ran --
		//	false and a true return together mean "correctly skipped", which is what a
		//	`fit_to_level` world always does; see macroterraingenerator.h's own class comment for
		//	why a fitted world's ground is never touched.
		static bool		Build_Macro_Terrain(const ProceduralWorldProfile & profile,bool * built_out,StringClass * error_out);

		//	Roadmap Section 34 Phase 5.  Only meaningful once macro terrain stands, so this is a
		//	no-op (not a failure) whenever Build_Macro_Terrain itself was -- a `fit_to_level`
		//	world has no chunk registry to read candidates from, same reasoning as Phase 4's own.
		static bool		Build_Interior_Cliffs(const ProceduralWorldProfile & profile,StringClass * error_out);

		static bool		Requested;
		static bool		WorldActive;

		static unsigned int		RequestedSeed;
		static StringClass		RequestedProfilePath;

		static ProceduralWorldProfile		ActiveProfile;
		static WorldGenerationManifest	Manifest;
};

#endif	// PROCEDURALWORLDMANAGER_H
