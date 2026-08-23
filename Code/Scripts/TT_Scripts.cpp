/******************************************************************************
*
* FILE
*     TT_Scripts.cpp
*
* DESCRIPTION
*     Scripts that put other scripts on things.  A level author uses these to
*     say "when this happens, start behaving like that" without having to
*     place the second script by hand -- on whoever walked in, on whoever
*     fired the shot, on every object of a preset, on every building.
*
*     Native port of the 4.8.4 library's jfwscr.cpp.  Each of them carries a
*     parameter list for the script it attaches, and a list cannot hold commas,
*     so the author picks a delimiter and names it in a `Delim` parameter.
*     Putting the commas back is `Get_Nested_Parameters` on the script base;
*     4.8.4 wrote that loop out sixteen times over a hand-allocated copy.
*
******************************************************************************/

#include "scripts.h"


/*JFW_Attatch_Script

  Attaches a script to whoever set this object off.  Which "set off" means is
  the Trigger parameter: 1 walked into it, 2 killed it, 3 sent it the custom
  named in `Custom`, 4 poked it.  With Destroy set, the object goes away once
  it has done its job.

  The registered name carries the 4.8.4 spelling.  It is a typo, but it is the
  name levels were saved against.

  Player_Type filters the first, second and fourth triggers but not the third:
  4.8.4 left the check out of the custom path, and a level that leans on a
  custom arriving from a script rather than a player would break if it were
  put back.
*/

DECLARE_SCRIPT_TT_NAMED(JFW_Attach_Script, "JFW_Attatch_Script",
	"Trigger:int,Player_Type:int,Script_Name:string,Script_Params:string,"
	"Delim:string,Destroy:int,Custom:int")
{
	enum
	{
		TRIGGER_ENTERED	= 1,
		TRIGGER_KILLED	= 2,
		TRIGGER_CUSTOM	= 3,
		TRIGGER_POKED	= 4
	};

	void Entered(GameObject* obj, GameObject* enterer) override
	{
		if (Get_Int_Parameter("Trigger") != TRIGGER_ENTERED) {
			return;
		}

		if (!Is_Player_Type(enterer, Get_Int_Parameter("Player_Type"))) {
			return;
		}

		Fire(obj, enterer);
	}

	void Killed(GameObject* obj, GameObject* killer) override
	{
		if (Get_Int_Parameter("Trigger") != TRIGGER_KILLED) {
			return;
		}

		Fire(obj, killer);
	}

	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* sender) override
	{
		if (Get_Int_Parameter("Trigger") != TRIGGER_CUSTOM) {
			return;
		}

		if (type != Get_Int_Parameter("Custom")) {
			return;
		}

		Fire(obj, sender);
	}

	void Poked(GameObject* obj, GameObject* poker) override
	{
		if (Get_Int_Parameter("Trigger") != TRIGGER_POKED) {
			return;
		}

		if (!Is_Player_Type(poker, Get_Int_Parameter("Player_Type"))) {
			return;
		}

		Fire(obj, poker);
	}

	void Fire(GameObject* obj, GameObject* target)
	{
		if (target != nullptr) {
			StringClass params;
			Get_Nested_Parameters("Script_Params", "Delim", params);
			ScriptEngine::Attach_Script(target, Get_Parameter("Script_Name"), params.Peek_Buffer());
		}

		if (Get_Int_Parameter("Destroy") == 1) {
			ScriptEngine::Destroy_Object(obj);
		}
	}
};


/*JFW_Attach_Script_Custom

  On the message, attaches the script to the object this one is on.
*/

DECLARE_SCRIPT_TT(JFW_Attach_Script_Custom, "Script:string,Params:string,Delim:string,Message:int")
{
	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type != Get_Int_Parameter("Message")) {
			return;
		}

		StringClass params;
		Get_Nested_Parameters("Params", "Delim", params);
		ScriptEngine::Attach_Script(obj, Get_Parameter("Script"), params.Peek_Buffer());
	}
};


/*JFW_Attach_Script_Once_Custom

  The same, but the object does not collect a second copy if the message
  arrives twice.
*/

DECLARE_SCRIPT_TT(JFW_Attach_Script_Once_Custom, "Script:string,Params:string,Delim:string,Message:int")
{
	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type != Get_Int_Parameter("Message")) {
			return;
		}

		StringClass params;
		Get_Nested_Parameters("Params", "Delim", params);
		ScriptEngine::Attach_Script_Once(obj, Get_Parameter("Script"), params.Peek_Buffer());
	}
};


/*JFW_Attach_Script_Custom_Until_Custom

  One message puts the script on, another takes it off again.
*/

DECLARE_SCRIPT_TT(JFW_Attach_Script_Custom_Until_Custom,
	"Script:string,Params:string,Delim:string,AttachMessage:int,RemoveMessage:int")
{
	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type == Get_Int_Parameter("AttachMessage")) {
			StringClass params;
			Get_Nested_Parameters("Params", "Delim", params);
			ScriptEngine::Attach_Script(obj, Get_Parameter("Script"), params.Peek_Buffer());

		} else if (type == Get_Int_Parameter("RemoveMessage")) {
			ScriptEngine::Remove_Script(obj, Get_Parameter("Script"));
		}
	}
};


/*JFW_Attach_Script_Sender

  On the message, attaches the script to whoever sent it rather than to the
  object this one is on.
*/

DECLARE_SCRIPT_TT(JFW_Attach_Script_Sender, "Script:string,Params:string,Delim:string,Message:int")
{
	void Custom(GameObject* /*obj*/, int type, intptr_t /*param*/, GameObject* sender) override
	{
		if (type != Get_Int_Parameter("Message") || sender == nullptr) {
			return;
		}

		StringClass params;
		Get_Nested_Parameters("Params", "Delim", params);
		ScriptEngine::Attach_Script(sender, Get_Parameter("Script"), params.Peek_Buffer());
	}
};


/*JFW_Attach_Script_Collector

  Goes on a powerup.  Whoever picks it up gets the script.
*/

DECLARE_SCRIPT_TT(JFW_Attach_Script_Collector, "Script:string,Params:string,Delim:string")
{
	void Custom(GameObject* /*obj*/, int type, intptr_t /*param*/, GameObject* sender) override
	{
		if (type != CUSTOM_EVENT_POWERUP_GRANTED || sender == nullptr) {
			return;
		}

		StringClass params;
		Get_Nested_Parameters("Params", "Delim", params);
		ScriptEngine::Attach_Script(sender, Get_Parameter("Script"), params.Peek_Buffer());
	}
};


/*JFW_Attach_Script_Preset_Custom

  On the message, attaches the script to every object of a preset on a side.
*/

DECLARE_SCRIPT_TT(JFW_Attach_Script_Preset_Custom,
	"Script:string,Params:string,Delim:string,Message:int,Preset:string,Player_Type:int")
{
	void Custom(GameObject* /*obj*/, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type != Get_Int_Parameter("Message")) {
			return;
		}

		StringClass params;
		Get_Nested_Parameters("Params", "Delim", params);
		ScriptEngine::Attach_Script_Preset(Get_Parameter("Script"), params.Peek_Buffer(),
				Get_Parameter("Preset"), Get_Int_Parameter("Player_Type"), false);
	}
};


/*JFW_Attach_Script_Preset_Once_Custom

  The same, skipping anything that already has it.
*/

DECLARE_SCRIPT_TT(JFW_Attach_Script_Preset_Once_Custom,
	"Script:string,Params:string,Delim:string,Message:int,Preset:string,Player_Type:int")
{
	void Custom(GameObject* /*obj*/, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type != Get_Int_Parameter("Message")) {
			return;
		}

		StringClass params;
		Get_Nested_Parameters("Params", "Delim", params);
		ScriptEngine::Attach_Script_Preset(Get_Parameter("Script"), params.Peek_Buffer(),
				Get_Parameter("Preset"), Get_Int_Parameter("Player_Type"), true);
	}
};


/*JFW_Attach_Script_Preset_Startup

  The same, once, as the level opens, and then this script goes away.
*/

DECLARE_SCRIPT_TT(JFW_Attach_Script_Preset_Startup,
	"Script:string,Params:string,Delim:string,Preset:string,Player_Type:int")
{
	void Created(GameObject* /*obj*/) override
	{
		StringClass params;
		Get_Nested_Parameters("Params", "Delim", params);
		ScriptEngine::Attach_Script_Preset(Get_Parameter("Script"), params.Peek_Buffer(),
				Get_Parameter("Preset"), Get_Int_Parameter("Player_Type"), false);
		Destroy_Script();
	}
};


/*JFW_Attach_Script_Type_Custom

  On the message, attaches the script to every object of a class on a side.
  Type is the definition's class id, not its preset.
*/

DECLARE_SCRIPT_TT(JFW_Attach_Script_Type_Custom,
	"Script:string,Params:string,Delim:string,Message:int,Type:int,Player_Type:int")
{
	void Custom(GameObject* /*obj*/, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type != Get_Int_Parameter("Message")) {
			return;
		}

		StringClass params;
		Get_Nested_Parameters("Params", "Delim", params);
		ScriptEngine::Attach_Script_Type(Get_Parameter("Script"), params.Peek_Buffer(),
				(unsigned long)Get_Int_Parameter("Type"), Get_Int_Parameter("Player_Type"), false);
	}
};


/*JFW_Attach_Script_Type_Once_Custom

  The same, skipping anything that already has it.
*/

DECLARE_SCRIPT_TT(JFW_Attach_Script_Type_Once_Custom,
	"Script:string,Params:string,Delim:string,Message:int,Type:int,Player_Type:int")
{
	void Custom(GameObject* /*obj*/, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type != Get_Int_Parameter("Message")) {
			return;
		}

		StringClass params;
		Get_Nested_Parameters("Params", "Delim", params);
		ScriptEngine::Attach_Script_Type(Get_Parameter("Script"), params.Peek_Buffer(),
				(unsigned long)Get_Int_Parameter("Type"), Get_Int_Parameter("Player_Type"), true);
	}
};


/*JFW_Attach_Script_Type_Startup

  The same, once, as the level opens.
*/

DECLARE_SCRIPT_TT(JFW_Attach_Script_Type_Startup,
	"Script:string,Params:string,Delim:string,Type:int,Player_Type:int")
{
	void Created(GameObject* /*obj*/) override
	{
		StringClass params;
		Get_Nested_Parameters("Params", "Delim", params);
		ScriptEngine::Attach_Script_Type(Get_Parameter("Script"), params.Peek_Buffer(),
				(unsigned long)Get_Int_Parameter("Type"), Get_Int_Parameter("Player_Type"), false);
		Destroy_Script();
	}
};


/*JFW_Attach_Script_Building_Startup

  Puts the script on every building on a side as the level opens.
*/

DECLARE_SCRIPT_TT(JFW_Attach_Script_Building_Startup,
	"Script:string,Params:string,Delim:string,Player_Type:int")
{
	void Created(GameObject* /*obj*/) override
	{
		StringClass params;
		Get_Nested_Parameters("Params", "Delim", params);
		ScriptEngine::Attach_Script_Building(Get_Parameter("Script"), params.Peek_Buffer(),
				Get_Int_Parameter("Player_Type"));
		Destroy_Script();
	}
};


/*JFW_Remove_Script_Custom

  On the message, takes the script off the object this one is on.
*/

DECLARE_SCRIPT_TT(JFW_Remove_Script_Custom, "Script:string,Message:int")
{
	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type == Get_Int_Parameter("Message")) {
			ScriptEngine::Remove_Script(obj, Get_Parameter("Script"));
		}
	}
};


/*JFW_Remove_All_Scripts_Custom

  On the message, takes every script off, including this one.
*/

DECLARE_SCRIPT_TT(JFW_Remove_All_Scripts_Custom, "Message:int")
{
	void Custom(GameObject* obj, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type == Get_Int_Parameter("Message")) {
			ScriptEngine::Remove_All_Scripts(obj);
		}
	}
};


/*JFW_Remove_Script_Death

  Takes the script off as the object dies, and then goes away itself.
*/

DECLARE_SCRIPT_TT(JFW_Remove_Script_Death, "Script:string")
{
	void Killed(GameObject* obj, GameObject* /*killer*/) override
	{
		ScriptEngine::Remove_Script(obj, Get_Parameter("Script"));
		Destroy_Script();
	}
};


/*JFW_Remove_Script_Preset_Custom

  On the message, takes the script off every object of a preset on a side.
*/

DECLARE_SCRIPT_TT(JFW_Remove_Script_Preset_Custom, "Script:string,Message:int,Preset:string,Player_Type:int")
{
	void Custom(GameObject* /*obj*/, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type == Get_Int_Parameter("Message")) {
			ScriptEngine::Remove_Script_Preset(Get_Parameter("Script"), Get_Parameter("Preset"),
					Get_Int_Parameter("Player_Type"));
		}
	}
};


/*JFW_Remove_Script_Type_Custom

  On the message, takes the script off every object of a class on a side.
*/

DECLARE_SCRIPT_TT(JFW_Remove_Script_Type_Custom, "Script:string,Message:int,Type:int,Player_Type:int")
{
	void Custom(GameObject* /*obj*/, int type, intptr_t /*param*/, GameObject* /*sender*/) override
	{
		if (type == Get_Int_Parameter("Message")) {
			ScriptEngine::Remove_Script_Type(Get_Parameter("Script"),
					(unsigned long)Get_Int_Parameter("Type"), Get_Int_Parameter("Player_Type"));
		}
	}
};
