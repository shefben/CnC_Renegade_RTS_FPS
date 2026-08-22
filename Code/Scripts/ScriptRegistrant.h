/******************************************************************************
*
* FILE
*     ScriptRegistrant.h
*
* DESCRIPTION
*     Registers one built-in script class with the native script registry.
*
******************************************************************************/

#ifndef _SCRIPTREGISTRANT_H_
#define _SCRIPTREGISTRANT_H_

#include <cstdlib>
#include <cassert>
#include "nativescriptregistry.h"

class ScriptImpClass;

template<class T>
class	ScriptRegistrant : public ScriptFactoryClass
	{
	public:
		ScriptRegistrant(const char* name, const char* param,
				ScriptSourceEnum source = SCRIPT_SOURCE_STOCK, const char* alias = nullptr)
			: ScriptFactoryClass(name, param, source, alias)
			{}

		virtual ScriptClass* Create(void) override
			{
			T* script = new T;
			assert(script != nullptr);
			script->SetFactory(this);
			script->Register_Auto_Save_Variables();
			return script;
			}
	};

//
//	The script name is the class name.  A script whose registered name differs
//	from its class name, or that carries an alias or a non-stock provenance,
//	declares its ScriptRegistrant directly instead of using this.
//
#define REGISTER_SCRIPT(x, d) \
	class x; \
	ScriptRegistrant<x> _## x ##Registrant(#x, d);

#endif // _SCRIPTREGISTRANT_H_
