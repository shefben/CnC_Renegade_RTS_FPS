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
 *                     $Archive:: /Commando/Code/Combat/weatherenvironment.h                  $*
 *                                                                                             *
 *	Roadmap Section 36, Zero Hour / SAGE feature 23 -- the weather / environment particle        *
 *	layer.  The half of that feature Renegade did not already have.                             *
 *                                                                                             *
 *	Renegade's own weather is precipitation: WeatherSystemClass casts a grid of rays down from   *
 *	a box above the camera, spawns particles on the rays that can see the sky, and lets them     *
 *	fall until they hit what the ray hit.  That is why snow does not fall inside a building      *
 *	here, and it is a better answer than the donor's screen-space snow, so it stays.             *
 *                                                                                             *
 *	What it cannot be is atmosphere.  Dust hanging in the air, tiberium haze near a field,       *
 *	motes drifting through a shaft of light: none of them falls, none of them lands, and none    *
 *	of them wants a ray cast for it.  This layer is those -- it holds no particles of its own    *
 *	and no geometry at all.  It decides how many motes of each kind the air should hold and      *
 *	asks the batched particle pool of Section 26 for them, which is where the budget, the        *
 *	batching and the distance culling already live.                                             *
 *                                                                                             *
 *	WeatherMgrClass remains the one owner of weather state: these modes are precipitation types  *
 *	like the other three, server-set, saved and networked through the same parameters.  This is  *
 *	only what draws them.                                                                        *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef WEATHERENVIRONMENT_H
#define WEATHERENVIRONMENT_H

#include "always.h"
#include "aabox.h"
#include "vector3.h"


/*
**	The atmospheric modes.  Section 36 lists six initial visual modes; snow, rain-like
**	precipitation and ash are Renegade's own three and stay with WeatherSystemClass, so these
**	are the other three.  GENERIC_FALLING is the open one: something falls slowly through the
**	air near the viewer, and what it looks like is a texture and five numbers.
*/
enum WeatherEnvironmentModeType
{
	WEATHER_ENVIRONMENT_DUST	= 0,
	WEATHER_ENVIRONMENT_TIBERIUM,
	WEATHER_ENVIRONMENT_FALLING,

	WEATHER_ENVIRONMENT_MODE_COUNT
};


class WeatherEnvironmentRenderer
{
public:

	/*
	**	Lifetime.  Init defines one particle kind per mode in the batched pool and allocates
	**	nothing else, ever.  Reset is a level change: the air of the last place is not the air
	**	of this one.
	*/
	static void			Init(void);
	static void			Shutdown(void);
	static void			Reset(void);

	/*
	**	How much of each mode the air holds, as the density WeatherMgrClass ramps: zero is off,
	**	one is the mode's full strength, and more than one is more than that up to the budget.
	*/
	static void			Set_Density(int mode,float density);
	static float		Get_Density(int mode);

	/*
	**	The wind, as metres per second.  Taken from the same WindClass the precipitation uses,
	**	so a gust moves the dust and the snow together.
	*/
	static void			Set_Wind(const Vector3 & velocity);
	static const Vector3 &	Get_Wind(void);

	/*
	**	Where the air is.  By default the volume follows the camera, which is what weather is;
	**	a region pins it to one part of the world instead, which is what a tiberium field or a
	**	burning building is.
	*/
	static void			Set_Region(const AABoxClass & region);
	static void			Clear_Region(void);
	static bool			Has_Region(void);
	static const AABoxClass &	Get_Volume(void);

	/*
	**	The layer's own ceiling, in particles, across every mode.  The pool has a ceiling of its
	**	own for the whole ENVIRONMENT budget class; this is the share of it the weather may use,
	**	so that turning the dust up cannot take the slots a Commander's smoke was counting on.
	*/
	static void			Set_Budget(int particles);
	static int			Get_Budget(void);

	/*
	**	One step.  Works out what each mode's population should be, and emits the difference.
	*/
	static void			Update(const Vector3 & camera_position);

	/*
	**	Accounting.  Live against target is the acceptance in two numbers: a mode converges on
	**	its target and stays there however long the weather runs.
	*/
	static int			Get_Live_Count(int mode);
	static int			Get_Live_Count(void);
	static int			Get_Target_Count(int mode);
	static int			Get_Target_Count(void);
	static int			Get_Emitted_Count(void);
	static int			Get_Refused_Count(void);

	/*
	**	What a mode is, and what draws it.  The modes name no texture until the art exists --
	**	see docs/assets/WeatherSprites.md -- and naming one is how the layer is looked at.
	*/
	static const char *	Get_Mode_Name(int mode);
	static int			Get_Definition_Index(int mode);
	static bool			Names_A_Texture(int mode);
	static bool			Set_Texture(int mode,const char * texture_name);

private:

	static int			Mode_Full_Population(int mode);
	static void			Compute_Targets(int * targets);
	static int			Emit_Into(int mode,int count,const AABoxClass & volume);

	static int			Definition[WEATHER_ENVIRONMENT_MODE_COUNT];
	static float		Density[WEATHER_ENVIRONMENT_MODE_COUNT];
	static int			Target[WEATHER_ENVIRONMENT_MODE_COUNT];
	static unsigned		Phase[WEATHER_ENVIRONMENT_MODE_COUNT];

	static Vector3		Wind;
	static AABoxClass	Region;
	static bool			RegionValid;
	static AABoxClass	Volume;

	static int			Budget;
	static int			Emitted;
	static int			Refused;
	static bool			Initialized;
};


#endif //WEATHERENVIRONMENT_H
