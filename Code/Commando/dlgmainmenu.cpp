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
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat																		  *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/Commando/dlgmainmenu.cpp       $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 2/12/02 1:28p                                               $*
 *                                                                                             *
 *                    $Revision:: 29                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#include "dlgmainmenu.h"
#include "ttsettings.h"
#include "assetmgr.h"
#include "renegadedialog.h"
#include "rendobj.h"
#include "hanim.h"
#include "gameinitmgr.h"
#include "mainmenutransition.h"
#include "menubackdrop.h"
#include "scene.h"
#include "renegadedialog.h"
#include "mesh.h"
#include "meshgeometry.h"
#include "dialogmgr.h"
#include "gameinitmgr.h"
#include "debug.h"
#include "dialogcontrol.h"
#include "specialbuilds.h"
#include "buildnum.h"
#include "campaign.h"
#include "gamedata.h"
#include "imagectrl.h"
#include "init.h"
#include "registry.h"
#include "_globals.h"
#include "dialogtests.h"
#include "DlgWOLWait.h"
#include "nicenum.h"
#include "DlgMessageBox.h"
#include "translatedb.h"
#include "string_ids.h"
#include "gamespyadmin.h"
#include "proceduralworldmanager.h"

////////////////////////////////////////////////////////////////
//	Static member initialization
////////////////////////////////////////////////////////////////
MainMenuDialogClass *	MainMenuDialogClass::_TheInstance	= nullptr;
bool MainMenuDialogClass::Animated = true;


////////////////////////////////////////////////////////////////
//	Q-030 option (b) -- the fit-to-level fallback
//
//	The native generator refuses a world several frames and two layers away from the button that
//	asked for it: the request is made here, the generation runs inside
//	CombatManager::Post_Load_Level, and CombatGameModeClass abandons the load and comes back to
//	this menu.  These two flags are what carry the decision across that gap.
//
//	NativeInFlight is armed only for a native request, so the fallback can happen once per
//	button press and a refused fallback can never arm another one.  FallbackPending holds the
//	start until the player has dismissed the box saying why -- a reason that flashes past under
//	a loading screen is a reason nobody reads.
////////////////////////////////////////////////////////////////
static bool	_ProceduralNativeInFlight	= false;
static bool	_ProceduralFallbackPending	= false;

////////////////////////////////////////////////////////////////
//
//	MainMenuDialogClass
//
////////////////////////////////////////////////////////////////
MainMenuDialogClass::MainMenuDialogClass (void)	:
	MenuDialogClass (GetRenegadeDialog(RenegadeDialogID::IDD_MENU_MAIN)),
	TitleTransModel (nullptr),
	LogoModel (nullptr),
	GizmoModel (nullptr),
	IsStartingPractice (false)
{
	LogoModel			= WW3DAssetManager::Get_Instance ()->Create_Render_Obj ("IF_RENLOGO");
	TitleTransModel	= WW3DAssetManager::Get_Instance ()->Create_Render_Obj ("IF_TITLETRANS");
	GizmoModel			= WW3DAssetManager::Get_Instance ()->Create_Render_Obj ("IF_EVAGIZMO");

	RegistryClass reg(APPLICATION_SUB_KEY_NAME_OPTIONS);
	if (reg.Get_Int("DisableMenuAnim", 0) != 0) {
		Animated = false;
	}


	if (TitleTransModel != nullptr && GizmoModel != nullptr && Animated) {

		//
		//	Play the gizmo animation
		//
		HAnimClass *gizmo_anim = WW3DAssetManager::Get_Instance ()->Get_HAnim ("IF_EVAGIZMO.IF_EVAGIZMO");
		if (gizmo_anim != nullptr) {
			GizmoModel->Set_Animation (gizmo_anim, 0.0F, RenderObjClass::ANIM_MODE_LOOP);
			REF_PTR_RELEASE (gizmo_anim);
		}

		//
		//	Add the gizmo to the transition model
		//
		TitleTransModel->Add_Sub_Object_To_Bone (GizmoModel, "IF_GIZMOBONE");
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	~MainMenuDialogClass
//
////////////////////////////////////////////////////////////////
MainMenuDialogClass::~MainMenuDialogClass (void)
{
	if (TitleTransModel != nullptr) {
		TitleTransModel->Remove ();
	}

	if (LogoModel != nullptr) {
		LogoModel->Remove ();
	}

	REF_PTR_RELEASE (LogoModel);
	REF_PTR_RELEASE (TitleTransModel);
	REF_PTR_RELEASE (GizmoModel);
	return ;
}


////////////////////////////////////////////////////////////////
//
//	On_Menu_Activate
//
////////////////////////////////////////////////////////////////
void
MainMenuDialogClass::On_Menu_Activate (bool onoff)
{
	if (TitleTransModel != nullptr) {

		//
		//	Either add or remove the logo from the scene
		//
		if (onoff) {

			// Put the logo pack into the scene when reactivated.
			if (LogoModel && LogoModel->Peek_Scene() == nullptr) {
				if (MenuBackDropClass *backdrop = Get_BackDrop()) {
					backdrop->Peek_Scene()->Add_Render_Object(LogoModel);
				}
			}

			//
			//	Force-shutdown any interfaces that are running... (this could
			// happen if the user navigates through the multiplay menus and then
			// returns to the main menu).
			//
			GameInitMgrClass::Shutdown ();
		} else {

			//
			//	Remove the logo from the screen
			//
			if (LogoModel != nullptr) {
				LogoModel->Remove ();
			}
		}
	}

	MenuDialogClass::On_Menu_Activate (onoff);

	/*
	**	A Procedural Skirmish whose world could not be generated lands back here (see the abandon
	**	path in CombatGameModeClass) rather than loading the empty native level, and this is where
	**	the player is told why.  Done on activation rather than in On_Init_Dialog because by now
	**	the skirmish mode above has been shut down, so starting another game is as safe as it is
	**	from the button itself.
	**
	**	Q-030 option (b): a refused *native* world falls back to a fit-to-level world over a stock
	**	skirmish map, which the generator does build, so the button produces a playable match
	**	instead of nothing.  Native stays what the button asks for first, and the refusal is still
	**	reported every time.
	*/
	if (onoff && ProceduralWorldManager::Did_Generation_Fail ()) {

		StringClass reason (ProceduralWorldManager::Get_Generation_Error ());
		ProceduralWorldManager::Clear_Generation_Failure ();

		bool may_fall_back			= _ProceduralNativeInFlight;
		_ProceduralNativeInFlight	= false;

		StringClass narrow;
		if (may_fall_back) {
			narrow.Format ("The generated world could not be built:\n\n%s\n\n"
								"Loading a Procedural Skirmish over a stock map instead.",reason.Peek_Buffer ());
		} else {
			narrow = reason;
		}

		WideStringClass message;
		message.Convert_From (narrow.Peek_Buffer ());

		if (may_fall_back) {
			_ProceduralFallbackPending = true;
			DlgMsgBox::DoDialog (TRANSLATE (IDS_MENU_TEXT513),message,DlgMsgBox::Okay,this);
		} else {
			DlgMsgBox::DoDialog (TRANSLATE (IDS_MENU_TEXT513),message);
		}
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	HandleNotification
//
////////////////////////////////////////////////////////////////
void
MainMenuDialogClass::HandleNotification (DlgMsgBoxEvent &event)
{
	if (!_ProceduralFallbackPending) {
		return ;
	}

	//	Dismissed either way -- the box has one button, and a dialog that quit without it is
	//	still the player done reading.
	if ((event.Event () != DlgMsgBoxEvent::Okay) && (event.Event () != DlgMsgBoxEvent::Quitting)) {
		return ;
	}

	_ProceduralFallbackPending = false;

	/*
	**	The fallback profile is fit-to-level, so `Uses_Hybrid_Generator` is false for it and it
	**	never enters the hybrid phases that refused the native world; Start_Procedural_Skirmish
	**	lays it over a stock skirmish map by itself.  A refusal here is reported and nothing
	**	further is attempted -- NativeInFlight is already clear, so this cannot recurse.
	*/
	StringClass world_error;
	if (!Start_Procedural_Skirmish (0,ProceduralWorldManager::DEFAULT_PROFILE_PATH,&world_error)) {
		WideStringClass message;
		message.Convert_From (world_error.Peek_Buffer ());
		DlgMsgBox::DoDialog (TRANSLATE (IDS_MENU_TEXT513),message);
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	On_Init_Dialog
//
////////////////////////////////////////////////////////////////
void
MainMenuDialogClass::On_Init_Dialog (void)
{
	Update_Version_Number ();

#if defined(BETACLIENT) || defined(FREEDEDICATEDSERVER) || defined(MULTIPLAYERDEMO)
	Get_Dlg_Item(IDC_MENU_START_SP_GAME_BUTTON)->Enable(false);
	Get_Dlg_Item(IDC_MENU_START_PRACTICE_GAME_BUTTON)->Enable(false);
#endif

#ifndef BETACLIENT
	if (Get_Dlg_Item (IDC_BETA_TEST_TEXT) != nullptr) {
		Get_Dlg_Item (IDC_BETA_TEST_TEXT)->Show (false);
	}
#endif

	ImageCtrlClass *image_ctrl = (ImageCtrlClass *)Get_Dlg_Item (IDC_IMAGE);
	if (image_ctrl != nullptr) {
		image_ctrl->Set_Texture ("ESRB_RATING.TGA");
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Get_Transition_In
//
////////////////////////////////////////////////////////////////
DialogTransitionClass *
MainMenuDialogClass::Get_Transition_In (DialogBaseClass *prev_dlg)
{
	MainMenuTransitionClass *transition = nullptr;

	//
	//	Add the transition model to the scene
	//
	if (TitleTransModel != nullptr && TitleTransModel->Peek_Scene () == nullptr) {
		if (MenuBackDropClass *backdrop = Get_BackDrop ()) {
			backdrop->Peek_Scene ()->Add_Render_Object (TitleTransModel);
		}
	}

	//
	//	Add the logo to the screen
	//
	if (LogoModel != nullptr && LogoModel->Peek_Scene () == nullptr) {
		if (MenuBackDropClass *backdrop = Get_BackDrop ()) {
			backdrop->Peek_Scene ()->Add_Render_Object (LogoModel);
		}
	}

	//
	//	We only want to transition between menu dialogs
	//
	if (prev_dlg == nullptr ||
			(prev_dlg != QuitVerificationDialogClass::Get_Instance () &&
			 prev_dlg != DlgWOLWait::Get_Instance ()))
	{
		transition = new MainMenuTransitionClass;
		transition->Set_Model (TitleTransModel);
		if (MenuBackDropClass *backdrop = Get_BackDrop ()) {
			transition->Set_Camera (backdrop->Peek_Camera ());
		}
		transition->Set_Type (DialogTransitionClass::SCREEN_IN);
		transition->Set_Dialogs (this, prev_dlg);

		//
		//	Don't do the transition if something is missing
		//
		if (transition->Is_Valid () == false) {
			REF_PTR_RELEASE (transition);
		}
	}

	return transition;
}


////////////////////////////////////////////////////////////////
//
//	Get_Transition_Out
//
////////////////////////////////////////////////////////////////
DialogTransitionClass *
MainMenuDialogClass::Get_Transition_Out (DialogBaseClass *next_dlg)
{
	MainMenuTransitionClass *transition = nullptr;

	//
	//	We only want to transition between menu dialogs
	//
	if (	IsStartingPractice == false &&
			(next_dlg == nullptr || next_dlg->As_MenuDialogClass () != nullptr))
	{
		transition = new MainMenuTransitionClass;
		transition->Set_Model (TitleTransModel);
		if (MenuBackDropClass *backdrop = Get_BackDrop ()) {
			transition->Set_Camera (backdrop->Peek_Camera ());
		}
		transition->Set_Type (DialogTransitionClass::SCREEN_OUT);
		transition->Set_Dialogs (this, next_dlg);

		//
		//	Don't do the transition if something is missing
		//
		if (transition->Is_Valid () == false) {
			REF_PTR_RELEASE (transition);
		}
	}

	return transition;
}


////////////////////////////////////////////////////////////////
//
//	Choose_Skirmish_Map
//
////////////////////////////////////////////////////////////////
StringClass
MainMenuDialogClass::Choose_Skirmish_Map (void)
{
	DynamicVectorClass<StringClass>	map_list;
	WIN32_FIND_DATAA find_info	= { 0 };
	BOOL keep_going				= true;
	HANDLE file_find				= nullptr;
	StringClass file_filter;

	//
	// Look for any skirmish maps.
	//
	file_filter.Format("data/skirmish*.mix");
	keep_going = true;
	for (file_find = ::FindFirstFileA (file_filter, &find_info);
		 (file_find != INVALID_HANDLE_VALUE) && keep_going;
		  keep_going = ::FindNextFileA (file_find, &find_info))
	{
		map_list.Add (find_info.cFileName);
	}

	if (file_find != INVALID_HANDLE_VALUE) {
		::FindClose (file_find);
	}

	if (map_list.Count() == 0) {
		//
		// No skirmish maps found. Look for a C&C map.
		//
		//	See MPLanHostMapCycleOptionsTabClass::Build_Map_List
		file_filter.Format("data/%s*.mix", TTSettingsClass::MapPrefix.Peek_Buffer());
		keep_going = true;
		for (file_find = ::FindFirstFileA (file_filter, &find_info);
			 (file_find != INVALID_HANDLE_VALUE) && keep_going;
			  keep_going = ::FindNextFileA (file_find, &find_info))
		{
			map_list.Add (find_info.cFileName);
		}

		if (file_find != INVALID_HANDLE_VALUE) {
			::FindClose (file_find);
		}
	}

	StringClass mapname;
	if (map_list.Count() > 0) {
		int choice = rand() % map_list.Count();
		mapname = map_list[choice];
	}

	return mapname;
}


////////////////////////////////////////////////////////////////
//
//	Start_Procedural_Skirmish
//
////////////////////////////////////////////////////////////////
bool
MainMenuDialogClass::Start_Procedural_Skirmish (unsigned int seed, const char *profile_path, StringClass *error_out)
{
	if (GameInitMgrClass::Is_Game_In_Progress ()) {
		if (error_out != nullptr) *error_out = "a game is running; end it first";
		return false;
	}

	if (!ProceduralWorldManager::Request_World (seed, profile_path, error_out)) {
		return false;
	}

	//	A profile that generates its own terrain starts on no level file at all (D-002); a
	//	fit_to_level profile is laid over a stock skirmish map as before.
	StringClass mapname;
	if (ProceduralWorldManager::Is_Requested_Native ()) {
		mapname = ProceduralWorldManager::NATIVE_MAP_NAME;
	} else {
		mapname = Choose_Skirmish_Map ();
		if (mapname.Is_Empty ()) {
			ProceduralWorldManager::Clear_Request ();
			if (error_out != nullptr) *error_out = "no skirmish map found to host the world";
			return false;
		}
	}

	/*
	**	Q-030 option (b).  Armed only for a native request, because only a native request can
	**	leave the player with nothing: a fit-to-level world that is refused still has its host
	**	map underneath.  Read and cleared by On_Menu_Activate when the load comes back refused.
	*/
	_ProceduralNativeInFlight = ProceduralWorldManager::Is_Requested_Native ();

	const int SKIRMISH_LOAD_MENU_NUMBER	= 96;
	CampaignManager::Select_Backdrop_Number (SKIRMISH_LOAD_MENU_NUMBER);
	GameInitMgrClass::Initialize_Skirmish ();

	WWASSERT (The_Game () != nullptr);
	The_Game ()->Set_Map_Cycle (0, mapname);

	GameInitMgrClass::Start_Game (mapname, -1, 0);
	return true;
}


////////////////////////////////////////////////////////////////
//
//	On_Command
//
////////////////////////////////////////////////////////////////
void
MainMenuDialogClass::On_Command (int ctrl_id, int message_id, unsigned int param)
{
	bool allow_default = true;

	switch (ctrl_id)
	{
		case IDCANCEL:
			ctrl_id = IDC_MENU_QUIT_BUTTON;
			break;

		case IDC_MENU_START_PRACTICE_GAME_BUTTON:
		{
			StringClass mapname = Choose_Skirmish_Map();

			if (!mapname.Is_Empty()) {
				IsStartingPractice = true;
				const int SKIRMISH_LOAD_MENU_NUMBER	= 96;
				CampaignManager::Select_Backdrop_Number(SKIRMISH_LOAD_MENU_NUMBER);
				GameInitMgrClass::Initialize_Skirmish();

				//
				// We will cycle on the same map until they get tired of practicing.
				//
				WWASSERT(The_Game() != nullptr);
				The_Game()->Set_Map_Cycle(0, mapname);

				GameInitMgrClass::Start_Game(mapname, -1, 0);
				IsStartingPractice = false;
			}
			break;
		}

		case IDC_MENU_START_PROCEDURAL_GAME_BUTTON:
		{
			/*
			**	D-002: the button launches a fully native generated world -- no stock map
			**	underneath; terrain, bases, spawns and navigation all come from the generator.
			**	A seed of zero asks the manager to choose one and log it, so a world worth
			**	revisiting can be asked for again by number.
			*/
			IsStartingPractice = true;
			StringClass world_error;
			bool started = Start_Procedural_Skirmish(0,ProceduralWorldManager::NATIVE_PROFILE_PATH,&world_error);
			IsStartingPractice = false;

			if (!started) {
				//	Refused before anything loads, which is the point of validating the profile
				//	at request time: the player finds out now rather than after a level load.
				WideStringClass message;
				message.Convert_From(world_error.Peek_Buffer());
				DlgMsgBox::DoDialog(TRANSLATE(IDS_MENU_TEXT513),message);
				allow_default = false;
			}
			break;
		}

		case IDC_MENU_MP_LAN_GAME_BUTTON:

			//
			// Clear any gamespyadmin flags
			//
			cGameSpyAdmin::Reset();

			if (cNicEnum::Get_Num_Nics() > 0) {
				GameInitMgrClass::Initialize_LAN ();
			} else {
				DlgMsgBox::DoDialog(
					TRANSLATE(IDS_MP_UNABLE_INITIALIZE_LAN),
					TRANSLATE(IDS_MP_NO_LAN_IP_ADDRESSES_FOUND));
				allow_default = false;
			}
			break;

		case IDC_MENU_MP_INTERNET_GAME_BUTTON:
			START_DIALOG (InternetMainDialogClass);
			allow_default = false;
			break;

		default:
			break;
	}

	if (allow_default) {
		MenuDialogClass::On_Command (ctrl_id, message_id, param);
	}
	return ;
}


////////////////////////////////////////////////////////////////
//
//	Display
//
////////////////////////////////////////////////////////////////
void
MainMenuDialogClass::Display (void)
{
	//
	//	Create the dialog if necessary, otherwise simply bring it to the front
	//
	if (_TheInstance == nullptr) {

		//
		//	Create the dialog
		//
		MainMenuDialogClass *dialog = new MainMenuDialogClass;

		//
		//	Create the backdrop if necessary
		//
		if (Animated) {

			if (MenuBackDropClass *backdrop = dialog->Get_BackDrop ()) {
				if (backdrop->Peek_Model () == nullptr) {
					backdrop->Set_Model ("IF_BACK01");
					backdrop->Set_Animation ("IF_BACK01.IF_BACK01");

					/*RenderObjClass *model = WW3DAssetManager::Get_Instance ()->Create_Render_Obj ("IF_RENLOGO");
					if (model != nullptr) {
						backdrop->Peek_Scene ()->Add_Render_Object(model);
					}*/
				}
			}
		}

		//
		//	Start the dialog
		//
		dialog->Start_Dialog ();
		REF_PTR_RELEASE (dialog);

	} else {
		if (_TheInstance->Is_Active_Menu () == false) {
			DialogMgrClass::Rollback (_TheInstance);
		}
	}

	return ;
}


////////////////////////////////////////////////////////////////
//
//	Update_Version_Number
//
////////////////////////////////////////////////////////////////
void
MainMenuDialogClass::Update_Version_Number (void)
{
	//
	// Version 1.0 by default
	//
	unsigned int version_major = 1;
	unsigned int version_minor = 0;
	Get_Version_Number(&version_major, &version_minor);

	//
	// Put the version string into the dialog
	//
	WideStringClass version_string;
	// Add build number temporarily. Will probably be removed for shipping.
	WideStringClass build_number(BuildInfoClass::Get_Build_Number_String(), true);
	WideStringClass build_initials(BuildInfoClass::Get_Builder_Initials(), true);
	WideStringClass build_date(BuildInfoClass::Get_Build_Date_String(), true);
	version_string.Format (U_CHAR("v%d.%.3d %s-%s %s"), (version_major >> 16), (version_major & 0xFFFF), build_initials.Peek_Buffer(), build_number.Peek_Buffer(), build_date.Peek_Buffer());
	Set_Dlg_Item_Text (IDC_VERSION_STATIC, version_string);
}
























				//TRANSLATE_ME
				//const unichar_t * title	= U_CHAR("Unable to initialize LAN");
				//IDS_MP_UNABLE_INITIALIZE_LAN
				//const unichar_t * text	= U_CHAR("No LAN IP addresses found.");
				//IDS_MP_NO_LAN_IP_ADDRESSES_FOUND

				//DlgMsgBox::DoDialog(title, text);

/*
#ifdef MULTIPLAYERDEMO
			START_DIALOG (GameSpyMainDialogClass);
#else
			START_DIALOG (InternetMainDialogClass);
#endif
*/
